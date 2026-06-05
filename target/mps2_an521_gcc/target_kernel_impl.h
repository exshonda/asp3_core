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
 *  ターゲット依存部モジュール（ARM MPS2-AN521 用）
 *
 *  カーネルのターゲット依存部のインクルードファイル．kernel_impl.h のター
 *  ゲット依存部の位置付けとなる．
 */

#ifndef TOPPERS_TARGET_KERNEL_IMPL_H
#define TOPPERS_TARGET_KERNEL_IMPL_H

/*
 *  ボードのハードウェア資源定義
 */
#include "mps2_an521.h"

/*
 *  TBITW_IPRI の定義のため読み込み
 */
#include <sil.h>

/*
 *  デフォルトの非タスクコンテキスト用のスタック領域の定義
 */
#define DEFAULT_ISTKSZ    (0x1000)  /* 4KByte */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  ターゲットシステム依存の初期化
 */
extern void target_initialize(void);

/*
 *  ターゲットシステムの終了
 */
extern void target_exit(void) NoReturn;

/*
 *  エラー発生時の処理
 */
extern void Error_Handler(void);

#endif /* TOPPERS_MACRO_ONLY */

/*
 *  トレースログに関する設定
 */
#ifdef TOPPERS_ENABLE_TRACE
#include "logtrace/trace_config.h"
#endif /* TOPPERS_ENABLE_TRACE */

/*
 *  コア依存モジュール（ARM-M 用）
 */
#include <core_kernel_impl.h>

#endif /* TOPPERS_TARGET_KERNEL_IMPL_H */
