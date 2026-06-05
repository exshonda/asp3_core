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
#  $Id: gentest.py (converted from gentest.rb) $
#

#
#		テストプログラム生成ツール（Python版）
#

import os
import re
import sys
from collections import defaultdict

#
#  生成動作を決めるための設定
#
PARAMETER_DEFINITION = {
    "get_tst": {2: "STAT"},
    "get_pri": {2: "PRI"},
    "get_inf": {1: "EXINF"},
    "ref_tsk": {2: "T_RTSK"},
    "ref_sem": {2: "T_RSEM"},
    "wai_flg": {4: "FLGPTN"},
    "pol_flg": {4: "FLGPTN"},
    "twai_flg": {4: "FLGPTN"},
    "ref_flg": {2: "T_RFLG"},
    "rcv_dtq": {2: "intptr_t"},
    "prcv_dtq": {2: "intptr_t"},
    "trcv_dtq": {2: "intptr_t"},
    "ref_dtq": {2: "T_RDTQ"},
    "rcv_pdq": {2: "intptr_t", 3: "PRI"},
    "prcv_pdq": {2: "intptr_t", 3: "PRI"},
    "trcv_pdq": {2: "intptr_t", 3: "PRI"},
    "ref_pdq": {2: "T_RPDQ"},
    "ref_mtx": {2: "T_RMTX"},
    "ref_mbf": {2: "T_RMBF"},
    "ref_spn": {2: "T_RSPN"},
    "get_mpf": {2: "void *"},
    "pget_mpf": {2: "void *"},
    "tget_mpf": {2: "void *"},
    "ref_mpf": {2: "T_RMPF"},
    "get_tim": {1: "SYSTIM"},
    "ref_cyc": {2: "T_RCYC"},
    "ref_alm": {2: "T_RALM"},
    "ref_ovr": {2: "T_ROVR"},
    "get_tid": {1: "ID"},
    "get_did": {1: "ID"},
    "get_pid": {1: "ID"},
    "get_lod": {2: "uint_t"},
    "mget_lod": {3: "uint_t"},
    "get_nth": {3: "ID"},
    "mget_nth": {4: "ID"},
    "ref_mem": {2: "T_RMEM"},
    "get_ipm": {1: "PRI"},
    "get_som": {1: "ID"},
}

FUNCTION_PARAMETERS = {
    "target_hrt_set_event": "HRTCNT hrtcnt",
    "hook_hrt_set_event": "HRTCNT hrtcnt",
}

FUNCTION_VALUE = {
    "target_hrt_get_current": "HRTCNT",
}

FUNCTION_RETURN = {
    "target_hrt_get_current": "0U",
}

FUNCTION_CHECK_PARAMETER = {
    "target_hrt_set_event": "hrtcnt",
    "hook_hrt_set_event": "hrtcnt",
}


#
#  出力行を保持するセル（Ruby版のString#sub!による共有更新の代替）
#
class Cell:
    def __init__(self, val):
        self.val = val


