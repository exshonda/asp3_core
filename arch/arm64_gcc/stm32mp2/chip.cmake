#
#		チップ依存部のCMake定義（STM32MP2用）
#
#  target.cmake からincludeされる（Makefile.chipのCMake版に相当）．
#

set(CHIPDIR ${ASP3_ROOT_DIR}/arch/arm64_gcc/stm32mp2)

list(APPEND ASP3_INCLUDE_DIRS
    ${CHIPDIR}
)

list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-a35
)

list(APPEND ASP3_LINK_OPTIONS
    -mcpu=cortex-a35
    -Wl,-N
)

list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_CORTEX_A35
    # セキュア割込みは FIQ 配送・CPUロック=FIQマスク(chip_kernel.h の TOPPERS_TZ_S 分岐)．
    # zynqmp と同じネイティブ Secure 構成で GIC_NO_FIQ_IN_SECURE は不要(実機実証済み)．
)

list(APPEND ASP3_ARCH_C_FILES
    ${CHIPDIR}/chip_kernel_impl.c
)

#
#  コア依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm64_gcc/common/arch.cmake)
