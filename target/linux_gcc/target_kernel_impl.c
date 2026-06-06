/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2006-2023 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: target_kernel_impl.c 1793 2023-03-05 10:30:13Z ertl-hiro $
 */

/*
 *		カーネルのターゲット依存部（Linux用）
 *
 *  【asp3_core変更】CLIターゲット対応（経緯は docs/dev/cli-target.md）：
 *    - main()をargc/argv化し，--tap／--slog／--helpオプションを追加
 */

#include "kernel_impl.h"
#include "intno.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 *  【asp3_core追加】テストプログラム用サービスのTAP出力モード
 *  （syssvc/test_svc.c）への弱参照．test_svc.cがリンクされていない
 *  ビルド（sample1等）でもリンクできるよう，weakシンボルとして参照し，
 *  存在するときのみ--tapで設定する．
 */
extern bool_t	test_tap_mode __attribute__((weak));

/*
 *  SIGIOシグナルのハンドラ
 */
static void
sigio_handler(int sig)
{
	if (pthread_getspecific(thrcb_key) != NULL) {
		pthread_kill(main_thread, sig);
		return;
	}
	raise_int_main(INTNO_SIGIO);
}

/*
 *  SIGHUPシグナルのハンドラ
 */
static void
sighup_handler(int sig)
{
	if (pthread_getspecific(thrcb_key) != NULL) {
		pthread_kill(main_thread, sig);
		return;
	}
	raise_int_main(INTNO_SIGHUP);
}

/*
 *  ターゲット依存の初期化
 */
void
target_initialize(void)
{
	struct sigaction	sigact;

	/*
	 *  POSIX依存部の初期化
	 */
	posix_initialize();

	/*
	 *  SIGIOシグナルのハンドラの設定
	 */
	sigact.sa_handler = sigio_handler;
	sigact.sa_flags = 0;
	sigemptyset(&(sigact.sa_mask));
	sigaction(SIGIO, &sigact, NULL);

	/*
	 *  SIGHUPシグナルのハンドラの設定
	 */
	sigact.sa_handler = sighup_handler;
	sigact.sa_flags = 0;
	sigemptyset(&(sigact.sa_mask));
	sigaction(SIGHUP, &sigact, NULL);
}

/*
 *  ターゲット依存の終了処理
 */
void
target_exit(void)
{
	/*
	 *  プロセスの終了処理
	 */
	exit(0);
}

/*
 *  【asp3_core追加】コマンドラインオプションの使用方法の表示
 */
static void
usage(FILE *fp)
{
	fputs("usage: asp [options]\n"
		  "  --tap     output test results in TAP format (ok N / not ok N)\n"
		  "  --slog    output structured trace log (T=<tick>,EV=<event>,...)\n"
		  "            to stderr (requires a build with ASP3_ENABLE_TRACE=ON)\n"
		  "  --help    show this help and exit\n", fp);
}

/*
 *  メイン関数
 *
 *  【asp3_core変更】argc/argv化し，--tap／--slog／--helpを追加．
 */
int
main(int argc, char *argv[])
{
	sigset_t	sigmask;
	int			i;

	/*
	 *  コマンドラインオプションの処理
	 */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--tap") == 0) {
			if (&test_tap_mode != NULL) {
				test_tap_mode = true;
			}
			else {
				fputs("asp: warning: --tap has no effect "
					  "(test_svc.c is not linked).\n", stderr);
			}
		}
		else if (strcmp(argv[i], "--slog") == 0) {
#ifdef TOPPERS_ENABLE_TRACE
			trace_initialize((intptr_t) TRACE_SLOG);
#else /* TOPPERS_ENABLE_TRACE */
			fputs("asp: error: --slog requires a build with "
				  "ASP3_ENABLE_TRACE=ON.\n", stderr);
			return(2);
#endif /* TOPPERS_ENABLE_TRACE */
		}
		else if (strcmp(argv[i], "--help") == 0) {
			usage(stdout);
			return(0);
		}
		else {
			fprintf(stderr, "asp: unknown option: %s\n", argv[i]);
			usage(stderr);
			return(2);
		}
	}

	/*
	 *  全てのシグナルをマスクする．
	 */
	pthread_sigmask_blockall(NULL);

	/*
	 *  カーネルの起動
	 */
	sta_ker();
	start_dispatch_task();

	/*
	 *  割込みの受付けと分配
	 */
	while (true) {
		sigemptyset(&sigmask);
		sigsuspend(&sigmask);
	}
}

/*
 *  微少時間待ち（本来はSILのターゲット依存部）
 */
void
sil_dly_nse(ulong_t dlytim)
{
	volatile uint_t		i;

	for (i = 0; i < 10; i++) ;
	if (dlytim > SIL_DLY_TIM1) {
		dlytim -= SIL_DLY_TIM1;
		for (i = 0; i < 2; i++) ;
		while (dlytim > SIL_DLY_TIM2) {
			dlytim -= SIL_DLY_TIM2;
			for (i = 0; i < 2; i++) ;
		}
	}
}
