#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  TOPPERS Software
#      Toyohashi Open Platform for Embedded Real-Time Systems
#
#  Copyright (C) 2003-2018 by Embedded and Real-Time Systems Laboratory
#              Graduate School of Information Science, Nagoya Univ., JAPAN
#
#  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
#  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
#  トウェアは無保証で提供される．
#
#  $Id: makerelease.py (converted from makerelease.rb) $
#

#
#		リリースアーカイブ作成ツール（Python版）
#

import os
import re
import sys
import time
import subprocess

#
#  オプションの定義
#
#  -e <dirname>			アーカイブファイルを展開して削除する．dirname
#						は展開するディレクトリ名（省略可能）．

package = ""
e_package = 0
version = ""
file_list = []
prefix = ""


def system(command):
    return subprocess.call(command, shell=True) == 0


#
#  ".."を含むパスの整形
#
#  例）"yyy/../zzz" → "zzz"
#  例）"xxx/yyy/../zzz" → "xxx/zzz"
#
def canonical_path(path):
    while True:
        path, n = re.subn(r"([^\/]+\/\.\.\/)", "", path, count=1)
        if n == 0:
            break
    return path


#
#  ファイルの読み込み
#
def read_file(in_file_name):
    global package, e_package, version

    base_directory = os.path.dirname(in_file_name)
    if base_directory in (".", ""):
        base_directory = ""
    else:
        base_directory += "/"

    try:
        in_file = open(in_file_name, encoding="utf-8")
    except OSError as e:
        sys.exit(str(e))

    for line in in_file:
        line = line.rstrip("\n")
        line = re.sub(r"\s*\#.*$", "", line)
        if re.match(r"^\s*$", line):
            continue

        m = re.match(r"^E_PACKAGE[ \t]+(.*)$", line)
        if m:
            if package != "":
                sys.exit("Duplicated E_PACKAGE directive.")
            else:
                package = m.group(1)
                e_package = 1
            continue
        m = re.match(r"^PACKAGE[ \t]+(.*)$", line)
        if m:
            if package != "":
                if not e_package and package != m.group(1):
                    sys.exit("Inconsistent PACKAGE directive.")
            else:
                package = m.group(1)
            continue
        m = re.match(r"^VERSION[ \t]+(.*)$", line)
        if m:
            if version != "":
                if not e_package and version != m.group(1):
                    sys.exit("Inconsistent VERSION directive.")
            else:
                version = m.group(1)
                if re.search(r"%date", version):
                    current_time = time.localtime()
                    vdate = "%04d%02d%02d" % (current_time.tm_year,
                                              current_time.tm_mon,
                                              current_time.tm_mday)
                    version = re.sub(r"%date", vdate, version, count=1)
            continue
        m = re.match(r"^INCLUDE[ \t]+(.*)$", line)
        if m:
            read_file(canonical_path(base_directory + m.group(1)))
            continue

        file_name = prefix + "/" + canonical_path(base_directory + line)
        if not os.path.isfile("../" + file_name) \
                and not os.path.isdir("../" + file_name):
            sys.exit(f"{file_name} is not a file or a directory.")
        elif file_name in file_list:
            sys.exit(f"{file_name} is duplicated.")
        else:
            file_list.append(file_name)
    in_file.close()


def main():
    global prefix

    #
    #  オプションの処理
    #
    expand_flag = False
    expand_dirname = None
    args = []
    argv = sys.argv[1:]
    i = 0
    while i < len(argv):
        if argv[i] == "-e":
            expand_flag = True
            # 次の引数がオプションでもMANIFESTファイルでもなければdirname
            if i + 1 < len(argv) and not argv[i + 1].startswith("-") \
                    and i + 2 <= len(argv) - 1:
                i += 1
                expand_dirname = argv[i]
        elif argv[i].startswith("-e"):
            expand_flag = True
            expand_dirname = argv[i][2:]
        else:
            args.append(argv[i])
        i += 1

    #
    #  プリフィックス（./カレントディレクトリ名）の取り出し
    #
    prefix = "./" + os.path.basename(os.getcwd())

    #
    #  パラメータの取り出し
    #
    if len(args) >= 1:
        arg = args[0]
        arg = re.sub(r"\/$", "/MANIFEST", arg, count=1)
        arg = re.sub(r"^\.\/", "", arg, count=1)
    else:
        arg = "MANIFEST"

    #
    #  ファイルの読み込み
    #
    read_file(arg)
    if package == "":
        sys.exit("PACKAGE/E_PACKAGE directive not found.")
    if version == "":
        sys.exit("VERSION directive not found.")

    #
    #  RELEASEディレクトリの作成
    #
    if not os.path.isdir("RELEASE"):
        os.mkdir("RELEASE")

    #
    #  アーカイブ（.tar.gz）の作成
    #
    archive_name = package + "-" + version + ".tar.gz"
    file_list_str = " ".join(file_list)
    command = f"tar cvfz RELEASE/{archive_name} -C .. {file_list_str}"
    system(command)
    print(f"== RELEASE/{archive_name} is generated. ==")
    command = f"tar xvfz RELEASE/{archive_name} -C RELEASE"
    system(command)
    archive_zipname = package + "-" + version + ".zip"
    archive_dirname = os.path.basename(os.getcwd())
    cwd = os.getcwd()
    os.chdir("RELEASE")
    try:
        command = f"zip -r {archive_zipname} {archive_dirname}"
        print(command)
        system(command)
        command = f"rm -rf {archive_dirname}"
        print(command)
        system(command)
    finally:
        os.chdir(cwd)

    #
    #  アーカイブファイルの展開と削除
    #
    if expand_flag:
        command = f"tar xf RELEASE/{archive_name}; rm RELEASE/{archive_name}"
        system(command)

        dirname = expand_dirname if expand_dirname is not None else prefix
        if os.path.exists(dirname):
            os.rename(dirname, dirname + ".bak")
            print(f"== '{dirname}' is renamed to '{dirname}.bak'. ==")
        if expand_dirname is not None:
            os.rename(prefix, expand_dirname)
        print(f"== RELEASE/{archive_name} is expanded to '{dirname}'. ==")
        print(f"== RELEASE/{archive_name} is deleted. ==")


if __name__ == "__main__":
    main()
