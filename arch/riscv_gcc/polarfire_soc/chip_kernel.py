# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのチップ依存部（PolarFire SoC 用）
#
#   $Id: chip_kernel.py (converted from chip_kernel.trb) $
#

#
#  使用できる割込み番号とそれに対応する割込みハンドラ番号
#  （PLICの割込み番号1〜182．0は予約）
#
INTNO_VALID = list(range(1, 183))
INHNO_VALID = INTNO_VALID

#
#  生成スクリプトのPLIC依存部
#
IncludeTrb("plic_kernel.py")

#
#  生成スクリプトのコア依存部
#
IncludeTrb("core_kernel.py")
