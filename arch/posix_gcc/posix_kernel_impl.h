/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2006-2026 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: posix_kernel_impl.h 1881 2026-05-18 03:52:07Z ertl-hiro $
 */

/*
 *		カーネルのターゲット依存部（POSIX用）
 *
 *  カーネルのPOSIX依存部のヘッダファイル．kernel_impl.hのPOSIX依存部
 *  の位置付けとなる．
 */

#ifndef TOPPERS_POSIX_KERNEL_IMPL_H
#define TOPPERS_POSIX_KERNEL_IMPL_H

#ifndef TOPPERS_MACRO_ONLY
#include <signal.h>
#include <pthread.h>
#endif /* TOPPERS_MACRO_ONLY */

#include <kernel.h>
#include <queue.h>
#include <t_syslog.h>
#ifndef TOPPERS_MACRO_ONLY
#include "thread_ctrl.h"
#include "interrupt_sim.h"
#endif /* TOPPERS_MACRO_ONLY */

/*
 *  非タスクコンテキスト用のスタック領域を使用しない
 */
#define OMIT_ISTACK

#ifndef TOPPERS_MACRO_ONLY

/*
 *  メインスレッドのスレッドID
 */
extern pthread_t	main_thread;

/*
 *  スレッド管理ブロックを格納するスレッドローカルデータのキー
 *
 *  スレッド制御モジュールの管理対象のスレッドに対しては，スレッドロー
 *  カルデータに，自スレッドのスレッド管理ブロックへのポインタを格納す
 *  る．メインスレッドに対しては，格納しない．
 */
extern pthread_key_t	thrcb_key;

/*
 *  割込みロック状態／CPUロック状態でマスクするシグナルを保持する変数
 */
extern sigset_t	sigmask_intlock;
extern sigset_t	sigmask_cpulock;

/*
 *  アイドルスレッドの管理ブロック
 */
extern THRCB	idle_thread;

/*
 *  割込みハンドラ／CPU例外ハンドラのネスト数
 */
extern uint_t	excpt_nest_count;

/*
 *  POSIXミューテックスのロック/ロック解放のマクロ
 *
 *  POSIXミューテックスのロック中にスレッドが終了しても問題ないように，
 *  pthread_cleanup_pushを用いてミューテックスを解放する．
 */
#define MUTEX_LOCK(p_mutex) \
	pthread_mutex_lock(p_mutex); \
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, p_mutex);

#define MUTEX_UNLOCK(p_mutex) \
	pthread_cleanup_pop(1);

/*
 *  すべてのシグナルのマスク
 *
 *  すべてのシグナル（マスクできないものを除く）をマスクして，変更前の
 *  シグナルマスクをp_saved_sigmaskが指す領域に返す．
 */
Inline void
pthread_sigmask_blockall(sigset_t *p_saved_sigmask)
{
	sigset_t	sigmask;

	sigfillset(&sigmask);
	pthread_sigmask(SIG_BLOCK, &sigmask, p_saved_sigmask);
}

/*
 *  コンテキストの参照
 */
Inline bool_t
sense_context(void)
{
	return(excpt_nest_count != 0);
}

/*
 *  CPUロック状態の参照
 */
Inline bool_t
sense_lock(void)
{
	sigset_t	sigmask;

	pthread_sigmask(SIG_SETMASK, NULL, &sigmask);
	return(sigismember(&sigmask, SIGUSR1));
}

/*
 *  CPUロック状態への移行
 */
Inline void
lock_cpu(void)
{
	assert(!sense_lock());
	pthread_sigmask(SIG_BLOCK, &sigmask_cpulock, NULL);
}

/*
 *  CPUロック状態への移行（ディスパッチできる状態）
 */
#define lock_cpu_dsp()		lock_cpu()

/*
 *  CPUロック状態の解除
 */
Inline void
unlock_cpu(void)
{
	assert(sense_lock());
	suspend_thread(scheduled_thread());
	pthread_sigmask(SIG_UNBLOCK, &sigmask_cpulock, NULL);
}

/*
 *  CPUロック状態の解除（ディスパッチできる状態）
 */
#define unlock_cpu_dsp()	unlock_cpu()

/*
 *  割込みを受け付けるための遅延処理
 */
Inline void
delay_for_interrupt(void)
{
}

/*
 *  割込みスレッドのメイン関数
 */
