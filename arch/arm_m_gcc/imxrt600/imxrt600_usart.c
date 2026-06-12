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
 *  TECS版（tUsart.c）のレジスタ操作ロジックを非TECS版の構造に移植した
 *  もの．オープン時にFRG（Fractional Rate Generator）の設定・Flexcomm
 *  クロックの供給・USART機能の選択・ボーレート設定を行う．
 */

#include <sil.h>
#include "target_syssvc.h"
#include "imxrt600_usart.h"

/*
 *  ボーレート
 */
#define IMXRT600_USART_BAUDRATE		115200U

/*
 *  SIOポート初期化ブロックの定義
 */
typedef struct sio_port_initialization_block {
	uintptr_t	base;			/* USARTレジスタのベースアドレス */
	uintptr_t	flexcomm_base;	/* Flexcommレジスタのベースアドレス */
	uint32_t	index;			/* Flexcomm（FRG）のインデックス */
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
	{ SIO_USART_BASE, SIO_FLEXCOMM_BASE, SIO_USART_INDEX }
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
imxrt600_usart_initialize(void)
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
imxrt600_usart_terminate(void)
{
	uint_t	i;

	/*
	 *  オープンされているSIOポートのクローズ
	 */
	for (i = 0; i < TNUM_SIOP; i++) {
		imxrt600_usart_cls_por(&(siopcb_table[i]));
	}
}

/*
 *  SIOポートのオープン
 */
SIOPCB *
imxrt600_usart_opn_por(ID siopid, EXINF exinf)
{
	SIOPCB		*p_siopcb;
	uintptr_t	base;
	uint32_t	index;
	const uint32_t BRG = FRG_PLL_HZ / 2 / 16 / IMXRT600_USART_BAUDRATE + 1;
	const uint32_t MULT = ((((16ULL << 1)
				* (FRG_PLL_HZ - 16 * IMXRT600_USART_BAUDRATE * BRG))
				/ (IMXRT600_USART_BAUDRATE * BRG)) + 1) >> 1;

	p_siopcb = get_siopcb(siopid);

	if (!(p_siopcb->opened)) {
		/*
		 *  既にオープンしている場合は，二重にオープンしない．
		 */
		base = p_siopcb->siopinib->base;
		index = p_siopcb->siopinib->index;

		/*
		 *  FRGの分周比の設定とFRGクロック（frg_pll）の選択
		 */
		sil_wrw_mem(IMXRT600_SYSCON_CLKCTL1_FRGnCLKCTL(index),
					IMXRT600_SYSCON_CLKCTL1_FRGnCLKCTL_DIV(0xFF)
						| IMXRT600_SYSCON_CLKCTL1_FRGnCLKCTL_MULT(MULT));
		sil_wrw_mem(IMXRT600_SYSCON_CLKCTL1_FRGnCLKSEL(index),
					IMXRT600_SYSCON_CLKCTL1_FRGnCLKSEL_SEL_FRG_PLL_CLOCK);

		/*
		 *  FlexcommクロックにFRGクロックを選択し，クロックを供給して
		 *  リセットを解除する
		 */
		sil_wrw_mem(IMXRT600_SYSCON_CLKCTL1_FCnFCLKSEL(index),
					IMXRT600_SYSCON_CLKCTL1_FCnFCLKSEL_SEL_FCn_FRG_CLOCK);
		sil_wrw_mem(IMXRT600_SYSCON_CLKCTL1_PSCCTL0_SET,
					IMXRT600_SYSCON_CLKCTL1_PSCCTL0_FCn_CLK(index));
		sil_wrw_mem(IMXRT600_SYSCON_RSTCTL1_PRSTCTL0_CLR,
					IMXRT600_SYSCON_RSTCTL1_PRSTCTL0_FLEXCOMMn_RST(index));

		/*
		 *  FlexcommのUSART機能を選択
		 */
		sil_wrw_mem(IMXRT600_FLEXCOMM_PSELID(p_siopcb->siopinib->flexcomm_base),
					IMXRT600_FLEXCOMM_PSELID_PERSEL_USART);

		/*
		 *  ボーレートの設定
		 */
		sil_wrw_mem(IMXRT600_USART_BRG(base), BRG - 1);

		/*
		 *  FIFOの有効化と割込みトリガレベルの設定
		 *  （RXはFIFO非空で割込み，TXはFIFOに空きがあれば割込み）
		 */
		sil_wrw_mem(IMXRT600_USART_FIFOCFG(base),
					IMXRT600_USART_FIFOCFG_ENABLETX
						| IMXRT600_USART_FIFOCFG_ENABLERX);
		sil_wrw_mem(IMXRT600_USART_FIFOTRIG(base),
					IMXRT600_USART_FIFOTRIG_RXLVLENA
						| IMXRT600_USART_FIFOTRIG_TXLVLENA
						| IMXRT600_USART_FIFOTRIG_TXLVL(15));

		/*
		 *  データ長8ビットでUSARTをイネーブル
		 */
		sil_wrw_mem(IMXRT600_USART_CFG(base),
					IMXRT600_USART_CFG_DATALEN_8BIT
						| IMXRT600_USART_CFG_ENABLE);

		p_siopcb->opened = true;
	}
	p_siopcb->exinf = exinf;
	return(p_siopcb);
}

/*
 *  SIOポートのクローズ
 */
void
imxrt600_usart_cls_por(SIOPCB *p_siopcb)
{
	if (p_siopcb->opened) {
		/*
		 *  送信FIFOが掃けるのを待つ（待たずにディスエーブルすると
		 *  カーネル終了時の最後の出力（TAPサマリ行等）が失われる）
		 */
		while ((sil_rew_mem(IMXRT600_USART_FIFOSTAT(p_siopcb->siopinib->base))
				& IMXRT600_USART_FIFOSTAT_TXEMPTY) == 0U) ;

		/*
		 *  USARTをディスエーブル
		 */
		sil_wrw_mem(IMXRT600_USART_CFG(p_siopcb->siopinib->base), 0U);

		p_siopcb->opened = false;
	}
}

/*
 *  SIOポートへの文字送信
 */
bool_t
imxrt600_usart_snd_chr(SIOPCB *p_siopcb, char c)
{
	if (imxrt600_usart_putready(p_siopcb->siopinib->base)) {
		imxrt600_usart_putchar(p_siopcb->siopinib->base, c);
		return(true);
	}
	return(false);
}

/*
 *  SIOポートからの文字受信
 */
int_t
imxrt600_usart_rcv_chr(SIOPCB *p_siopcb)
{
	if (imxrt600_usart_getready(p_siopcb->siopinib->base)) {
		return((int_t)(uint8_t)
					imxrt600_usart_getchar(p_siopcb->siopinib->base));
	}
	return(-1);
}

/*
 *  SIOポートからのコールバックの許可
 */
void
imxrt600_usart_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_wrw_mem(IMXRT600_USART_FIFOINTENSET(base),
					IMXRT600_USART_FIFOINTEN_TXLVL);
		break;
	case SIO_RDY_RCV:
		sil_wrw_mem(IMXRT600_USART_FIFOINTENSET(base),
					IMXRT600_USART_FIFOINTEN_RXLVL);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートからのコールバックの禁止
 */
void
imxrt600_usart_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_wrw_mem(IMXRT600_USART_FIFOINTENCLR(base),
					IMXRT600_USART_FIFOINTEN_TXLVL);
		break;
	case SIO_RDY_RCV:
		sil_wrw_mem(IMXRT600_USART_FIFOINTENCLR(base),
					IMXRT600_USART_FIFOINTEN_RXLVL);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートに対する割込み処理
 */
static void
imxrt600_usart_isr_siop(SIOPCB *p_siopcb)
{
	uint32_t	stat;

	stat = sil_rew_mem(IMXRT600_USART_FIFOINTSTAT(p_siopcb->siopinib->base));

	if ((stat & IMXRT600_USART_FIFOINTSTAT_TXLVL) != 0U) {
		/*
		 *  送信可能コールバックルーチンを呼び出す．
		 */
		imxrt600_usart_irdy_snd(p_siopcb->exinf);
	}
	if ((stat & IMXRT600_USART_FIFOINTSTAT_RXLVL) != 0U) {
		/*
		 *  受信通知コールバックルーチンを呼び出す．
		 */
		imxrt600_usart_irdy_rcv(p_siopcb->exinf);
	}
}

/*
 *  SIOの割込みサービスルーチン
 */
void
imxrt600_usart_isr(ID siopid)
{
	imxrt600_usart_isr_siop(get_siopcb(siopid));
}
