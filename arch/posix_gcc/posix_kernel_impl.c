/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2006-2023 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: posix_kernel_impl.c 1793 2023-03-05 10:30:13Z ertl-hiro $
 */

/*
 *		カーネルのターゲット依存部（POSIX用）
 */

#include "kernel_impl.h"
#include "task.h"
#ifdef TOPPERS_SUPPORT_OVRHDR
#include "overrun.h"
#endif /* TOPPERS_SUPPORT_OVRHDR */

/*
 *  トレースログマクロのデフォルト定義
 */
#ifndef LOG_DSP_ENTER
#define LOG_DSP_ENTER(p_tcb)
#endif /* LOG_DSP_ENTER */

#ifndef LOG_DSP_LEAVE
#define LOG_DSP_LEAVE(p_tcb)
#endif /* LOG_DSP_LEAVE */

#ifndef LOG_INH_ENTER
#define LOG_INH_ENTER(inhno)
#endif /* LOG_INH_ENTER */

#ifndef LOG_INH_LEAVE
#define LOG_INH_LEAVE(inhno)
#endif /* LOG_INH_LEAVE */

#ifndef LOG_EXC_ENTER
#define LOG_EXC_ENTER(excno)
#endif /* LOG_EXC_ENTER */

#ifndef LOG_EXC_LEAVE
#define LOG_EXC_LEAVE(excno)
#endif /* LOG_EXC_LEAVE */

/*
 *  メインスレッドのスレッドID
 */
pthread_t	main_thread;

/*
 *  スレッド管理ブロックを格納するスレッドローカルデータのキー
 */
pthread_key_t	thrcb_key;

/*
 *  割込みロック状態／CPUロック状態でマスクするシグナルを保持する変数
 */
sigset_t	sigmask_intlock;
sigset_t	sigmask_cpulock;

/*
 *  アイドルスレッドの管理ブロック
 */
THRCB	idle_thread;

/*
 *  割込みハンドラ／CPU例外ハンドラのネスト数
 */
uint_t	excpt_nest_count;

/*
 *  SIGUSR1のシグナルハンドラ（プリエンプション処理）
 */
static void
sigusr1_handler(int sig)
{
	/* メインスレッドがSIGUSR1を受け取ることはない．*/
	assert(pthread_getspecific(thrcb_key) != NULL);

	/* 実行すべきスレッドへの切換え */
	suspend_thread(scheduled_thread());
}

/*
 *  割込みスレッドのメイン関数
 */
void *
int_entry(void *arg)
{
	INTRCB		*p_my_intrcb = arg;
	THRCB		*p_my_thrcb = &p_my_intrcb->thrcb;

	/* スレッドローカルデータの設定 */
	pthread_setspecific(thrcb_key, p_my_thrcb);

	/* SIGUSR1以外のシグナルマスクを解除 */
	pthread_sigmask(SIG_SETMASK, &sigmask_cpulock, NULL);

	if (excpt_nest_count++ == 0) {
		/* 
		 *  割込みがタスクコンテキストで発生した場合の処理
		 */
#ifdef TOPPERS_SUPPORT_OVRHDR
		ovrtimer_stop();				/* オーバランタイマの停止 */
#endif /* TOPPERS_SUPPORT_OVRHDR */
	}

	/* CPUロック状態の解除 */
	unlock_cpu();

	/* 割込みハンドラを呼び出す */
	LOG_INH_ENTER(inhno);
	(*(p_my_intrcb->p_inhinib->inthdr))();
	LOG_INH_LEAVE(inhno);

	/* CPUロック状態に */
	if (!sense_lock()) {
		lock_cpu();
	}

	/*
	 *  割込み出口処理
	 */
	if (--excpt_nest_count == 0) {
		if (p_runtsk != p_schedtsk) {
			/*
			 *  タスク切換え
			 */
			if (p_runtsk != NULL) {
				LOG_DSP_ENTER(p_runtsk);
			}
			p_runtsk = p_schedtsk;
			if (p_runtsk != NULL) {
				LOG_DSP_LEAVE(p_runtsk);
			}
		}
		if (p_runtsk != NULL) {
#ifdef TOPPERS_SUPPORT_OVRHDR
			ovrtimer_start();			/* オーバランタイマの動作開始 */
#endif /* TOPPERS_SUPPORT_OVRHDR */
		}
	}

	/* 割込みハンドラからのリターン処理 */
	return_intr(p_my_intrcb);

	/* 実行すべきスレッドへの切換え */
	exit_thread(scheduled_thread());
	assert(false);
	return(NULL);
}

/*
 *  最高優先順位タスクへのディスパッチ
 */
void
dispatch(void)
{
#ifdef TOPPERS_SUPPORT_OVRHDR
	ovrtimer_stop();					/* オーバランタイマの停止 */
#endif /* TOPPERS_SUPPORT_OVRHDR */
	LOG_DSP_ENTER(p_runtsk);
	p_runtsk = p_schedtsk;
	if (p_runtsk != NULL) {
		LOG_DSP_LEAVE(p_runtsk);
#ifdef TOPPERS_SUPPORT_OVRHDR
		ovrtimer_start();				/* オーバランタイマの動作開始 */
#endif /* TOPPERS_SUPPORT_OVRHDR */
	}
	suspend_thread(scheduled_thread());
}

