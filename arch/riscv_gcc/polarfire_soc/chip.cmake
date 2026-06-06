#
#		チップ依存部のCMake定義（PolarFire SoC用）
#
#  target.cmake からincludeされる（Makefile.chipのCMake版に相当）．
#

set(CHIPDIR ${ASP3_ROOT_DIR}/arch/riscv_gcc/polarfire_soc)

list(APPEND ASP3_INCLUDE_DIRS
    ${CHIPDIR}
)

list(APPEND ASP3_COMPILE_OPTIONS
    -march=rv64gc
    -mabi=lp64d
    -mcmodel=medany
    -msmall-data-limit=8
    -mstrict-align
    -mno-save-restore
    -fsigned-char
)

list(APPEND ASP3_LINK_OPTIONS
    -march=rv64gc
    -mabi=lp64d
)

list(APPEND ASP3_ARCH_C_FILES
    ${CHIPDIR}/chip_kernel_impl.c
    ${CHIPDIR}/chip_support.S
)

#
#  非TECS版SIOドライバ（MMUART）
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${CHIPDIR}/chip_serial.c
    ${CHIPDIR}/mmuart.c
)

#
#  コア依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/riscv_gcc/common/arch.cmake)

#
#  ベアメタルリンクのため libc は使用しない（aptのriscv64-unknown-elf
#  ツールチェーンはlibcを同梱しない）．コンパイラが生成する
#  memcpy/memset 等は libc_stub.c が提供する．
#
list(REMOVE_ITEM ASP3_LINK_LIBS c)
list(APPEND ASP3_ARCH_C_FILES
    ${CHIPDIR}/libc_stub.c
)
