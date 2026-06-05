/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 * 
 *  Copyright (C) 2018-2025 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 * 
 *  上記著作権者は，以下の(1)〜(4)の条件を満たす場合に限り，本ソフトウェ
 *  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
 *  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 * 
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 * 
 *  $Id: sim_timer.c 1881 2026-05-18 03:52:07Z ertl-hiro $
 */

/*
 *		タイマドライバシミュレータ
 */

#include "kernel_impl.h"
#include "time_event.h"
#ifdef TOPPERS_SUPPORT_OVRHDR
#include "overrun.h"
#endif /* TOPPERS_SUPPORT_OVRHDR */
#include "target_timer.h"

/*
 *  テストプログラム向けのタイマドライバシミュレータのパラメータ
 */
#ifdef SIMTIM_TEST
#define SIMTIM_INIT_CURRENT		10
#define SIMTIM_OVERHEAD_HRTINT	10
#ifdef TOPPERS_SUPPORT_OVRHDR
#define SIMTIM_OVERHEAD_OVRINT	10
#endif /* TOPPERS_SUPPORT_OVRHDR */
#endif /* SIMTIM_TEST */

/*
 *  シミュレーション時刻のデータ型
 */
typedef uint64_t	SIMTIM;

/*
 *  タイマ割込みの発生時刻の設定状況
 */
typedef struct {
	bool_t		enable;				/* 発生時刻が設定されているか？ */
	SIMTIM		simtim;				/* 発生時刻 */
	void		(*raise)(void);		/* タイマ割込みの要求 */
} INT_EVENT;

/*
 *  現在のシミュレーション時刻
 */
static SIMTIM	current_simtim;

/*
 *  最初に発生するタイマ割込みの選択
 */
static void		select_event(void);

/*
 *  高分解能タイマ割込みの発生時刻
 */
static INT_EVENT	hrt_event;

/*
 *  高分解能タイマのカウント値の操作
 */
Inline SIMTIM
truncate_simtim(SIMTIM simtim)
{
	return(simtim / TSTEP_HRTCNT * TSTEP_HRTCNT);
}

Inline SIMTIM
roundup_simtim(SIMTIM simtim)
{
	return((simtim + TSTEP_HRTCNT - 1) / TSTEP_HRTCNT * TSTEP_HRTCNT);
}

/*
 *  高分解能タイマの現在のカウント値の読出し
 *
 *  この関数はシステムログへのログ情報の出力時に呼び出されるため，この
 *  関数内でsyslogやassertを使ってはならない（無限の再帰呼出しが起こ
 *  る）．
 */
HRTCNT
target_hrt_get_current(void)
{
#ifdef TCYC_HRTCNT
	return((HRTCNT)(truncate_simtim(current_simtim) % TCYC_HRTCNT));
#else /* TCYC_HRTCNT */
	return((HRTCNT) truncate_simtim(current_simtim));
#endif /* TCYC_HRTCNT */
}

/*
 *  高分解能タイマへの割込みタイミングの設定
 */
void
target_hrt_set_event(HRTCNT hrtcnt)
{
#ifdef SIMTIM_TEST
	hook_hrt_set_event(hrtcnt);
#endif /* SIMTIM_TEST */

	hrt_event.enable = true;
	hrt_event.simtim = roundup_simtim(current_simtim + hrtcnt);
	select_event();
}

/*
 *  高分解能タイマへの割込みタイミングのクリア
 */
#ifdef USE_64BIT_HRTCNT

void
target_hrt_clear_event(void)
{
#ifdef SIMTIM_TEST
	hook_hrt_clear_event();
#endif /* SIMTIM_TEST */

	hrt_event.enable = false;
	select_event();
}

#endif /* USE_64BIT_HRTCNT */

/*
 *  高分解能タイマ割込みの要求
 */
void
target_hrt_raise_event(void)
{
#ifdef SIMTIM_TEST
	hook_hrt_raise_event();
#endif /* SIMTIM_TEST */

	simtim_target_raise_hrt_int();
}

/*
 *  シミュレートされた高分解能タイマ割込みハンドラ
 */
void
target_hrt_handler(void)
{
#ifdef SIMTIM_OVERHEAD_HRTINT
	simtim_advance(SIMTIM_OVERHEAD_HRTINT);
#endif /* SIMTIM_OVERHEAD_HRTINT */
	signal_time();
}

#ifdef TOPPERS_SUPPORT_OVRHDR
/*
 *  オーバランタイマ割込みの発生時刻
 */
static INT_EVENT	ovr_event;

/*
 *  オーバランタイマの動作開始
 */
