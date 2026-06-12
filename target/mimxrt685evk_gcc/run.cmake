#
#		実機実行・デバッグ用CMakeターゲット（NXP EVK-MIMXRT685用）
#
#  aspターゲット定義後にroot CMakeLists.txtから取り込まれる
#  （ASP3_TARGET_RUN_CMAKE機構）．
#
#  オンボードのLPC-Link2のファームウェアに応じて，書込み・デバッグの
#  ツールをキャッシュ変数 MIMXRT685_DEBUGGER で切り替える：
#    linkserver（既定）: CMSIS-DAP（出荷時標準ファームウェア）＋LinkServer
#    jlink             : J-Linkファームウェア＋SEGGER J-Link Software
#  例: cmake --preset mimxrt685evk -B build/mimxrt685evk -DMIMXRT685_DEBUGGER=jlink
#
#  ターゲット（どちらのデバッガでも同名）：
#    flash      : フラッシュ書込み→リセット実行（gdb不要の一発実行）
#    gdbserver  : gdbサーバだけを起動（前面．Ctrl-Cで終了）
#    gdb        : gdbだけを起動し接続（サーバは別端末で起動済みのこと）
#    debug      : gdbサーバを自動起動しgdbで接続
#    osdebug    : 同上＋OS-awareness（gdb-multiarch）
#    console    : UARTコンソール（picocom/minicom/cu自動選択）
#
#  上書き可能なキャッシュ変数: MIMXRT685_DEBUGGER / LINKSERVER / LS_DEVICE /
#                              JLINK_DEVICE / JLINK_SPEED / GDBPORT /
#                              TTY / BAUD / OSGDB
#

set(AWARENESS ${ASP3_ROOT_DIR}/scripts/gdb_os_aware/os_awareness.py)

set(MIMXRT685_DEBUGGER linkserver CACHE STRING
    "debug probe firmware/tool: linkserver | jlink")
set(LINKSERVER /usr/local/LinkServer/LinkServer CACHE STRING "LinkServer executable")
set(LS_DEVICE MIMXRT685S:EVK-MIMXRT685 CACHE STRING "LinkServer device:board")
set(JLINK_DEVICE MIMXRT685S_M33 CACHE STRING "J-Link device name")
set(JLINK_SPEED 4000 CACHE STRING "J-Link interface speed (kHz)")
set(GDBPORT "" CACHE STRING "gdb server port (default: linkserver=3333 / jlink=2331)")
set(TTY /dev/ttyACM0 CACHE STRING "serial console device (debug probe VCOM)")
set(BAUD 115200 CACHE STRING "serial console baudrate")
set(OSGDB gdb-multiarch CACHE STRING "gdb for OS-awareness")

if(MIMXRT685_DEBUGGER STREQUAL "jlink")
    #  J-Linkファームウェア＋SEGGER J-Link Software
    if(NOT GDBPORT)
        set(GDBPORT 2331)
    endif()
    set(GDB_SCRIPT ${TARGETDIR}/jlink-debug.gdb)
    set(FLASH_COMMAND
        "printf 'loadfile $<TARGET_FILE:asp>\\nr\\ng\\nqc\\n' > flash.jlink && JLinkExe -device ${JLINK_DEVICE} -if SWD -speed ${JLINK_SPEED} -autoconnect 1 -NoGui 1 -CommandFile flash.jlink")
    set(GDBSERVER_COMMAND
        "JLinkGDBServerCLExe -device ${JLINK_DEVICE} -if SWD -speed ${JLINK_SPEED} -port ${GDBPORT}")
    set(GDBSERVER_BG_COMMAND "${GDBSERVER_COMMAND} -silent")
    #  J-Linkは接続時リセットなし＝実行中システムにそのまま接続できる
    set(GDBSERVER_OS_COMMAND "${GDBSERVER_BG_COMMAND}")
