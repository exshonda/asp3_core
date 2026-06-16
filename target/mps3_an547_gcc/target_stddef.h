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
 *  t_stddef.h のターゲット依存部（ARM MPS3-AN547 用）
 *
 *  このインクルードファイルは，t_stddef.h の先頭でインクルードされる．
 *  他のインクルードファイルに先立って処理されるため，他のインクルードファ
 *  イルに依存してはならない．
 */

#ifndef TOPPERS_TARGET_STDDEF_H
#define TOPPERS_TARGET_STDDEF_H

/*
 *  ターゲットを識別するためのマクロの定義
 */
#define TOPPERS_MPS3_AN547      /* システム略称 */

/*
 *  開発環境で共通な定義
 */
#ifndef TOPPERS_MACRO_ONLY
#include <stdint.h>
#endif /* TOPPERS_MACRO_ONLY */
#include "tool_stddef.h"

/*
 *  プロセッサで共通な定義
 */
#include <core_stddef.h>

/*
 *  アサーションの失敗時の実行中断処理
 */
#ifndef TOPPERS_MACRO_ONLY
#ifndef TECSGEN

Inline void
TOPPERS_assert_abort(void)
{
    while (1) ;
}

#endif /* TECSGEN */
#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_TARGET_STDDEF_H */
