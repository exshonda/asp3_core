/*
 *		トレースログの構造化ログ出力（asp3_core新規）
 *
 *  トレースログ1件を，行指向の構造化ログフォーマット
 *
 *      T=<tick_us>,EV=<event>[,<key>=<val>]*
 *
 *  で低レベル文字出力（target_fput_log）へ逐次出力する（AGENTS.md §8）．
 *  出力は scripts/parse_slog.py でパースし，scripts/check_events.py で
 *  期待イベント列（test/porting/expected/）と照合する．
 *
 *  出力イベント：
 *      TSK_STAT   タスク状態変化（ID, STAT）
 *      DSP_ENTER  ディスパッチ元タスク（ID）
 *      TSK_RUN    ディスパッチ先タスク＝実行開始（ID）
 *      INH_ENTER/INH_LEAVE    割込みハンドラ（INHNO）
 *      ISR_ENTER/ISR_LEAVE    割込みサービスルーチン（INTNO）
 *      CYC_ENTER/CYC_LEAVE    周期ハンドラ（ID）
 *      ALM_ENTER/ALM_LEAVE    アラームハンドラ（ID）
 *      EXC_ENTER/EXC_LEAVE    CPU例外ハンドラ（EXCNO）
 *      SVC_ENTER  サービスコール入口（API, A1〜A5＝引数）
 *      SVC_LEAVE  サービスコール出口（API, RET, V1/V2＝出力値）
 *      ERR        サービスコールのエラー復帰（API, CODE）．SVC_LEAVEの
 *                 RETがエラーコード範囲（-99〜-1）のとき追加出力する
 *
 *  システムログ（LOG_TYPE_COMMENT/ASSERT）はコンソールへ通常出力される
 *  ため，ここでは出力しない．
 */

#include "kernel_impl.h"
#include "task.h"
#include "cyclic.h"
#include "alarm.h"
#include "target_syssvc.h"

/*
 *  出力先の文字出力関数（ターゲット依存で変更する場合は
 *  TRACE_SLOG_PUTCに文字出力関数を定義する）
 */
#ifndef TRACE_SLOG_PUTC
#define TRACE_SLOG_PUTC		target_fput_log
#endif /* TRACE_SLOG_PUTC */

/*
 *  サービスコールの機能コード→名称・引数個数の対応表
 *
 *  enter_nargs：LOG_*_ENTERでlogparに記録される引数の個数
 *  leave_nargs：LOG_*_LEAVEでlogparに記録される値の個数（先頭は戻り値）
 */
typedef struct {
	int_t		tfn;			/* 機能コード（TFN_*） */
	const char	*name;			/* サービスコール名 */
	uint_t		enter_nargs;
	uint_t		leave_nargs;
} SVC_NAME_ENTRY;

