#
#		ターゲット依存部のCMake定義（ダミーターゲット用）
#
#  ホストのgccでビルドする疑似ターゲット（cfgテスト用）．
#  Makefile.targetのCMake版に相当．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/dummy_gcc)

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

list(APPEND ASP3_OFFSET_TRB_FILES
    ${TARGETDIR}/target_offset.py
)

list(APPEND ASP3_SYMVAL_TABLES
    ${TARGETDIR}/target_sym.def
)

#
#  インクルードディレクトリ
#
list(APPEND ASP3_INCLUDE_DIRS
    ${TARGETDIR}
    ${ASP3_ROOT_DIR}/arch/gcc
)

#
#  スタートアップ（START_OBJS相当）とリンクオプション
#  （Makefile.targetと同一：start.cがmainを提供し-nostdlibでリンク）
#
list(APPEND ASP3_START_FILES
    ${TARGETDIR}/start.c
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
)

list(APPEND ASP3_LINK_LIBS c gcc)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
)

#
#  非TECS版SIOドライバ（ダミー）
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${TARGETDIR}/target_serial.c
)
