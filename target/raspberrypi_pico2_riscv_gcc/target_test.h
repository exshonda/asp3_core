/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアをTOPPERSライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソ
 *  フトウェアは無保証で提供される．
 */

/*
 *  テストプログラムのターゲット依存部（RaspberryPi Pico2 RISC-V用）
 */

#ifndef TOPPERS_TARGET_TEST_H
#define TOPPERS_TARGET_TEST_H

#define STACK_SIZE (1024)

/*
 *  テストプログラムで使用する割込み（TIMER0 ALARM1．ras_intは
 *  Xh3irqの割込み強制ビットで実現＝ARM版のNVIC ISPRに相当）
 */
#define INTNO1			(RP2350_TIMER0_1_IRQn + 1)	/* 2 */
#define INTNO1_INTATR	TA_ENAINT
#define INTNO1_INTPRI	(-2)

/*
 *  INTNO1の割込み要求のクリア
 */
#ifndef TOPPERS_MACRO_ONLY
#include <sil.h>
Inline void
intno1_clear(void)
{
	sil_wrw_mem(RP2350_TIMER0_INTR, RP2350_TIMER0_INT_ALARM_1);
}
#endif /* TOPPERS_MACRO_ONLY */

/*
 *  コアで共通な定義
 */
#include "core_test.h"

#endif /* TOPPERS_TARGET_TEST_H */
