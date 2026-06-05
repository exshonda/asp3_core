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
#  $Id: testcfg.py (converted from testcfg.rb) $
#

#
#  testcfg.rb のPython版（コンフィギュレータのテストランナ）
#

import os
import re
import sys
import subprocess

#  オプションの定義
#
#  -c，--copy			生成されたファイルをテスト期待値とする

#
#  テストプログラム毎に必要なオプションの定義
#
#  【asp3_core変更】非TECSデフォルト環境では，TARGET_OPTIONSに以下を
#  指定してシステムサービスなしの素のビルドとする：
#    -T dummy_gcc OMIT_DEFAULT_SYSSVC -O "-DTOPPERS_OMIT_SYSLOG"
#  （OMIT_DEFAULT_SYSSVC＝syssvcオブジェクト自動付与の抑止，
#    TOPPERS_OMIT_SYSLOG＝カーネル内LOGマクロの無効化）
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

copy_flag = False
src_dir = "."
used_src_dir = "."
target_options = {}


def system(command):
    return subprocess.call(command, shell=True) == 0


def obj_dir_name(test):
    return test.upper().replace("_", "-")


#
#  カーネルライブラリの作成
#
def BuildKernel():
    if not os.path.isdir("KERNELLIB"):
        os.mkdir("KERNELLIB")

    cwd = os.getcwd()
    os.chdir("KERNELLIB")
    try:
        print("== building: KERNELLIB ==")
        config_command = f"python3 {used_src_dir}/configure.py"
        config_command += f" {target_options[0]}"
        config_command += " -c empty.cfg"
        print(config_command)
        system(config_command)
        system("touch empty.cfg")
        system("make libkernel.a")
        if os.path.isfile("Makefile.bak"):
            os.remove("Makefile.bak")
    finally:
        os.chdir(cwd)


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
        system("rm *.timestamp")

        print(f"== building: {obj_dir} ==")
        config_command = f"python3 {used_src_dir}/configure.py"
        if "TARGET" in test_spec:
            config_command += f" {target_options[test_spec['TARGET']]}"
        else:
            config_command += f" {target_options[0]}"
        config_command += f" -a {used_src_dir}/test_cfg"

        if "TARGET" not in test_spec or test_spec["TARGET"] == 0:
            config_command += " -L ../KERNELLIB"
        if "SRC" in test_spec:
            config_command += f" -A {test_spec['SRC']}"
        else:
            config_command += f" -A {test}"
        if "CFG" in test_spec:
            config_command += f" -c {test_spec['CFG']}.cfg"
        if "SYSOBJ" in test_spec:
            config_command += " -S \"" \
                + " ".join(f + ".o" for f in test_spec["SYSOBJ"].split()) \
                + "\""
        if "APPLOBJ" in test_spec:
            config_command += " -U \"" \
                + " ".join(f + ".o" for f in test_spec["APPLOBJ"].split()) \
                + "\""
        if "DEFS" in test_spec:
            config_command += f" -O \"{test_spec['DEFS']}\""
        if "OPTS" in test_spec:
            config_command += f" {test_spec['OPTS']}"
        print(config_command)
        system(config_command)

        if "ML_MANUAL" in test_spec:
            # 手動メモリ配置の場合は，リンカスクリプトをコピーする
            cp_command = f"cp {used_src_dir}/test_cfg/{test}/ldscript.ld ."
            print(cp_command)
            system(cp_command)

        make_commands = [
            "make cfg1_out.c 2> error.txt",
            "make cfg1_out.syms",
            "make kernel_cfg.c 2>> error.txt",
            "make asp.syms",
            "make 2>> error.txt",
        ]
        for command in make_commands:
            status = system(command)
            if not status:
                break

        if os.path.isfile("Makefile.bak"):
            os.remove("Makefile.bak")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムの作成
