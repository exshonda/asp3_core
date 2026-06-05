/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2008-2011 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，以下の(1)～(4)の条件を満たす場合に限り，本ソフトウェ
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
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 *
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 */

/*
 *		STM32 USART用 簡易SIOドライバ（STM32MP2用）
 *
 *  STM32MP2のUSART（STM32H5等と同一のレジスタマップ）を対象とする．
 *  ボーレート等の通信パラメータはTF-A（FSBL）が115200 8N1・FIFO有効で
 *  初期化済みであることを前提とし，オープン時に再初期化は行わない
 *  （クロックツリーの取得手段が無いため．stm32usart.cと同じ方針）．
 */

#include <sil.h>
#include "stm32usart.h"
#include "tUsart_tecsgen.h"

/*
 *  プリミティブな送信／受信関数
 */

/*
 *  受信バッファに文字があるか？
 */
Inline bool_t
usart_getready(CELLCB *p_cellcb)
{
	return((sil_rew_mem(USART_ISR(ATTR_baseAddress)) & USART_ISR_RXNE) != 0U);
}

/*
 *  送信バッファに空きがあるか？
 */
Inline bool_t
usart_putready(CELLCB *p_cellcb)
{
	return((sil_rew_mem(USART_ISR(ATTR_baseAddress)) & USART_ISR_TXE) != 0U);
}

/*
 *  受信した文字の取出し
 */
Inline char
usart_getchar(CELLCB *p_cellcb)
{
	return((char)(sil_rew_mem(USART_RDR(ATTR_baseAddress)) & 0xffU));
}

/*
 *  送信する文字の書込み
 */
Inline void
usart_putchar(CELLCB *p_cellcb, char c)
{
	sil_wrw_mem(USART_TDR(ATTR_baseAddress), (uint32_t)c);
}

/*
 *  シリアルI/Oポートのオープン
 *
 *  TF-Aが初期化済みであることを前提に，送受信の有効化の確認と
 *  エラーフラグのクリアのみを行う（再初期化はしない）．
 */
void
eSIOPort_open(CELLIDX idx)
{
	uint32_t	cr1;
	CELLCB		*p_cellcb = GET_CELLCB(idx);

	/*
	 *  USART・送信・受信が無効なら有効にする
	 */
	cr1 = sil_rew_mem(USART_CR1(ATTR_baseAddress));
	if ((cr1 & (USART_CR1_UE | USART_CR1_RE | USART_CR1_TE))
			!= (USART_CR1_UE | USART_CR1_RE | USART_CR1_TE)) {
		sil_wrw_mem(USART_CR1(ATTR_baseAddress),
					cr1 | USART_CR1_UE | USART_CR1_RE | USART_CR1_TE);
	}

	/*
	 *  オーバーランエラーをクリア
	 */
	sil_wrw_mem(USART_ICR(ATTR_baseAddress), USART_ICR_ORECF);
}

/*
 *  シリアルI/Oポートのクローズ
 *
 *  TF-Aによる初期化状態を維持するため，USART自体は無効化せず，
 *  割込みの発生のみを止める．
 */
void
eSIOPort_close(CELLIDX idx)
{
	uint32_t	cr1;
	CELLCB		*p_cellcb = GET_CELLCB(idx);

	cr1 = sil_rew_mem(USART_CR1(ATTR_baseAddress));
	sil_wrw_mem(USART_CR1(ATTR_baseAddress),
				cr1 & ~(USART_CR1_RXNEIE | USART_CR1_TXEIE));
}

/*
 *  シリアルI/Oポートへの文字送信
 */
bool_t
eSIOPort_putChar(CELLIDX idx, char c)
{
	CELLCB	*p_cellcb = GET_CELLCB(idx);

	if (usart_putready(p_cellcb)) {
		usart_putchar(p_cellcb, c);
		return(true);
	}
	return(false);
}

/*
 *  シリアルI/Oポートからの文字受信
 */
int_t
eSIOPort_getChar(CELLIDX idx)
{
	CELLCB	*p_cellcb = GET_CELLCB(idx);

	if (usart_getready(p_cellcb)) {
		return((int_t)(uint8_t) usart_getchar(p_cellcb));
	}
	return(-1);
}

/*
 *  シリアルI/Oポートからのコールバックの許可
 */
void
eSIOPort_enableCBR(CELLIDX idx, uint_t cbrtn)
{
	uint32_t	cr1;
	CELLCB		*p_cellcb = GET_CELLCB(idx);

	cr1 = sil_rew_mem(USART_CR1(ATTR_baseAddress));
	switch (cbrtn) {
	case SIOSendReady:
		sil_wrw_mem(USART_CR1(ATTR_baseAddress), cr1 | USART_CR1_TXEIE);
		break;
	case SIOReceiveReady:
		sil_wrw_mem(USART_CR1(ATTR_baseAddress), cr1 | USART_CR1_RXNEIE);
		break;
	}
}

/*
 *  シリアルI/Oポートからのコールバックの禁止
 */
void
eSIOPort_disableCBR(CELLIDX idx, uint_t cbrtn)
{
	uint32_t	cr1;
	CELLCB		*p_cellcb = GET_CELLCB(idx);

	cr1 = sil_rew_mem(USART_CR1(ATTR_baseAddress));
	switch (cbrtn) {
	case SIOSendReady:
		sil_wrw_mem(USART_CR1(ATTR_baseAddress), cr1 & ~USART_CR1_TXEIE);
		break;
	case SIOReceiveReady:
		sil_wrw_mem(USART_CR1(ATTR_baseAddress), cr1 & ~USART_CR1_RXNEIE);
		break;
	}
}

/*
 *  シリアルI/Oポートに対する割込み処理
 */
void
eiISR_main(CELLIDX idx)
{
	CELLCB	*p_cellcb = GET_CELLCB(idx);

	/*
	 *  オーバーランエラーはクリアして読み進める
	 */
	if ((sil_rew_mem(USART_ISR(ATTR_baseAddress)) & USART_ISR_ORE) != 0U) {
		sil_wrw_mem(USART_ICR(ATTR_baseAddress), USART_ICR_ORECF);
	}

	if (usart_getready(p_cellcb)) {
		/*
		 *  受信通知コールバックルーチンを呼び出す．
		 */
		ciSIOCBR_readyReceive();
	}
	if (usart_putready(p_cellcb)) {
		/*
		 *  送信可能コールバックルーチンを呼び出す．
		 */
		ciSIOCBR_readySend();
	}
}
