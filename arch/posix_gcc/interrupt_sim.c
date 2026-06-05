/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 * 
 *  Copyright (C) 2022-2024 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: interrupt_sim.c 1881 2026-05-18 03:52:07Z ertl-hiro $
 */

/*
 *		割込みシミュレーションモジュール（POSIX用）
 */

#include "kernel_impl.h"
#include "task.h"

/*
 *  割込み管理ブロックのエリア
 */
INTRCB	intrcb_table[TMAX_INTNO + 1];

/*
 *  割込み優先度マスク実現のための変数
 */
static PRI		intpri_value;			/* 割込み優先度マスクを表す変数 */

/*
 *  アクティブな割込みのキュー
 */
static QUEUE	active_intr_queue;

/*
 *  ペンディングしている割込みのキュー
 */
static QUEUE	pending_intr_queue;

/*
 *  実行すべきスレッド
 */
static THRCB	*p_sched_thread;

/*
 *  割込みシミュレーションモジュールを排他制御するためのミューテックス
 */
static pthread_mutex_t	intr_mutex;

/*
 *  ペンディングしている割込みの受付け判定
 *
 *  ペンディングしている割込みの中で，実行開始すべきものがあれば，その
 *  割込み管理ブロックへのポインタを返す．ない場合は，NULLを返す．
 */
static INTRCB *
scheduled_intr(void)
{
	INTRCB	*p_top_intrcb;

	if (!queue_empty(&pending_intr_queue)) {
		p_top_intrcb = (INTRCB *) pending_intr_queue.p_next;
		if (p_top_intrcb->p_inhinib->intpri < intpri_value) {
			return(p_top_intrcb);
		}
	}
	return(NULL);
}

/*
 *  ペンディングしている割込みの受付け
 */
static void
activate_intr(INTRCB *p_intrcb)
{
	/* 割込み状態の変更 */
	p_intrcb->istat = INTR_STAT_ACTIVE;
	queue_delete(&(p_intrcb->intr_queue));
	queue_insert_next(&active_intr_queue, &(p_intrcb->intr_queue));

	/* 割込み優先度マスクを設定 */
	p_intrcb->saved_intpri = intpri_value;
	intpri_value = p_intrcb->p_inhinib->intpri;
}

/*
 *  実行すべきスレッドの決定
 *
 *  ペンディングしている割込みを受け付けるべき状態の場合には，その受付
 *  け処理を行う．決定した実行すべきスレッドを，p_sched_threadに記憶し
 *  ておく．
 */
THRCB *
scheduled_thread(void)
{
	INTRCB		*p_intrcb;
	THRCB		*p_thrcb;

	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	p_intrcb = scheduled_intr();
	if (p_intrcb != NULL) {
		activate_intr(p_intrcb);
		p_thrcb = &(p_intrcb->thrcb);
	}
	else if (!queue_empty(&active_intr_queue)) {
		p_thrcb = &(((INTRCB *) active_intr_queue.p_next)->thrcb);
	}
	else {
		if (p_runtsk != NULL) {
			p_thrcb = &(p_runtsk->tskctxb);
		}
		else {
			p_thrcb = &idle_thread;
		}
	}
	p_sched_thread = p_thrcb;
	MUTEX_UNLOCK(&intr_mutex);
	return(p_thrcb);
}

/*
 *  ペンディングしている割込みのキューへの挿入
 */
static void
pending_queue_insert(INTRCB *p_intrcb)
{
	QUEUE	*p_entry;
	PRI		intpri = p_intrcb->p_inhinib->intpri;

	for (p_entry = pending_intr_queue.p_next;
				p_entry != &pending_intr_queue; p_entry = p_entry->p_next) {
		if (intpri < ((INTRCB *) p_entry)->p_inhinib->intpri) {
			break;
		}
	}
	queue_insert_prev(p_entry, &(p_intrcb->intr_queue));
}

/*
 *  割込み要求禁止フラグのセット
 */
void
disable_int(INTNO intno)
{
	INTRCB	*p_intrcb = &(intrcb_table[intno]);

	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	if (!(p_intrcb->disabled)) {
		p_intrcb->disabled = true;
		if (p_intrcb->istat == INTR_STAT_PENDING) {
			queue_delete(&(p_intrcb->intr_queue));
		}
	}
	MUTEX_UNLOCK(&intr_mutex);
}

/*
 *  割込み要求禁止フラグのクリア
 */
void
enable_int(INTNO intno)
{
	INTRCB	*p_intrcb = &(intrcb_table[intno]);

	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	if (p_intrcb->disabled) {
		p_intrcb->disabled = false;
		if (p_intrcb->istat == INTR_STAT_PENDING) {
			pending_queue_insert(p_intrcb);
		}
	}
	MUTEX_UNLOCK(&intr_mutex);
}

/*
 *  割込み要求のクリア
 */
void
clear_int(INTNO intno)
{
	INTRCB	*p_intrcb = &(intrcb_table[intno]);

	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	switch (p_intrcb->istat) {
	case INTR_STAT_PENDING:
		p_intrcb->istat = INTR_STAT_INACTIVE;
		if (!(p_intrcb->disabled)) {
			queue_delete(&(p_intrcb->intr_queue));
		}
		break;
	case INTR_STAT_ACTPEND:
		p_intrcb->istat = INTR_STAT_ACTIVE;
		break;
	}
	MUTEX_UNLOCK(&intr_mutex);
}

