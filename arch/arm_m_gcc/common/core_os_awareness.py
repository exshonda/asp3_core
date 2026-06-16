# Core (ARMv7-M/ARMv8-M / NVIC) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: コア（arm_m 共通）依存の低レベル知識。
#       - NVIC のレジスタ配置を知り，「指定 INTNO の割込み許可/禁止・ペンディング状態」
#         を NVIC のレジスタから読む処理を提供する。NVIC はコア内蔵（0xE000Exxx 固定）
#         なのでベースアドレス引数は不要（GIC/PLIC との違い）。
#       - 割込みハンドラテーブル _kernel_exc_tbl（例外番号＝INTNO で添字付けした
#         FP 配列。arm_m は例外とハンドラを同じ表で管理）から「指定 INTNO の
#         ハンドラ番地」を引く。
#       - レディキュービットマップのビット方向 primap_bit(pri) を提供する。
#
# arm_m の INTNO は例外番号そのもの（15=SysTick，16以降=IRQ0〜。
# arm_m.h: IRQNO_SYSTICK=15，target_syssvc.h 等: INTNO_SIO=IRQn+16）。
#
# 注意: mps2-an505 等の Secure 動作では NVIC（0xE000Exxx）は現在のセキュリティ
# 状態のバンクが見える。QEMU の gdbserver 経由ではアクセスできない場合があり，
# その場合は呼び出し側（intr コマンド）が "?" 表示に劣化する。
#
# NVIC レジスタは動的読み出しのため，実行中ターゲット/コアダンプへの接続が必要（要 halt）。
# ハンドラテーブルは const（.rodata）なので ELF 単体（静的）でも読める。

import gdb

# NVIC / SysTick レジスタ（arch/arm_m_gcc/common/arm_m.h と一致）
_NVIC_ISER0 = 0xE000E100   # 割込みイネーブルセット（read = 現在の許可状態）
_NVIC_ISPR0 = 0xE000E200   # 割込みペンディングセット（read = ペンディング状態）
_SYST_CSR   = 0xE000E010   # SysTick Control and Status（bit1 = TICKINT）
_NVIC_ICSR  = 0xE000ED04   # Interrupt Control State（bit26 = PENDSTSET）

# 例外番号とIRQ番号のオフセット（INTNO 16以降が IRQ0〜）
_IRQNO_OFFSET   = 16
_IRQNO_SYSTICK  = 15


def _read32(addr):
    addr &= 0xFFFFFFFF
    return int(gdb.parse_and_eval("*(unsigned int *)0x%x" % addr)) & 0xFFFFFFFF


def _irq_bit(base, irqno):
    reg = base + (irqno // 32) * 4
    return bool(_read32(reg) & (1 << (irqno % 32)))


def int_enabled(intno):
    """指定 INTNO の割込み許可状態（True=許可 / False=禁止）。

    IRQ（INTNO>=16）は NVIC_ISER，SysTick（INTNO=15）は SYST_CSR.TICKINT を見る。
    それ以外（CPU例外域）は対象外として ValueError（呼び出し側で "?" 表示）。
    """
    intno = int(intno)
    if intno >= _IRQNO_OFFSET:
        return _irq_bit(_NVIC_ISER0, intno - _IRQNO_OFFSET)
    if intno == _IRQNO_SYSTICK:
        return bool(_read32(_SYST_CSR) & 0x02)
    raise ValueError("not an interrupt: INTNO %d" % intno)


def int_pending(intno):
    """指定 INTNO のペンディング状態（True=ペンディング）。

    IRQ（INTNO>=16）は NVIC_ISPR，SysTick（INTNO=15）は ICSR.PENDSTSET を見る。
    """
    intno = int(intno)
    if intno >= _IRQNO_OFFSET:
        return _irq_bit(_NVIC_ISPR0, intno - _IRQNO_OFFSET)
    if intno == _IRQNO_SYSTICK:
        return bool(_read32(_NVIC_ICSR) & (1 << 26))
    raise ValueError("not an interrupt: INTNO %d" % intno)


def inh_handler(intno):
    """指定 INTNO の割込みハンドラ番地を返す。

    arm_m 共通部の _kernel_exc_tbl[]（例外番号＝INTNO で添字付けした FP 配列。
    0〜14 が CPU 例外，15 以降が割込み）を読む。読めなければ None。
    Thumb 命令セットのため最下位ビットが 1 になっている（シンボル解決時は
    gdb の info symbol が吸収する）。
    """
    try:
        fp = gdb.parse_and_eval("_kernel_exc_tbl[%d]" % int(intno))
        return int(fp) & 0xFFFFFFFF
    except gdb.error:
        return None


def primap_bit(pri):
    """内部優先度 pri のレディキュービットマップのビット値。

    arm_m は PRIMAP_BIT を上書きしない（kernel/task.c 既定の 1<<pri）。
    """
    return 1 << int(pri)
