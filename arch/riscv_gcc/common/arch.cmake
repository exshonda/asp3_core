#
#		アーキテクチャ依存部のCMake定義（RISC-Vコア共通）
#
#  chip.cmake／target.cmake からincludeされる．
#  start.S はライブラリ外で先頭にリンクする（ASP3_START_FILES）．
#

set(COREDIR ${ASP3_ROOT_DIR}/arch/riscv_gcc/common)

list(APPEND ASP3_SYMVAL_TABLES
    ${COREDIR}/core_sym.def
)

list(APPEND ASP3_OFFSET_TRB_FILES
    ${COREDIR}/core_offset.py
)

list(APPEND ASP3_INCLUDE_DIRS
    ${COREDIR}
    ${ASP3_ROOT_DIR}/arch/gcc
)

list(APPEND ASP3_ARCH_C_FILES
    ${COREDIR}/core_kernel_impl.c
    ${COREDIR}/plic_kernel_impl.c
    ${COREDIR}/mtimer.c
    ${COREDIR}/core_support.S
)

list(APPEND ASP3_START_FILES
    ${COREDIR}/start.S
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
)

list(APPEND ASP3_LINK_LIBS c gcc)
