#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  TOPPERS Software
#      Toyohashi Open Platform for Embedded Real-Time Systems
#
#  Copyright (C) 2015-2026 by Embedded and Real-Time Systems Laboratory
#              Graduate School of Information Science, Nagoya Univ., JAPAN
#
#  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
#  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
#  トウェアは無保証で提供される．
#
#  $Id: testcfg.py (CMake-based cfg test runner) $
#

#
#		コンフィギュレータのテストランナ（CMake版）
#
#  test_cfg/ のテストプログラムをCMakeでビルドし，cfgのエラーメッセージ
#  と生成ファイル（kernel_cfg.c/h）を期待値と比較する．
#
# 【実行方法】
#	testcfg.py [-c] <処理内容> <処理対象>
#
#	-c，--copy		生成されたファイル・エラー出力をテスト期待値とする
#
#	処理内容：デフォルト=buildとexec／build／exec／clean
#	処理対象：デフォルト=all／all／<テスト名>
#
# 【ターゲットの指定】
#	CMakeのconfigure引数をTARGET_OPTIONSの1行目に記述する．
#	素のカーネルビルド（dummyターゲット）の例：
#	  -G Ninja -DASP3_TARGET=dummy_gcc -DASP3_OMIT_DEFAULT_SYSSVC=ON -DASP3_EXTRA_COMPILE_DEFS=TOPPERS_OMIT_SYSLOG
#
# 【エラー出力の比較方法】
#	ビルドのstderrを取得し，以下の正規化を行ってから期待値と比較する：
#	  - make/ninja等のビルドツール由来の行を除去
#	  - ファイルパスを test_cfg/ 起点に正規化
#	これによりビルドシステム（make/CMake）や実行場所に依存しない比較とする．

import os
import re
import sys
import subprocess

#
#  テストプログラム毎に必要なオプションの定義
#
CFG_TEST_SPEC = {
    # ビルドに成功するテスト
    "cfg_all1": {},
}

PASS3_TEST_SPEC = {
    # パス3でエラーになるテスト
    "pass3_all1": {},
}

PASS2_TEST_SPEC = {
    # パス2でエラーになるテスト
    "pass2_cfg1": {},
    "pass2_int1": {},
    "pass2_obj1": {},
    "pass2_obj2": {},
    "pass2_task1": {},

    # サブ優先度機能拡張パッケージの機能テストプログラム
    "pass2_subprio1": {},
}

PASS1_TEST_SPEC = {
    # パス1でエラーになるテスト
    "pass1_cfg1": {},
}

ALL_SPECS = (
    (CFG_TEST_SPEC, True),
    (PASS3_TEST_SPEC, False),
    (PASS2_TEST_SPEC, False),
    (PASS1_TEST_SPEC, False),
)

#  ソースルート（本スクリプトの位置から決定）
src_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
copy_flag = False
target_options = ""


def system(command):
    return subprocess.call(command, shell=True) == 0


def obj_dir_name(test):
    return test.upper().replace("_", "-")


#
#  エラー出力の正規化
#
#  エラーメッセージ行（"... error: ..."）のみを抽出し，パスを
#  test_cfg/ 起点に正規化する．ビルドツール（make/ninja）由来の行や
#  コンパイラの進捗・引用行はここで除外される．
#
def normalize_error_lines(text):
    lines = []
    seen = set()
    for line in text.splitlines():
        if not re.search(r"(^|: )error: ", line):
            continue
        if re.match(r"^(make(\[[0-9]+\])?: |ninja: |FAILED: )", line):
            continue
        #  パスを test_cfg/ 起点に正規化（"path/to/test_cfg/xxx.cfg:NN:" 等）
        line = re.sub(r"(^|[\s\"'(])[^\s\"'(]*?/(test_cfg/)", r"\1\2", line)
        #  offset.h用とkernel_cfg用でcfgのパス2が2回走るため，
        #  同一エラーの重複を除去する（Makefile版は1回目で停止）
        if line in seen:
            continue
        seen.add(line)
        lines.append(line)
    return "\n".join(lines) + ("\n" if lines else "")


#
#  テストプログラムの作成
#
def BuildTest(test, test_spec, mkdir_flag=False):
    obj_dir = obj_dir_name(test)

    if not os.path.isdir(obj_dir):
        if mkdir_flag:
            os.mkdir(obj_dir)
        else:
            return

    cwd = os.getcwd()
    os.chdir(obj_dir)
    try:
        print(f"== building: {obj_dir} ==")

        config_command = f"cmake -S {src_root} -B . {target_options}"
        config_command += f" -DASP3_APPLDIR={src_root}/test_cfg"
        config_command += f" -DASP3_APPLNAME={test_spec.get('SRC', test)}"
        if "CFG" in test_spec:
            config_command += f" -DASP3_APPCFGNAME={test_spec['CFG']}"
        print(config_command)
        if not system(config_command):
            return

        #  ビルドし，出力（cfgのエラーメッセージを含む）をerror.txtに
        #  取得する（ninjaは子プロセスの出力をstdoutに集約する）
        with open("error.txt", "w", encoding="utf-8") as err:
            subprocess.call("cmake --build .", shell=True,
                            stderr=subprocess.STDOUT, stdout=err)
    finally:
        os.chdir(cwd)


