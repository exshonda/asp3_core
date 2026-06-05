#
#		ターゲット依存部のCMake定義（Linux用ホストシミュレーション）
#
#  Makefile.targetのCMake版に相当．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/linux_gcc)

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

list(APPEND ASP3_SYMVAL_TABLES
    ${TARGETDIR}/target_sym.def
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
list(APPEND ASP3_COMPILE_OPTIONS
    -pthread
    -Werror
)

list(APPEND ASP3_LINK_OPTIONS
    -pthread
)

list(APPEND ASP3_LINK_LIBS c)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
)

#
#  非TECS版SIOドライバ（POSIX依存部）
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${ASP3_ROOT_DIR}/arch/posix_gcc/posix_serial.c
)

#
#  アーキ依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/posix_gcc/arch.cmake)

#
#  実行（cmake --build <dir> --target run）
#
set(ASP3_RUN_COMMAND $<TARGET_FILE:asp>)
