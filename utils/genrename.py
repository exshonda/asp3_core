#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  TOPPERS Software
#      Toyohashi Open Platform for Embedded Real-Time Systems
#
#  Copyright (C) 2005-2017 by Embedded and Real-Time Systems Laboratory
#              Graduate School of Information Science, Nagoya Univ., JAPAN
#
#  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
#  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソフ
#  トウェアは無保証で提供される．
#
#  $Id: genrename.py (converted from genrename.rb) $
#

#
#		リネームヘッダファイル生成ツール（Python版）
#

import re
import sys


#
#  先頭につける文字列
#
def prefix_string(sym):
    if re.search(r"[a-z]", sym):
        return "_kernel_"
    else:
        return "_KERNEL_"


#
#  リネーム定義を生成する
#
def generate_define(out_file, sym, prefix):
    out_file.write(f"#define {prefix}{sym}")
    out_file.write("\t" * max(6 - (len(sym) + len(prefix)) // 4, 0))
    out_file.write(f"\t{prefix}{prefix_string(sym)}{sym}\n")


#
#  リネーム解除を生成する
#
def generate_undef(out_file, sym, prefix):
    out_file.write(f"#undef {prefix}{sym}\n")


#
#  コメントを生成する
#
def generate_comment(out_file, comment):
    out_file.write("/*\n")
    out_file.write(f" *  {comment}\n")
    out_file.write(" */\n")


def main():
    #
    #  エラーチェック
    #
    if len(sys.argv) < 2:
        sys.exit("Usage: python3 genrename.py <prefix>")

    #
    #  初期化
    #
    syms = []
    name = sys.argv[1]
    name_upper = name.upper()

    in_file_name = name + "_rename.def"
    header_define_symbol = "TOPPERS_" + name_upper + "_RENAME_H"

    #
    #  シンボルリストを読み込む
    #
    try:
        in_file = open(in_file_name, encoding="utf-8")
    except OSError as e:
        sys.exit(str(e))

    for line in in_file:
        syms.append(line.rstrip("\n"))
    in_file.close()

    #
    #  ???_rename.h を生成する
    #
    try:
        out_file = open(name + "_rename.h", "w", encoding="utf-8")
    except OSError as e:
        sys.exit(str(e))

    out_file.write(f"""/* This file is generated from {in_file_name} by genrename. */

#ifndef {header_define_symbol}
#define {header_define_symbol}

""")

    includes = ""
    for sym in syms:
        m_comment = re.match(r"^#\s*(.*)$", sym)
        m_include = re.match(r"^INCLUDE\s+(.*)$", sym)
        m_prefix = re.match(r"^(_+)(.*)$", sym)
        if m_comment:
            generate_comment(out_file, m_comment.group(1))
        elif m_include:
            file_name = re.sub(r'([>"])$', r"_rename.h\1", m_include.group(1))
            includes += f"#include {file_name}\n"
        elif m_prefix:
            generate_define(out_file, m_prefix.group(2), m_prefix.group(1))
        elif sym != "":
            generate_define(out_file, sym, "")
        else:
            out_file.write("\n")

    out_file.write(f"""
{includes}
#endif /* {header_define_symbol} */
""")
    out_file.close()

    #
    #  ???_unrename.h を生成する
    #
    try:
        out_file = open(name + "_unrename.h", "w", encoding="utf-8")
    except OSError as e:
        sys.exit(str(e))

    out_file.write(f"""/* This file is generated from {in_file_name} by genrename. */

/* This file is included only when {name}_rename.h has been included. */
#ifdef {header_define_symbol}
#undef {header_define_symbol}

""")

    includes = ""
    for sym in syms:
        m_comment = re.match(r"^#\s*(.*)$", sym)
        m_include = re.match(r"^INCLUDE\s+(.*)$", sym)
        if m_comment:
            generate_comment(out_file, m_comment.group(1))
        elif m_include:
            file_name = re.sub(r'([>"])$', r"_unrename.h\1", m_include.group(1))
            includes += f"#include {file_name}\n"
        elif sym != "":
            generate_undef(out_file, sym, "")
        else:
            out_file.write("\n")

    out_file.write(f"""
{includes}
#endif /* {header_define_symbol} */
""")
    out_file.close()


if __name__ == "__main__":
    main()
