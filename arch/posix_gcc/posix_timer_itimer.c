/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2005-2023 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: posix_timer_itimer.c 1793 2023-03-05 10:30:13Z ertl-hiro $
 */

/*
 *		タイマドライバ（POSIX用）
 *
 *  POSIXのsetitimer機能で複数のタイマをシミュレーションする．
 *
 *  インターバルタイマの動作・停止を繰り返すため，時間のずれが生じると
 *  いう問題があるが，シミュレーション環境で時間のずれは大きい問題では
 *  ないため，この方法を採用している．
 */

#include "kernel_impl.h"
#include "target_timer.h"
#include <sys/time.h>

/*
 *  タイマの絶対時刻（インターバルタイマの動作時間の積算値）
 */
typedef uint64_t	ABSTIM;

/*
 *  タイマ管理ブロック
 */
typedef struct timer_control_block {
	QUEUE		active_queue;		/* キューにつなぐための領域 */
	ABSTIM		expire_abstim;		/* タイマがexpireする絶対時刻 */
	INTNO		intno;				/* 発生させる割込み番号 */
} TMRCB;

/*
 *  動作しているタイマのキュー
 */
static QUEUE	active_tmrcb_queue;

/*
 *  インターバルタイマ設定時点での絶対時刻
 */
static ABSTIM	current_abstim;

/*
 *  インターバルタイマの設定値
 */
static uint_t	current_itimer_value;

/*
 *  インターバルタイマを停止させるための変数
 */
static const struct itimerval	itimerval_stop = {{ 0, 0 }, { 0, 0 }};

/*
 *  タイマドライバを排他制御するためのミューテックス
 *
 *  syslogやassertが使われると，target_hrt_get_currentが呼び出され，そ
 *  の中でtimer_mutexがロックされる．syslogやassertは，シグナルハンド
 *  ラから呼び出される可能性があるため，デッドロックの回避のために，
 *  timer_mutexをロックする場合には，すべてのシグナルをマスクする．
 */
static pthread_mutex_t	timer_mutex;

/*
 *  インターバルタイマの経過時間の算出
 */
Inline uint_t
itimer_progress(struct itimerval *p_val)
{
	return(current_itimer_value - (p_val->it_value.tv_sec * 1000000U
											+ p_val->it_value.tv_usec));
}

/*
 *  インターバルタイマの動作開始処理
 */
static void
itimer_start(void)
{
	struct itimerval	val;
	TMRCB				*p_top_tmrcb;

	/*
	 *  インターバルタイマの設定値を求める．
	 */
	if (queue_empty(&active_tmrcb_queue)) {
		current_itimer_value = UINT_MAX;
	}
	else {
		p_top_tmrcb = (TMRCB *) active_tmrcb_queue.p_next;
		if (p_top_tmrcb->expire_abstim <= current_abstim) {
			/*
			 *  すぐにSIGALRMを発生させる．
			 */
			pthread_kill(main_thread, SIGALRM);
			return;
		}
		else {
			assert(p_top_tmrcb->expire_abstim - current_abstim <= UINT_MAX);
			current_itimer_value = p_top_tmrcb->expire_abstim - current_abstim;
		}
	}

	/*
	 *  インターバルタイマの動作開始
	 */
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	val.it_value.tv_sec = current_itimer_value / 1000000U;
	val.it_value.tv_usec = current_itimer_value % 1000000U;
	setitimer(ITIMER_REAL, &val, NULL);
}

/*
 *  インターバルタイマの停止処理
 */
static void
itimer_stop(void)
{
	struct itimerval	val;

	/*
	 *  インターバルタイマを停止し，current_abstimを更新する．
	 */
	setitimer(ITIMER_REAL, &itimerval_stop, &val);
	current_abstim += itimer_progress(&val);
	current_itimer_value = 0;
}

/*
 *  現在の絶対時刻の読み出し
 */
static ABSTIM
get_current_abstim(void)
{
	struct itimerval	val;

	/*
	 *  インターバルタイマを読み出し，現在の絶対時刻を求める．
	 */
	getitimer(ITIMER_REAL, &val);
	return(current_abstim + itimer_progress(&val));
}

