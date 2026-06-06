# -*- coding: utf-8 -*-
#
#		パス2の生成スクリプトの割込みコントローラ依存部（PLIC用）
#
#   $Id: plic_kernel.py (converted from plic_kernel.trb) $
#

#
#  シングルプロセッサのため，FMP3版にあった割込みターゲットコンテキスト
#  INDEXテーブル（plic_target_cidx_table）の生成と，複数プロセッサでの
#  割込み受付けに関するチェックは不要．
#