void
target_ovrtimer_start(PRCTIM ovrtim)
{
	if (ovrtim == 0) {
		ovr_event.enable = false;
		select_event();
		simtim_target_raise_ovr_int();
	}
	else {
		ovr_event.enable = true;
		ovr_event.simtim = current_simtim + ovrtim;
		select_event();
	}
}

/*
 *  オーバランタイマの停止
 *
 *  ここでオーバランタイマ割込み要求をクリアすると，割込み源の特定に失
 *  敗する（QEMUで確認．QEMUだけの問題か，実機にもある問題かは未確認）
 *  ため，クリアしない．
 */
PRCTIM
target_ovrtimer_stop(void)
{
	PRCTIM	ovrtim;

	if (ovr_event.simtim <= current_simtim) {
		ovrtim = 0U;
	}
	else {
		ovrtim = ovr_event.simtim - current_simtim;
	}
	ovr_event.enable = false;
	select_event();
	simtim_target_clear_ovr_int();
	return(ovrtim);
}

/*
 *  オーバランタイマの現在値の読出し
 */
PRCTIM
target_ovrtimer_get_current(void)
{
	if (ovr_event.simtim <= current_simtim) {
		return(0U);
	}
	else {
		return(ovr_event.simtim - current_simtim);
	}
}

/*
 *  シミュレートされたオーバランタイマ割込みハンドラ
 */
void
target_ovrtimer_handler(void)
{
#ifdef SIMTIM_OVERHEAD_OVRINT
	simtim_advance(SIMTIM_OVERHEAD_OVRINT);
#endif /* SIMTIM_OVERHEAD_OVRINT */
	call_ovrhdr();
}

#endif /* TOPPERS_SUPPORT_OVRHDR */

/*
 *  最初に発生するタイマ割込みの情報
 */
static INT_EVENT	*p_next_event;

/*
 *  タイマの起動処理
 */
void
target_timer_initialize(EXINF exinf)
{
#ifdef SIMTIM_INIT_CURRENT
	current_simtim = SIMTIM_INIT_CURRENT;
#else /* SIMTIM_INIT_CURRENT */
	current_simtim = 0U;
#endif /* SIMTIM_INIT_CURRENT */
	hrt_event.enable = false;
	hrt_event.raise = &simtim_target_raise_hrt_int;
#ifdef TOPPERS_SUPPORT_OVRHDR
	ovr_event.enable = false;
	ovr_event.raise = &simtim_target_raise_ovr_int;
#endif /* TOPPERS_SUPPORT_OVRHDR */
	p_next_event = NULL;
}

/*
 *  タイマの停止処理
 */
void
target_timer_terminate(EXINF exinf)
{
	hrt_event.enable = false;
#ifdef TOPPERS_SUPPORT_OVRHDR
	ovr_event.enable = false;
#endif /* TOPPERS_SUPPORT_OVRHDR */
	p_next_event = NULL;
}

/*
 *  次に発生するタイマ割込みの選択
 */
static void
select_event(void)
{
	if (hrt_event.enable) {
		p_next_event = &hrt_event;
	}
	else {
		p_next_event = NULL;
	}

#ifdef TOPPERS_SUPPORT_OVRHDR
	if (ovr_event.enable && (p_next_event == NULL
								|| ovr_event.simtim <= p_next_event->simtim)) {
		p_next_event = &ovr_event;
	}
#endif /* TOPPERS_SUPPORT_OVRHDR */
}

/*
 *  カーネルのアイドル処理
 */
void
target_custom_idle(void)
{
	lock_cpu();
	if (p_next_event != NULL) {
		current_simtim = p_next_event->simtim;
		p_next_event->enable = false;
		(*(p_next_event->raise))();
		select_event();
	}
	unlock_cpu();
}

/*
 *  シミュレーション時刻を進める
 */
void
simtim_advance(uint_t time)
{
	bool_t	locked;

	locked = sense_lock();
	if (!locked) {
		lock_cpu();
	}

	while (p_next_event != NULL
					&& p_next_event->simtim <= current_simtim + time) {
		/*
		 *  時刻をtime進めると，タイマ割込みの発生時刻を過ぎる場合
		 */
		if (current_simtim < p_next_event->simtim) {
			time -= (p_next_event->simtim - current_simtim);
			current_simtim = p_next_event->simtim;
		}
		p_next_event->enable = false;
		(*(p_next_event->raise))();
		select_event();

		/*
		 *  ここで割込みを受け付ける．
		 */
		if (!locked) {
			unlock_cpu();
			delay_for_interrupt();
			lock_cpu();
		}
	}	 
	current_simtim += time;

	if (!locked) {
		unlock_cpu();
	}
}
