/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2006-2016 by Embedded and Real-Time Systems Laboratory
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
 * 
 *  $Id: tUsart.c 1799 2023-04-01 00:50:30Z ertl-komori $
 */

/*
 * RP2350用 簡易SIOドライバ
 */

#include <sil.h>
#include "RP2350.h"
#include "tUsart_tecsgen.h"
#include "kernel/kernel_impl.h"

/*
 * シリアルI/Oポートのオープン
 */
void eSIOPort_open(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    /* Select clk_sys */
    sil_wrw_mem(RP2350_CLOCKS_CLK_PERI_CTRL, RP2350_CLOCKS_CLK_PERI_CTRL_ENABLE
                                           | RP2350_CLOCKS_CLK_PERI_CTRL_SRC_CLK_SYS);

    /* Reset UART */
    switch (ATTR_index) {
        case 0:
            sil_orw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_UART0);
            sil_clrw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_UART0);
            while ((sil_rew_mem(RP2350_RESETS_RESET_DONE) & RP2350_RESETS_RESET_UART0) == 0) ;
            break;

        case 1:
            sil_orw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_UART1);
            sil_clrw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_UART1);
            while ((sil_rew_mem(RP2350_RESETS_RESET_DONE) & RP2350_RESETS_RESET_UART1) == 0) ;
            break;

        default:
            break;
    }

    sil_wrw_mem(RP2350_UART_IMSC(ATTR_base), 0); /* Disable all interrupts */
    sil_wrw_mem(RP2350_UART_ICR(ATTR_base), 0x7FF); /* Clear all interrupts */
    const uint32_t baud = 115200;
    const uint64_t divisor = ((((uint64_t)CPU_CLOCK_HZ) << 7) / (16 * baud) + 1) >> 1;
    sil_wrw_mem(RP2350_UART_IBRD(ATTR_base), divisor >> 6);
    sil_wrw_mem(RP2350_UART_FBRD(ATTR_base), divisor & 0x3F);
    sil_wrw_mem(RP2350_UART_LCR_H(ATTR_base), RP2350_UART_LCR_H_WLEN_8BITS /* 8bit word */
                                            | RP2350_UART_LCR_H_FEN); /* Enable FIFO */
    sil_wrw_mem(RP2350_UART_IFLS(ATTR_base), (0b000 << 3) | (0b100 << 0)); /* Set FIFO level */
    /* Enable UART */
    sil_wrw_mem(RP2350_UART_CR(ATTR_base), RP2350_UART_CR_RXE | RP2350_UART_CR_TXE | RP2350_UART_CR_UARTEN);
}

/*
 * シリアルI/Oポートのクローズ
 */
void eSIOPort_close(CELLIDX idx)
{
    /*
     *  UARTをディスエーブル
     */
    CELLCB *p_cellcb = GET_CELLCB(idx);
    sil_wrw_mem(RP2350_UART_CR(ATTR_base), 0);
}

/*
 * シリアルI/Oポートへの文字送信
 */
bool_t eSIOPort_putChar(CELLIDX idx, char c)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);
    if ((sil_rew_mem(RP2350_UART_FR(ATTR_base)) & RP2350_UART_FR_TXFF) == 0) {
        sil_wrw_mem(RP2350_UART_DR(ATTR_base), c);
        return true;
    }
    return false;
}

/*
 * シリアルI/Oポートからの文字受信
 */
int_t eSIOPort_getChar(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);
    if ((sil_rew_mem(RP2350_UART_FR(ATTR_base)) & RP2350_UART_FR_RXFE) == 0) {
        return sil_rew_mem(RP2350_UART_DR(ATTR_base));
    }
    return -1;
}

/*
 * シリアルI/Oポートからのコールバックの許可
 */
void eSIOPort_enableCBR(CELLIDX idx, uint_t cbrtn)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);
    switch (cbrtn) {
        case SIOSendReady:
            sil_orw(RP2350_UART_IMSC(ATTR_base), RP2350_UART_IMSC_TXIM);
            break;
        case SIOReceiveReady:
            sil_orw(RP2350_UART_IMSC(ATTR_base), RP2350_UART_IMSC_RTIM | RP2350_UART_IMSC_RXIM);
            break;
    }
}

/*
 * シリアルI/Oポートからのコールバックの禁止
 */
void eSIOPort_disableCBR(CELLIDX idx, uint_t cbrtn)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);
    switch (cbrtn) {
        case SIOSendReady:
            sil_clrw(RP2350_UART_IMSC(ATTR_base), RP2350_UART_IMSC_TXIM);
            break;
        case SIOReceiveReady:
            sil_clrw(RP2350_UART_IMSC(ATTR_base), RP2350_UART_IMSC_RTIM | RP2350_UART_IMSC_RXIM);
            break;
    }
}

/*
 * シリアルI/Oポートに対する割込み処理
 */
void eiISR_main(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    const uint32_t stat = sil_rew_mem(RP2350_UART_MIS(ATTR_base));
    if (stat & RP2350_UART_MIS_TXMIS) {
        ciSIOCBR_readySend();
    }
    if (stat & (RP2350_UART_MIS_RTIM | RP2350_UART_MIS_RXMIS)) {
        ciSIOCBR_readyReceive();
    }
}
