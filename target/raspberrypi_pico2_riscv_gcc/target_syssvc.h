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
 *  システムサービスのターゲット依存部（RaspberryPi Pico2 RISC-V用）
 */

#ifndef TOPPERS_TARGET_SYSSVC_H
#define TOPPERS_TARGET_SYSSVC_H

#ifdef TOPPERS_OMIT_TECS

#include "rpi_pico.h"
#include "RP2350.h"

/*
 *  ターゲットシステムのハードウェア資源の定義
 */
#define TARGET_NAME "RaspberryPi Pico2 <RISC-V Hazard3>"

/*
 *  低レベル出力
 */
extern void target_fput_log(char c);

/*
 *  使用するUART（UART0．TX:GP0/RX:GP1＝ARM版と同一）
 */
#define SIO_UART_BASE		RP2350_UART0_BASE		/* UARTのベース番地 */
#define SIO_UART_RESET_BIT	RP2350_RESETS_RESET_UART0	/* リセットビット */

/*
 *  UART割込み（ASP3の割込み番号INTNO = RP2350のIRQ番号 + 1）
 */
#define INTNO_SIO		(RP2350_UART0_IRQn + 1)		/* UART割込み番号 */
#define ISRPRI_SIO		1							/* UART ISR優先度 */
#define INTPRI_SIO		(-2)						/* UART割込み優先度 */
#define INTATR_SIO		TA_NULL						/* UART割込み属性 */

/*
 *  シリアルポート数の定義
 */
#define TNUM_PORT	1

/*
 *  低レベル出力用のSIOポートID
 */
#define SIOPID_FPUT 1

#endif /* TOPPERS_OMIT_TECS */

/*
 *  コアで共通な定義
 */
#include "core_syssvc.h"

#endif /* TOPPERS_TARGET_SYSSVC_H */
