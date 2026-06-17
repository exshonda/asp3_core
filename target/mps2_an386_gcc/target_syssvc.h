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
 *  システムサービスのターゲット依存部（ARM MPS2-AN386 用）
 */
#ifndef TOPPERS_TARGET_SYSSVC_H
#define TOPPERS_TARGET_SYSSVC_H

#ifdef TOPPERS_OMIT_TECS

#include "mps2_an386.h"

/*
 *  起動メッセージのターゲットシステム名
 */
#define TARGET_NAME "ARM MPS2-AN386"

/*
 *  システムログの低レベル出力のための文字出力
 */
extern void target_fput_log(char c);

/*
 *  シリアルポートの数
 */
#define TNUM_PORT		1		/* サポートするシリアルポートの数 */

/*
 *  SIOドライバで使用するUARTに関する設定
 */
#define SIO_UART_BASE		MPS2_AN386_UART0_BASE	/* UARTのベース番地 */
#define SIO_UART_BAUDRATE	115200U					/* ボーレート */
#define SIO_UART_BAUDDIV	(CPU_CLOCK_HZ / SIO_UART_BAUDRATE)
												/* ボーレート分周比の設定値 */

/*
 *  SIO割込みを登録するための定義
 *
 *  AN386（レガシー CMSDK）の UART には combined 割込みが無く，TX / RX が
 *  別々の NVIC 線に割り当てられる（UART0：RX=IRQ0／TX=IRQ1）．SIO ドライバ
 *  は受信通知（RX）と送信可能通知（TX）の双方を必要とするため，同一の ISR
 *  （sio_isr）を RX・TX 両割込みに登録する．ISR は INTSTATUS を読んで両要因
 *  を処理・クリアするため，どちらの線で起動されても正しく動作する．
 */
#define INTNO_SIO_RX	(MPS2_AN386_UART0_RX_IRQn + 16)
												/* UART RX割込み番号 */
#define INTNO_SIO_TX	(MPS2_AN386_UART0_TX_IRQn + 16)
												/* UART TX割込み番号 */
#define ISRPRI_SIO		1						/* UART ISR優先度 */
#define INTPRI_SIO		(-2)					/* UART割込み優先度 */
#define INTATR_SIO		TA_NULL					/* UART割込み属性 */

/*
 *  低レベル出力で使用する SIO ポート
 */
#define SIOPID_FPUT 1

#endif /* TOPPERS_OMIT_TECS */

/*
 *  システムサービスのコア依存部の読み込み（性能評価の時間源等）
 *
 *  QEMU は DWT CYCCNT を実装しないため，USE_ARM_DWT_PMCNT は指定せず
 *  histogram は既定の fch_hrt を用いる（core_syssvc.h は何も上書きしない）．
 */
#include "core_syssvc.h"

#endif /* TOPPERS_TARGET_SYSSVC_H */