#
#  処理単位のコードを格納するクラス
#
class PUCode:
    # 初期化
    def __init__(self, pu_name):
        self.pu_name = pu_name				# 処理単位の名前
        self.current_count = ""				# 処理カウント
        self.count_flag = False				# 処理カウントが使われたか
        self.code = defaultdict(list)		# 処理単位のコード
        self.variable_list = {}				# 処理単位の変数
        self.sil_flag = False				# SILが使われたか

        # 処理カウント変数名の生成
        m = re.match(r"^TASK([0-9]*)$", pu_name)
        if m:
            self.count_var = f"task{m.group(1)}_count"
            return
        m = re.match(r"^CYC([0-9]*)$", pu_name)
        if m:
            self.count_var = f"cyclic{m.group(1)}_count"
            return
        m = re.match(r"^ALM([0-9]*)$", pu_name)
        if m:
            self.count_var = f"alarm{m.group(1)}_count"
            return
        if re.match(r"^OVR$", pu_name):
            self.count_var = "overrun_count"
            return
        m = re.match(r"^ISR([0-9]*)$", pu_name)
        if m:
            self.count_var = f"isr{m.group(1)}_count"
            return
        m = re.match(r"^INTHDR([0-9]*)$", pu_name)
        if m:
            self.count_var = f"inthdr{m.group(1)}_count"
            return
        m = re.match(r"^CPUEXC([0-9]*)$", pu_name)
        if m:
            self.count_var = f"cpuexc{m.group(1)}_count"
            return
        m = re.match(r"^EXTSVC([0-9]*)$", pu_name)
        if m:
            self.count_var = f"extsvc{m.group(1)}_count"
            return
        self.count_var = pu_name + "_count"

    # 処理カウントの設定
    def set_count(self, count):
        if count != "":
            self.count_flag = True
        self.current_count = count

    # 処理カウントのインクリメント
    def increment_count(self):
        if self.current_count != "":
            self.current_count = str(int(self.current_count) + 1)

    # コードの追加
    def append(self, *lines):
        for line in lines:
            self.code[self.current_count].append(line)

    # 変数の追加
    def add_variable(self, var_name, type_name):
        self.variable_list[var_name] = type_name

    # SILの使用
    def use_sil(self):
        self.sil_flag = True

    # 処理単位のコードの出力
    def generate_code(self, out_file):
        # 不要な処理単位の判定
        if len(self.code) == 0:
            return

        # 処理カウント変数の生成
        if self.count_flag:
            out_file.write(f"\nstatic uint_t\t{self.count_var} = 0;\n")

        # 関数ヘッダの生成
        pu_name = self.pu_name
        m_task = re.match(r"^TASK([0-9]*)$", pu_name)
        m_cyc = re.match(r"^CYC([0-9]*)$", pu_name)
        m_alm = re.match(r"^ALM([0-9]*)$", pu_name)
        m_isr = re.match(r"^ISR([0-9]*)$", pu_name)
        m_inthdr = re.match(r"^INTHDR([0-9]*)$", pu_name)
        m_cpuexc = re.match(r"^CPUEXC([0-9]*)$", pu_name)
        m_extsvc = re.match(r"^EXTSVC([0-9]*)$", pu_name)
        if m_task:
            out_file.write("\nvoid\n")
            out_file.write(f"task{m_task.group(1)}(EXINF exinf)\n")
        elif m_cyc:
            out_file.write("\nvoid\n")
            out_file.write(f"cyclic{m_cyc.group(1)}_handler(EXINF exinf)\n")
        elif m_alm:
            out_file.write("\nvoid\n")
            out_file.write(f"alarm{m_alm.group(1)}_handler(EXINF exinf)\n")
        elif re.match(r"^OVR$", pu_name):
            out_file.write("\nvoid\n")
            out_file.write("overrun_handler(ID tskid, EXINF exinf)\n")
        elif m_isr:
            out_file.write("\nvoid\n")
            out_file.write(f"isr{m_isr.group(1)}(EXINF exinf)\n")
        elif m_inthdr:
            out_file.write("\nvoid\n")
            out_file.write(f"inthdr{m_inthdr.group(1)}_handler(void)\n")
        elif m_cpuexc:
            out_file.write("\nvoid\n")
            out_file.write(f"cpuexc{m_cpuexc.group(1)}_handler(void *p_excinf)\n")
        elif m_extsvc:
            out_file.write("\nER_UINT\n")
            out_file.write(f"extsvc{m_extsvc.group(1)}_routine")
            out_file.write("(intptr_t par1, intptr_t par2, intptr_t par3,\n")
            out_file.write("\t\t\t\t\t\t\t\tintptr_t par4, intptr_t par5, ID cdmid)\n")
        else:
            if pu_name in FUNCTION_VALUE:
                out_file.write(f"\n{FUNCTION_VALUE[pu_name]}\n")
            else:
                out_file.write("\nvoid\n")
            out_file.write(pu_name)
            if pu_name in FUNCTION_PARAMETERS:
                out_file.write(f"({FUNCTION_PARAMETERS[pu_name]})\n")
            else:
                out_file.write("(void)\n")

        out_file.write("{\n")

        for var_name, var_type in self.variable_list.items():
            m = re.match(r"^(.+)\s*\*$", var_type)
            if m:
                var_base_type = m.group(1)
                out_file.write(f"\t{var_base_type}")
                out_file.write("\t\t*" if len(var_base_type) < 4 else "\t*")
            else:
                out_file.write(f"\t{var_type}")
                out_file.write("\t\t" if len(var_type) < 4 else "\t")
            out_file.write(f"{var_name};\n")
        if self.sil_flag:
            out_file.write("\tSIL_PRE_LOC;\n")
        out_file.write("\n")

        if self.count_flag:
            out_file.write(f"\tswitch (++{self.count_var}) {{\n")
            for count in sorted(self.code.keys(), key=lambda c: int(c)):
                out_file.write(f"\tcase {count}:\n")
                for line in self.code[count]:
                    if line != "":
                        out_file.write("\t" + line)
                    out_file.write("\n")
                out_file.write("\t\tcheck_assert(false);\n\n")
            out_file.write("\tdefault:\n")
            out_file.write("\t\tcheck_assert(false);\n")
            out_file.write("\t}\n")
        else:
            for line in self.code[""]:
                out_file.write(line + "\n")

        out_file.write("\tcheck_assert(false);\n")
        if re.match(r"^EXTSVC([0-9]*)$", pu_name):
            out_file.write("\treturn(E_SYS);\n")
        # Ruby版では $functionReturn[@pu_nama]（typo）の参照により
        # この分岐は実質無効であるため，出力互換のために生成しない
        out_file.write("}\n")


