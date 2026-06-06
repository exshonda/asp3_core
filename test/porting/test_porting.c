/*
 *		移植検証テスト（カーネル基本機能6項目・TAP形式）
 *
 *  新ターゲット移植時の最初の動作確認テスト．sample1（目視確認）・
 *  testexec全件（重い）の前段として，カーネル基本機能6項目を
 *  TAP形式で機械判定する．項目の並びは故障切り分けの順序になっている：
 *
 *    1. syslog_output         ブート〜UART（低レベル出力）の経路
 *    2. tick_timer_basic      高分解能タイマの歩進（割込み不要）
 *    3. task_create_activate  タスク起動とディスパッチャ
 *    4. semaphore_signal_wait セマフォによるタスク間同期（カーネル本体）
 *    5. eventflag_set_wait    イベントフラグによるタスク間同期（同上）
 *    6. alarm_handler         アラームハンドラ＝タイマ割込みの経路
 *
 *  2.までしか通らなければタイマ/ブート周り，5.まで通って6.で落ちれば
 *  タイマ割込み（割込みコントローラ設定）を疑う，という使い方をする．
 *
 *  項目2.と6.の待ちループは fch_hrt() による時間上限と繰返し回数上限の
 *  二重バウンドとし，タイマが停止していてもテストがハングしない．
 */

#include <kernel.h>
#include <t_syslog.h>
#include "kernel_cfg.h"
#include "test_porting_cfg.h"
#include "tap.h"

/*
 *  待ちループの上限
 */
#define TICK_POLL_MAX	100000000UL		/* 項目2：歩進待ちの回数上限 */
#define ALM_TIMEOUT		1000000U		/* 項目6：アラーム待ち時間上限[μs] */
#define ALM_POLL_MAX	1000000000UL	/* 項目6：待ちの回数上限 */

#define ALM1_TIME		10000U			/* 項目6：アラーム時間[μs] */

/*
 *  被験タスク・ハンドラとの通信用フラグ
 */
static volatile bool_t	task2_ran;		/* 項目3 */
static volatile bool_t	task3_ran;		/* 項目4 */
static volatile bool_t	task4_ran;		/* 項目5 */
static volatile bool_t	alarm1_ran;		/* 項目6 */

/*
 *  項目3：起動されたことを記録するだけのタスク
 */
void
task2(EXINF exinf)
{
	task2_ran = true;
	ext_tsk();
}

/*
 *  項目4：セマフォ待ち後に記録するタスク
 */
void
task3(EXINF exinf)
{
	if (wai_sem(SEM1) == E_OK) {
		task3_ran = true;
	}
	ext_tsk();
}

/*
 *  項目5：イベントフラグ待ち後に記録するタスク
 */
void
task4(EXINF exinf)
{
	FLGPTN	flgptn;

	if (wai_flg(FLG1, 0x01U, TWF_ORW, &flgptn) == E_OK) {
		task4_ran = true;
	}
	ext_tsk();
}

/*
 *  項目6：アラームハンドラ
 */
void
alarm1_handler(EXINF exinf)
{
	alarm1_ran = true;
}

/*
 *  メインタスク
 */
void
main_task(EXINF exinf)
{
	ER			ercd;
	HRTCNT		hrt1, hrt2, hrt0;
	bool_t		blocked;
	unsigned long	i;

	tap_diag("test_porting: kernel porting verification");
	tap_plan(6U);

	/*
	 *  1. syslog_output：ここまで出力できていれば，ブートと低レベル
	 *     出力（UART等）の経路は通っている
	 */
	tap_ok(true, "syslog_output");

	/*
	 *  2. tick_timer_basic：高分解能タイマが歩進すること
	 *     （タイマ割込みは使わない＝カウンタの動作のみを見る）
	 */
	hrt1 = fch_hrt();
	hrt2 = hrt1;
	for (i = 0UL; i < TICK_POLL_MAX && hrt2 == hrt1; i++) {
		hrt2 = fch_hrt();
	}
	tap_ok(hrt2 != hrt1, "tick_timer_basic");

	/*
	 *  3. task_create_activate：高優先度タスクの起動で即座に
	 *     ディスパッチされ，制御が戻るまでに実行済みになること
	 */
	ercd = act_tsk(TASK2);
	tap_ok(ercd == E_OK && task2_ran, "task_create_activate");

	/*
	 *  4. semaphore_signal_wait：高優先度タスクがセマフォ待ちで
	 *     ブロックし，sig_semで待ち解除されること
	 */
	ercd = act_tsk(TASK3);
	blocked = (ercd == E_OK && !task3_ran);		/* 待ちに入ったこと */
	if (blocked) {
		ercd = sig_sem(SEM1);
	}
	tap_ok(blocked && ercd == E_OK && task3_ran, "semaphore_signal_wait");

	/*
	 *  5. eventflag_set_wait：同様にイベントフラグで待ち解除されること
	 */
	ercd = act_tsk(TASK4);
	blocked = (ercd == E_OK && !task4_ran);		/* 待ちに入ったこと */
	if (blocked) {
		ercd = set_flg(FLG1, 0x01U);
	}
	tap_ok(blocked && ercd == E_OK && task4_ran, "eventflag_set_wait");

	/*
	 *  6. alarm_handler：アラームハンドラが起動すること
	 *     （高分解能タイマ割込みの経路の確認．時間上限と回数上限の
	 *     二重バウンドの待ちループで，割込みが来なくてもハングしない）
	 */
	ercd = sta_alm(ALM1, ALM1_TIME);
	hrt0 = fch_hrt();
	for (i = 0UL;
		 i < ALM_POLL_MAX && !alarm1_ran
			&& (HRTCNT)(fch_hrt() - hrt0) < ALM_TIMEOUT;
		 i++) {
	}
	tap_ok(ercd == E_OK && alarm1_ran, "alarm_handler");

	tap_done();
}
