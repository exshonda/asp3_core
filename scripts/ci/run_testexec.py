#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#		testexec.py のCI用ラッパ（合否判定付き）
#
#  test/testexec.py は実行結果（"All check points passed."）を目視確認する
#  運用のため終了コードで合否を返さない．本スクリプトは作業ディレクトリの
#  準備（TARGET_OPTIONS／TARGET_RUN生成）→ testexec.py実行 → 出力の解析を
#  行い，TAP形式のサマリと終了コード（全パス=0）を返す．CI・ローカル共用．
#
# 【実行方法】
#	run_testexec.py --options "<TARGET_OPTIONSの1行>" [--run "<TARGET_RUNの内容>"]
#	               --workdir <作業ディレクトリ> [--log <ログファイル>] <テスト名>...
#
# 【例】
#	# POSIX（linuxプリセット・ホスト実行）
#	scripts/ci/run_testexec.py --options "--preset linux" \
#	    --run "timeout 60 ./asp" --workdir build/testexec-linux task1 sem1
#
#	# QEMU mps2-an505
#	scripts/ci/run_testexec.py --options "--preset mps2_an505-qemu" \
#	    --run "timeout 60 qemu-system-arm -machine mps2-an505 -nographic \
#	           -semihosting-config enable=on,target=native -kernel asp.elf" \
#	    --workdir build/testexec-mps2 task1 sem1 flg1 tmevt1 hrt1
#
#  合否判定：テスト毎の "== executing: <NAME> ==" セクションに
#  "All check points passed." が含まれ，かつ失敗行（TAPモードの "not ok"／
#  非TAPモードの "## "）が無いこと．セクション自体が無い場合（ビルド失敗・
#  タイムアウト等）も失敗とする．

import argparse
import os
import re
import subprocess
import sys

PASS_MARK = "All check points passed."
FAIL_PATTERNS = (
    re.compile(r"^not ok "),            # TAP出力モードの失敗行
    re.compile(r"^## "),                # 非TAPモードの失敗行（test_svc.c参照）
)

#  ターゲット構成によりテスト自体が不要な場合の出力（cpuexc系）→ SKIP扱い
SKIP_MARK = "This test program is not necessary."

#  測定系テスト（check_finish(0)で終了しPASS_MARKを出力しない）の個別判定
#  pass：完走マーカ／fail：異常出力（FAIL_PATTERNSに追加で適用）
SPECIAL_SPEC = {
    "hrt1": {
        "pass": "high resolution timer count test finishes.",
        "fail": (re.compile(r"goes back"),),
    },
    "dlynse": {
        "pass": "-- for checking boundary conditions --",
        "fail": (re.compile(r"sil_dly_nse\(\d+\): \d+ NG"),),
    },
}


def main():
    parser = argparse.ArgumentParser(
        description="testexec.py CI wrapper with pass/fail judgement")
    parser.add_argument("--options", required=True,
                        help="TARGET_OPTIONS line (cmake configure args)")
    parser.add_argument("--run", default=None,
                        help="TARGET_RUN content (run command; cwd=OBJ dir)")
    parser.add_argument("--workdir", required=True,
                        help="working directory (created if missing)")
    parser.add_argument("--log", default=None,
                        help="log file (default: <workdir>/testexec.log)")
    parser.add_argument("tests", nargs="+", help="test names (testexec.py TEST_SPEC)")
    args = parser.parse_args()

    src_root = os.path.abspath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    testexec = os.path.join(src_root, "test", "testexec.py")

    workdir = os.path.abspath(args.workdir)
    os.makedirs(workdir, exist_ok=True)
    with open(os.path.join(workdir, "TARGET_OPTIONS"), "w", encoding="utf-8") as f:
        f.write(args.options.strip() + "\n")
    if args.run is not None:
        with open(os.path.join(workdir, "TARGET_RUN"), "w", encoding="utf-8") as f:
            f.write(args.run.strip() + "\n")

    log_path = args.log or os.path.join(workdir, "testexec.log")

    #  testexec.py を実行し，出力をコンソールとログに二重化する．
    #  PYTHONUNBUFFERED：パイプ接続時にtestexec.pyのセクション見出し（print）が
    #  子プロセス（cmake／テスト実行）の出力と順序逆転しないようにする
    env = dict(os.environ, PYTHONUNBUFFERED="1")
    proc = subprocess.Popen(
        [sys.executable, testexec] + args.tests,
        cwd=workdir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, errors="replace", env=env)
    lines = []
    with open(log_path, "w", encoding="utf-8") as logf:
        for line in proc.stdout:
            sys.stdout.write(line)
            sys.stdout.flush()
            logf.write(line)
            lines.append(line.rstrip("\n"))
    proc.wait()

    #  "== executing: <NAME> ==" でセクション分割して判定
    sections = {}
    current = None
    for line in lines:
        m = re.match(r"== executing: (\S+) ==", line)
        if m:
            current = m.group(1).lower()
            sections[current] = []
        elif current is not None:
            sections[current].append(line)

    print()
    print(f"1..{len(args.tests)}")
    num_failed = 0
    for i, test in enumerate(args.tests, 1):
        body = sections.get(test.lower())
        if body is None:
            print(f"not ok {i} - {test} # no execution section (build failed?)")
            num_failed += 1
            continue
        special = SPECIAL_SPEC.get(test.lower(), {})
        pass_mark = special.get("pass", PASS_MARK)
        fail_patterns = FAIL_PATTERNS + special.get("fail", ())
        failed = [l for l in body for pat in fail_patterns if pat.search(l)]
        if failed:
            print(f"not ok {i} - {test} # {failed[0].strip()}")
            num_failed += 1
        elif any(SKIP_MARK in l for l in body):
            print(f"ok {i} - {test} # SKIP not necessary on this target")
        elif any(pass_mark in l for l in body):
            print(f"ok {i} - {test}")
        else:
            print(f"not ok {i} - {test} # '{pass_mark}' not found")
            num_failed += 1

    print(f"# log: {log_path}")
    if num_failed:
        print(f"# FAILED: {num_failed}/{len(args.tests)}")
        return 1
    print(f"# all {len(args.tests)} tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