#
#  全テストプログラムの作成
#
def BuildAllTest():
    for spec, _ in ALL_SPECS:
        for test, test_spec in spec.items():
            BuildTest(test, test_spec)


#
#  ファイルの比較（正規化なし：生成ファイル用）
#
def diffFile(test, expected, actual, obj_dir):
    diff_command = f"diff {expected} {actual}"
    print(diff_command)
    ret = system(diff_command)
    if not ret:
        if copy_flag:
            cp_command = f"cp {actual} {expected}"
            print(cp_command)
            system(cp_command)
        else:
            print(f"#TODO# cp {obj_dir}/{actual} {expected}")


#
#  エラー出力の比較（正規化あり）
#
def diffError(test, obj_dir):
    expected_path = f"{src_root}/test_cfg/{test}/error.txt"
    if os.path.isfile("error.txt"):
        with open("error.txt", encoding="utf-8") as f:
            actual = normalize_error_lines(f.read())
    else:
        actual = ""

    if os.path.isfile(expected_path):
        with open(expected_path, encoding="utf-8") as f:
            expected = normalize_error_lines(f.read())
        if expected != actual:
            print(f"== error.txt mismatch (normalized) ==")
            import difflib
            sys.stdout.writelines(difflib.unified_diff(
                expected.splitlines(keepends=True),
                actual.splitlines(keepends=True),
                fromfile="expected", tofile="actual"))
            if copy_flag:
                with open(expected_path, "w", encoding="utf-8") as f:
                    f.write(actual)
                print(f"== copied normalized error.txt to {expected_path} ==")
            else:
                print(f"#TODO# update {expected_path}")
    elif actual != "":
        print("== unexpected errors ==")
        print(actual)


#
#  テスト結果のチェック
#
def CfgTest(test, test_spec, gen_flag=False):
    obj_dir = obj_dir_name(test)
    if not os.path.isdir(obj_dir):
        return

    cwd = os.getcwd()
    os.chdir(obj_dir)
    try:
        print(f"== checking: {obj_dir} ==")

        # エラーメッセージのチェック
        diffError(test, obj_dir)

        # 生成ファイルのチェック
        if gen_flag:
            diffFile(test, f"{src_root}/test_cfg/{test}/kernel_cfg.h",
                     "generated/kernel_cfg.h", obj_dir)
            diffFile(test, f"{src_root}/test_cfg/{test}/kernel_cfg.c",
                     "generated/kernel_cfg.c", obj_dir)
    finally:
        os.chdir(cwd)


#
#  全テストプログラムのテスト結果のチェック
#
def ExecAllTest():
    for spec, gen_flag in ALL_SPECS:
        for test, test_spec in spec.items():
            CfgTest(test, test_spec, gen_flag)


#
#  テストプログラムのクリーン
#
def CleanTest(test, test_spec):
    obj_dir = obj_dir_name(test)
    if not os.path.isdir(obj_dir):
        return

    cwd = os.getcwd()
    os.chdir(obj_dir)
    try:
        system("cmake --build . --target clean")
        if os.path.isfile("error.txt"):
            os.remove("error.txt")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムのクリーン
#
def CleanAllTest():
    for spec, _ in ALL_SPECS:
        for test, test_spec in spec.items():
            CleanTest(test, test_spec)


def find_spec(test):
    for spec, gen_flag in ALL_SPECS:
        if test in spec:
            return spec[test], gen_flag
    return None, False


def main():
    global copy_flag, target_options

    #
    #  オプションの処理
    #
    params = []
    for arg in sys.argv[1:]:
        if arg in ("-c", "--copy"):
            copy_flag = True
        else:
            params.append(arg)

    #
    #  ターゲットの指定（CMakeのconfigure引数）を読む
    #
    with open("TARGET_OPTIONS", encoding="utf-8") as file:
        target_options = file.readline().rstrip("\n")

    #
    #  パラメータで指定された処理の実行
    #
    build_only = False
    exec_only = False
    clean_flag = False
    proc_flag = False

    for param in params:
        if param == "build":
            build_only = True
        elif param == "exec":
            exec_only = True
        elif param == "clean":
            clean_flag = True

        elif param == "all":
            if clean_flag:
                CleanAllTest()
            else:
                if not exec_only:
                    BuildAllTest()
                if not build_only:
                    ExecAllTest()
            proc_flag = True

        else:
            test_spec, gen_flag = find_spec(param)
            if test_spec is not None:
                if clean_flag:
                    CleanTest(param, test_spec)
                else:
                    if not exec_only:
                        BuildTest(param, test_spec, True)
                    if not build_only:
                        CfgTest(param, test_spec, gen_flag)
            else:
                print(f"invalid parameter: {param}")
            proc_flag = True

    if not proc_flag:
        # デフォルトの処理対象（all）
        if clean_flag:
            CleanAllTest()
        else:
            if not exec_only:
                BuildAllTest()
            if not build_only:
                ExecAllTest()


if __name__ == "__main__":
    main()
