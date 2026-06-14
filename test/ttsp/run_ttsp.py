#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  TOPPERS Software
#      Toyohashi Open Platform for Embedded Real-Time Systems
#
#  Copyright (C) 2016-2026 by Embedded and Real-Time Systems Laboratory
#              Graduate School of Information Science, Nagoya Univ., JAPAN
#
#  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
#  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
#  トウェアは無保証で提供される．
#
#  $Id: run_ttsp.py (TTSP3 conformance test driver for asp3_core) $
#

#
#       TTSP3 適合性テストの実行ドライバ（asp3_core 側・外部ドライバ方式）
#
#  TOPPERS テストスイート TTSP3 の API 適合性テスト（api_test）を，TTSP3 を
#  改変せず読み取り専用のテスト供給源として扱い，asp3_core の CMake + QEMU で
#  実行・判定する（設計の経緯は docs/dev/ttsp3-conformance.md，案B）．
#
#  TTSP3 のテスト1件は以下の3系統のいずれかに分類される（自動判定）：
#
#    error    … err_code.txt を持つディレクトリ（チェックイン済 out.cfg）．
#               cfg が err_code.txt の E_* を検出してビルドが失敗すれば PASS．
#               QEMU 実行は不要．
#    runtime  … out.c を持ち err_code.txt を持たないディレクトリ（チェック
#               イン済）．ビルド → QEMU 実行 → "All check points passed." で PASS．
#    yaml     … *.yaml ファイル（functional / staticAPI 正常系）．ttg.rb（Ruby・
#               ターゲット非依存）で out.{c,cfg,h} を生成 → runtime と同じ判定．
#
# 【実行方法】
#       run_ttsp.py [オプション] [テストパス ...]
#
#       テストパス：TTSP3 ルートからの相対 or 絶対パス．ディレクトリ指定で
#                   配下を再帰探索．省略時は api_test/ASP 全体．
#
#       --target <名>       asp3_core 側のターゲット名（既定 zybo_z7）
#       --ttsp3-root <dir>  TTSP3 ルート（既定 $TTSP3_ROOT または
#                           <repo>/../../TTSP3/work/ttsp3）
#       --build-dir <dir>   ビルド作業ルート（既定 <repo>/build/ttsp）
#       --only error|runtime|yaml   指定系統のみ実行（複数可・カンマ区切り）
#       --list              実行せずに対象テストと系統を列挙
#       --keep              ビルドディレクトリを残す（既定は使い回し）
#       --jobs N            未使用（将来の並列化用・現状は逐次）
#       --tap               TAP 形式（ok N / not ok N）で出力
#       -v, --verbose       失敗時にビルド/実行ログを表示
#

import argparse
import os
import re
import shutil
import subprocess
import sys

#  ソースルート（本スクリプト test/ttsp/ の2つ上）
REPO_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

#
#  ターゲット定義
#
#  preset       … asp3_core の CMake プリセット名
#  ttsp_target  … TTSP3 library/ASP/target/<ttsp_target> のディレクトリ名
#  qemu         … QEMU 実行コマンド（{elf} を asp.elf の絶対パスに置換）
#                 None の場合はホスト実行（./asp）
#
TARGETS = {
    "zybo_z7": {
        "preset": "zybo_z7-qemu",
        "ttsp_target": "zybo_z7_gcc",
        "qemu": ("qemu-system-arm -M xilinx-zynq-a9 -semihosting -m 512M "
                 "-serial null -serial mon:stdio -nographic -kernel {elf}"),
    },
    # 他ターゲットは TTSP3 側に library/ASP/target/<t> と ttsp_target_test.c が
    # 必要．移植時にここへ追加する（docs/dev/ttsp3-conformance.md 参照）．
}

#  QEMU 実行のタイムアウト（秒）
QEMU_TIMEOUT = 30
#  ビルドのタイムアウト（秒）
BUILD_TIMEOUT = 300


# --------------------------------------------------------------------------
#  ユーティリティ
# --------------------------------------------------------------------------

def run(cmd, cwd=None, timeout=None):
    """コマンドを実行し (returncode, combined_output) を返す."""
    try:
        p = subprocess.run(cmd, cwd=cwd, shell=isinstance(cmd, str),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=timeout)
        return p.returncode, p.stdout.decode("utf-8", "replace")
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode("utf-8", "replace") if e.stdout else ""
        return 124, out + "\n[TIMEOUT]\n"


def read_err_codes(path):
    """err_code.txt から期待エラーコード（E_*）の集合を読む."""
    codes = set()
    with open(path, encoding="utf-8") as f:
        for line in f:
            for tok in re.findall(r"E_[A-Z0-9]+", line):
                codes.add(tok)
    return codes


# --------------------------------------------------------------------------
#  テストの探索と分類
# --------------------------------------------------------------------------

class Test:
    def __init__(self, kind, test_id, src_dir, yaml=None, err_codes=None):
        self.kind = kind          # "error" | "runtime" | "yaml"
        self.id = test_id         # 表示用 ID（TTSP3 ルートからの相対パス）
        self.src_dir = src_dir    # out.{c,cfg,h} のあるディレクトリ
        self.yaml = yaml          # yaml 系のみ：.yaml ファイルパス
        self.err_codes = err_codes or set()


