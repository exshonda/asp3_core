#!/bin/bash
#
#		testexec.py のTARGET_RUN用ボードランナ（RaspberryPi Pico2用）
#
#  PICO2実機にOpenOCD（CMSIS-DAP／Debugprobe）で書き込み，UART出力を
#  完走マーカ（またはタイムアウト）まで取得して標準出力へ流す．
#  カレントディレクトリ＝OBJディレクトリ（asp.elfがある）で実行される．
#
#  UARTキャプチャは書き込みより先に開始する（起動直後のバナー取り
#  こぼし防止）．テストバッチを並行実行してはならない（ボードと
#  ttyを共有するため．arch/arm_m_gcc/rp2350/PORTING.md参照）．
#
#  使い方: run_board_pico2.sh [OpenOCDターゲットcfg] [タイムアウト秒]
#    ARM版    : run_board_pico2.sh target/rp2350.cfg
#    RISC-V版 : run_board_pico2.sh target/rp2350-riscv.cfg
#  環境変数: PICO2_TTY（既定 /dev/ttyACM0）
#
set -u
OCD_TGT=${1:-target/rp2350-riscv.cfg}
TMO=${2:-120}
TTY=${PICO2_TTY:-/dev/ttyACM0}

stty -F "$TTY" 115200 raw -echo

#  前のテストの残留バイト（ttyバッファ）を読み捨てる（誤判定防止）
timeout 0.3 cat "$TTY" > /dev/null 2>&1

rm -f uart.log
cat "$TTY" > uart.log & CAT=$!
trap 'kill $CAT 2>/dev/null' EXIT INT TERM
sleep 0.5

if ! openocd -f interface/cmsis-dap.cfg -f "$OCD_TGT" \
        -c "adapter speed 5000" \
        -c "program asp.elf verify reset exit" > openocd.log 2>&1; then
    echo "## openocd program failed (see openocd.log)"
    exit 1
fi

#  完走マーカ／致命エラーの検出まで待つ（最大TMO秒）．検出後も残りの
#  出力（dlynseの境界測定など）のため数秒待ってから打ち切る
for ((i = 0; i < TMO; i++)); do
    sleep 1
    if grep -qE "All check points passed\.|^## |high resolution timer count test finishes\.|-- for checking boundary conditions --|This test program is not necessary\.|Unregistered" uart.log; then
        sleep 5
        break
    fi
done

kill $CAT 2>/dev/null
wait $CAT 2>/dev/null
cat uart.log
