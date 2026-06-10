#
#		ターゲット依存部のCMake定義（RaspberryPi Pico2用）
#
#  変数を積み上げ，チップ依存部の chip.cmake をincludeする
#  （Makefile.targetのCMake版に相当）．pico-sdkは使用しない
#  （ベアメタル移植．arch/arm_m_gcc/rp2350/PORTING.md参照）．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/pico2_arm_gcc)

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
#  コンパイル定義（Makefile.target/chip/coreと同一）
#
list(APPEND ASP3_COMPILE_DEFS
    USE_TIM_AS_HRT
    TOPPERS_CORTEX_M33
    TOPPERS_ENABLE_TRUSTZONE
    __TARGET_ARCH_THUMB=5
    __TARGET_FPU_FPV4_SP
    TOPPERS_FPU_ENABLE
    TOPPERS_FPU_LAZYSTACKING
    TOPPERS_FPU_CONTEXT
)

#
#  コンパイル・リンクオプション（Makefile.target/chip/coreと同一）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=softfp
    -mfpu=fpv5-sp-d16
    -ffunction-sections
    -nostartfiles
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
    -nostartfiles
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=softfp
    -mfpu=fpv5-sp-d16
    -Wl,--print-memory-usage
    -Wl,--gc-sections
)

list(APPEND ASP3_LINK_LIBS c gcc)

set(ASP3_LDSCRIPT ${TARGETDIR}/rpi_pico2.ld)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
    ${TARGETDIR}/image_def.S
)

#
#  チップ依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_m_gcc/rp2350/chip.cmake)

#
#  実機への書込み・実行（cmake --build <dir> --target run）
#  OpenOCD（CMSIS-DAP／debugprobe）でprogram→reset
#
set(ASP3_RUN_COMMAND
    openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg
    -c "adapter speed 5000"
    -c "program $<TARGET_FILE:asp> verify reset exit"
)

#
#  デバッグターゲット（openocd/gdb/console等．aspターゲット定義後に取込み）
#
set(ASP3_TARGET_RUN_CMAKE ${TARGETDIR}/run.cmake)
