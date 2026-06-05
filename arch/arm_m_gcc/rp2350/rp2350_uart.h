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
 *		RP2350 UART用 簡易SIOドライバ（非TECS版専用）
 *
 *  レジスタの定義は RP2350.h に置かれている．
 */

#ifndef TOPPERS_RP2350_UART_H
#define TOPPERS_RP2350_UART_H

#ifdef TOPPERS_OMIT_TECS

#include <sil.h>
#include "RP2350.h"

/*
 *  SIOポート数の定義
 */
#define TNUM_SIOP		1		/* サポートするSIOポートの数 */

/*
 *  コールバックルーチンの識別番号
 */
#define SIO_RDY_SND		1U		/* 送信可能コールバック */
#define SIO_RDY_RCV		2U		/* 受信通知コールバック */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  SIOポート管理ブロックの定義
 */
typedef struct sio_port_control_block	SIOPCB;

/*
 *  プリミティブな送信／受信関数
 */

/*
 *  受信バッファに文字があるか？
 */
Inline bool_t
rp2350_uart_getready(uintptr_t base)
{
	return((sil_rew_mem(RP2350_UART_FR(base)) & RP2350_UART_FR_RXFE) == 0U);
}

/*
 *  送信バッファに空きがあるか？
 */
Inline bool_t
rp2350_uart_putready(uintptr_t base)
{
	return((sil_rew_mem(RP2350_UART_FR(base)) & RP2350_UART_FR_TXFF) == 0U);
}

/*
 *  受信した文字の取出し
 */
Inline char
rp2350_uart_getchar(uintptr_t base)
{
	return((char) sil_rew_mem(RP2350_UART_DR(base)));
}

/*
 *  送信する文字の書込み
 */
Inline void
rp2350_uart_putchar(uintptr_t base, char c)
{
	sil_wrw_mem(RP2350_UART_DR(base), (uint32_t) c);
}

/*
 *  シリアルインタフェースドライバに提供する機能
 */

/*
 *  SIOドライバの初期化
 */
extern void		rp2350_uart_initialize(void);

/*
 *  SIOドライバの終了処理
 */
extern void		rp2350_uart_terminate(void);

/*
 *  SIOの割込みサービスルーチン
 */
extern void		rp2350_uart_isr(ID siopid);

/*
 *  SIOポートのオープン
 */
extern SIOPCB	*rp2350_uart_opn_por(ID siopid, EXINF exinf);

/*
 *  SIOポートのクローズ
 */
extern void		rp2350_uart_cls_por(SIOPCB *siopcb);

/*
 *  SIOポートへの文字送信
 */
extern bool_t	rp2350_uart_snd_chr(SIOPCB *siopcb, char c);

/*
 *  SIOポートからの文字受信
 */
extern int_t	rp2350_uart_rcv_chr(SIOPCB *siopcb);

/*
 *  SIOポートからのコールバックの許可
 */
extern void		rp2350_uart_ena_cbr(SIOPCB *siopcb, uint_t cbrtn);

/*
 *  SIOポートからのコールバックの禁止
 */
extern void		rp2350_uart_dis_cbr(SIOPCB *siopcb, uint_t cbrtn);

/*
 *  SIOポートからの送信可能コールバック
 */
extern void		rp2350_uart_irdy_snd(EXINF exinf);

/*
 *  SIOポートからの受信通知コールバック
 */
extern void		rp2350_uart_irdy_rcv(EXINF exinf);

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_OMIT_TECS */
#endif /* TOPPERS_RP2350_UART_H */
