/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
 *  トウェアは無保証で提供される．
 */

/*
 *		システムサービスのコア依存部（ARM-M用）
 *
 *  このヘッダファイルは，target_syssvc.h（または，そこからインクルード
 *  されるファイル）のみからインクルードされる．他のファイルから直接イ
 *  ンクルードしてはならない（arm_gcc/riscv の core_syssvc.h と同じ規約）．
 *
 *  実行時間分布集計サービス（syssvc/histogram）の時間源を，DWT の CYCCNT
 *  サイクルカウンタにする設定を提供する．DWT/CYCCNT は ARMv7-M 以降の
 *  オプション機能で，コアクロック 1 サイクル単位で計数するため，fch_hrt()
 *  （マイクロ秒分解能の HRT）より遥かに細かくコンテキストスイッチや
 *  サービスコールのサイクル数を計測できる．
 *
 *  arm_gcc の USE_ARM_PMCNT（PMCCNTR）に倣い，USE_ARM_DWT_PMCNT を定義した
 *  ビルドでのみ有効化する（オプトイン）．未定義の通常ビルドでは何も定義せず，
 *  histogram は既定の fch_hrt() を用いる（＝DWT は無効・副作用なし）．
 *  カウンタの有効化（DEMCR.TRCENA / DWT_CTRL.CYCCNTENA）は core_initialize()
 *  が同フラグの下で行う（arm_m_pmcnt_initialize）．
 *
 *  なお QEMU は DWT CYCCNT を実装せず常に 0 を返すため，CYCCNT を時間源に
 *  するのは実機（Cortex-M33 等）のボードに限ること．
 */

#ifndef TOPPERS_CORE_SYSSVC_H
#define TOPPERS_CORE_SYSSVC_H

#include <t_stddef.h>
#include "arm_m.h"

/*
 *  DWT CYCCNT サイクルカウンタによる性能評価
 */
#if defined(USE_ARM_DWT_PMCNT) && __TARGET_ARCH_THUMB >= 4

/*
 *  パフォーマンスカウンタのデータ型（DWT CYCCNT は 32bit）
 */
typedef uint32_t	PMCNT;

/*
 *  パフォーマンスカウンタ（CYCCNT）の読み込み
 */
Inline void
arm_m_get_pmcnt(PMCNT *p_count)
{
	*p_count = *((volatile uint32_t *) DWT_CYCCNT_ADDR);
}

/*
 *  実行時間分布集計サービスの設定
 *
 *  計測単位はサイクル（生値）とする．CYCCNT は full 32bit のフリーラン
 *  カウンタなので，histogram.c の end-begin（符号無し 32bit 減算）だけで
 *  ラップは正しく扱える（HISTTIM_CYCLE は定義しない）．
 */
#define HISTTIM					PMCNT
#define HIST_GET_TIM(p_time)	(arm_m_get_pmcnt(p_time))

#endif /* defined(USE_ARM_DWT_PMCNT) && __TARGET_ARCH_THUMB >= 4 */

#endif /* TOPPERS_CORE_SYSSVC_H */