extern void *int_entry(void *arg);

/*
 *  タスクコンテキストブロックの定義
 *
 *  スレッド管理ブロックをそのまま使う．
 */
typedef THRCB	TSKCTXB;

/*
 *  タスク初期化コンテキストブロックの定義
 */
#define USE_TSKINICTXB

typedef struct {
	size_t		stksz;			/* スタック領域のサイズ */
} TSKINICTXB;

/*
 *  最高優先順位タスクへのディスパッチ
 *
 *  dispatchは，タスクコンテキストから呼び出されたサービスコール処理か
 *  ら呼び出すべきもので，タスクコンテキスト・CPUロック状態・ディスパッ
 *  チ許可状態・（モデル上の）割込み優先度マスク全解除状態で呼び出さな
 *  ければならない．
 */
extern void	dispatch(void);

/*
 *  非タスクコンテキストからのディスパッチ要求
 */
#define request_dispatch_retint()

/*
 *  ディスパッチャの動作開始
 *
 *  start_dispatchをreturnにマクロ定義することで，カーネルの初期化完了
 *  後に，sta_kerからmainにリターンさせる．
 */
#define start_dispatch()	 return

/*
 *  現在のコンテキストを捨ててディスパッチ
 *
 *  exit_and_dispatchは，ext_tskから呼び出すべきもので，タスクコンテキ
 *  スト・CPUロック状態・ディスパッチ許可状態・（モデル上の）割込み優先
 *  度マスク全解除状態で呼び出さなければならない．
 */
extern void	exit_and_dispatch(void);

/*
 *  ディスパッチャの開始（メインスレッド用）
 */
extern void start_dispatch_task(void);

/*
 *  カーネルの終了処理の呼出し
 *
 *  call_exit_kernelは，カーネルの終了時に呼び出すべきもので，非タスク
 *  コンテキストに切り換えて，カーネルの終了処理（exit_kernel）を呼び出
 *  す．
 */
extern void call_exit_kernel(void) NoReturn;

/*
 *  タスクコンテキストの初期化
 *
 *  タスクが休止状態から実行できる状態に移行する時に呼ばれる．この時点
 *  でスタック領域を使ってはならない．
 *
 *  activate_contextを，インライン関数ではなくマクロ定義としているのは，
 *  この時点ではTCBが定義されていないためである．
 */
#define activate_context(p_tcb)	terminate_thread(&((p_tcb)->tskctxb))

/*
 *  CPU例外ハンドラの汎用出入口処理
 */
extern void exc_entry_generic(int sig, siginfo_t *p_siginfo,
										void *p_ctx, EXCHDR exchdr);

/*
 *  CPU例外ハンドラの入口処理の生成マクロ
 */
#define EXC_ENTRY(excno, exchdr)	_kernel_##exchdr##_##excno

#define EXCHDR_ENTRY(excno, excno_num, exchdr)							\
void _kernel_##exchdr##_##excno(int sig,								\
									siginfo_t *p_info, void *p_ctx)		\
{																		\
	_kernel_exc_entry_generic(sig, p_info, p_ctx, exchdr);				\
}

/*
 *  割込み優先度マスクの参照（CPU例外ハンドラ用）
 */
Inline PRI
x_get_ipm(void)
{
	sigset_t	saved_sigmask;
	PRI			retval;

	pthread_sigmask(SIG_BLOCK, &sigmask_cpulock, &saved_sigmask);
	retval = t_get_ipm();
	pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
	return(retval);
}

/*
 *  カーネル管理外のCPU例外の判別
 *
 *  カーネル動作状態で実行されているCPU例外ハンドラから呼び出され，カー
 *  ネル管理外のCPU例外の時にtrue，そうでない時にfalseを返す．
 *
 *  カーネル管理外のCPU例外とは，カーネル非動作状態，カーネル内のクリ
 *  ティカルセクションの実行中，全割込みロック状態，CPUロック状態，カー
 *  ネル管理外の割込みハンドラ実行中，カーネル管理外のCPU例外ハンドラ
 *  実行中のいずれかで発生したCPU例外によって起動されるCPU例外ハンドラ
 *  である．ただし，カーネル非動作状態は，この関数を呼び出す側で判別す
 *  るため，この関数では判別する必要がない．
 *
 *  各条件は，以下の方法で判別できる．
 *    カーネル内のクリティカルセクションの実行中 → SIGUSR1がマスク
 *    全割込みロック状態 → SIGUSR1がマスク
 *    CPUロック状態 → SIGUSR1がマスク
 *    カーネル管理外の割込みハンドラ実行中 → intpri_value < TMIN_INTPRI
 *     ※ 現時点では，カーネル管理外の割込みハンドラはサポートしておらず，
 *        この条件が満たされることはない．
 *
 *  SIGUSR1がマスクされている状態で発生したCPU例外ハンドラでは，ミュー
 *  テックスをロックしてはならない（デッドロックする）ため，その条件を
 *  最初に判定する必要がある．
 */
