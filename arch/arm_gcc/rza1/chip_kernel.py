# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのチップ依存部（RZ/A1用）
#
#   $Id: chip_kernel.py (converted from chip_kernel.trb) $
#

#
#  使用できる割込み番号とそれに対応する割込みハンドラ番号
#
if TOPPERS_RZA1H:
    INTNO_VALID = list(range(0, 587))
else:
    INTNO_VALID = list(range(0, 538))
INHNO_VALID = INTNO_VALID

#
#  ターゲット定義の割込み要求ライン属性
#
TARGET_INTATR = TA_NEGEDGE | TA_POSEDGE | TA_BOTHEDGE

#
#  生成スクリプトのコア依存部
#
IncludeTrb("core_kernel.py")

#
#  割込み要求ライン属性に関するターゲット依存のエラーチェック
#
for _, params in cfgData["CFG_INT"].items():
    if INTNO_IRQ0 <= params["intno"] and params["intno"] <= INTNO_IRQ7:
        # IRQ割込みの場合
        #（TA_EDGEがセットされている場合）
        if (params["intatr"] & TA_EDGE) != 0:
            error_illegal_sym("E_RSATR", params, "intatr", "intno")
    else:
        # その他の割込みの場合
        #（TA_NEGEDGE，TA_POSEDGE，TARGET_BOTHEDGEがセットされている場合）
        if (params["intatr"] & TARGET_INTATR) != 0:
            error_illegal_sym("E_RSATR", params, "intatr", "intno")
