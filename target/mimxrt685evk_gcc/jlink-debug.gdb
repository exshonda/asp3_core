# jlink-debug.gdb -- NXP EVK-MIMXRT685 用 gdb 自己完結スクリプト
#
#   J-Link GDB Server(:2331)へ接続し，
#     monitor reset（リセットして停止）→ load（フラッシュへ書き込み）→ monitor reset
#   までを行い，halt のまま待機する．break 設定後に continue で実行を開始する．
#
#   前提: J-Link GDB Server が起動済みであること（ninja jlinkgdbserver）．
#         ninja jlink-debug / osdebug はサーバの起動も含めて 1 端末で行う．

target remote localhost:2331

# リセットして停止（bootROM 実行後＝halt after bootloader）
monitor reset

# ELF をフラッシュへ書き込み（J-Link のフラッシュローダ経由）
load

# 再度リセットして停止（bootROM から実行を開始できる状態で待機）
monitor reset

echo \n[jlink-debug] loaded. set breakpoints and 'continue' to run.\n
