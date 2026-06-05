# -*- coding: utf-8 -*-
#
#		パス3の生成スクリプトのターゲット依存部（ダミーターゲット用）
#
#   $Id: target_check.py (converted from target_check.trb) $
#

#
#  ターゲット非依存部のインクルード
#
IncludeTrb("kernel/kernel_check.py")

#
#  割込みハンドラ初期化ブロックに関するチェック
#
inhinib = SYMBOL("_kernel_inhinib_table")
for _, params in cfgData["DEF_INH"].items():
    # inthdrがプログラムの先頭番地として正しくない場合（E_PAR）［NGKI3056］
    inthdr = PEEK(inhinib + offsetof_INHINIB_int_entry, sizeof_FP)
    if (inthdr & (CHECK_FUNC_ALIGN - 1)) != 0:
        error_wrong_id("E_PAR", params, "inthdr", "inhno", "not aligned")
    if CHECK_FUNC_NONNULL and inthdr == 0:
        error_wrong_id("E_PAR", params, "inthdr", "inhno", "null")
    inhinib += sizeof_INHINIB

#
#  CPU例外ハンドラ初期化ブロックに関するチェック
#
excinib = SYMBOL("_kernel_excinib_table")
for _, params in cfgData["DEF_EXC"].items():
    # exchdrがプログラムの先頭番地として正しくない場合（E_PAR）［NGKI3135］
    exchdr = PEEK(excinib + offsetof_EXCINIB_exc_entry, sizeof_FP)
    if (exchdr & (CHECK_FUNC_ALIGN - 1)) != 0:
        error_wrong_id("E_PAR", params, "exchdr", "excno", "not aligned")
    if CHECK_FUNC_NONNULL and exchdr == 0:
        error_wrong_id("E_PAR", params, "exchdr", "excno", "null")
    excinib += sizeof_EXCINIB