/*
 *  タイマの動作開始
 */
static void
tmrcb_insert(TMRCB *p_tmrcb)
{
	QUEUE	*p_entry;
	ABSTIM	abstim = p_tmrcb->expire_abstim;

	/*
	 *  タイマ管理ブロックを，動作しているタイマのキューに挿入する．
	 */
	for (p_entry = active_tmrcb_queue.p_next;
				p_entry != &active_tmrcb_queue; p_entry = p_entry->p_next) {
		if (abstim < ((TMRCB *) p_entry)->expire_abstim) {
			break;
		}
	}
	queue_insert_prev(p_entry, &(p_tmrcb->active_queue));

	/*
	 *  先頭に挿入した場合には，インターバルタイマを再設定する．
	 */
	if (p_tmrcb == (TMRCB *) active_tmrcb_queue.p_next) {
		itimer_stop();
		itimer_start();
	}
}

/*
 *  タイマの動作停止
 */
static void
tmrcb_delete(TMRCB *p_tmrcb)
{
	TMRCB	*p_top_tmrcb;

	/*
	 *  タイマ管理ブロックを，動作しているタイマのキューから削除する．
	 */
	p_top_tmrcb = (TMRCB *) active_tmrcb_queue.p_next;
	queue_delete(&(p_tmrcb->active_queue));
	queue_initialize(&(p_tmrcb->active_queue));

	/*
	 *  先頭を削除した場合には，インターバルタイマを再設定する．
	 */
	if (p_tmrcb == p_top_tmrcb) {
		itimer_stop();
		itimer_start();
	}
}

/*
 *  SIGALRMシグナルのハンドラ（メインスレッドで実行）
 */
static void
sigalrm_handler(int sig)
{
	TMRCB	*p_top_tmrcb;

	/*
	 *  メインスレッド以外がSIGALRMシグナルを受け取った場合は，メイン
	 *  スレッドに回送する．
	 */
	if (pthread_getspecific(thrcb_key) != NULL) {
		pthread_kill(main_thread, sig);
		return;
	}

	/*
	 *  ミューテックスをロック
	 *
	 *  SIGALRMのハンドラ内では，SIGALRMはマスクされているため，ここで
	 *  SIGALRMをマスクする必要はない．
	 */
	MUTEX_LOCK(&timer_mutex);

	/*
	 *  current_abstimを更新する．
	 */
	current_abstim += current_itimer_value;
	current_itimer_value = 0;

	/*
	 *  先頭のタイマ管理ブロックのexpire処理を行う．
	 */
	p_top_tmrcb = (TMRCB *) active_tmrcb_queue.p_next;
	if (p_top_tmrcb->expire_abstim <= current_abstim) {
		/* タイマのキューから削除 */
		queue_delete(&(p_top_tmrcb->active_queue));
		queue_initialize(&(p_top_tmrcb->active_queue));

		/* 割込みを要求 */
		raise_int_main(p_top_tmrcb->intno);
	}

	/*
	 *  インターバルタイマの動作を開始する．
	 */
	itimer_start();

	/*
	 *  ミューテックスを解放
	 */
	MUTEX_UNLOCK(&timer_mutex);
}

/*
 *  高分解能タイマドライバ
 *
 *  64ビットの高分解能タイマをシミュレートすることも可能であるが，シミュ
 *  レーション環境であることを考え，高分解能タイマのビット数は32ビット
 *  とし，絶対時刻の下位32ビットを現在のカウント値とする．
 */

/*
 *  高分解能タイマに用いるタイマ管理ブロック
 */
static TMRCB	hrt_tmrcb;

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
	HRTCNT		hrtcnt;
	sigset_t	saved_sigmask;

	pthread_sigmask_blockall(&saved_sigmask);
	MUTEX_LOCK(&timer_mutex);

	hrtcnt = (HRTCNT) get_current_abstim();

	MUTEX_UNLOCK(&timer_mutex);
	pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
	return(hrtcnt);
}

/*
 *  高分解能タイマへの割込みタイミングの設定
 */
