/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2015 by Ushio Laboratory
 *              Graduate School of Engineering Science, Osaka Univ., JAPAN
 *  Copyright (C) 2015,2016 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
 *  トウェアは無保証で提供される．
 *
 */

/*
 *  シリアルインタフェースドライバのターゲット依存部
 */

/*
 *  SIO ポートのオープン
 */
Inline void
eSIOPort_open(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    /*
     *  デバイス依存のオープン処理
     */
    cSIOPort_open();

    /*
     *  SIO の割込みマスクを解除する．
     */
    cInterruptRequest_enable();
}

/*
 *  SIO ポートのクローズ
 */
Inline void
eSIOPort_close(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    /*
     *  デバイス依存のクローズ処理
     */
    cSIOPort_close();

    /*
     *  SIO の割込みをマスクする．
     */
    cInterruptRequest_disable();
}

/*
 *  SIO ポートへの文字送信
 */
Inline bool_t
eSIOPort_putChar(CELLIDX idx, char c)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    return cSIOPort_putChar(c);
}

/*
 *  SIO ポートからの文字受信
 */
Inline int_t
eSIOPort_getChar(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    return cSIOPort_getChar();
}

/*
 *  SIO ポートからのコールバックの許可
 */
Inline void
eSIOPort_enableCBR(CELLIDX idx, uint_t cbrtn)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    cSIOPort_enableCBR(cbrtn);
}

/*
 *  SIO ポートからのコールバックの禁止
 */
Inline void
eSIOPort_disableCBR(CELLIDX idx, uint_t cbrtn)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    cSIOPort_disableCBR(cbrtn);
}

/*
 *  SIO ポートからの送信可能コールバック
 */
Inline void
eiSIOCBR_readySend(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    if (is_ciSIOCBR_joined()) {
        ciSIOCBR_readySend();
    }
}

/*
 *  SIO ポートからの受信通知コールバック
 */
Inline void
eiSIOCBR_readyReceive(CELLIDX idx)
{
    CELLCB *p_cellcb = GET_CELLCB(idx);

    if (is_ciSIOCBR_joined()) {
        ciSIOCBR_readyReceive();
    }
}