static const SVC_NAME_ENTRY svc_name_table[] = {
	/* タスク管理機能 */
	{ TFN_ACT_TSK,	"act_tsk",	1, 1 },
	{ TFN_GET_TST,	"get_tst",	2, 2 },
	{ TFN_CAN_ACT,	"can_act",	1, 1 },
	{ TFN_CHG_PRI,	"chg_pri",	2, 1 },
	{ TFN_GET_PRI,	"get_pri",	2, 2 },
	{ TFN_GET_INF,	"get_inf",	1, 2 },
	{ TFN_REF_TSK,	"ref_tsk",	2, 2 },
	/* タスク付属同期機能 */
	{ TFN_SLP_TSK,	"slp_tsk",	0, 1 },
	{ TFN_TSLP_TSK,	"tslp_tsk",	1, 1 },
	{ TFN_WUP_TSK,	"wup_tsk",	1, 1 },
	{ TFN_CAN_WUP,	"can_wup",	1, 1 },
	{ TFN_REL_WAI,	"rel_wai",	1, 1 },
	{ TFN_SUS_TSK,	"sus_tsk",	1, 1 },
	{ TFN_RSM_TSK,	"rsm_tsk",	1, 1 },
	{ TFN_DLY_TSK,	"dly_tsk",	1, 1 },
	/* タスク終了機能 */
	{ TFN_EXT_TSK,	"ext_tsk",	0, 1 },
	{ TFN_RAS_TER,	"ras_ter",	1, 1 },
	{ TFN_DIS_TER,	"dis_ter",	0, 1 },
	{ TFN_ENA_TER,	"ena_ter",	0, 1 },
	{ TFN_SNS_TER,	"sns_ter",	0, 1 },
	{ TFN_TER_TSK,	"ter_tsk",	1, 1 },
	/* セマフォ機能 */
	{ TFN_SIG_SEM,	"sig_sem",	1, 1 },
	{ TFN_WAI_SEM,	"wai_sem",	1, 1 },
	{ TFN_POL_SEM,	"pol_sem",	1, 1 },
	{ TFN_TWAI_SEM,	"twai_sem",	2, 1 },
	{ TFN_INI_SEM,	"ini_sem",	1, 1 },
	{ TFN_REF_SEM,	"ref_sem",	2, 2 },
	/* イベントフラグ機能 */
	{ TFN_SET_FLG,	"set_flg",	2, 1 },
	{ TFN_CLR_FLG,	"clr_flg",	2, 1 },
	{ TFN_WAI_FLG,	"wai_flg",	4, 2 },
	{ TFN_POL_FLG,	"pol_flg",	4, 2 },
	{ TFN_TWAI_FLG,	"twai_flg",	5, 2 },
	{ TFN_INI_FLG,	"ini_flg",	1, 1 },
	{ TFN_REF_FLG,	"ref_flg",	2, 2 },
	/* データキュー機能 */
	{ TFN_SND_DTQ,	"snd_dtq",	2, 1 },
	{ TFN_PSND_DTQ,	"psnd_dtq",	2, 1 },
	{ TFN_TSND_DTQ,	"tsnd_dtq",	3, 1 },
	{ TFN_FSND_DTQ,	"fsnd_dtq",	2, 1 },
	{ TFN_RCV_DTQ,	"rcv_dtq",	2, 2 },
	{ TFN_PRCV_DTQ,	"prcv_dtq",	2, 2 },
	{ TFN_TRCV_DTQ,	"trcv_dtq",	3, 2 },
	{ TFN_INI_DTQ,	"ini_dtq",	1, 1 },
	{ TFN_REF_DTQ,	"ref_dtq",	2, 2 },
	/* 優先度データキュー機能 */
	{ TFN_SND_PDQ,	"snd_pdq",	3, 1 },
	{ TFN_PSND_PDQ,	"psnd_pdq",	3, 1 },
	{ TFN_TSND_PDQ,	"tsnd_pdq",	4, 1 },
	{ TFN_RCV_PDQ,	"rcv_pdq",	3, 3 },
	{ TFN_PRCV_PDQ,	"prcv_pdq",	3, 3 },
	{ TFN_TRCV_PDQ,	"trcv_pdq",	4, 3 },
	{ TFN_INI_PDQ,	"ini_pdq",	1, 1 },
	{ TFN_REF_PDQ,	"ref_pdq",	2, 2 },
	/* ミューテックス機能 */
	{ TFN_LOC_MTX,	"loc_mtx",	1, 1 },
	{ TFN_PLOC_MTX,	"ploc_mtx",	1, 1 },
	{ TFN_TLOC_MTX,	"tloc_mtx",	2, 1 },
	{ TFN_UNL_MTX,	"unl_mtx",	1, 1 },
	{ TFN_INI_MTX,	"ini_mtx",	1, 1 },
	{ TFN_REF_MTX,	"ref_mtx",	2, 2 },
	/* 固定長メモリプール機能 */
	{ TFN_GET_MPF,	"get_mpf",	2, 2 },
	{ TFN_PGET_MPF,	"pget_mpf",	2, 2 },
	{ TFN_TGET_MPF,	"tget_mpf",	3, 2 },
	{ TFN_REL_MPF,	"rel_mpf",	2, 1 },
	{ TFN_INI_MPF,	"ini_mpf",	1, 1 },
	{ TFN_REF_MPF,	"ref_mpf",	2, 2 },
	/* システム時刻管理機能 */
	{ TFN_SET_TIM,	"set_tim",	1, 1 },
	{ TFN_GET_TIM,	"get_tim",	1, 2 },
	{ TFN_ADJ_TIM,	"adj_tim",	1, 1 },
	{ TFN_SET_DFT,	"set_dft",	1, 1 },
	{ TFN_FCH_HRT,	"fch_hrt",	0, 1 },
	/* 周期ハンドラ機能 */
	{ TFN_STA_CYC,	"sta_cyc",	1, 1 },
	{ TFN_STP_CYC,	"stp_cyc",	1, 1 },
	{ TFN_REF_CYC,	"ref_cyc",	2, 2 },
	/* アラームハンドラ機能 */
	{ TFN_STA_ALM,	"sta_alm",	2, 1 },
	{ TFN_STP_ALM,	"stp_alm",	1, 1 },
	{ TFN_REF_ALM,	"ref_alm",	2, 2 },
	/* システム状態管理機能 */
	{ TFN_ROT_RDQ,	"rot_rdq",	1, 1 },
	{ TFN_GET_TID,	"get_tid",	1, 2 },
	{ TFN_GET_LOD,	"get_lod",	2, 2 },
	{ TFN_GET_NTH,	"get_nth",	3, 2 },
	{ TFN_LOC_CPU,	"loc_cpu",	0, 1 },
	{ TFN_UNL_CPU,	"unl_cpu",	0, 1 },
	{ TFN_DIS_DSP,	"dis_dsp",	0, 1 },
	{ TFN_ENA_DSP,	"ena_dsp",	0, 1 },
	{ TFN_SNS_CTX,	"sns_ctx",	0, 1 },
	{ TFN_SNS_LOC,	"sns_loc",	0, 1 },
	{ TFN_SNS_DSP,	"sns_dsp",	0, 1 },
	{ TFN_SNS_DPN,	"sns_dpn",	0, 1 },
	{ TFN_SNS_KER,	"sns_ker",	0, 1 },
	/* 割込み管理機能 */
	{ TFN_DIS_INT,	"dis_int",	1, 1 },
	{ TFN_ENA_INT,	"ena_int",	1, 1 },
	{ TFN_CLR_INT,	"clr_int",	1, 1 },
	{ TFN_RAS_INT,	"ras_int",	1, 1 },
	{ TFN_PRB_INT,	"prb_int",	1, 1 },
	{ TFN_CHG_IPM,	"chg_ipm",	1, 1 },
	{ TFN_GET_IPM,	"get_ipm",	1, 2 },
	/* CPU例外管理機能 */
	{ TFN_XSNS_DPN,	"xsns_dpn",	1, 1 },
	/* カーネルの終了処理 */
	{ TFN_EXT_KER,	"ext_ker",	0, 0 },
};

