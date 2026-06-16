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
#  $Id: testexec.py (CMake-based test runner) $
#

#
#		テストプログラムの実行スクリプト（CMake版）
#
# 【実行方法】
#	testexec.py <処理内容> <処理対象>
#
#	処理内容：
#		デフォルト		buildとexec
#		build			ビルドのみ
#		exec			実行のみ
#		clean			クリーン処理
#
#	処理対象：
#		デフォルト		all
#		all				ディレクトリが作られているすべてのテストプログラム
#		<テスト名>		指定したテストプログラム
#
# 【ターゲットの指定】
#	CMakeのconfigure引数を，TARGET_OPTIONSの1行目に記述する．
#	例：
#		--preset m33-qemu
#		-G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake -DASP3_TARGET=mps2_an505_gcc
#
#	テストプログラム共通で必要な test_svc.c は自動で追加される．
#
# 【ターゲット毎の実行方法】
#	ターゲット上でテストプログラムを実行するための記述を，TARGET_RUNに
#	作成する（ビルドディレクトリ内で実行される．実行ファイルは asp.elf
#	または asp）．TARGET_RUNが無い場合は ./asp（ホスト実行）を試みる．

import os
import re
import sys
import subprocess

#
#  テストプログラム毎に必要なオプションの定義
#
#  SRC：ソースファイル名（省略時はテスト名）／CFG：.cfgファイル名
#  SRCDIR：testディレクトリ以外のソース位置／DEFS・NK_DEFS：追加マクロ定義
#  SYSOBJ：追加するシステムサービスソース（test_svcは自動付与）
#
TEST_SPEC = {
    # 機能テストプログラム
    "cpuexc1":  {"SRC": "test_cpuexc1", "CFG": "test_cpuexc"},
    "cpuexc2":  {"SRC": "test_cpuexc2", "CFG": "test_cpuexc"},
    "cpuexc3":  {"SRC": "test_cpuexc3", "CFG": "test_cpuexc"},
    "cpuexc4":  {"SRC": "test_cpuexc4", "CFG": "test_cpuexc"},
    "cpuexc5":  {"SRC": "test_cpuexc5", "CFG": "test_cpuexc"},
    "cpuexc6":  {"SRC": "test_cpuexc6", "CFG": "test_cpuexc"},
    "cpuexc7":  {"SRC": "test_cpuexc7", "CFG": "test_cpuexc"},
    "cpuexc8":  {"SRC": "test_cpuexc8", "CFG": "test_cpuexc"},
    "cpuexc9":  {"SRC": "test_cpuexc9", "CFG": "test_cpuexc"},
    "cpuexc10": {"SRC": "test_cpuexc10", "CFG": "test_cpuexc"},
    "dlynse":   {"SRC": "test_dlynse"},
    "dtq1":     {"SRC": "test_dtq1"},
    "exttsk":   {"SRC": "test_exttsk"},
    "flg1":     {"SRC": "test_flg1"},
    "hrt1":     {"SRC": "test_hrt1"},
    "int1":     {"SRC": "test_int1"},
    "mpf1":     {"SRC": "test_mpf1"},
    "mutex1":   {"SRC": "test_mutex1"},
    "mutex2":   {"SRC": "test_mutex2"},
    "mutex3":   {"SRC": "test_mutex3"},
    "mutex4":   {"SRC": "test_mutex4"},
    "mutex5":   {"SRC": "test_mutex5"},
    "mutex6":   {"SRC": "test_mutex6"},
    "mutex7":   {"SRC": "test_mutex7"},
    "mutex8":   {"SRC": "test_mutex8"},
    "notify1":  {"SRC": "test_notify1"},
    "pdq1":     {"SRC": "test_pdq1"},
    "raster1":  {"SRC": "test_raster1"},
    "raster2":  {"SRC": "test_raster2"},
    "sem1":     {"SRC": "test_sem1"},
    "sem2":     {"SRC": "test_sem2"},
    "suspend1": {"SRC": "test_suspend1"},
    "sysman1":  {"SRC": "test_sysman1"},
    "sysstat1": {"SRC": "test_sysstat1"},
    "task1":    {"SRC": "test_task1"},
    "tmevt1":   {"SRC": "test_tmevt1"},

    # メッセージバッファ機能拡張パッケージの機能テストプログラム
    "messagebuf1": {"SRC": "test_messagebuf1"},
    "messagebuf2": {"SRC": "test_messagebuf2"},

    # オーバランハンドラ機能拡張パッケージの機能テストプログラム
    "ovrhdr1":  {"SRC": "test_ovrhdr1"},
    "ovrhdr2":  {"SRC": "test_ovrhdr2"},
    "ovrhdr4":  {"SRC": "test_ovrhdr4"},

    # 制約タスク拡張パッケージの機能テストプログラム
    "rstr1":    {"SRC": "test_rstr1"},
    "rstr2":    {"SRC": "test_rstr2"},

    # サブ優先度機能拡張パッケージの機能テストプログラム
    "subprio1": {"SRC": "test_subprio1"},
    "subprio2": {"SRC": "test_subprio2"},
    "subprio3": {"SRC": "test_subprio3"},

    # 優先度継承拡張パッケージの機能テストプログラム
    "inherit1": {"SRC": "test_inherit1"},
    "inherit2": {"SRC": "test_inherit2"},
    "inherit3": {"SRC": "test_inherit3"},
    "inherit4": {"SRC": "test_inherit4"},
    "inherit5": {"SRC": "test_inherit5"},
    "inherit6": {"SRC": "test_inherit6"},
    "inherit7": {"SRC": "test_inherit7"},

    # 性能評価プログラム
    "perf0": {"SYSOBJ": "histogram", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf1": {"SYSOBJ": "histogram", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf2": {"SYSOBJ": "histogram", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf3": {"SYSOBJ": "histogram", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf4": {"SYSOBJ": "histogram", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf5": {"SYSOBJ": "histogram", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},

    # ARM向けテストプログラム
    "arm_cpuexc1": {"SRC": "arm_cpuexc1", "SRCDIR": "arch/arm_gcc/test"},
    "arm_fpu1": {"SRC": "arm_fpu1", "SRCDIR": "arch/arm_gcc/test",
                 "DEFS": "-DUSE_ARM_FPU_ALWAYS"},
}

#  ソースルート（本スクリプトの位置から決定）
src_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
target_options = ""


def system(command):
    return subprocess.call(command, shell=True) == 0


#
#  追加マクロ定義（-DXXX形式）をCMakeリスト形式へ変換
#
def defs_to_cmake_list(*defs_strings):
    items = []
    for defs in defs_strings:
        if not defs:
            continue
        for d in defs.split():
            items.append(re.sub(r"^-D", "", d))
    return ";".join(items)


#
#  テストプログラムの作成
#
def BuildTest(test, test_spec, mkdir_flag=False):
    test_name = test.upper()
    obj_dir = f"OBJ-{test_name}"

    if not os.path.isdir(obj_dir):
        if mkdir_flag:
            os.mkdir(obj_dir)
        else:
            return

    cwd = os.getcwd()
    os.chdir(obj_dir)
    try:
        print(f"== building: {test_name} ==")

        src_abs = src_root
        applname = test_spec.get("SRC", test)

        config_command = f"cmake -S {src_abs} -B . {target_options}"
        if "SRCDIR" in test_spec:
            config_command += f" -DASP3_APPLDIR={src_abs}/{test_spec['SRCDIR']}"
            config_command += f" -DASP3_APP_INCLUDE_DIRS={src_abs}/test"
        else:
            config_command += f" -DASP3_APPLDIR={src_abs}/test"
        config_command += f" -DASP3_APPLNAME={applname}"

        #  .cfgファイル名がソース名と異なる場合（cpuexc系等）
        if "CFG" in test_spec:
            config_command += f" -DASP3_APPCFGNAME={test_spec['CFG']}"

        #  テストプログラム共通のtest_svcと，スペック指定の追加ソース
        extra_files = [f"{src_abs}/syssvc/test_svc.c"]
        for f in test_spec.get("SYSOBJ", "").split():
            extra_files.append(f"{src_abs}/syssvc/{f}.c")
        config_command += " \"-DASP3_EXTRA_APP_C_FILES=" + ";".join(extra_files) + "\""

        defs = defs_to_cmake_list(test_spec.get("DEFS"), test_spec.get("NK_DEFS"))
        if defs:
            config_command += f" \"-DASP3_EXTRA_COMPILE_DEFS={defs}\""

        print(config_command)
        if system(config_command):
            system("cmake --build .")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムの作成
#
def BuildAllTest():
    for test, test_spec in TEST_SPEC.items():
        BuildTest(test, test_spec)


#
#  テストプログラムの実行
#
def ExecTest(test, test_spec):
    test_name = test.upper()
    obj_dir = f"OBJ-{test_name}"

    if not os.path.isdir(obj_dir):
        return

    cwd = os.getcwd()
    os.chdir(obj_dir)
    try:
        print(f"== executing: {test_name} ==")
        if os.path.isfile("../TARGET_RUN"):
            with open("../TARGET_RUN", encoding="utf-8") as f:
                system(f.read())
        elif os.path.isfile("asp"):
            system("./asp")
        else:
            system("./asp.elf")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムの実行
#
def ExecAllTest():
    for test, test_spec in TEST_SPEC.items():
        ExecTest(test, test_spec)


#
#  テストプログラムのクリーン
#
def CleanTest(test, test_spec):
    test_name = test.upper()
    obj_dir = f"OBJ-{test_name}"

    if not os.path.isdir(obj_dir):
        return

    cwd = os.getcwd()
    os.chdir(obj_dir)
    try:
        system("cmake --build . --target clean")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムのクリーン
#
def CleanAllTest():
    for test, test_spec in TEST_SPEC.items():
        CleanTest(test, test_spec)


def main():
    global target_options

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

    for param in sys.argv[1:]:
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
            if param in TEST_SPEC:
                if clean_flag:
                    CleanTest(param, TEST_SPEC[param])
                else:
                    if not exec_only:
                        BuildTest(param, TEST_SPEC[param], True)
                    if not build_only:
                        ExecTest(param, TEST_SPEC[param])
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
