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
 *  ターゲット依存モジュール（ARM MPS2-AN505 用）
 *
 *  QEMU の mps2-an505 マシンでは，クロックや GPIO/パッドの初期化は不要で
 *  あり，SysTick・NVIC・FPU の設定はコア依存部（core_initialize）が行う．
 *  このため hardware_init_hook / software_init_hook は start.S の弱いデフォ
 *  ルト定義をそのまま用い，本ファイルでは定義しない．
 */
#include "kernel_impl.h"
#include "target_syssvc.h"
#include <sil.h>

/*
 *  起動時のハードウェア初期化処理
 *
 *  start.S から，BSS/DATA の初期化に先立って呼び出される（弱いデフォルト
 *  定義を上書きする）．
 */
void
hardware_init_hook(void)
{
#ifdef TOPPERS_FPU_ENABLE
    /*
     *  FPU（CP10/CP11）を有効化する．
     *
     *  core_initialize() でも CPACR を設定するが，CPACR の変更を反映する
     *  には DSB/ISB が必要である（特に QEMU では，ISB が無いと変換ブロック
     *  にキャッシュされた「FPU 無効」状態が更新されず，FP 命令が NOCP
     *  UsageFault となる）．このため，カーネル初期化で FP 命令が使われる
     *  前に，ここでバリア付きで有効化しておく．
     */
    *((volatile uint32_t *) CPACR_BASE) |= CPACR_FPU_ENABLE;
    __asm__ volatile ("dsb 0xf" ::: "memory");
    __asm__ volatile ("isb 0xf" ::: "memory");
#endif /* TOPPERS_FPU_ENABLE */
}

/*
 *  システムログの低レベル出力のための初期化
 */
#ifndef TOPPERS_OMIT_TECS
extern void tPutLogSIOPort_initialize(void);
#else /* TOPPERS_OMIT_TECS */
extern void sio_initialize(EXINF exinf);
extern void target_fput_initialize(void);
#endif /* TOPPERS_OMIT_TECS */

/*
 *  ターゲット依存部 初期化処理
 */
void
target_initialize(void)
{
    /*
     *  コア依存部の初期化（VTOR・例外優先度・FPU の設定）
     */
    core_initialize();

    /*
     *  SIO を初期化
     */
#ifndef TOPPERS_OMIT_TECS
    tPutLogSIOPort_initialize();
#else /* TOPPERS_OMIT_TECS */
    sio_initialize(0);
    target_fput_initialize();
#endif /* TOPPERS_OMIT_TECS */
}

/*
 *  ターゲット依存部 終了処理
 */
void
target_exit(void)
{
    /*
     *  コア依存部の終了処理
     */
    core_terminate();

#ifdef TOPPERS_USE_QEMU
    /*
     *  QEMU をセミホスティングで終了させる（SYS_EXIT）．
     *  これにより，テスト終了（ext_ker）時に QEMU が停止し，テストランナー
     *  （testexec.rb）が次のテストへ進める．実行時は -semihosting-config
     *  enable=on を指定すること．
     */
    {
        register uint32_t r0 __asm__("r0") = 0x18U;       /* SYS_EXIT */
        register uint32_t r1 __asm__("r1") = 0x20026U;    /* ADP_Stopped_ApplicationExit */
        __asm__ volatile ("bkpt 0xab" : : "r"(r0), "r"(r1) : "memory");
    }
#endif /* TOPPERS_USE_QEMU */

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
 *  デフォルトの software_term_hook（弱い定義）
 */
__attribute__((weak)) void
software_term_hook(void)
{
}

#ifdef TOPPERS_OMIT_TECS
/*
 *		システムログの低レベル出力（非TECS版専用）
 */

#include "target_serial.h"

/*
 *  低レベル出力用のSIOポート管理ブロック
 */
static SIOPCB	*p_siopcb_target_fput;

/*
 *  SIOポートの初期化
 */
void
target_fput_initialize(void)
{
    p_siopcb_target_fput = cmsdk_uart_opn_por(SIOPID_FPUT, 0);
}

/*
 *  SIOポートへのポーリング出力
 */
static void
mps2_an505_uart_fput(char c)
{
    /*
     *  送信できるまでポーリング
     */
    while (!(cmsdk_uart_snd_chr(p_siopcb_target_fput, c))) {
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
        mps2_an505_uart_fput('\r');
    }
    mps2_an505_uart_fput(c);
}

#endif /* TOPPERS_OMIT_TECS */
