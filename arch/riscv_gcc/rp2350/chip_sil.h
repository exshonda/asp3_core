/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアをTOPPERSライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソ
 *  フトウェアは無保証で提供される．
 */

/*
 *    sil.hのチップ依存部（RP2350 RISC-V（Hazard3）用）
 *
 *  ビット操作マクロは，RP2350のバスファブリックが提供するアトミック
 *  アクセスエイリアス（+0x1000:XOR／+0x2000:SET／+0x3000:CLR）を使用
 *  する（ARM版RP2350チップ依存部と同一．ISA非依存のバス機能）．
 */

#ifndef TOPPERS_CHIP_SIL_H
#define TOPPERS_CHIP_SIL_H

/*
 *  コアで共通な定義
 */
#include "core_sil.h"

/*
 *  ビット操作（アトミックアクセスエイリアス使用）
 */
#define sil_mskb( mem, val, msk ) sil_wrb_mem((uint8_t *)((uintptr_t)(mem) + 0x1000), ((sil_reb_mem(mem) ^ (val)) & (msk)))
#define sil_orb( mem, val )  sil_wrb_mem((uint8_t *)((uintptr_t)(mem) + 0x2000), val)
#define sil_andb( mem, val ) sil_wrb_mem(mem, sil_reb_mem(mem) & val)
#define sil_clrb( mem, val ) sil_wrb_mem((uint8_t *)((uintptr_t)(mem) + 0x3000), val)
#define sil_mskh( mem, val, msk ) sil_wrh_mem((uint16_t *)((uintptr_t)(mem) + 0x1000), ((sil_reh_mem(mem) ^ (val)) & (msk)))
#define sil_orh( mem, val )  sil_wrh_mem((uint16_t *)((uintptr_t)(mem) + 0x2000), val)
#define sil_andh( mem, val ) sil_wrh_mem(mem, sil_reh_mem(mem) & val)
#define sil_clrh( mem, val ) sil_wrh_mem((uint16_t *)((uintptr_t)(mem) + 0x3000), val)
#define sil_mskw( mem, val, msk ) sil_wrw_mem((uint32_t *)((uintptr_t)(mem) + 0x1000), ((sil_rew_mem(mem) ^ (val)) & (msk)))
#define sil_orw( mem, val )  sil_wrw_mem((uint32_t *)((uintptr_t)(mem) + 0x2000), val)
#define sil_andw( mem, val ) sil_wrw_mem(mem, sil_rew_mem(mem) & val)
#define sil_clrw( mem, val ) sil_wrw_mem((uint32_t *)((uintptr_t)(mem) + 0x3000), val)

#include "RP2350.h"

#endif /* TOPPERS_CHIP_SIL_H */
