# swd-debug.gdb -- Raspberry Pi Pico 2 (RP2350 RISC-V) 用 gdb 自己完結スクリプト
#
#   OpenOCD の gdb サーバ(:3333)へ接続し，
#     reset init（リセットして停止）→ load（フラッシュへ書き込み）→ reset init
#   までを行い，halt のまま待機する．break 設定後に continue で実行を開始する．
#
#   前提: OpenOCD（Raspberry Piフォーク・target/rp2350-riscv.cfg）が
#         起動済みであること（--target openocd）．
#         --target swd-debug は OpenOCD の起動も含めて 1 端末で行う．

target extended-remote localhost:3333

# リセットして停止（bootROM 実行前の状態）
monitor reset init

# ELF をフラッシュへ書き込み（OpenOCD のフラッシュドライバ経由）
load

# 再度リセットして停止（bootROM から実行を開始できる状態で待機）
monitor reset init

echo \n[swd-debug] loaded. set breakpoints and 'continue' to run.\n
