/*
 *		t_stddef.hのターゲット依存部（STM32MP257F-DK用）
 *
 *  このヘッダファイルは，t_stddef.hの先頭でインクルードされる．他のファ
 *  イルからは直接インクルードすることはない．他のヘッダファイルに先立っ
 *  て処理されるため，他のヘッダファイルに依存してはならない．
 * 
 *  $Id: target_stddef.h 246 2020-07-09 06:31:08Z ertl-honda $
 */

#ifndef TOPPERS_TARGET_STDDEF_H
#define TOPPERS_TARGET_STDDEF_H

/*
 *  ターゲットを識別するためのマクロの定義
 */
#define TOPPERS_STM32MP257F_DK_ARM64             /* システム略称 */

/*
 *  チッブ依存で共通な定義
 */
#include "chip_stddef.h"

/*
 *  アサーションの失敗時の実行中断処理
 */
#ifndef TOPPERS_MACRO_ONLY

Inline void
TOPPERS_assert_abort(void)
{

}

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_TARGET_STDDEF_H */
