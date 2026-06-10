/*
 *  RP2350用 IMAGE_DEFブロック
 *
 *  RP2350のブートROMは，フラッシュ先頭4KB以内に置かれたIMAGE_DEF
 *  ブロックを検索し，有効なブロックが見つかるとイメージを起動する．
 *  RP2040と異なりboot stage2は不要である．
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
	.hword	0x1021		/* IMAGE_TYPE_EXE | EXE_SECURITY_S |
				   EXE_CPU_ARM | EXE_CHIP_RP2350 */

	/* LAST item */
	.byte	0xff		/* PICOBIN_BLOCK_ITEM_2BS_LAST */
	.hword	(embedded_block_end - embedded_block - 16) / 4
	.byte	0

	/* 単一ブロックループ（自分自身を指す） */
	.word	0

	.word	0xab123579	/* PICOBIN_BLOCK_MARKER_END */
embedded_block_end:
