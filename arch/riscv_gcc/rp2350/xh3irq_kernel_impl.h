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
 *     カーネルの割込みコントローラ依存部（Hazard3 Xh3irq用）
 *
 *  このヘッダファイルは，target_kernel_impl.h（または，そこからインク
 *  ルードされるファイル）のみからインクルードされる．他のファイルから
 *  直接インクルードしてはならない．
 *
 *  Xh3irqはHazard3のカスタム割込みコントローラで，カスタムCSR
 *  （meiea/meipa/meifa/meipra/meinext/meicontext）で制御する．
 *  PLICと異なりメモリマップトレジスタを持たない．
 *
 *  ■ ウィンドウ方式のCSRアクセス（meiea/meipa/meifa/meipra）
 *  書込みデータの下位ビット（meiea等はbit4:0，meipraはbit6:0）が
 *  ウィンドウINDEX，bit31:16がウィンドウデータ．INDEXフィールドは
 *  0として読み返されるため，csrrs/csrrcに (data<<16)|index を渡すと
 *  当該ウィンドウのビットset/clearになり，csrrsにindexのみを渡すと
 *  ウィンドウの読出しになる．
 *
 *  ■ 割込み番号の対応
 *  ASP3の割込み番号INTNO = RP2350のIRQ番号 + 1 とする（1〜52）．
 *  INTNO=0は割込み入口処理（core_support.S）がSpurious割込みの番兵に
 *  使うため空ける（PLICではIRQ 0が予約だったが，Xh3irqはIRQ 0
 *  （TIMER0_IRQ_0）が実在するため，+1ずらす）．
 *
 *  ■ 割込み優先度
 *  meipraの4ビット優先度（0〜15，大きいほど高優先度）に，ASP3の内部
 *  表現（INT_IPM：1〜15）をそのまま対応付ける．割込み優先度マスクは
 *  meicontext.PREEMPT（優先度 >= PREEMPT の割込みのみmip.MEIPが立つ）
 *  で実現し，内部表現mのとき PREEMPT = m + 1 とする（優先度 <= m を
 *  ブロック）．meinext読出し（UPDATE=1）でハードウェアがPREEMPTを
 *  受け付けた割込みの優先度+1に自動更新するため，割込み入口では
 *  更新前のmeicontextを保存し，出口で書き戻す．
 *
 *  参照：RP2350 Datasheet §3.8（Hazard3）／Hazard3仕様書 csr.adoc
 */

#ifndef TOPPERS_XH3IRQ_KERNEL_IMPL_H
#define TOPPERS_XH3IRQ_KERNEL_IMPL_H

#include <sil.h>
#include "riscv.h"

/*
 *  Xh3irqカスタムCSR番号（アセンブリ言語からも参照する）
 */
#define XH3IRQ_MEIEA       0xbe0    /* 割込み許可ビット列（窓方式） */
#define XH3IRQ_MEIPA       0xbe1    /* 割込みペンディングビット列（窓方式） */
#define XH3IRQ_MEIFA       0xbe2    /* 割込み強制ビット列（窓方式） */
#define XH3IRQ_MEIPRA      0xbe3    /* 割込み優先度配列（窓方式・4bit/IRQ） */
#define XH3IRQ_MEINEXT     0xbe4    /* 次に処理すべき割込みの取得 */
#define XH3IRQ_MEICONTEXT  0xbe5    /* 割込みコンテキスト（PREEMPT等） */

/*
 *  meinextのフィールド
 */
#define XH3IRQ_MEINEXT_UPDATE   UINT_C(0x00000001)  /* meicontext更新指示 */

/*
 *  meicontextのフィールド
 */
#define XH3IRQ_MEICONTEXT_PREEMPT_SHIFT  16
#define XH3IRQ_MEICONTEXT_PREEMPT_MASK   UINT_C(0x001f0000)

/*
 *  RP2350が結線しているIRQ本数（IRQ番号0〜51）
 */
#define XH3IRQ_TNUM_IRQ    UINT_C(52)

/*
 *  RISC-Vコアで共通な定義
 */
#include "core_kernel_impl.h"

#ifndef TOPPERS_MACRO_ONLY

/*
 *  窓方式CSRのプリミティブ
 *
 *  CSR番号はアセンブリの即値でなければならないため，レジスタ毎に
 *  関数を定義する．
 */
Inline void
xh3irq_meiea_set(uint32_t val)
{
	ulong_t tmp;
	Asm("csrrs %0, 0xbe0, %1" : "=r"(tmp) : "r"(val));
}

Inline void
xh3irq_meiea_clear(uint32_t val)
{
	ulong_t tmp;
	Asm("csrrc %0, 0xbe0, %1" : "=r"(tmp) : "r"(val));
}

Inline void
xh3irq_meiea_write(uint32_t val)
{
	Asm("csrw 0xbe0, %0" : : "r"(val));
}

