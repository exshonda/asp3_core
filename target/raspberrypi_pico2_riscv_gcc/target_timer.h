/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアをTOPPERSライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソ
 *  フトウェアは無保証で提供される．
 */

/*
 *  タイマドライバのターゲット依存部（RaspberryPi Pico2 RISC-V用）
 *
 *  ARM版（target/raspberrypi_pico2_gcc/target_timer.h）のTIMER0 ALARM0
 *  ロジックを踏襲し，割込みの強制（過去時刻設定時のペンディング）を
 *  NVIC（ISPR）からXh3irqの割込み強制ビット（meifa）に置き換えたもの．
 *  meifaの強制ビットはmeinextでの受付け時にハードウェアが自動クリア
 *  する（エッジペンディング相当）．
 */

#ifndef TOPPERS_TARGET_TIMER_H
#define TOPPERS_TARGET_TIMER_H

#include <sil.h>
#include "RP2350.h"

/*
 *  タイマ割込みハンドラ登録のための定数
 *  （ASP3の割込み番号INTNO = RP2350のIRQ番号 + 1）
 */
#define INTNO_TIMER  (RP2350_TIMER0_0_IRQn + 1)   /* 割込み番号 */
#define INHNO_TIMER  (RP2350_TIMER0_0_IRQn + 1)   /* 割込みハンドラ番号 */
#define INTPRI_TIMER (TMAX_INTPRI - 1)            /* 割込み優先度 */
#define INTATR_TIMER TA_NULL                      /* 割込み属性 */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  高分解能タイマの起動処理
 */
extern void target_hrt_initialize(intptr_t exinf);

/*
 *  高分解能タイマの停止処理
 */
extern void target_hrt_terminate(intptr_t exinf);

/*
 *  高分解能タイマの現在のカウント値の読出し
 */
Inline HRTCNT target_hrt_get_current(void)
{
    return sil_rew_mem(RP2350_TIMER0_TIMERAWL);
}

/*
 *  タイマ割込みの強制（Xh3irqのmeifa．xh3irq_kernel_impl.hと同じ
 *  窓方式CSRアクセス．本ヘッダはアプリケーションからもインクルード
 *  されるため，カーネル内部ヘッダに依存しないよう直接記述する）
 */
Inline void target_timer_force_int(void)
{
    ulong_t tmp;
    uint32_t val = ((1U << ((INTNO_TIMER - 1) % 16)) << 16)
                        | ((uint32_t)(INTNO_TIMER - 1) / 16U);
    Asm("csrrs %0, 0xbe2, %1" : "=r"(tmp) : "r"(val));   /* meifa */
}

/*
 *  高分解能タイマへの割込みタイミングの設定
 *
 *  高分解能タイマを，hrtcntで指定した値カウントアップしたら割込みを発
 *  生させるように設定する．
 */
Inline void target_hrt_set_event(HRTCNT hrtcnt)
{
    const uint32_t current = target_hrt_get_current();
    sil_wrw_mem(RP2350_TIMER0_ALARM0, current + hrtcnt);
    if (target_hrt_get_current() - current >= hrtcnt) {
        sil_wrw_mem(RP2350_TIMER0_ARMED, RP2350_TIMER0_INT_ALARM_0);
        target_timer_force_int();
    }
}

/*
 *  高分解能タイマ割込みの要求
 */
Inline void target_hrt_raise_event(void)
{
    target_timer_force_int();
}

/*
 *  割込みタイミングに指定する最大値
 */
#define HRTCNT_BOUND 4000000002U

/*
 *  高分解能タイマ割込みハンドラ
 */
extern void target_hrt_handler(void);

#endif /* TOPPERS_MACRO_ONLY */

#endif /* TOPPERS_TARGET_TIMER_H */
