/*
 *		移植検証テスト用 最小TAPフレームワーク
 *
 *  新ターゲット移植時の最初の動作確認（test_porting）専用の小さな
 *  TAP（Test Anything Protocol）出力ライブラリ．syssvc/test_svc.c とは
 *  独立（test_svcはチェックポイント方式，こちらは項目別ok/not ok方式）．
 *
 *  合否判定は出力のパース（"# N/N passed" 行）で行う（全ターゲット共通．
 *  scripts/ci/run_testexec.py と同方式）．QEMUセミホスティング等の
 *  終了コードには依存しない．
 */

#ifndef TOPPERS_TAP_H
#define TOPPERS_TAP_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  テストプラン（"1..N"）の出力
 */
extern void	tap_plan(uint_t num);

/*
 *  1項目の合否出力（"ok N - name" ／ "not ok N - name"）
 */
extern void	tap_ok(bool_t cond, const char *name);

/*
 *  診断メッセージ（"# msg"）の出力
 */
extern void	tap_diag(const char *msg);

/*
 *  テスト終了（"# N/N passed" を出力し，ログをフラッシュして ext_ker()）
 */
extern void	tap_done(void);

#ifdef __cplusplus
}
#endif

#endif /* TOPPERS_TAP_H */
