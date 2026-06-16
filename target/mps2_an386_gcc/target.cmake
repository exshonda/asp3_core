#
#		ターゲット依存部のCMake定義（ARM MPS2-AN386 / QEMU用）
#
#  変数を積み上げ，アーキ依存部の arch.cmake をincludeする
#  （AGENTS.md §11．Makefile.targetのCMake版に相当）．
#
#  AN386 は ARMv7-M（シングル Cortex-M4・非 TrustZone）の FPGA イメージで，
#  QEMU では hw/arm/mps2.c（レガシー CMSDK 系．AN505/AN521 の IoTKit/SSE を
#  用いる hw/arm/mps2-tz.c とは別系統）でモデル化される．本ターゲットは
#  arch/arm_m_gcc/common の __TARGET_ARCH_THUMB == 4（ARMv7-M）経路を検証する．
#
#  Cortex-M4 は FPv4-SP（単精度）FPU を実装する（QEMU cpu-v7m.c の
#  cortex_m4_initfn：mvfr0=0x10110021／mvfr1=0x11000011）．これを有効化し，
#  ディスパッチ時の S16-S31 退避・遅延スタッキングを AN505（FPv5-SP）と同型で
#  行う．TrustZone は持たないため TOPPERS_ENABLE_TRUSTZONE は定義しない
#  （PSPLIM 等の __TARGET_ARCH_THUMB >= 5 経路は arch 側で自動無効化される）．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/mps2_an386_gcc)

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
#  コンパイル定義（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_USE_QEMU
    USE_TIM_AS_HRT
    TOPPERS_CORTEX_M4
    __TARGET_ARCH_THUMB=4
    TOPPERS_FPU_ENABLE       # CPACR で FPU(CP10/CP11)を有効化（core_kernel_impl.c）
    TOPPERS_FPU_CONTEXT      # ディスパッチ時に S16-S31 を退避（core_support.S）
    TOPPERS_FPU_LAZYSTACKING # FPCCR=ASPEN|LSPEN（遅延スタッキング．AN505 等と同一）
)

#
#  コンパイル・リンクオプション（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-m4
    -mthumb
    -mlittle-endian
    -mfloat-abi=softfp
    -mfpu=fpv4-sp-d16
    -ffunction-sections
    -fdata-sections
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
    -nostartfiles
    -mcpu=cortex-m4
    -mthumb
    -mlittle-endian
    -mfloat-abi=softfp
    -mfpu=fpv4-sp-d16
    -Wl,--print-memory-usage
    -Wl,--gc-sections
)

list(APPEND ASP3_LINK_LIBS c gcc)

set(ASP3_LDSCRIPT ${TARGETDIR}/mps2_an386.ld)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
)

#
#  非TECS版SIOドライバ
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${TARGETDIR}/target_serial.c
    ${TARGETDIR}/cmsdk_uart.c
)

#
#  アーキ依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_m_gcc/common/arch.cmake)

#
#  QEMUによる実行（cmake --build <dir> --target run）
#
set(ASP3_RUN_COMMAND
    qemu-system-arm -machine mps2-an386 -nographic
    -semihosting-config enable=on,target=native
    -kernel $<TARGET_FILE:asp>
)
