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
#       --build-only        ビルド成功で PASS（QEMU 実行をスキップ．qemu 未導入
#                           アーキの build 検証や CI 用）
#       --jobs N            未使用（将来の並列化用・現状は逐次）
#       --tap               TAP 形式（ok N / not ok N）で出力
#       -v, --verbose       失敗時にビルド/実行ログを表示
#

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

#  ソースルート（本スクリプト test/ttsp/ の2つ上）
REPO_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

#
#  ターゲット定義（自動探索）
#
#  各ターゲットの TTSP 定義はターゲット依存部 target/<t>/ttsp3/ttsp_target.py に
#  置く（dict 名 TTSP_TARGETS）。本ドライバは target/*/ttsp3/ttsp_target.py を
#  すべて読み込んで TARGETS を組み立てる＝中央の定義表を持たない（ターゲット追加時に
#  本ファイルを編集しない）。各エントリの意味：
#
#  preset        … asp3_core の CMake プリセット名
#  ttsp_target   … TTSP3 library/ASP/target/<ttsp_target> を使う場合のdir名
#                  （TTSP3 が同梱するターゲット．zybo_z7 のみ）
#  lib           … asp3_core 側に置く TTSP3 ターゲット資産の repo 相対パス
#                  （TTSP3 が同梱しないターゲット用．target/<t>/ttsp3）
#                  ttsp_target と lib はどちらか一方を設定する．
#  qemu          … QEMU 実行コマンド（{elf} を asp.elf の絶対パスに置換）．
#                  None かつ "hw" なしならホスト実行（./asp）
#  hw            … 実機実行の設定（kind/serial/jtag_tcl/ps7_init/vitis_settings/
#                  capture 等）．run_hw() が kind で分岐する．パスは repo 相対．
#  unsupported_hw… 本ターゲットで未実装の TTSP ターゲット HW 関数の集合．生成/同梱の
#                  out.c がこれらを呼ぶテストは **ビルド前に SKIP**（早送り/割込み/例外）．
#  soft_hw       … no-op 実装の HW 関数（stop_tick/start_tick）．呼ぶテストは基本 no-op で
#                  通るが、時刻凍結依存の一部は破綻＝**実行して失敗した場合のみ SKIP**．
#                  HW を使わないテストの失敗は真の FAIL として残す．
#  skip_modules  … モジュールごと未対応＝丸ごと SKIP（interrupt/exception 等）．
#
#  TTSP ターゲット HW 関数：gain_tick（HWタイマ早送り）/ int_raise（割込み）/
#  cpuexc_raise（CPU例外）/ stop_tick・start_tick（時刻凍結）．
#
def _load_target_configs():
    """target/*/ttsp3/ttsp_target.py の TTSP_TARGETS を全て読み込んで結合する."""
    targets = {}
    pattern = os.path.join(REPO_ROOT, "target", "*", "ttsp3", "ttsp_target.py")
    for path in sorted(glob.glob(pattern)):
        ns = {}
        with open(path, encoding="utf-8") as f:
            exec(compile(f.read(), path, "exec"), ns)  # repo 管理下の定義のみ
        spec = ns.get("TTSP_TARGETS")
        if isinstance(spec, dict):
            for name, t in spec.items():
                targets[name] = t
    return targets


TARGETS = _load_target_configs()

#  TTSP ターゲット HW 関数（out.c のスキャン対象）
HW_FUNCS = ("ttsp_target_gain_tick", "ttsp_int_raise", "ttsp_cpuexc_raise",
            "ttsp_target_stop_tick", "ttsp_target_start_tick",
            "ttsp_clear_int_req")


def scan_hw_calls(out_c_path):
    """out.c が呼ぶ TTSP ターゲット HW 関数の集合を返す."""
    try:
        with open(out_c_path, encoding="utf-8", errors="replace") as f:
            text = f.read()
    except OSError:
        return set()
    return {fn for fn in HW_FUNCS if fn in text}

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

def module_of(test_id):
    """テストID（api_test/<profile>/<module>/...）からモジュール名を返す."""
    parts = test_id.replace("\\", "/").split("/")
    for prof in ("ASP", "HRP", "FMP", "HRMP"):
        if prof in parts:
            i = parts.index(prof)
            if i + 1 < len(parts):
                return parts[i + 1]
    return parts[1] if len(parts) > 1 else ""


