# Chip (PolarFire SoC) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: チップ（SoC）依存の知識。PolarFire SoC（MPFS）の PLIC ベースアドレスと
#       ブートハートの PLIC コンテキスト index を持ち，コア層
#       (core_os_awareness, PLIC レジスタ配置)を使って「指定 INTNO の割込み
#       許可/禁止・ペンディング状態」を返す。
#
# core_os_awareness.py は arch/riscv_gcc/common にあるため，本ファイルからの相対パスで
# sys.path に追加してから import する（__file__ は Python import 時に設定される）。

import os
import sys

# 下位層(core)の import で .pyc を生成させない（ソースツリーに __pycache__ を残さない）。
# 上位層を経由せず本モジュールを直接 import した場合への防御。
sys.dont_write_bytecode = True

sys.path.insert(0, os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "../common")))

import core_os_awareness

# PolarFire SoC PLIC ベース（polarfire_soc.h: PLIC_BASE）
PLIC_BASE = 0x0C000000

# ブートハートの PLIC コンテキスト index（chip_kernel_impl.h: PLIC_BOOT_CIDX。
# E51=hart0 はコンテキスト 0（M-mode のみ），U54=hart1〜 は M/S 2 コンテキストずつ
# なので ((TOPPERS_BOOT_HARTID - 1) * 2) + 1。TOPPERS_BOOT_HARTID=1（polarfire_soc.h））
PLIC_BOOT_CIDX = ((1 - 1) * 2) + 1


def int_enabled(intno):
    """指定 INTNO の割込み許可状態（True=許可 / False=禁止）。"""
    return core_os_awareness.plic_int_enabled(PLIC_BASE, PLIC_BOOT_CIDX, intno)


def int_pending(intno):
    """指定 INTNO のペンディング状態（True=ペンディング）。"""
    return core_os_awareness.plic_int_pending(PLIC_BASE, intno)


# 割込みハンドラ番地の取得（_kernel_inh_table）とレディキュービットマップの
# ビット方向（primap_bit）は riscv 共通部の知識なので core 層の実装をそのまま
# 公開する（チップ固有の追加なし）。
inh_handler = core_os_awareness.inh_handler
primap_bit = core_os_awareness.primap_bit
