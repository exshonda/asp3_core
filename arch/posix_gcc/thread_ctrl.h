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
 *  $Id: thread_ctrl.h 1720 2022-10-23 04:07:15Z ertl-hiro $
 */

/*
 *		POSIX用スレッド制御モジュール
 */

#ifndef THREAD_CTRL_H
#define THREAD_CTRL_H

#include <pthread.h>
#include <t_stddef.h>

/*
 *  スレッドタイプの定義
 */
#define THREAD_TYPE_TASK		0U		/* タスク */
#define THREAD_TYPE_INTR		1U		/* 割込みハンドラ */
#define THREAD_TYPE_IDLE		2U		/* アイドル処理 */

/*
 *  スレッド状態の定義
 */
#define THREAD_STAT_RUN			0U		/* 実行状態 */
#define THREAD_STAT_SUSPEND		1U		/* 中断状態 */
#define THREAD_STAT_TERMINATE	2U		/* 終了状態（初期状態）*/

/*
 *  スレッド管理ブロック
 */
typedef struct thread_control_block {
	uint_t			type;			/* スレッドタイプ */
	uint_t			state;			/* スレッド状態 */
	pthread_t		thrid;			/* スレッドID */
	pthread_cond_t	thrcond;		/* スレッド用の条件変数 */
	size_t			stacksize;		/* スタックサイズ */
	void			*(*start_routine)(void *);	/* スレッドのメイン関数 */
	void			*arg;			/* メイン関数に渡すパラメータ */
} THRCB;

/*
 *  スレッド管理ブロックの初期化
 *
 *  スレッドのメイン関数（start_routine），メイン関数に渡すパラメータ
 *  （arg），スタックサイズ（stacksize，デフォルト値以外を設定する場合
 *  のみ）は，この関数を呼び出す側で初期化する必要がある．
 */
extern void init_thrcb(THRCB *p_thrcb, uint_t type);

/*
 *  自スレッドを中断して指定したスレッドへ切換え
 *
 *  指定したスレッドが自スレッド以外の場合は，自スレッドを中断状態にし
 *  て，指定したスレッドを実行状態にする．指定したスレッドが自スレッド
 *  の場合は，何もせずにリターンする．
 */
extern void suspend_thread(THRCB *p_thrcb);

/*
 *  自スレッドを終了して指定したスレッドへ切換え
 *
 *  指定したスレッドが自スレッド以外の場合は，自スレッドを終了状態にし
 *  て，指定したスレッドを実行状態にする．指定したスレッドが自スレッド
 *  の場合は，自スレッドを終了させ，再度スレッドを生成して実行状態にす
 *  る．
 */
extern void exit_thread(THRCB *p_thrcb);

/*
 *  指定したスレッドの終了
 *
 *  指定したスレッドが中断状態であれば，終了状態にする．指定したスレッ
 *  ドが実行状態の場合（自スレッドの場合）は，何もしない．
 */
extern void terminate_thread(THRCB *p_thrcb);

/*
 *  スレッドのプリエンプトの要求
 *
 *  指定したスレッドが終了状態でなければ，SIGUSR1シグナルを送る．メイ
 *  ンスレッドが呼び出すことを想定している．
 */
extern void preempt_thread(THRCB *p_thrcb);

/*
 *  スレッドディスパッチャの開始（メインスレッド用）
 *
 *  カーネルの初期化後にディスパッチャの実行を開始するために，メインス
 *  レッドが呼び出すための関数．最初に実行すべきスレッドをp_thrcbに渡
 *  す．
 */
extern void start_dispatch_thread(THRCB *p_thrcb);

/*
 *  スレッド操作モジュールの初期化
 *
 *  カーネルの初期化時にスレッド操作モジュールを初期化するために，メイ
 *  ンスレッドが呼び出すための関数．
 */
extern void initialize_thread_ctrl(void);

#endif /* THREAD_CTRL_H */
