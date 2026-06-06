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
 *    kernel.hのチップ依存部（RP2350 RISC-V（Hazard3）用）
 *
 *  このヘッダファイルは，target_kernel.h（または，そこからインクルー
 *  ドされるファイル）のみからインクルードされる．他のファイルから直接
 *  インクルードしてはならない．
 */

#ifndef TOPPERS_CHIP_KERNEL_H
#define TOPPERS_CHIP_KERNEL_H

/*
 *  割込み優先度の範囲
 *
 *  Xh3irqの割込み優先度は4ビット（16段階）．優先度0は割込み優先度
 *  マスク全解除状態（PREEMPT=1）でもマスクされる値として空け，
 *  -1〜-15を内部表現1〜15に対応付ける（xh3irq_kernel_impl.h参照）．
 */
#define TMIN_INTPRI  (-15)   /* 割込み優先度の最小値（最高値）*/
#define TMAX_INTPRI  (-1)    /* 割込み優先度の最大値（最低値）*/

/*
 *  サポートできる機能の定義
 *
 *  ena_int／dis_int／clr_int／ras_int／prb_intをサポートする
 *  （clr_int／ras_intはXh3irqの割込み強制ビット（meifa）による）．
 */
#define TOPPERS_TARGET_SUPPORT_ENA_INT    /* ena_int */
#define TOPPERS_TARGET_SUPPORT_DIS_INT    /* dis_int */
#define TOPPERS_TARGET_SUPPORT_CLR_INT    /* clr_int */
#define TOPPERS_TARGET_SUPPORT_RAS_INT    /* ras_int */
#define TOPPERS_TARGET_SUPPORT_PRB_INT    /* prb_int */

/*
 *  コアで共通な定義
 */
#include "core_kernel.h"

#endif /* TOPPERS_CHIP_KERNEL_H */
