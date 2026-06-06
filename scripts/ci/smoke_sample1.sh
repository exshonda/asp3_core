#!/usr/bin/env bash
#
#		sample1スモークテスト（CI・ローカル共用）
#
#  与えられた実行コマンド（QEMU等）をtimeout付きで起動し，出力に
#    1. 起動バナー（TOPPERS/ASP3 Kernel）
#    2. タスク実行ログ（task1 is running）
#  が含まれることを確認する．sample1は無限実行のためtimeoutによる
#  打ち切りは正常（合否はgrepのみで判定）．
#
# 【実行方法】
#	smoke_sample1.sh <timeout秒> <実行コマンド...>
#
# 【例】
#	scripts/ci/smoke_sample1.sh 20 ./build/linux/asp
#	scripts/ci/smoke_sample1.sh 30 qemu-system-arm -M xilinx-zynq-a9 \
#	    -semihosting -nographic -serial /dev/null -serial mon:stdio \
#	    -kernel build/zybo_z7-qemu/asp.elf
#
set -u

if [ $# -lt 2 ]; then
    echo "usage: $0 <timeout_sec> <command...>" >&2
    exit 2
fi

timeout_sec=$1
shift

log=$(mktemp)
trap 'rm -f "$log"' EXIT

#  </dev/null：QEMUモニタ等が標準入力を待たないようにする
timeout --signal=TERM --kill-after=5 "$timeout_sec" "$@" </dev/null >"$log" 2>&1
status=$?

echo "== output (exit=$status, timeout=${timeout_sec}s) =="
cat "$log"
echo "=============================================="

pass=0
if ! grep -q "TOPPERS/ASP3 Kernel" "$log"; then
    echo "not ok 1 - banner 'TOPPERS/ASP3 Kernel' not found"
    pass=1
else
    echo "ok 1 - banner found"
fi
if ! grep -q "task1 is running" "$log"; then
    echo "not ok 2 - 'task1 is running' not found"
    pass=1
else
    echo "ok 2 - task1 is running"
fi

exit $pass