/*
 *  現在のコンテキストを捨ててディスパッチ
 */
void
exit_and_dispatch(void)
{
	LOG_DSP_ENTER(p_runtsk);
	p_runtsk = p_schedtsk;
	if (p_runtsk != NULL) {
		LOG_DSP_LEAVE(p_runtsk);
#ifdef TOPPERS_SUPPORT_OVRHDR
		ovrtimer_start();				/* オーバランタイマの動作開始 */
#endif /* TOPPERS_SUPPORT_OVRHDR */
	}
	exit_thread(scheduled_thread());
}

/*
 *  タスクスレッドのメイン関数
 */
static void *
start_r(void *arg)
{
	TCB		*p_my_tcb = arg;
	THRCB	*p_my_thrcb = &(p_my_tcb->tskctxb);

	/* スレッドローカルデータの設定 */
	pthread_setspecific(thrcb_key, p_my_thrcb);

	/* SIGUSR1以外のシグナルマスクを解除 */
	pthread_sigmask(SIG_SETMASK, &sigmask_cpulock, NULL);

	/* CPUロック状態の解除 */
	unlock_cpu();

	/* タスクのメイン関数の呼び出し */
	(*(p_my_tcb->p_tinib->task))(p_my_tcb->p_tinib->exinf);

	/* ext_tskの呼び出し */
	ext_tsk();
	assert(false);
	return(NULL);
}

/*
 *  ディスパッチャの開始（メインスレッド用）
 */
void
start_dispatch_task(void)
{
	p_runtsk = p_schedtsk;
	if (p_runtsk != NULL) {
		LOG_DSP_LEAVE(p_runtsk);
		/*
		 *  カーネルの起動直後は，オーバランハンドラは動作停止している
		 *  ので，オーバランタイマの動作開始は必要ない．
		 */
	}

	/*
	 *  スレッドディスパッチャの開始
	 */
	start_dispatch_thread(scheduled_thread());
}

/*
 *  CPU例外ハンドラの汎用出入口処理
 */
void
exc_entry_generic(int sig, siginfo_t *p_siginfo, void *p_ctx, EXCHDR exchdr)
{
	struct sigaction	sigact;

	if (pthread_getspecific(thrcb_key) == NULL) {
		/*
		 *  メインスレッドで発生した場合
		 *
		 *  シグナルハンドラの設定を解除し，シグナルを再度発生させるこ
		 *  とで，プロセスを終了させる．
		 */
		sigact.sa_handler = SIG_DFL;
		sigact.sa_flags = 0;
		sigemptyset(&(sigact.sa_mask));
		sigaction(sig, &sigact, NULL);
		pthread_kill(pthread_self(), sig);
		assert(false);
	}

	if (!kerflg || exc_sense_nonkernel(p_ctx)) {
		/*
		 *  カーネル管理外のCPU例外ハンドラの場合
		 */
		excpt_nest_count += 1;
		exchdr(p_ctx);
		excpt_nest_count -= 1;
	}
	else {
		/*
		 *  カーネル管理のCPU例外ハンドラの場合
		 */
		if (excpt_nest_count++ == 0) {
			/* 
			 *  CPU例外がタスクコンテキストで発生した場合の処理
			 */
#ifdef TOPPERS_SUPPORT_OVRHDR
			ovrtimer_stop();			/* オーバランタイマの停止 */
#endif /* TOPPERS_SUPPORT_OVRHDR */
		}

		/* CPUロック状態の解除 */
		unlock_cpu();

		/* CPU例外ハンドラを呼び出す */
		LOG_EXC_ENTER((EXCNO) sig);
		exchdr(p_ctx);
		LOG_EXC_LEAVE((EXCNO) sig);

		/* CPUロック状態に */
		if (!sense_lock()) {
			lock_cpu();
		}

		/*
		 *  CPU例外出口処理
		 */
		if (--excpt_nest_count == 0) {
			if (p_runtsk != p_schedtsk) {
				/*
				 *  タスク切換え
				 */
				if (p_runtsk != NULL) {
					LOG_DSP_ENTER(p_runtsk);
				}
				p_runtsk = p_schedtsk;
				if (p_runtsk != NULL) {
					LOG_DSP_LEAVE(p_runtsk);
				}
			}
			if (p_runtsk != NULL) {
#ifdef TOPPERS_SUPPORT_OVRHDR
				ovrtimer_start();		/* オーバランタイマの動作開始 */
#endif /* TOPPERS_SUPPORT_OVRHDR */
			}
		}

		/* 実行すべきスレッドへの切換え */
		suspend_thread(scheduled_thread());
	}
}

