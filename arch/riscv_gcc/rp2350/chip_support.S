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
 *    カーネルのチップ依存部のアセンブリ言語部（RP2350 RISC-V（Hazard3）用）
 *
 *  PolarFire SoC依存部（PLIC claim/complete方式）をXh3irq
 *  （meinext/meicontext方式）に置き換えたもの：
 *    - 割込み番号の取得はPLICのclaimレジスタではなくmeinext（UPDATE=1）
 *      で行う．UPDATEによりハードウェアがmeicontext.PREEMPTを受け付けた
 *      割込みの優先度+1へ自動更新するため，割込み優先度マスクの個別設定
 *      は不要．
 *    - 割込み前の優先度（irc_begin_intのa1）はmeicontext全体の値とし，
 *      irc_end_intで書き戻す（PLICの閾値レジスタ保存/復元の置換え）．
 *    - ASP3の割込み番号INTNO = IRQ番号 + 1（INTNO=0はSpurious番兵．
 *      xh3irq_kernel_impl.h参照）．
 *    - MTI/MSIは使用しない（mieで許可しない）．万一入った場合は
 *      PolarFire版と同様 INTNO = (0x10000 | mcause) として
 *      default_int_handlerへ飛ばす．
 */

#define TOPPERS_MACRO_ONLY
#define TOPPERS_ASM_MACRO
#define UINT_C(val)     (val) /* uint_t型の定数を作るマクロ */
#define ULONG_C(val)    (val) /* ulong_t型の定数を作るマクロ */
#define CAST(type, val) (val) /* 型キャストを行うマクロ */

#include "kernel_impl.h"
#include "offset.h"
#include "target_asm.inc"

/*
 *  トラップベクタテーブル(256byteアライン)
 *
 *  Hazard3のローカル割込みはMSI/MTI/MEIのみだが，PolarFire版と同じ
 *  形のテーブルを置く（mtvecのVECTOREDモード）．
 */
    AO_PUSH    /* 設定の保存 */
    AO_NORVC
    ABALIGN(256)
    AGLOBAL(trap_vector_table)
ALABEL(trap_vector_table)
    j core_exc_entry  /*  0:Synchronous Exception */
    j core_int_entry  /*  1:SSI */
    j core_exc_entry  /*  2:Reserved */
    j core_int_entry  /*  3:MSI */
    j core_exc_entry  /*  4:Reserved */
    j core_int_entry  /*  5:STI */
    j core_exc_entry  /*  6:Reserved */
    j core_int_entry  /*  7:MTI */
    j core_exc_entry  /*  8:Reserved */
    j core_int_entry  /*  9:SEI */
    j core_exc_entry  /* 10:Reserved */
    j core_int_entry  /* 11:MEI */
    j core_exc_entry  /* 12:Reserved */
    j core_exc_entry  /* 13:Reserved */
    j core_exc_entry  /* 14:Reserved */
    j core_exc_entry  /* 15:Reserved */
    AO_POP     /* 設定の復帰 */

/*
 *  割込み入口処理での割込みコントローラ操作
 *
 *  受け付けた割込みに応じて割込み優先度マスクをセットする
 *  以下の内容をレジスタに入れてリターンする
 *   a0:割込み番号（0ならSpurious割込み）
 *   a1:割込み前の優先度(内部表現)＝割込み前のmeicontextの値
 *   a2:割込みハンドラアドレス
 */
    ATEXT
    AALIGN(4)
    AGLOBAL(irc_begin_int)
ALABEL(irc_begin_int)
    csrr t0, mcause
    andi t0, t0, 0x3f  /* 割込み番号のみマスク */

    /*
     *  MEIなら分岐
     */
    li  t1, MCAUSE_MEI
    beq t0, t1, do_mei

    /*
     *  それ以外のローカル割込み（MTI/MSI等．mieで許可していないため
     *  通常は来ない）
     */
    li     t1, 0x10000             /* ローカル割込みの割込み番号の生成 */
    add    a0, t0, t1              /*   (0x10000 | mcause) */
    csrr   a1, XH3IRQ_MEICONTEXT   /* meicontextは変更しない（復元用） */
    la     a2, default_int_handler /* 割込みハンドラアドレス */
    jr     ra

    /*
     *  Machine External interruptの場合
     */
ALABEL(do_mei)
    /*
     *  割込み前のmeicontextを保存（a1．irc_end_intで書き戻す）
     */
    csrr   a1, XH3IRQ_MEICONTEXT

    /*
     *  割込み番号の取得（UPDATE=1：meicontext.PREEMPTを受け付けた
     *  割込みの優先度+1へ自動更新する）
     */
    csrrsi t0, XH3IRQ_MEINEXT, XH3IRQ_MEINEXT_UPDATE
    bgez   t0, do_mei_1

    /*
     *  NOIRQ（bit31）ならSpurious割込みとする．優先度スタックを
     *  ソフトウェアでpopして（後述），割込み前の優先度マスクに戻す．
     */
    jal    t2, xh3irq_pop_priority
    li     a0, 0
    jr     ra

