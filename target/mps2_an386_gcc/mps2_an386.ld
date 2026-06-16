/*
 *  リンカスクリプト（ARM MPS2-AN386 / QEMU 用）
 *
 *  AN386（レガシー CMSDK）のメモリマップ（hw/arm/mps2.c の case FPGA_AN386）
 *  に基づく．
 *    0x00000000.. : SSRAM1（コード／ベクタ用，4MB．0x00400000 にミラー）
 *    0x20000000.. : SSRAM2&3（データ用，4MB）
 *
 *  Cortex-M4（ARMv7-M）はリセット時に VTOR=0 で 0x00000000 からベクタテーブル
 *  を読むため，ベクタ（.vector）を FLASH（=SSRAM1）先頭に配置する．
 */

MEMORY
{
    FLASH(rx)  : ORIGIN = 0x00000000, LENGTH = 4096k
    RAM(rwx)   : ORIGIN = 0x20000000, LENGTH = 4096k
}

ENTRY(_kernel_start)

SECTIONS
{
    /* TEXT セクション（ベクタテーブルを先頭に配置） */
    .text : ALIGN(4)
    {
        KEEP(*(.vector))
        *(.text*)
        *(.rodata .rodata.* .constdata .constdata.*)
        . = ALIGN(4);
    } > FLASH

    __idata_start = .;
    /* DATA セクション（RAM に配置，初期値は FLASH に置く） */
    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
    } > RAM AT>FLASH
    __idata_end = __idata_start + SIZEOF(.data);

    /* BSS セクション */
    .bss : ALIGN(4)
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM
}