#
#  グローバル状態
#
class GenTest:
    def __init__(self):
        self.proc_flag = False				# スクリプト処理中フラグ
        self.proc_flag_end = False			# スクリプト処理終了フラグ
        self.start_flag = False				# テスト開始コードの出力フラグ
        self.current_pu = {}				# 読み込み中の処理単位
        self.pu_list = {}					# 処理単位のリスト
        self.output_lines = []				# 出力すべき行のリストのリスト
        self.cp_suffix = defaultdict(str)	# チェックポイント関数のサフィックス
        self.last_check_point = defaultdict(int)	# 最後のチェックポイント番号

    #
    #  サービスコール呼び出しの読み込み
    #
    def gen_service_call(self, pu, svc_call, error_code):
        pu.add_variable("ercd", "ER_UINT")
        pu.append(f"\tercd = {svc_call};")

        m = re.match(r"^([a-z_]+)\((.*)\)$", svc_call)
        if m:
            svc_name = m.group(1)
            params = re.split(r"\s*,\s*", m.group(2))

            if svc_name in PARAMETER_DEFINITION:
                for pos, type_name in PARAMETER_DEFINITION[svc_name].items():
                    if len(params) >= pos:
                        m2 = re.match(r"^\&([a-zA-Z0-9_]+)$", params[pos - 1])
                        if m2:
                            pu.add_variable(m2.group(1), type_name)

        if not error_code:
            # E_OKが返る場合
            pu.append("\tcheck_ercd(ercd, E_OK);", "")
        elif error_code == "noreturn":
            # リターンしない場合
            pu.append("")
        else:
            pu.append(f"\tcheck_ercd(ercd, {error_code});", "")

    #
    #  テスト開始コードの生成
    #
    def test_start_code(self, pu):
        # テスト開始コードは一度のみ出力する
        if not self.start_flag:
            pu.append("\ttest_start(__FILE__);", "")
            self.start_flag = True

    #
    #  ターゲット依存部関数の振る舞いの読み込み
    #
    def target_function(self, line, check_num, prcid):
        pu = None
        function_name = None
        m = re.match(r"^([a-zA-Z_]+)\s*(.*)$", line)
        if m:
            function_name = m.group(1)
            line = m.group(2)
            pu = self.pu_list.get(function_name)
            if pu is None:
                # 新しい処理単位の生成
                pu = self.pu_list[function_name] = PUCode(function_name)
                pu.set_count("1")
                self.test_start_code(pu)

        retval = None
        param = None
        m = re.match(r"\-\>\s*([^\s]+)\s*(.*)$", line)
        if m:
            retval = m.group(1)
            line = m.group(2)
        m = re.match(r"\<\-\s*([^\s]+)\s*(.*)$", line)
        if m:
            param = m.group(1)
            line = m.group(2)

        pu.append(f"\tcheck_point{self.cp_suffix[prcid]}({check_num});")
        if param and function_name in FUNCTION_CHECK_PARAMETER:
            pu.append("\tcheck_assert(%s == %s);"
                      % (FUNCTION_CHECK_PARAMETER[function_name], param), "")
        if retval:
            pu.append(f"\treturn({retval});", "")
        else:
            pu.append("\treturn;", "")
        pu.increment_count()

    #
    #  チェックポイント番号の処理
    #
    def proc_check_point(self, prcid, original_check_num, oline_list):
        self.last_check_point[prcid] += 1
        check_num = str(self.last_check_point[prcid])
        m = re.match(r"^([0-9]\:(\t| |  ))", original_check_num)
        if m:
            original_check_num = m.group(1)
        else:
            m = re.match(r"^([0-9][0-9]\:(\t| ))", original_check_num)
            if m:
                original_check_num = m.group(1)
            else:
                m = re.match(r"^([0-9][0-9][0-9]\:)", original_check_num)
                if m:
                    original_check_num = m.group(1)
        if len(check_num) < 3:
            w_space = "\t"
        else:
            w_space = ""
        for oline in oline_list:
            oline.val = re.sub(re.escape(original_check_num),
                               f"{check_num}:{w_space}", oline.val, count=1)
        return check_num

    #
    #  テストスクリプトの読み込み
    #
    def parse_line(self, line, prcid, oline_list):
        m = re.match(r"^==\s*START(_[a-zA-Z0-9]+)?(.*)$", line)
        if m:
            if not self.proc_flag_end:
                self.proc_flag = True
            self.cp_suffix[prcid] = m.group(1) or ""
            return

        m = re.match(r"^==\s*(([a-zA-Z_]+)[0-9]*)(.*)\s*==$", line)
        if m:
            # 処理単位の開始
            if not self.proc_flag_end:
                self.proc_flag = True
            pu_name = m.group(1)
            line2 = m.group(3)
            pu = self.pu_list.get(pu_name)
            if pu is None:
                # 新しい処理単位の生成
                pu = self.pu_list[pu_name] = PUCode(pu_name)
            self.current_pu[prcid] = pu

            m2 = re.match(r"^\-([0-9]+)(.*)$", line2)
            if m2:
                pu.set_count(m2.group(1))
            elif re.match(r"^\-[nN](.*)$", line2):
                pass  # do nothing.
            else:
                pu.set_count("")
            self.test_start_code(pu)
            return

        if re.match(r"^==\s*(([a-zA-Z_]+)[0-9]*)(.*)$", line):
            # 後ろの"=="がない行は，処理単位の開始と見なさない
            return

        if not self.proc_flag:
            return

        pu = self.current_pu[prcid]
        m = re.match(r"^([0-9]+\:\s*)(.*)$", line)
        if m:
            # チェックポイント番号の処理
            line = m.group(2)
            check_num = self.proc_check_point(prcid, m.group(1), oline_list)

            if re.match(r"^END$", line):
                pu.append(f"\tcheck_finish{self.cp_suffix[prcid]}({check_num});")
                self.proc_flag_end = True
                return
            m2 = re.match(r"^HOOK\((.*)\)$", line)
            if m2:
                hook_text = f"\t{m2.group(1)};"
                # Ruby版のsprintfと同様の挙動とする（%dには番号を整数化して
                # 渡し，%指定が無ければそのまま）
                try:
                    hook_text = hook_text % (int(check_num),)
                except (TypeError, ValueError):
                    try:
                        hook_text = hook_text % (check_num,)
                    except TypeError:
                        pass
                pu.append(hook_text)
                return
            m2 = re.match(r"^\[(.*)\]$", line)
            if m2:
                self.target_function(m2.group(1), check_num, prcid)
                return
            pu.append(f"\tcheck_point{self.cp_suffix[prcid]}({check_num});")

        m2 = re.match(r"^((assert|state|ipm)\(.*\))$", line)
        if m2:
            pu.append(f"\tcheck_{m2.group(1)};", "")
            return
        m2 = re.match(r"^(call|DO)\((.*)\)$", line)
        if m2:
            call_string = m2.group(2)
            pu.append(f"\t{call_string};", "")
            if re.match(r"^SIL_..._INT\(\)$", call_string):
                pu.use_sil()
            return
        m2 = re.match(r"^VAR\(\s*(.*)\s+(.*)\s*\)$", line)
        if m2:
            pu.add_variable(m2.group(2), m2.group(1))
            return
        m2 = re.match(r"^RETURN((\(.*\))?)$", line)
        if m2:
            pu.append(f"\treturn{m2.group(1)};", "")
            pu.increment_count()
            return
        m2 = re.match(r"^GOTO\((.*)\)$", line)
        if m2:
            pu.append(f"\tgoto {m2.group(1)};", "")
            return
        m2 = re.match(r"^LABEL\((.*)\)$", line)
        if m2:
            pu.append(f"{m2.group(1)}:", "")
            return
        m2 = re.match(r"^BARRIER\((.*)\)$", line)
        if m2:
            pu.append(f"\ttest_barrier({m2.group(1)});", "")
            return
        m2 = re.match(r"^((SET|RESET|WAIT|WAIT_WO_RESET|WAIT_RESET)\(.*\))$", line)
        if m2:
            pu.append(f"\t{m2.group(1)};", "")
            return
        m2 = re.match(r"^([a-z_]+\(.*\))\s*(\-\>\s*([A-Za-z0-9_]*))?\s*$", line)
        if m2:
            self.gen_service_call(pu, m2.group(1), m2.group(3))
            return
        print(f"Error: {line}", file=sys.stderr)


