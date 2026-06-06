# Core (RISC-V / PLIC) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: コア（riscv 共通）依存の低レベル知識。
#       - PLIC のレジスタ配置を知り，「指定 INTNO の割込み許可/禁止・ペンディング状態」
#         を PLIC のレジスタから読む処理を提供する。PLIC のベースアドレスと
#         コンテキスト index（ハート×特権モード毎）はチップ依存なので引数で受け取る
#         （chip_os_awareness が渡す）。
#       - riscv 共通部の割込みハンドラテーブル _kernel_inh_table（INTNO＝PLIC ソース ID
#         で添字付けした FP 配列）から「指定 INTNO のハンドラ番地」を引く。
#       - レディキュービットマップのビット方向 primap_bit(pri) を提供する。
#
# riscv の INTNO は PLIC のソース ID そのもの（外部割込みのみ。マシンタイマ等の
# CLINT 割込みは静的 API の対象外で intinib に現れない）。
#
# PLIC レジスタは動的読み出しのため，実行中ターゲット/コアダンプへの接続が必要（要 halt）。
# ハンドラテーブルは const（.rodata）なので ELF 単体（静的）でも読める。

import gdb

# PLIC レジスタオフセット（arch/riscv_gcc/common/plic_kernel_impl.h と一致）
_PLIC_IPEND_OFF = 0x001000   # 割込みペンディング（read = ペンディング状態）
_PLIC_IEM_OFF   = 0x002000   # 割込みイネーブル（コンテキスト毎，read = 許可状態）
_PLIC_IEM_CTX   = 0x80       # コンテキスト毎の IEM ストライド


def _read32(addr):
    addr &= 0xFFFFFFFFFFFFFFFF
    return int(gdb.parse_and_eval("*(unsigned int *)0x%x" % addr)) & 0xFFFFFFFF


def plic_int_enabled(plic_base, cidx, intno):
    """PLIC INTNO が許可状態か（IEM[cidx] の該当ビット）。"""
    reg = plic_base + _PLIC_IEM_OFF + cidx * _PLIC_IEM_CTX + (intno // 32) * 4
    return bool(_read32(reg) & (1 << (intno % 32)))


def plic_int_pending(plic_base, intno):
    """PLIC INTNO がペンディングか（IPEND の該当ビット）。"""
    reg = plic_base + _PLIC_IPEND_OFF + (intno // 32) * 4
    return bool(_read32(reg) & (1 << (intno % 32)))


def inh_handler(intno):
    """指定 INTNO の割込みハンドラ番地を返す。

    riscv 共通部の _kernel_inh_table[]（INTNO で添字付けした FP 配列）を読む。
    読めなければ None。ASP3 はシングルプロセッサなのでプロセッサ index は無い。
    """
    try:
        fp = gdb.parse_and_eval("_kernel_inh_table[%d]" % int(intno))
        return int(fp) & 0xFFFFFFFFFFFFFFFF
    except gdb.error:
        return None


def primap_bit(pri):
    """内部優先度 pri のレディキュービットマップのビット値。

    riscv は PRIMAP_BIT を上書きしない（kernel/task.c 既定の 1<<pri）。
    """
    return 1 << int(pri)
