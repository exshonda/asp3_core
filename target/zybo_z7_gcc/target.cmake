#
#		ターゲット依存部のCMake定義（ZYBO Z7用）
#
#  変数を積み上げ，チップ依存部の chip.cmake をincludeする
#  （Makefile.targetのCMake版に相当）．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/zybo_z7_gcc)

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
#  コンパイル・リンクオプション（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_DEFS
    USE_ARM_FPU_ALWAYS
)

list(APPEND ASP3_COMPILE_OPTIONS
    -mfpu=vfpv3-d16
    -mfloat-abi=hard
    -mlittle-endian
)

list(APPEND ASP3_LINK_OPTIONS
    -mfpu=vfpv3-d16
    -mfloat-abi=hard
    -mlittle-endian
)

set(ASP3_LDSCRIPT ${TARGETDIR}/zybo_z7.ld)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
)

#
#  非TECS版SIOドライバ
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${TARGETDIR}/target_serial.c
    ${ASP3_ROOT_DIR}/arch/arm_gcc/zynq7000/xuartps.c
)

#
#  チップ依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_gcc/zynq7000/chip.cmake)

#
#  QEMUによる実行（cmake --build <dir> --target run）
#  UART1（コンソール）が2本目の-serialに接続される
#
set(ASP3_RUN_COMMAND
    qemu-system-arm -M xilinx-zynq-a9 -semihosting -nographic
    -serial /dev/null -serial mon:stdio
    -kernel $<TARGET_FILE:asp>
)
