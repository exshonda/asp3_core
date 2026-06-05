# Core (AArch64 / GICv2) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: コア（arm64 共通）依存の低レベル知識。
#       - GICv2 のレジスタ配置を知り，「指定 INTID の割込み許可/禁止状態」を GICD の
#         レジスタから読む処理を提供する。GICD のベースアドレスはチップ依存なので
#         引数で受け取る（chip_os_awareness が渡す）。
#       - arm64 共通部の割込みハンドラテーブル _kernel_inh_table（INTNO で添字付けした
#         FP 配列）から「指定 INTNO のハンドラ番地」を引く。
#
# ASP3 はシングルプロセッサなので，FMP3 のプロセッサ毎ハンドラ表
# （_kernel_p_inh_table[prc][intid]）ではなく単一の _kernel_inh_table[intno] を使う。
# IPI（プロセッサ間割込み）関連の判定（is_dispatch_ipi_bypassed 等）は持たない。
#
# GIC レジスタは動的読み出しのため，実行中ターゲット/コアダンプへの接続が必要（要 halt）。
# ハンドラテーブルは const（.rodata）なので ELF 単体（静的）でも読める。

import gdb

# GICv2 Distributor レジスタオフセット（arch/arm64_gcc/common/gic_kernel_impl.h と一致）
_GICD_ISENABLER = 0x100   # 割込みイネーブルセット（read = 現在の許可状態）
_GICD_ISPENDR   = 0x200   # 割込みペンディングセット（read = ペンディング状態）


def _read32(addr):
    addr &= 0xFFFFFFFFFFFFFFFF
    return int(gdb.parse_and_eval("*(unsigned int *)0x%x" % addr)) & 0xFFFFFFFF


def _bit_in_reg(gicd_base, base_off, intid):
    reg = gicd_base + base_off + (intid // 32) * 4
    return bool(_read32(reg) & (1 << (intid % 32)))


def gicv2_int_enabled(gicd_base, intid):
    """GIC INTID が許可状態か（GICD_ISENABLER<n> の該当ビット）。"""
    return _bit_in_reg(gicd_base, _GICD_ISENABLER, intid)


def gicv2_int_pending(gicd_base, intid):
    """GIC INTID がペンディングか（GICD_ISPENDR<n> の該当ビット）。"""
    return _bit_in_reg(gicd_base, _GICD_ISPENDR, intid)


def inh_handler(intno):
    """指定 INTNO の割込みハンドラ番地を返す。

    arm64 共通部の _kernel_inh_table[]（INTNO で添字付けした FP 配列）を読む。
    読めなければ None。ASP3 はシングルプロセッサなのでプロセッサ index は無い。
    """
    try:
        fp = gdb.parse_and_eval("_kernel_inh_table[%d]" % int(intno))
        return int(fp) & 0xFFFFFFFFFFFFFFFF
    except gdb.error:
        return None
