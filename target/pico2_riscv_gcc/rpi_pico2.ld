/*
 *  RaspberryPi Pico2 RISC-V（Hazard3）用リンカスクリプト
 *
 *  ARM版（target/pico2_arm_gcc/rpi_pico2.ld）のメモリマップを
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
    /*
     *  ROM化コピー元はLOADADDRで取る．「__idata_start = .;」（=.text終端）
     *  方式は，セクションALIGNでLMAが繰り上がると4バイトずれて.dataが
     *  化ける（test_notify1のcount_variable初期値消失で発覚）
     */
    __idata_start = LOADADDR(.data);
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
