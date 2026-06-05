#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  TOPPERS Software
#      Toyohashi Open Platform for Embedded Real-Time Systems
#
#  Copyright (C) 2017-2022 by Embedded and Real-Time Systems Laboratory
#              Graduate School of Information Science, Nagoya Univ., JAPAN
#
#  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
#  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
#  トウェアは無保証で提供される．
#
#  $Id: applyrename.py (converted from applyrename.rb) $
#

#
#		リネーム適用ツール（Python版）
#

import os
import re
import sys
import filecmp


#
#  ファイルにリネームを適用する
#
def apply_rename(in_file_name, syms):
    syms_regexp = "|".join(syms)
    out_file_name = in_file_name + ".new"
    pattern = re.compile(r"\b(_?)(" + syms_regexp + r")\b")

    try:
        in_file = open(in_file_name, encoding="utf-8")
        out_file = open(out_file_name, "w", encoding="utf-8")
    except OSError as e:
        sys.exit(str(e))

    for line in in_file:
        line = pattern.sub(r"\1_kernel_\2", line)
        out_file.write(line)

    in_file.close()
    out_file.close()

    if filecmp.cmp(in_file_name, out_file_name, shallow=False):
        # ファイルの内容が変化しなかった場合
        os.unlink(out_file_name)
    else:
        # ファイルの内容が変化した場合
        os.rename(in_file_name, in_file_name + ".bak")
        os.rename(out_file_name, in_file_name)
        print(f"Modified: {in_file_name}")


def main():
    #
    #  エラーチェック
    #
    if len(sys.argv) < 2:
        sys.exit("Usage: python3 applyrename.py <prefix> <filelists>")

    #
    #  初期化
    #
    syms = []
    name = sys.argv[1]

    #
    #  シンボルリストを読み込む
    #
    def_file_name = name + "_rename.def"

    try:
        def_file = open(def_file_name, encoding="utf-8")
    except OSError as e:
        sys.exit(str(e))

    for line in def_file:
        line = line.rstrip("\n")
        if re.match(r"^#\s*(.*)$", line):
            pass  # do nothing
        elif re.match(r"^INCLUDE\s+(.*)$", line):
            pass  # do nothing
        elif line != "":
            syms.append(line)
    def_file.close()

    #
    #  ファイルにリネームを適用する
    #
    for in_file_name in sys.argv[2:]:
        if in_file_name != def_file_name and os.access(in_file_name, os.R_OK):
            apply_rename(in_file_name, syms)


if __name__ == "__main__":
    main()
