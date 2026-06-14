/*
 *  TTSP3 適合性テスト：ターゲット依存処理（zcu102_arm64_gcc / Cortex-A53）
 *
 *  asp3_core 側に置く TTSP3 ターゲットテスト資産（TTSP3 本体は不変）．
 *  詳細は ttsp_target_test.h 冒頭・docs/dev/ttsp3-conformance.md を参照．
 *
 *  純カーネル系モジュールのカバーが目的．HWタイマ早送り（gain_tick）・
 *  割込み発生（int_raise）・CPU例外（cpuexc）を呼ぶテストはドライバが
 *  SKIP するため、stop_tick/start_tick/gain_tick は no-op とする．
 *  int_raise は arm64 の raise_int（GIC）を呼ぶ実装、cpuexc は
 *  RAISE_CPU_EXCEPTION とする（いずれも SKIP モジュール専用）．
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
    raise_int(intno);
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
