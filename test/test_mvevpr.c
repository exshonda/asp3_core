/*
 *  TOPPERS/ASP3 Core
 *
 *  MVE(Helium) VPR レジスタのコンテキスト退避テスト(1)
 *
 * 【テストの目的】
 *
 *  ARMv8.1-M＋MVE ターゲット（Cortex-M55/M85 等）で，コンテキストスイッチを
 *  跨いで各タスクの VPR（ベクタ述語/ループ制御レジスタ）が保持されることを
 *  確認する．これは arch 共通部 core_support.S の MVE VPR 退避/復帰
 *  （#ifdef __ARM_FEATURE_MVE）が正しく機能していることの検証である．
 *
 * 【テストの方法】
 *
 *  TASK1(中優先度)が VPR にパターンAを書き込み，より高優先度の TASK2 を
 *  act_tsk で起動する．TASK2 は VPR に別パターンBを書き込んで終了し，TASK1 へ
 *  制御が戻る．このとき TASK1 の VPR が（TASK2 の書込みに汚染されず）パターンA
 *  のまま復元されていることを確認する．s16-s31 と同様 VPR はソフトウェアで
 *  退避される callee-saved 相当の状態であり，退避が無ければ TASK2 の値に
 *  破壊される．
 *
 * 【テスト項目】
 *
 *	(1) TASK1 が VPR にパターンA を設定（HWが保持する値を記録）
 *	(2) TASK1 が act_tsk(TASK2) で高優先度タスクへディスパッチ
 *	(3) TASK2 が VPR にパターンB を設定（A とは異なることを確認）
 *	(4) TASK2 終了で TASK1 へ復帰
 *	(5) TASK1 の VPR がパターンA のまま保持されていることを確認
 *
 * 【使用リソース】
 *
 *	TASK1: 中優先度，TA_ACT
 *	TASK2: 高優先度
 *
 *  本ファイルは TOPPERS ライセンスに従う新規追加（asp3_core）．
 */

#include <kernel.h>
#include <t_syslog.h>
#include "syssvc/test_svc.h"
#include "kernel_cfg.h"
#include "test_mvevpr.h"

#ifdef __ARM_FEATURE_MVE

/*
 *  VPR の読み書き（MVE 命令．実行により CONTROL.FPCA がセットされ，
 *  ディスパッチ時に FP/MVE コンテキストの退避対象となる）
 */
static inline uint32_t
read_vpr(void)
{
	uint32_t v;
	__asm__ volatile ("vmrs %0, VPR" : "=r" (v) : : "memory");
	return v;
}

static inline void
write_vpr(uint32_t v)
{
	__asm__ volatile ("vmsr VPR, %0" : : "r" (v) : "memory");
}

/*
 *  VPR の P0（述語）フィールドに置く試験パターン．設定後に読み戻した
 *  「HW が実際に保持した値」を基準に比較するため，予約ビットの影響を受けない．
 */
#define VPR_PATTERN_1	0x0000ffffU
#define VPR_PATTERN_2	0x00005a5aU

static volatile uint32_t	task1_vpr_stored;	/* TASK1 が設定し HW が保持した値 */

void
task1(EXINF exinf)
{
	test_start(__FILE__);

	/*
	 *  (1) TASK1 が VPR にパターンA を設定し，HW が保持した値を記録する．
	 */
	check_point(1);
	write_vpr(VPR_PATTERN_1);
	task1_vpr_stored = read_vpr();

	/*
	 *  (2) より高優先度の TASK2 を起動 → ディスパッチ（TASK1 の文脈が退避される）．
	 */
	check_point(2);
	check_ercd(act_tsk(TASK2), E_OK);

	/*
	 *  (5) TASK2 実行後に TASK1 へ復帰．VPR がパターンA のまま復元されていること．
	 */
	check_point(5);
	check_assert(read_vpr() == task1_vpr_stored);

	check_finish(6);
	check_point(0);
}

void
task2(EXINF exinf)
{
	/*
	 *  (3) TASK2 は自身の VPR（パターンB）を設定．TASK1 の値と異なることを確認．
	 */
	check_point(3);
	write_vpr(VPR_PATTERN_2);
	check_assert(read_vpr() != task1_vpr_stored);

	/*
	 *  (4) TASK2 終了（ext_tsk）→ TASK1 へ復帰．
	 */
	check_point(4);
	return;

	check_point(0);
}

#else /* !__ARM_FEATURE_MVE */

/*
 *  MVE 非対応ビルドでは SKIP（run_testexec.py が SKIP として扱う）．
 */
void
task1(EXINF exinf)
{
	test_start(__FILE__);
	/* run_testexec.py の SKIP_MARK に一致させ，非MVEターゲットでは SKIP 扱いにする */
	syslog_0(LOG_NOTICE, "This test program is not necessary. (MVE/Helium not enabled)");
	check_finish(0);
}

void
task2(EXINF exinf)
{
}

#endif /* __ARM_FEATURE_MVE */