def discover(ttsp_root, paths):
    """指定パス配下の TTSP3 テストを探索して Test のリストを返す."""
    tests = []
    for base in paths:
        abase = base if os.path.isabs(base) else os.path.join(ttsp_root, base)
        abase = os.path.abspath(abase)
        if os.path.isfile(abase) and abase.endswith(".yaml"):
            tests.append(_make_yaml_test(ttsp_root, abase))
            continue
        for dirpath, _dirs, files in os.walk(abase):
            if "out.c" in files:
                rel = os.path.relpath(dirpath, ttsp_root)
                if "err_code.txt" in files:
                    tests.append(Test(
                        "error", rel, dirpath,
                        err_codes=read_err_codes(
                            os.path.join(dirpath, "err_code.txt"))))
                else:
                    tests.append(Test("runtime", rel, dirpath))
            for f in files:
                if f.endswith(".yaml"):
                    tests.append(_make_yaml_test(
                        ttsp_root, os.path.join(dirpath, f)))
    #  ID で安定ソート・重複除去
    uniq = {}
    for t in tests:
        uniq[(t.kind, t.id)] = t
    return sorted(uniq.values(), key=lambda t: t.id)


def _make_yaml_test(ttsp_root, yaml_path):
    rel = os.path.relpath(yaml_path, ttsp_root)
    return Test("yaml", rel, None, yaml=yaml_path)


# --------------------------------------------------------------------------
#  ビルド・実行
# --------------------------------------------------------------------------

def ttsp_include_dirs(ttsp_root, ttsp_target):
    return [
        os.path.join(ttsp_root, "library/ASP/test"),
        os.path.join(ttsp_root, "library/ASP/target", ttsp_target),
    ]


def ttsp_extra_c_files(ttsp_root, ttsp_target):
    return [
        os.path.join(ttsp_root, "library/ASP/test/ttsp_test_lib.c"),
        os.path.join(ttsp_root, "library/ASP/target", ttsp_target,
                     "ttsp_target_test.c"),
    ]


def gen_with_ttg(ttsp_root, yaml_path, gen_dir):
    """ttg.rb（読み取り専用）で out.{c,cfg,h} を gen_dir に生成する."""
    os.makedirs(gen_dir, exist_ok=True)
    #  生成物が前回分で残っていると誤判定するので消す
    for f in ("out.c", "out.cfg", "out.h"):
        p = os.path.join(gen_dir, f)
        if os.path.exists(p):
            os.remove(p)
    ttg = os.path.join(ttsp_root, "tools/ttg/bin/ttg.rb")
    rc, out = run(["ruby", ttg, "-a", "-p", yaml_path],
                  cwd=gen_dir, timeout=BUILD_TIMEOUT)
    ok = rc == 0 and all(
        os.path.exists(os.path.join(gen_dir, f))
        for f in ("out.c", "out.cfg", "out.h"))
    return ok, out


def configure_and_build(tgt, ttsp_root, appldir, build_dir):
    """1テスト分の CMake configure + build を行う．(rc, log) を返す."""
    inc = ";".join(ttsp_include_dirs(ttsp_root, tgt["ttsp_target"]))
    extra = ";".join(ttsp_extra_c_files(ttsp_root, tgt["ttsp_target"]))
    cfg_cmd = [
        "cmake", "--preset", tgt["preset"], "-B", build_dir,
        f"-DASP3_APPLDIR={appldir}", "-DASP3_APPLNAME=out",
        f"-DASP3_APP_INCLUDE_DIRS={inc}",
        f"-DASP3_EXTRA_APP_C_FILES={extra}",
    ]
    rc, log = run(cfg_cmd, cwd=REPO_ROOT, timeout=BUILD_TIMEOUT)
    if rc != 0:
        return rc, log
    rc, blog = run(["cmake", "--build", build_dir],
                   cwd=REPO_ROOT, timeout=BUILD_TIMEOUT)
    return rc, log + blog


def run_qemu(tgt, build_dir):
    elf = os.path.join(build_dir, "asp.elf")
    if not os.path.exists(elf):
        elf = os.path.join(build_dir, "asp")
    if tgt["qemu"] is None:
        rc, out = run([elf], timeout=QEMU_TIMEOUT)
    else:
        cmd = tgt["qemu"].format(elf=elf)
        rc, out = run(cmd, timeout=QEMU_TIMEOUT)
    return rc, out


# --------------------------------------------------------------------------
#  1テストの判定
# --------------------------------------------------------------------------

class Result:
    def __init__(self, test, status, detail="", log=""):
        self.test = test
        self.status = status      # "PASS" | "FAIL" | "SKIP"
        self.detail = detail
        self.log = log


