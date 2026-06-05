/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 * 
 *  Copyright (C) 2022 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Informatics, Nagoya Univ., JAPAN
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
 *  $Id: thread_ctrl.c 1720 2022-10-23 04:07:15Z ertl-hiro $
 */

/*
 *		POSIX用スレッド制御モジュール
 */

#include "kernel_impl.h"
#include <pthread.h>
#include <signal.h>

/*
 *  実行状態のスレッド
 *
 *  実行状態のスレッド（タスクスレッド，割込みスレッド，アイドルスレッ
 *  ド）のスレッド管理ブロックを指すポインタ．
 */
static THRCB	*p_run_thread;

/*
 *  スレッド制御モジュールを排他制御するためのミューテックス
 */
static pthread_mutex_t	thrcb_mutex;

/*
 *  スレッド管理ブロックの初期化
 */
void
init_thrcb(THRCB *p_thrcb, uint_t type)
{
	/*
	 *  スレッド管理ブロックの初期化
	 */
	p_thrcb->type = type;
	p_thrcb->state = THREAD_STAT_TERMINATE;
	p_thrcb->stacksize = 0;

	/*
	 *  条件変数の初期化
	 */
	pthread_cond_init(&(p_thrcb->thrcond), NULL);
}

/*
 *  スレッドの生成
 */
static void
create_thread(THRCB *p_thrcb)
{
	pthread_attr_t	thr_attr;
	sigset_t		saved_sigmask;

	/*
	 *  スレッド属性の作成
	 */
	pthread_attr_init(&thr_attr);
	pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_DETACHED);
	if (p_thrcb->stacksize != 0) {
		pthread_attr_setstacksize(&thr_attr, p_thrcb->stacksize);
	}

	/*
	 *  スレッドの生成
	 *
	 *  スレッドの実行開始時にすべてのシグナルがマスクされている状態と
	 *  するために，すべてのシグナルをマスクして，スレッドを生成する．
	 */
	pthread_sigmask_blockall(&saved_sigmask);
	pthread_create(&p_thrcb->thrid, &thr_attr,
							p_thrcb->start_routine, p_thrcb->arg);
	pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
}

/*
 *  指定したスレッドの実行
 */
static void
dispatch_thread(THRCB *p_thrcb)
{
	switch (p_thrcb->state) {
	case THREAD_STAT_SUSPEND:
		/*
		 *  スレッドの実行再開
		 */
		pthread_cond_signal(&p_thrcb->thrcond);
		break;

	case THREAD_STAT_TERMINATE:
		/*
		 *  スレッドの生成
		 */
		create_thread(p_thrcb);
		break;

	default:
		assert(false);
	}
	p_run_thread = p_thrcb;
	p_run_thread->state = THREAD_STAT_RUN;
}

/*
 *  自スレッドを中断して指定したスレッドへ切換え
 *
 *  dispatch_threadの中でp_run_threadが変更されるため，それを呼び出す
 *  前に，p_run_threadをp_my_thrcbに取得しておく．
 */
void
suspend_thread(THRCB *p_thrcb)
{
	THRCB	*p_my_thrcb;

	MUTEX_LOCK(&thrcb_mutex);
	if (p_thrcb != p_run_thread) {
		p_run_thread->state = THREAD_STAT_SUSPEND;
		p_my_thrcb = p_run_thread;
		dispatch_thread(p_thrcb);

		/*
		 *  スレッドの条件変数待ちの偽の待ち解除（spurious wakeup）へ
		 *  の対策．macOSにおいて起こっている．
		 */
	  retry:
		pthread_cond_wait(&(p_my_thrcb->thrcond), &thrcb_mutex);
		if (p_my_thrcb->state != THREAD_STAT_RUN) {
			syslog_0(LOG_NOTICE, "spurious thread wakeup.");
			goto retry;
		}
	}
	MUTEX_UNLOCK(&thrcb_mutex);
}

/*
 *  自スレッドを終了して指定したスレッドへ切換え
 *
 *  指定したスレッドが自スレッドの場合のために，dispatch_threadを呼び
 *  出す前に，p_run_thread->stateを書き換える必要がある．
 */
void
exit_thread(THRCB *p_thrcb)
{
	MUTEX_LOCK(&thrcb_mutex);
	p_run_thread->state = THREAD_STAT_TERMINATE;
	dispatch_thread(p_thrcb);
	MUTEX_UNLOCK(&thrcb_mutex);
	pthread_exit(NULL);
}

/*
 *  指定したスレッドの終了
 */
void
terminate_thread(THRCB *p_thrcb)
{
	MUTEX_LOCK(&thrcb_mutex);
	if (p_thrcb->state == THREAD_STAT_SUSPEND) {
		p_thrcb->state = THREAD_STAT_TERMINATE;
		pthread_cancel(p_thrcb->thrid);
	}
	MUTEX_UNLOCK(&thrcb_mutex);
}

/*
 *  スレッドのプリエンプトの要求
 */
void
preempt_thread(THRCB *p_thrcb)
{
	MUTEX_LOCK(&thrcb_mutex);
	if (p_thrcb->state != THREAD_STAT_TERMINATE) {
		pthread_kill(p_thrcb->thrid, SIGUSR1);
	}
	MUTEX_UNLOCK(&thrcb_mutex);
}

/*
 *  スレッドディスパッチャの開始（メインスレッド用）
 */
void
start_dispatch_thread(THRCB *p_thrcb)
{
	assert(pthread_getspecific(thrcb_key) == NULL);

	MUTEX_LOCK(&thrcb_mutex);
	dispatch_thread(p_thrcb);
	MUTEX_UNLOCK(&thrcb_mutex);
}

/*
 *  スレッド操作モジュールの初期化
 */
void
initialize_thread_ctrl(void)
{
	/*
	 *  ミューテックスの初期化
	 */
	pthread_mutex_init(&thrcb_mutex, NULL);
}
