/*
 *  TTSP3 適合性テスト：ターゲット依存処理（polarfire_soc_kit_gcc / RV64GC）
 *
 *  asp3_core 側に置く TTSP3 ターゲットテスト資産（TTSP3 本体は不変）．
 *  詳細は ttsp_target_test.h 冒頭・docs/dev/ttsp3-conformance.md を参照．
 *
 *  純カーネル系モジュールのカバーが目的．HWタイマ早送り（gain_tick）・
 *  割込み発生（int_raise）・CPU例外（cpuexc）を呼ぶテストはドライバが
 *  SKIP するため、stop_tick/start_tick/gain_tick は no-op とする．
 *  int_raise は PLIC のソフト発生手段を本資産では用意しないため no-op
 *  （interrupt モジュールは SKIP＝呼ばれない）．cpuexc は RAISE_CPU_EXCEPTION．
 */

#include "kernel/kernel_impl.h"
#include <sil.h>
#include "ttsp_target_test.h"

void
ttsp_target_stop_tick(void)
{
}

void
ttsp_target_start_tick(void)
{
}

void
ttsp_target_gain_tick(void)
{
}

void
ttsp_int_raise(INTNO intno)
{
    /* interrupt モジュールは SKIP のため呼ばれない（PLIC ソフト発生は未提供） */
    (void) intno;
}

void
ttsp_cpuexc_raise(EXCNO excno)
{
    if (excno == TTSP_EXCNO_A) {
        RAISE_CPU_EXCEPTION;
    }
}

void
ttsp_cpuexc_hook(EXCNO excno, void* p_excinf)
{
}

void
ttsp_clear_int_req(INTNO intno)
{
}
