/*
 *  minimal_boot リンカスクリプト（STM32MP257F-DK）
 *
 *  ロードアドレスは Makefile の -Wl,-Ttext で与える（既定 0x88000000）．
 *  FSBL はこのアドレスにロードし，同アドレス(=_start)へ entry する．
 */
OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(_start)

SECTIONS {
	.text : {
		KEEP(*(.text.start))	/* _start を先頭に置く */
		*(.text*)
		*(.rodata*)
	}
	/DISCARD/ : {
		*(.note*)
		*(.comment)
		*(.eh_frame*)
		*(.ARM.*)
	}
}
