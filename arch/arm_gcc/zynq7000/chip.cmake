#
#		チップ依存部のCMake定義（Zynq7000用）
#
#  target.cmake からincludeされる（Makefile.chipのCMake版に相当）．
#

set(CHIPDIR ${ASP3_ROOT_DIR}/arch/arm_gcc/zynq7000)
set(ARM_COMMON_DIR ${ASP3_ROOT_DIR}/arch/arm_gcc/common)

list(APPEND ASP3_INCLUDE_DIRS
    ${CHIPDIR}
)

list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-a9+nofp
)

list(APPEND ASP3_LINK_OPTIONS
    -mcpu=cortex-a9+nofp
)

list(APPEND ASP3_COMPILE_DEFS
    __TARGET_ARCH_ARM=7
)

list(APPEND ASP3_ARCH_C_FILES
    ${CHIPDIR}/chip_kernel_impl.c
    ${ARM_COMMON_DIR}/mpcore_kernel_impl.c
    ${ARM_COMMON_DIR}/gic_kernel_impl.c
    ${ARM_COMMON_DIR}/pl310.c
    ${ARM_COMMON_DIR}/mpcore_timer.c
    ${ARM_COMMON_DIR}/gic_support.S
)

#
#  コア依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_gcc/common/arch.cmake)
