/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2006-2025 by Embedded and Real-Time Systems Laboratory
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
 *  @(#) $Id: core_kernel_impl.c 458 2026-05-06 06:01:33Z ertl-honda $
 */

/*
 *		カーネルのコア依存部（ARM64用）
 */

#include "kernel_impl.h"
#include "check.h"
#include "task.h"
#include "arm64.h"

/*
 *  例外ネストカウント
 */
uint32_t	excpt_nest_count;


/*
 *  パフォーマンスモニタによる性能評価
 */
#ifdef USE_ARM64_PMCNT

/*
 *  パフォーマンスモニタの初期化
 */
Inline void
arm64_pmcnt_initialize(void)
{
	volatile uint64_t	reg;

	/*
	 *  全カウンターの有効化
	 */
	PMCR_EL0_READ(reg);
	reg |= PMCR_LC_BIT | PMCR_E_BIT;
	PMCR_EL0_WRITE(reg);

	INST_SYNC_BARRIER();
	DATA_SYNC_BARRIER();

	/*
	 *  パフォーマンスカウンタの有効化
	 */
	PMCNTENSET_EL0_WRITE(PMCNTENSET_C_BIT);

	INST_SYNC_BARRIER();
	DATA_SYNC_BARRIER();

	/*
	 *  全カウンターのリセット
	 */
	PMCR_EL0_WRITE(reg | PMCR_C_BIT);

	INST_SYNC_BARRIER();
	DATA_SYNC_BARRIER();
}

#endif /* USE_ARM64_PMCNT */

/*
 *  コア依存の初期化
 */
void
core_initialize(void)
{
	uint32_t tmp;

	/*
	 *  カーネル起動時は非タスクコンテキストとして動作させるために，例外
	 *  のネスト回数を1に初期化する．
	 */ 
	excpt_nest_count = 1U;

	/*
	 *  例外ベクタテーブルをVECTOR BASE ADDRESS レジスタに設定する
	 */
	extern void *vector_table;
	VBAR_EL1_WRITE((uint64_t)&vector_table);

	/*
	 *  アラインメントチェックの設定
	 */
#ifdef ENABLE_ALIGNMENT_CHECK
	SCTLR_EL1_READ(tmp);
	SCTLR_EL1_WRITE(tmp | SCTLR_SA_BIT | SCTLR_A_BIT);
#else /* ENABLE_ALIGNMENT_CHECK */
	SCTLR_EL1_READ(tmp);
	SCTLR_EL1_WRITE(tmp & ~(SCTLR_SA_BIT | SCTLR_A_BIT));
#endif /* ENABLE_ALIGNMENT_CHECK */
	inst_sync_barrier();

	/*
	 *  キャッシュを無効にする前にフラッシュする
	 */
	cache_flush();
    
	/*
	 *  キャッシュを無効に
	 */
	cache_disable();

#if defined(TOPPERS_CORTEX_A53) || defined(TOPPERS_CORTEX_A57) || defined(TOPPERS_CORTEX_A35)
	/*
	 *  SMPモードに設定（シングルコアでも，Shareable属性のメモリ領域を
	 *  キャッシュ有効にするために必要）
	 */
	enable_smp();
#endif /* defined(TOPPERS_CORTEX_A53) || defined(TOPPERS_CORTEX_A57) || defined(TOPPERS_CORTEX_A35) */

	/*
	 *  MMUを有効に
	 */
	mmu_init();

	/*
	 *  キャッシュを有効に
	 */
	cache_enable();

	/*
	 *  GIC Distributorの初期化
	 */
	gicd_initialize();

	/*
	 *  GIC CPUインタフェース等の初期化
	 */
	gic_init();

	/*
	 *  パフォーマンスモニタの初期化
	 */
#ifdef USE_ARM64_PMCNT
	arm64_pmcnt_initialize();
#endif /* USE_ARM64_PMCNT */
}

/*
 *  コア依存の終了処理
 */
void
core_terminate(void)
{
	/*
	 *  GICのCPUインタフェースを停止
	 */
	gicc_stop();

	/*
	 *  GIC Distributorを停止
	 */
	gicd_terminate();
}

/*
 *  CPU例外の発生状況のログ出力
 */
#ifndef OMIT_XLOG_SYS

/*
 *  CPU例外ハンドラの中から，CPU例外情報ポインタ（p_excinf）を引数とし
 *  て呼び出すことで，CPU例外の発生状況をシステムログに出力する．
 */
void
xlog_sys(void *p_excinf)
{
	T_EXCINF *p_exc_frame = (T_EXCINF*)p_excinf;

	syslog_2(LOG_EMERG, "nest_count = %d, intpri = %d.",
			 (int)p_exc_frame->nest_count, (int)p_exc_frame->intpri);
	syslog_2(LOG_EMERG, "pc = 0x%016lx, pstate = 0x%08x",
			 p_exc_frame->pc, (uint32_t)p_exc_frame->pstate);
	syslog_2(LOG_EMERG, "x0 = 0x%016lx, x1 = 0x%016lx",
			 p_exc_frame->x0, p_exc_frame->x1);
	syslog_2(LOG_EMERG, "x2 = 0x%016lx, x3 = 0x%016lx",
			 p_exc_frame->x2, p_exc_frame->x3);
	syslog_2(LOG_EMERG, "x4 = 0x%016lx, x5 = 0x%016lx",
			 p_exc_frame->x4, p_exc_frame->x5);
	syslog_2(LOG_EMERG, "x6 = 0x%016lx, x7 = 0x%016lx",
			 p_exc_frame->x6, p_exc_frame->x7);
	syslog_2(LOG_EMERG, "x8 = 0x%016lx, x9 = 0x%016lx",
			 p_exc_frame->x8, p_exc_frame->x9);
	syslog_2(LOG_EMERG, "x10 = 0x%016lx, x11 = 0x%016lx",
			 p_exc_frame->x10, p_exc_frame->x11);
	syslog_2(LOG_EMERG, "x12 = 0x%016lx, x13 = 0x%016lx",
			 p_exc_frame->x12, p_exc_frame->x13);
	syslog_2(LOG_EMERG, "x14 = 0x%016lx, x15 = 0x%016lx",
			 p_exc_frame->x14, p_exc_frame->x15);
	syslog_2(LOG_EMERG, "x16 = 0x%016lx, x17 = 0x%016lx",
			 p_exc_frame->x16, p_exc_frame->x17);
	syslog_2(LOG_EMERG, "x18 = 0x%016lx, x19 = 0x%016lx",
			 p_exc_frame->x18, p_exc_frame->x19);
	syslog_2(LOG_EMERG, "x20 = 0x%016lx, x21 = 0x%016lx",
			 p_exc_frame->x20, p_exc_frame->x21);
	syslog_2(LOG_EMERG, "x22 = 0x%016lx, x23 = 0x%016lx",
			 p_exc_frame->x22, p_exc_frame->x23);
	syslog_2(LOG_EMERG, "x24 = 0x%016lx, x25 = 0x%016lx",
			 p_exc_frame->x24, p_exc_frame->x25);
	syslog_2(LOG_EMERG, "x26 = 0x%016lx, x27 = 0x%016lx",
			 p_exc_frame->x26, p_exc_frame->x27);
	syslog_2(LOG_EMERG, "x28 = 0x%016lx, x29 = 0x%016lx",
			 p_exc_frame->x28, p_exc_frame->x29);
	syslog_2(LOG_EMERG, "x30(lr) = 0x%016lx, sp = 0x%016lx",
			 p_exc_frame->x30, p_exc_frame->sp);
}
#endif /* OMIT_XLOG_SYS */

#ifndef OMIT_DEFAULT_INT_HANDLER

/*
 *  未定義の割込みが入った場合の処理
 */
void
default_int_handler(uint32_t intno){
	syslog_1(LOG_EMERG, "Unregistered Interrupt occurs at %d.", intno);
	ext_ker();
}

#endif /* OMIT_DEFAULT_INT_HANDLER */

#ifndef OMIT_DEFAULT_EXC_HANDLER

void
default_exc_handler(void *p_excinf, EXCNO excno)
{
	volatile uint32_t	esr_el1;

#ifdef OMIT_XLOG_SYS
	syslog_1(LOG_EMERG, "\nUnregistered exception %d occurs.", excno);
#else /* OMIT_XLOG_SYS */

	switch (excno) {
	  case EXCNO_CUR_SP0_SYNC:
		syslog_0(LOG_EMERG, "Synchronous exception with current EL with SP0");
		break;
	  case EXCNO_CUR_SP0_IRQ:
		syslog_0(LOG_EMERG, "IRQ exception with current EL with SP0");
		break;
	  case EXCNO_CUR_SP0_FIQ:
		syslog_0(LOG_EMERG, "FIQ exception with current EL with SP0");
		break;
	  case EXCNO_CUR_SP0_SERR:
		syslog_0(LOG_EMERG, "SError exception with current EL with SP0");
		break;
	  case EXCNO_CUR_SPX_SYNC:
		syslog_0(LOG_EMERG, "Synchronous exception with current EL with SPx");
		break;
	  case EXCNO_CUR_SPX_IRQ:
		syslog_0(LOG_EMERG, "IRQ exception with current EL with SPx");
		break;
	  case EXCNO_CUR_SPX_FIQ:
		syslog_0(LOG_EMERG, "FIQ exception with current EL with SPx");
		break;
	  case EXCNO_CUR_SPX_SERR:
		syslog_0(LOG_EMERG, "SError exception with current EL with SPx");
		break;
	  case EXCNO_L64_SYNC:
		syslog_0(LOG_EMERG, "Synchronous exception with lower EL using AArch64");
		break;
	  case EXCNO_L64_IRQ:
		syslog_0(LOG_EMERG, "IRQ exception with lower EL using AArch64");
		break;
	  case EXCNO_L64_FIQ:
		syslog_0(LOG_EMERG, "FIQ exception with lower EL using AArch64");
		break;
	  case EXCNO_L64_SERR:
		syslog_0(LOG_EMERG, "SError exception with lower EL using AArch64");
		break;
	  case EXCNO_L32_SYNC:
		syslog_0(LOG_EMERG, "Synchronous exception with lower EL using AArch32");
		break;
	  case EXCNO_L32_IRQ:
		syslog_0(LOG_EMERG, "IRQ exception with lower EL using AArch32");
		break;
	  case EXCNO_L32_FIQ:
		syslog_0(LOG_EMERG, "FIQ exception with lower EL using AArch32");
		break;
	  case EXCNO_L32_SERR:
		syslog_0(LOG_EMERG, "SError exception with lower EL using AArch32");
		break;
	}

	Asm("mrs %0, esr_el1":"=r"(esr_el1));
	syslog_1(LOG_EMERG, "ESR_EL1.EC  = 0x%06x", esr_el1 >> 26);
	syslog_1(LOG_EMERG, "ESR_EL1.IL  = 0x%x", (esr_el1 >> 25) & 0x01);
	syslog_1(LOG_EMERG, "ESR_EL1.ISS = 0x%x", esr_el1 & 0x1ffffff);

	xlog_sys(p_excinf);
#endif /* OMIT_XLOG_SYS */
	ext_ker();
}
#endif /* OMIT_DEFAULT_EXC_HANDLER */
