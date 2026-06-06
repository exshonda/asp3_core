# Target (ZCU102) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: ターゲット（ボード）依存の知識。本ボードはチップ(ZynqMP)の機能をそのまま使い，
#       現時点でボード固有の追加項目は無いため，chip_os_awareness の API を再エクスポートする。
#
# chip_os_awareness.py は arch/arm64_gcc/zynqmp にあるため，本ファイルからの相対パスで
# sys.path に追加してから import する。os_awareness.py（scripts/gdb_os_aware/）は
# この target_os_awareness を（発見できれば）import して，割込み状態の表示等に用いる。

import os
import sys

# 下位層(chip/core)の import で .pyc を生成させない（ソースツリーに __pycache__ を残さない）。
sys.dont_write_bytecode = True

sys.path.insert(0, os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../arch/arm64_gcc/zynqmp")))

import chip_os_awareness

# GICD レジスタ読出し（intr の ena/pend 列）は既定で無効。
#
# QEMU 8.2（Ubuntu 24.04 同梱）の xlnx-zcu102 マシンは，gdbstub 経由で
# GIC-400 Distributor 領域（0xF9010000）を読むと QEMU 自体が segfault する
# （UART 等の他デバイス領域は読める。QEMU 11 の zynq/GIC では発生しない＝
# QEMU 側の不具合）。デバッグセッションを道連れにしないため既定では
# int_enabled/int_pending を公開せず（→ engine は ena/pend 列を省略），
# 新しい QEMU や実機接続時は環境変数 ASP3_OSA_GICREAD=1 で有効化する。
if os.environ.get("ASP3_OSA_GICREAD", "") == "1":
    int_enabled = chip_os_awareness.int_enabled
    int_pending = chip_os_awareness.int_pending

# ハンドラ表・primap はメモリ読出しのみで安全なのでそのまま公開する。
inh_handler = chip_os_awareness.inh_handler
primap_bit = chip_os_awareness.primap_bit
