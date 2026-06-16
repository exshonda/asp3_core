/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2024 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアをTOPPERSライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソ
 *  フトウェアは無保証で提供される．
 */

/*
 *  PolarFire SoC 実機（Discovery/Icicle Kit）用リンカスクリプト（L2-LIM）
 *
 *  L2-LIM（0x08000000，512KB）に全セクションを配置する（実機JTAG
 *  デバッグ＝lim-debug構成．HSSが電源投入時にクロック/DDR/IOを初期化済み，
 *  reset initはhartのみリセットするため周辺設定は保持される）．
 *  JTAGデバッガが直接ロードするため，.dataのROM化は行わない
 *  （VMA==LMA．TOPPERS_OMIT_DATA_INITを定義してビルドする）．
 */

OUTPUT_ARCH(riscv)
ENTRY(start)

SECTIONS {
    . = 0x08000000;

    .text :
    {
        __text = . ;
        *(.text.start)
        *(.text)
        *(.text.*)
    }
    _etext = . ;
    PROVIDE (etext = .) ;

    . = ALIGN(8);
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        *(.srodata)
        *(.srodata.*)
    }

    . = ALIGN(8);
    __idata_start = . ;
    .data ALIGN(8) : AT(__idata_start)
    {
        __data_start = . ;
        *(.data)
        *(.data.*)
        . = ALIGN(8);
        __global_pointer$ = . + 0x800;
        *(.sdata)
        *(.sdata.*)
    }
    __idata_end = __idata_start + SIZEOF(.data);
    __data_end = . ;
    PROVIDE (edata = .);

    . = ALIGN(8);
    .bss :
    {
        __bss_start = . ;
        *(.sbss)
        *(.sbss.*)
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(8);
    }
    __bss_end = .;

    _end = . ;
    PROVIDE (end = .) ;
    .comment 0 : { *(.comment) }

  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to
     the beginning of the section so we begin them at 0.  */

  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }

  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }

  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }

  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }

  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
}
