/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアをTOPPERSライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソ
 *  フトウェアは無保証で提供される．
 */

/*
 *    カーネルのチップ依存部（RP2350 RISC-V（Hazard3）用）
 *
 *  PolarFire SoC依存部（PLIC）をXh3irq用に置き換えたもの．
 *  高分解能タイマはMachine TimerではなくTIMER0（ターゲット依存部の
 *  target_timer.[ch]，ATT_INIで初期化）を使用するため，本ファイルでは
 *  タイマの初期化を行わない．
 */

#include "kernel_impl.h"
#include "interrupt.h"
#include <sil.h>

/*
 *  Xh3irqの初期化
 */
void
xh3irq_initialize(void)
{
	uint32_t window;

	/*
	 *  全割込みの禁止と強制ビットのクリア
	 */
	for (window = 0U; window <= ((XH3IRQ_TNUM_IRQ - 1U) / 16U); window++) {
		xh3irq_meiea_write(window);		/* ウィンドウデータ0＝全禁止 */
		xh3irq_meifa_write(window);		/* ウィンドウデータ0＝強制クリア */
	}

	/*
	 *  すべての割込みを最低優先度（0）に設定
	 */
	for (window = 0U; window <= ((XH3IRQ_TNUM_IRQ - 1U) / 4U); window++) {
		xh3irq_meipra_write(window);	/* ウィンドウデータ0 */
	}

	/*
	 *  割込み優先度マスクを全解除（内部表現0＝PREEMPT 1）に設定
	 */
	xh3irq_meicontext_write((ulong_t)1U << XH3IRQ_MEICONTEXT_PREEMPT_SHIFT);
}

/*
 *  チップ依存の初期化
 */
void
chip_initialize(void)
{
	extern void *trap_vector_table;

	/*
	 *  Machine Trap-Vector Base の設定
	 */
	riscv_write_mtvec((ulong_t)&trap_vector_table | MTVEC_MODE_VECTORD);

	/*
	 *  Xh3irqの初期化
	 */
	xh3irq_initialize();

	/*
	 *  Machine External Interrupt のみ許可（MTI/MSIは使用しない）
	 */
	riscv_clear_mie(MIE_MSIE | MIE_MTIE);
	riscv_set_mie(MIE_MEIE);

	/*
	 *  コア依存の初期化
	 */
	core_initialize();
}

/*
 *  チップ依存の終了処理
 */
void
chip_terminate(void)
{
	/*
	 *  コア依存の終了処理
	 */
	core_terminate();
}

/*
 *  割込み要求ラインの属性の設定
 */
Inline void
xh3irq_config_int(INTNO intno, ATR intatr, PRI intpri)
{
	assert(VALID_INTNO(intno));
	assert(TMIN_INTPRI <= intpri && intpri <= TMAX_INTPRI);

	/*
	 *  割込み優先度を設定
	 */
	xh3irq_set_priority(intno, INT_IPM(intpri));

	/*
	 *  割込みを許可
	 */
	if ((intatr & TA_ENAINT) != 0U) {
		xh3irq_enable_int(intno);
	}
}

/*
 *  割込み管理機能の初期化
 */
void
initialize_interrupt(void)
{
	uint_t			i;
	const INTINIB	*p_intinib;

	for (i = 0U; i < tnum_cfg_intno; i++) {
		p_intinib = &(intinib_table[i]);
		xh3irq_config_int(p_intinib->intno, p_intinib->intatr,
						  p_intinib->intpri);
	}
}
