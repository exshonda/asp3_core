#
#		実機実行・デバッグ用CMakeターゲット（RaspberryPi Pico2 RISC-V用）
#
#  ARM版（target/raspberrypi_pico2_gcc/run.cmake）の流用．差分：
#    - OpenOCDターゲット設定：target/rp2350-riscv.cfg（RPiフォーク同梱）
#    - gdb：gdb-multiarch（riscv64-unknown-elf-gdbはaptパッケージに無い）
#  （書込み・実行はrunターゲット＝target.cmakeのASP3_RUN_COMMAND）
#
#    openocd    : OpenOCDだけを起動（前面．Ctrl-Cで終了）
#    gdb        : gdbだけを起動しロード＆デバッグ（OpenOCDは別端末で起動済みのこと）
#    swd-debug  : OpenOCDを自動起動しgdbでロード＆デバッグ
#    console    : UARTコンソール（picocom/minicom/cu自動選択）
#
#  上書き可能なキャッシュ変数: OCD_IF / OCD_TGT / OCD_SPEED / TTY / BAUD / OSGDB
#

set(GDB_SCRIPT ${TARGETDIR}/swd-debug.gdb)

set(OCD_IF interface/cmsis-dap.cfg CACHE STRING "OpenOCD interface config")
set(OCD_TGT target/rp2350-riscv.cfg CACHE STRING "OpenOCD target config")
set(OCD_SPEED 5000 CACHE STRING "OpenOCD adapter speed")
set(TTY /dev/ttyACM0 CACHE STRING "serial console device")
set(BAUD 115200 CACHE STRING "serial console baudrate")
set(OSGDB gdb-multiarch CACHE STRING "gdb for RISC-V debug")

#  OpenOCDを単体起動（前面．Ctrl-Cで終了）
add_custom_target(openocd
    COMMAND openocd -f ${OCD_IF} -f ${OCD_TGT} -c "adapter speed ${OCD_SPEED}"
    USES_TERMINAL
    COMMENT "Starting OpenOCD (Ctrl-C to stop)"
)

#  gdbを単体起動しロード＆デバッグ（OpenOCDは別端末で起動済みのこと）
add_custom_target(gdb
    COMMAND ${OSGDB} $<TARGET_FILE:asp> -x ${GDB_SCRIPT}
    DEPENDS asp
    USES_TERMINAL
    COMMENT "Starting gdb (connects to :3333)"
)

#  OpenOCDをバックグラウンド起動→gdbでロード＆デバッグ→gdb終了でOpenOCDも終了
add_custom_target(swd-debug
    COMMAND bash -c "echo '[swd-debug] starting OpenOCD in background (log: openocd-bg.log)'; openocd -f ${OCD_IF} -f ${OCD_TGT} -c 'adapter speed ${OCD_SPEED}' > openocd-bg.log 2>&1 & OCD_PID=$!; trap 'kill $OCD_PID 2>/dev/null' EXIT INT TERM; sleep 2; ${OSGDB} $<TARGET_FILE:asp> -x ${GDB_SCRIPT}"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "OpenOCD + gdb debug session"
)

#  シリアルコンソール
add_custom_target(console
    COMMAND bash -c "if command -v picocom >/dev/null 2>&1; then echo '[console] picocom -b ${BAUD} ${TTY} (exit: Ctrl-A Ctrl-X)'; picocom -b ${BAUD} ${TTY}; elif command -v minicom >/dev/null 2>&1; then echo '[console] minicom -b ${BAUD} -D ${TTY} (exit: Ctrl-A X)'; minicom -b ${BAUD} -D ${TTY}; elif command -v cu >/dev/null 2>&1; then echo '[console] cu -l ${TTY} -s ${BAUD} (exit: ~.)'; cu -l ${TTY} -s ${BAUD}; else echo 'install one of picocom / minicom / cu'; exit 1; fi"
    USES_TERMINAL
    VERBATIM
    COMMENT "Opening serial console"
)
