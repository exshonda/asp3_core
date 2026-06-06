/*
 *  RP2350用 IMAGE_DEFブロック（RISC-V EXE）
 *
 *  RP2350のブートROMは，フラッシュ先頭4KB以内に置かれたIMAGE_DEF
 *  ブロックを検索し，有効なブロックが見つかるとイメージを起動する．
 *  RISC-VイメージではアーキテクチャセレクタをRISC-Vに自動切替して
 *  起動する（RP2350 Datasheet §5.9.3）．
 *
 *  RISC-Vイメージの既定のエントリは「イメージの先頭番地」だが，
 *  本イメージはENTRY_POINT item（§5.9.3.4）でエントリ（start）と
 *  初期スタックポインタを明示する（pico-sdkのcrt0_riscv.Sと同方式）．
 *  初期SPはstart.Sが直ちにistkptへ差し替えるため安全値（RAM終端）で
 *  よい．
 *
 *  ブロックの形式は pico-sdk の
 *  src/common/boot_picobin_headers/include/boot/picobin.h および
 *  src/rp2_common/pico_crt0/embedded_start_block.inc.S を参照．
 */

	.section .picobin_block, "a"
	.p2align 2
	.global image_def_block
image_def_block:
embedded_block:
	.word	0xffffded3	/* PICOBIN_BLOCK_MARKER_START */

	/* IMAGE_TYPE item */
	.byte	0x42		/* PICOBIN_BLOCK_ITEM_1BS_IMAGE_TYPE */
	.byte	0x1		/* サイズ: 1ワード */
	.hword	0x1101		/* IMAGE_TYPE_EXE | EXE_CPU_RISCV |
				   EXE_CHIP_RP2350 */

	/* ENTRY_POINT item（エントリ番地と初期SP） */
	.byte	0x44		/* PICOBIN_BLOCK_ITEM_1BS_ENTRY_POINT */
	.byte	0x3		/* サイズ: 3ワード */
	.hword	0
	.word	start		/* エントリ番地（start.S） */
	.word	0x20080000	/* 初期SP（RAM終端．start.Sがistkptへ差替え） */

	/* LAST item */
	.byte	0xff		/* PICOBIN_BLOCK_ITEM_2BS_LAST */
	.hword	(embedded_block_end - embedded_block - 16) / 4
	.byte	0

	/* 単一ブロックループ（自分自身を指す） */
	.word	0

	.word	0xab123579	/* PICOBIN_BLOCK_MARKER_END */
embedded_block_end:
