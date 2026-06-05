#
#		チップ依存部のCMake定義（RP2350用）
#
#  target.cmake からincludeされる（Makefile.chipのCMake版に相当）．
#

set(CHIPDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc/rp2350)

list(APPEND ASP3_INCLUDE_DIRS
    ${CHIPDIR}
)

list(APPEND ASP3_COMPILE_OPTIONS
    -mlittle-endian
)

list(APPEND ASP3_LINK_OPTIONS
    -mlittle-endian
)

#
#  非TECS版SIOドライバ（チップ依存部）
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${CHIPDIR}/chip_serial.c
    ${CHIPDIR}/rp2350_uart.c
)

#
#  コア依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_m_gcc/common/arch.cmake)
