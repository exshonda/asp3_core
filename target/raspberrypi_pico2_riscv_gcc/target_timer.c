/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2016 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアをTOPPERSライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソ
 *  フトウェアは無保証で提供される．
 */

/*
 *  タイマドライバ（RaspberryPi Pico 2 RISC-V ASP3用）
 *  TIMER0 ALARM0 を使用して高分解能タイマを実現する．
 *  ARM版（target/raspberrypi_pico2_gcc/target_timer.c）からの流用
 *  （すべてMMIO操作＝ISA非依存）．
 */

#include "kernel_impl.h"
#include "time_event.h"
#include "target_timer.h"
#include <sil.h>

/*
 *  タイマの起動処理
 */
void target_hrt_initialize(intptr_t exinf)
{
    sil_orw(RP2350_TIMER0_INTE, RP2350_TIMER0_INT_ALARM_0);
    while (target_hrt_get_current() < 1) ;
}

/*
 *  タイマの停止処理
 */
void target_hrt_terminate(intptr_t exinf)
{
    sil_orw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_TIMER0);
}

/*
 *  タイマ割込みハンドラ
 */
void target_hrt_handler(void)
{
    sil_wrw_mem(RP2350_TIMER0_INTR, RP2350_TIMER0_INT_ALARM_0);
    signal_time();
}
