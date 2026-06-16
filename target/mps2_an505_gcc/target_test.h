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
 *  テストプログラムのターゲット依存定義（ARM MPS2-AN505 用）
 */

#ifndef TOPPERS_TARGET_TEST_H
#define TOPPERS_TARGET_TEST_H

#define STACK_SIZE (1024)

/*
 *  int1（割込み管理機能）テスト用の割込み定義
 *
 *  QEMU mps2-an505（AN505・numirq=92）でデバイスに接続されていない
 *  予備の外部IRQ 60（→ NVIC例外番号 60+16=76）をソフト割込み源として使う
 *  （デバイスIRQは 32〜58 に集中＝uart/eth/dma/spi・uart_overflow=47）．
 *  arm_m は ras_int / prb_int をサポートし，NVIC のソフトpend（ISPR）で
 *  発生・ハンドラ入口で自動クリアされるため intno1_clear() は空でよい．
 */
#define INTNO1			(60U + 16U)		/* 予備NVIC IRQ60 → INTNO 76 */
#define INTNO1_INTATR	TA_ENAINT
#define INTNO1_INTPRI	(-2)
#define intno1_clear()

/*
 *  コア依存モジュール（ARM-M 用）
 */
#include "core_test.h"

#endif /* TOPPERS_TARGET_TEST_H */
