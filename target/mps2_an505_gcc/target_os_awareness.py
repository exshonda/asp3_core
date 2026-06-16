# Target (MPS2-AN505) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: ターゲット（ボード）依存の知識。本ターゲットはチップ層を持たず
#       コア（arm_m 共通）の上に直接実装されている（target.cmake が
#       arch/arm_m_gcc/common/arch.cmake を直接 include する構成）ため，
#       core_os_awareness の API を再エクスポートする。
#
# core_os_awareness.py は arch/arm_m_gcc/common にあるため，本ファイルからの相対パスで
# sys.path に追加してから import する。os_awareness.py（scripts/gdb_os_aware/）は
# この target_os_awareness を（発見できれば）import して，割込み状態の表示等に用いる。
#
# 注意: mps2-an505 は Secure 動作（QEMU -cpu cortex-m33）。NVIC 読み出しの
# セキュア/非セキュアエイリアスについては core_os_awareness.py 冒頭を参照。

import os
import sys

# 下位層(core)の import で .pyc を生成させない（ソースツリーに __pycache__ を残さない）。
sys.dont_write_bytecode = True

sys.path.insert(0, os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../arch/arm_m_gcc/common")))

import core_os_awareness

# ボード固有の追加は今回なし。コア層の API をそのまま公開する。
int_enabled = core_os_awareness.int_enabled
int_pending = core_os_awareness.int_pending
inh_handler = core_os_awareness.inh_handler
primap_bit = core_os_awareness.primap_bit