ALABEL(do_mei_1)
    /*
     *  meinextはIRQ番号を2ビット左シフトして返す（bit10:2）
     */
    srli   t0, t0, 2
    addi   a0, t0, 1               /* INTNO = IRQ番号 + 1 */

    /*
     *  割込みハンドラアドレスの取得
     */
    la     t1, inh_table           /* 割込みハンドラテーブルの読込み */
    slli   t2, a0, INTPTR_T_SHIFT
    add    t2, t1, t2
    lx     a2, 0(t2)               /* 割込みハンドラの番地 */
    jr     ra

/*
 *  割込み出口処理での割込みコントローラ操作
 *
 *  a0:割込み番号
 *  a1:割込み前の優先度＝割込み前のmeicontextの値
 */
    ATEXT
    AALIGN(4)
    AGLOBAL(irc_end_int)
ALABEL(irc_end_int)
    /*
     *  0ならSpurious割込みとして何もしない（irc_begin_intで復元済み）
     */
    beqz   a0, irc_end_int_1

    /*
     *  ローカル割込み（0x10000|mcause）はMEIではなく，入口で優先度
     *  スタックがpushされていないため，popしない
     */
    li     t0, 0x10000
    bgeu   a0, t0, irc_end_int_1

    /*
     *  割込み前の優先度マスクを復元する（優先度スタックのpop）
     */
    jal    t2, xh3irq_pop_priority

ALABEL(irc_end_int_1)
    jr ra

/*
 *  優先度スタックのソフトウェアpop
 *
 *  Hazard3はMEIトラップの「入口」で優先度スタックをpushする
 *  （PREEMPT←受け付けた割込みの優先度+1，PPREEMPT←旧PREEMPT，
 *  PPPREEMPT←旧PPREEMPT，MRETEIRQ←1）．本来はmret時のハードウェア
 *  によるpop（MRETEIRQ=1のとき）と対になっているが，ASP3はディス
 *  パッチ等でmretを通らずにタスクへ戻る経路があるため，popをソフト
 *  ウェアで行うことに一本化する（MRETEIRQは0にしてハードウェアの
 *  popを抑止する）：
 *    PREEMPT←PPREEMPT，PPREEMPT←PPPREEMPT，PPPREEMPT←0，
 *    MRETEIRQ←0（IRQ・NOIRQフィールドはハードウェアに影響しない）
 *
 *  入口：t2＝戻り番地（jal t2で呼び出す）／破壊：t0, t1, a1
 */
ALABEL(xh3irq_pop_priority)
    csrr   t0, XH3IRQ_MEICONTEXT
    srli   t1, t0, 8               /* PPREEMPT（27:24）→PREEMPT位置（20:16） */
    li     a1, 0x000f0000
    and    t1, t1, a1
    srli   t0, t0, 4               /* PPPREEMPT（31:28）→PPREEMPT位置（27:24） */
    li     a1, 0x0f000000
    and    t0, t0, a1
    or     t0, t0, t1
    csrw   XH3IRQ_MEICONTEXT, t0
    jr     t2

/*
 *  CPU例外発生前の割込み優先度の取得
 *
 *  CPU例外の発生で割込み優先度が変わることはないため，現在の割込み優先
 *  度を外部表現（負値）に変換して返す．内部表現 = PREEMPT - 1．
 */
    ATEXT
    AALIGN(4)
    AGLOBAL(irc_get_intpri)
ALABEL(irc_get_intpri)
    csrr   t0, XH3IRQ_MEICONTEXT
    srli   t0, t0, XH3IRQ_MEICONTEXT_PREEMPT_SHIFT
    andi   t0, t0, 0x1f            /* PREEMPTフィールド（5ビット） */
    addi   t0, t0, -1              /* 内部表現に変換 */
    neg    a0, t0                  /* 外部表現に変換 */
    jr     ra

/*
 *  CPU例外ハンドラ呼出し前の割込みコントローラ操作
 *
 *  irc_end_excで用いる情報（CPU例外発生前のmeicontextの値）を返す．
 */
    ATEXT
    AALIGN(4)
    AGLOBAL(irc_begin_exc)
ALABEL(irc_begin_exc)
    csrr   a0, XH3IRQ_MEICONTEXT
    jr     ra

/*
 *  CPU例外ハンドラ呼出し後の割込みコントローラ操作
 */
    ATEXT
    AALIGN(4)
    AGLOBAL(irc_end_exc)
ALABEL(irc_end_exc)
    andi   a0, a0, -2              /* MRETEIRQをクリア（irc_end_int参照） */
    csrw   XH3IRQ_MEICONTEXT, a0
    jr     ra
