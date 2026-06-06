/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2024 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: mpcore_timer.h 376 2023-09-02 04:34:49Z ertl-honda $
 */

/*
 *     タイマドライバ（Machine Timer用）
 *
 *  TOPPERS/FMP3のRISC-V依存部をASP3（シングルプロセッサ）用に変換した
 *  もの．mtimecmpはブートハートのもの（MTIMER_MTIMECMP_SELF，チップ依
 *  存部で定義）に固定．
 */

#ifndef TOPPERS_MTIMER_H
#define TOPPERS_MTIMER_H

#include "kernel/kernel_impl.h"
#include <sil.h>

/*
 *    Machine Timer を用いて高分解能タイマを実現
 */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  Machine Timer の現在のカウント値（64bit）の読出し
 */
Inline uint64_t
mtimer_get_count(void)
{
#if __riscv_xlen == 64
    uint64_t count;
    count = sil_redw_mem(MTIMER_MTIME);
    return(count);
#elif __riscv_xlen == 32
    uint32_t count_l, count_u, prev_count_u;
    count_u = sil_rew_mem((void *)MTIMER_MTIME_U);
    do {
        prev_count_u = count_u;
        count_l = sil_rew_mem((void *)MTIMER_MTIME_L);
        count_u = sil_rew_mem((void *)MTIMER_MTIME_U);
    } while (count_u != prev_count_u);
    return((((uint64_t) count_u) << 32) | ((uint64_t) count_l));
#endif /* __riscv_xlen == 64 */
}

/*
 *  Machine Timer の現在のコンパレータ値（64bit）の設定
 */
Inline void
mtimer_set_mtimecmp(uint64_t cmp)
{
#if __riscv_xlen == 64
    sil_swrdw_mem(MTIMER_MTIMECMP_SELF, cmp);
#elif __riscv_xlen == 32
    uint32_t cmp_u, cmp_l;
    cmp_u = (uint32_t)(cmp >> 32);
    cmp_l = (uint32_t)cmp;
    sil_swrw_mem((void *)MTIMER_MTIMECMP_SELF_U, 0xFFFFFFFFU);
    sil_swrw_mem((void *)MTIMER_MTIMECMP_SELF_L, cmp_l);
    sil_swrw_mem((void *)MTIMER_MTIMECMP_SELF_U, cmp_u);
#endif /* __riscv_xlen == 64 */
}

/*
 *  高分解能タイマの起動処理
 */
extern void	target_hrt_initialize(EXINF exinf);

/*
 *  高分解能タイマの停止処理
 */
extern void	target_hrt_terminate(EXINF exinf);

/*
 *  高分解能タイマの現在のカウント値の読出し
 *
 *  この関数はシステムログへのログ情報の出力時に呼び出されるため，この
 *  関数内でsyslogやassertを使ってはならない（無限の再帰呼出しが起こ
 *  る）．
 */
Inline HRTCNT
target_hrt_get_current(void)
{
    /*
     *  Machine Timerのカウント値（64ビット）を読み出し，
     *  MTIMER_FREQ_MHZで除し，32ビットに切り詰めた値を返す．
     */
    return((HRTCNT)(mtimer_get_count() / MTIMER_FREQ_MHZ));
}

/*
 *  高分解能タイマへの割込みタイミングの設定
 *
 *  高分解能タイマを，hrtcntで指定した値カウントアップしたら割込みを発
 *  生させるように設定する．
 */
Inline void
target_hrt_set_event(HRTCNT hrtcnt)
{
    /*
     *  コンパレータ値を，(現在のカウント値＋hrtcnt×MTIMER_FREQ_MHZ)
     *  に設定．
     */
    mtimer_set_mtimecmp(mtimer_get_count()
                                + (((uint64_t) hrtcnt) * MTIMER_FREQ_MHZ));
}

/*
 *  高分解能タイマへの割込みタイミングのクリア
 */
Inline void
target_hrt_clear_event(void)
{
    mtimer_set_mtimecmp(0xFFFFFFFFFFFFFFFFULL);
}

/*
 *  高分解能タイマ割込みの要求
 */
Inline void
target_hrt_raise_event(void)
{
    mtimer_set_mtimecmp(0U);
}

/*
 *  割込みタイミングに指定する最大値
 */
#ifndef USE_64BIT_HRTCNT

#if !defined(TCYC_HRTCNT) || (TCYC_HRTCNT > 4002000002U)
#define HRTCNT_BOUND		4000000002U
#else
#define HRTCNT_BOUND		(TCYC_HRTCNT - 2000000U)
#endif

#endif /* USE_64BIT_HRTCNT */

/*
 *  高分解能タイマ割込みハンドラ
 */
extern void	target_hrt_handler(void);

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_MTIMER_H */
