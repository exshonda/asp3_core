# -*- coding: utf-8 -*-
#
#		オフセットファイル生成用Pythonテンプレート（RISC-V用）
#
#   $Id: core_offset.py (converted from core_offset.trb) $
#

#
#  ターゲット非依存部のインクルード
#
IncludeTrb("kernel/genoffset.py")

#
#  フィールドのオフセットの定義の生成
#
offsetH.append(f"""#define TCB_p_tinib\t\t{offsetof_TCB_p_tinib}
#define TCB_sp\t\t\t{offsetof_TCB_sp}
#define TCB_pc\t\t\t{offsetof_TCB_pc}
#define TINIB_exinf\t\t{offsetof_TINIB_exinf}
#define TINIB_task\t\t{offsetof_TINIB_task}
#define TINIB_stksz\t\t{offsetof_TINIB_stksz}
#define TINIB_stk\t\t{offsetof_TINIB_stk}
""")
