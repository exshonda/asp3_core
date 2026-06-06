#
#		実機実行・デバッグ用CMakeターゲット（STM32MP257F-DK用）
#
#  Makefile.targetのswd系ターゲットのCMake版．aspターゲット定義後に
#  root CMakeLists.txtから取り込まれる（ASP3_TARGET_RUN_CMAKE機構）．
#
#    openocd    : OpenOCDだけを起動（前面．Ctrl-Cで終了）
#    gdb        : gdbだけを起動しロード＆デバッグ（OpenOCDは別端末で起動済みのこと）
#    swd-run    : OpenOCDだけでロード＆実行（gdb不要）
#    swd-debug  : OpenOCDを自動起動しgdbでロード＆デバッグ
#    osdebug    : 同上＋OS-awareness（gdb-multiarch）
#    console    : UARTコンソール（picocom/minicom/cu自動選択）
#
#  上書き可能なキャッシュ変数: EN_CA35_1 / TTY / BAUD / OSGDB
#

set(OCD_DIR ${TARGETDIR}/openocd)
set(GDB_SCRIPT ${TARGETDIR}/swd-debug.gdb)
set(AWARENESS ${ASP3_ROOT_DIR}/scripts/gdb_os_aware/os_awareness.py)
set(STM32_GDB ${A35_TOOLCHAIN_PREFIX}gdb)
set(STM32_READELF ${A35_TOOLCHAIN_PREFIX}readelf)

set(EN_CA35_1 0 CACHE STRING "enable CA35 core1 in OpenOCD")
set(TTY /dev/ttyACM0 CACHE STRING "serial console device")
set(BAUD 115200 CACHE STRING "serial console baudrate")
set(OSGDB gdb-multiarch CACHE STRING "gdb for OS-awareness")

#  OpenOCDを単体起動（前面．Ctrl-Cで終了）．別端末のgdbターゲットと対で使う
add_custom_target(openocd
    WORKING_DIRECTORY ${OCD_DIR}
    COMMAND openocd -c "set EN_CA35_1 ${EN_CA35_1}" -f openocd.cfg
    USES_TERMINAL
    COMMENT "Starting OpenOCD (Ctrl-C to stop)"
)

#  gdbを単体起動しロード＆デバッグ（OpenOCDは別端末で起動済みのこと）
add_custom_target(gdb
    COMMAND ${STM32_GDB} $<TARGET_FILE:asp> -x ${GDB_SCRIPT}
    DEPENDS asp
    USES_TERMINAL
    COMMENT "Starting gdb (connects to :3333)"
)

#  OpenOCDだけでロード＆実行（gdbを使わない一発実行）
add_custom_target(swd-run
    WORKING_DIRECTORY ${OCD_DIR}
    COMMAND bash -c "ENTRY_PC=$(${STM32_READELF} -h $<TARGET_FILE:asp> | sed -n 's/.*Entry point address:\\s*//p'); exec openocd -c 'set EN_CA35_1 ${EN_CA35_1}' -f openocd.cfg -c init -c 'reset run' -c 'sleep 6000' -c 'stm32mp25x.a35_0 arp_examine' -c 'sleep 500' -c 'stm32mp25x.a35_0 arp_halt' -c 'sleep 500' -c 'stm32mp25x.a35_0 arp_poll' -c \"load_image $<TARGET_FILE:asp>\" -c \"reg pc $ENTRY_PC\" -c resume -c shutdown"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "Loading and running on STM32MP257F-DK via OpenOCD"
)

#  OpenOCDをバックグラウンド起動→gdbでロード＆デバッグ→gdb終了でOpenOCDも終了
add_custom_target(swd-debug
    COMMAND bash -c "echo '[swd-debug] starting OpenOCD in background (log: openocd-bg.log)'; ( cd ${OCD_DIR} && exec openocd -c 'set EN_CA35_1 ${EN_CA35_1}' -f openocd.cfg ) > openocd-bg.log 2>&1 & OCD_PID=$!; trap 'kill $OCD_PID 2>/dev/null' EXIT INT TERM; sleep 2; ${STM32_GDB} $<TARGET_FILE:asp> -x ${GDB_SCRIPT}"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "OpenOCD + gdb debug session"
)

#  OS-awareness付きデバッグ（gdb-multiarch）
add_custom_target(osdebug
    COMMAND bash -c "command -v ${OSGDB} >/dev/null 2>&1 || { echo '${OSGDB} not found (sudo apt-get install -y gdb-multiarch)'; exit 1; }; echo '[osdebug] in gdb: continue -> (run) -> Ctrl-C -> atask / stask'; ( cd ${OCD_DIR} && exec openocd -c 'set EN_CA35_1 ${EN_CA35_1}' -f openocd.cfg ) > openocd-bg.log 2>&1 & OCD_PID=$!; trap 'kill $OCD_PID 2>/dev/null' EXIT INT TERM; sleep 2; ${OSGDB} $<TARGET_FILE:asp> -x ${GDB_SCRIPT} -ex \"python import sys; sys.path.insert(0, '${TARGETDIR}')\" -ex 'source ${AWARENESS}'"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "OpenOCD + gdb-multiarch with OS-awareness"
)

#  シリアルコンソール（USART2／ST-LINK仮想COM）
add_custom_target(console
    COMMAND bash -c "if command -v picocom >/dev/null 2>&1; then echo '[console] picocom -b ${BAUD} ${TTY} (exit: Ctrl-A Ctrl-X)'; picocom -b ${BAUD} ${TTY}; elif command -v minicom >/dev/null 2>&1; then echo '[console] minicom -b ${BAUD} -D ${TTY} (exit: Ctrl-A X)'; minicom -b ${BAUD} -D ${TTY}; elif command -v cu >/dev/null 2>&1; then echo '[console] cu -l ${TTY} -s ${BAUD} (exit: ~.)'; cu -l ${TTY} -s ${BAUD}; else echo 'install one of picocom / minicom / cu'; exit 1; fi"
    USES_TERMINAL
    VERBATIM
    COMMENT "Opening serial console"
)
