/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
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
 *  $Id: gic_kernel_impl.h 361 2023-07-21 06:23:26Z ertl-honda $
 */

/*
 *     カーネルの割込みコントローラ依存部（PLIC用）
 *
 *  このヘッダファイルは，target_kernel_impl.h（または，そこからインク
 *  ルードされるファイル）のみからインクルードされる．他のファイルから
 *  直接インクルードしてはならない．
 *
 *  TOPPERS/FMP3のRISC-V依存部をASP3（シングルプロセッサ）用に変換した
 *  もの．割込みターゲットテーブル（plic_target_cidx_table）を廃止し，
 *  自ハートのコンテキスト（get_my_plic_cidx()，チップ依存部で定義）の
 *  みを操作する．
 */

#ifndef TOPPERS_PLIC_KERNEL_IMPL_H
#define TOPPERS_PLIC_KERNEL_IMPL_H

#include <sil.h>
#include "riscv.h"

/*
 *  PLICレジスタのアドレスを定義するためのマクロ
 *
 *  PLICレジスタのアドレスを，アセンブリ言語からも参照できるようにする
 *  ためのマクロ．
 */
#ifndef PLIC_REG
#define PLIC_REG(base, offset)	((base) + (offset))
#endif /* PLIC_REG */

/*
 *  レジスタの定義
 */
#define PLIC_IPRI(n)       ((uint32_t *)(PLIC_BASE + 0x000000U + ((n) * 4U)))
#define PLIC_IPEND(n)      ((uint32_t *)(PLIC_BASE + 0x001000U + (n) * 4U))
#define PLIC_IEM(c,n)      ((uint32_t *)(PLIC_BASE + 0x002000U + (((c) * 0x80U) + ((n) * 4U))))
#define PLIC_TH(c)         ((uint32_t *)(PLIC_BASE + 0x200000U + ((c) * 0x1000U)))
#define PLIC_CC(c)         ((uint32_t *)(PLIC_BASE + 0x200004U + ((c) * 0x1000U)))

#define PLIC_IPRI_BASE  PLIC_REG(PLIC_BASE, 0x000000)
#define PLIC_TH_BASE    PLIC_REG(PLIC_BASE, 0x200000)
#define PLIC_CC_BASE    PLIC_REG(PLIC_BASE, 0x200004)
#define PLIC_TH_OFFSET  UINT_C(0x1000)
#define PLIC_CC_OFFSET  UINT_C(0x1000)

/*
 *  RISC-Vコアで共通な定義
 */
#include "core_kernel_impl.h"

#ifndef TOPPERS_MACRO_ONLY

/*
 *  PLICの操作
 */

/*
 *  割込み優先度マスクを設定（priは内部表現）
 */
Inline void
plic_set_context_priority(uint_t cidx, uint_t pri)
{
    sil_wrw_mem(PLIC_TH(cidx), pri);
}

/*
 *  割込み優先度マスクを取得（内部表現で返す）
 */
Inline uint_t
plic_get_context_priority(uint_t cidx)
{
    return(sil_rew_mem(PLIC_TH(cidx)));
}

/*
 *  割込み禁止
 */
Inline void
plic_disable_int(INTNO intno)
{
    uint_t val;
    uint_t cidx = get_my_plic_cidx();

    val = sil_rew_mem(PLIC_IEM(cidx, (intno / 32)));
    val &= ~(1U << (intno % 32));
    sil_swrw_mem(PLIC_IEM(cidx, (intno / 32)), val);
}

/*
 *  割込み許可
 */
Inline void
plic_enable_int(INTNO intno)
{
    uint_t val;
    uint_t cidx = get_my_plic_cidx();

    val = sil_rew_mem(PLIC_IEM(cidx, (intno / 32)));
    val |= (1U << (intno % 32));
    sil_swrw_mem(PLIC_IEM(cidx, (intno / 32)), val);
}

/*
 *  割込みペンディングのチェック
 */
Inline bool_t
plic_probe_pending(INTNO intno)
{
    return((sil_rew_mem(PLIC_IPEND(intno / 32)) & (1U << (intno % 32))) != 0U);
}

/*
 *  割込み要求ラインに対する割込み優先度の設定（priは内部表現）
 *
 *  この関数は，カーネルの初期化中に呼び出すことを想定しているため，
 *  PLICの操作後にメモリ同期バリアを入れていない．
 */
Inline void
plic_set_priority(INTNO intno, uint_t pri)
{
    sil_swrw_mem(PLIC_IPRI(intno), pri);
}

/*
 *  PLICのコンテキスト単位の初期化
 */
extern void plic_context_initialize(void);

/*
 *  PLICのグローバルな初期化
 */
extern void plic_global_initialize(void);

/*
 *  割込み管理機能の初期化
 */
extern void plic_initialize_interrupt(void);

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_PLIC_KERNEL_IMPL_H */
