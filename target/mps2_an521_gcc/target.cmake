#
#		ターゲット依存部のCMake定義（ARM MPS2-AN521 / QEMU用）
#
#  変数を積み上げ，アーキ依存部の arch.cmake をincludeする
#  （AGENTS.md §11．Makefile.targetのCMake版に相当）．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/mps2_an521_gcc)

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
#  コンパイル定義（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_USE_QEMU
    USE_TIM_AS_HRT
    TOPPERS_CORTEX_M33
    TOPPERS_ENABLE_TRUSTZONE
    __TARGET_ARCH_THUMB=5
)

#
#  コンパイル・リンクオプション（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-m33
    -mthumb
    -mlittle-endian
    -ffunction-sections
    -fdata-sections
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
    -nostartfiles
    -mcpu=cortex-m33
    -mthumb
    -mlittle-endian
    -Wl,--print-memory-usage
    -Wl,--gc-sections
)

list(APPEND ASP3_LINK_LIBS c gcc)

set(ASP3_LDSCRIPT ${TARGETDIR}/mps2_an521.ld)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
)

#
#  非TECS版SIOドライバ
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${TARGETDIR}/target_serial.c
    ${TARGETDIR}/cmsdk_uart.c
)

#
#  アーキ依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_m_gcc/common/arch.cmake)

#
#  QEMUによる実行（cmake --build <dir> --target run）
#
set(ASP3_RUN_COMMAND
    qemu-system-arm -machine mps2-an521 -nographic
    -semihosting-config enable=on,target=native
    -kernel $<TARGET_FILE:asp>
)