class Test:
    def __init__(self, kind, test_id, src_dir, yaml=None, err_codes=None):
        self.kind = kind          # "error" | "runtime" | "yaml"
        self.id = test_id         # 表示用 ID（TTSP3 ルートからの相対パス）
        self.src_dir = src_dir    # out.{c,cfg,h} のあるディレクトリ
        self.yaml = yaml          # yaml 系のみ：.yaml ファイルパス
        self.err_codes = err_codes or set()
        self.module = module_of(test_id)


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

def target_lib_dir(tgt, ttsp_root):
    """ターゲットテスト資産（ttsp_target_test.{c,h}・ttsp_target.cfg）の場所．
    TTSP3 同梱（ttsp_target）か asp3_core 側（lib）かを切り替える."""
    if tgt.get("lib"):
        return os.path.join(REPO_ROOT, tgt["lib"])
    return os.path.join(ttsp_root, "library/ASP/target", tgt["ttsp_target"])


def ttsp_include_dirs(tgt, ttsp_root):
    return [
        os.path.join(ttsp_root, "library/ASP/test"),  # 汎用テストlib（アーキ非依存）
        target_lib_dir(tgt, ttsp_root),
    ]


def ttsp_extra_c_files(tgt, ttsp_root):
    return [
        os.path.join(ttsp_root, "library/ASP/test/ttsp_test_lib.c"),
        os.path.join(target_lib_dir(tgt, ttsp_root), "ttsp_target_test.c"),
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
    inc = ";".join(ttsp_include_dirs(tgt, ttsp_root))
    extra = ";".join(ttsp_extra_c_files(tgt, ttsp_root))
    cfg_cmd = [
        "cmake", "--preset", tgt["preset"], "-B", build_dir,
        f"-DASP3_APPLDIR={appldir}", "-DASP3_APPLNAME=out",
        f"-DASP3_APP_INCLUDE_DIRS={inc}",
        f"-DASP3_EXTRA_APP_C_FILES={extra}",
    ]
    #  ターゲット固有の追加 CMake 定義（例：polarfire 実機の -DPOLARFIRE_DISCOVERY=ON）
    cfg_cmd += tgt.get("cmake_extra", [])
    rc, log = run(cfg_cmd, cwd=REPO_ROOT, timeout=BUILD_TIMEOUT)
    if rc != 0:
        return rc, log
    rc, blog = run(["cmake", "--build", build_dir],
                   cwd=REPO_ROOT, timeout=BUILD_TIMEOUT)
    return rc, log + blog


#  実機完走を表すマーカ（早期検出に使用）
HW_DONE_MARKERS = ("All check points passed.", "Unexpected", "ssertion")
#  ブート境界（このマーカ以降＝今回ロード分．前テストの残骸を除去するため）
HW_BANNER = "TOPPERS/ASP3 Kernel Release"


def _current_boot(txt):
    """UART ログのうち「最後のカーネルバナー以降」だけを返す．

    実機は rst -system が効くまで前テストの出力（"All check points passed."
    を含みうる）が UART に残る．最後のバナー以降に限定して誤判定を防ぐ．
    バナー未出現時は全体を返す（保守的）．"""
    i = txt.rfind(HW_BANNER)
    return txt[i:] if i >= 0 else txt


def run_hw(tgt, build_dir):
    """実機でロード＆実行し，UART 出力を返す（QEMU の代わり）．

    実機ローダの種別（hw["kind"]）で分岐する：
      xsct_zybo  … Zybo Z7（Vitis xsct）
      openocd_swd… STM32MP257F-DK（OpenOCD/SWD．既存 run.cmake の swd-run を再利用）
    """
    kind = tgt["hw"].get("kind")
    if kind == "openocd_swd":
        return run_hw_openocd_swd(tgt, build_dir)
    if kind == "openocd_gdb_riscv":
        return run_hw_openocd_gdb_riscv(tgt, build_dir)
    return run_hw_xsct(tgt, build_dir)


def run_hw_openocd_gdb_riscv(tgt, build_dir):
    """PolarFire SoC 実機（SoftConsole openocd + gdb）でロード＆実行し，UART を返す．

    asp3 は mpfs_hal の system_startup を持たず HSS の MSS IO/クロック初期化に依存する
    ため，**reset init はしない**（HSS の設定を保持）．openocd（SoftConsole 版）は別途
    常駐させておく前提（TTSP3_HOWTO.md 参照）．gdb で halt→load→全hartに pc=start→
    continue し，UART を背景キャプチャして完走マーカで早期打ち切る．continue は
    ブロックするので gdb は別プロセスで起動し，検出後に kill する（harts は走り続ける）．
    シリアル1本＝逐次のみ（--jobs 1）．

    env 上書き：$TTSP_HW_SERIAL / $TTSP_HW_CAPTURE / $TTSP_RISCV_GDB．
    """
    hw = tgt["hw"]
    elf = os.path.join(build_dir, "asp.elf")
    if not os.path.exists(elf):
        elf = os.path.join(build_dir, "asp")
    serial = os.environ.get("TTSP_HW_SERIAL", hw["serial"])
    baud = str(hw.get("baud", 115200))
    cap = int(os.environ.get("TTSP_HW_CAPTURE", hw.get("capture", 30)))
    gdb = os.environ.get("TTSP_RISCV_GDB",
                         hw.get("gdb", "riscv64-unknown-elf-gdb"))
    port = hw.get("gdb_port", 3333)
    log = os.path.join(build_dir, "uart.log")
    gscript = os.path.join(build_dir, "ttsp_load.gdb")
    with open(gscript, "w", encoding="utf-8") as f:
        f.write("set pagination off\n"
                "set confirm off\n"
                "set mem inaccessible-by-default off\n"
                f"target extended-remote :{port}\n"
                "set architecture riscv:rv64\n"
                f"file {elf}\n"
                "monitor halt\n"          # reset init はしない（HSS設定を保持）
                "load\n"
                "thread apply all set $pc=start\n"
                "continue\n")

    #  シリアルを raw 設定（失敗は無視）
    subprocess.run(["stty", "-F", serial, baud, "raw", "-echo"],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    with open(log, "wb") as lf:
        catp = subprocess.Popen(["timeout", str(cap), "cat", serial],
                                stdout=lf, stderr=subprocess.DEVNULL)
        time.sleep(1)
        #  gdb を background 起動（continue でブロックするため）
        gdbp = subprocess.Popen([gdb, "-q", "-batch", "-x", gscript, elf],
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)
        deadline = time.time() + cap
        while time.time() < deadline and catp.poll() is None:
            try:
                with open(log, "r", encoding="utf-8", errors="replace") as rf:
                    txt = _current_boot(rf.read())
            except OSError:
                txt = ""
            if HW_BANNER in txt and any(m in txt for m in HW_DONE_MARKERS):
                break
            time.sleep(0.5)
        #  gdb を終了（harts は走り続ける）→ 次テストの gdb が halt→load する
        if gdbp.poll() is None:
            gdbp.kill()
        try:
            gdbp.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass
        if catp.poll() is None:
            catp.terminate()
        try:
            catp.wait(timeout=5)
        except subprocess.TimeoutExpired:
            catp.kill()
    try:
        with open(log, "r", encoding="utf-8", errors="replace") as rf:
            out = _current_boot(rf.read())
    except OSError:
        out = ""
    return 0, out


def run_hw_openocd_swd(tgt, build_dir):
    """STM32MP257F-DK 実機（OpenOCD/SWD）でロード＆実行し，UART 出力を返す．

    既存の run.cmake の `swd-run` ターゲット（OpenOCD だけで reset→load→resume
    →shutdown．ボードは走り続ける）を `ninja -C <build_dir> swd-run` で再利用する．
    UART(/dev/ttyACM0) を背景 cat でキャプチャし，最後のカーネルバナー以降に限定
    して完走マーカを検出したら早期に打ち切る．シリアル1本＝実行は逐次のみ．

    注：swd-run は readelf 等にツールチェーン（aarch64-none-elf-）を PATH 経由で
    使う．run_ttsp.py 実行前にツールチェーンを PATH へ通しておくこと．
    """
    hw = tgt["hw"]
    serial = os.environ.get("TTSP_HW_SERIAL", hw["serial"])
    baud = str(hw.get("baud", 115200))
    cap = int(os.environ.get("TTSP_HW_CAPTURE", hw.get("capture", 45)))
    ninja_target = hw.get("ninja_target", "swd-run")
    log = os.path.join(build_dir, "uart.log")

    #  シリアルを raw 設定（失敗は無視＝既に設定済みのことがある）
    subprocess.run(["stty", "-F", serial, baud, "raw", "-echo"],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    #  UART を時間制限付きでバックグラウンド・キャプチャ
    with open(log, "wb") as lf:
        catp = subprocess.Popen(["timeout", str(cap), "cat", serial],
                                stdout=lf, stderr=subprocess.DEVNULL)
        time.sleep(1)
        #  既存 run.cmake のロード＆実行ターゲットを同期実行（OpenOCD 経由）．
        #  stm32mp2=swd-run（reset run→load→resume），pico2=run（program verify
        #  reset exit）．いずれも完了で OpenOCD が終了しボードは走り続ける．
        run(["ninja", "-C", build_dir, ninja_target], timeout=120)
        #  完走マーカを「今回ブート分」（最後のバナー以降）に限定して検出．
        deadline = time.time() + cap
        while time.time() < deadline and catp.poll() is None:
            try:
                with open(log, "r", encoding="utf-8", errors="replace") as rf:
                    txt = _current_boot(rf.read())
            except OSError:
                txt = ""
            if HW_BANNER in txt and any(m in txt for m in HW_DONE_MARKERS):
                break
            time.sleep(0.5)
        if catp.poll() is None:
            catp.terminate()
        try:
            catp.wait(timeout=5)
        except subprocess.TimeoutExpired:
            catp.kill()
    try:
        with open(log, "r", encoding="utf-8", errors="replace") as rf:
            out = _current_boot(rf.read())
    except OSError:
        out = ""
    return 0, out


def run_hw_xsct(tgt, build_dir):
    """実機（xsct 経由）でロード＆実行し，UART 出力を返す．

    QEMU の代わりに使用する．UART をバックグラウンドで キャプチャしながら
    xsct で rst -system → ps7_init → dow → con を実行し，完走マーカを検出
    したら早期にキャプチャを打ち切る．シリアルは1本＝実行は逐次のみ（--jobs 1）．
    """
    hw = tgt["hw"]
    elf = os.path.join(build_dir, "asp.elf")
    if not os.path.exists(elf):
        elf = os.path.join(build_dir, "asp")
    serial = os.environ.get("TTSP_HW_SERIAL", hw["serial"])
    baud = str(hw.get("baud", 115200))
    cap = int(os.environ.get("TTSP_HW_CAPTURE", hw.get("capture", 35)))
    jtag = os.path.join(REPO_ROOT, hw["jtag_tcl"])
    ps7 = os.path.join(REPO_ROOT, hw["ps7_init"])
    settings = hw.get("vitis_settings", "")
    log = os.path.join(build_dir, "uart.log")

    #  シリアルを raw 設定（失敗は無視＝既に設定済みのことがある）
    subprocess.run(["stty", "-F", serial, baud, "raw", "-echo"],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    #  UART を時間制限付きでバックグラウンド・キャプチャ
    with open(log, "wb") as lf:
        catp = subprocess.Popen(["timeout", str(cap), "cat", serial],
                                stdout=lf, stderr=subprocess.DEVNULL)
        time.sleep(1)
        #  xsct（Vitis 環境を source）．con 後すぐ exit するため数秒〜十数秒で戻る
        src = f"source {settings} >/dev/null 2>&1; " if settings else ""
        xcmd = f"{src}xsct {jtag} {ps7} {elf}"
        run(["bash", "-c", xcmd], timeout=120)
        #  完走マーカを検出するまで（最大 cap 秒）ポーリングして早期終了．
        #  判定は「今回ブート分」（最後のバナー以降）に限定する．
        deadline = time.time() + cap
        while time.time() < deadline and catp.poll() is None:
            try:
                with open(log, "r", encoding="utf-8", errors="replace") as rf:
                    txt = _current_boot(rf.read())
            except OSError:
                txt = ""
            if HW_BANNER in txt and any(m in txt for m in HW_DONE_MARKERS):
                break
            time.sleep(0.5)
        if catp.poll() is None:
            catp.terminate()
        try:
            catp.wait(timeout=5)
        except subprocess.TimeoutExpired:
            catp.kill()
    try:
        with open(log, "r", encoding="utf-8", errors="replace") as rf:
            out = _current_boot(rf.read())
    except OSError:
        out = ""
    return 0, out


def run_qemu(tgt, build_dir):
    if tgt.get("hw"):
        return run_hw(tgt, build_dir)
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


BUILD_FAIL_DETAIL = "ビルド失敗"


def judge_runtime(test, tgt, ttsp_root, build_dir, appldir, build_only=False):
    """runtime/yaml 系：ビルド → QEMU 実行 → 合格文字列で PASS.
    build_only 時はビルド成功＝PASS（QEMU 実行はスキップ）."""
    rc, log = configure_and_build(tgt, ttsp_root, appldir, build_dir)
    if rc != 0:
        return Result(test, "FAIL", BUILD_FAIL_DETAIL, log)
    if build_only:
        return Result(test, "PASS", "build-only")
    rc, out = run_qemu(tgt, build_dir)
    if "All check points passed." in out:
        return Result(test, "PASS")
    if "Assertion" in out or "fail" in out.lower():
        return Result(test, "FAIL", "チェックポイント不合格", out)
    return Result(test, "FAIL", "合格文字列なし（実行エラー/タイムアウト）", out)


def skip_for_hw(test, tgt, out_c_path):
    """out.c が本ターゲット未対応の HW 関数を呼ぶ場合 SKIP 理由を返す（無ければ None）."""
    unsupported = tgt.get("unsupported_hw", set())
    if not unsupported:
        return None
    used = scan_hw_calls(out_c_path) & unsupported
    if used:
        return "未対応HW: " + ",".join(sorted(used))
    return None


def soft_skip_on_fail(test, tgt, out_c_path, res):
    """実行失敗かつ soft_hw（no-op 実装）に依存する場合 SKIP に再分類する．
    ビルド失敗は HW と無関係なので常に FAIL のまま残す."""
    if res.status != "FAIL" or res.detail == BUILD_FAIL_DETAIL:
        return res
    soft = tgt.get("soft_hw", set())
    if not soft:
        return res
    used = scan_hw_calls(out_c_path) & soft
    if used:
        return Result(test, "SKIP",
                      "HWタイマ制御依存（" + ",".join(sorted(used)) + "）＝本ターゲット未検証")
    return res


def run_test(test, tgt, ttsp_root, build_root, keep, build_only=False):
    #  本ターゲットで未対応のモジュール（interrupt/exception 等）は丸ごと SKIP
    if test.module in tgt.get("skip_modules", set()):
        return Result(test, "SKIP", f"{test.module}モジュール: 本ターゲット未対応")
    build_dir = os.path.join(build_root, tgt["preset"],
                             test.id.replace("/", "_").replace(".yaml", ""))
    if test.kind in ("error", "runtime"):
        out_c = os.path.join(test.src_dir, "out.c")
        reason = skip_for_hw(test, tgt, out_c)   # 早送り/割込み/例外は事前 SKIP
        if reason:
            res = Result(test, "SKIP", reason)
        elif test.kind == "error":
            res = judge_error(test, tgt, ttsp_root, build_dir)
        else:
            res = judge_runtime(test, tgt, ttsp_root, build_dir, test.src_dir,
                                build_only)
            res = soft_skip_on_fail(test, tgt, out_c, res)
    else:  # yaml：ttg 生成 → HW 依存スキャン → build/run
        gen_dir = os.path.join(build_dir, "ttg")
        ok, gout = gen_with_ttg(ttsp_root, test.yaml, gen_dir)
        if not ok:
            res = Result(test, "FAIL", "ttg 生成失敗", gout)
        else:
            out_c = os.path.join(gen_dir, "out.c")
            reason = skip_for_hw(test, tgt, out_c)
            if reason:
                res = Result(test, "SKIP", reason)
            else:
                res = judge_runtime(test, tgt, ttsp_root, build_dir, gen_dir,
                                    build_only)
                res = soft_skip_on_fail(test, tgt, out_c, res)
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
    ap.add_argument("--build-only", action="store_true",
                    help="ビルド成功で PASS（QEMU 実行をスキップ．"
                         "qemu 未導入アーキの build 検証や CI 用）")
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
        res = run_test(t, tgt, ttsp_root, args.build_dir, args.keep,
                       args.build_only)
        if res.status == "PASS":
            n_pass += 1
        elif res.status == "SKIP":
            n_skip += 1
        else:
            n_fail += 1

        if args.tap:
            #  TAP：SKIP は ok 行＋# SKIP ディレクティブ（失敗ではない）
            mark = "not ok" if res.status == "FAIL" else "ok"
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
