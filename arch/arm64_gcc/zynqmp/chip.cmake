#
#		チップ依存部のCMake定義（ZynqMP用）
#
#  target.cmake からincludeされる（Makefile.chipのCMake版に相当）．
#

set(CHIPDIR ${ASP3_ROOT_DIR}/arch/arm64_gcc/zynqmp)

list(APPEND ASP3_INCLUDE_DIRS
    ${CHIPDIR}
)

list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-a53
    -fno-stack-protector
    -fno-pic
)

list(APPEND ASP3_LINK_OPTIONS
    -mcpu=cortex-a53
    -nostdlib
    -static
    -Wl,--no-dynamic-linker
    -Wl,-N
)

list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_CORTEX_A53
    # セキュア割込みは FIQ 配送・CPUロック=FIQマスク(chip_kernel.h の TOPPERS_TZ_S 分岐)．
    # ネイティブな GICv2+Secure 構成で GIC_NO_FIQ_IN_SECURE は不要．
)

list(APPEND ASP3_ARCH_C_FILES
    ${CHIPDIR}/chip_kernel_impl.c
)

#
#  非TECS版SIOドライバ（Cadence UART = XUartPs）
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${CHIPDIR}/chip_serial.c
    ${CHIPDIR}/xuartps.c
)

#
#  コア依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm64_gcc/common/arch.cmake)

#
#  ベアメタルリンクのため libc は使用しない（aarch64-linux-gnu 等の
#  glibc 系ツールチェーンで静的 glibc が混入するのを防ぐ）．
#  コンパイラが生成する memcpy/memset 等は libc_stub.c が提供する．
#
list(REMOVE_ITEM ASP3_LINK_LIBS c)
list(APPEND ASP3_ARCH_C_FILES
    ${CHIPDIR}/libc_stub.c
)
