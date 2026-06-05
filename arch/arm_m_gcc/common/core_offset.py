# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#       Toyohashi Open Platform for Embedded Real-Time Systems/
#       Advanced Standard Profile Kernel
#
#   $Id: core_offset.py (converted from core_offset.trb) $
#

#
#  オフセットファイル生成用テンプレートファイル（ARM-M用）
#

IncludeTrb("kernel/genoffset.py")

offsetH.append(f"""\
#define TCB_p_tinib\t\t\t{offsetof_TCB_p_tinib}
#define TCB_pc\t\t\t\t{offsetof_TCB_pc}
#define TCB_sp\t\t\t\t{offsetof_TCB_sp}
#define TCB_stk_top         {offsetof_TCB_stk_top}
#define TCB_fpu_flag        {offsetof_TCB_fpu_flag}
#define TINIB_exinf\t\t\t{offsetof_TINIB_exinf}
#define TINIB_task\t\t\t{offsetof_TINIB_task}
#define TINIB_stk_bottom\t{offsetof_TINIB_TSKINICTXB_stk_bottom}
""")