Inline uint32_t
xh3irq_meipa_read(uint32_t index)
{
	ulong_t tmp;
	Asm("csrrs %0, 0xbe1, %1" : "=r"(tmp) : "r"(index));
	return((uint32_t)(tmp >> 16));
}

Inline void
xh3irq_meifa_set(uint32_t val)
{
	ulong_t tmp;
	Asm("csrrs %0, 0xbe2, %1" : "=r"(tmp) : "r"(val));
}

Inline void
xh3irq_meifa_clear(uint32_t val)
{
	ulong_t tmp;
	Asm("csrrc %0, 0xbe2, %1" : "=r"(tmp) : "r"(val));
}

Inline void
xh3irq_meifa_write(uint32_t val)
{
	Asm("csrw 0xbe2, %0" : : "r"(val));
}

Inline void
xh3irq_meipra_set(uint32_t val)
{
	ulong_t tmp;
	Asm("csrrs %0, 0xbe3, %1" : "=r"(tmp) : "r"(val));
}

Inline void
xh3irq_meipra_clear(uint32_t val)
{
	ulong_t tmp;
	Asm("csrrc %0, 0xbe3, %1" : "=r"(tmp) : "r"(val));
}

Inline void
xh3irq_meipra_write(uint32_t val)
{
	Asm("csrw 0xbe3, %0" : : "r"(val));
}

Inline ulong_t
xh3irq_meicontext_read(void)
{
	ulong_t val;
	Asm("csrr %0, 0xbe5" : "=r"(val));
	return(val);
}

Inline void
xh3irq_meicontext_write(ulong_t val)
{
	Asm("csrw 0xbe5, %0" : : "r"(val));
}

/*
 *  Xh3irqの操作（intnoはASP3の割込み番号＝IRQ番号+1）
 */

/*
 *  割込み禁止
 */
Inline void
xh3irq_disable_int(INTNO intno)
{
	uint32_t irq = (uint32_t)(intno - 1U);

	xh3irq_meiea_clear(((1U << (irq % 16U)) << 16) | (irq / 16U));
}

/*
 *  割込み許可
 */
Inline void
xh3irq_enable_int(INTNO intno)
{
	uint32_t irq = (uint32_t)(intno - 1U);

	xh3irq_meiea_set(((1U << (irq % 16U)) << 16) | (irq / 16U));
}

/*
 *  割込みペンディングのチェック
 */
Inline bool_t
xh3irq_probe_int(INTNO intno)
{
	uint32_t irq = (uint32_t)(intno - 1U);

	return((xh3irq_meipa_read(irq / 16U) & (1U << (irq % 16U))) != 0U);
}

/*
 *  割込みの強制（ソフトウェアからのペンディング．meinextでの受付け時に
 *  ハードウェアが自動クリアする＝エッジペンディング相当）
 */
Inline void
xh3irq_raise_int(INTNO intno)
{
	uint32_t irq = (uint32_t)(intno - 1U);

	xh3irq_meifa_set(((1U << (irq % 16U)) << 16) | (irq / 16U));
}

/*
 *  強制した割込みのクリア
 */
Inline void
xh3irq_clear_int(INTNO intno)
{
	uint32_t irq = (uint32_t)(intno - 1U);

	xh3irq_meifa_clear(((1U << (irq % 16U)) << 16) | (irq / 16U));
}

/*
 *  割込み要求ラインに対する割込み優先度の設定（priは内部表現1〜15）
 */
Inline void
xh3irq_set_priority(INTNO intno, uint_t pri)
{
	uint32_t irq = (uint32_t)(intno - 1U);
	uint32_t shift = (irq % 4U) * 4U;

	xh3irq_meipra_clear(((0xfU << shift) << 16) | (irq / 4U));
	xh3irq_meipra_set((((uint32_t)pri << shift) << 16) | (irq / 4U));
}

/*
 *  割込み優先度マスクの設定（mは内部表現0〜15．PREEMPT = m + 1）
 */
Inline void
xh3irq_set_preempt(uint_t m)
{
	ulong_t ctx = xh3irq_meicontext_read();

	ctx = (ctx & ~(ulong_t)XH3IRQ_MEICONTEXT_PREEMPT_MASK)
			| ((ulong_t)(m + 1U) << XH3IRQ_MEICONTEXT_PREEMPT_SHIFT);
	xh3irq_meicontext_write(ctx);
}

/*
 *  割込み優先度マスクの参照（内部表現で返す）
 */
Inline uint_t
xh3irq_get_preempt(void)
{
	return((uint_t)((xh3irq_meicontext_read()
						& XH3IRQ_MEICONTEXT_PREEMPT_MASK)
					>> XH3IRQ_MEICONTEXT_PREEMPT_SHIFT) - 1U);
}

/*
 *  Xh3irqの初期化
 */
extern void xh3irq_initialize(void);

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_XH3IRQ_KERNEL_IMPL_H */
