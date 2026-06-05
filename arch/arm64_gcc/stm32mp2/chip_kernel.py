# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのチップ依存部（STM32MP2用）
#

#
#  使用できる割込み番号とそれに対応する割込みハンドラ番号
#
INTNO_VALID = list(range(0, 416))
INHNO_VALID = INTNO_VALID

#
#  生成スクリプトのGIC依存部
#
IncludeTrb("gic_kernel.py")

#
#  生成スクリプトのコア依存部
#
IncludeTrb("core_kernel.py")
