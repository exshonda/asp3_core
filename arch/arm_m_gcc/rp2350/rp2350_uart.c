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
 *  TECS版（tUsart.c）のレジスタ操作ロジックを非TECS版の構造に移植した
 *  もの．オープン時にclk_periの選択・UARTのリセット・ボーレート設定を
 *  行う．
 */

#include <sil.h>
#include "target_syssvc.h"
#include "rp2350_uart.h"

/*
 *  ボーレート
 */
#define RP2350_UART_BAUDRATE	115200U

/*
 *  SIOポート初期化ブロックの定義
 */
typedef struct sio_port_initialization_block {
	uintptr_t	base;			/* UARTレジスタのベースアドレス */
	uint32_t	reset_bit;		/* RESETSレジスタのリセットビット */
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
	{ SIO_UART_BASE, SIO_UART_RESET_BIT }
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
rp2350_uart_initialize(void)
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
rp2350_uart_terminate(void)
{
	uint_t	i;

	/*
	 *  オープンされているSIOポートのクローズ
	 */
	for (i = 0; i < TNUM_SIOP; i++) {
		rp2350_uart_cls_por(&(siopcb_table[i]));
	}
}

/*
 *  SIOポートのオープン
 */
SIOPCB *
rp2350_uart_opn_por(ID siopid, EXINF exinf)
{
	SIOPCB		*p_siopcb;
	uintptr_t	base;
	uint64_t	divisor;

	p_siopcb = get_siopcb(siopid);

	if (!(p_siopcb->opened)) {
		/*
		 *  既にオープンしている場合は，二重にオープンしない．
		 */
		base = p_siopcb->siopinib->base;

		/*
		 *  clk_periの選択（clk_sys）
		 */
		sil_wrw_mem(RP2350_CLOCKS_CLK_PERI_CTRL,
					RP2350_CLOCKS_CLK_PERI_CTRL_ENABLE
						| RP2350_CLOCKS_CLK_PERI_CTRL_SRC_CLK_SYS);

		/*
		 *  UARTのリセット
		 */
		sil_orw(RP2350_RESETS_RESET, p_siopcb->siopinib->reset_bit);
		sil_clrw(RP2350_RESETS_RESET, p_siopcb->siopinib->reset_bit);
		while ((sil_rew_mem(RP2350_RESETS_RESET_DONE)
								& p_siopcb->siopinib->reset_bit) == 0U) ;

		/*
		 *  全割込みの禁止とクリア
		 */
		sil_wrw_mem(RP2350_UART_IMSC(base), 0U);
		sil_wrw_mem(RP2350_UART_ICR(base), 0x7ffU);

		/*
		 *  ボーレートの設定
		 */
		divisor = ((((uint64_t) CPU_CLOCK_HZ) << 7)
								/ (16U * RP2350_UART_BAUDRATE) + 1U) >> 1;
		sil_wrw_mem(RP2350_UART_IBRD(base), (uint32_t)(divisor >> 6));
		sil_wrw_mem(RP2350_UART_FBRD(base), (uint32_t)(divisor & 0x3fU));

		/*
		 *  データ長8ビット・FIFO有効・FIFOレベルの設定
		 */
		sil_wrw_mem(RP2350_UART_LCR_H(base),
					RP2350_UART_LCR_H_WLEN_8BITS | RP2350_UART_LCR_H_FEN);
		sil_wrw_mem(RP2350_UART_IFLS(base), (0x0U << 3) | (0x4U << 0));

		/*
		 *  UARTをイネーブル
		 */
		sil_wrw_mem(RP2350_UART_CR(base),
					RP2350_UART_CR_RXE | RP2350_UART_CR_TXE
						| RP2350_UART_CR_UARTEN);

		p_siopcb->opened = true;
	}
	p_siopcb->exinf = exinf;
	return(p_siopcb);
}

/*
 *  SIOポートのクローズ
 */
void
rp2350_uart_cls_por(SIOPCB *p_siopcb)
{
	if (p_siopcb->opened) {
		/*
		 *  送信FIFOが掃けるのを待つ（待たずにディスエーブルすると
		 *  カーネル終了時の最後の出力（TAPサマリ行等）が失われる）
		 */
		while ((sil_rew_mem(RP2350_UART_FR(p_siopcb->siopinib->base))
				& RP2350_UART_FR_BUSY) != 0U) ;

		/*
		 *  UARTをディスエーブル
		 */
		sil_wrw_mem(RP2350_UART_CR(p_siopcb->siopinib->base), 0U);

		p_siopcb->opened = false;
	}
}

/*
 *  SIOポートへの文字送信
 */
bool_t
rp2350_uart_snd_chr(SIOPCB *p_siopcb, char c)
{
	if (rp2350_uart_putready(p_siopcb->siopinib->base)) {
		rp2350_uart_putchar(p_siopcb->siopinib->base, c);
		return(true);
	}
	return(false);
}

/*
 *  SIOポートからの文字受信
 */
int_t
rp2350_uart_rcv_chr(SIOPCB *p_siopcb)
{
	if (rp2350_uart_getready(p_siopcb->siopinib->base)) {
		return((int_t)(uint8_t) rp2350_uart_getchar(p_siopcb->siopinib->base));
	}
	return(-1);
}

/*
 *  SIOポートからのコールバックの許可
 */
void
rp2350_uart_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_orw(RP2350_UART_IMSC(base), RP2350_UART_IMSC_TXIM);
		break;
	case SIO_RDY_RCV:
		sil_orw(RP2350_UART_IMSC(base),
					RP2350_UART_IMSC_RTIM | RP2350_UART_IMSC_RXIM);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートからのコールバックの禁止
 */
void
rp2350_uart_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	uintptr_t	base = p_siopcb->siopinib->base;

	switch (cbrtn) {
	case SIO_RDY_SND:
		sil_clrw(RP2350_UART_IMSC(base), RP2350_UART_IMSC_TXIM);
		break;
	case SIO_RDY_RCV:
		sil_clrw(RP2350_UART_IMSC(base),
					RP2350_UART_IMSC_RTIM | RP2350_UART_IMSC_RXIM);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートに対する割込み処理
 */
static void
rp2350_uart_isr_siop(SIOPCB *p_siopcb)
{
	uintptr_t	base = p_siopcb->siopinib->base;
	uint32_t	stat;

	stat = sil_rew_mem(RP2350_UART_MIS(base));

	if ((stat & RP2350_UART_MIS_TXMIS) != 0U) {
		/*
		 *  送信可能コールバックルーチンを呼び出す．
		 */
		rp2350_uart_irdy_snd(p_siopcb->exinf);
	}
	if ((stat & (RP2350_UART_MIS_RTIM | RP2350_UART_MIS_RXMIS)) != 0U) {
		/*
		 *  受信通知コールバックルーチンを呼び出す．
		 */
		rp2350_uart_irdy_rcv(p_siopcb->exinf);
	}
}

/*
 *  SIOの割込みサービスルーチン
 */
void
rp2350_uart_isr(ID siopid)
{
	rp2350_uart_isr_siop(get_siopcb(siopid));
}
