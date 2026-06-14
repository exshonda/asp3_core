/*
 *  TTSP3 適合性テスト：ターゲット依存定義（zcu102_arm64_gcc / Cortex-A53）
 *
 *  asp3_core 側に置く TTSP3 ターゲットテスト資産（TTSP3 本体は不変＝
 *  読み取り専用とする方針・docs/dev/ttsp3-conformance.md）．
 *  arch/arm64_gcc の規約に合わせた版．
 *
 *  本ファイルは TTSP3 library/ASP/test/ttsp_test_lib.h から include される．
 *  kernel.h / sil.h / arm64.h は include 側で取り込み済み．
 *
 *  arm64_gcc は USE_TSKINICTXB 非定義のため、ref_tsk のスタック取得は
 *  汎用 lib 側の TINIB.stksz / TINIB.stk 直接参照を使う（get_stk マクロは
 *  定義しない）．
 *
 *  カバー範囲：HWタイマ早送り（gain_tick）・割込み発生（int_raise）・
 *  CPU例外（cpuexc）依存テストはドライバ run_ttsp.py で SKIP．時刻凍結
 *  （stop_tick）依存の timed-API 系は実行して失敗時に SKIP 再分類する．
 */

#ifndef TTSP_TARGET_TEST_H
#define TTSP_TARGET_TEST_H

/*
 *  CPU例外を発生させる命令（arm64：SVC）
 *  asp3_core arch/arm64_gcc/common/core_test.h と同一．
 */
#define RAISE_CPU_EXCEPTION     Asm("svc #0xF000")

/*
 *  タスク／非タスクコンテキストのスタックサイズ（AArch64＝64bit・例外フレーム大）
 */
#define TTSP_TASK_STACK_SIZE      4096
#define TTSP_NON_TASK_STACK_SIZE  4096

/*
 *  各種異常値
 */
#define TTSP_INVALID_FUNC_ADDRESS  0x123456
#define TTSP_INVALID_STK_ADDRESS   0x123456
#define TTSP_INVALID_MPF_ADDRESS   0x123456
#define TTSP_INVALID_VAR_ADDRESS   0x123456
#define TTSP_INVALID_STACK_SIZE    0x01

/*
 *  割込み優先度
 */
#define TTSP_TMIN_INTPRI       TMIN_INTPRI
#define TTSP_GE_TIMER_INTPRI   TMIN_INTPRI
#define TTSP_HIGH_INTPRI       -5
#define TTSP_MID_INTPRI        -4
#define TTSP_LOW_INTPRI        -3

/*
 *  割込み番号（interrupt モジュールは SKIP のため値はコンパイル用．
 *  GIC SPI＝32〜）
 */
#define TTSP_INTNO_A   32
#define TTSP_INTNO_B   33
#define TTSP_INTNO_C   34
#define TTSP_INTNO_D   35
#define TTSP_INTNO_E   36
#define TTSP_INTNO_F   37
#define TTSP_INVALID_INTNO  0x400
#define TTSP_NOT_SET_INTNO  48

#define TTSP_INHNO_A   TTSP_INTNO_A
#define TTSP_INHNO_B   TTSP_INTNO_B
#define TTSP_INHNO_C   TTSP_INTNO_C
#define TTSP_INVALID_INHNO  TTSP_INVALID_INTNO

/*
 *  CPU例外ハンドラ番号（exception モジュールは SKIP）
 *  arm64：EXCNO_CUR_SPX_SYNC=4（同期例外）．
 */
#define TTSP_EXCNO_A   4
#define TTSP_EXCNO_B   6
#define TTSP_INVALID_EXCNO  100

/*
 *  タイムアウト用変数・fch_hrt 係数
 */
#define TTSP_SIL_DLY_NSE_TIME  100000
#define TTSP_LOOP_COUNT        500000
#define TTSP_MOD_FCH_CNT       1

/*
 *  TTSP3用の関数（実体は ttsp_target_test.c）
 */
extern void ttsp_target_stop_tick(void);
extern void ttsp_target_start_tick(void);
extern void ttsp_target_gain_tick(void);
extern void ttsp_int_raise(INTNO intno);
extern void ttsp_cpuexc_raise(EXCNO excno);
extern void ttsp_cpuexc_hook(EXCNO excno, void* p_excinf);
extern void ttsp_clear_int_req(INTNO intno);

#endif /* TTSP_TARGET_TEST_H */
