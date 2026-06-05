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
 *		XUartPs用 簡易SIOドライバ（非TECS版専用）
 *
 *  TECS版（tXUartPs.c）のレジスタ操作ロジックを非TECS版の構造に移植
 *  したもの．
 */

#include <sil.h>
#include "target_syssvc.h"
#include "xuartps.h"

/*
 *  SIOポート初期化ブロックの定義
 */
typedef struct sio_port_initialization_block {
	uintptr_t	base;			/* UARTレジスタのベースアドレス */
	uint16_t	mode;			/* モードレジスタの設定値 */
	uint16_t	baudgen;		/* ボーレート生成レジスタの設定値 */
	uint8_t		bauddiv;		/* ボーレート分割レジスタの設定値 */
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
	{ SIO_UART_BASE, SIO_UART_MODE, SIO_UART_BAUDGEN, SIO_UART_BAUDDIV }
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
xuartps_initialize(void)
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
xuartps_terminate(void)
{
	uint_t	i;

	/*
	 *  オープンされているSIOポートのクローズ
	 */
	for (i = 0; i < TNUM_SIOP; i++) {
		if (siopcb_table[i].opened) {
			/*
			 *  送信FIFOが空になるまで待つ
			 */
			while ((sil_rew_mem(XUARTPS_SR(siopcb_table[i].siopinib->base))
											& XUARTPS_SR_TXEMPTY) == 0U) {
				sil_dly_nse(100);
			}
			xuartps_cls_por(&(siopcb_table[i]));
		}
	}
}

/*
 *  SIOポートのオープン
 */
SIOPCB *
xuartps_opn_por(ID siopid, EXINF exinf)
{
	SIOPCB		*p_siopcb;
	uintptr_t	base;

	p_siopcb = get_siopcb(siopid);

	if (!(p_siopcb->opened)) {
		/*
		 *  既にオープンしている場合は，二重にオープンしない．
		 */
		base = p_siopcb->siopinib->base;

		/*
		 *  全割込みをディスエーブル
		 */
		sil_wrw_mem(XUARTPS_IDR(base), XUARTPS_IXR_ALL);

		/*
		 *  ペンディングしている割込みをクリア
		 */
		sil_wrw_mem(XUARTPS_ISR(base), sil_rew_mem(XUARTPS_ISR(base)));

		/*
		 *  送受信のリセットとディスエーブル
		 */
		sil_wrw_mem(XUARTPS_CR(base),
							XUARTPS_CR_TXRST | XUARTPS_CR_RXRST
								| XUARTPS_CR_TX_DIS | XUARTPS_CR_RX_DIS);

		/*
		 *  ボーレートの設定
		 */
		sil_wrw_mem(XUARTPS_BAUDGEN(base), p_siopcb->siopinib->baudgen);
		sil_wrw_mem(XUARTPS_BAUDDIV(base), p_siopcb->siopinib->bauddiv);

		/*
		 *  データ長，ストップビット，パリティの設定
		 */
		sil_wrw_mem(XUARTPS_MR(base), p_siopcb->siopinib->mode);

		/*
		 *  受信トリガを1バイトに設定
		 */
		sil_wrw_mem(XUARTPS_RXWM(base), 1U);

		/*
		 *  タイムアウトを設定
		 */
		sil_wrw_mem(XUARTPS_RXTOUT(base), 10U);

		/*
		 *  送受信のイネーブル
		 */
		sil_wrw_mem(XUARTPS_CR(base),
					XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN | XUARTPS_CR_STOPBRK);

		p_siopcb->opened = true;
	}
	p_siopcb->exinf = exinf;
	return(p_siopcb);
}

/*
 *  SIOポートのクローズ
 */
void
xuartps_cls_por(SIOPCB *p_siopcb)
{
	uintptr_t	base;

	if (p_siopcb->opened) {
		base = p_siopcb->siopinib->base;

		/*
		 *  送受信のディスエーブル
		 */
		sil_wrw_mem(XUARTPS_CR(base),
				XUARTPS_CR_TX_DIS | XUARTPS_CR_RX_DIS | XUARTPS_CR_STOPBRK);

		/*
		 *  全割込みをディスエーブル
		 */
		sil_wrw_mem(XUARTPS_IDR(base), XUARTPS_IXR_ALL);

		p_siopcb->opened = false;
	}
}

/*
 *  SIOポートへの文字送信
 */
bool_t
xuartps_snd_chr(SIOPCB *p_siopcb, char c)
{
	if (xuartps_putready(p_siopcb->siopinib->base)) {
		xuartps_putchar(p_siopcb->siopinib->base, c);
		return(true);
	}
	return(false);
}

/*
 *  SIOポートからの文字受信
 */
int_t
xuartps_rcv_chr(SIOPCB *p_siopcb)
{
	if (xuartps_getready(p_siopcb->siopinib->base)) {
		return((int_t)(uint8_t) xuartps_getchar(p_siopcb->siopinib->base));
	}
	return(-1);
}

/*
 *  SIOポートからのコールバックの許可
 */
void
xuartps_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_wrw_mem(XUARTPS_IER(base), XUARTPS_IXR_TXEMPTY);
		break;
	case SIO_RDY_RCV:
		sil_wrw_mem(XUARTPS_IER(base), XUARTPS_IXR_RXTRIG);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートからのコールバックの禁止
 */
void
xuartps_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_wrw_mem(XUARTPS_IDR(base), XUARTPS_IXR_TXEMPTY);
		break;
	case SIO_RDY_RCV:
		sil_wrw_mem(XUARTPS_IDR(base), XUARTPS_IXR_RXTRIG);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートに対する割込み処理
 */
static void
xuartps_isr_siop(SIOPCB *p_siopcb)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	if (xuartps_getready(base)) {
		/*
		 *  受信通知コールバックルーチンを呼び出す．
		 */
		xuartps_irdy_rcv(p_siopcb->exinf);
	}
	if (xuartps_putready(base)) {
		/*
		 *  送信可能コールバックルーチンを呼び出す．
		 */
		xuartps_irdy_snd(p_siopcb->exinf);
	}

	/*
	 *  ペンディングしている割込みをクリア
	 */
	sil_wrw_mem(XUARTPS_ISR(base), sil_rew_mem(XUARTPS_ISR(base)));
}

/*
 *  SIOの割込みサービスルーチン
 */
void
xuartps_isr(ID siopid)
{
	xuartps_isr_siop(get_siopcb(siopid));
}
