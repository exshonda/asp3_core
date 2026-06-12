# linkserver-debug.gdb -- NXP EVK-MIMXRT685 用 gdb スクリプト（LinkServer用）
#
#   LinkServer gdbserver(:3333)へ接続する．
#   - ninja debug    : 非attach（接続時にリセット→bootROMストールで停止）．
#                      break 設定後に continue で先頭から実行を開始する．
#   - ninja osdebug  : attach（実行中システムへ接続）．continue →（実行中に）
#                      Ctrl-C → atask / stask / intr で断面を観測する．
#
#   set mem inaccessible-by-default off が必須：LinkServer のデバイス定義
#   （EVK-MIMXRT685.json）のメモリマップには SRAM のデータバスエイリアス
#   （0x20000000〜）が無く，既定では gdb がその領域へのアクセスを拒否して
#   カーネルオブジェクトを読めない（"Cannot access memory"）．
#
#   前提: LinkServer gdbserver が起動済みであること（ninja gdbserver）．
#         ninja debug / osdebug はサーバの起動も含めて 1 端末で行う．

set mem inaccessible-by-default off

target remote localhost:3333

echo \n[linkserver-debug] connected. 'load' to (re)flash, 'continue' to run.\n
