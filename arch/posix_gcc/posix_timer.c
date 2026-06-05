/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2005-2024 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: posix_timer.c 1881 2026-05-18 03:52:07Z ertl-hiro $
 */

/*
 *		タイマドライバ（POSIX用）
 *
 *  POSIXのタイマ機能（clock_gettime，timer_create等）でタイマドライバ
 *  を実現する．
 */

#include "kernel_impl.h"
#include "target_timer.h"
#include <time.h>

/*
 *  タイマを停止させるための変数
 */
#ifdef TOPPERS_SUPPORT_OVRHDR
static const struct itimerspec	itimerspec_stop = {{ 0, 0 }, { 0, 0 }};
#endif /* TOPPERS_SUPPORT_OVRHDR */

/*
 *  SIGALRMシグナルのハンドラ（メインスレッドで実行）
 */
static void
sigalrm_handler(int sig, siginfo_t *p_siginfo, void *p_ctx)
{
	/*
	 *  メインスレッド以外がSIGALRMシグナルを受け取った場合は，メイン
	 *  スレッドに回送する．
	 */
	if (pthread_getspecific(thrcb_key) != NULL) {
		/*
		 *  問題点：以下の処理では，p_siginfo->si_value.sival_intがメ
		 *  インスレッドに回送されず，正しく動作しない．
		 */
		pthread_kill(main_thread, sig);
		return;
	}

	/*
	 *  割込みを要求する．
	 */
	raise_int_main((INTNO) p_siginfo->si_value.sival_int);
}

/*
 *  高分解能タイマドライバ
 *
 *  64ビットの高分解能タイマをシミュレートすることも可能であるが，シミュ
 *  レーション環境であることを考え，高分解能タイマのビット数は32ビット
 *  とし，絶対時刻の下位32ビットを現在のカウント値とする．
 */

/*
 *  高分解能タイマに用いるタイマのID
 */
static timer_t	hrt_timerid;

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
	struct timespec	tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return((HRTCNT)(tp.tv_sec * 1000000U + tp.tv_nsec / 1000U));
}

/*
 *  高分解能タイマへの割込みタイミングの設定
 */
void
target_hrt_set_event(HRTCNT hrtcnt)
{
	struct itimerspec	spec;

	spec.it_interval.tv_sec = 0;
	spec.it_interval.tv_nsec = 0;
	spec.it_value.tv_sec = hrtcnt / 1000000U;
	spec.it_value.tv_nsec = hrtcnt % 1000000U * 1000U;
	timer_settime(hrt_timerid, 0, &spec, NULL);
}

/*
 *  高分解能タイマ割込みの要求
 */
void
target_hrt_raise_event(void)
{
	raise_int(INTNO_TIMER);
}

/*
 *  高分解能タイマの初期化
 */
Inline void
target_hrt_initialize(void)
{
	struct sigevent	sev;

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_int = INTNO_TIMER;
	timer_create(CLOCK_MONOTONIC, &sev, &hrt_timerid);
}

/*
 *  オーバランタイマドライバ
 */
#ifdef TOPPERS_SUPPORT_OVRHDR

/*
 *  オーバランタイマに用いるタイマ管理ブロック
 */
static timer_t	ovr_timerid;

/*
 *  オーバランタイマの動作開始
 */
void
target_ovrtimer_start(PRCTIM ovrtim)
{
	struct itimerspec	spec;

	if (ovrtim > 0U) {
		spec.it_interval.tv_sec = 0;
		spec.it_interval.tv_nsec = 0;
		spec.it_value.tv_sec = ovrtim / 1000000U;
		spec.it_value.tv_nsec = ovrtim % 1000000U * 1000U;
		timer_settime(ovr_timerid, 0, &spec, NULL);
	}
	else {
		raise_int(INTNO_OVRTIMER);
	}
}

/*
 *  オーバランタイマの停止
 */
PRCTIM
target_ovrtimer_stop(void)
{
	struct itimerspec	spec;

	timer_settime(ovr_timerid, 0, &itimerspec_stop, &spec);
	return((PRCTIM)(spec.it_value.tv_sec * 1000000U
								+ spec.it_value.tv_nsec / 1000U));
}

/*
 *  オーバランタイマの現在値の読出し
 */
PRCTIM
target_ovrtimer_get_current(void)
{
	struct itimerspec	spec;

	timer_gettime(ovr_timerid, &spec);
	return((PRCTIM)(spec.it_value.tv_sec * 1000000U
								+ spec.it_value.tv_nsec / 1000U));
}

/*
 *  オーバランタイマの初期化
 */
Inline void
target_ovrtimer_initialize(void)
{
	struct sigevent	sev;

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_int = INTNO_OVRTIMER;
	timer_create(CLOCK_MONOTONIC, &sev, &ovr_timerid);
}

#endif /* TOPPERS_SUPPORT_OVRHDR */

/*
 *  タイマの初期化処理
 */
void
target_timer_initialize(EXINF exinf)
{
	struct sigaction	sigact;

	/*
	 *  SIGALRMシグナルのハンドラの設定
	 */
	sigact.sa_sigaction = sigalrm_handler;
	sigact.sa_flags = SA_SIGINFO;
	sigemptyset(&(sigact.sa_mask));
	sigaction(SIGALRM, &sigact, NULL);

	/*
	 *  各タイマの初期化
	 */
	target_hrt_initialize();
#ifdef TOPPERS_SUPPORT_OVRHDR
	target_ovrtimer_initialize();
#endif /* TOPPERS_SUPPORT_OVRHDR */
}

/*
 *  タイマの終了処理
 */
void
target_timer_terminate(EXINF exinf)
{
	/*
	 *  タイマを削除する．
	 */
	timer_delete(hrt_timerid);
#ifdef TOPPERS_SUPPORT_OVRHDR
	timer_delete(ovr_timerid);
#endif /* TOPPERS_SUPPORT_OVRHDR */
}