void
target_hrt_set_event(HRTCNT hrtcnt)
{
	sigset_t	saved_sigmask;

	pthread_sigmask_blockall(&saved_sigmask);
	MUTEX_LOCK(&timer_mutex);

	if (!queue_empty(&(hrt_tmrcb.active_queue))) {
		tmrcb_delete(&(hrt_tmrcb));
	}
	hrt_tmrcb.expire_abstim = get_current_abstim() + hrtcnt;
	tmrcb_insert(&(hrt_tmrcb));

	MUTEX_UNLOCK(&timer_mutex);
	pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
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
	queue_initialize(&(hrt_tmrcb.active_queue));
	hrt_tmrcb.intno = INTNO_TIMER;
}

/*
 *  オーバランタイマドライバ
 */
#ifdef TOPPERS_SUPPORT_OVRHDR

/*
 *  オーバランタイマに用いるタイマ管理ブロック
 */
static TMRCB	ovr_tmrcb;

/*
 *  オーバランタイマの動作開始
 */
void
target_ovrtimer_start(PRCTIM ovrtim)
{
	sigset_t	saved_sigmask;

	if (ovrtim > 0U) {
		pthread_sigmask_blockall(&saved_sigmask);
		MUTEX_LOCK(&timer_mutex);

		assert(queue_empty(&(ovr_tmrcb.active_queue)));
		ovr_tmrcb.expire_abstim = get_current_abstim() + ovrtim;
		tmrcb_insert(&(ovr_tmrcb));

		MUTEX_UNLOCK(&timer_mutex);
		pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
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
	PRCTIM		lefttim;
	ABSTIM		abstim;
	sigset_t	saved_sigmask;

	pthread_sigmask_blockall(&saved_sigmask);
	MUTEX_LOCK(&timer_mutex);

	tmrcb_delete(&(ovr_tmrcb));
	abstim = get_current_abstim();
	if (abstim < ovr_tmrcb.expire_abstim) {
		lefttim = ovr_tmrcb.expire_abstim - abstim;
	}
	else {
		lefttim = 0U;
	}

	MUTEX_UNLOCK(&timer_mutex);
	pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
	return(lefttim);
}

/*
 *  オーバランタイマの現在値の読出し
 */
PRCTIM
target_ovrtimer_get_current(void)
{
	PRCTIM		lefttim;
	ABSTIM		abstim;
	sigset_t	saved_sigmask;

	pthread_sigmask_blockall(&saved_sigmask);
	MUTEX_LOCK(&timer_mutex);

	abstim = get_current_abstim();
	if (abstim < ovr_tmrcb.expire_abstim) {
		lefttim = ovr_tmrcb.expire_abstim - abstim;
	}
	else {
		lefttim = 0U;
	}

	MUTEX_UNLOCK(&timer_mutex);
	pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
	return(lefttim);
}

/*
 *  オーバランタイマの初期化
 */
Inline void
target_ovrtimer_initialize(void)
{
	queue_initialize(&(ovr_tmrcb.active_queue));
	ovr_tmrcb.intno = INTNO_OVRTIMER;
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
	 *  ミューテックスの初期化
	 */
	pthread_mutex_init(&timer_mutex, NULL);

	/*
	 *  グローバル変数の初期化
	 */
	queue_initialize(&active_tmrcb_queue);
	current_abstim = 0;
	current_itimer_value = 0;

	/*
	 *  SIGALRMシグナルのハンドラの設定
	 */
	sigact.sa_handler = sigalrm_handler;
	sigact.sa_flags = 0;
	sigemptyset(&(sigact.sa_mask));
	sigaction(SIGALRM, &sigact, NULL);

	/*
	 *  各タイマの初期化
	 */
	target_hrt_initialize();
#ifdef TOPPERS_SUPPORT_OVRHDR
	target_ovrtimer_initialize();
#endif /* TOPPERS_SUPPORT_OVRHDR */

	/*
	 *  インターバルタイマの動作開始
	 */
	itimer_start();
}

/*
 *  タイマの終了処理
 */
void
target_timer_terminate(EXINF exinf)
{
	/*
	 *  インターバルタイマの動作を停止する．
	 */
	setitimer(ITIMER_REAL, &itimerval_stop, NULL);
}