#define TNUM_SVC_NAME	(sizeof(svc_name_table) / sizeof(SVC_NAME_ENTRY))

/*
 *  エラーコードとみなす戻り値の範囲（E_SYS〜E_BOVR：docs/errors.md）
 */
#define IS_ERCD(val)	(-99 <= (val) && (val) <= -1)

/*
 *  出力用の小ヘルパ（syslog_printfは1呼出しの変換数に上限があるため，
 *  本ファイル専用の文字出力ヘルパを用いる）
 */
static void
slog_putstr(const char *str)
{
	char	c;

	while ((c = *str++) != '\0') {
		TRACE_SLOG_PUTC(c);
	}
}

static void
slog_putudec(ULOGPAR val)
{
	char	buf[24];
	uint_t	i = 0U;

	do {
		buf[i++] = (char)('0' + (val % 10U));
		val /= 10U;
	} while (val != 0U && i < sizeof(buf));
	while (i > 0U) {
		TRACE_SLOG_PUTC(buf[--i]);
	}
}

static void
slog_putdec(LOGPAR val)
{
	if (val < 0) {
		TRACE_SLOG_PUTC('-');
		slog_putudec((ULOGPAR)(-val));
	}
	else {
		slog_putudec((ULOGPAR) val);
	}
}

/*
 *  カーネル情報の取出し
 */
static ID
slog_tskid(LOGPAR info)
{
	TCB		*p_tcb = (TCB *) info;

	return((p_tcb == NULL) ? 0 : TSKID(p_tcb));
}

