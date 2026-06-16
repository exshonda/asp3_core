/*
 *  リンカスクリプト（ARM MPS3-AN547 / QEMU 用）
 *
 *  SSE-300/AN547 のメモリマップ（hw/arm/armsse.c・hw/arm/mps2-tz.c）に基づく．
 *    0x00000000.. : ITCM（512KB．SSE-300 は ITCM を 0x0 に配置．armsse.c
 *                   line 1227-1238 「The SSE-300 has an ITCM at 0x0000_0000」）
 *    0x01000000.. : sram（2MB．an547_raminfo[]．mpc=0）
 *    0x20000000.. : DTCM（512KB．armsse.c：DTCM at 0x2000_0000）
 *    0x21000000.. : sram 2（4MB．an547_raminfo[]．mpc=-1）
 *    0x28000000.. : QSPI(ROM)（8MB．an547_raminfo[]．IS_ROM）
 *    0x60000000.. : DDR（an547_raminfo[]）
 *
 *  CPU はリセット時に init_svtor（mps3tz_an547_class_init：
 *  mmc->init_svtor = 0x00000000）から初期 SP/PC（ベクタテーブル）を読む．
 *  したがってベクタ（.vector）とコードは init_svtor が指す ITCM（0x0）の
 *  先頭に配置する．データは大容量の sram 2（0x21000000，4MB）に置く．
 *
 *  ※ AN505（mps2_an505_gcc）からの派生．AN505 は init_svtor=0x10000000 で
 *    コード用 SSRAM 先頭にベクタを置いたが，AN547 は init_svtor=0x00000000
 *    のため ITCM 先頭がブートアドレスとなる点が異なる．
 */

MEMORY
{
    FLASH(rx)  : ORIGIN = 0x00000000, LENGTH = 512k    /* ITCM（init_svtor=0） */
    RAM(rwx)   : ORIGIN = 0x21000000, LENGTH = 4096k   /* sram 2 */
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
