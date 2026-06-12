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
 *		I.MX RT600 Flexcomm USART用 簡易SIOドライバ（非TECS版専用）
 *
 *  レジスタの定義は IMXRT600.h に置かれている．
 */

#ifndef TOPPERS_IMXRT600_USART_H
#define TOPPERS_IMXRT600_USART_H

#ifdef TOPPERS_OMIT_TECS

#include <sil.h>
#include "IMXRT600.h"

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
 *  受信FIFOに文字があるか？
 */
Inline bool_t
imxrt600_usart_getready(uintptr_t base)
{
	return((sil_rew_mem(IMXRT600_USART_FIFOSTAT(base))
						& IMXRT600_USART_FIFOSTAT_RXNOTEMPTY) != 0U);
}

/*
 *  送信FIFOに空きがあるか？
 */
Inline bool_t
imxrt600_usart_putready(uintptr_t base)
{
	return((sil_rew_mem(IMXRT600_USART_FIFOSTAT(base))
						& IMXRT600_USART_FIFOSTAT_TXNOTFULL) != 0U);
}

/*
 *  受信した文字の取出し
 */
Inline char
imxrt600_usart_getchar(uintptr_t base)
{
	return((char) sil_rew_mem(IMXRT600_USART_FIFORD(base)));
}

/*
 *  送信する文字の書込み
 */
Inline void
imxrt600_usart_putchar(uintptr_t base, char c)
{
	sil_wrw_mem(IMXRT600_USART_FIFOWR(base), (uint32_t) c);
}

/*
 *  シリアルインタフェースドライバに提供する機能
 */

/*
 *  SIOドライバの初期化
 */
extern void		imxrt600_usart_initialize(void);

/*
 *  SIOドライバの終了処理
 */
extern void		imxrt600_usart_terminate(void);

/*
 *  SIOの割込みサービスルーチン
 */
extern void		imxrt600_usart_isr(ID siopid);

/*
 *  SIOポートのオープン
 */
extern SIOPCB	*imxrt600_usart_opn_por(ID siopid, EXINF exinf);

/*
 *  SIOポートのクローズ
 */
extern void		imxrt600_usart_cls_por(SIOPCB *siopcb);

/*
 *  SIOポートへの文字送信
 */
extern bool_t	imxrt600_usart_snd_chr(SIOPCB *siopcb, char c);

/*
 *  SIOポートからの文字受信
 */
extern int_t	imxrt600_usart_rcv_chr(SIOPCB *siopcb);

/*
 *  SIOポートからのコールバックの許可
 */
extern void		imxrt600_usart_ena_cbr(SIOPCB *siopcb, uint_t cbrtn);

/*
 *  SIOポートからのコールバックの禁止
 */
extern void		imxrt600_usart_dis_cbr(SIOPCB *siopcb, uint_t cbrtn);

/*
 *  SIOポートからの送信可能コールバック
 */
extern void		imxrt600_usart_irdy_snd(EXINF exinf);

/*
 *  SIOポートからの受信通知コールバック
 */
extern void		imxrt600_usart_irdy_rcv(EXINF exinf);

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_OMIT_TECS */
#endif /* TOPPERS_IMXRT600_USART_H */
