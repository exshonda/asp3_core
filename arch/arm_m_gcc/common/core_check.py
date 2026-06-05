# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: core_check.py (converted from core_check.trb) $
#

#
#  パス3のプロセッサ依存テンプレート（ARM-M用）
#

def GetStackTskinictxb(key, params, tinib):
    return PEEK(tinib + offsetof_TINIB_TSKINICTXB_stk_top, sizeof_void_ptr)


IncludeTrb("kernel/kernel_check.py")
