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
 */

/*
 *		PolarFire SoC Icicle Kit（QEMU microchip-icicle-kit）の
 *		ハードウェア資源の定義
 */
#ifndef POLARFIRE_SOC_KIT_H
#define POLARFIRE_SOC_KIT_H

/*
 *  起動メッセージのターゲット名
 */
#ifdef TOPPERS_USE_QEMU
#define TARGET_NAME  "PolarFire SoC Icicle Kit(QEMU) <U54, RISC-V>"
#elif defined(POLARFIRE_BOARD_DISCOVERY)  /* 実機 Discovery Kit */
#define TARGET_NAME  "PolarFire SoC Discovery Kit <U54, RISC-V>"
#else  /* 実機 Icicle Kit */
#define TARGET_NAME  "PolarFire SoC FPGA Icicle Kit <U54, RISC-V>"
#endif /* TOPPERS_USE_QEMU */

/*
 *  コアクロック周波数（MHz）
 */
#define CORE_CLK_MHZ  600

/*
 *  Machine Timer（mtime）の周波数（MHz）
 *
 *  実機・QEMU（clint-timebase-frequency既定値）とも1MHz．
 */
#define MTIMER_FREQ_MHZ  1

/*
 *  コンソールに使用するUART
 *
 *  Icicle Kit / QEMU は MMUART0，Discovery Kit は MMUART1 が
 *  FlashPro5(FT4232 if1) のコンソールに配線されている（実機確認済み．
 *  TTSP3_HOWTO.md 参照）．ボード選択は target.cmake の POLARFIRE_DISCOVERY．
 */
#if defined(POLARFIRE_BOARD_DISCOVERY)
#define USE_UART1
#else
#define USE_UART0
#endif

/*
 *  微少時間待ちのための定義（本来はSILのターゲット依存部）
 *
 *  値はFMP3のIcicle Kit実機向けのもの．QEMUでは命令実行時間が実時間と
 *  対応しないため，この値に意味はない（実機対応時に dlynse で再較正
 *  すること）．
 */
#define SIL_DLY_TIM1    70
#define SIL_DLY_TIM2    44

#endif /* POLARFIRE_SOC_KIT_H */
