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
 *  $Id: core_kernel_impl.h 335 2023-04-18 10:50:40Z ertl-honda $
 */

/*
 *     kernel_impl.hのコア依存部（RISC-V用）
 *
 *  このヘッダファイルは，target_kernel_impl.h（または，そこからインク
 *  ルードされるファイル）のみからインクルードされる．他のファイルから
 *  直接インクルードしてはならない．
 *
 *  TOPPERS/FMP3のRISC-V依存部をASP3（シングルプロセッサ）用に変換した
 *  もの．TPCB（プロセッサ管理ブロックのターゲット依存部）を廃止し，例
 *  外ネストカウントをグローバル変数化した．ロック（ジャイアントロック・
 *  スピンロック）関連の定義は削除した．
 */

#ifndef TOPPERS_CORE_KERNEL_IMPL_H
#define TOPPERS_CORE_KERNEL_IMPL_H

#include "riscv.h"

/*
 *  サポート範囲のチェック
 */
#ifndef __riscv_xlen
    #error define __riscv_xlen
#elif __riscv_xlen == 128
    #error __riscv_xlen == 128 is not support
#endif /* __riscv_xlen */

/*
 *  エラーチェック方法の指定
 */
#ifndef __riscv_32e
#define CHECK_STKSZ_ALIGN    16    /* スタックサイズのアライン単位 */
#else /* __riscv_32e */
#define CHECK_STKSZ_ALIGN     4    /* スタックサイズのアライン単位 */
#endif /* !__riscv_32e */

#if __riscv_xlen == 64
#define CHECK_INTPTR_ALIGN    8    /* intptr_t型の変数のアライン単位 */
#elif __riscv_xlen == 32
#define CHECK_INTPTR_ALIGN    4    /* intptr_t型の変数のアライン単位 */
#endif /* __riscv_xlen == 64 */

#define CHECK_INTPTR_NONNULL       /* intptr_t型の変数の非NULLチェック */
#define CHECK_FUNC_ALIGN      2    /* 関数のアライン単位 */
#define CHECK_FUNC_NONNULL         /* 関数の非NULLチェック */
#define CHECK_STACK_ALIGN    16    /* スタック領域のアライン単位 */
#define CHECK_STACK_NONNULL        /* スタック領域の非NULLチェック */
#define CHECK_MPF_ALIGN       4    /* 固定長メモリプール領域のアライン単位 */
#define CHECK_MPF_NONNULL          /* 固定長メモリプール領域の非NULLチェック */
#define CHECK_MPK_ALIGN       4    /* カーネルメモリプール領域のアライン単位 */
#define CHECK_MPK_NONNULL          /* カーネルメモリプール領域の非NULLチェック  */
#define CHECK_MB_ALIGN        4    /* 管理領域のアライン単位 */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  コンテキストの参照
 *
 *  RISC-V依存部では，タスクコンテキストと非タスクコンテキストの両方を
 *  マシンモードで動作させるため，実行中のコンテキストをプロセッサモー
 *  ドで判断することができない．そのため，割込みハンドラ／CPU例外ハンド
 *  ラのネスト段数（これを，例外ネストカウントと呼ぶ）を管理し，例外ネス
 *  トカウントが0の時にタスクコンテキスト，0より大きい場合に非タスクコン
 *  テキストであると判断する．
 */
extern uint32_t excpt_nest_count;      /* 例外ネストカウント */

Inline bool_t
sense_context(void)
{
    return(excpt_nest_count > 0U);
}

#endif /* TOPPERS_MACRO_ONLY */

/*
 *  TOPPERS標準割込み処理モデルの実現
 *
 *  RISC-Vコア依存部では，mstatus.MIEによりCPUロック状態を実現する．
 *  カーネル管理外の割込みはサポートしない．
 *
 *  TOPPERS標準割込み処理モデルの中で，割込み優先度マスクと割込み要求禁
 *  止フラグに関しては，割込みコントローラによって実現方法が異なるため，
 *  RISC-Vコア依存部では扱わない．
 */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  CPUロック状態への遷移
 */
Inline void
lock_cpu(void)
{
    riscv_clear_mstatus_mie();
    riscv_dmb();
    /*
     *  メモリ参照が，この関数を超えて最適化されることを抑止
     */
    RISCV_MEMORY_CHANGED;
}

