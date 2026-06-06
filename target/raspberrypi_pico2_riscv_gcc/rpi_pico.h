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
 *  微少時間待ちのための定義（sil_dly_nse用）
 *
 *  実機のdlynseテストで較正（2026-06-06）：呼出オーバヘッドの実測は
 *  46ns，1ループの実測はビルド（XIPフェッチの整列）により13.3〜20.5ns
 *  で変動した．「実測≥要求」を常に満たすため，理論下限
 *  （呼出≈6サイクル=40ns・ループ2サイクル=13.3ns @150MHz）を
 *  下回らない値に設定する．
 */
#define SIL_DLY_TIM1    40
#define SIL_DLY_TIM2    13

#endif /* TOPPERS_RPI_PICO_H */
