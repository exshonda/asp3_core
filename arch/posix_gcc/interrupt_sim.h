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
 *  $Id: interrupt_sim.h 1881 2026-05-18 03:52:07Z ertl-hiro $
 */

/*
 *		割込みシミュレーションモジュール（POSIX用）
 */

#ifndef TOPPERS_INTERRUPT_SIM_H
#define TOPPERS_INTERRUPT_SIM_H

#include <t_stddef.h>
#include <queue.h>
#include "thread_ctrl.h"

/* 
 *  標準の割込み管理機能の初期化を行わないための定義
 */
#define OMIT_INITIALIZE_INTERRUPT

/*
 *  割込みハンドラ初期化ブロック
 *
 *  標準の割込みハンドラ初期化ブロックに，割込み要求ライン属性と割込み
 *  優先度を追加したもの．
 */
typedef struct interrupt_handler_initialization_block {
	INHNO		inhno;			/* 割込みハンドラ番号 */
	ATR			inhatr;			/* 割込みハンドラ属性 */
	INTHDR		inthdr;			/* 割込みハンドラの起動番地 */
	ATR			intatr;			/* 割込み要求ライン属性 */
	PRI			intpri;			/* 割込み優先度 */
} INHINIB;

/*
 *  割込みハンドラ番号の数（kernel_cfg.c）
 */
extern const uint_t	tnum_def_inhno;

/*
 *  割込みハンドラ初期化ブロックのエリア（kernel_cfg.c）
 */
extern const INHINIB	inhinib_table[];

/*
 *  割込み状態
 */
#define INTR_STAT_INACTIVE	0x00U		/* アイドル */
#define INTR_STAT_PENDING	0x01U		/* ペンディング */
#define INTR_STAT_ACTIVE	0x02U		/* アクティブ */
#define INTR_STAT_ACTPEND	0x03U		/* アクティブ＆ペンディング */

/*
 *  割込み管理ブロック
 */
typedef struct interrupt_control_block {
	QUEUE			intr_queue;		/* キューにつなぐための領域 */
	const INHINIB	*p_inhinib;		/* 割込みハンドラ初期化ブロック */
	uint8_t			istat;			/* 割込み状態 */
	bool_t			disabled;		/* 割込み禁止フラグ */
	PRI				saved_intpri;	/* 割込み受付け前の割込み優先度マスク */
	THRCB			thrcb;			/* スレッド管理ブロック */
} INTRCB;

/*
 *  割込み管理ブロックのエリア
 */
extern INTRCB	intrcb_table[TMAX_INTNO + 1];

/*
 *  実行すべきスレッドの決定
 */
extern THRCB *scheduled_thread(void);

/*
 *  割込み番号の範囲の判定
 */
#define	VALID_INTNO(intno)	(0 <= (intno) && (intno) <= TMAX_INTNO)

/*
 *  割込み属性の設定のチェック
 */
Inline bool_t
check_intno_cfg(INTNO intno)
{
	return(intrcb_table[intno].p_inhinib != NULL);
}

/*
 *  割込み要求禁止フラグのセット
 */
extern void disable_int(INTNO intno);

/*
 *  割込み要求禁止フラグのクリア
 */
extern void enable_int(INTNO intno);

/*
 *  割込み要求がクリアできる状態か？
 */
Inline bool_t
check_intno_clear(INTNO intno)
{
	return(true);
}

/*
 *  割込み要求のクリア
 */
extern void clear_int(INTNO intno);

/*
 *  割込みが要求できる状態か？
 */
Inline bool_t
check_intno_raise(INTNO intno)
{
	return(true);
}

/*
 *  割込みの要求
 */
extern void raise_int(INTNO intno);

/*
 *  割込みの要求（メインスレッド用）
 */
extern void raise_int_main(INTNO intno);

/*
 *  割込み要求のチェック
 */
extern bool_t probe_int(INTNO intno);

/*
 *  割込み優先度マスクの設定
 */
extern void t_set_ipm(PRI intpri);

/*
 *  割込み優先度マスクの参照
 */
extern PRI t_get_ipm(void);

/*
 *  割込みハンドラからのリターン処理
 */
extern void return_intr(INTRCB *p_my_intrcb);

/*
 *  割込みシミュレーションモジュールの初期化
 */
extern void initialize_interrupt(void);

#endif /* TOPPERS_INTERRUPT_SIM_H */
