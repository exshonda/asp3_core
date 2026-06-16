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
 *  sil.h のターゲット依存部（ARM MPS3-AN547 用）
 *
 *  このインクルードファイルは，sil.h の先頭でインクルードされる．
 */

#ifndef TOPPERS_TARGET_SIL_H
#define TOPPERS_TARGET_SIL_H

/*
 *  プロセッサのインディアン定義（Cortex-M55，リトルエンディアン）
 */
#define SIL_ENDIAN_LITTLE

/*
 *  割込み優先度のビット幅
 *
 *  実機 AN547（SSE-300）の NVIC は 3 ビット（8 レベル）を実装する．QEMU は
 *  8 ビットを実装するが，3 ビットで設定した割込み優先度は上位 3 ビットに
 *  配置されるため，QEMU・実機の双方で正しく動作する．
 */
#define TBITW_IPRI     3

/*
 *  プロセッサで共通な定義
 */
#include <core_sil.h>
#ifndef TOPPERS_MACRO_ONLY
#include <core_insn.h>
#endif /* TOPPERS_MACRO_ONLY */

/*
 *  一般共通レジスタに対するビット操作関数
 *
 *  CMSDK/SSE-300 にはアトミックなビット操作機構（RP2350 のような
 *  set/clear エイリアス）が無いため，読出し・修正・書込みで実現する．
 *  コア依存部（core_kernel_impl.c の FPU/MPU 設定）が sil_orw / sil_andw
 *  を使用するため，これらの定義は必須である．呼出し側で割込みロック等の
 *  排他制御を行うこと．
 */
#ifndef TOPPERS_MACRO_ONLY
#define sil_orb(mem, val)   sil_wrb_mem((mem), (uint8_t)(sil_reb_mem(mem) | (val)))
#define sil_andb(mem, val)  sil_wrb_mem((mem), (uint8_t)(sil_reb_mem(mem) & (val)))
#define sil_clrb(mem, val)  sil_wrb_mem((mem), (uint8_t)(sil_reb_mem(mem) & ~(val)))
#define sil_mskb(mem, val, msk) \
				sil_wrb_mem((mem), (uint8_t)((sil_reb_mem(mem) & ~(msk)) | ((val) & (msk))))

#define sil_orh(mem, val)   sil_wrh_mem((mem), (uint16_t)(sil_reh_mem(mem) | (val)))
#define sil_andh(mem, val)  sil_wrh_mem((mem), (uint16_t)(sil_reh_mem(mem) & (val)))
#define sil_clrh(mem, val)  sil_wrh_mem((mem), (uint16_t)(sil_reh_mem(mem) & ~(val)))
#define sil_mskh(mem, val, msk) \
				sil_wrh_mem((mem), (uint16_t)((sil_reh_mem(mem) & ~(msk)) | ((val) & (msk))))

#define sil_orw(mem, val)   sil_wrw_mem((mem), (uint32_t)(sil_rew_mem(mem) | (val)))
#define sil_andw(mem, val)  sil_wrw_mem((mem), (uint32_t)(sil_rew_mem(mem) & (val)))
#define sil_clrw(mem, val)  sil_wrw_mem((mem), (uint32_t)(sil_rew_mem(mem) & ~(val)))
#define sil_mskw(mem, val, msk) \
				sil_wrw_mem((mem), (uint32_t)((sil_rew_mem(mem) & ~(msk)) | ((val) & (msk))))
#endif /* TOPPERS_MACRO_ONLY */

/*
 *  ボード（CMSDK/SSE-300）のハードウェア資源定義
 */
#include "mps3_an547.h"

/*
 *  SIL スピンロック（単一プロセッサ版）
 *
 *  ASP3 は単一プロセッサであり，スピンロックは割込みロックと等価である．
 */
#define SIL_LOC_SPN()  SIL_LOC_INT()
#define SIL_UNL_SPN()  SIL_UNL_INT()

#endif /* TOPPERS_TARGET_SIL_H */
