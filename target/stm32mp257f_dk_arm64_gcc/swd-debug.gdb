# TOPPERS/FMP3 STM32MP257F-DK : SWD ロード & ソースデバッグ（gdb だけで完結）
#
# 処理: OpenOCD(:3333) に接続 → reset run（DDR 初期化, landing pad で停止）→ a35_0 examine/halt
#       → ELF を load → エントリへ PC 設定 → halt のまま待機．break 設定後 continue でデバッグ．
#       そのまま走らせるだけなら continue（または monitor resume）．
#
# 使い方:
#   (A) ビルドディレクトリ(<OBJ>)の make ルールから:
#         make openocd     # 別端末で OpenOCD を起動しておく
#         make gdb         # 本スクリプトで gdb を起動（OpenOCD 自動起動込みなら make swd-debug）
#   (B) 直接起動（OpenOCD を別端末で起動済みのこと）:
#         aarch64-none-elf-gdb <OBJ>/asp \
#           -x <FMP3>/target/stm32mp257f_dk_arm64_gcc/swd-debug.gdb
#
#   OpenOCD の起動は同梱の openocd/ で:
#     cd <FMP3>/target/stm32mp257f_dk_arm64_gcc/openocd
#     openocd -c "set EN_CA35_1 0" -f openocd.cfg

set pagination off
set remotetimeout 20

# OpenOCD(:3333) に接続
target extended-remote localhost:3333

# FSBL を走らせて DDR 初期化 → landing pad("Connect using OpenOCD")で停止させる
monitor reset run
# DDR 初期化 + landing pad 起動を待つ（環境により調整）
shell sleep 3

# CA35 が起動しきった後に a35_0 を examine/halt（reset 直後は早すぎて失敗するため）
monitor stm32mp25x.a35_0 arp_examine
monitor stm32mp25x.a35_0 arp_halt

# ELF をターゲット(LMA)へ転送
load

# エントリ(start)へ PC を設定する
#   ※ TEXT_START を変えた場合は readelf -h <asp> の Entry に合わせること
monitor reg pc 0x88001000

# 起点となるブレークポイントの例（必要に応じてコメントを外す／書き換える）
# break main
# break sta_ker

echo \n[swd-debug] Loaded. Target is halted and ready.\n
echo [swd-debug] Set a breakpoint then 'continue' to debug.\n
echo [swd-debug] To just run, type 'continue' (or 'monitor resume').\n