static const char *
slog_tskstat(LOGPAR info)
{
	uint_t	tstat = (uint_t) info;

	if (TSTAT_DORMANT(tstat)) {
		return("DORMANT");
	}
	else if (TSTAT_SUSPENDED(tstat)) {
		return(TSTAT_WAITING(tstat) ? "WAITING-SUSPENDED" : "SUSPENDED");
	}
	else if (TSTAT_WAITING(tstat)) {
		return("WAITING");
	}
	else {
		return("RUNNABLE");
	}
}

static ID
slog_cycid(LOGPAR info)
{
	return((ID)((((CYCCB *) info)->p_cycinib - cycinib_table) + TMIN_CYCID));
}

static ID
slog_almid(LOGPAR info)
{
	return((ID)((((ALMCB *) info)->p_alminib - alminib_table) + TMIN_ALMID));
}

/*
 *  行頭の共通部（T=<tick>,EV=<event>）の出力
 */
static void
slog_header(const TRACE *p_trace, const char *event)
{
	slog_putstr("T=");
	slog_putudec((ULOGPAR)(p_trace->logtim));
	slog_putstr(",EV=");
	slog_putstr(event);
}

/*
 *  ID等の数値1個を伴うイベントの出力
 */
static void
slog_event_1(const TRACE *p_trace, const char *event,
								const char *key, LOGPAR val)
{
	slog_header(p_trace, event);
	TRACE_SLOG_PUTC(',');
	slog_putstr(key);
	TRACE_SLOG_PUTC('=');
	slog_putdec(val);
	TRACE_SLOG_PUTC('\n');
}

/*
 *  サービスコールの機能コード→対応表エントリの検索
 */
static const SVC_NAME_ENTRY *
slog_svc_lookup(LOGPAR tfn)
{
	uint_t	i;

	for (i = 0U; i < TNUM_SVC_NAME; i++) {
		if (svc_name_table[i].tfn == (int_t) tfn) {
			return(&(svc_name_table[i]));
		}
	}
	return(NULL);
}

/*
 *  サービスコール入口の出力
 *      T=<tick>,EV=SVC_ENTER,API=<name>[,A1=..〜A5=..]
 */
static void
slog_svc_enter(const TRACE *p_trace)
{
	const SVC_NAME_ENTRY	*p_ent = slog_svc_lookup(p_trace->logpar[0]);
	static const char		*argkey[] = { "A1", "A2", "A3", "A4", "A5" };
	uint_t					i;

	slog_header(p_trace, "SVC_ENTER");
	slog_putstr(",API=");
	if (p_ent == NULL) {
		slog_putstr("tfn");
		slog_putdec(p_trace->logpar[0]);
		TRACE_SLOG_PUTC('\n');
		return;
	}
	slog_putstr(p_ent->name);
	for (i = 0U; i < p_ent->enter_nargs && i < TNUM_LOGPAR - 1; i++) {
		TRACE_SLOG_PUTC(',');
		slog_putstr(argkey[i]);
		TRACE_SLOG_PUTC('=');
		slog_putdec(p_trace->logpar[i + 1]);
	}
	TRACE_SLOG_PUTC('\n');
}

/*
 *  サービスコール出口の出力
 *      T=<tick>,EV=SVC_LEAVE,API=<name>[,RET=..][,V1=..,V2=..]
 *  戻り値がエラーコード範囲のときは，続けて
 *      T=<tick>,EV=ERR,API=<name>,CODE=<ercd>
 */
