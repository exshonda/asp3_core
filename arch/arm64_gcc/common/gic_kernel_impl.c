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
 *  @(#) $Id: gic_kernel_impl.c 465 2026-06-03 08:51:58Z ertl-honda $
 */

/*
 *		カーネルの割込みコントローラ依存部（GIC用）
 * 
 * 		初期化処理・終了処理のみで呼び出される関数には，最後にメモリ・命令同期は入れていない．
 */

#include "kernel_impl.h"
#include "interrupt.h"
#include <sil.h>
#include "arm64.h"

/* 前方参照 */
static void gicc_init(void);
static void gic_sgi_ppi_init(void);

/*
 *  GICv2,3,4 で共通の関数
 */
/*
 *  Distributor 終了
 */
void
gicd_terminate(void)
{
	uint32_t ctlr_val;

	/* 現在値を確認 */
	ctlr_val = sil_rew_mem((void *)GICD_CTLR);

	/* ARE ビットは保持し、残りはクリア */
	ctlr_val &= GICD_CTLR_ARE_MASK;
	
	/* Distributor を無効に */
	sil_wrw_mem((void *)(GICD_CTLR), ctlr_val);
}

/*
 *  Distributor 初期化共通処理
 */
static void
gicd_initialize_common(void)
{
	int32_t i;
	uint32_t ctlr_val;

	/* 現在値を確認 */
	ctlr_val = sil_rew_mem((void *)GICD_CTLR);

	/* ARE ビットは保持し、残りはクリア */
	ctlr_val &= GICD_CTLR_ARE_MASK;

	/* Distributor を無効に */
	sil_wrw_mem((void *)(GICD_CTLR), ctlr_val);

#ifdef TOPPERS_TZ_S
	/* 割込みを全てグループ1(IRQ)に */
	for(i = TMIN_GLOBAL_INTNO/32; i < GIC_TNUM_INT/32; i++){
		sil_wrw_mem((void *)(GICD_IGROUPRn + (uintptr_t)(4 * i)), 0xffffffff);
		sil_wrw_mem((void *)(GICD_IGRPMODRn + (uintptr_t)(4 * i)), 0x00000000);
	}
#endif /* TOPPERS_TZ_S */

	/* 割込みを全て禁止 */
	for(i = TMIN_GLOBAL_INTNO/32; i < GIC_TNUM_INT/32; i++){
		sil_wrw_mem((void *)(GICD_ICENABLERn + (uintptr_t)(4 * i)), 0xffffffff);
	}

	/* ペンディングをクリア */
	for(i = TMIN_GLOBAL_INTNO/32; i < GIC_TNUM_INT/32; i++){
		sil_wrw_mem((void *)(GICD_ICPENDRn + (uintptr_t)(4 * i)), 0xffffffff);
	}

	/*
	 *  残留するアクティブ／ペンディング状態の正規化（リセット無し再起動対策）
	 *
	 *  リセットを介さずにカーネルを再起動した場合（JTAGによる再ロード等）に，
	 *  前回実行時に完了せず残ったアクティブ割込みや，ハルト中に発生したペン
	 *  ディング割込みがディストリビュータに残存することがある．アクティブが
	 *  残るとその実行優先度により以降の割込みがマスクされる．通常のリセット
	 *  起動時はいずれも存在しないため，本処理は実質的な副作用を持たない．
	 *
	 *  上のペンディングクリアはグローバル割込み（SPI，番号32以上）のみを対象
	 *  とするため，ここでは (1) アクティブ状態を banked 割込み（SGI 0-15／
	 *  PPI 16-31，セキュア物理タイマ INTID 29 を含む）も含めて全割込みでクリ
	 *  アし，(2) banked 割込みのペンディングもクリアする．
	 */
	for(i = 0; i < GIC_TNUM_INT/32; i++){
		sil_wrw_mem((void *)(GICD_ICACTIVERn + (uintptr_t)(4 * i)), 0xffffffff);
	}
	/* banked 割込み（SGI/PPI, 0-31）のペンディングをクリア */
	sil_wrw_mem((void *)(GICD_ICPENDRn), 0xffffffff);
	sil_wrw_mem((void *)(GICD_CPENDSGIRn + 0), 0xffffffff);
	sil_wrw_mem((void *)(GICD_CPENDSGIRn + 4), 0xffffffff);
	sil_wrw_mem((void *)(GICD_CPENDSGIRn + 8), 0xffffffff);
	sil_wrw_mem((void *)(GICD_CPENDSGIRn + 12), 0xffffffff);

	/* 優先度最低に設定  */
	for(i = TMIN_GLOBAL_INTNO/4; i < GIC_TNUM_INT/4; i++){
		sil_wrw_mem((void *)(GICD_IPRIORITYRn + (uintptr_t)(4 * i)), 0xffffffff);
	}

	/* ターゲット初期化（全てCPU0へ） */
	for(i = TMIN_GLOBAL_INTNO/4; i < GIC_TNUM_INT/4; i++){
		sil_wrw_mem((void *)(GICD_ITARGETSRn + (uintptr_t)(4 * i)), 0x01010101);
	}

	/* モード初期化(1-N Level) */
	for(i = TMIN_GLOBAL_INTNO/16; i < GIC_TNUM_INT/16; i++){
		sil_wrw_mem((void *)(GICD_ICFGRn + (uintptr_t)(4 * i)), 0x55555555);
	}
}

/*
 *  GIC初期化（GICv2）
 */
void
gic_init(void)
{
	/*
	 *  SGIとPPIの 初期化
	 */
	gic_sgi_ppi_init();

	/*
	 *  GIC CPUインタフェース初期化
	 */
	gicc_init();
}

/*
 *  CPU Interface の初期化（GICv2）
 */
static void
gicc_init(void)
{
	/* CPUインタフェースを無効に */
	sil_wrw_mem((void *)GICC_CTLR, 0);

	/* 最低優先度に設定 */
	gicc_set_priority(INT_IPM(TIPM_ENAALL));

	/* 割込み優先度の全ビット有効に */
	gicc_set_bp(0);

	/* ペンディングしている可能性があるので，EOI によりクリア */
	sil_wrw_mem((void *)GICC_EOIR, sil_rew_mem((void *)GICC_IAR));

	/* CPUインタフェースを有効に */
#ifdef TOPPERS_TZ_S
#ifdef GIC_NO_FIQ_IN_SECURE
	/*
	 *  セキュア(Group0)割込みを FIQ ではなく IRQ で配送するため，FIQEN を立てず
	 *  ENABLEGRP0 のみとする．FIQEN を立てるとハンドラが GIC ack 前に FIQ を再許可して
	 *  同一割込みが暴走再入する（カーネルの CPU ロックは IRQ マスクモデルのため）．
	 *  既定（マクロ未定義）の挙動は従来どおり（FIQEN を立てる）で他ターゲットに影響しない．
	 */
	sil_wrw_mem((void *)GICC_CTLR, GICC_CTLR_ENABLEGRP0);
#else  /* !GIC_NO_FIQ_IN_SECURE */
	sil_wrw_mem((void *)GICC_CTLR, (GICC_CTLR_FIQEN|GICC_CTLR_ENABLEGRP0));
#endif /* GIC_NO_FIQ_IN_SECURE */
#else  /* !TOPPERS_TZ_S */
	sil_wrw_mem((void *)GICC_CTLR, GICC_CTLR_ENABLE);
#endif /* TOPPERS_TZ_S */

	/*
	 *  残留ペンディング割込みのドレイン（リセット無し再起動対策）
	 *
	 *  CPUインタフェース有効化後に IAR で ack→EOIR で完了，を spurious(1023)
	 *  が返るまで繰り返し，前回実行から残ったペンディング割込み（ハルト中に
	 *  発生したタイマ割込み等）を排出して CPUインタフェースの実行優先度を
	 *  アイドルへ戻す．有効化前の IAR は CTLR=0 では機能しないため，ここで
	 *  改めて行う．通常のリセット起動時は即座に spurious が返り無害．
	 *  （ディストリビュータ側の残留 active/pending クリアは gicd_initialize_common）
	 */
	{
		uint_t		i;
		uint32_t	iar;

		for (i = 0; i < 64U; i++) {
			iar = sil_rew_mem((void *)GICC_IAR);
			if ((iar & 0x03ffU) == 0x03ffU) {
				break;					/* spurious(1023)＝ペンディング無し */
			}
			sil_wrw_mem((void *)GICC_EOIR, iar);
		}
	}
}

/*
 *  CPU Interface の終了（GICv2）
 */
void
gicc_stop(void)
{
	sil_wrw_mem((void *)(GICC_CTLR), 0);
}

/*
 *  Distoributor 関連
 */

/*
 *  Distributor 初期化（GICv2）
 * 
 *  この関数は，他のプロセッサが実行を開始する前に，マスタプロセッサの
 *  みから呼び出されるため，プロセッサ間排他制御は必要ない．
 */
void
gicd_initialize(void)
{
	/*
	 *  共通処理を呼び出す
	 */
	gicd_initialize_common();

	/* Distibutor を有効に */
	sil_wrw_mem((void *)(GICD_CTLR), GICD_CTLR_ENABLE);
}

/*
 *  割込み禁止（GICv2）
 * 
 *  ディストリビュータのレジスタへの1回の書き込みのみであるため，プロ
 *  セッサ間排他制御は必要ない．
 */
void
gicd_disable_int(uint8_t id)
{
	uintptr_t offset_addr = (id / 32) * 4;
	uint16_t offset_bit   = id % 32;

	sil_swrw_mem((void *)(GICD_ICENABLERn + offset_addr), (1 << offset_bit));
}

/*
 *  割込み許可（GICv2）
 * 
 *  ディストリビュータのレジスタへの1回の書き込みのみであるため，プロ
 *  セッサ間排他制御は必要ない．  
 */
void
gicd_enable_int(uint8_t id)
{
	uintptr_t offset_addr = (id / 32) * 4;
	uint16_t offset_bit  = id % 32;

	sil_swrw_mem((void *)(GICD_ISENABLERn + offset_addr), (1 << offset_bit));
}

/*
 *  割込みペンディングクリア（GICv2）
 * 
 *  ディストリビュータのレジスタへの1回の書き込みのみであるため，プロ
 *  セッサ間排他制御は必要ない．  
 */
void
gicd_clear_pending(uint8_t id)
{
	uintptr_t offset_addr = (id / 32) * 4;
	uint16_t offset_bit  = id % 32;

	sil_swrw_mem((void *)(GICD_ICPENDRn + offset_addr), (1 << offset_bit));
}

/*
 *  割込みペンディングセット（GICv2）
 * 
 *  ディストリビュータのレジスタへの1回の書き込みのみであるため，プロ
 *  セッサ間排他制御は必要ない．  
 */
void
gicd_set_pending(uint8_t id)
{
	uintptr_t offset_addr = (id / 32) * 4;
	uint16_t offset_bit  = id % 32;

	sil_swrw_mem((void *)(GICD_ISPENDRn + offset_addr), (1 << offset_bit));
}

/*
 *  割込み要求のチェック（GICv2）
 */
bool_t
gicd_probe_pending(uint8_t id)
{
	uintptr_t offset_addr = (id / 32) * 4;
	uint16_t offset_bit  = id % 32;
	uint32_t state;

	state = sil_rew_mem((void *)(GICD_ISPENDRn + offset_addr));

	return ((state & (1 << offset_bit)) == (1 << offset_bit));
}

/*
 *  割込みコンフィギュレーション設定（GICv2）
 * 
 *  この関数は，プロセッサ間排他制御を行った状態で呼び出さなければなら
 *  ない．
 */
void
gicd_config(uint8_t id,  bool_t is_edge, bool_t is_1_n)
{
	uintptr_t offset_addr;
	uint16_t offset_bit;
	uint32_t cfgr_reg_val;
	uint8_t  config;
	SIL_PRE_LOC;

	if (is_edge) {
		config = GICD_ICFGRn_EDGE;
	}
	else {
		config = GICD_ICFGRn_LEVEL;
	}

	if (is_1_n) {
		config |= GICD_ICFGRn_1_N;
	}
	else {
		config |= GICD_ICFGRn_N_N;
	}

	offset_addr = (id / 16) * 4;
	offset_bit  = (id % 16) * 2;

	SIL_LOC_INT();

	cfgr_reg_val  = sil_rew_mem((void *)(GICD_ICFGRn + offset_addr));
	cfgr_reg_val &= ~(0x03U << offset_bit);
	cfgr_reg_val |= (0x03U & config) << offset_bit;
	sil_wrw_mem((void *)(GICD_ICFGRn + offset_addr), cfgr_reg_val);

#ifdef TOPPERS_TZ_S
	offset_addr = (id / 32) * 4;
	offset_bit  = id % 32;
	sil_wrw_mem((void *)(GICD_IGROUPRn + offset_addr),
				sil_rew_mem((void *)(GICD_IGROUPRn+ offset_addr)) & ~(1 << offset_bit));
#endif /* TOPPERS_TZ_S */

	SIL_UNL_INT();
}

/*
 *  割込み優先度のセット（GICv2）
 *  内部表現で渡す．
 * 
 *  この関数は，プロセッサ間排他制御を行った状態で呼び出さなければなら
 *  ない．  
 */
void
gicd_set_priority(INTNO intno, uint_t pri)
{
	uintptr_t offset_addr = (intno / 4) * 4;
	uint16_t shift  = ((intno % 4) * 8);
	uint32_t pr_reg_val;
	SIL_PRE_LOC;

	SIL_LOC_INT();

	pr_reg_val  = sil_rew_mem((void *)(GICD_IPRIORITYRn + offset_addr));
	pr_reg_val &= ~(0xffU << shift);
	pr_reg_val |= (pri << shift);
	sil_wrw_mem((void *)(GICD_IPRIORITYRn + offset_addr), pr_reg_val);

	SIL_UNL_INT();
}

/*
 *  SGIとPPIの 初期化（GICv2）
 */
static void
gic_sgi_ppi_init(void)
{
	int32_t i;

	/* 割込みを全てグループ1(IRQ)に */
	sil_wrw_mem((void *)(GICD_IGROUPRn + (uintptr_t)(4 * 0)), 0xffffffff);

	/* 割込みを全て禁止 */
	sil_wrw_mem((void *)(GICD_ICENABLERn + (uintptr_t)(4 * 0)), 0xffffffff);

	/* ペンディングをクリア */
	sil_wrw_mem((void *)(GICD_ICPENDRn + (uintptr_t)(4 * 0)), 0xffff0000);

	/* 優先度最低に設定  */
	for(i = 0; i < TMIN_GLOBAL_INTNO/4; i++){
		sil_wrw_mem((void *)(GICD_IPRIORITYRn + (uintptr_t)(4 * i)), 0xffffffff);
	}

	/* モード初期化(1-N Level) */
	sil_wrw_mem((void *)(GICD_ICFGRn + (uintptr_t)(4 * 1)), 0x55555555);
}

/*
 *  GIC割込みターゲットの設定（GICv2）
 * 
 *  この関数は，プロセッサ間排他制御を行った状態で呼び出さなければなら
 *  ない．  
 */
void
gicd_set_target(uint8_t id, uint8_t cpus)
{
	uintptr_t	offset_addr = (id / 4) * 4;
	uint32_t	shift  = (id % 4) * 8;
	uint32_t itr_reg_val;
	SIL_PRE_LOC;

	SIL_LOC_INT();
    
	itr_reg_val  = sil_rew_mem((void *)(GICD_ITARGETSRn + offset_addr));
	itr_reg_val &= ~(0xf << shift);
	itr_reg_val |= (cpus << shift);
	sil_wrw_mem((void *)(GICD_ITARGETSRn + offset_addr), itr_reg_val);

	SIL_UNL_INT();
}

#ifndef OMIT_GIC_INITIALIZE_INTERRUPT

/*
 *  割込み要求ラインの属性の設定
 *
 *  ASP3カーネルでの利用を想定して，パラメータエラーはアサーションでチェッ
 *  クしている．
 */
Inline void
config_int(INTNO intno, ATR intatr, PRI intpri)
{
	SIL_PRE_LOC;

	assert(VALID_INTNO(intno));
	assert(TMIN_INTPRI <= intpri && intpri <= TMAX_INTPRI);

	/*
	 *  割込みをロック
	 */
	SIL_LOC_INT();

	/*
	 *  割込みを禁止
	 *
	 *  割込みを受け付けたまま，レベルトリガ／エッジトリガの設定や，割
	 *  込み優先度の設定を行うのは危険なため，割込み属性にかかわらず，
	 *  一旦マスクする．
	 */
	disable_int(intno);

	/*
	 *  割込みをコンフィギュレーション
	 */
	if ((intatr & TA_EDGE) != 0U) {
		gicd_config(intno, GICD_ICFGRn_EDGE, true);
		clear_int(intno);
	}
	else {
		gicd_config(intno, GICD_ICFGRn_LEVEL, true);
	}

	/*
	 *  割込み優先度とターゲットプロセッサを設定
	 */
	gicd_set_priority(intno, INT_IPM(intpri));
	if (intno >= TMIN_GLOBAL_INTNO) {
		gicd_set_target(intno, GICD_ITARGETSRn_CPU0);
	}

	/*
	 * 割込みを許可
	 */
	if ((intatr & TA_ENAINT) != 0U) {
		enable_int(intno);
	}

	/*
	 *  割込みロックを解除
	 */
	SIL_UNL_INT();
}

/*
 *  割込み管理機能の初期化
 */
void
initialize_interrupt(void)
{
	uint_t			i;
	const INTINIB	*p_intinib;

	for (i = 0; i < tnum_cfg_intno; i++) {
		p_intinib = &(intinib_table[i]);
		config_int(p_intinib->intno, p_intinib->intatr, p_intinib->intpri);
	}
}

#endif /* OMIT_GIC_INITIALIZE_INTERRUPT */
