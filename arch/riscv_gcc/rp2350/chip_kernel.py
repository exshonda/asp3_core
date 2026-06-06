# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのチップ依存部（RP2350 RISC-V（Hazard3）用）
#

#
#  使用できる割込み番号とそれに対応する割込みハンドラ番号
#  （ASP3の割込み番号INTNO = RP2350のIRQ番号(0〜51) + 1．
#  INTNO=0はSpurious割込みの番兵として空ける）
#
INTNO_VALID = list(range(1, 53))
INHNO_VALID = INTNO_VALID

#
#  生成スクリプトのコア依存部
#
IncludeTrb("core_kernel.py")
