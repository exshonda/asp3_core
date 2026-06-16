/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
 *  トウェアは無保証で提供される．
 *
 */

/*
 *  テストプログラムのターゲット依存定義（ARM MPS2-AN386 用）
 */

#ifndef TOPPERS_TARGET_TEST_H
#define TOPPERS_TARGET_TEST_H

#define STACK_SIZE (1024)

/*
 *  int1（割込み管理機能）テスト用の割込み定義
 *
 *  QEMU mps2-an386（AN386・num-irq=32）でデバイスに接続されていない
 *  予備の外部IRQ 30（→ NVIC例外番号 30+16=46）をソフト割込み源として使う
 *  （hw/arm/mps2.c の case FPGA_AN386：デバイスIRQは uart=0〜5,18〜21／
 *  uart_overflow=12／timer=8〜11／spi=11,24／adc=22／eth=13 に分布し，
 *  30 は未接続）．arm_m は ras_int / prb_int をサポートし，NVIC のソフト
 *  pend（ISPR）で発生・ハンドラ入口で自動クリアされるため intno1_clear()
 *  は空でよい．
 */
#define INTNO1			(30U + 16U)		/* 予備NVIC IRQ30 → INTNO 46 */
#define INTNO1_INTATR	TA_ENAINT
#define INTNO1_INTPRI	(-2)
#define intno1_clear()

/*
 *  コア依存モジュール（ARM-M 用）
 */
#include "core_test.h"

#endif /* TOPPERS_TARGET_TEST_H */
