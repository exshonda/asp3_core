#
#		チップ依存部のCMake定義（I.MX RT600用）
#
#  target.cmake からincludeされる（Makefile.chipのCMake版に相当）．
#
#  スタートアップは共通のstart.Sではなく，リンカスクリプトのセクション
#  テーブル（__data_section_table等）を用いるstart_imxrt600.Sを使用する
#  （XIP実行のため複数のDATA/BSS領域をテーブル駆動で初期化する）．
#

set(CHIPDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc/imxrt600)

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
    ${CHIPDIR}/imxrt600_usart.c
)

#
#  コア依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_m_gcc/common/arch.cmake)

#
#  スタートアップの差し替え（start.S → start_imxrt600.S）
#
list(REMOVE_ITEM ASP3_ARCH_C_FILES ${ARCHDIR}/common/start.S)
list(APPEND ASP3_ARCH_C_FILES ${CHIPDIR}/start_imxrt600.S)
