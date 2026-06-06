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
 *  RaspberryPi Pico2（RISC-V）ボード依存の定義
 */

#ifndef TOPPERS_RPI_PICO_H
#define TOPPERS_RPI_PICO_H

/*
 *  コアクロック周波数（clk_sys＝150MHz．hardware_init_hookで設定）
 */
#define CPU_CLOCK_HZ    150000000
#define CORE_CLK_MHZ    150

/*
 *  微少時間待ちのための定義（sil_dly_nse用．未較正＝ARM版の値を初期値
 *  とする．test/のdlynseテストで較正すること）
 */
#define SIL_DLY_TIM1    46
#define SIL_DLY_TIM2    33

#endif /* TOPPERS_RPI_PICO_H */
