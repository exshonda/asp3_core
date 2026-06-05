# -*- coding: utf-8 -*-
#
#		パス3の生成スクリプトのターゲット依存部（POSIX用）
#
#   $Id: posix_check.py (converted from posix_check.trb) $
#

#
#  タスク初期化コンテキストブロックからスタックの番地の取得
#
def GetStackTskinictxb(key, params, tinib):
    return 0

#
#  ターゲット非依存部のインクルード
#
IncludeTrb("kernel/kernel_check.py")
