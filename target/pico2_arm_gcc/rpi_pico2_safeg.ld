/*
 *  リンカスクリプト（Raspberry Pi Pico2 / RP2350, Secure・SafeG-M 構成）
 *
 *  RP2350 メモリマップ:
 *    0x10000000.. : XIP QSPI フラッシュ（4MB, Pico2）
 *    0x20000000.. : SRAM（520KB のうち先頭 512KB を使用）
 *
 *  SafeG-M の Secure/NS 分割（an505 mps2_an505_safeg.ld を踏襲し RP2350 アドレスへ移植）:
 *    Secure code : 0x10000000 .. 0x101FFDFF (約 2MB, 先頭 4KB 内に picobin IMAGE_DEF)
 *    NSC veneer  : 0x101FFE00 .. 0x101FFFFF (512B, .gnu.sgstubs)
 *    NS  code    : 0x10200000.. (TOPPERS_NS_VTOR, NS 側 ld が使用)
 *    Secure RAM  : 0x20000000 .. 0x2003FFFF (256KB)
 *    NS  RAM     : 0x20040000.. (NS 側 ld が使用)
 *
 *  RP2350 ブートROM は先頭 4KB 内の picobin_block(IMAGE_DEF) を要求するため
 *  KEEP(*(.picobin_block)) を .text 先頭付近に維持する。
 *  共通スタートアップ start.S が参照するシンボル
 *  （__idata_start/__idata_end/__data_start/__bss_start/__bss_end）を定義する。
 */

MEMORY
{
    FLASH    (rx)  : ORIGIN = 0x10000000, LENGTH = 0x1FFE00  /* Secure code (~2MB) */
    SG_veneer(rx)  : ORIGIN = 0x101FFE00, LENGTH = 0x200     /* NSC veneer 512B    */
    RAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 0x40000   /* Secure RAM 256KB   */
}

ENTRY(_kernel_start)

SECTIONS
{
    .text : ALIGN(4)
    {
        KEEP(*(.vector))
        KEEP(*(.picobin_block))
        *(.text*)
        *(.rodata .rodata.* .constdata .constdata.*)
        KEEP(*(.empty.*))
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : ALIGN(4) { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    /* TrustZone Secure Gateway veneers を NSC 領域に配置 */
    .gnu.sgstubs : ALIGN(32)
    {
        . = ALIGN(32);
        *(.gnu.sgstubs*)
        . = ALIGN(32);
    } > SG_veneer

    __idata_start = LOADADDR(.data);
    .data : ALIGN(4)
    {
        __data_start = .;
        *(vtable)
        *(.data*)
        . = ALIGN(4);
    } > RAM AT> FLASH
    __idata_end = __idata_start + SIZEOF(.data);

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
        PROVIDE(end = .);
    } > RAM
}
