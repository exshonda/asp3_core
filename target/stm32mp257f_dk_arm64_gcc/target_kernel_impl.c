/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2007-2023 by Embedded and Real-Time Systems Laboratory
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
 *  @(#) $Id: target_kernel_impl.c 353 2023-05-05 04:49:18Z ertl-honda $  
 */

/*
 *  ターゲット依存モジュール（STM32MP257F-DK用）
 */
#include "kernel_impl.h"
#include <sil.h>

/*
 *  システムログの低レベル出力のための初期化
 */
#ifndef TOPPERS_OMIT_TECS

/*
 *  セルタイプtPutLogSIOPort内に実装されている関数を直接呼び出す．
 */
extern void	tPutLogSIOPort_initialize(void);

#else /* TOPPERS_OMIT_TECS */

extern void	sio_initialize(EXINF exinf);
extern void	target_fput_initialize(void);

#endif /* TOPPERS_OMIT_TECS */

/*
 *  タイマの周波数を保持する変数
 *  単位はkHz
 *  target_initialize() で初期化される
 */
uint32_t	timer_clock_mhz;

/*
 *  EL3で行う初期化処理
 */
void
target_el3_initialize(void)
{
	chip_el3_initialize();
}

/*
 *  EL2で行う初期化処理
 */
void
target_el2_initialize(void)
{
	chip_el2_initialize();
}

/*
 *  MMUへの設定情報（メモリエリアの情報）
 *
 *  core依存部のmmu_init()が本テーブル（arm64_memory_area）を走査して
 *  メモリマップを設定する．weak定義であり，アプリケーション側で同名の
 *  テーブルを定義することでテーブル全体を差し替えることができる．
 *  全領域は物理アドレス = 仮想アドレス
 */
#ifdef TOPPERS_TZ_S
#define MMU_MEM_NS		MEM_NS_SECURE
#else  /* TOPPERS_TZ_S */
#define MMU_MEM_NS		MEM_NS_NONSECURE
#endif /* TOPPERS_TZ_S */

#ifdef TOPPERS_32BIT_ABOVE_ADDR
/* Cortex-A53で利用可能な全てのメモリとレジスタをMMUに設定する場合に定義する。
 * ATFなどの同時動作するソフトウェアを利用する場合、定義しない方が良い */
//#define INITMMU_ALL_MEM

#ifndef INITMMU_ALL_MEM
/*
 *  使用領域のみ設定（STM32MP2）
 *    ペリフェラル: 0x40000000-0x50000000（USART2 0x400E0000 /
 *                  RCC 0x44200000 / GIC 0x4AC00000 を含む）
 *    DDR        : 0x80000000-0xC0000000（text 0x88000040 /
 *                  data 0x90000000 を含む）
 */
__attribute__((weak))
const ARM64_MMU_CONFIG arm64_memory_area[] = {
	/* { va, pa, size, attr, ap, ns } */
	/* Registers (peripherals + GIC) : Device-nGnRnE */
	{ 0x0040000000, 0x0040000000, 0x0010000000,
				MEM_ATTR_SO,    MEM_AP_RW_EL1, MMU_MEM_NS },
	/* TOPPERS/ASP3が動作するメモリ（DDR 全域をマップ）
	 * : Normal, Outer and Inner Write-Back No-transient */
	{ DDR_ADDR,     DDR_ADDR,     DDR_SIZE,
				MEM_ATTR_NML_C, MEM_AP_RW_EL1, MMU_MEM_NS },
};
#else  /* INITMMU_ALL_MEM */
/*
 *  全領域設定
 */
__attribute__((weak))
const ARM64_MMU_CONFIG arm64_memory_area[] = {
	/* { va, pa, size, attr, ap, ns } */
	/* Internal RAM : Normal, Outer and Inner Write-Back No-transient */
	{ 0x0000100000, 0x0000100000, 0x0000A00000,
				MEM_ATTR_NML_C, MEM_AP_RW_EL1, MMU_MEM_NS },
	/* QSPI Flash memory : Normal, Write-Back, Read only */
	{ 0x0008000000, 0x0008000000, 0x0010000000,
				MEM_ATTR_NML_C, MEM_AP_RO_EL1, MMU_MEM_NS },
	/* Registers : Device-nGnRnE */
	{ 0x0018000000, 0x0018000000, 0x0028000000,
				MEM_ATTR_SO,    MEM_AP_RW_EL1, MMU_MEM_NS },
	/* DDR : Normal, Outer and Inner Write-Back No-transient */
	{ 0x0040000000, 0x0040000000, 0x0080000000,
				MEM_ATTR_NML_C, MEM_AP_RW_EL1, MMU_MEM_NS },
};
#endif /* INITMMU_ALL_MEM */
#else	/* TOPPERS_32BIT_ABOVE_ADDR */
#ifdef TOPPERS_TZ_S
/*
 *  使用領域のみ設定 (Secure OS)
 *
 *  注: 旧target_mmu_init()ではapを設定していなかったため，
 *  既定値と同じMEM_AP_RW_EL1を明示している．
 */
__attribute__((weak))
const ARM64_MMU_CONFIG arm64_memory_area[] = {
	/* { va, pa, size, attr, ap, ns } */
	/* TOPPERS/ASP3が動作するメモリ
	 * : Normal, Outer and Inner Write-Back No-transient */
	{ TOPPERS_MEM_ADDR, TOPPERS_MEM_ADDR, TOPPERS_MEM_SIZE,
				MEM_ATTR_NML_C, MEM_AP_RW_EL1, MEM_NS_SECURE },
	/* Registers : Device-nGnRnE（Secure/NonSecureの両方を登録） */
	{ 0x0008000000, 0x0008000000, 0x0038000000,
				MEM_ATTR_SO,    MEM_AP_RW_EL1, MEM_NS_SECURE },
	{ 0x0008000000, 0x0008000000, 0x0038000000,
				MEM_ATTR_SO,    MEM_AP_RW_EL1, MEM_NS_NONSECURE },
};
#else  /* TOPPERS_TZ_S */
/*
 *  使用領域のみ設定 (NonSecure OS)
 *
 *  注: 旧target_mmu_init()ではapを設定していなかったため，
 *  既定値と同じMEM_AP_RW_EL1を明示している．
 */
__attribute__((weak))
const ARM64_MMU_CONFIG arm64_memory_area[] = {
	/* { va, pa, size, attr, ap, ns } */
	/* TOPPERS/ASP3が動作するメモリ
	 * : Normal, Outer and Inner Write-Back No-transient */
	{ TOPPERS_MEM_ADDR, TOPPERS_MEM_ADDR, TOPPERS_MEM_SIZE,
				MEM_ATTR_NML_C, MEM_AP_RW_EL1, MEM_NS_NONSECURE },
	/* Registers : Device-nGnRnE */
	{ 0x0008000000, 0x0008000000, 0x0038000000,
				MEM_ATTR_SO,    MEM_AP_RW_EL1, MEM_NS_NONSECURE },
};
#endif /* TOPPERS_TZ_S */
#endif	/* TOPPERS_32BIT_ABOVE_ADDR */

/*
 *  MMUの設定情報の数（メモリエリアの数）
 */
__attribute__((weak))
const uint_t arm64_tnum_memory_area
					= sizeof(arm64_memory_area) / sizeof(ARM64_MMU_CONFIG);

/*
 *  ターゲット依存の初期化
 */
void
target_initialize(void)
{
	uint32_t timer_clock_hz;

	/*
	 *  タイマのクロックの取得
	 */
	CNTFRQ_EL0_READ(timer_clock_hz);
	timer_clock_mhz = timer_clock_hz / 1000000;

	/*
	 * チップ依存の初期化
	 */
	chip_initialize();

	/*
	 *  バナー表示，低レベル出力用にUARTを初期化
	 */
#ifndef TOPPERS_OMIT_TECS
	tPutLogSIOPort_initialize();
#else /* TOPPERS_OMIT_TECS */
	sio_initialize(0);
	target_fput_initialize();
#endif /* TOPPERS_OMIT_TECS */
}

/*
 *  デフォルトのsoftware_term_hook（weak定義）
 */
__attribute__((weak))
void software_term_hook(void)
{
}

/*
 *  ターゲット依存の終了処理
 */
void
target_exit(void)
{
	/*
	 *  software_term_hookの呼出し
	 */
	software_term_hook();    

	/*
	 *  チップ依存の終了処理
	 */
	chip_terminate();

	while (true) ;
}

#ifdef TOPPERS_OMIT_TECS
/*
 *		システムログの低レベル出力（非TECS版専用）
 */

#include "target_syssvc.h"
#include "target_serial.h"

/*
 *  低レベル出力用のSIOポート管理ブロック
 */
static SIOPCB	*p_siopcb_target_fput;

/*
 *  SIOポートの初期化
 */
void
target_fput_initialize(void)
{
	p_siopcb_target_fput = stm32usart_opn_por(SIOPID_FPUT, 0);
}

/*
 *  SIOポートへのポーリング出力
 */
static void
stm32mp257f_dk_uart_fput(char c)
{
	/*
	 *  送信できるまでポーリング
	 */
	while (!(stm32usart_snd_chr(p_siopcb_target_fput, c))) {
		sil_dly_nse(100);
	}
}

/*
 *  SIOポートへの文字出力
 */
void
target_fput_log(char c)
{
	if (c == '\n') {
		stm32mp257f_dk_uart_fput('\r');
	}
	stm32mp257f_dk_uart_fput(c);
}

#endif /* TOPPERS_OMIT_TECS */
