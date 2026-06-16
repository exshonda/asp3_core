/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2005-2016 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 * 
 *  上記著作権者は，以下の(1)～(4)の条件を満たす場合に限り，本ソフトウェ
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
 */

/*
 * ターゲット依存モジュール（RaspberryPi Pico2用）
 */
#include "kernel_impl.h"
#include "target_syssvc.h"
#include <sil.h>
#ifdef TOPPERS_OMIT_TECS
#include "chip_serial.h"
#endif

/*
 * エラー時の処理
 */
extern void Error_Handler(void);

extern volatile int image_def_block;

/*
 * 起動時のハードウェア初期化処理
 */
void hardware_init_hook(void)
{
    if (image_def_block) ; /* Enforce image_def_block to be linked */

    /* Reset everything but the fundamental parts */
    sil_orw(RP2350_RESETS_RESET,
            ~(RP2350_RESETS_RESET_PLL_SYS | RP2350_RESETS_RESET_PADS_QSPI | RP2350_RESETS_RESET_IO_QSPI));
    sil_clrw(RP2350_RESETS_RESET, ~(RP2350_RESETS_RESET_PLL_SYS | RP2350_RESETS_RESET_PADS_QSPI | RP2350_RESETS_RESET_IO_QSPI));

    /*
     * +------------+    +-------+  1500MHz  +-----+  300MHz  +-----+
     * | XOSC 12MHz | -> | * 125 | --------> | / 5 | -------> | / 2 | -> 150MHz
     * +------------+    +-------+           +-----+          +-----+
     */
    if ((sil_rew_mem(RP2350_XOSC_STATUS) & RP2350_XOSC_STATUS_STABLE) == 0) {
        /* XOSC is inactive. Activate it first */
        sil_wrw_mem(RP2350_XOSC_CTRL, RP2350_XOSC_CTRL_FREQ_RANGE_MAGIC);
        sil_wrw_mem(RP2350_XOSC_STARTUP, 47); /* 1ms delay. refer to the datasheet */
        sil_orw(RP2350_XOSC_CTRL, RP2350_XOSC_CTRL_ENABLE_MAGIC); /* Enable OSC */
        while ((sil_rew_mem(RP2350_XOSC_STATUS) & RP2350_XOSC_STATUS_STABLE) == 0) ; /* Wait to be stable */
        /*
         * Some chip crashes if we switch clocks to XOSC here (hardware issue ?).
         * We assume this is the first power-on reset because XOSC was not active.
         * All the clocks should be derived from ROSC, and we safely reset the PLL.
         */
    } else {
        /* XOSC is already active. Switch clocks */
        /* XTAL -> clk_ref (glitchless) */
        sil_wrw_mem(RP2350_CLOCKS_CLK_REF_CTRL, RP2350_CLOCKS_CLK_REF_CTRL_SRC_XOSC);
        sil_wrw_mem(RP2350_CLOCKS_CLK_REF_DIV, 1 << 16);
        while (sil_rew_mem(RP2350_CLOCKS_CLK_REF_SELECTED) != (1 << RP2350_CLOCKS_CLK_REF_CTRL_SRC_XOSC)) ;

        /* clk_ref -> clk_sys (glitchless) */
        sil_wrw_mem(RP2350_CLOCKS_CLK_SYS_CTRL, RP2350_CLOCKS_CLK_SYS_CTRL_SRC_REF);
        while (sil_rew_mem(RP2350_CLOCKS_CLK_SYS_SELECTED) != (1 << RP2350_CLOCKS_CLK_SYS_CTRL_SRC_REF)) ;
    }

    /* Reset PLL */
    sil_orw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_PLL_SYS);
    sil_clrw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_PLL_SYS);
    while ((sil_rew_mem(RP2350_RESETS_RESET_DONE) & RP2350_RESETS_RESET_PLL_SYS) == 0) ;

    sil_wrw_mem(RP2350_PLL_SYS_CS, RP2350_PLL_SYS_CS_REFDIV(1)); /* Set pre-divide 1 */
    sil_wrw_mem(RP2350_PLL_SYS_FBDIV_INT, 125); /* Set mult 125 (VCO = 1500MHz) */
    sil_clrw(RP2350_PLL_SYS_PWR, RP2350_PLL_SYS_PWR_PD|RP2350_PLL_SYS_PWR_VCOPD);
    while ((sil_rew_mem(RP2350_PLL_SYS_CS) & RP2350_PLL_SYS_CS_LOCK) == 0) ; /* Wait for locking */
    sil_wrw_mem(RP2350_PLL_SYS_PRIM, RP2350_PLL_SYS_PRIM_POSTDIV1(5)
                                   | RP2350_PLL_SYS_PRIM_POSTDIV2(2)); /* Set post-divide */
    sil_wrw_mem(RP2350_PLL_SYS_PWR, 0); /* Power up PLL */

    /* XOSC -> clk_ref (glitchless) */
    sil_wrw_mem(RP2350_CLOCKS_CLK_REF_CTRL, RP2350_CLOCKS_CLK_REF_CTRL_SRC_XOSC);
    sil_wrw_mem(RP2350_CLOCKS_CLK_REF_DIV, 1 << 16);
    while (sil_rew_mem(RP2350_CLOCKS_CLK_REF_SELECTED) != (1 << RP2350_CLOCKS_CLK_REF_CTRL_SRC_XOSC)) ;

    /* clk_ref -> clk_sys (glitchless) */
    sil_wrw_mem(RP2350_CLOCKS_CLK_SYS_CTRL, RP2350_CLOCKS_CLK_SYS_CTRL_SRC_REF);
    while (sil_rew_mem(RP2350_CLOCKS_CLK_SYS_SELECTED) != (1 << RP2350_CLOCKS_CLK_SYS_CTRL_SRC_REF)) ;

    /* Set clk_sys = pll_sys / 1 */
    sil_wrw_mem(RP2350_CLOCKS_CLK_SYS_DIV, 1 << 16);
    sil_wrw_mem(RP2350_CLOCKS_CLK_SYS_CTRL, RP2350_CLOCKS_CLK_SYS_CTRL_AUXSRC_PLL_SYS
                                          | RP2350_CLOCKS_CLK_SYS_CTRL_SRC_AUX);

    /*
     * TIMER0のtickソースの設定（RP2350固有）
     * RP2350ではTIMER0はTICKSブロックからtickの供給を受ける．
     * TICKSブロックを設定しないとTIMER0は歩進しない．
     * clk_ref(XOSC 12MHz)を12分周して1MHzのtickを生成する．
     */
    while ((sil_rew_mem(RP2350_RESETS_RESET_DONE) & RP2350_RESETS_RESET_TIMER0) == 0) ;
    sil_wrw_mem(RP2350_TIMER0_SOURCE, RP2350_TIMER0_SOURCE_TICK);
    sil_wrw_mem(RP2350_TICKS_TIMER0_CYCLES, 12);
    sil_wrw_mem(RP2350_TICKS_TIMER0_CTRL, RP2350_TICKS_TIMER0_CTRL_ENABLE);
    while ((sil_rew_mem(RP2350_TICKS_TIMER0_CTRL) & RP2350_TICKS_TIMER0_CTRL_RUNNING) == 0) ;

    /* TX:GP0(F2) RX:GP1(F2) */
    /* Reset GPIO */
    sil_orw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_IO_BANK0 | RP2350_RESETS_RESET_PADS_BANK0);
    sil_clrw(RP2350_RESETS_RESET, RP2350_RESETS_RESET_IO_BANK0 | RP2350_RESETS_RESET_PADS_BANK0);
    while ((sil_rew_mem(RP2350_RESETS_RESET_DONE)
            & (RP2350_RESETS_RESET_IO_BANK0 | RP2350_RESETS_RESET_PADS_BANK0))
           != (RP2350_RESETS_RESET_IO_BANK0 | RP2350_RESETS_RESET_PADS_BANK0)) ;
    sil_wrw_mem(RP2350_IO_BANK0_GPIO_CTRL(0), 2);
    sil_wrw_mem(RP2350_IO_BANK0_GPIO_CTRL(1), 2);
    /*
     * パッドの設定（RP2350固有）
     * RP2350ではリセット後にパッドがアイソレーション状態となるため，
     * ISOビットをクリアしないと信号が出力されない．
     */
    sil_orw(RP2350_PADS_BANK0_GPIO(0), RP2350_PADS_BANK0_GPIOX_IE);
    sil_clrw(RP2350_PADS_BANK0_GPIO(0), RP2350_PADS_BANK0_GPIOX_ISO | RP2350_PADS_BANK0_GPIOX_OD);
    sil_orw(RP2350_PADS_BANK0_GPIO(1), RP2350_PADS_BANK0_GPIOX_IE);
    sil_clrw(RP2350_PADS_BANK0_GPIO(1), RP2350_PADS_BANK0_GPIOX_ISO | RP2350_PADS_BANK0_GPIOX_OD);

