/*
 *  TTSP3 適合性テスト：ターゲット依存定義（mps3_an547_gcc / Cortex-M55）
 *
 *  asp3_core 側に置く TTSP3 ターゲットテスト資産（TTSP3 本体は不変＝
 *  読み取り専用とする方針のため・docs/dev/ttsp3-conformance.md）。
 *  TTSP3 library/ASP/target/lpc55s69evk_gcc（同 arm_m_gcc）を
 *  ベースに、asp3_core の arm_m 規約へ合わせた版．
 *
 *  本ファイルは TTSP3 library/ASP/test/ttsp_test_lib.h から include される．
 *  kernel.h / sil.h / TCB 定義は include 側で取り込み済み．
 *
 *  カバー範囲：HWタイマ早送り（ttsp_target_gain_tick）・割込み発生
 *  （ttsp_int_raise）・CPU例外（ttsp_cpuexc_raise）を要するモジュール
 *  （cyclic / alarm / time_event / interrupt / exception）はドライバ
 *  run_ttsp.py の skip_modules で SKIP する．それ以外（純カーネル系）の
 *  ティック停止/再開はテスト冒頭の防御的呼び出しのみで、QEMU 上の短時間
 *  実行では no-op で正しく通る（実証済）．
 */

#ifndef TTSP_TARGET_TEST_H
#define TTSP_TARGET_TEST_H

/*
 *  CPU例外を発生させる命令（arm_m：未定義命令）
 *  asp3_core arch/arm_m_gcc/common/core_test.h と同一．
 */
#define RAISE_CPU_EXCEPTION     Asm("udf #0")

/*
 *  タスク／非タスクコンテキストのスタックサイズ
 */
#define TTSP_TASK_STACK_SIZE      2048
#define TTSP_NON_TASK_STACK_SIZE  2048

/*
 *  各種異常値（ターゲット非依存・lpc55s69evk_gcc と同じ）
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
#define TTSP_GE_TIMER_INTPRI   TMIN_INTPRI  /* タイマ割込み優先度より高い優先度 */
#define TTSP_HIGH_INTPRI       -5           /* 割込み優先度高 */
#define TTSP_MID_INTPRI        -4           /* 割込み優先度中 */
#define TTSP_LOW_INTPRI        -3           /* 割込み優先度低 */

/*
 *  割込み番号（interrupt モジュールは SKIP のため値はコンパイル用）
 */
#define TTSP_INTNO_A   16
#define TTSP_INTNO_B   17
#define TTSP_INTNO_C   18
#define TTSP_INTNO_D   19
#define TTSP_INTNO_E   20
#define TTSP_INTNO_F   22
#define TTSP_INVALID_INTNO  168  /* ターゲット非サポートの割込み番号 */
#define TTSP_NOT_SET_INTNO  46   /* 割込み属性が設定されていない割込み番号 */

/*
 *  割込みハンドラ番号
 */
#define TTSP_INHNO_A   TTSP_INTNO_A
#define TTSP_INHNO_B   TTSP_INTNO_B
#define TTSP_INHNO_C   TTSP_INTNO_C
#define TTSP_INVALID_INHNO  TTSP_INVALID_INTNO

/*
 *  CPU例外ハンドラ番号（exception モジュールは SKIP）
 *  arm_m：UsageFault=6 / SVCall=11．
 */
#define TTSP_EXCNO_A   6   /* 未定義命令＝UsageFault（復帰可能） */
#define TTSP_EXCNO_B   11  /* SVCall（本番号で例外を発生させるケースはない） */
#define TTSP_INVALID_EXCNO  7

/*
 *  タイムアウト用変数・fch_hrt 係数（lpc55s69evk_gcc と同じ）
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

/*
 *  ref_tsk 用：スタック先頭・サイズ取得（arm_m の TCB レイアウト）
 */
#define ttsp_target_get_stksz(p_tinib)  \
    ((size_t)((p_tinib)->tskinictxb.stk_bottom) - (size_t)((p_tinib)->tskinictxb.stk_top))
#define ttsp_target_get_stk(p_tinib)    ((p_tinib)->tskinictxb.stk_top)

#endif /* TTSP_TARGET_TEST_H */
