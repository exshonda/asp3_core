/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2024 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，以下の(1)〜(4)の条件を満たす場合に限り，本ソフトウェ
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
 *  $Id: target_syssvc.h 343 2023-04-19 13:51:34Z ertl-honda $
 */

/*
 *    システムサービスのターゲット依存部（PolarFire SoC Kit用）
 *
 *  システムサービスのターゲット依存部のヘッダファイル．
 */

#ifndef TOPPERS_TARGET_SYSSVC_H
#define TOPPERS_TARGET_SYSSVC_H

#include "polarfire_soc_kit.h"
#include "polarfire_soc.h"

/*
 *  システムログの低レベル出力のための文字出力
 *
 *  ターゲット依存の方法で，文字cを表示/出力/保存する．
 */
extern void target_fput_log(char c);

/*
 *  シリアルポートの数
 */
#define TNUM_PORT  1

/*
 *  SIOドライバで使用するMMUARTに関する設定
 */
#ifdef USE_UART0
#define INTNO_SIO   MMUART0_INTNO     /* SIO割込み番号 */
#define SIO0_BASE   MMUART0_BASE
#endif /* USE_UART0 */

#ifdef USE_UART1
#define INTNO_SIO   MMUART1_INTN1     /* SIO割込み番号 */
#define SIO0_BASE   MMUART1_BASE
#endif /* USE_UART1 */

/*
 *  SIO割込みを登録するための定義
 */
#define ISRPRI_SIO  1                 /* SIO ISR優先度 */
#define INTPRI_SIO  (-4)              /* SIO割込み優先度 */
#define INTATR_SIO  TA_NULL           /* SIO割込み属性 */

/*
 *  MMUARTのボーレート設定（115200bps 8N1）
 */
#define SIO0_LCONFIG   (MMUART_LCR_WLS_8BIT | MMUART_LCR_PEN_NOP | MMUART_LCR_STB_ONE)
#define SIO0_BAUD      UINT_C(81)
#define SIO0_FBAUDDIV  UINT_C(24)

/*
 *  低レベル出力で使用するSIOポート
 */
#define SIOPID_FPUT  1

/*
 *  コアで共通な定義
 */
#include "core_syssvc.h"

#endif /* TOPPERS_TARGET_SYSSVC_H */
