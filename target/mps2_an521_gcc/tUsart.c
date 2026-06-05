/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2006-2016,2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
 *  トウェアは無保証で提供される．
 *
 */

/*
 *  CMSDK APB UART 用 簡易 SIO ドライバ
 *
 *  ARM CMSDK の APB UART（QEMU の cmsdk-apb-uart）を駆動する．送受信は
 *  1 文字バッファで，送信完了割込み・受信割込みを SIO のコールバックに
 *  対応付ける．割込みは UART の combined 割込み（TX/RX いずれの要因でも
 *  発火）1 本で処理する．
 */

#include <sil.h>
#include "mps2_an521.h"
#include "tUsart_tecsgen.h"
#include "kernel/kernel_impl.h"

/*
 *  ボーレート
 */
#define USART_BAUDRATE  115200U

/*
 *  シリアル I/O ポートのオープン
 */
void
eSIOPort_open(CELLIDX idx)
{
    CELLCB    *p_cellcb = GET_CELLCB(idx);
    uint32_t  bauddiv;

    /*
     *  いったん停止し，割込みステータスをすべてクリアする．
     */
    sil_wrw_mem((void *) CMSDK_UART_CTRL(ATTR_base), 0U);
    sil_wrw_mem((void *) CMSDK_UART_INTSTATUS(ATTR_base),
                CMSDK_UART_INT_TX | CMSDK_UART_INT_RX
                | CMSDK_UART_INT_TXOVRRUN | CMSDK_UART_INT_RXOVRRUN);

    /*
     *  ボーレートの設定（BAUDDIV = pclk / baud，最小値は 16）
     */
    bauddiv = CPU_CLOCK_HZ / USART_BAUDRATE;
    if (bauddiv < 16U) {
        bauddiv = 16U;
    }
    sil_wrw_mem((void *) CMSDK_UART_BAUDDIV(ATTR_base), bauddiv);

    /*
     *  送受信を許可する（割込みはまだ許可しない）．
     */
    sil_wrw_mem((void *) CMSDK_UART_CTRL(ATTR_base),
                CMSDK_UART_CTRL_TXEN | CMSDK_UART_CTRL_RXEN);
}

/*
 *  シリアル I/O ポートのクローズ
 */
void
eSIOPort_close(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    sil_wrw_mem((void *) CMSDK_UART_CTRL(ATTR_base), 0U);
}

/*
 *  シリアル I/O ポートへの文字送信
 */
bool_t
eSIOPort_putChar(CELLIDX idx, char c)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    if ((sil_rew_mem((void *) CMSDK_UART_STATE(ATTR_base))
         & CMSDK_UART_STATE_TXFULL) == 0U) {
        sil_wrw_mem((void *) CMSDK_UART_DATA(ATTR_base), (uint32_t)(uint8_t) c);
        return true;
    }
    return false;
}

/*
 *  シリアル I/O ポートからの文字受信
 */
int_t
eSIOPort_getChar(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    if ((sil_rew_mem((void *) CMSDK_UART_STATE(ATTR_base))
         & CMSDK_UART_STATE_RXFULL) != 0U) {
        /* DATA の読出しにより RXFULL はクリアされる */
        return (int_t)(sil_rew_mem((void *) CMSDK_UART_DATA(ATTR_base)) & 0xFFU);
    }
    return -1;
}

/*
 *  シリアル I/O ポートからのコールバックの許可
 */
void
eSIOPort_enableCBR(CELLIDX idx, uint_t cbrtn)
{
    CELLCB    *p_cellcb = GET_CELLCB(idx);
    uint32_t  ctrl;

    ctrl = sil_rew_mem((void *) CMSDK_UART_CTRL(ATTR_base));
    switch (cbrtn) {
    case SIOSendReady:
        ctrl |= CMSDK_UART_CTRL_TXINTEN;
        break;
    case SIOReceiveReady:
        ctrl |= CMSDK_UART_CTRL_RXINTEN;
        break;
    default:
        break;
    }
    sil_wrw_mem((void *) CMSDK_UART_CTRL(ATTR_base), ctrl);
}

/*
 *  シリアル I/O ポートからのコールバックの禁止
 */
void
eSIOPort_disableCBR(CELLIDX idx, uint_t cbrtn)
{
    CELLCB    *p_cellcb = GET_CELLCB(idx);
    uint32_t  ctrl;

    ctrl = sil_rew_mem((void *) CMSDK_UART_CTRL(ATTR_base));
    switch (cbrtn) {
    case SIOSendReady:
        ctrl &= ~CMSDK_UART_CTRL_TXINTEN;
        break;
    case SIOReceiveReady:
        ctrl &= ~CMSDK_UART_CTRL_RXINTEN;
        break;
    default:
        break;
    }
    sil_wrw_mem((void *) CMSDK_UART_CTRL(ATTR_base), ctrl);
}

/*
 *  シリアル I/O ポートに対する割込み処理
 */
void
eiISR_main(CELLIDX idx)
{
    CELLCB    *p_cellcb = GET_CELLCB(idx);
    uint32_t  stat;

    stat = sil_rew_mem((void *) CMSDK_UART_INTSTATUS(ATTR_base));

    if ((stat & CMSDK_UART_INT_TX) != 0U) {
        /* 送信割込みをクリアし，送信可能を通知する */
        sil_wrw_mem((void *) CMSDK_UART_INTSTATUS(ATTR_base), CMSDK_UART_INT_TX);
        ciSIOCBR_readySend();
    }
    if ((stat & CMSDK_UART_INT_RX) != 0U) {
        /* 受信割込みをクリアし，受信を通知する */
        sil_wrw_mem((void *) CMSDK_UART_INTSTATUS(ATTR_base), CMSDK_UART_INT_RX);
        ciSIOCBR_readyReceive();
    }
}
