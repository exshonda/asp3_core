/*
 *  TTSP3 適合性テスト：ターゲット依存処理（mps2_an386_gcc / Cortex-M4）
 *
 *  asp3_core 側に置く TTSP3 ターゲットテスト資産（TTSP3 本体は不変）．
 *  詳細は ttsp_target_test.h 冒頭・docs/dev/ttsp3-conformance.md を参照．
 *
 *  純カーネル系モジュールのカバーが目的．HWタイマ早送り（gain_tick）・
 *  割込み発生（int_raise）・CPU例外（cpuexc）を実行時に呼ぶモジュール
 *  （cyclic / alarm / time_event / interrupt / exception）はドライバ側で
 *  SKIP するため、本ファイルでは：
 *    - stop_tick / start_tick … no-op（テスト冒頭の防御的呼び出しのみ。
 *      QEMU 上の短時間実行では spurious timeout は起きない＝実証済）
 *    - gain_tick … 未対応（SKIP モジュール専用。呼ばれない）
 *    - int_raise … 実コール（raise_int）
 *    - cpuexc_raise … RAISE_CPU_EXCEPTION（SKIP モジュール専用）
 *  とする。HW モジュール本対応時は本ファイルを実装し skip_modules を外す．
 */

#include "kernel/kernel_impl.h"
#include <sil.h>
#include "ttsp_target_test.h"

/*
 *  ティック更新の停止／再開
 *
 *  純カーネル系では防御的に呼ばれるのみのため no-op とする
 *  （HW タイマ制御を要するモジュールは run_ttsp.py で SKIP）．
 */
void
ttsp_target_stop_tick(void)
{
}

void
ttsp_target_start_tick(void)
{
}

/*
 *  ティックの更新（HWタイマ早送り）
 *
 *  cyclic / alarm / time_event 専用。これらは SKIP のため呼ばれない．
 *  本対応時に arm_m の HRT（SysTick）操作で実装する．
 */
void
ttsp_target_gain_tick(void)
{
}

/*
 *  割込みの発生
 */
void
ttsp_int_raise(INTNO intno)
{
    raise_int(intno);
}

/*
 *  CPU例外の発生（exception モジュール専用＝SKIP）
 */
void
ttsp_cpuexc_raise(EXCNO excno)
{
    if (excno == TTSP_EXCNO_A) {
        RAISE_CPU_EXCEPTION;
    }
}

/*
 *  CPU例外発生時のフック処理
 */
void
ttsp_cpuexc_hook(EXCNO excno, void* p_excinf)
{
}

/*
 *  割込み要求のクリア
 */
void
ttsp_clear_int_req(INTNO intno)
{
}