Inline bool_t
exc_sense_nonkernel(void *p_excinf)
{
	return(sigismember(&(((ucontext_t *) p_excinf)->uc_sigmask), SIGUSR1)
											|| x_get_ipm() < TMIN_INTPRI);
}

/*
 *  CPU例外の発生した時のコンテキストと割込みのマスク状態の参照
 *
 *  CPU例外の発生した時のシステム状態が，カーネル内のクリティカルセクショ
 *  ンの実行中でなく，全割込みロック状態でなく，CPUロック状態でなく，カー
 *  ネル管理外の割込みハンドラ実行中でなく，カーネル管理外のCPU例外ハン
 *  ドラ実行中でなく，タスクコンテキストであり，割込み優先度マスクが全
 *  解除である時にtrue，そうでない時にfalseを返す．
 *
 *  各条件は，以下の方法で判別できる．
 *    最初の5条件 → カーネル管理外のCPU例外ハンドラ実行中と同様
 *    タスクコンテキスト → excpt_nest_countで判別
 *    割込み優先度マスクが全解除 → intpri_value == TIPM_ENAALL 
 *
 *  SIGUSR1がマスクされている状態で発生したCPU例外ハンドラでは，ミュー
 *  テックスをロックしてはならない（デッドロックする）ため，その条件を
 *  最初に判定する必要がある．
 */
Inline bool_t
exc_sense_intmask(void *p_excinf)
{
	return(!sigismember(&(((ucontext_t *) p_excinf)->uc_sigmask), SIGUSR1)
					&& excpt_nest_count <= 1 && x_get_ipm() == TIPM_ENAALL);
}

/*
 *  CPU例外ハンドラの設定
 *
 *  シグナル番号excnoのシグナルハンドラをexc_entryに設定する．
 */
extern void define_exc(EXCNO excno, FP exc_entry);

/*
 *  非タスクコンテキスト用のスタックに関する定義
 */
#define DEFAULT_ISTKSZ		0
#define DEFAULT_ISTK		NULL

/*
 *  POSIX依存部の初期化
 */
extern void	posix_initialize(void);

/*
 *  動的生成機能拡張パッケージのための定義
 */
#ifdef TOPPERS_SUPPORT_DYNAMIC_CRE

/*
 *  タスク初期化コンテキストブロックの初期化
 */
Inline void
init_tskinictxb(TSKINICTXB *p_tskinictxb, size_t stksz, STK_T *stk)
{
	p_tskinictxb->stksz = stksz;
}

/*
 *  タスク初期化コンテキストブロックの終了時処理
 */
Inline void
term_tskinictxb(TSKINICTXB *p_tskinictxb)
{
}

/*
 *  TLSFを用いたメモリプール管理機能
 *
 *  TLSF（オープンソースのメモリ管理ライブラリ）を用いてメモリプール領
 *  域の管理を行うための定義など．
 */
#define OMIT_MEMPOOL_DEFAULT

#include "tlsf.h"

Inline bool_t
initialize_mempool(MB_T *mempool, size_t size)
{
	if (init_memory_pool(size, mempool) != -1) {
		return(true);
	}
	else {
		return(false);
	}
}

Inline void *
malloc_mempool(MB_T *mempool, size_t size)
{
	return(malloc_ex(size, mempool));
}

Inline void *
aligned_alloc_mempool(MB_T *mempool, size_t alignment, size_t size)
{
	return(malloc_ex(size, mempool));
}

Inline void
free_mempool(MB_T *mempool, void *ptr)
{
	free_ex(ptr, mempool);
}

#endif /* TOPPERS_SUPPORT_DYNAMIC_CRE */
#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_POSIX_KERNEL_IMPL_H */
