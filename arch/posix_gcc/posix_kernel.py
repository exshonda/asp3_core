# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのターゲット依存部（POSIX用）
#
#   $Id: posix_kernel.py (converted from posix_kernel.trb) $
#

#
#  使用できる割込み番号とそれに対応する割込みハンドラ番号
#
INTNO_VALID = list(range(0, TMAX_INTNO + 1))
INHNO_VALID = INTNO_VALID

#
#  スタック領域の確保関数
#
def AllocStack(stack, size):
    return f"{size}"

#
#  タスク初期化コンテキストブロックの生成
#
def GenerateTskinictxb(key, params):
    return f"{{ {params['tinib_stksz']} }}"

#
#  ターゲット非依存部のインクルード
#
IncludeTrb("kernel/kernel.py")

#
#  ターゲット依存の定義の生成
#
kernelCfgC.comment_header("Target-dependent Definitions (POSIX)")

#
#  割込み要求ラインに関するターゲット依存のエラーチェック
#
for _, params in sorted(cfgData["CFG_INT"].items()):
    if (params["intatr"] & TA_EDGE) == 0:
        error_ercd("E_RSATR", params, "level trigger is not supported "
                   "for %%intno in %apiname")

#
#  割込みハンドラの初期化に必要な情報
#

#
#  定義する割込みハンドラの数
#
kernelCfgC.add(f"""#define TNUM_DEF_INHNO\t{len(cfgData['DEF_INH'])}
const uint_t _kernel_tnum_def_inhno = TNUM_DEF_INHNO;
""")

if len(cfgData["DEF_INH"]) != 0:
    #
    #  割込みハンドラ初期化ブロック
    #
    kernelCfgC.add("const INHINIB _kernel_inhinib_table[TNUM_DEF_INHNO] = {")
    for index, (_, params) in enumerate(cfgData["DEF_INH"].items()):
        if index > 0:
            kernelCfgC.add(",")
        #  不正な割込みハンドラ番号を指定した場合に，コンフィギュレータが
        #  落ちないようにするための対策
        if int(params["inhno"]) in to_intno_val:
            intnoVal = to_intno_val[int(params["inhno"])]
            intnoParams = cfgData["CFG_INT"][intnoVal]
            kernelCfgC.append(f"\t{{ ({params['inhno']}), ({params['inhatr']}), "
                              f"{params['inthdr']}, ({intnoParams['intatr']}), "
                              f"({intnoParams['intpri']}) }}")
    kernelCfgC.add()
    kernelCfgC.add2("};")
else:
    kernelCfgC.add2("TOPPERS_EMPTY_LABEL(const INHINIB, "
                    "_kernel_inhinib_table);")
