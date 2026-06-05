/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 * 
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2004-2022 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: target_sil.h 1723 2022-10-23 07:41:14Z ertl-hiro $
 */

/*
 *		sil.hのターゲット依存部（Linux用）
 *
 *  このインクルードファイルは，sil.hの先頭でインクルードされる．他のファ
 *  イルからは直接インクルードすることはない．このファイルをインクルー
 *  ドする前に，t_stddef.hがインクルードされるので，それらに依存しても
 *  よい．
 */

#ifndef TOPPERS_TARGET_SIL_H
#define TOPPERS_TARGET_SIL_H

#ifndef TOPPERS_MACRO_ONLY

/*
 *  標準のインクルードファイル
 */
#include <signal.h>
#include <endian.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/membarrier.h>

#include <t_stddef.h>

/*
 *  割込みロックでマスクするシグナルをマスク
 */
Inline void
TOPPERS_dissig(sigset_t *p_sigmask)
{
	extern sigset_t _kernel_sigmask_intlock;

	pthread_sigmask(SIG_BLOCK, &_kernel_sigmask_intlock, p_sigmask);
}

/*
 *  シグナルマスクを設定
 */
Inline void
TOPPERS_setsig(sigset_t *p_sigmask)
{
	pthread_sigmask(SIG_SETMASK, p_sigmask, NULL);
}

/*
 *  全割込みロック状態の制御
 */
#define SIL_PRE_LOC		sigset_t TOPPERS_sigmask
#define SIL_LOC_INT()	(TOPPERS_dissig(&TOPPERS_sigmask))
#define SIL_UNL_INT()	(TOPPERS_setsig(&TOPPERS_sigmask))

#endif /* TOPPERS_MACRO_ONLY */

/*
 *  プロセッサのエンディアン
 */
#ifdef BYTE_ORDER
#if BYTE_ORDER == BIG_ENDIAN
#define SIL_ENDIAN_BIG				/* ビッグエンディアン */
#elif BYTE_ORDER == LITTLE_ENDIAN
#define SIL_ENDIAN_LITTLE			/* リトルエンディアン */
#else
#error BYTE_ORDER is unknown.
#endif
#else /* BYTE_ORDER */
#error BYTE_ORDER is not defined.
#endif /* BYTE_ORDER */

/*
 *  メモリ同期バリア
 */
#define TOPPERS_SIL_WRITE_SYNC() \
				syscall(__NR_membarrier, MEMBARRIER_CMD_SHARED, 0)

#endif /* TOPPERS_TARGET_SIL_H */
