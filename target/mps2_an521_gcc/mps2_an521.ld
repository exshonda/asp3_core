/*
 *  リンカスクリプト（ARM MPS2-AN521 / QEMU 用）
 *
 *  SSE-200 のメモリマップ（hw/arm/armsse.c）に基づく．
 *    0x10000000.. : コード用 SSRAM（0x00000000 のエイリアス，4MB）
 *    0x38000000.. : ボード SSRAM（0x28000000 のエイリアス，4MB）
 *
 *  CPU はリセット時に init_svtor（=0x10000000）からベクタテーブルを読む
 *  ため，ベクタ（.vector）を FLASH 先頭に配置する．
 */

MEMORY
{
    FLASH(rx)  : ORIGIN = 0x10000000, LENGTH = 4096k
    RAM(rwx)   : ORIGIN = 0x38000000, LENGTH = 4096k
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
