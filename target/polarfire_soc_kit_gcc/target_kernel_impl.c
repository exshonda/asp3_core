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
 *  $Id: target_kernel_impl.c 335 2023-04-18 10:50:40Z ertl-honda $
 */

/*
 *    カーネルのターゲット依存部（PolarFire SoC Kit用）
 *
 *  QEMU(microchip-icicle-kit) での実行を対象とする．
 *  TOPPERS/FMP3のRISC-V依存部をASP3（シングルプロセッサ）用に変換した
 *  もの（target_mprc_initializeを削除し，target_initialize(void)化）．
 */

#include "kernel_impl.h"
#include <sil.h>
#include "riscv.h"

extern void sio_initialize(EXINF exinf);
extern void target_fput_initialize(void);

/*
 *  ターゲット依存の初期化
 */
void
target_initialize(void)
{
    /*
     *  チップ依存の初期化
     */
    chip_initialize();

    /*
     *  SIOを初期化
     */
#ifdef USE_UART0
    /* クロックの有効化 */
    sil_wrw_mem(SYSREG_SUBBLK_CLOCK_CR,
                sil_rew_mem(SYSREG_SUBBLK_CLOCK_CR)
                | SYSREG_SUBBLK_CLOCK_CR_UART0);

    /* リセットの解除 */
    sil_wrw_mem(SYSREG_SOFT_RESET_CR,
                sil_rew_mem(SYSREG_SOFT_RESET_CR)
                & ~SYSREG_SOFT_RESET_CR_UART0);
#endif /* USE_UART0 */

#ifdef USE_UART1
    /* クロックの有効化 */
    sil_wrw_mem(SYSREG_SUBBLK_CLOCK_CR,
                sil_rew_mem(SYSREG_SUBBLK_CLOCK_CR)
                | SYSREG_SUBBLK_CLOCK_CR_UART1);

    /* リセットの解除 */
    sil_wrw_mem(SYSREG_SOFT_RESET_CR,
                sil_rew_mem(SYSREG_SOFT_RESET_CR)
                & ~SYSREG_SOFT_RESET_CR_UART1);
#endif /* USE_UART1 */

    sio_initialize(0);
    target_fput_initialize();
}

/*
 *  デフォルトのsoftware_term_hook（weak定義）
 */
__attribute__((weak))
void software_term_hook(void)
{
}

/*
 *  ターゲット依存の終了処理
 */
void
target_exit(void)
{
    /*
     *  software_term_hookの呼出し
     */
    software_term_hook();

    /*
     *  チップ依存の終了処理
     */
    chip_terminate();

#ifdef TOPPERS_USE_QEMU
    /*
     *  セミホスティングでQEMUを終了させる（SYS_EXIT）．
     *  RV64では a1 にパラメタブロックを渡す．
     */
    {
        static volatile uint64_t exit_block[2] = {
            0x20026ULL,     /* ADP_Stopped_ApplicationExit */
            0ULL            /* exit code */
        };
        register uint64_t a0 __asm__("a0") = 0x18;  /* SYS_EXIT */
        register uint64_t a1 __asm__("a1") = (uint64_t)exit_block;
        __asm__ volatile(
            ".balign 16\n\t"
            ".option push\n\t"
            ".option norvc\n\t"
            "slli zero, zero, 0x1f\n\t"
            "ebreak\n\t"
            "srai zero, zero, 7\n\t"
            ".option pop"
            : : "r"(a0), "r"(a1) : "memory");
    }
#endif /* TOPPERS_USE_QEMU */

    while (true) ;
}

/*
 *		システムログの低レベル出力（非TECS版）
 */
#include "target_syssvc.h"
#include "target_serial.h"

/*
 *  低レベル出力用のSIOポート管理ブロック
 */
static SIOPCB  *p_siopcb_target_fput;

/*
 *  SIOポートの初期化
 */
void
target_fput_initialize(void)
{
    p_siopcb_target_fput = sio_opn_por(SIOPID_FPUT, 0);
}

/*
 *  SIOポートへのポーリング出力
 */
Inline void
polarfire_soc_kit_uart_fput(char c)
{
    /*
     *  送信できるまでポーリング
     */
    while (!(sio_snd_chr(p_siopcb_target_fput, c))) {
        sil_dly_nse(100);
    }
}

/*
 *  SIOポートへの文字出力
 */
void
target_fput_log(char c)
{
    if (c == '\n') {
        polarfire_soc_kit_uart_fput('\r');
    }
    polarfire_soc_kit_uart_fput(c);
}