/*
 *  CPUロック状態への移行（ディスパッチできる状態）
 */
#define lock_cpu_dsp()    lock_cpu()

/*
 *  CPUロック状態の解除
 */
Inline void
unlock_cpu(void)
{
    /*
     *  メモリ参照が，この関数を超えて最適化されることを抑止
     */
    RISCV_MEMORY_CHANGED;
    riscv_dmb();
    riscv_set_mstatus_mie();
}

/*
 *  CPUロック状態の解除（ディスパッチできる状態）
 */
#define unlock_cpu_dsp()  unlock_cpu()

/*
 *  CPUロック状態の参照
 */
Inline bool_t
sense_lock(void)
{
    return((riscv_read_mstatus() & MSTATUS_MIE) == 0U);
}

#ifndef OMIT_DELAY_FOR_INTERRUPT
/*
 *  割込みを受け付けるための遅延処理
 */
Inline void
delay_for_interrupt(void)
{
}
#endif /* OMIT_DELAY_FOR_INTERRUPT */

/*
 *  非タスクコンテキスト用のスタック初期値
 */
#define TOPPERS_ISTKPT(istk, istksz)  ((STK_T *)((char *)(istk) + (istksz)))

/*
 *  タスクコンテキストブロックの定義
 */
typedef struct task_context_block {
    void    *sp;    /* スタックポインタ */
    FP      pc;     /* 実行再開番地 */
} TSKCTXB;

/*
 *  タスクディスパッチャ
 */

/*
 *  最高優先順位タスクへのディスパッチ（core_support.S）
 *
 *  dispatchは，タスクコンテキストから呼び出されたサービスコール処理か
 *  ら呼び出すべきもので，タスクコンテキスト・CPUロック状態・ディスパッ
 *  チ許可状態・（モデル上の）割込み優先度マスク全解除状態で呼び出さな
 *  ければならない．
 */
extern void dispatch(void);

/*
 *  非タスクコンテキストからのディスパッチ要求
 */
#define request_dispatch_retint()

/*
 *  ディスパッチャの動作開始（core_support.S）
 *
 *  start_dispatchは，カーネル起動時に呼び出すべきもので，すべての割込
 *  みを禁止した状態（割込みロック状態と同等の状態）で呼び出さなければ
 *  ならない．
 */
extern void start_dispatch(void) NoReturn;

/*
 *  現在のコンテキストを捨ててディスパッチ（core_support.S）
 *
 *  exit_and_dispatchは，ext_tskから呼び出すべきもので，タスクコンテキ
 *  スト・CPUロック状態・ディスパッチ許可状態・（モデル上の）割込み優先
 *  度マスク全解除状態で呼び出さなければならない．
 */
extern void exit_and_dispatch(void) NoReturn;

/*
 *  カーネルの終了処理の呼出し（core_support.S）
 *
 *  call_exit_kernelは，カーネルの終了時に呼び出すべきもので，非タスク
 *  コンテキストに切り換えて，カーネルの終了処理（exit_kernel）を呼び出
 *  す．
 */
extern void call_exit_kernel(void) NoReturn;

/*
 *  メモリバリア操作
 */
Inline void
memory_barrier(void)
{
    riscv_dmb();
}

/*
 *  タスクコンテキストの初期化
 *
 *  タスクが休止状態から実行できる状態に遷移する時に呼ばれる．この時点
 *  でスタック領域を使ってはならない．
 *
 *  activate_contextを，インライン関数ではなくマクロ定義としているのは，
 *  この時点ではTCBが定義されていないためである．
 */
extern void start_r(void);

#define activate_context(p_tcb)                                      \
{                                                                    \
    (p_tcb)->tskctxb.sp = (void *)((char *)((p_tcb)->p_tinib->stk)   \
                                        + (p_tcb)->p_tinib->stksz);  \
    (p_tcb)->tskctxb.pc = (FP) start_r;                              \
}

/*
 *  標準の割込み管理機能の初期化処理を用いない
 */
#define OMIT_INITIALIZE_INTERRUPT

