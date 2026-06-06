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
 *  ターゲット依存モジュール（RaspberryPi Pico2 RISC-V用）
 *
 *  hardware_init_hookの内容（RESETS／クロック／TICKS／PADSアイソレー
 *  ション解除／UART用GPIO）はARM版（target/raspberrypi_pico2_gcc）から
 *  の流用（すべてMMIO操作＝ISA非依存）．ARM版にあったFPU（CPACR）設定
 *  はHazard3にFPUが無いため削除．
 */
#include "kernel_impl.h"
#include "target_syssvc.h"
#include <sil.h>
#ifdef TOPPERS_OMIT_TECS
#include "chip_serial.h"
#endif

/*
 *  エラー時の処理
 */
extern void Error_Handler(void);

extern volatile int image_def_block;

/*
 *  起動時のハードウェア初期化処理
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
}

void software_init_hook(void)
{
    /* Initialize sio for fput */
#ifdef TOPPERS_OMIT_TECS
    sio_initialize(0);
    sio_opn_por(SIOPID_FPUT, 0);
#endif
}

/*
 *  ターゲット依存部 初期化処理
 */
void
target_initialize(void)
{
    /*
     *  チップ依存の初期化（mtvec・Xh3irq・コア依存部）
     */
    chip_initialize();
}

/*
 *  ターゲット依存部 終了処理
 */
void
target_exit(void)
{
    /* チップ依存部の終了処理 */
    chip_terminate();
    while (1) ;
}

/*
 *  エラー発生時の処理
 */
void
Error_Handler(void)
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
