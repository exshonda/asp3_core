# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトのターゲット依存部（ダミーターゲット用）
#
#   $Id: target_kernel.py (converted from target_kernel.trb) $
#

#
#  使用できる割込み番号とそれに対応する割込みハンドラ番号
#
INTNO_VALID = list(range(0, 32))
INHNO_VALID = INTNO_VALID

#
#  CFG_INTで使用できる割込み優先度
#
INTPRI_CFGINT_VALID = list(range(TMIN_INTPRI - 1, TMAX_INTPRI + 1))

#
#  カーネル管理に固定されている割込み
#
INTNO_FIX_KERNEL = [30]
INHNO_FIX_KERNEL = [30]

#
#  カーネル管理外に固定されている割込み
#
INTNO_FIX_NONKERNEL = [31]
INHNO_FIX_NONKERNEL = [31]

#
#  使用できるCPU例外ハンドラ番号
#
EXCNO_VALID = list(range(0, 8))

#
#  ターゲット非依存部のインクルード
#
IncludeTrb("kernel/kernel.py")
