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
 *  タイマドライバ（ARM MPS2-AN521 用）
 *
 *  高分解能タイマ（HRT）を SysTick で実現する．SysTick は 24bit のダウン
 *  カウンタであり，プロセッサクロック（QEMU では 20MHz）で駆動する．次の
 *  タイムイベントまでの相対時間を SysTick のリロード値に設定してワンショッ
 *  ト的に割込みを発生させる（イベント駆動）．
 *
 *  HRT のカウント値（us，単調増加）はソフトウェアの積算器 hrt_base で保持
 *  する．SysTick のカウント値（20MHz）の差分を逐次 us に換算して積算する
 *  ことで，リロード時のカウンタ再設定によるカウント値の非単調化（QEMU では
 *  SYST_CVR 書込み直後に 0 が読めることに起因）を避ける．端数（< 1us）は
 *  hrt_subus に繰り越し，ドリフトを生じさせない．
 */

#ifndef TOPPERS_TARGET_TIMER_H
#define TOPPERS_TARGET_TIMER_H

#include "kernel/kernel_impl.h"
#include "mps2_an521.h"
#include "arm_m.h"

/*
 *  タイマ割込みハンドラ登録のための定数（SysTick 例外）
 */
#define INTNO_TIMER  IRQNO_SYSTICK          /* 割込み番号 */
#define INHNO_TIMER  IRQNO_SYSTICK          /* 割込みハンドラ番号 */
#define INTPRI_TIMER (TMAX_INTPRI - 1)      /* 割込み優先度 */
#define INTATR_TIMER TA_NULL                /* 割込み属性 */

/*
 *  プロセッサクロックの 1us あたりのカウント数（QEMU AN521 は 20MHz）
 */
#define HRT_CLOCKS_PER_US   (CPU_CLOCK_HZ / 1000000U)

/*
 *  SysTick のリロード値の最大（24bit）
 */
#define HRT_MAX_TICKS       0x00FFFFFFU

/*
 *  割込みタイミングに指定する最大値（us）
 *
 *  24bit / 20MHz ＝ 約 0.84 秒．これより先のイベントはカーネルが分割する．
 */
#define HRTCNT_BOUND        (HRT_MAX_TICKS / HRT_CLOCKS_PER_US)

#ifndef TOPPERS_MACRO_ONLY

/*
 *  高分解能タイマの操作
 *
 *  いずれもカーネルから CPU ロック状態（タイマ割込みがマスクされた状態）で
 *  呼び出されることを前提とする．
 *
 *  target_hrt_get_current / set_event / raise_event は，システムサービス
 *  （tSysLog 等）やカーネル本体が局所的に _kernel_ プレフィクスへリネーム
 *  して参照するため，インライン関数として提供する必要がある（実体シンボル
 *  を作るとリネーム不整合でリンクできない）．本体ロジックと積算器の状態は
 *  target_timer.c に置き，ここでは薄い転送関数とする．
 */
extern void    target_hrt_initialize(intptr_t exinf);
extern void    target_hrt_terminate(intptr_t exinf);
extern void    target_hrt_handler(void);

extern HRTCNT  hrt_get_current_body(void);
extern void    hrt_set_event_body(HRTCNT hrtcnt);
extern void    hrt_raise_event_body(void);

Inline HRTCNT
target_hrt_get_current(void)
{
    return hrt_get_current_body();
}

Inline void
target_hrt_set_event(HRTCNT hrtcnt)
{
    hrt_set_event_body(hrtcnt);
}

Inline void
target_hrt_raise_event(void)
{
    hrt_raise_event_body();
}

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_TARGET_TIMER_H */