else()
    #  CMSIS-DAP（標準ファームウェア）＋LinkServer
    if(NOT GDBPORT)
        set(GDBPORT 3333)
    endif()
    set(GDB_SCRIPT ${TARGETDIR}/linkserver-debug.gdb)
    set(FLASH_COMMAND
        "${LINKSERVER} flash ${LS_DEVICE} load $<TARGET_FILE:asp>")
    set(GDBSERVER_COMMAND
        "${LINKSERVER} gdbserver --gdb-port ${GDBPORT} ${LS_DEVICE}")
    set(GDBSERVER_BG_COMMAND "${GDBSERVER_COMMAND}")
    #  osdebug（実行中システムの観測）はattachモード＝接続時にリセットしない
    set(GDBSERVER_OS_COMMAND
        "${LINKSERVER} gdbserver --attach --gdb-port ${GDBPORT} ${LS_DEVICE}")
endif()

#  フラッシュ書込み→リセット実行（gdbを使わない一発実行）
add_custom_target(flash
    COMMAND bash -c "${FLASH_COMMAND}"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "Flashing and running on EVK-MIMXRT685 (${MIMXRT685_DEBUGGER})"
)

#  gdbサーバを単体起動（前面．Ctrl-Cで終了）．別端末のgdbターゲットと対で使う
add_custom_target(gdbserver
    COMMAND bash -c "${GDBSERVER_COMMAND}"
    USES_TERMINAL
    VERBATIM
    COMMENT "Starting gdb server on :${GDBPORT} (${MIMXRT685_DEBUGGER}, Ctrl-C to stop)"
)

#  gdbを単体起動し接続（サーバは別端末で起動済みのこと）
add_custom_target(gdb
    COMMAND arm-none-eabi-gdb $<TARGET_FILE:asp> -x ${GDB_SCRIPT}
    DEPENDS asp
    USES_TERMINAL
    COMMENT "Starting gdb (connects to :${GDBPORT})"
)

#  gdbサーバをバックグラウンド起動→gdbで接続→gdb終了でサーバも終了
add_custom_target(debug
    COMMAND bash -c "echo '[debug] starting gdb server in background (log: gdbserver-bg.log)'; ${GDBSERVER_BG_COMMAND} > gdbserver-bg.log 2>&1 & SRV_PID=$!; trap 'kill $SRV_PID 2>/dev/null' EXIT INT TERM; sleep 2; arm-none-eabi-gdb $<TARGET_FILE:asp> -x ${GDB_SCRIPT}"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "gdb server + gdb debug session (${MIMXRT685_DEBUGGER})"
)

#  OS-awareness付きデバッグ（gdb-multiarch）
add_custom_target(osdebug
    COMMAND bash -c "command -v ${OSGDB} >/dev/null 2>&1 || { echo '${OSGDB} not found (sudo apt-get install -y gdb-multiarch)'; exit 1; }; echo '[osdebug] in gdb: continue -> (run) -> Ctrl-C -> atask / stask'; ${GDBSERVER_OS_COMMAND} > gdbserver-bg.log 2>&1 & SRV_PID=$!; trap 'kill $SRV_PID 2>/dev/null' EXIT INT TERM; sleep 2; ${OSGDB} $<TARGET_FILE:asp> -x ${GDB_SCRIPT} -ex \"python import sys; sys.path.insert(0, '${TARGETDIR}')\" -ex 'source ${AWARENESS}'"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "gdb server + gdb-multiarch with OS-awareness (${MIMXRT685_DEBUGGER})"
)

#  シリアルコンソール（Flexcomm0 USART／デバッグプローブのVCOM）
add_custom_target(console
    COMMAND bash -c "if command -v picocom >/dev/null 2>&1; then echo '[console] picocom -b ${BAUD} ${TTY} (exit: Ctrl-A Ctrl-X)'; picocom -b ${BAUD} ${TTY}; elif command -v minicom >/dev/null 2>&1; then echo '[console] minicom -b ${BAUD} -D ${TTY} (exit: Ctrl-A X)'; minicom -b ${BAUD} -D ${TTY}; elif command -v cu >/dev/null 2>&1; then echo '[console] cu -l ${TTY} -s ${BAUD} (exit: ~.)'; cu -l ${TTY} -s ${BAUD}; else echo 'install one of picocom / minicom / cu'; exit 1; fi"
    USES_TERMINAL
    VERBATIM
    COMMENT "Opening serial console"
)
