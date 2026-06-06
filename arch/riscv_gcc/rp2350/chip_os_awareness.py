# Chip (RP2350 RISC-V / Hazard3 Xh3irq) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: チップ（SoC）依存の知識。RP2350 の RISC-V コア（Hazard3）の割込みは
#       カスタム割込みコントローラ Xh3irq（カスタム CSR: meiea/meipa 等）で
#       管理される。「指定 INTNO の割込み許可/禁止・ペンディング状態」を
#       Xh3irq から読んで返す。
#
# ■ 窓方式 CSR の読出し
#   meiea(0xbe0)/meipa(0xbe1) は「書込みデータの bit4:0＝窓 INDEX・
#   bit31:16＝窓データ」の窓方式で，デバッガの単純なレジスタ読出しでは
#   窓 0 しか見えない（INDEX の選択には書込みが必要で，csrw だと窓データ
#   まで書いてしまい meiea を破壊する）。そこで OpenOCD の
#   `riscv exec_progbuf` で `csrrs s0, <csr>, s0`（set ビット＝INDEX のみ．
#   INDEX フィールドは self-clearing なので副作用なし）を 1 命令実行し，
#   同一命令の読出し値（bit31:16）から窓データを得る。s0 は実行前後で
#   退避・復元する。
#
# ■ INTNO の対応
#   ASP3 の INTNO = RP2350 の IRQ 番号 + 1（xh3irq_kernel_impl.h 参照）。
#
# core_os_awareness.py は arch/riscv_gcc/common にあるため，本ファイルからの
# 相対パスで sys.path に追加してから import する。

import os
import sys

# 下位層(core)の import で .pyc を生成させない（ソースツリーに __pycache__ を残さない）。
sys.dont_write_bytecode = True

sys.path.insert(0, os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "../common")))

import core_os_awareness

import gdb

# csrrs s0, <csr>, s0 の命令語（rd=rs1=x8）
_CSRRS_S0_MEIEA = 0xBE042473    # csrrs s0, 0xbe0, s0
_CSRRS_S0_MEIPA = 0xBE142473    # csrrs s0, 0xbe1, s0


def _xh3irq_window_read(insn, index):
    """窓方式CSRの窓 index の16ビットデータを読む（exec_progbuf使用）。"""
    saved = int(gdb.parse_and_eval("$s0"))
    try:
        gdb.execute("set $s0 = %d" % int(index), to_string=True)
        gdb.execute("monitor riscv exec_progbuf 0x%08x" % insn, to_string=True)
        # OpenOCD側で実行されたためgdbのレジスタキャッシュを無効化する
        try:
            gdb.execute("maintenance flush register-cache", to_string=True)
        except gdb.error:
            gdb.execute("flushregs", to_string=True)
        return (int(gdb.parse_and_eval("$s0")) >> 16) & 0xFFFF
    finally:
        gdb.execute("set $s0 = %d" % saved, to_string=True)


def int_enabled(intno):
    """指定 INTNO の割込み許可状態（True=許可 / False=禁止）。meieaを読む。"""
    irq = int(intno) - 1
    window = _xh3irq_window_read(_CSRRS_S0_MEIEA, irq // 16)
    return bool(window & (1 << (irq % 16)))


def int_pending(intno):
    """指定 INTNO のペンディング状態（True=ペンディング）。meipaを読む。"""
    irq = int(intno) - 1
    window = _xh3irq_window_read(_CSRRS_S0_MEIPA, irq // 16)
    return bool(window & (1 << (irq % 16)))


# 割込みハンドラ番地の取得（_kernel_inh_table[INTNO]．INTNOで直接添字付け）と
# レディキュービットマップのビット方向は riscv 共通部の知識なので core 層の
# 実装をそのまま公開する。
inh_handler = core_os_awareness.inh_handler
primap_bit = core_os_awareness.primap_bit