/*
 *  割込みハンドラテーブル（kernel_cfg.c）
 */
extern const FP inh_table[TMAX_INHNO + 1];

/*
 *  割込み要求ライン設定テーブル（kernel_cfg.c）
 *
 *  割込み要求ラインに対して割込み属性が設定されていれば1，設定されてい
 *  なければ0を保持するテーブル．
 */
#ifdef USE_INTCFG_TABLE
extern const uint8_t intcfg_table[TMAX_INTNO + 1];
#endif /* USE_INTCFG_TABLE */

/*
 *  標準の例外管理機能の初期化処理を用いない
 */
#define OMIT_INITIALIZE_EXCEPTION

/*
 *  CPU例外ハンドラ番号の最大値
 */
#define TMAX_EXCNO      15

/*
 *  CPU例外ハンドラテーブル（kernel_cfg.c）
 */
extern const FP exc_table[TMAX_EXCNO + 1];

/*
 *  CPU例外ハンドラの初期化
 */
Inline void
initialize_exception(void)
{
}

/*
 *  CPU例外の発生した時のコンテキストの参照
 *
 *  CPU例外の発生した時のコンテキストが，タスクコンテキストの時にfalse，
 *  そうでない時にtrueを返す．
 */
Inline bool_t
exc_sense_context(void *p_excinf)
{
    return(((T_EXCINF *)(p_excinf))->exncnt != 0U);
}

/*
 *  CPU例外の発生した時の割込み優先度マスクの参照
 *
 *  この関数は，CPU例外がタスクコンテキストで発生した場合にのみ呼び出
 *  される．そのため，CPU例外が非タスクコンテキストで発生した場合には，
 *  正しい値を返す必要がない．
 */
Inline PRI
exc_get_intpri(void *p_excinf)
{
    return((PRI)(((T_EXCINF *)(p_excinf))->intpri));
}

/*
 *  CPUロック状態または全割込みロック状態かの参照
 */
Inline bool_t
exc_sense_lock(void *p_excinf)
{
    return(((((T_EXCINF *)(p_excinf))->mstatus) & MSTATUS_MPIE) == 0U);
}

/*
 *  CPU例外の発生した時のコンテキストと割込みのマスク状態の参照
 *
 *  CPU例外の発生した時のシステム状態が，カーネル実行中でなく，タスクコ
 *  ンテキストであり，全割込みロック状態でなく，CPUロック状態でなく，割
 *  込み優先度マスク全解除状態である時にtrue，そうでない時にfalseを返す
 *  （CPU例外がカーネル管理外の割込み処理中で発生した場合にもfalseを返
 *  す）．
 *
 */
Inline bool_t
exc_sense_intmask(void *p_excinf)
{
    return(!exc_sense_context(p_excinf)
                && exc_get_intpri(p_excinf) == TIPM_ENAALL
                && !exc_sense_lock(p_excinf));
}

/*
 *  コア依存の初期化
 */
extern void core_initialize(void);

/*
 *  コア依存の終了処理
 */
extern void core_terminate(void);

/*
 *  CPU例外の発生状況のログ出力
 */
#ifndef OMIT_XLOG_SYS
extern void xlog_sys(void *p_excinf);
#endif /* OMIT_XLOG_SYS */

/*
 *  未定義の割込みが入った場合の処理
 */
extern void default_int_handler(INTNO intno);

/*
 *  未定義の例外が入った場合の処理
 */
extern void default_exc_handler(void *p_excinf, EXCNO excno);

#endif /* TOPPERS_MACRO_ONLY */

/*
 *  整合性検査のための定義
 *
 *  保存されているスタックポインタが，16バイト境界にアラインし，スタッ
 *  ク上を指しているかをチェックする．
 *
 *  VALID_TSKCTXBを，インライン関数ではなくマクロ定義としているのは，
 *  この時点ではTCBが定義されていないためである．
 */
#define VALID_TSKCTXB(p_tskctxb, p_tcb)                             \
            ((((uintptr_t)((p_tskctxb)->sp) & 0x0fU) == 0U)         \
                && on_sstack((p_tskctxb)->sp, 0, (p_tcb)->p_tinib))

#endif /* TOPPERS_CORE_KERNEL_IMPL_H */
