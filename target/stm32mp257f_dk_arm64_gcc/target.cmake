#
#		ターゲット依存部のCMake定義（STM32MP257F-DK用）
#
#  変数を積み上げ，チップ依存部の chip.cmake をincludeする
#  （Makefile.targetのCMake版に相当）．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/stm32mp257f_dk_arm64_gcc)

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
#  メモリアドレスの設定（Makefile.targetと同一）
#
set(TEXT_START_ADDRESS 0x0088001000)
set(DATA_START_ADDRESS 0x0090000000)

#
#  コンパイル定義（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_DEFS
    USE_ARM64_FPU
    TOPPERS_TZ_S
    TOPPERS_MEM_ADDR=${TEXT_START_ADDRESS}
    TOPPERS_MEM_SIZE=0x0040000000
    TOPPERS_32BIT_ABOVE_ADDR
)

#
#  コンパイル・リンクオプション（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mlittle-endian
)

list(APPEND ASP3_LINK_OPTIONS
    -mlittle-endian
    -Wl,--build-id=none
    -Wl,-Ttext,${TEXT_START_ADDRESS}
    -Wl,-Tdata,${DATA_START_ADDRESS}
)

set(ASP3_LDSCRIPT ${TARGETDIR}/stm32mp257f_dk.ld)

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
    ${ASP3_ROOT_DIR}/arch/arm64_gcc/stm32mp2/stm32usart.c
)

#
#  hardware_init_hook（エントリ時キャッシュ/MMU正規化）
#
#  start.S の weak 定義を上書きするため，ライブラリ（libasp3.a）では
#  なく start.S と同じく直接リンクする（アーカイブ経由では weak 定義
#  が既に解決済みのため引き込まれない）．
#
list(APPEND ASP3_START_FILES
    ${TARGETDIR}/target_start_hook.S
)

#
#  チップ依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm64_gcc/stm32mp2/chip.cmake)

#
#  実機実行・デバッグターゲット（swd-run/osdebug等．aspターゲット定義後に取込み）
#
set(ASP3_TARGET_RUN_CMAKE ${TARGETDIR}/run.cmake)
