#!/usr/bin/env bash
#
#		testcfg.py のCI用ラッパ（合否判定付き）
#
#  test_cfg/testcfg.py は期待値との差分を表示する運用のため終了コードで
#  合否を返さない．本スクリプトは作業ディレクトリの準備（TARGET_OPTIONS
#  生成）→ testcfg.py実行（全テスト明示指定）→ 出力の解析を行い，
#  終了コード（全パス=0）を返す．CI・ローカル共用．
#
# 【実行方法】
#	run_testcfg.sh <作業ディレクトリ>
#
# 【例】
#	scripts/ci/run_testcfg.sh build/testcfg
#
set -u

workdir=${1:?usage: run_testcfg.sh <workdir>}
root=$(cd "$(dirname "$0")/../.." && pwd)

#  testcfg.pyの全テスト（CFG_TEST_SPEC／PASS3／PASS2／PASS1）．
#  「all」は既存OBJ-*ディレクトリのみ対象のため明示的に列挙する
tests="cfg_all1 pass3_all1
       pass2_cfg1 pass2_int1 pass2_obj1 pass2_obj2 pass2_task1 pass2_subprio1
       pass1_cfg1"
num_tests=9

mkdir -p "$workdir"
cd "$workdir"
echo '-G Ninja -DASP3_TARGET=dummy_gcc -DASP3_OMIT_DEFAULT_SYSSVC=ON -DASP3_EXTRA_COMPILE_DEFS=TOPPERS_OMIT_SYSLOG' > TARGET_OPTIONS

# shellcheck disable=SC2086
python3 "$root/test_cfg/testcfg.py" $tests 2>&1 | tee testcfg.log

#  失敗マーカ（期待値不一致・想定外エラー）の検出
if grep -nE '#TODO#|== error.txt mismatch|== unexpected errors ==' testcfg.log; then
    echo "not ok - cfg test mismatch (see testcfg.log above)"
    exit 1
fi

#  全テストのチェックが実行されたことの確認
checked=$(grep -c '^== checking: ' testcfg.log)
if [ "$checked" -ne "$num_tests" ]; then
    echo "not ok - only $checked/$num_tests cfg tests checked"
    exit 1
fi

echo "ok - all $num_tests cfg tests passed"
