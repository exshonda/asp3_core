# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのターゲット依存部（CT11MPCore用）
#

#
#  使用できる割込み番号とそれに対応する割込みハンドラ番号
#
INTNO_VALID = list(range(0, 48))
INHNO_VALID = INTNO_VALID

#
#  生成スクリプトのコア依存部
#
IncludeTrb("core_kernel.py")
