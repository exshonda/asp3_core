#
#		アーキテクチャ依存部のCMake定義（ARM-Mコア共通）
#
#  target.cmake からincludeされる．CPU固有のフラグ（-mcpu等）と
#  コア種別の定義（TOPPERS_CORTEX_M33等）はtarget.cmake側で
#  ASP3_COMPILE_OPTIONS／ASP3_COMPILE_DEFS に積む．
#

set(ARCHDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc)

list(APPEND ASP3_SYMVAL_TABLES
    ${ARCHDIR}/common/core_sym.def
)

list(APPEND ASP3_OFFSET_TRB_FILES
    ${ARCHDIR}/common/core_offset.py
)

list(APPEND ASP3_INCLUDE_DIRS
    ${ARCHDIR}/common
    ${ASP3_ROOT_DIR}/arch/gcc
)

list(APPEND ASP3_ARCH_C_FILES
    ${ARCHDIR}/common/core_kernel_impl.c
    ${ARCHDIR}/common/core_support.S
    ${ARCHDIR}/common/start.S
)
