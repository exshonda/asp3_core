/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
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
 *  @(#) $Id: gic_kernel_impl.h 446 2025-06-27 07:54:07Z ertl-honda $
 */

/*
 *		カーネルの割込みコントローラ依存部（GIC用）
 *
 *  このヘッダファイルは，target_kernel_impl.h（または，そこからインク
 *  ルードされるファイル）のみからインクルードされる．他のファイルから
 *  直接インクルードしてはならない．
 */

#ifndef TOPPERS_GIC_KERNEL_IMPL_H
#define TOPPERS_GIC_KERNEL_IMPL_H

#include <sil.h>
#include "arm64.h"

/*
 *  このARM64依存部は，GICv2のみをサポートする．
 */
#if TOPPERS_GIC_VER != 2
#error This ARM64-dependent part supports only GICv2.
#endif /* TOPPERS_GIC_VER != 2 */

/*
 *  割込み番号の最大値
 */
#define TMAX_INTNO		(GIC_TNUM_INTNO - 1)

/*
 *  割込みハンドラ番号の最大値
 */
#define TMAX_INHNO		TMAX_INTNO

/*
 *  割込み番号の定義
 */
#define GIC_INTNO_SGI0		UINT_C(0)
#define GIC_INTNO_PPI0		UINT_C(16)
#define GIC_INTNO_SPI0		UINT_C(32)

#ifdef TOPPERS_TZ_S
#if GIC_PRI_LEVEL == 16
#define GIC_PRI_SHIFT		4
#define GIC_PRI_MASK		UINT_C(0x0f)
#elif GIC_PRI_LEVEL == 32
#define GIC_PRI_SHIFT		3
#define GIC_PRI_MASK		UINT_C(0x1f)
#elif GIC_PRI_LEVEL == 64
#define GIC_PRI_SHIFT		2
#define GIC_PRI_MASK		UINT_C(0x3f)
#elif GIC_PRI_LEVEL == 128
#define GIC_PRI_SHIFT		1
#define GIC_PRI_MASK		UINT_C(0x7f)
#elif GIC_PRI_LEVEL == 256
#define GIC_PRI_SHIFT		0
#define GIC_PRI_MASK		UINT_C(0xff)
#else
#error Invalid number of priority levels for GIC.
#endif /* GIC_PRI_LEVEL == 16 */
#else /* TOPPERS_TZ_S */
#if GIC_PRI_LEVEL == 16
#define GIC_PRI_SHIFT		5
#define GIC_PRI_MASK		UINT_C(0x0f)
#elif GIC_PRI_LEVEL == 32
#define GIC_PRI_SHIFT		4
#define GIC_PRI_MASK		UINT_C(0x1f)
#elif GIC_PRI_LEVEL == 64
#define GIC_PRI_SHIFT		3
#define GIC_PRI_MASK		UINT_C(0x3f)
#elif GIC_PRI_LEVEL == 128
#define GIC_PRI_SHIFT		2
#define GIC_PRI_MASK		UINT_C(0x7f)
#elif GIC_PRI_LEVEL == 256
#define GIC_PRI_SHIFT		1
#define GIC_PRI_MASK		UINT_C(0xff)
#else
#error Invalid number of priority levels for GIC.
#endif /* GIC_PRI_LEVEL == 16 */
#endif /* TOPPERS_TZ_S */

/* 外部表現への変換 */
#define EXT_IPM(pri) \
			(((PRI)((pri) >> GIC_PRI_SHIFT)) - ((GIC_PRI_LEVEL >> 1) - 1))

/* 内部表現への変換 */
#define INT_IPM(ipm) \
			(((uint_t)((ipm) + ((GIC_PRI_LEVEL >> 1) - 1))) << GIC_PRI_SHIFT)

/*
 *  GICレジスタのアドレスを定義するためのマクロ
 *
 *  GICレジスタのアドレスを，アセンブリ言語からも参照できるようにするた
 *  めのマクロ．
 */
#ifndef GIC_REG
#define GIC_REG(base, offset)	((uint32_t *)((base) + (offset)))
#endif /* GIC_REG */

/*
 *  CPUインタフェース関連の定義
 */
#define GICC_CTLR		GIC_REG(GICC_BASE, 0x00)
#define GICC_PMR		GIC_REG(GICC_BASE, 0x04)
#define GICC_BPR		GIC_REG(GICC_BASE, 0x08)
#define GICC_IAR		GIC_REG(GICC_BASE, 0x0C)
#define GICC_EOIR		GIC_REG(GICC_BASE, 0x10)
#define GICC_RPR		GIC_REG(GICC_BASE, 0x14)
#define GICC_HPPIR		GIC_REG(GICC_BASE, 0x18)


#define GICC_CTLR_ENABLE		0x01
#define GICC_CTLR_ENABLEGRP0	0x01
#define GICC_CTLR_ENABLEGRP1	0x02
#define GICC_CTLR_ACKCTL		0x04
#define GICC_CTLR_CBPR			0x10
#define GICC_CTLR_FIQEN			0x08
#define GICC_CTLR_IRQBYPDISGRP1		(1 << 8)
#define GICC_CTLR_FIQBYPDISGRP1		(1 << 7)
#define GICC_CTLR_IRQBYPDISGRP0		(1 << 6)
#define GICC_CTLR_FIQBYPDISGRP0		(1 << 5)

#define GICC_IAR_INTERRUPTID_MASK		0x03ff
#define GICC_IAR_CPUID_MASK				0x1c00


/*
 *  ディストリビュータ関連の定義
 */
#define GICD_CTLR			GIC_REG(GICD_BASE, 0x000)
#define GICD_TYPER			GIC_REG(GICD_BASE, 0x004)
#define GICD_IIDR			GIC_REG(GICD_BASE, 0x008)
#define GICD_IGROUPR(n)		GIC_REG(GICD_BASE, 0x080 + (n) * 4)
#define GICD_ISENABLER(n)	GIC_REG(GICD_BASE, 0x100 + (n) * 4)
#define GICD_SETSPI_NSR		(GICD_BASE + 0x0040)		/* Set SPI Register */
#define GICD_CLRSPI_NSR		(GICD_BASE + 0x0048)		/* Clear SPI Register */
#define GICD_SETSPI_SR		(GICD_BASE + 0x0050)		/* Set SPI, Secure Register */
#define GICD_CLRSPI_SR		(GICD_BASE + 0x0058)		/* Clear SPI, Secure Register */
#define GICD_IGROUPRn		(GICD_BASE + 0x0080)		/* 割込みセキュリティ */
#define GICD_ISENABLERn		(GICD_BASE + 0x0100)		/* 割込みイネーブルセット   */
#define GICD_ICENABLERn		(GICD_BASE + 0x0180)		/* 割込みイネーブルクリアー */
#define GICD_ISPENDRn		(GICD_BASE + 0x0200)		/* 割込みセットペンディング */
#define GICD_ISPENDR(n)		(GICD_BASE + 0x200 + (n) * 4)
#define GICD_ICPENDRn		(GICD_BASE + 0x0280)		/* 割込みクリアーペンディング */
#define GICD_ISACTIVERn		(GICD_BASE + 0x0300)		/* 割込みセットアクティブレジスター */
#define GICD_IPRIORITYRn	(GICD_BASE + 0x0400)		/* 割込み優先度レジスタ */
#define GICD_IPRIORITYR(n)	(GICD_BASE + 0x0400 + (n) * 4)
#define GICD_ITARGETSRn		(GICD_BASE + 0x0800)		/* 割込みターゲットレジスタ/CA9はSPIターゲットレジスタ */
#define GICD_ITARGETSR(n)	(GICD_BASE + 0x0800 + (n) * 4)
#define GICD_ICFGRn			(GICD_BASE + 0x0C00)		/* 割込みコンフィギュレーションレジスタ */
#define GICD_IGRPMODRn		(GICD_BASE + 0x0D00)		/* Interrupt Group Modifier Registers */
#define GICD_IGRPMODR(n)	(GICD_BASE + 0x0D00 + (n) * 4)
#define GICD_SGIR			(GICD_BASE + 0x0F00)		/* ソフトウェア割込みレジスタ  */
#define GICD_CPENDSGIRn		(GICD_BASE + 0x0F10)		/* SGI Clear-Pending Registers */
#define GICD_SPENDSGIRn		(GICD_BASE + 0x0F20)		/* SGI Set-Pending Registers */
#define GICD_IROUTERn		(GICD_BASE + 0x6000)		/* Interrupt Routing Registers */

#define GICD_PPIS		(GICD_BASE + 0x0D00)		/* PPIステータス */
#define GICD_SPIS		(GICD_BASE + 0x0D04)		/* SPIステータス */

#define GICD_CTLR_ENABLE		0x01
#define GICD_CTLR_ENABLEGRP0	0x01
#define GICD_CTLR_ENABLEGRP1	0x02
#define GICD_CTLR_ENABLEGRP1NS	(1U << 1)
#define GICD_CTLR_ENABLEGRP1S	(1U << 2)
#define GICD_CTLR_ARE_S			(1U << 4)
#ifdef TOPPERS_TZ_S
#define GICD_CTLR_ARE_NS		(1U << 5)
#else  /* !TOPPERS_TZ_S */
#define GICD_CTLR_ARE_NS		(1U << 4)
#endif /* TOPPERS_TZ_S */

#define GICD_CTLR_ARE_MASK		0x00000030

/*
 *  割込み先のプロセッサの指定
 */
#define GICD_ITARGETSRn_CPU0		0x01
#define GICD_ITARGETSRn_CPU1		0x02
#define GICD_ITARGETSRn_CPU2		0x04
#define GICD_ITARGETSRn_CPU3		0x08

#define GICD_SGIR_CPU0		0x01
#define GICD_SGIR_CPU1		0x02
#define GICD_SGIR_CPU2		0x04
#define GICD_SGIR_CPU3		0x08
#define GICD_SGIR_CPUS		0x0f

#define GICD_SGIR_CPU_OFFSET		16

/*
 *  コンフィギュレーションレジスタの設定値
 */
#define GICD_ICFGRn_EDGE		0x03	/* エッジ割込み */
#define GICD_ICFGRn_LEVEL		0x00	/* レベル割込み */
#define GICD_ICFGRn_N_N			0x00	/* N-Nモデル    */
#define GICD_ICFGRn_1_N			0x01	/* 1-Nモデル    */

/*
 *  GICでサポートしている割込み数
 */
#define GIC_TMIN_INTNO		0U

#ifndef GIC_TMAX_INTNO
#define GIC_TMAX_INTNO		255U
#endif /* GIC_TMAX_INTNO */

#ifndef GIC_TNUM_INT
#define GIC_TNUM_INT		256U
#endif /* GIC_TNUM_INT */

/*
 *  グローバル割込みの開始番号
 */
#define TMIN_GLOBAL_INTNO	32U

#ifndef TOPPERS_MACRO_ONLY

/*
 *  GIC CPU Interface 関連のドライバ
 */
/*
 *  割込み優先度マスクを設定（priは内部表現）（GICv2）
 */ 
Inline void
gicc_set_priority(uint_t pri)
{
	sil_swrw_mem(GICC_PMR, pri);
}

/*
 *  割込み優先度マスクを取得（内部表現で返す）（GICv2）
 */ 
Inline uint_t
gicc_get_priority(void)
{
	return(sil_rew_mem(GICC_PMR));
}

/*
 *  GICのプロセッサの割込み優先度のどのビットを使用するか（GICv2）
 * 
 *  この関数は，カーネルの初期化中に呼び出すことを想定しているため，
 *  GICの操作後にメモリ同期バリアを入れていない．
 */
Inline void
gicc_set_bp(int mask_bit)
{
	sil_wrw_mem((void *)GICC_BPR, mask_bit);
}

/*
 *  ソフトウェア割込みを発行（GICv2）
 * 
 *  ディストリビュータのレジスタへの1回の書き込みのみであるため，プロ
 *  セッサ間排他制御は必要ない．
 */
Inline void
gicd_raise_sgi(INTNO intno)
{
	data_sync_barrier();
	sil_swrw_mem((void *)GICD_SGIR, (0x02000000 | intno));
}



/*
 *  GICの初期化
 */
extern void gic_init(void);

/*
  *  GIC CPU Interface の終了
 */
extern void gicc_stop(void);

/*
 *  Distributor 関連のドライバ
 */

/*
 *  割込み禁止
 */
extern void gicd_disable_int(uint8_t id);

/*
 *  割込み許可
 */
extern void gicd_enable_int(uint8_t id);

/*
 *  割込みペンディングクリア
 */
extern void gicd_clear_pending(uint8_t id);

/*
 *  割込みペンディングセット
 */
extern void gicd_set_pending(uint8_t id);

/*
 *  割込み要求のチェック
 */
extern bool_t gicd_probe_pending(uint8_t id);

/*
 *  割込み設定のセット
 */
extern void gicd_config(uint8_t id, bool_t is_edge, bool_t is_1_n);

/*
 *  割込み優先度のセット
 *  内部表現で渡す． 
 */
extern void gicd_set_priority(INTNO intno, uint_t pri);

/*
 *  割込みターゲットの設定
 *  CPUはORで指定  
 */
extern void gicd_set_target(uint8_t id, uint8_t cpus);

/*
 *  ディストリビュータの初期化
 */
extern void gicd_initialize(void);

/*
 *  ディストリビュータの終了
 */
extern void gicd_terminate(void);

#endif /* TOPPERS_MACRO_ONLY */

/*
 *  割込み番号の範囲の判定
 */
#define VALID_INTNO(intno)	(0 <= (intno) && (intno) <= TMAX_INTNO)

/*
 *  割込み要求ラインのための標準的な初期化情報を生成する
 */
#define USE_INTINIB_TABLE

/*
 *  割込み要求ライン設定テーブルを生成する
 */
#define USE_INTCFG_TABLE

/*
 *  コア依存部
 */
#include "core_kernel_impl.h"

/*
 *  ターゲット非依存部に提供する関数
 */
#ifndef TOPPERS_MACRO_ONLY

/*
 *  割込み属性の設定のチェック
 */
Inline bool_t
check_intno_cfg(INTNO intno)
{
	return(intcfg_table[intno] != 0U);
}

/*
 *  割込み優先度マスクの設定
 */
Inline void
t_set_ipm(PRI intpri)
{
	gicc_set_priority(INT_IPM(intpri));
}

/*
 *  割込み優先度マスクの参照
 */
Inline PRI
t_get_ipm(void)
{
	return(EXT_IPM(gicc_get_priority()));
}

/*
 *  割込み要求禁止フラグが操作できる割込み番号の範囲の判定
 */
#ifdef GIC_SUPPORT_DISABLE_SGI
#define VALID_INTNO_DISINT(intno)	VALID_INTNO(intno)
#else /* GIC_SUPPORT_DISABLE_SGI */
#define VALID_INTNO_DISINT(intno) \
				(GIC_INTNO_PPI0 <= (intno) && (intno) <= TMAX_INTNO)
#endif /* GIC_SUPPORT_DISABLE_SGI */

/*
 *  割込み要求禁止フラグのセット
 */
Inline void
disable_int(INTNO intno)
{
	gicd_disable_int(intno);
}

/* 
 *  割込み要求禁止フラグのクリア
 */
Inline void
enable_int(INTNO intno)
{
	gicd_enable_int(intno);
}

/*
 *  割込み要求がクリアできる割込み番号の範囲の判定
 */
#define VALID_INTNO_CLRINT(intno) \
				(GIC_INTNO_PPI0 <= (intno) && (intno) <= TMAX_INTNO)

/*
 *  割込み要求がクリアできる状態か？
 */
Inline bool_t
check_intno_clear(INTNO intno)
{
	return(true);
}

/*
 *  割込み要求のクリア
 */
Inline void
clear_int(INTNO intno)
{
	gicd_clear_pending(intno);
}

/*
 *  割込みが要求できる状態か？
 */
Inline bool_t
check_intno_raise(INTNO intno)
{
	return(true);
}

/*
 *  割込みの要求
 */
Inline void
raise_int(INTNO intno)
{
	if (intno < GIC_INTNO_PPI0) {
		gicd_raise_sgi(intno);
	}
	else {
		gicd_set_pending(intno);
	}
}

/*
 *  割込み要求のチェック
 */
Inline bool_t
probe_int(INTNO intno)
{
	return(gicd_probe_pending(intno));
}

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_GIC_KERNEL_IMPL_H */
