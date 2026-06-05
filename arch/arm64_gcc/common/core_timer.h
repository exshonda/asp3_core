/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2005-2021 by Embedded and Real-Time Systems Laboratory
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
 *  @(#) $Id: core_timer.h 350 2023-04-21 01:59:41Z ertl-honda $
 */

/*
 *  タイマドライバ（ARM64用）
 */

#ifndef TOPPERS_CORE_TIMER_H
#define TOPPERS_CORE_TIMER_H

#include "kernel/kernel_impl.h"
#include <sil.h>

/*
 *  高分解能タイマ割込みハンドラ登録のための定数
 */
#ifdef TOPPERS_TZ_S
#define INHNO_TIMER			GIC_IRQNO_SPHYTIMER
#define INTNO_TIMER			GIC_IRQNO_SPHYTIMER
#else /* TOPPERS_TZ_S */
#define INHNO_TIMER			GIC_IRQNO_NSPHYTIMER
#define INTNO_TIMER			GIC_IRQNO_NSPHYTIMER
#endif /* TOPPERS_TZ_S */

#ifndef INTPRI_TIMER
#define INTPRI_TIMER		(TMAX_INTPRI - 1)				/* 割込み優先度 */
#endif /* INTPRI_TIMER */

#ifndef INTATR_TIMER
#define INTATR_TIMER		TA_NULL							/* 割込み属性 */
#endif /* INTATR_TIMER */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  タイマ値の内部表現の型
 */
typedef uint64_t CLOCK;

/*
 *  タイマの設定値
 */
extern CLOCK timer_cval;

/*
 *  タイマの起動処理
 *
 *  タイマを初期化し，周期的なタイマ割込み要求を発生させる．
 */
extern void target_hrt_initialize(EXINF exinf);

/*
 *  タイマの停止処理
 *
 *  タイマの動作を停止させる．
 */
extern void target_hrt_terminate(EXINF exinf);


/*
 *  ジェネリックタイマの現在値の読出し
 */
Inline CLOCK
target_timer_get_count(void)
{
	CLOCK cnt;

	/*  現在のカウンタ値を返す  */
	CNTPCT_EL0_READ(cnt);
	return cnt;
}

/*
 *  タイマのコンパレータ値の設定
 */
Inline void
target_timer_set_cval(uint64_t cvr)
{
	/* 割込みをディスエーブル */
#ifdef TOPPERS_TZ_S
	CNTPS_CTL_EL1_WRITE((uint32_t)CNTPS_CTL_IMASK_BIT);
#else /* !TOPPERS_TZ_S */
	CNTP_CTL_EL0_WRITE((uint32_t)CNTP_CTL_IMASK_BIT);
#endif /* TOPPERS_TZ_S */

	/* コンパレータ値を設定 */
#ifdef TOPPERS_TZ_S
	CNTPS_CVAL_EL1_WRITE(cvr);
#else /* !TOPPERS_TZ_S */
	CNTP_CVAL_EL0_WRITE(cvr);
#endif /* TOPPERS_TZ_S */

	/* 割込みをイネーブル */
#ifdef TOPPERS_TZ_S
	CNTPS_CTL_EL1_WRITE((uint32_t)CNTPS_CTL_ENABLE_BIT);
#else /* !TOPPERS_TZ_S */
	CNTP_CTL_EL0_WRITE((uint32_t)CNTP_CTL_ENABLE_BIT);
#endif /* TOPPERS_TZ_S */
}

/*
 *  タイマ割込み要求のチェック
 */
Inline bool_t
target_timer_probe_int(void)
{
#if 0
	uint32_t ctl;

#if defined(TOPPERS_TZ_S)
	CNTPS_CTL_EL1_READ(ctl);
#else
	CNTP_CTL_EL0_READ(ctl);
#endif

	return ((ctl & CNTP_CTL_ISTATUS_BIT) == CNTP_CTL_ISTATUS_BIT);
#else
	/*
	 * Generic Timerはタイムアウト後はCNTP_CVALを更新するまで，CNTP_TVALの値は0のまま
	 * であるため， target_timer_probe_int()がtrueを返すとget_utm()に不整合が発生するため，
	 * 常にfalseを返すこととする． 
	 */
	return false;
#endif /* 0 */
}

/*
 *  高分解能タイマ
 */
/*
 *  高分解能タイマ割込みハンドラ
 */
extern void	target_hrt_handler(void);


/*
 *  高分解能タイマの起動処理
 */
extern void	target_hrt_initialize(EXINF exinf);

/*
 *  高分解能タイマの停止処理
 */
extern void	target_hrt_terminate(EXINF exinf);

/*
 *  高分解能タイマの現在のカウント値の読出し
 */
Inline HRTCNT
target_hrt_get_current(void)
{
	/*
	 *  タイマのカウント値を読み出し，
	 *  timer_clockで除した値を返す．
	 */
	return(HRT_CNT_TO_HRTCNT(target_timer_get_count()));

}

/*
 *  高分解能タイマへの割込みタイミングの設定
 *
 *  高分解能タイマを，hrtcntで指定した値カウントアップしたら割込みを発
 *  生させるように設定する．
 */
Inline void
target_hrt_set_event(HRTCNT hrtcnt)
{
	/*
	 *  コンパレータ値を，(現在のカウント値＋hrtcnt×timer_clock)
	 *  に設定し，コンパレータと割込みをイネーブルする．
	 */
	target_timer_set_cval(target_timer_get_count()
						  + HRT_HRTCNT_TO_CNT((uint64_t)hrtcnt));
}

/*
 *  高分解能タイマへの割込みタイミングのクリア
 */
Inline void
target_hrt_clear_event(void)
{
	uint32_t ctl;

#ifdef TOPPERS_TZ_S
	CNTPS_CTL_EL1_READ(ctl);
	CNTPS_CTL_EL1_WRITE(ctl & ~((uint32_t)CNTPS_CTL_ENABLE_BIT));
#else /* !TOPPERS_TZ_S */
	CNTP_CTL_EL0_READ(ctl);
	CNTP_CTL_EL0_WRITE(ctl & ~((uint32_t)CNTP_CTL_ENABLE_BIT));
#endif /* TOPPERS_TZ_S */
}

/*
 *  高分解能タイマ割込みの要求
 */
Inline void
target_hrt_raise_event(void)
{
	target_hrt_set_event(1U);
}

/*
 *  割込みタイミングに指定する最大値
 */
#if !defined(TCYC_HRTCNT) || (TCYC_HRTCNT > 4002000002U)
#define HRTCNT_BOUND		4000000002U
#else
#define HRTCNT_BOUND		(TCYC_HRTCNT - 2000000U)
#endif

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_CORE_TIMER_H */