def main():
    #
    #  エラーチェック
    #
    if len(sys.argv) != 2:
        sys.exit("Usage: python3 gentest.py <test_program>")

    #
    #  初期化
    #
    in_file_name = sys.argv[1]
    out_file_name = in_file_name + ".new"

    gen = GenTest()

    try:
        in_file = open(in_file_name, encoding="utf-8")
        out_file = open(out_file_name, "w", encoding="utf-8")
    except OSError as e:
        sys.exit(str(e))

    #
    #  スクリプトファイル読み込み処理
    #
    statements = defaultdict(str)
    oline_lists = defaultdict(list)
    for line in in_file:
        line = line.rstrip("\n")					# 末尾の改行の削除
        if re.search(r"DO NOT DELETE THIS LINE", line):
            gen.output_lines.append([line])
            break
        m = re.match(r"^(\s*\*)(.*)$", line)		# 行頭のスペースと "*" の削除
        if m:
            output_line = [m.group(1)]
            for index, sline in enumerate(m.group(2).split("｜")):
                out_line = Cell(sline)
                output_line.append(out_line)
                sline = re.sub(r"^\s*", "", sline)			# 先頭のスペースの削除
                sline = re.sub(r"\/\/.*$", "", sline)		# // コメントの削除
                sline = re.sub(r"\.\.\..*$", "", sline)		# ... コメントの削除

                prcid = index + 1
                statements[prcid] += sline
                oline_lists[prcid].append(out_line)
                statements[prcid], n = re.subn(r"\\\s*$", "", statements[prcid])
                if n > 0:
                    continue					# 継続行の場合

                statements[prcid] = re.sub(r"\s*$", "", statements[prcid])
                if not re.match(r"^\s*$", sline):
                    gen.parse_line(statements[prcid], prcid, oline_lists[prcid])
                statements[prcid] = ""
                oline_lists[prcid] = []
            gen.output_lines.append(output_line)
            if gen.proc_flag_end:
                gen.proc_flag = False
        else:
            gen.output_lines.append([line])
    in_file.close()

    for output_line in gen.output_lines:
        for index, sline in enumerate(output_line):
            if index > 1:
                out_file.write("｜")
            out_file.write(sline.val if isinstance(sline, Cell) else sline)
        out_file.write("\n")

    #
    #  テストプログラム出力処理
    #
    for pu_name in sorted(gen.pu_list.keys()):
        gen.pu_list[pu_name].generate_code(out_file)
    out_file.close()

    #
    #  ファイルの置き換え
    #
    os.rename(in_file_name, in_file_name + ".bak")
    os.rename(out_file_name, in_file_name)


if __name__ == "__main__":
    main()
