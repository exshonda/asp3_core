# Chip (I.MX RT600) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: チップ（SoC）依存の知識。I.MX RT600（Cortex-M33）の割込みはコア内蔵の
#       NVIC で管理されるため，現時点でチップ固有の追加項目は無く，
#       コア層(core_os_awareness, NVIC レジスタ配置)の API を再エクスポートする。
#
# core_os_awareness.py は arch/arm_m_gcc/common にあるため，本ファイルからの相対パスで
# sys.path に追加してから import する（__file__ は Python import 時に設定される）。

import os
import sys

# 下位層(core)の import で .pyc を生成させない（ソースツリーに __pycache__ を残さない）。
# 上位層を経由せず本モジュールを直接 import した場合への防御。
sys.dont_write_bytecode = True

sys.path.insert(0, os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "../common")))

import core_os_awareness

# チップ固有の追加は今回なし。コア層の API をそのまま公開する。
int_enabled = core_os_awareness.int_enabled
int_pending = core_os_awareness.int_pending
inh_handler = core_os_awareness.inh_handler
primap_bit = core_os_awareness.primap_bit
