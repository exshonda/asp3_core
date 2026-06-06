/*
 *  RaspberryPi Pico2 RISC-V（Hazard3）用リンカスクリプト
 *
 *  ARM版（target/raspberrypi_pico2_gcc/rpi_pico2.ld）のメモリマップを
 *  踏襲し，RISC-V流に変更したもの：
 *    - 先頭はベクタテーブルではなくエントリコード（.text.start＝start.S）
 *      ＋IMAGE_DEFブロック（エントリはIMAGE_DEFのENTRY_POINT itemで指定）
 *    - small data（.sdata/.sbss）と __global_pointer$ を配置
 *  .dataはFLASHからRAMへstart.Sがコピーする（__idata_*シンボル）．
 */

OUTPUT_ARCH(riscv)

MEMORY
{
    FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 4096k
    RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 512k
}

ENTRY(start)

SECTIONS
{
     /* MAIN TEXT SECTION */
    .text : ALIGN(4)
    {
        KEEP(*(.text.start))
        KEEP(*(.picobin_block))
        *(.text*)
        *(.rodata .rodata.* .srodata .srodata.*)
        KEEP(*(.empty.*))
        . = ALIGN(4);
    } > FLASH

    __idata_start = .;
    /* Main DATA section (SRAM) */
    .data : ALIGN(8)
    {
        __data_start = .;
        *(.data*)
        . = ALIGN(8);
        __global_pointer$ = . + 0x800;
        *(.sdata .sdata.*)
        . = ALIGN(4);
    } > RAM AT>FLASH
    __idata_end = __idata_start + SIZEOF(.data);

    /* MAIN BSS SECTION */
    .bss : ALIGN(4)
    {
        __bss_start = .;
        *(.sbss .sbss.*)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM
}
