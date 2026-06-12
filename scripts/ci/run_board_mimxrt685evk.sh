#!/bin/bash
#
#		testexec.py のTARGET_RUN用ボードランナ（EVK-MIMXRT685用）
#
#  EVK-MIMXRT685実機にJ-Link（オンボードLPC-Link2をJ-Linkファームウェア化）で
#  書き込み，UART出力を完走マーカ（またはタイムアウト）まで取得して標準出力へ
#  流す．カレントディレクトリ＝OBJディレクトリ（asp.elfがある）で実行される．
#
#  UARTキャプチャは書き込みより先に開始する（起動直後のバナー取り
#  こぼし防止）．テストバッチを並行実行してはならない（ボードと
#  ttyを共有するため）．
#
#  使い方: run_board_mimxrt685evk.sh [タイムアウト秒]
#  環境変数: MIMXRT685_TTY（既定 /dev/ttyACM0＝J-Link CDC VCOM）
#            JLINK_DEVICE（既定 MIMXRT685S_M33）
#
set -u
TMO=${1:-120}
TTY=${MIMXRT685_TTY:-/dev/ttyACM0}
DEV=${JLINK_DEVICE:-MIMXRT685S_M33}

stty -F "$TTY" 115200 raw -echo

#  前のテストの残留バイト（ttyバッファ）を読み捨てる（誤判定防止）
timeout 0.3 cat "$TTY" > /dev/null 2>&1

rm -f uart.log
cat "$TTY" > uart.log & CAT=$!
trap 'kill $CAT 2>/dev/null' EXIT INT TERM
sleep 0.5

#  loadfile はフラッシュ書込み（implicit reset & halt）．r＝リセット後 g＝実行
printf 'loadfile asp.elf\nr\ng\nqc\n' > jlink_flash.jlink
if ! JLinkExe -device "$DEV" -if SWD -speed 4000 -autoconnect 1 -NoGui 1 \
        -CommandFile jlink_flash.jlink > jlink.log 2>&1; then
    echo "## JLinkExe flash failed (see jlink.log)"
    exit 1
fi
if ! grep -q '^O\.K\.$' jlink.log; then
    echo "## JLinkExe flash not confirmed (see jlink.log)"
    exit 1
fi

#  完走マーカ／致命エラーの検出まで待つ（最大TMO秒）．検出後も残りの
#  出力（dlynseの境界測定など）のため数秒待ってから打ち切る
for ((i = 0; i < TMO; i++)); do
    sleep 1
    if grep -qE "All check points passed\.|^## |high resolution timer count test finishes\.|-- for checking boundary conditions --|This test program is not necessary\.|Unregistered|# [0-9]+/[0-9]+ passed" uart.log; then
        sleep 5
        break
    fi
done

kill $CAT 2>/dev/null
wait $CAT 2>/dev/null
cat uart.log
