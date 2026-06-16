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
 *  ARM MPS3 (AN547 / SSE-300, シングル Cortex-M55) サポートモジュール
 *
 *  本ファイルは，QEMU の "mps3-an547" マシンを対象とする．ハードウェア
 *  資源のアドレス・割込み番号は QEMU の hw/arm/mps2-tz.c および
 *  hw/arm/armsse.c に基づく（mps2_an505_gcc から派生．AN505=IoTKit/M33 に
 *  対し，AN547=SSE-300/Cortex-M55(ARMv8.1-M)＋MPS3 メモリマップ）．
 */

#ifndef TOPPERS_MPS3_AN547_H
#define TOPPERS_MPS3_AN547_H

/*
 *  コアのクロック周波数
 *
 *  QEMU の AN547 マシンの sysclk(MAINCLK) は 32MHz（mps2-tz.c の
 *  mps3tz_an547_class_init：mmc->sysclk_frq = 32 * 1000 * 1000）．SSE-300 は
 *  systick refclk を配線しないため（armsse.c：「The SSE subsystems do not
 *  wire up a systick refclk」），SysTick はプロセッサクロック（MAINCLK=
 *  cpuclk=32MHz）で駆動する（target_timer.c は CLKSOURCE=processor を設定）．
 *  なお UART(APB) のクロックは QEMU 上は apb_periph_frq=25MHz だが，QEMU の
 *  CMSDK UART は BAUDDIV をタイミングに用いないため SIO 出力には影響しない．
 */
#define CPU_CLOCK_HZ    32000000

/*
 *  割込み番号の最大値
 *
 *  SSE-300/AN547 の NVIC 外部割込みは 0〜95（EXP_NUMIRQ=96，mps2-tz.c の
 *  mmc->numirq = 96）．内部 32 ＋ 拡張 96 ＝ 128．例外番号はこれに 16 を
 *  加えたもの．
 */
#define TMAX_INTNO      (128 + 16)

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
 *  AN547 では UART0 のセキュアエイリアスが 0x49303000 に配置される
 *  （mps2-tz.c の an547_ppcs：apb_ppcexp2 の "uart0"．AN505 の 0x40200000
 *  とは異なる）．割込みは rx / tx / combined の 3 本があり（make_uart は
 *  irqs[] を rx, tx, combined の順で受ける．an547_ppcs uart0 = { 33, 34,
 *  43 }），combined（任意の要因で発火）を SIO ドライバの割込みに用いる．
 */
#define MPS3_AN547_UART0_BASE           0x49303000
#define MPS3_AN547_UART1_BASE           0x49304000
#define MPS3_AN547_UART2_BASE           0x49305000

#define MPS3_AN547_UART0_RX_IRQn        33
#define MPS3_AN547_UART0_TX_IRQn        34
#define MPS3_AN547_UART0_COMBINED_IRQn  43
#define MPS3_AN547_UART1_RX_IRQn        35
#define MPS3_AN547_UART1_TX_IRQn        36
#define MPS3_AN547_UART1_COMBINED_IRQn  44

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

#endif /* TOPPERS_MPS3_AN547_H */
