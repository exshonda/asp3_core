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
 *  システムサービスのターゲット依存部（ARM MPS2-AN521 用）
 */
#ifndef TOPPERS_TARGET_SYSSVC_H
#define TOPPERS_TARGET_SYSSVC_H

#ifdef TOPPERS_OMIT_TECS

/*
 *  起動メッセージのターゲットシステム名
 */
#define TARGET_NAME "ARM MPS2-AN521"

/*
 *  システムログの低レベル出力のための文字出力
 */
extern void target_fput_log(char c);

/*
 *  低レベル出力で使用する SIO ポート
 */
#define SIOPID_FPUT 1

#endif /* TOPPERS_OMIT_TECS */

#endif /* TOPPERS_TARGET_SYSSVC_H */
