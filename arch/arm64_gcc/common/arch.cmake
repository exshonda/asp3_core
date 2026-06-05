#
#		アーキテクチャ依存部のCMake定義（ARM64コア共通）
#
#  chip.cmake／target.cmake からincludeされる．
#  start.S はライブラリ外で先頭にリンクする（ASP3_START_FILES）．
#

set(COREDIR ${ASP3_ROOT_DIR}/arch/arm64_gcc/common)

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

list(APPEND ASP3_COMPILE_OPTIONS
    -mstrict-align
)

list(APPEND ASP3_ARCH_C_FILES
    ${COREDIR}/core_kernel_impl.c
    ${COREDIR}/core_timer.c
    ${COREDIR}/arm64.c
    ${COREDIR}/gic_kernel_impl.c
    ${COREDIR}/core_support.S
    ${COREDIR}/gic_support.S
)

list(APPEND ASP3_START_FILES
    ${COREDIR}/start.S
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
)

list(APPEND ASP3_LINK_LIBS c gcc)
