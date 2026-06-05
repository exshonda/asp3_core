#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  TOPPERS Software
#      Toyohashi Open Platform for Embedded Real-Time Systems
#
#  Copyright (C) 2001-2003 by Embedded and Real-Time Systems Laboratory
#                              Toyohashi Univ. of Technology, JAPAN
#  Copyright (C) 2006-2022 by Embedded and Real-Time Systems Laboratory
#              Graduate School of Information Science, Nagoya Univ., JAPAN
#
#  上記著作権者は，以下の(1)〜(4)の条件を満たす場合に限り，本ソフトウェ
#  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
#  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
#  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
#      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
#      スコード中に含まれていること．
#  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
#      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
#      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
#      の無保証規定を掲載すること．
#  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
#      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
#      と．
#    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
#        作権表示，この利用条件および下記の無保証規定を掲載すること．
#    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
#        報告すること．
#  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
#      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
#      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
#      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
#      免責すること．
#
#  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
#  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
#  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
#  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
#  の責任を負わない．
#
#  $Id: configure.py (converted from configure.rb) $
#

#
#  configure.rb のPython版（CLI・生成内容は同一）
#

import os
import re
import sys
import glob

#  オプションの定義
#
#  -T <target>			ターゲット名（必須）
#  -a <appldirs>		アプリケーションのディレクトリ名（複数指定可．デ
#						フォルトはsampleディレクトリ）
#  -A <applname>		アプリケーションプログラム名（デフォルトはsample1）
#  -t					メインのオブジェクトファイルをリンク対象に含めない
#  -c <cfgfile>			システムコンフィギュレーションファイル（.cfgファイ
#						ル名）名
#  -C <cdlflle>			コンポーネント記述ファイル（.cdlファイル）名
#  -U <applobjs>		アプリケーションの追加のオブジェクトファイル
#						（.oファイル名で指定．複数指定可）
#  -S <syssvcobjs>		システムサービスのオブジェクトファイル
#						（.oファイル名で指定．複数指定可）
#  -B <bannerobj>		バナー表示のオブジェクトファイル（.oファイル名で指定）
#  -L <kernel_lib>		カーネルライブラリ（libkernel.a）のディレクトリ名
#						（省略した場合，カーネルライブラリもmakeする）
#  -f					カーネルを関数単位でコンパイルするかどうかの指定
#  -D <srcdir>			カーネルソースの置かれているディレクトリ
#  -l <srclang>			プログラミング言語（現時点ではcとc++のみサポート）
#  -m <tempmakefile>	Makefileのテンプレートのファイル名の指定（デフォル
#						トはsampleディレクトリのMakefile）
#  -d <objdir>			中間オブジェクトファイルと依存関係ファイルを置く
#						ディレクトリ名（デフォルトはobjs）
#  -w					TECSを使用しない
#  -W <tecsdir>			TECS関係ファイルのディレクトリ名（デフォルトはソー
#						スファイルのディレクトリの下のtecsgen）
#  -r					トレースログ記録のサンプルコードを使用するかどうか
#						の指定
#  -V <devtooldir>		開発ツール（コンパイラ等）の置かれているディレクトリ
#  -R <ruby>			rubyのパス名（明示的に指定する場合）
#  -g <cfg>				コンフィギュレータ（cfg）のパス名
#  -G <tecsgen>			TECSジェネレータ（tecsgen）のパス名
#  -o <options>			コンパイルオプション（COPTSに追加）
#  -O <options>			シンボル定義オプション（CDEFSに追加）
#  -k <options>			リンカオプション（LDFLAGSに追加）
#  -b <options>			リンカオプション（LIBSに追加）


