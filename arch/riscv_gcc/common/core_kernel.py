# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのコア依存部（RISC-V用）
#
#   $Id: core_kernel.py (converted from core_kernel.trb) $
#

#
#  有効なCPU例外ハンドラ番号
#
EXCNO_VALID = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]

#
#  DEF_EXCで使用できるCPU例外ハンドラ番号
#
EXCNO_DEFEXC_VALID = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]

#
#  ターゲット非依存部のインクルード
#
IncludeTrb("kernel/kernel.py")

#
#  割込みハンドラテーブル
#
kernelCfgC.comment_header("Interrupt Handler Table")

kernelCfgC.add("const FP _kernel_inh_table[TMAX_INHNO + 1] = {")
for index, inhnoVal in enumerate(range(0, TMAX_INHNO + 1)):
    if index > 0:
        kernelCfgC.add(",")
    kernelCfgC.append("\t/* 0x%03x */ " % inhnoVal)
    if inhnoVal in cfgData["DEF_INH"]:
        kernelCfgC.append(f"(FP)({cfgData['DEF_INH'][inhnoVal]['inthdr']})")
    else:
        kernelCfgC.append("(FP)(_kernel_default_int_handler)")
kernelCfgC.add()
kernelCfgC.add2("};")

#
#  割込み要求ライン設定テーブル
#
if USE_INTCFG_TABLE:
    kernelCfgC.comment_header("Interrupt Configuration Table")

    kernelCfgC.add("const uint8_t _kernel_intcfg_table[TMAX_INTNO + 1] = {")
    for index, intnoVal in enumerate(range(0, TMAX_INTNO + 1)):
        if index > 0:
            kernelCfgC.add(",")
        kernelCfgC.append("\t/* 0x%03x */ " % intnoVal)
        if intnoVal in cfgData["CFG_INT"]:
            kernelCfgC.append("1U")
        else:
            kernelCfgC.append("0U")
    kernelCfgC.add()
    kernelCfgC.add2("};")

#
#  CPU例外ハンドラテーブル
#
kernelCfgC.comment_header("CPU Exception Handler Table")

kernelCfgC.add("const FP _kernel_exc_table[TMAX_EXCNO + 1] = {")
for index, excnoVal in enumerate(range(0, TMAX_EXCNO + 1)):
    if index > 0:
        kernelCfgC.add(",")
    kernelCfgC.append(f"\t/* {excnoVal} */ ")
    if excnoVal in cfgData["DEF_EXC"]:
        kernelCfgC.append(f"(FP)({cfgData['DEF_EXC'][excnoVal]['exchdr']})")
    else:
        kernelCfgC.append("(FP)(_kernel_default_exc_handler)")
kernelCfgC.add()
kernelCfgC.add2("};")