#
def BuildAllTest():
    for test, test_spec in CFG_TEST_SPEC.items():
        BuildTest(test, test_spec)
    for test, test_spec in PASS3_TEST_SPEC.items():
        BuildTest(test, test_spec)
    for test, test_spec in PASS2_TEST_SPEC.items():
        BuildTest(test, test_spec)
    for test, test_spec in PASS1_TEST_SPEC.items():
        BuildTest(test, test_spec)


#
#  ファイルの比較
#
def diffFile(test, filename, obj_dir):
    diff_command = f"diff {used_src_dir}/test_cfg/{test}/{filename} {filename}"
    print(diff_command)
    ret = system(diff_command)
    if not ret:
        if copy_flag:
            cp_command = f"cp {filename} {used_src_dir}/test_cfg/{test}/"
            print(cp_command)
            system(cp_command)
        else:
            print(f"#TODO# cp {obj_dir}/{filename} {src_dir}/test_cfg/{test}/")


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
        if os.path.isfile(f"{used_src_dir}/test_cfg/{test}/error.txt"):
            diffFile(test, "error.txt", obj_dir)
        elif os.path.getsize("error.txt") > 0:
            print("cat error.txt")
            system("cat error.txt")

        # 生成ファイルのチェック
        if gen_flag:
            diffFile(test, "kernel_cfg.h", obj_dir)
            diffFile(test, "kernel_cfg.c", obj_dir)
    finally:
        os.chdir(cwd)


#
#  全テストプログラムのテスト結果のチェック
#
def ExecAllTest():
    for test, test_spec in CFG_TEST_SPEC.items():
        CfgTest(test, test_spec, True)
    for test, test_spec in PASS3_TEST_SPEC.items():
        CfgTest(test, test_spec)
    for test, test_spec in PASS2_TEST_SPEC.items():
        CfgTest(test, test_spec)
    for test, test_spec in PASS1_TEST_SPEC.items():
        CfgTest(test, test_spec)


#
#  カーネルライブラリのクリーン
#
def CleanKernel():
    if os.path.isdir("KERNELLIB"):
        cwd = os.getcwd()
        os.chdir("KERNELLIB")
        try:
            system("make clean")
        finally:
            os.chdir(cwd)


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
        system("make clean")
        system("rm error.txt")
    finally:
        os.chdir(cwd)


#
#  全テストプログラムのクリーン
#
def CleanAllTest():
    for test, test_spec in CFG_TEST_SPEC.items():
        CleanTest(test, test_spec)
    for test, test_spec in PASS3_TEST_SPEC.items():
        CleanTest(test, test_spec)
    for test, test_spec in PASS2_TEST_SPEC.items():
        CleanTest(test, test_spec)
    for test, test_spec in PASS1_TEST_SPEC.items():
        CleanTest(test, test_spec)


def main():
    global copy_flag, src_dir, used_src_dir, target_options

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
    #  ソースディレクトリ名を取り出す
    #
    m = re.match(r"^(.*)\/test_cfg\/testcfg", sys.argv[0])
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
            target_options[index] = line.rstrip("\n")

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

        elif param == "kernel":
            if clean_flag:
                CleanKernel()
            else:
                if not exec_only:
                    BuildKernel()
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
            for spec, gen_flag in ((CFG_TEST_SPEC, True), (PASS3_TEST_SPEC, False),
                                   (PASS2_TEST_SPEC, False), (PASS1_TEST_SPEC, False)):
                if param in spec:
                    if clean_flag:
                        CleanTest(param, spec[param])
                    else:
                        if not exec_only:
                            BuildTest(param, spec[param], True)
                        if not build_only:
                            CfgTest(param, spec[param], gen_flag)
                    break
            else:
                print(f"invalid parameter: {param}")
            proc_flag = True

    if not proc_flag:
        # デフォルトの処理対象（kernelとall）
        if clean_flag:
            CleanKernel()
            CleanAllTest()
        else:
            if not exec_only:
                BuildKernel()
                BuildAllTest()
            if not build_only:
                ExecAllTest()


if __name__ == "__main__":
    main()