def main():
    #
    #  変数の初期化
    #
    #  【asp3_core変更】非TECS（TECSレス）ビルドをデフォルトとする．
    #  OMIT_TECSを初期値で設定する（-wオプションの付与は不要）．
    #
    vartable = {}
    vartable["OMIT_TECS"] = "true"		# 【asp3_core変更】非TECSをデフォルト化

    #
    #  オプションの処理
    #
    #  Ruby版のoptparseと同様，引数をとるオプションは次の引数を値として
    #  そのまま消費する（値が-で始まっていてもよい）．
    #
    target = None
    appldirs = []
    applname = None
    option_t = False
    cfgfile = None
    cdlfile = None
    applobjs = []
    syssvcobjs = []
    bannerobj = None
    kernel_lib = ""
    srcdir = None
    srclang = "c"
    tempmakefile = None
    objdir = "objs"
    tecsdir = None
    devtooldir = ""
    ruby = "ruby"
    cfg = None
    tecsgen = None
    copts = []
    cdefs = []
    ldflags = []
    libs = []

    rest = []
    argv = sys.argv[1:]
    i = 0

    def next_arg():
        nonlocal i
        i += 1
        if i >= len(argv):
            sys.exit(f"configure.py: missing argument for {argv[i - 1]}")
        return argv[i]

    while i < len(argv):
        arg = argv[i]
        if arg == "-T":
            target = next_arg()
        elif arg == "-a":
            appldirs += next_arg().split()
        elif arg == "-A":
            applname = next_arg()
        elif arg == "-t":
            option_t = True
        elif arg == "-c":
            cfgfile = next_arg()
        elif arg == "-C":
            cdlfile = next_arg()
        elif arg == "-U":
            applobjs += next_arg().split()
        elif arg == "-S":
            syssvcobjs += next_arg().split()
        elif arg == "-B":
            bannerobj = next_arg()
        elif arg == "-L":
            kernel_lib = next_arg()
        elif arg == "-f":
            vartable["KERNEL_FUNCOBJS"] = "true"
        elif arg == "-D":
            srcdir = next_arg()
        elif arg == "-l":
            srclang = next_arg()
        elif arg == "-m":
            tempmakefile = next_arg()
        elif arg == "-d":
            objdir = next_arg()
        elif arg == "-w":
            vartable["OMIT_TECS"] = "true"
        elif arg == "-W":
            tecsdir = next_arg()
        elif arg == "-r":
            vartable["ENABLE_TRACE"] = "true"
        elif arg == "-V":
            devtooldir = next_arg()
        elif arg == "-R":
            ruby = next_arg()
        elif arg == "-g":
            cfg = next_arg()
        elif arg == "-G":
            tecsgen = next_arg()
        elif arg == "-o":
            copts += next_arg().split()
        elif arg == "-O":
            cdefs += next_arg().split()
        elif arg == "-k":
            ldflags += next_arg().split()
        elif arg == "-b":
            libs += next_arg().split()
        else:
            rest.append(arg)
        i += 1

    #
    #  パラメータの処理
    #
    for arg in rest:
        m = re.match(r"^([A-Za-z0-9_]+)\s*\=\s*(.*)$", arg)
        if m:
            vartable[m.group(1)] = m.group(2)
        else:
            vartable[arg] = "true"

    #
    #  オブジェクトファイル名の拡張子を返す
    #
    def get_object_extension():
        if "cygwin" in sys.platform:
            return "exe"
        else:
            return ""

    #
    #  変数のデフォルト値（文字列変数のデフォルト値は初期化で与える）
    #
    if not appldirs:
        appldirs.append("$(SRCDIR)/sample")
    applname = applname or "sample1"
    cfgfile = cfgfile or applname + ".cfg"
    cdlfile = cdlfile or applname + ".cdl"
    if not option_t:
        applobjs.insert(0, applname + ".o")
    #
    #  【asp3_core変更】OMIT_TECSの判定を値の有無で行う（デフォルトで
    #  OMIT_TECS=trueのため，TECS構成にする場合は引数で OMIT_TECS= と
    #  空値を指定する）．
    #
    bannerobj = bannerobj or \
        ("banner.o" if vartable.get("OMIT_TECS", "") != "" else "tBannerMain.o")
    #
    #  【asp3_core変更】非TECS時は共通の非TECS版システムサービスのオブジェ
    #  クトファイルをデフォルトでSYSSVCOBJSに含める（-Sオプションの指定は
    #  ターゲット依存のSIOドライバ等の追加分のみで足りる）．システムサービ
    #  スを使わない構成（cfgテスト等）では，引数に OMIT_DEFAULT_SYSSVC を
    #  指定すると自動付与を抑止できる．
    #
    if vartable.get("OMIT_TECS", "") != "" \
            and vartable.get("OMIT_DEFAULT_SYSSVC", "") == "":
        defobjs = ["syslog.o", "banner.o", "serial.o", "serial_cfg.o",
                   "logtask.o"]
        syssvcobjs = defobjs + [o for o in syssvcobjs if o not in defobjs]
    if srcdir is None:
        # ソースディレクトリ名を取り出す
        m = re.match(r"^(.*)\/configure", sys.argv[0])
        if m:
            srcdir = m.group(1)
        else:
            srcdir = os.getcwd()
    if re.match(r"^\/", srcdir):
        srcabsdir = srcdir
    else:
        srcabsdir = os.getcwd() + "/" + srcdir
    tempmakefile = tempmakefile or srcdir + "/sample/Makefile"
    tecsdir = tecsdir or "$(SRCDIR)/tecsgen"
    #
    #  【asp3_core変更】コンフィギュレータのデフォルトをPython版（cfg.py）
    #  とする．Ruby版を使用する場合は -g "ruby $(SRCDIR)/cfg/cfg.rb" を指定
    #  する（その場合は生成テンプレートも.trbを指定すること）．
    #
    cfg = cfg or "python3 $(SRCDIR)/cfg/cfg.py"
    tecsgen = tecsgen or ruby + " $(TECSDIR)/tecsgen.rb"

    #
    #  -Tオプションとターゲット依存部ディレクトリの確認
    #
    if target is None:
        print("configure.py: -T option is mandatory")
    elif not os.path.isdir(srcdir + "/target/" + target):
        print(f"configure.py: {srcdir}/target/{target} not exist.")
        target = None

    if target is None:
        print("Installed targets are:")
        for tgt in glob.glob(srcdir + "/target/[a-zA-Z0-9]*"):
            tgt = tgt.replace(srcdir + "/target/", "")
            print("\t" + tgt)
        sys.exit(1)

    #
    #  変数テーブルの作成
    #
    vartable["TARGET"] = target
    vartable["APPLDIRS"] = " ".join(appldirs)
    vartable["APPLNAME"] = applname
    vartable["CFGFILE"] = cfgfile
    vartable["CDLFILE"] = cdlfile
    vartable["APPLOBJS"] = " ".join(applobjs)
    vartable["SYSSVCOBJS"] = " ".join(syssvcobjs)
    vartable["BANNEROBJ"] = bannerobj
    vartable["KERNEL_LIB"] = kernel_lib
    vartable["SRCDIR"] = srcdir
    vartable["SRCABSDIR"] = srcabsdir
    vartable["SRCLANG"] = srclang
    vartable["OBJDIR"] = objdir
    vartable["TECSDIR"] = tecsdir
    vartable["DEVTOOLDIR"] = devtooldir
    vartable["RUBY"] = ruby
    vartable["CFG"] = cfg
    vartable["TECSGEN"] = tecsgen
    vartable["COPTS"] = " ".join(copts)
    vartable["CDEFS"] = " ".join(cdefs)
    vartable["LDFLAGS"] = " ".join(ldflags)
    vartable["LIBS"] = " ".join(libs)
    vartable["OBJEXT"] = get_object_extension()

    #
    #  ファイルを変換する
    #
    def convert(in_file_name, out_file_name):
        print(f"Generating {out_file_name} from {in_file_name}.")
        if os.path.isfile(out_file_name):
            print(f"{out_file_name} exists.  Save as {out_file_name}.bak.")
            os.replace(out_file_name, out_file_name + ".bak")

        try:
            in_file = open(in_file_name, encoding="utf-8")
            out_file = open(out_file_name, "w", encoding="utf-8")
        except OSError as e:
            sys.exit(str(e))

        pattern = re.compile(r"\@\(([A-Za-z0-9_]+)\)")
        for line in in_file:
            line = line.rstrip("\n")
            #  Ruby版のsub!ループと同様，置換結果に@(...)が残らなくなる
            #  まで先頭から繰り返し置換する
            while True:
                line, count = pattern.subn(
                    lambda m: str(vartable.get(m.group(1), "")), line, count=1)
                if count == 0:
                    break
            out_file.write(line + "\n")

        in_file.close()
        out_file.close()

    #
    #  Makefileの生成
    #
    convert(tempmakefile, "Makefile")

    #
    #  中間オブジェクトファイルと依存関係ファイルを置くディレクトリの作成
    #
    if not os.path.isdir(objdir):
        os.mkdir(objdir)


if __name__ == "__main__":
    main()