/*
 *  CPU例外ハンドラの設定
 *
 *  シグナルハンドラが起動される時に，SIGUSR1がマスクされるようにして
 *  いる．また，sa_flagにSA_NODEFERを設定して，シグナルハンドラの起動
 *  時に，発生したシグナルをマスクするのを抑止している．
 */
void
define_exc(EXCNO excno, FP exc_entry)
{
	struct sigaction	sigact;

	sigact.sa_sigaction = (void (*)(int, siginfo_t *, void *))(exc_entry);
	sigact.sa_flags = (SA_SIGINFO | SA_NODEFER);
	sigemptyset(&(sigact.sa_mask));
	sigaddset(&(sigact.sa_mask), SIGUSR1);
	sigaction(excno, &sigact, NULL);
}

/*
 *  カーネルの終了処理の呼出し
 */
void
call_exit_kernel(void)
{
	/*
	 *  メインスレッドにSIGTERMシグナルを送る．
	 */
	pthread_kill(main_thread, SIGTERM);
	pthread_exit(NULL);
}

/*
 *  SIGTERMシグナルのハンドラ
 */
static void
sigterm_handler(int sig)
{
	if (pthread_getspecific(thrcb_key) != NULL) {
		pthread_kill(main_thread, sig);
		return;
	}

	/* カーネルの終了処理 */
	exit_kernel();
}

/*
 *  アイドルスレッドのメイン関数
 */
static void *
idle_thread_entry(void *arg)
{
#ifndef TOPPERS_CUSTOM_IDLE
	sigset_t	sigmask;
#endif /* TOPPERS_CUSTOM_IDLE */

	/* スレッドローカルデータの設定 */
	pthread_setspecific(thrcb_key, arg);

	/* SIGUSR1以外のシグナルマスクを解除 */
	pthread_sigmask(SIG_SETMASK, &sigmask_cpulock, NULL);

	/* CPUロック状態の解除 */
	unlock_cpu();

	/* アイドル処理 */
	while (true) {
#ifdef TOPPERS_CUSTOM_IDLE
		target_custom_idle();
#else /* TOPPERS_CUSTOM_IDLE */
		sigemptyset(&sigmask);
		sigsuspend(&sigmask);
#endif /* TOPPERS_CUSTOM_IDLE */
	}
	assert(false);
	return(NULL);
}

/*
 *  POSIX依存部の初期化
 */
void
posix_initialize(void)
{
	uint_t				i;
	TCB					*p_tcb;
	const TINIB			*p_tinib;
	struct sigaction	sigact;

	/*
	 *  メインスレッドのスレッドIDの取得
	 */
	main_thread = pthread_self();

	/*
	 *  スレッドローカルデータのキーの生成
	 */
	pthread_key_create(&thrcb_key, NULL);

	/*
	 *  スレッド操作モジュールの初期化
	 */
	initialize_thread_ctrl();

	/*
	 *  プリエンプション用のSIGUSR1のシグナルハンドラの設定
	 */
	sigact.sa_handler = sigusr1_handler;
	sigact.sa_flags = 0;
	sigemptyset(&(sigact.sa_mask));
	sigaction(SIGUSR1, &sigact, NULL);

	/*
	 *  割込みロック状態でマスクするシグナルを保持する変数の初期化
	 */
	sigemptyset(&sigmask_intlock);
	sigaddset(&sigmask_intlock, SIGUSR1);
	sigaddset(&sigmask_intlock, SIGUSR2);

	/*
	 *  CPUロック状態でマスクするシグナルを保持する変数の初期化
	 */
	sigemptyset(&sigmask_cpulock);
	sigaddset(&sigmask_cpulock, SIGUSR1);

	/*
	 *  CPUロック状態と判定される状態にする
	 */
	pthread_sigmask(SIG_BLOCK, &sigmask_cpulock, NULL);

	/*
	 *  タスクスレッドの初期化
	 */
	for (i = 0; i < tnum_tsk; i++) {
		p_tcb = &(tcb_table[i]);
		p_tinib = &(tinib_table[i]);
		init_thrcb(&(p_tcb->tskctxb), THREAD_TYPE_TASK);
		p_tcb->tskctxb.start_routine = start_r;
		p_tcb->tskctxb.arg = p_tcb;
		p_tcb->tskctxb.stacksize = p_tinib->tskinictxb.stksz;
	}

	/*
	 *  アイドルスレッドの初期化
	 */
	init_thrcb(&idle_thread, THREAD_TYPE_IDLE);
	idle_thread.start_routine = idle_thread_entry;
	idle_thread.arg = &idle_thread;

	/*
	 *  割込みハンドラ／CPU例外ハンドラのネスト数の初期化
	 */
	excpt_nest_count = 0;

	/*
	 *  SIGTERMシグナルのハンドラの設定
	 */
	sigact.sa_handler = sigterm_handler;
	sigact.sa_flags = 0;
	sigemptyset(&(sigact.sa_mask));
	sigaction(SIGTERM, &sigact, NULL);
}
