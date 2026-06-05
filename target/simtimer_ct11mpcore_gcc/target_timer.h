/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2007-2024 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: target_timer.h 1881 2026-05-18 03:52:07Z ertl-hiro $
 */

/*
 *		タイマドライバ
 *		（CT11MPCore＋タイマドライバシミュレータ用）
 */

#ifndef TOPPERS_TARGET_TIMER_H
#define TOPPERS_TARGET_TIMER_H

#include "kernel/kernel_impl.h"
#include "ct11mpcore.h"

/*
 *  タイマ割込みハンドラ登録のための定数
 */
#define INHNO_TIMER		MPCORE_IRQNO_TMR	/* 割込みハンドラ番号 */
#define INTNO_TIMER		MPCORE_IRQNO_TMR	/* 割込み番号 */
#define INTPRI_TIMER	(TMAX_INTPRI - 1)	/* 割込み優先度 */
#define INTATR_TIMER	TA_NULL				/* 割込み属性 */

#ifdef TOPPERS_SUPPORT_OVRHDR
/*
 *  オーバランタイマ割込みハンドラ登録のための定数
 */
#define INHNO_OVRTIMER		EB_IRQNO_TIMER23		/* 割込みハンドラ番号 */
#define INTNO_OVRTIMER		EB_IRQNO_TIMER23		/* 割込み番号 */
#define INTPRI_OVRTIMER		INTPRI_TIMER			/* 割込み優先度 */
#define INTATR_OVRTIMER		TA_EDGE					/* 割込み属性 */

#endif /* TOPPERS_SUPPORT_OVRHDR */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  シミュレートされた高分解能タイマ割込みの要求
 */
extern void simtim_target_raise_hrt_int(void);

#ifdef TOPPERS_SUPPORT_OVRHDR
/*
 *  シミュレートされたオーバランタイマ割込みの要求
 */
extern void simtim_target_raise_ovr_int(void);

/*
 *  シミュレートされたオーバランタイマ割込み要求のクリア
 */
extern void simtim_target_clear_ovr_int(void);

#endif /* TOPPERS_SUPPORT_OVRHDR */
#endif /* TOPPERS_MACRO_ONLY */

/*
 *  タイマドライバシミュレータ
 */
#include "sim_timer.h"

#endif /* TOPPERS_TARGET_TIMER_H */