def judge_error(test, tgt, ttsp_root, build_dir):
    """error 系：ビルドが失敗し，期待 E_* が cfg エラーに出れば PASS."""
    rc, log = configure_and_build(tgt, ttsp_root, test.src_dir, build_dir)
    found = set(re.findall(r"error:\s*(E_[A-Z0-9]+)", log))
    if rc == 0:
        return Result(test, "FAIL",
                      f"ビルド成功（期待エラー {sorted(test.err_codes)} 未検出）", log)
    if test.err_codes & found:
        return Result(test, "PASS",
                      f"期待エラー {sorted(test.err_codes & found)} を検出")
    return Result(test, "FAIL",
                  f"期待 {sorted(test.err_codes)} / 検出 {sorted(found) or 'なし'}", log)


def judge_runtime(test, tgt, ttsp_root, build_dir, appldir):
    """runtime/yaml 系：ビルド → QEMU 実行 → 合格文字列で PASS."""
    rc, log = configure_and_build(tgt, ttsp_root, appldir, build_dir)
    if rc != 0:
        return Result(test, "FAIL", "ビルド失敗", log)
    rc, out = run_qemu(tgt, build_dir)
    if "All check points passed." in out:
        return Result(test, "PASS")
    if "Assertion" in out or "fail" in out.lower():
        return Result(test, "FAIL", "チェックポイント不合格", out)
    return Result(test, "FAIL", "合格文字列なし（実行エラー/タイムアウト）", out)


def run_test(test, tgt, ttsp_root, build_root, keep):
    build_dir = os.path.join(build_root, tgt["preset"],
                             test.id.replace("/", "_").replace(".yaml", ""))
    if test.kind == "error":
        res = judge_error(test, tgt, ttsp_root, build_dir)
    elif test.kind == "runtime":
        res = judge_runtime(test, tgt, ttsp_root, build_dir, test.src_dir)
    else:  # yaml
        gen_dir = os.path.join(build_dir, "ttg")
        ok, gout = gen_with_ttg(ttsp_root, test.yaml, gen_dir)
        if not ok:
            res = Result(test, "FAIL", "ttg 生成失敗", gout)
        else:
            res = judge_runtime(test, tgt, ttsp_root, build_dir, gen_dir)
    if not keep and os.path.isdir(build_dir):
        shutil.rmtree(build_dir, ignore_errors=True)
    return res


# --------------------------------------------------------------------------
#  メイン
# --------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description="TTSP3 API 適合性テストを asp3_core の CMake+QEMU で実行する")
    ap.add_argument("paths", nargs="*", default=["api_test/ASP"],
                    help="TTSP3 ルート相対/絶対のテストパス（既定 api_test/ASP）")
    ap.add_argument("--target", default="zybo_z7", choices=sorted(TARGETS))
    ap.add_argument("--ttsp3-root", default=None)
    ap.add_argument("--build-dir", default=os.path.join(REPO_ROOT, "build", "ttsp"))
    ap.add_argument("--only", default=None,
                    help="error,runtime,yaml のうち実行する系統（カンマ区切り）")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument("--tap", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    ttsp_root = args.ttsp3_root or os.environ.get("TTSP3_ROOT") or \
        os.path.join(REPO_ROOT, "..", "..", "TTSP3", "work", "ttsp3")
    ttsp_root = os.path.abspath(ttsp_root)
    if not os.path.isdir(ttsp_root):
        sys.exit(f"TTSP3 ルートが見つかりません: {ttsp_root}")

    tgt = TARGETS[args.target]
    only = set(args.only.split(",")) if args.only else None

    paths = args.paths if args.paths else ["api_test/ASP"]
    tests = discover(ttsp_root, paths)
    if only:
        tests = [t for t in tests if t.kind in only]

    if not tests:
        sys.exit("対象テストが見つかりません")

    if args.list:
        for t in tests:
            print(f"{t.kind:8s} {t.id}")
        print(f"\n合計 {len(tests)} 件 "
              f"(error={sum(t.kind=='error' for t in tests)} "
              f"runtime={sum(t.kind=='runtime' for t in tests)} "
              f"yaml={sum(t.kind=='yaml' for t in tests)})")
        return

    n_pass = n_fail = n_skip = 0
    for i, t in enumerate(tests, 1):
        res = run_test(t, tgt, ttsp_root, args.build_dir, args.keep)
        if res.status == "PASS":
            n_pass += 1
        elif res.status == "SKIP":
            n_skip += 1
        else:
            n_fail += 1

        if args.tap:
            mark = "ok" if res.status == "PASS" else "not ok"
            dir_ = " # SKIP" if res.status == "SKIP" else ""
            print(f"{mark} {i} {t.id} [{t.kind}]{dir_}"
                  + (f" - {res.detail}" if res.detail else ""))
        else:
            print(f"[{i}/{len(tests)}] {res.status:4s} {t.kind:8s} {t.id}"
                  + (f"  ({res.detail})" if res.detail else ""))
        if args.verbose and res.status == "FAIL" and res.log:
            print("---- log ----")
            print(res.log[-3000:])
            print("-------------")
        sys.stdout.flush()

    total = len(tests)
    if args.tap:
        print(f"1..{total}")
    print(f"\n== TTSP3 [{args.target}] PASS={n_pass} FAIL={n_fail} "
          f"SKIP={n_skip} / {total} ==", file=sys.stderr)
    sys.exit(1 if n_fail else 0)


if __name__ == "__main__":
    main()
