#
#		ターゲット依存部のCMake定義（ZCU102 / QEMU xlnx-zcu102用）
#
#  変数を積み上げ，チップ依存部の chip.cmake をincludeする．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/zcu102_arm64_gcc)

#
#  コンフィギュレーション関連
#
list(APPEND ASP3_CFG_FILES
    ${TARGETDIR}/target_kernel.cfg
)

list(APPEND ASP3_KERNEL_CFG_TRB_FILES
    ${TARGETDIR}/target_kernel.py
)

list(APPEND ASP3_CHECK_TRB_FILES
    ${TARGETDIR}/target_check.py
)

#
#  インクルードディレクトリ
#
list(APPEND ASP3_INCLUDE_DIRS
    ${TARGETDIR}
)

#
#  QEMU／実機の切り替え（既定：QEMU）
#
#  OFF（実機ZCU102）にすると TOPPERS_USE_QEMU を定義しない：
#    - System Timestamp Generator の初期化とCNTFRQ設定を行う（chip依存部）
#    - target_exit のセミホスティング終了を行わない
#    - DDRマップが低位2GBになる（zcu102.h）
#
option(ZCU102_QEMU "Build for QEMU xlnx-zcu102 (OFF: real ZCU102 board)" ON)

#
#  メモリアドレスの設定（先頭1MBは空けておく．QEMU xlnx-zcu102 の
#  既定RAM=128MB（0x00000000-0x07FFFFFF）にも収まる配置）
#
set(TEXT_START_ADDRESS 0x0000100000)
set(DATA_START_ADDRESS 0x0004000000)

#
#  コンパイル定義
#
list(APPEND ASP3_COMPILE_DEFS
    USE_ARM64_FPU
    TOPPERS_TZ_S
    TOPPERS_32BIT_ABOVE_ADDR
)

if(ZCU102_QEMU)
    list(APPEND ASP3_COMPILE_DEFS TOPPERS_USE_QEMU)
endif()

#
#  コンパイル・リンクオプション
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mlittle-endian
)

list(APPEND ASP3_LINK_OPTIONS
    -mlittle-endian
    -Wl,--build-id=none
    -Wl,-Ttext,${TEXT_START_ADDRESS}
    -Wl,-Tdata,${DATA_START_ADDRESS}
)

set(ASP3_LDSCRIPT ${TARGETDIR}/zcu102.ld)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
)

#
#  チップ依存部のインクルード（非TECS版SIOドライバはchip側で追加）
#
include(${ASP3_ROOT_DIR}/arch/arm64_gcc/zynqmp/chip.cmake)

#
#  QEMUによる実行（cmake --build <dir> --target run．QEMUビルド時のみ）
#
if(ZCU102_QEMU)
    set(ASP3_RUN_COMMAND
        qemu-system-aarch64 -machine xlnx-zcu102,secure=on -nographic
        -semihosting-config enable=on,target=native
        -kernel $<TARGET_FILE:asp>
    )
endif()
