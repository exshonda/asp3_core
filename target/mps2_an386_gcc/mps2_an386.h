/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
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
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこと．
 *    (a) 再配布に伴うドキュメントに，上記の著作権表示，この利用条件およ
 *        び下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを免
 *      責すること．
 *
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的に
 *  対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェア
 *  の利用により直接的または間接的に生じたいかなる損害に関しても，その
 *  責任を負わない．
 *
 */

/*
 *  ARM MPS2+ (AN386 / レガシー CMSDK, シングル Cortex-M4) サポートモジュール
 *
 *  本ファイルは，QEMU の "mps2-an386" マシンを対象とする．AN386 は
 *  ARMv7-M（Cortex-M4・非 TrustZone）の FPGA イメージで，QEMU では
 *  hw/arm/mps2.c（レガシー CMSDK 系．AN505/AN521 の IoTKit/SSE を用いる
 *  hw/arm/mps2-tz.c とは別系統）でモデル化される．ハードウェア資源の
 *  アドレス・割込み番号は QEMU の hw/arm/mps2.c の case FPGA_AN386 に基づく．
 */

#ifndef TOPPERS_MPS2_AN386_H
#define TOPPERS_MPS2_AN386_H

/*
 *  コアのクロック周波数
 *
 *  QEMU の AN386 マシンの sysclk は 25MHz（mps2.c の
 *  #define SYSCLK_FRQ 25000000）．SysTick はプロセッサクロックで駆動され，
 *  APB ペリフェラル（UART）のクロックも同一（pclk-frq = SYSCLK_FRQ）である．
 */
#define CPU_CLOCK_HZ    25000000

/*
 *  割込み番号の最大値
 *
 *  AN386 の NVIC 外部割込みは 0〜31（mps2.c の case FPGA_AN386：
 *  num-irq = 32）．例外番号（INTNO）はこれに 16 を加えたもの．
 */
#define TMAX_INTNO      (32 + 16)

/*
 *  微少時間待ち（sil_dly_nse）のための定義（本来は SIL のターゲット依存部）
 *
 *  QEMU はサイクル精度のエミュレーションを行わないため，値は目安である．
 */
#define SIL_DLY_TIM1    10
#define SIL_DLY_TIM2    10

/*
 *  CMSDK APB UART
 *
 *  AN386（レガシー CMSDK）では UART0 のベース番地は 0x40004000．
 *  AN505（IoTKit）のセキュアエイリアス 0x40200000 とは異なる．
 *  割込みは TX / RX が個別の NVIC 線に割り当てられ，combined 割込みは
 *  存在しない（mps2.c の uartirq[] = {0, 2, 4, 18, 20}：RX 番号．TX は
 *  常にその +1）．したがって UART0 は RX=IRQ0 / TX=IRQ1．
 */
#define MPS2_AN386_UART0_BASE           0x40004000
#define MPS2_AN386_UART1_BASE           0x40005000
#define MPS2_AN386_UART2_BASE           0x40006000

#define MPS2_AN386_UART0_RX_IRQn        0
#define MPS2_AN386_UART0_TX_IRQn        1
#define MPS2_AN386_UART1_RX_IRQn        2
#define MPS2_AN386_UART1_TX_IRQn        3
#define MPS2_AN386_UART2_RX_IRQn        4
#define MPS2_AN386_UART2_TX_IRQn        5

/*
 *  CMSDK APB UART レジスタ（ベースアドレスからのオフセット）
 */
#define CMSDK_UART_DATA(base)       ((uint32_t)(base) + 0x00U)  /* データ */
#define CMSDK_UART_STATE(base)      ((uint32_t)(base) + 0x04U)  /* ステータス */
#define CMSDK_UART_CTRL(base)       ((uint32_t)(base) + 0x08U)  /* 制御 */
#define CMSDK_UART_INTSTATUS(base)  ((uint32_t)(base) + 0x0CU)  /* 割込みステータス／クリア */
#define CMSDK_UART_BAUDDIV(base)    ((uint32_t)(base) + 0x10U)  /* ボーレート分周比 */

/*
 *  STATE レジスタのビット
 */
#define CMSDK_UART_STATE_TXFULL     0x01U   /* 送信バッファフル */
#define CMSDK_UART_STATE_RXFULL     0x02U   /* 受信バッファフル */
#define CMSDK_UART_STATE_TXOVRRUN   0x04U   /* 送信オーバラン */
#define CMSDK_UART_STATE_RXOVRRUN   0x08U   /* 受信オーバラン */

/*
 *  CTRL レジスタのビット
 */
#define CMSDK_UART_CTRL_TXEN        0x01U   /* 送信許可 */
#define CMSDK_UART_CTRL_RXEN        0x02U   /* 受信許可 */
#define CMSDK_UART_CTRL_TXINTEN     0x04U   /* 送信割込み許可 */
#define CMSDK_UART_CTRL_RXINTEN     0x08U   /* 受信割込み許可 */

/*
 *  INTSTATUS／INTCLEAR レジスタのビット（書込み 1 でクリア）
 */
#define CMSDK_UART_INT_TX           0x01U   /* 送信割込み */
#define CMSDK_UART_INT_RX           0x02U   /* 受信割込み */
#define CMSDK_UART_INT_TXOVRRUN     0x04U   /* 送信オーバラン割込み */
#define CMSDK_UART_INT_RXOVRRUN     0x08U   /* 受信オーバラン割込み */

#endif /* TOPPERS_MPS2_AN386_H */
