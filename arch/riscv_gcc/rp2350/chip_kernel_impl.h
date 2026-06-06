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
 *    kernel_impl.hのチップ依存部（RP2350 RISC-V（Hazard3）用）
 *
 *  このヘッダファイルは，target_kernel_impl.h（または，そこからインク
 *  ルードされるファイル）のみからインクルードされる．他のファイルから
 *  直接インクルードしてはならない．
 *
 *  PolarFire SoC依存部（PLIC）をXh3irq用に置き換えたもの．カーネルは
 *  core0（Hazard3）のみで実行する（core1はbootROMが待機させる）．
 */

#ifndef TOPPERS_CHIP_KERNEL_IMPL_H
#define TOPPERS_CHIP_KERNEL_IMPL_H

/*
 *  RP2350のハードウェア資源の定義（ARM版チップ依存部と共有）
 */
#include "RP2350.h"

/*
 *  RISC-Vの定義
 */
#include "riscv.h"

/*
 *  ブートハート（RISC-VイメージはbootROMがcore0で起動する）
 */
#define TOPPERS_BOOT_HARTID    0

/*
 *  デフォルトの非タスクコンテキスト用のスタック領域の定義
 */
#ifndef DEFAULT_ISTKSZ
#define DEFAULT_ISTKSZ  0x1000U    /* 4KB */
#endif /* DEFAULT_ISTKSZ */

/*
 *  割込み番号の最小値と最大値
 *
 *  ASP3の割込み番号INTNO = RP2350のIRQ番号 + 1（xh3irq_kernel_impl.h
 *  参照）．
 */
#define TMIN_INTNO  UINT_C(1)
#define TMAX_INTNO  XH3IRQ_TNUM_IRQ

/*
 *  割込みハンドラ番号の最大値
 */
#define TMAX_INHNO  XH3IRQ_TNUM_IRQ

/* 外部表現への変換 */
#define EXT_IPM(pri)  (-(PRI)(pri))

/* 内部表現への変換 */
#define INT_IPM(ipm)  ((uint_t)(-(ipm)))

/*
 *  割込み番号の範囲の判定
 */
#define VALID_INTNO(intno) \
  ((TMIN_INTNO <= (intno)) && ((intno) <= TMAX_INTNO))

/*
 *  割込み要求ラインのための標準的な初期化情報を生成する
 */
#define USE_INTINIB_TABLE

/*
 *  割込み要求ライン設定テーブルを生成する
 */
#define USE_INTCFG_TABLE

/*
 *  Xh3irq依存部
 */
#include "xh3irq_kernel_impl.h"

#ifndef TOPPERS_MACRO_ONLY

/*
 *  割込み属性の設定のチェック
 */
Inline bool_t
check_intno_cfg(INTNO intno)
{
	return(intcfg_table[intno] != 0U);
}

/*
 *  割込み優先度マスクの設定
 */
Inline void
t_set_ipm(PRI intpri)
{
	xh3irq_set_preempt(INT_IPM(intpri));
}

/*
 *  割込み優先度マスクの参照
 */
Inline PRI
t_get_ipm(void)
{
	return(EXT_IPM(xh3irq_get_preempt()));
}

/*
 *  割込み要求禁止フラグが操作できる割込み番号の範囲の判定
 */
#define VALID_INTNO_DISINT(intno)  VALID_INTNO(intno)

/*
 *  割込み要求禁止フラグのセット
 */
Inline void
disable_int(INTNO intno)
{
	xh3irq_disable_int(intno);
}

/*
 *  割込み要求禁止フラグのクリア
 */
Inline void
enable_int(INTNO intno)
{
	xh3irq_enable_int(intno);
}

/*
 *  割込み要求のチェック
 */
Inline bool_t
probe_int(INTNO intno)
{
	return(xh3irq_probe_int(intno));
}

/*
 *  割込み要求のクリア（clr_int用．meifaの強制ビットをクリアする）
 */
#define VALID_INTNO_CLRINT(intno)  VALID_INTNO(intno)

Inline bool_t
check_intno_clear(INTNO intno)
{
	return(true);
}

Inline void
clear_int(INTNO intno)
{
	xh3irq_clear_int(intno);
}

/*
 *  割込みの要求（ras_int用．meifaの強制ビットをセットする．
 *  meinextでの受付け時にハードウェアが自動クリアする）
 */
#define VALID_INTNO_RASINT(intno)  VALID_INTNO(intno)

Inline bool_t
check_intno_raise(INTNO intno)
{
	return(true);
}

Inline void
raise_int(INTNO intno)
{
	xh3irq_raise_int(intno);
}

/*
 *  チップ依存の初期化
 */
extern void chip_initialize(void);

/*
 *  チップ依存の終了処理
 */
extern void chip_terminate(void);

#endif /* TOPPERS_MACRO_ONLY */
#endif /* TOPPERS_CHIP_KERNEL_IMPL_H */