static void
slog_svc_leave(const TRACE *p_trace)
{
	const SVC_NAME_ENTRY	*p_ent = slog_svc_lookup(p_trace->logpar[0]);
	static const char		*valkey[] = { "V1", "V2" };
	uint_t					i;

	slog_header(p_trace, "SVC_LEAVE");
	slog_putstr(",API=");
	if (p_ent == NULL) {
		slog_putstr("tfn");
		slog_putdec(p_trace->logpar[0]);
		TRACE_SLOG_PUTC('\n');
		return;
	}
	slog_putstr(p_ent->name);
	if (p_ent->leave_nargs > 0U) {
		slog_putstr(",RET=");
		slog_putdec(p_trace->logpar[1]);
		for (i = 1U; i < p_ent->leave_nargs && i < TNUM_LOGPAR - 1
											&& i <= 2U; i++) {
			TRACE_SLOG_PUTC(',');
			slog_putstr(valkey[i - 1U]);
			TRACE_SLOG_PUTC('=');
			slog_putdec(p_trace->logpar[i + 1]);
		}
	}
	TRACE_SLOG_PUTC('\n');

	if (p_ent->leave_nargs > 0U && IS_ERCD(p_trace->logpar[1])) {
		slog_header(p_trace, "ERR");
		slog_putstr(",API=");
		slog_putstr(p_ent->name);
		slog_putstr(",CODE=");
		slog_putdec(p_trace->logpar[1]);
		TRACE_SLOG_PUTC('\n');
	}
}

/*
 *  トレースログ1件の構造化ログ出力
 *
 *  trace_wri_log（trace_log.c）から，トレースモードがTRACE_SLOGのとき
 *  に呼び出される（CPUロック相当の状態で呼ばれるため，出力はターゲット
 *  非依存の低レベル文字出力のみを用いる）．
 */
void
trace_slog_print(const TRACE *p_trace)
{
	switch (p_trace->logtype) {
	case LOG_TYPE_TSKSTAT:
		slog_header(p_trace, "TSK_STAT");
		slog_putstr(",ID=");
		slog_putdec(slog_tskid(p_trace->logpar[0]));
		slog_putstr(",STAT=");
		slog_putstr(slog_tskstat(p_trace->logpar[1]));
		TRACE_SLOG_PUTC('\n');
		break;
	case LOG_TYPE_DSP|LOG_ENTER:
		slog_event_1(p_trace, "DSP_ENTER", "ID", slog_tskid(p_trace->logpar[0]));
		break;
	case LOG_TYPE_DSP|LOG_LEAVE:
		slog_event_1(p_trace, "TSK_RUN", "ID", slog_tskid(p_trace->logpar[0]));
		break;
	case LOG_TYPE_INH|LOG_ENTER:
		slog_event_1(p_trace, "INH_ENTER", "INHNO", p_trace->logpar[0]);
		break;
	case LOG_TYPE_INH|LOG_LEAVE:
		slog_event_1(p_trace, "INH_LEAVE", "INHNO", p_trace->logpar[0]);
		break;
	case LOG_TYPE_ISR|LOG_ENTER:
		slog_event_1(p_trace, "ISR_ENTER", "INTNO", p_trace->logpar[0]);
		break;
	case LOG_TYPE_ISR|LOG_LEAVE:
		slog_event_1(p_trace, "ISR_LEAVE", "INTNO", p_trace->logpar[0]);
		break;
	case LOG_TYPE_CYC|LOG_ENTER:
		slog_event_1(p_trace, "CYC_ENTER", "ID", slog_cycid(p_trace->logpar[0]));
		break;
	case LOG_TYPE_CYC|LOG_LEAVE:
		slog_event_1(p_trace, "CYC_LEAVE", "ID", slog_cycid(p_trace->logpar[0]));
		break;
	case LOG_TYPE_ALM|LOG_ENTER:
		slog_event_1(p_trace, "ALM_ENTER", "ID", slog_almid(p_trace->logpar[0]));
		break;
	case LOG_TYPE_ALM|LOG_LEAVE:
		slog_event_1(p_trace, "ALM_LEAVE", "ID", slog_almid(p_trace->logpar[0]));
		break;
	case LOG_TYPE_EXC|LOG_ENTER:
		slog_event_1(p_trace, "EXC_ENTER", "EXCNO", p_trace->logpar[0]);
		break;
	case LOG_TYPE_EXC|LOG_LEAVE:
		slog_event_1(p_trace, "EXC_LEAVE", "EXCNO", p_trace->logpar[0]);
		break;
	case LOG_TYPE_SVC|LOG_ENTER:
		slog_svc_enter(p_trace);
		break;
	case LOG_TYPE_SVC|LOG_LEAVE:
		slog_svc_leave(p_trace);
		break;
	default:
		/* システムログ（COMMENT/ASSERT）等は出力しない */
		break;
	}
}
