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
 *
 */

/*
 *  kernel.h のターゲット依存部（ARM MPS2-AN505 用）
 *
 *  このインクルードファイルは，kernel.h でインクルードされる．このファ
 *  イルをインクルードする前に，t_stddef.h がインクルードされる．
 */

#ifndef TOPPERS_TARGET_KERNEL_H
#define TOPPERS_TARGET_KERNEL_H

/*
 *  カーネル管理の割込み優先度の範囲
 *
 *  この値（最高の割込み優先度）よりも高い割込み優先度を持つものをカーネ
 *  ル管理外の割込みとする．
 */
#define TMIN_INTPRI (-3)    /* 割込み優先度の最小値（最高値）*/

/*
 *  高分解能タイマの設定
 *
 *  タイマ周期が 2^32 [us] の場合には TCYC_HRTCNT を定義しない．
 */
#undef TCYC_HRTCNT

#ifdef USE_TIM_AS_HRT
/*
 *  イベント駆動 HRT（SysTick）：カウント値の進み幅は 1us．
 */
#define TSTEP_HRTCNT 1U
#else /* 周期ティック（SYSTICK）：1 ティック = 1000us（1ms） */
#define TSTEP_HRTCNT 1000U
#endif /* USE_TIM_AS_HRT */

/*
 *  コア依存で共通な定義
 */
#include "core_kernel.h"

#endif /* TOPPERS_TARGET_KERNEL_H */
