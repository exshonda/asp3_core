/*
 *  TOPPERS/FMP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Flexible MultiProcessor Kernel
 *
 *  Copyright (C) 2006-2020 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，以下の(1)～(4)の条件を満たす場合に限り，本ソフトウェ
 *  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
 *  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 *
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 *
 *  @(#) $Id: chip_kernel_impl.c 345 2023-04-20 12:58:48Z ertl-honda $
 */

/*
 *		カーネルのチップ依存部（ZynqMP用）
 *
 *  QEMU(xlnx-zcu102) では ELF を -kernel で渡すと EL3(Secure) で起動する
 *  （実機では FSBL からの起動に相当）．start.S が EL3 からセキュア EL1 へ
 *  ドロップする（TOPPERS_TZ_S）．
 */
#include "kernel_impl.h"

/*
 *  EL3で行う初期化処理
 */
void
chip_el3_initialize(void)
{
	volatile uint32_t scr;
	volatile uint32_t cpuectlr;

	/*
	 *  例外・割込みを下位ELで処理し，セキュア状態を維持する．
	 */
	SCR_EL3_READ(scr);
	scr &= ~(SCR_EA_BIT | SCR_FIQ_BIT | SCR_IRQ_BIT | SCR_NS_BIT);
	SCR_EL3_WRITE(scr);

	/*
	 *  SMP コヒーレンシの有効化（シングルコアでも Shareable メモリの
	 *  キャッシュ有効化に必須）
	 */
	CPUECTLR_EL1_READ(cpuectlr);
	cpuectlr |= CPUECTLR_EL1_SMPEN;
	CPUECTLR_EL1_WRITE(cpuectlr);

	/*
	 *  CPTR_EL3 のトラップを解除する（EL1 から CPACR/FP を使用可能に）．
	 */
	{
		volatile uint64_t cptr;
		__asm__ volatile("mrs %0, cptr_el3" : "=r"(cptr));
		cptr &= ~(((uint64_t)1 << 31) | ((uint64_t)1 << 30)
				  | ((uint64_t)1 << 20) | ((uint64_t)1 << 10));
		__asm__ volatile("msr cptr_el3, %0" :: "r"(cptr));
		__asm__ volatile("isb");
	}

#ifndef TOPPERS_USE_QEMU
	/*
	 *  System Timestamp Generator (Secure) の有効化と周波数設定（実機のみ）．
	 *
	 *  QEMU は本レジスタ(0xFF260000)をモデル化しておらず，Generic Timer の
	 *  カウンタは CNTFRQ 相当の周波数で常時歩進するため，QEMU では設定しない
	 *  （CNTFRQ は実行時読出しで吸収する）．
	 */
	sil_wrw_mem((void *)XIOU_SCNTRS_CNT_CNTRL_REG,
				sil_rew_mem((void *)XIOU_SCNTRS_CNT_CNTRL_REG)
											& ~XIOU_SCNTRS_CNT_CNTRL_REG_EN);

	sil_wrw_mem((void *)XIOU_SCNTRS_FREQ_REG, XIOU_SCNTRS_FREQ_HZ);

	sil_wrw_mem((void *)XIOU_SCNTRS_CNT_CNTRL_REG,
				sil_rew_mem((void *)XIOU_SCNTRS_CNT_CNTRL_REG)
											| XIOU_SCNTRS_CNT_CNTRL_REG_EN);

	/* Generic Timer の周波数を設定 */
	CNTFRQ_EL0_WRITE(XIOU_SCNTRS_FREQ_HZ);
#endif /* TOPPERS_USE_QEMU */
}

/*
 *  EL2で行う初期化処理
 *
 *  TZ_S では start.S が EL3 から EL1 へ直接ドロップするため，本処理は
 *  呼ばれない（TZ_NS 構成のための保持）．
 */
void
chip_el2_initialize(void)
{
	/*
	 *  Generic Timerの初期化
	 *  Physical Counter, Physical Timerを EL1/EL0 からアクセス可能に
	 */
	CNTHCTL_EL2_WRITE(CNTHCTL_EL1PCEN_BIT | CNTHCTL_EL1PCTEN_BIT);

	/* Virtual Counterのオフセットを0に */
	CNTVOFF_EL2_WRITE(0);

	inst_sync_barrier();
}

/*
 *  チップ依存の初期化
 *
 *  キャッシュは core_initialize() の中で（MMU 初期化後に）有効化される
 *  ため，ここでは無効化しておく．
 */
void
chip_initialize(void)
{
	dcache_disable();
	icache_disable();

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
