/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2005-2014 by Embedded and Real-Time Systems Laboratory
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
 *  @(#) $Id: start_imxrt600.S 1799 2023-04-01 00:50:30Z ertl-komori $
 */

/*
 * カーネル用のスタートアップモジュール(I.MX RT600用)
 */
#define TOPPERS_MACRO_ONLY
#define UINT_C(val)        (val)        /* uint_t型の定数を作るマクロ */
#define ULONG_C(val)    (val)        /* ulong_t型の定数を作るマクロ */
#define CAST(type, val)    (val)        /* 型キャストを行うマクロ */

#include "kernel_impl.h"
#include "arm_m.h"
#include "target_asm.inc"

/*
 * スタートアップルーチン
 *
 * Threadモードで呼び出されることを前提としてる．
 */
    ATEXT
    AALIGN(2)
    ATHUMB(_kernel_start)
    AGLOBAL(_kernel_start)
ALABEL(_kernel_start)
    cpsid f                 /* 割込みロック状態へ */

#ifdef INIT_MSP
    /*
     * MSPの設定
     */
    movs r0, #CONTROL_MSP  /* MSPを有効に  */
    msr  control, r0
    isb                    /* control の操作後に必要 */
    ldr  r0,=_kernel_istkpt
    ldr  r1,[r0]
    msr  msp, r1
#endif /* INIT_MSP */

    /*
     * hardware_init_hookの呼び出し （0でない場合）
     *
     * ターゲットハードウェアに依存して必要な初期化処理がある場合
     * は，hardware_init_hook という関数を用意する
     */
    bl   hardware_init_hook

ALABEL(start_1)
#ifndef OMIT_BSS_INIT
    /*
     * BSSセクション初期化
     */
    movs r0, #0
    ldr  r1, =BSS_TABLE_START
    ldr  r2, =BSS_TABLE_END
ALABEL(start_2)
    cmp  r1, r2
    bhs  start_4
    ldr  r3, [r1]
    adds r1, #4
    ldr  r4, [r1]
    adds r1, #4
    cmp  r4, #0
    bls  start_2
ALABEL(start_3)
    str  r0, [r3]
    adds r3, #4
    subs r4, #4
    cmp  r4, #0
    bhi  start_3
    b    start_2
#endif /* OMIT_BSS_INIT */


ALABEL(start_4)
#ifndef OMIT_DATA_INIT
    /*
     * DATAセクション初期化
     */
    ldr  r1, =DATA_TABLE_START
    ldr  r2, =DATA_TABLE_END
ALABEL(start_5)
    cmp  r1, r2
    bhs  start_7
    ldr  r3, [r1]
    adds r1, #4
    ldr  r4, [r1]
    adds r1, #4
    ldr  r5, [r1]
    adds r1, #4
    cmp  r5, #0
    bls  start_5
ALABEL(start_6)
    ldr  r0, [r3]
    str  r0, [r4]
    adds r3, #4
    adds r4, #4
    subs r5, #4
    cmp  r5, #0
    bhi  start_6
    b    start_5
#endif /* OMIT_DATA_INIT */

    /*
     * software_init_hook を呼出し（0 でない場合）
     *
     * ソフトウェア環境（特にライブラリ）に依存して必要な初期化処
     * 理がある場合は，software_init_hook という関数を用意すれば
     * よい．
     */
ALABEL(start_7)
    bl   software_init_hook

ALABEL(start_8)
    bl   sta_ker

ALABEL(l_end)
    b    l_end

/*
 *  デフォルトのhardware_init_hook（weak定義）
 */
    ATEXT
    AALIGN(2)
    AWEAK(hardware_init_hook)
ALABEL(hardware_init_hook)
    bx   lr

/*
 *  デフォルトのsoftware_init_hook（weak定義）
 */
    ATEXT
    AALIGN(2)
    AWEAK(software_init_hook)
ALABEL(software_init_hook)
    bx   lr
