# -*- coding: utf-8 -*-
#
#		パス3の生成スクリプトのコア依存部（ARM用）
#
#   $Id: core_check.py (converted from core_check.trb) $
#

#
#  ターゲット非依存部のインクルード
#
IncludeTrb("kernel/kernel_check.py")

#
#  割込みハンドラテーブルに関するチェック
#
# DEF_INHで登録した割込みハンドラとコンフィギュレータで生成した割込み
# ハンドラのみチェックする．逆に言うと，default_int_handlerのチェック
# は行わない．
#
inhTable = SYMBOL("_kernel_inh_table")
for _, params in cfgData["DEF_INH"].items():
    inthdr = PEEK(inhTable + params["inhno"] * sizeof_FP, sizeof_FP)

    #	割込みハンドラの先頭番地のチェック［NGKI3056］
    if (inthdr & (CHECK_FUNC_ALIGN - 1)) != 0:
        error_wrong_id("E_PAR", params, "inthdr", "inhno", "not aligned")
    if CHECK_FUNC_NONNULL and inthdr == 0:
        error_wrong_id("E_PAR", params, "inthdr", "inhno", "null")

#
#  CPU例外ハンドラテーブルに関するチェック
#
# DEF_EXCで登録したCPU例外ハンドラのみチェックする．逆に言うと，
# default_exc_handlerのチェックは行わない．
#
excTable = SYMBOL("_kernel_exc_table")
for _, params in cfgData["DEF_EXC"].items():
    exchdr = PEEK(excTable + params["excno"] * sizeof_FP, sizeof_FP)

    # CPU例外ハンドラの先頭番地のチェック［NGKI3135］
    if (exchdr & (CHECK_FUNC_ALIGN - 1)) != 0:
        error_wrong_id("E_PAR", params, "exchdr", "excno", "not aligned")
    if CHECK_FUNC_NONNULL and exchdr == 0:
        error_wrong_id("E_PAR", params, "exchdr", "excno", "null")
