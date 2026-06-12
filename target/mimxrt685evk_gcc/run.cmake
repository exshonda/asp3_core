#
#		実機実行・デバッグ用CMakeターゲット（NXP EVK-MIMXRT685用）
#
#  aspターゲット定義後にroot CMakeLists.txtから取り込まれる
#  （ASP3_TARGET_RUN_CMAKE機構）．書込み・デバッグはオンボードの
#  MCU-Link/LPC-Link2をJ-Linkファームウェア化したプローブを前提とする
#  （SEGGER J-Link Software導入済みのこと．target_user.md参照）．
#
#    jlink-run      : JLinkExeでフラッシュ書込み→リセット→実行（gdb不要の一発実行）
#    jlinkgdbserver : J-Link GDB Serverだけを起動（前面．Ctrl-Cで終了）
#    gdb            : gdbだけを起動しロード＆デバッグ（サーバは別端末で起動済みのこと）
#    jlink-debug    : J-Link GDB Serverを自動起動しgdbでロード＆デバッグ
#    osdebug        : 同上＋OS-awareness（gdb-multiarch）
#    console        : UARTコンソール（picocom/minicom/cu自動選択）
#
#  上書き可能なキャッシュ変数: JLINK_DEVICE / JLINK_SPEED / JLINK_GDBPORT /
#                              TTY / BAUD / OSGDB
#

set(GDB_SCRIPT ${TARGETDIR}/jlink-debug.gdb)
set(AWARENESS ${ASP3_ROOT_DIR}/scripts/gdb_os_aware/os_awareness.py)

set(JLINK_DEVICE MIMXRT685S_M33 CACHE STRING "J-Link device name")
set(JLINK_SPEED 4000 CACHE STRING "J-Link interface speed (kHz)")
set(JLINK_GDBPORT 2331 CACHE STRING "J-Link GDB Server port")
set(TTY /dev/ttyACM0 CACHE STRING "serial console device (J-Link CDC VCOM)")
set(BAUD 115200 CACHE STRING "serial console baudrate")
set(OSGDB gdb-multiarch CACHE STRING "gdb for OS-awareness")

#  JLinkExeでフラッシュ書込み→リセット→実行（gdbを使わない一発実行）
add_custom_target(jlink-run
    COMMAND bash -c "printf 'loadfile $<TARGET_FILE:asp>\\nr\\ng\\nqc\\n' > jlink-run.jlink; JLinkExe -device ${JLINK_DEVICE} -if SWD -speed ${JLINK_SPEED} -autoconnect 1 -NoGui 1 -CommandFile jlink-run.jlink"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "Flashing and running on EVK-MIMXRT685 via J-Link"
)

#  J-Link GDB Serverを単体起動（前面．Ctrl-Cで終了）．別端末のgdbターゲットと対で使う
add_custom_target(jlinkgdbserver
    COMMAND JLinkGDBServerCLExe -device ${JLINK_DEVICE} -if SWD
            -speed ${JLINK_SPEED} -port ${JLINK_GDBPORT}
    USES_TERMINAL
    COMMENT "Starting J-Link GDB Server (Ctrl-C to stop)"
)

#  gdbを単体起動しロード＆デバッグ（サーバは別端末で起動済みのこと）
add_custom_target(gdb
    COMMAND arm-none-eabi-gdb $<TARGET_FILE:asp> -x ${GDB_SCRIPT}
    DEPENDS asp
    USES_TERMINAL
    COMMENT "Starting gdb (connects to :${JLINK_GDBPORT})"
)

#  J-Link GDB Serverをバックグラウンド起動→gdbでロード＆デバッグ→gdb終了でサーバも終了
add_custom_target(jlink-debug
    COMMAND bash -c "echo '[jlink-debug] starting J-Link GDB Server in background (log: jlinkgdbserver-bg.log)'; JLinkGDBServerCLExe -device ${JLINK_DEVICE} -if SWD -speed ${JLINK_SPEED} -port ${JLINK_GDBPORT} -silent > jlinkgdbserver-bg.log 2>&1 & SRV_PID=$!; trap 'kill $SRV_PID 2>/dev/null' EXIT INT TERM; sleep 2; arm-none-eabi-gdb $<TARGET_FILE:asp> -x ${GDB_SCRIPT}"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "J-Link GDB Server + gdb debug session"
)

#  OS-awareness付きデバッグ（gdb-multiarch）
add_custom_target(osdebug
    COMMAND bash -c "command -v ${OSGDB} >/dev/null 2>&1 || { echo '${OSGDB} not found (sudo apt-get install -y gdb-multiarch)'; exit 1; }; echo '[osdebug] in gdb: continue -> (run) -> Ctrl-C -> atask / stask'; JLinkGDBServerCLExe -device ${JLINK_DEVICE} -if SWD -speed ${JLINK_SPEED} -port ${JLINK_GDBPORT} -silent > jlinkgdbserver-bg.log 2>&1 & SRV_PID=$!; trap 'kill $SRV_PID 2>/dev/null' EXIT INT TERM; sleep 2; ${OSGDB} $<TARGET_FILE:asp> -x ${GDB_SCRIPT} -ex \"python import sys; sys.path.insert(0, '${TARGETDIR}')\" -ex 'source ${AWARENESS}'"
    DEPENDS asp
    USES_TERMINAL
    VERBATIM
    COMMENT "J-Link GDB Server + gdb-multiarch with OS-awareness"
)

#  シリアルコンソール（Flexcomm0 USART／J-Link CDC VCOM）
add_custom_target(console
    COMMAND bash -c "if command -v picocom >/dev/null 2>&1; then echo '[console] picocom -b ${BAUD} ${TTY} (exit: Ctrl-A Ctrl-X)'; picocom -b ${BAUD} ${TTY}; elif command -v minicom >/dev/null 2>&1; then echo '[console] minicom -b ${BAUD} -D ${TTY} (exit: Ctrl-A X)'; minicom -b ${BAUD} -D ${TTY}; elif command -v cu >/dev/null 2>&1; then echo '[console] cu -l ${TTY} -s ${BAUD} (exit: ~.)'; cu -l ${TTY} -s ${BAUD}; else echo 'install one of picocom / minicom / cu'; exit 1; fi"
    USES_TERMINAL
    VERBATIM
    COMMENT "Opening serial console"
)
