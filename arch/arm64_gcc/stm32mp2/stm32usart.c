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
 *		STM32 USART用 簡易SIOドライバ（非TECS版専用）
 *
 *  STM32MP2のUSART（STM32H5等と同一のレジスタマップ）を対象とする．
 *  ボーレート等の通信パラメータはTF-A（FSBL）が115200 8N1・FIFO有効で
 *  初期化済みであることを前提とし，オープン時に再初期化は行わない
 *  （クロックツリーの取得手段が無いため．tUsart.cと同じ方針）．
 */

#include <sil.h>
#include "target_syssvc.h"
#include "stm32usart.h"

/*
 *  SIOポート初期化ブロックの定義
 */
typedef struct sio_port_initialization_block {
	uintptr_t	base;			/* USARTレジスタのベースアドレス */
} SIOPINIB;

/*
 *  SIOポート管理ブロックの定義
 */
struct sio_port_control_block {
	const SIOPINIB *siopinib;	/* SIOポート初期化ブロック */
	EXINF		exinf;			/* 拡張情報 */
	bool_t		opened;			/* オープン済み */
};

/*
 *  SIOポート初期化ブロック
 */
const SIOPINIB siopinib_table[TNUM_SIOP] = {
	{ SIO_UART_BASE }
};

/*
 *  SIOポート管理ブロックのエリア
 */
SIOPCB	siopcb_table[TNUM_SIOP];

/*
 *  SIOポートIDから管理ブロックを取り出すためのマクロ
 */
#define INDEX_SIOP(siopid)	((uint_t)((siopid) - 1))
#define get_siopcb(siopid)	(&(siopcb_table[INDEX_SIOP(siopid)]))

/*
 *  SIOドライバの初期化
 */
void
stm32usart_initialize(void)
{
	SIOPCB	*p_siopcb;
	uint_t	i;

	/*
	 *  SIOポート管理ブロックの初期化
	 */
	for (p_siopcb = siopcb_table, i = 0; i < TNUM_SIOP; p_siopcb++, i++) {
		p_siopcb->siopinib = &(siopinib_table[i]);
		p_siopcb->opened = false;
	}
}

/*
 *  SIOドライバの終了処理
 */
void
stm32usart_terminate(void)
{
	uint_t	i;

	/*
	 *  オープンされているSIOポートのクローズ
	 */
	for (i = 0; i < TNUM_SIOP; i++) {
		stm32usart_cls_por(&(siopcb_table[i]));
	}
}

/*
 *  SIOポートのオープン
 *
 *  TF-Aが初期化済みであることを前提に，送受信の有効化の確認と
 *  エラーフラグのクリアのみを行う（再初期化はしない）．
 */
SIOPCB *
stm32usart_opn_por(ID siopid, EXINF exinf)
{
	SIOPCB		*p_siopcb;
	uintptr_t	base;
	uint32_t	cr1;

	p_siopcb = get_siopcb(siopid);

	if (!(p_siopcb->opened)) {
		/*
		 *  既にオープンしている場合は，二重にオープンしない．
		 */
		base = p_siopcb->siopinib->base;

		/*
		 *  USART・送信・受信が無効なら有効にする
		 */
		cr1 = sil_rew_mem(USART_CR1(base));
		if ((cr1 & (USART_CR1_UE | USART_CR1_RE | USART_CR1_TE))
				!= (USART_CR1_UE | USART_CR1_RE | USART_CR1_TE)) {
			sil_wrw_mem(USART_CR1(base),
						cr1 | USART_CR1_UE | USART_CR1_RE | USART_CR1_TE);
		}

		/*
		 *  オーバーランエラーをクリア
		 */
		sil_wrw_mem(USART_ICR(base), USART_ICR_ORECF);

		p_siopcb->opened = true;
	}
	p_siopcb->exinf = exinf;
	return(p_siopcb);
}

/*
 *  SIOポートのクローズ
 *
 *  TF-Aによる初期化状態を維持するため，USART自体は無効化せず，
 *  割込みの発生のみを止める．
 */
void
stm32usart_cls_por(SIOPCB *p_siopcb)
{
	uintptr_t	base;
	uint32_t	cr1;

	if (p_siopcb->opened) {
		base = p_siopcb->siopinib->base;

		cr1 = sil_rew_mem(USART_CR1(base));
		sil_wrw_mem(USART_CR1(base),
					cr1 & ~(USART_CR1_RXNEIE | USART_CR1_TXEIE));

		p_siopcb->opened = false;
	}
}

/*
 *  SIOポートへの文字送信
 */
bool_t
stm32usart_snd_chr(SIOPCB *p_siopcb, char c)
{
	if (stm32usart_putready(p_siopcb->siopinib->base)) {
		stm32usart_putchar(p_siopcb->siopinib->base, c);
		return(true);
	}
	return(false);
}

/*
 *  SIOポートからの文字受信
 */
int_t
stm32usart_rcv_chr(SIOPCB *p_siopcb)
{
	if (stm32usart_getready(p_siopcb->siopinib->base)) {
		return((int_t)(uint8_t) stm32usart_getchar(p_siopcb->siopinib->base));
	}
	return(-1);
}

/*
 *  SIOポートからのコールバックの許可
 */
void
stm32usart_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;
	uint32_t	cr1;

	cr1 = sil_rew_mem(USART_CR1(base));
	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_wrw_mem(USART_CR1(base), cr1 | USART_CR1_TXEIE);
		break;
	case SIO_RDY_RCV:
		sil_wrw_mem(USART_CR1(base), cr1 | USART_CR1_RXNEIE);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートからのコールバックの禁止
 */
void
stm32usart_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;
	uint32_t	cr1;

	cr1 = sil_rew_mem(USART_CR1(base));
	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_wrw_mem(USART_CR1(base), cr1 & ~USART_CR1_TXEIE);
		break;
	case SIO_RDY_RCV:
		sil_wrw_mem(USART_CR1(base), cr1 & ~USART_CR1_RXNEIE);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートに対する割込み処理
 */
static void
stm32usart_isr_siop(SIOPCB *p_siopcb)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	/*
	 *  オーバーランエラーはクリアして読み進める
	 */
	if ((sil_rew_mem(USART_ISR(base)) & USART_ISR_ORE) != 0U) {
		sil_wrw_mem(USART_ICR(base), USART_ICR_ORECF);
	}

	if (stm32usart_getready(base)) {
		/*
		 *  受信通知コールバックルーチンを呼び出す．
		 */
		stm32usart_irdy_rcv(p_siopcb->exinf);
	}
	if (stm32usart_putready(base)) {
		/*
		 *  送信可能コールバックルーチンを呼び出す．
		 */
		stm32usart_irdy_snd(p_siopcb->exinf);
	}
}

/*
 *  SIOの割込みサービスルーチン
 */
void
stm32usart_isr(ID siopid)
{
	stm32usart_isr_siop(get_siopcb(siopid));
}