/*
 *  割込みの要求
 */
void
raise_int(INTNO intno)
{
	INTRCB	*p_intrcb = &(intrcb_table[intno]);

	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	switch (p_intrcb->istat) {
	case INTR_STAT_INACTIVE:
		p_intrcb->istat = INTR_STAT_PENDING;
		if (!(p_intrcb->disabled)) {
			pending_queue_insert(p_intrcb);
		}
		break;
	case INTR_STAT_ACTIVE:
		p_intrcb->istat = INTR_STAT_ACTPEND;
		break;
	}
	MUTEX_UNLOCK(&intr_mutex);
}

/*
 *  割込みの要求（メインスレッド用）
 */
void
raise_int_main(INTNO intno)
{
	INTRCB		*p_intrcb = &(intrcb_table[intno]);
	sigset_t	saved_sigmask;

	/*
	 *  intr_mutexをロックする前に，すべてのシグナルをマスクする．
	 */
	pthread_sigmask_blockall(&saved_sigmask);

	MUTEX_LOCK(&intr_mutex);
	switch (p_intrcb->istat) {
	case INTR_STAT_INACTIVE:
		p_intrcb->istat = INTR_STAT_PENDING;
		if (!(p_intrcb->disabled)) {
			pending_queue_insert(p_intrcb);
			if (p_intrcb->p_inhinib->intpri < intpri_value) {
				/*
				 *  要求された割込みを受け付けるべき状態の場合は，実行
				 *  状態すべきスレッドにプリエンプトを要求する．
				 */
				preempt_thread(p_sched_thread);
			}
		}
		break;
	case INTR_STAT_ACTIVE:
		p_intrcb->istat = INTR_STAT_ACTPEND;
		break;
	}
	MUTEX_UNLOCK(&intr_mutex);

	pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL);
}

/*
 *  割込み要求のチェック
 */
bool_t
probe_int(INTNO intno)
{
	INTRCB	*p_intrcb = &(intrcb_table[intno]);
	bool_t	retval;

	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	switch (p_intrcb->istat) {
	case INTR_STAT_PENDING:
	case INTR_STAT_ACTPEND:
		retval = true;
		break;
	default:
		retval = false;
		break;
	}
	MUTEX_UNLOCK(&intr_mutex);
	return(retval);
}

/*
 *  割込み優先度マスクの設定
 */
void
t_set_ipm(PRI intpri)
{
	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	intpri_value = intpri;
	MUTEX_UNLOCK(&intr_mutex);
}

/*
 *  割込み優先度マスクの参照
 */
PRI
t_get_ipm(void)
{
	PRI		retval;

	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);
	retval = intpri_value;
	MUTEX_UNLOCK(&intr_mutex);
	return(retval);
}

/*
 *  割込みハンドラからのリターン処理
 */
void
return_intr(INTRCB *p_my_intrcb)
{
	assert(sense_lock());

	MUTEX_LOCK(&intr_mutex);

	/* 割込み状態の変更 */
	queue_delete(&(p_my_intrcb->intr_queue));
	switch (p_my_intrcb->istat) {
	case INTR_STAT_ACTIVE:
		p_my_intrcb->istat = INTR_STAT_INACTIVE;
		break;
	case INTR_STAT_ACTPEND:
		p_my_intrcb->istat = INTR_STAT_PENDING;
		if (!(p_my_intrcb->disabled)) {
			pending_queue_insert(p_my_intrcb);
		}
		break;
	default:
		assert(false);
		break;
	}

	/* 割込み優先度マスクを戻す */
	intpri_value = p_my_intrcb->saved_intpri;

	MUTEX_UNLOCK(&intr_mutex);
}

/*
 *  割込み管理モジュールの初期化
 */
void
initialize_interrupt(void)
{
	uint_t			i;
	INTRCB			*p_intrcb;
	const INHINIB	*p_inhinib;

	/*
	 *  ミューテックスの初期化
	 */
	pthread_mutex_init(&intr_mutex, NULL);

	/*
	 *  割込み管理ブロックの初期化
	 */
	for (i = 0; i <= TMAX_INTNO; i++) {
		intrcb_table[i].p_inhinib = NULL;
	}
	for (i = 0; i < tnum_def_inhno; i++) {
		p_inhinib = &(inhinib_table[i]);
		p_intrcb = &(intrcb_table[p_inhinib->inhno]);
		p_intrcb->p_inhinib = p_inhinib;
		p_intrcb->istat = INTR_STAT_INACTIVE;
		p_intrcb->disabled = ((p_inhinib->intatr & TA_ENAINT) == 0);

		/* 割込みスレッドの初期化 */
		init_thrcb(&(p_intrcb->thrcb), THREAD_TYPE_INTR);
		p_intrcb->thrcb.start_routine = int_entry;
		p_intrcb->thrcb.arg = p_intrcb;
	}

	/*
	 *  割込み優先度マスク実現のための変数の初期化
	 */
	intpri_value = TIPM_ENAALL;

	/*
	 *  割込みキューの初期化
	 */
	queue_initialize(&active_intr_queue);
	queue_initialize(&pending_intr_queue);
}
