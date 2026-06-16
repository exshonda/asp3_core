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
 *		ARM CMSDK APB UART用 簡易SIOドライバ（非TECS版専用）
 *
 *  ARM CMSDK の APB UART（QEMU の cmsdk-apb-uart）を駆動する．送受信は
 *  1文字バッファで，送信完了割込み・受信割込みをSIOのコールバックに対
 *  応付ける．割込みはUARTのcombined割込み（TX/RXいずれの要因でも発火）
 *  1本で処理する．
 */

#include <sil.h>
#include "target_syssvc.h"
#include "cmsdk_uart.h"

/*
 *  SIOポート初期化ブロックの定義
 */
typedef struct sio_port_initialization_block {
	uintptr_t	base;			/* UARTレジスタのベースアドレス */
	uint32_t	bauddiv;		/* ボーレート分周比の設定値 */
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
	{ SIO_UART_BASE, SIO_UART_BAUDDIV }
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
cmsdk_uart_initialize(void)
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
cmsdk_uart_terminate(void)
{
	uint_t	i;

	/*
	 *  オープンされているSIOポートのクローズ
	 */
	for (i = 0; i < TNUM_SIOP; i++) {
		cmsdk_uart_cls_por(&(siopcb_table[i]));
	}
}

/*
 *  SIOポートのオープン
 */
SIOPCB *
cmsdk_uart_opn_por(ID siopid, EXINF exinf)
{
	SIOPCB		*p_siopcb;
	uintptr_t	base;
	uint32_t	bauddiv;

	p_siopcb = get_siopcb(siopid);

	if (!(p_siopcb->opened)) {
		/*
		 *  既にオープンしている場合は，二重にオープンしない．
		 */
		base = p_siopcb->siopinib->base;

		/*
		 *  いったん停止し，割込みステータスをすべてクリアする．
		 */
		sil_wrw_mem((void *) CMSDK_UART_CTRL(base), 0U);
		sil_wrw_mem((void *) CMSDK_UART_INTSTATUS(base),
					CMSDK_UART_INT_TX | CMSDK_UART_INT_RX
					| CMSDK_UART_INT_TXOVRRUN | CMSDK_UART_INT_RXOVRRUN);

		/*
		 *  ボーレートの設定（BAUDDIVの最小値は16）
		 */
		bauddiv = p_siopcb->siopinib->bauddiv;
		if (bauddiv < 16U) {
			bauddiv = 16U;
		}
		sil_wrw_mem((void *) CMSDK_UART_BAUDDIV(base), bauddiv);

		/*
		 *  送受信を許可する（割込みはまだ許可しない）．
		 */
		sil_wrw_mem((void *) CMSDK_UART_CTRL(base),
					CMSDK_UART_CTRL_TXEN | CMSDK_UART_CTRL_RXEN);

		p_siopcb->opened = true;
	}
	p_siopcb->exinf = exinf;
	return(p_siopcb);
}

/*
 *  SIOポートのクローズ
 */
void
cmsdk_uart_cls_por(SIOPCB *p_siopcb)
{
	if (p_siopcb->opened) {
		/*
		 *  UARTをディスエーブル
		 */
		sil_wrw_mem((void *) CMSDK_UART_CTRL(p_siopcb->siopinib->base), 0U);

		p_siopcb->opened = false;
	}
}

/*
 *  SIOポートへの文字送信
 */
bool_t
cmsdk_uart_snd_chr(SIOPCB *p_siopcb, char c)
{
	if (cmsdk_uart_putready(p_siopcb->siopinib->base)) {
		cmsdk_uart_putchar(p_siopcb->siopinib->base, c);
		return(true);
	}
	return(false);
}

/*
 *  SIOポートからの文字受信
 */
int_t
cmsdk_uart_rcv_chr(SIOPCB *p_siopcb)
{
	if (cmsdk_uart_getready(p_siopcb->siopinib->base)) {
		/* DATAの読出しによりRXFULLはクリアされる */
		return((int_t)(uint8_t) cmsdk_uart_getchar(p_siopcb->siopinib->base));
	}
	return(-1);
}

/*
 *  SIOポートからのコールバックの許可
 */
void
cmsdk_uart_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;
	uint32_t	ctrl;

	ctrl = sil_rew_mem((void *) CMSDK_UART_CTRL(base));
	switch (cbrtn) {
	case SIO_RDY_SND:
		ctrl |= CMSDK_UART_CTRL_TXINTEN;
		break;
	case SIO_RDY_RCV:
		ctrl |= CMSDK_UART_CTRL_RXINTEN;
		break;
	default:
		break;
	}
	sil_wrw_mem((void *) CMSDK_UART_CTRL(base), ctrl);
}

/*
 *  SIOポートからのコールバックの禁止
 */
void
cmsdk_uart_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;
	uint32_t	ctrl;

	ctrl = sil_rew_mem((void *) CMSDK_UART_CTRL(base));
	switch (cbrtn) {
	case SIO_RDY_SND:
		ctrl &= ~CMSDK_UART_CTRL_TXINTEN;
		break;
	case SIO_RDY_RCV:
		ctrl &= ~CMSDK_UART_CTRL_RXINTEN;
		break;
	default:
		break;
	}
	sil_wrw_mem((void *) CMSDK_UART_CTRL(base), ctrl);
}

/*
 *  SIOポートに対する割込み処理
 */
static void
cmsdk_uart_isr_siop(SIOPCB *p_siopcb)
{
	uintptr_t	base = p_siopcb->siopinib->base;
	uint32_t	stat;

	stat = sil_rew_mem((void *) CMSDK_UART_INTSTATUS(base));

	if ((stat & CMSDK_UART_INT_TX) != 0U) {
		/*
		 *  送信割込みをクリアし，送信可能を通知する．
		 */
		sil_wrw_mem((void *) CMSDK_UART_INTSTATUS(base), CMSDK_UART_INT_TX);
		cmsdk_uart_irdy_snd(p_siopcb->exinf);
	}
	if ((stat & CMSDK_UART_INT_RX) != 0U) {
		/*
		 *  受信割込みをクリアし，受信を通知する．
		 */
		sil_wrw_mem((void *) CMSDK_UART_INTSTATUS(base), CMSDK_UART_INT_RX);
		cmsdk_uart_irdy_rcv(p_siopcb->exinf);
	}
}

/*
 *  SIOの割込みサービスルーチン
 */
void
cmsdk_uart_isr(ID siopid)
{
	cmsdk_uart_isr_siop(get_siopcb(siopid));
}
