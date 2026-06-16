/*
 *  TOPPERS/ASP3 Core
 *
 *  MVE(Helium) VPR レジスタのコンテキスト退避テスト
 *
 *  Cortex-M55/M85 等 ARMv8.1-M＋MVE ターゲット専用。コンテキストスイッチを
 *  跨いで各タスクの VPR（ベクタ述語/ループ制御レジスタ）が保持されることを
 *  検証する。arch 共通部 core_support.S の MVE VPR 退避（#ifdef __ARM_FEATURE_MVE）
 *  に対応する（docs/dev/mps3-an547.md Step 2）。MVE 無効ビルドでは SKIP する。
 *
 *  本ファイルは TOPPERS ライセンスに従う新規追加（asp3_core）。
 */

#include <kernel.h>

/*
 *  ターゲット依存の定義
 */
#include "target_test.h"

/*
 *  優先度の定義
 */
#define HIGH_PRIORITY	4		/* 高優先度 */
#define MID_PRIORITY	9		/* 中優先度 */

#ifndef STACK_SIZE
#define	STACK_SIZE		4096
#endif /* STACK_SIZE */

/*
 *  関数のプロトタイプ宣言
 */
#ifndef TOPPERS_MACRO_ONLY

extern void	task1(EXINF exinf);
extern void	task2(EXINF exinf);

#endif /* TOPPERS_MACRO_ONLY */
