/*
 *		移植検証テスト用 最小TAPフレームワーク（本体）
 *
 *  出力はsyslogを使用（テスト用サービス syssvc/test_svc.c と同じ経路．
 *  終了時は割込みロック状態で syslog_fls_log() により低レベル出力へ
 *  フラッシュしてから ext_ker() する）．
 */

#include <kernel.h>
#include <sil.h>
#include <t_syslog.h>
#include "syssvc/syslog.h"
#include "tap.h"

/*
 *  テストの進行状態
 */
static uint_t	tap_num_planned;	/* 計画した項目数 */
static uint_t	tap_num_run;		/* 実施した項目数 */
static uint_t	tap_num_passed;		/* 合格した項目数 */

/*
 *  テストプランの出力
 */
void
tap_plan(uint_t num)
{
	tap_num_planned = num;
	tap_num_run = 0U;
	tap_num_passed = 0U;
	syslog_1(LOG_NOTICE, "1..%d", num);
}

/*
 *  1項目の合否出力
 */
void
tap_ok(bool_t cond, const char *name)
{
	tap_num_run++;
	if (cond) {
		tap_num_passed++;
		syslog_2(LOG_NOTICE, "ok %d - %s", tap_num_run, name);
	}
	else {
		syslog_2(LOG_ERROR, "not ok %d - %s", tap_num_run, name);
	}
}

/*
 *  診断メッセージの出力
 */
void
tap_diag(const char *msg)
{
	syslog_1(LOG_NOTICE, "# %s", msg);
}

/*
 *  テスト終了処理
 *
 *  サマリ行（"# N/N passed"）が機械判定の対象．全ターゲット共通で
 *  この行のパースで合否を判定する（linuxではctestの出力照合に直結）．
 */
void
tap_done(void)
{
	SIL_PRE_LOC;

	syslog_2(LOG_NOTICE, "# %d/%d passed", tap_num_passed, tap_num_planned);

	SIL_LOC_INT();
	(void) syslog_fls_log();
	(void) ext_ker();

	/* ここへ来ることはないはず */
	SIL_UNL_INT();
}