#ifdef TOPPERS_FPU_ENABLE
    /*
     * FPUの有効化（CP10/CP11）
     * core_initialize()でもsil_orw()によりCPACRを設定するが，RP2350の
     * sil_orw()はアトミックアクセスエイリアス（+0x2000）を使用しており，
     * M33のPPB領域には適用されないため，ここで直接設定する．
     */
    sil_wrw_mem(RP2350_M33_CPACR, sil_rew_mem(RP2350_M33_CPACR) | CPACR_FPU_ENABLE);
#endif /* TOPPERS_FPU_ENABLE */
}

void software_init_hook(void)
{
    /* Initialize sio for fput */
#ifdef TOPPERS_OMIT_TECS
    sio_initialize(0);
    sio_opn_por(SIOPID_FPUT, 0);
#endif
}

#ifndef TOPPERS_OMIT_TECS
/*
 *  システムログの低レベル出力のための初期化
 *
 */
extern void tPutLogSIOPort_initialize(void);
#endif

/*
 * ターゲット依存部 初期化処理
 */
void target_initialize(void)
{
#ifdef TOPPERS_SAFEG_M
    /*
     *  【SAFEG】SAU の設定（RP2350 アドレス空間）。core_initialize() の
     *  ITNS 全 NS 化 / AIRCR.PRIS より前に NS 領域を確定する。
     *    R0: NSC(Secure gate veneer) 0x101FFE00..0x101FFFFF
     *    R1: NS code  0x10200000..0x103FFFFF
     *    R2: NS RAM   0x20040000..0x2007FFFF
     *  その他は Secure（SAU 有効時の既定）。
     *
     *  注意(RP2350 固有): RP2350 は ARM SAU に加え ACCESSCTRL ブロックで
     *  SRAM/ペリフェラルの Secure/NS アクセスを別途ゲートする。NS 側が NS RAM や
     *  必要ペリフェラルへアクセスするには ACCESSCTRL の NS 許可設定が要る可能性が高い
     *  （vanilla RP2350 ポートは Secure 単独のため未設定）。SAU のみで NS が
     *  起動しない場合は ACCESSCTRL(0x40060000 系)の NS マスク設定を追加すること。
     */
    sil_wrw_mem((uint32_t *)SAU_RNR, 0);
    sil_wrw_mem((uint32_t *)SAU_RBAR, 0x101FFE00);
    sil_wrw_mem((uint32_t *)SAU_RLAR,
                (0x101FFFFF & SAU_RLAR_LADDR_MASK) | SAU_RLAR_NSC | SAU_RLAR_ENABLE);
    sil_wrw_mem((uint32_t *)SAU_RNR, 1);
    sil_wrw_mem((uint32_t *)SAU_RBAR, 0x10200000);
    sil_wrw_mem((uint32_t *)SAU_RLAR,
                (0x103FFFFF & SAU_RLAR_LADDR_MASK) | SAU_RLAR_ENABLE);
    sil_wrw_mem((uint32_t *)SAU_RNR, 2);
    sil_wrw_mem((uint32_t *)SAU_RBAR, 0x20040000);
    sil_wrw_mem((uint32_t *)SAU_RLAR,
                (0x2007FFFF & SAU_RLAR_LADDR_MASK) | SAU_RLAR_ENABLE);
    sil_wrw_mem((uint32_t *)SAU_CTRL, SAU_CTRL_ENABLE);
#endif /* TOPPERS_SAFEG_M */

    /*
     *  コア依存部の初期化
     */
    core_initialize();
    /*
     *  SIOを初期化
     */
#ifndef TOPPERS_OMIT_TECS
    tPutLogSIOPort_initialize();
#endif /* TOPPERS_OMIT_TECS */
}

/*
 * ターゲット依存部 終了処理
 */
void target_exit(void)
{
    /* チップ依存部の終了処理 */
    core_terminate();
    while(1) ;
}

/*
 * エラー発生時の処理
 */
void Error_Handler(void)
{
    while (1) ;
}

/*
 *  デフォルトのsoftware_term_hook（weak定義）
 */
__attribute__((weak))
void software_term_hook(void)
{
}
