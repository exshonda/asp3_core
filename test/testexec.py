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
#  $Id: testexec.py (converted from testexec.rb) $
#

#
#		テストプログラムの実行スクリプト（Python版）
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
#		デフォルト		kernelとall
#		kernel			ディレクトリが作られているすべてのカーネル
#		kernel<数字>	指定したビルドオプションのカーネル
#		all				ディレクトリが作られているすべてのテストプログラム
#		<テスト名>		指定したテストプログラム
#
# 【ターゲット毎のビルドオプション】
#	ターゲット毎のビルドオプションを，TARGET_OPTIONSに作成する．異なる
#	テスト用のビルドオプションを，各行に記述する．
#
#	各行（最初の行が0）に記述するビルドオプション：
#		0		機能テストプログラム
#		1		性能評価プログラム
#		2		タイマドライバシミュレータを用いたテストプログラム
#		3		FPUを使用するテストプログラム（ARM向け）
#
# 【ターゲット毎の実行方法】
#	ターゲット上でテストプログラムを実行するための記述を，TARGET_RUNに
#	作成する．

import os
import re
import sys
import subprocess

#
#  テストプログラム毎に必要なオプションの定義
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
    "exttsk":   {"SRC": "test_exttsk", "CDL": "test_pf_bitkernel"},
    "flg1":     {"SRC": "test_flg1"},
    "hrt1":     {"SRC": "test_hrt1"},
    "int1":     {"SRC": "test_int1"},
    "mpf1":     {"SRC": "test_mpf1"},
    "mutex1":   {"SRC": "test_mutex1", "CDL": "test_pf_bitkernel"},
    "mutex2":   {"SRC": "test_mutex2", "CDL": "test_pf_bitkernel"},
    "mutex3":   {"SRC": "test_mutex3", "CDL": "test_pf_bitkernel"},
    "mutex4":   {"SRC": "test_mutex4", "CDL": "test_pf_bitkernel"},
    "mutex5":   {"SRC": "test_mutex5", "CDL": "test_pf_bitkernel"},
    "mutex6":   {"SRC": "test_mutex6", "CDL": "test_pf_bitkernel"},
    "mutex7":   {"SRC": "test_mutex7", "CDL": "test_pf_bitkernel"},
    "mutex8":   {"SRC": "test_mutex8", "CDL": "test_pf_bitkernel"},
    "notify1":  {"SRC": "test_notify1"},
    "pdq1":     {"SRC": "test_pdq1"},
    "raster1":  {"SRC": "test_raster1", "CDL": "test_pf_bitkernel"},
    "raster2":  {"SRC": "test_raster2"},
    "sem1":     {"SRC": "test_sem1"},
    "sem2":     {"SRC": "test_sem2"},
    "suspend1": {"SRC": "test_suspend1"},
    "sysman1":  {"SRC": "test_sysman1"},
    "sysstat1": {"SRC": "test_sysstat1"},
    "task1":    {"SRC": "test_task1", "CDL": "test_pf_bitkernel"},
    "tmevt1":   {"SRC": "test_tmevt1"},

    # メッセージバッファ機能拡張パッケージの機能テストプログラム
    "messagebuf1": {"SRC": "test_messagebuf1", "CDL": "test_pf_bitkernel"},
    "messagebuf2": {"SRC": "test_messagebuf2", "CDL": "test_pf_bitkernel"},

    # オーバランハンドラ機能拡張パッケージの機能テストプログラム
    "ovrhdr1":  {"SRC": "test_ovrhdr1"},
    "ovrhdr2":  {"SRC": "test_ovrhdr2"},
    "ovrhdr3":  {"TARGET": 2, "SRC": "simt_ovrhdr3",
                 "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST"},
    "ovrhdr4":  {"SRC": "test_ovrhdr4"},

    # 制約タスク拡張パッケージの機能テストプログラム
    "rstr1":    {"SRC": "test_rstr1"},
    "rstr2":    {"SRC": "test_rstr2"},

    # サブ優先度機能拡張パッケージの機能テストプログラム
    "subprio1": {"SRC": "test_subprio1"},
    "subprio2": {"SRC": "test_subprio2"},
    "subprio3": {"SRC": "test_subprio3"},

    # 優先度継承拡張パッケージの機能テストプログラム
    "inherit1": {"SRC": "test_inherit1", "CDL": "test_pf_bitkernel"},
    "inherit2": {"SRC": "test_inherit2", "CDL": "test_pf_bitkernel"},
    "inherit3": {"SRC": "test_inherit3", "CDL": "test_pf_bitkernel"},
    "inherit4": {"SRC": "test_inherit4", "CDL": "test_pf_bitkernel"},
    "inherit5": {"SRC": "test_inherit5", "CDL": "test_pf_bitkernel"},
    "inherit6": {"SRC": "test_inherit6", "CDL": "test_pf_bitkernel"},
    "inherit7": {"SRC": "test_inherit7", "CDL": "test_pf_bitkernel"},

    # タイマドライバシミュレータのテストプログラム
    "simtimer1": {"TARGET": 2, "SRC": "simt_simtimer1",
                  "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST"},

    # システム時刻管理機能テストプログラム
    "systim1": {"TARGET": 2, "SRC": "simt_systim1",
                "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST"},
    "systim2": {"TARGET": 2, "SRC": "simt_systim2",
                "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST"},
    "systim3": {"TARGET": 2, "SRC": "simt_systim3",
                "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST"},
    "systim4": {"TARGET": 2, "SRC": "simt_systim4",
                "DEFS": "-DHRT_CONFIG2 -DSIMTIM_TEST"},
    "systim1_64hrt": {"TARGET": 2, "SRC": "simt_systim1_64hrt",
                      "CFG": "simt_systim1", "DEFS": "-DHRT_CONFIG3 -DSIMTIM_TEST"},
    "systim2_64hrt": {"TARGET": 2, "SRC": "simt_systim2_64hrt",
                      "CFG": "simt_systim2", "DEFS": "-DHRT_CONFIG3 -DSIMTIM_TEST"},
    "systim3_64hrt": {"TARGET": 2, "SRC": "simt_systim3_64hrt",
                      "CFG": "simt_systim3", "DEFS": "-DHRT_CONFIG3 -DSIMTIM_TEST"},

    # ドリフト調整機能拡張パッケージのシステム時刻管理機能テストプログラム
    "drift1":        {"TARGET": 2, "SRC": "simt_drift1",
                      "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST"},
    "drift1_64hrt":  {"TARGET": 2, "SRC": "simt_drift1_64hrt",
                      "CFG": "simt_drift1", "DEFS": "-DHRT_CONFIG3 -DSIMTIM_TEST"},
    "drift1_64ops":  {"TARGET": 2, "SRC": "simt_drift1",
                      "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST -DUSE_64BIT_OPS"},
    "systim1_64ops": {"TARGET": 2, "SRC": "simt_systim1",
                      "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST -DUSE_64BIT_OPS"},
    "systim2_64ops": {"TARGET": 2, "SRC": "simt_systim2",
                      "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST -DUSE_64BIT_OPS"},
    "systim3_64ops": {"TARGET": 2, "SRC": "simt_systim3",
                      "DEFS": "-DHRT_CONFIG1 -DSIMTIM_TEST -DUSE_64BIT_OPS"},

    # 性能評価プログラム
    "perf0": {"TARGET": 1, "CDL": "perf_pf", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf1": {"TARGET": 1, "CDL": "perf_pf", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf2": {"TARGET": 1, "CDL": "perf_pf", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf3": {"TARGET": 1, "CDL": "perf_pf", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf4": {"TARGET": 1, "CDL": "perf_pf", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},
    "perf5": {"TARGET": 1, "CDL": "perf_pf", "NK_DEFS": "-DHIST_INVALIDATE_CACHE"},

    # ARM向けテストプログラム
    "arm_cpuexc1": {"SRC": "arm_cpuexc1", "SRCDIR": "arch/arm_gcc/test"},
    "arm_fpu1": {"TARGET": 3, "SRC": "arm_fpu1", "SRCDIR": "arch/arm_gcc/test"},
}

used_src_dir = "."
target_options = {}


def system(command):
    return subprocess.call(command, shell=True) == 0


#
#  カーネルライブラリの作成
#
def BuildKernel(target, mkdir_flag=False):
    if target not in target_options:
        return

    kernel_dir = "KERNELLIB" + str(target)
    if not os.path.isdir(kernel_dir):
        if mkdir_flag:
            os.mkdir(kernel_dir)
        else:
            return

    cwd = os.getcwd()
    os.chdir(kernel_dir)
    try:
        print(f"== building: {kernel_dir} ==")
        config_command = f"python3 {used_src_dir}/configure.py"
        config_command += " -f"
        config_command += f" {target_options[target]}"
        print(config_command)
        system(config_command)
        system("make libkernel.a")
        if os.path.isfile("Makefile.bak"):
            os.remove("Makefile.bak")
    finally:
        os.chdir(cwd)


#
#  全カーネルライブラリの作成
#
def BuildAllKernel():
    for target in target_options:
        BuildKernel(target)


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
        config_command = f"python3 {used_src_dir}/configure.py"
        if "TARGET" in test_spec:
            config_command += f" {target_options[test_spec['TARGET']]}"
        else:
            config_command += f" {target_options[0]}"
        if "SRCDIR" in test_spec:
            config_command += f" -a \"{used_src_dir}/{test_spec['SRCDIR']}" \
                              f" {used_src_dir}/test\""
        else:
            config_command += f" -a {used_src_dir}/test"

        if "DEFS" not in test_spec:
            if "TARGET" in test_spec:
                kernel_dir = "KERNELLIB" + str(test_spec["TARGET"])
            else:
                kernel_dir = "KERNELLIB0"
            if os.path.isdir("../" + kernel_dir):
                config_command += " -L ../" + kernel_dir
        if "SRC" in test_spec:
            config_command += f" -A {test_spec['SRC']}"
        else:
            config_command += f" -A {test}"
        if "CFG" in test_spec:
            config_command += f" -c {test_spec['CFG']}.cfg"
        if "CDL" in test_spec:
            config_command += f" -C {test_spec['CDL']}.cdl"
        else:
            config_command += " -C test_pf.cdl"
        if "SYSOBJ" in test_spec:
            config_command += " -S \"" \
                + " ".join(f + ".o" for f in test_spec["SYSOBJ"].split()) \
                + "\""
        if "APPLOBJ" in test_spec:
            config_command += " -U \"" \
                + " ".join(f + ".o" for f in test_spec["APPLOBJ"].split()) \
                + "\""
        if "OPTS" in test_spec:
            config_command += f" -o \"{test_spec['OPTS']}\""
        if "DEFS" in test_spec:
            config_command += f" -O \"{test_spec['DEFS']}\""
        if "NK_DEFS" in test_spec:
            config_command += f" -O \"{test_spec['NK_DEFS']}\""
        print(config_command)
        system(config_command)

        #  【asp3_core変更】非TECS環境向け：テストプログラムの.cfgが
        #  INCLUDEするtecsgen.cfgのスタブを生成する（非TECS版システム
        #  サービス ユーザーズマニュアル §2.7の方法）
        if not os.path.isfile("tecsgen.cfg"):
            with open("tecsgen.cfg", "w", encoding="utf-8") as f:
                f.write('INCLUDE("syssvc/syslog.cfg");\n'
                        'INCLUDE("syssvc/banner.cfg");\n'
                        'INCLUDE("syssvc/serial.cfg");\n')

        system("make")
        if os.path.isfile("Makefile.bak"):
            os.remove("Makefile.bak")
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
        else:
            system("./asp")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムの実行
#
def ExecAllTest():
    for test, test_spec in TEST_SPEC.items():
        ExecTest(test, test_spec)


#
#  カーネルライブラリのクリーン
#
def CleanKernel(target):
    if target not in target_options:
        return

    kernel_dir = "KERNELLIB" + str(target)
    if os.path.isdir(kernel_dir):
        cwd = os.getcwd()
        os.chdir(kernel_dir)
        try:
            system("make clean")
        finally:
            os.chdir(cwd)


#
#  全カーネルライブラリのクリーン
#
def CleanAllKernel():
    for target in target_options:
        CleanKernel(target)


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
        system("make clean")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムのクリーン
#
def CleanAllTest():
    for test, test_spec in TEST_SPEC.items():
        CleanTest(test, test_spec)


def main():
    global used_src_dir, target_options

    #
    #  ソースディレクトリ名を取り出す
    #
    m = re.match(r"^(.*)\/test\/testexec", sys.argv[0])
    if m:
        src_dir = m.group(1)
    else:
        src_dir = "."

    if re.match(r"^\/", src_dir):
        used_src_dir = src_dir
    else:
        used_src_dir = "../" + src_dir

    #
    #  ターゲット依存のオプションを読む
    #
    target_options = {}
    with open("TARGET_OPTIONS", encoding="utf-8") as file:
        for index, line in enumerate(file):
            line = line.rstrip("\n")
            if line != "":
                target_options[index] = line

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

        elif param == "kernel":
            if clean_flag:
                CleanAllKernel()
            else:
                if not exec_only:
                    BuildAllKernel()
                # カーネルには，execはない
            proc_flag = True

        elif re.match(r"^kernel([0-9]+)$", param):
            target = int(re.match(r"^kernel([0-9]+)$", param).group(1))
            if clean_flag:
                CleanKernel(target)
            else:
                if not exec_only:
                    BuildKernel(target, True)
                # カーネルには，execはない
            proc_flag = True

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
        # デフォルトの処理対象（kernelとall）
        if clean_flag:
            CleanAllKernel()
            CleanAllTest()
        else:
            if not exec_only:
                BuildAllKernel()
                BuildAllTest()
            if not build_only:
                ExecAllTest()


if __name__ == "__main__":
    main()
