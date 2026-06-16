#
#		ターゲット依存部のCMake定義（ARM MPS3-AN547 / QEMU用）
#
#  変数を積み上げ，アーキ依存部の arch.cmake をincludeする
#  （AGENTS.md §11．Makefile.targetのCMake版に相当）．
#
#  AN547 は SSE-300（シングル Cortex-M55．ARMv8.1-M）．CPU0 が FPU を実装
#  するため（QEMU armsse.c の sse300_properties：CPU0_FPU=true），ハード
#  ウェア FPU を有効化する．Cortex-M55 の FPU は FPv5・倍精度対応（QEMU
#  cpu-v7m.c：MVFR0=0x10110221 ＝ 単精度/倍精度とも実装）のため
#  -mfpu=fpv5-d16 を用いる．
#
#  本ファイルは mps2_an505_gcc（IoTKit/Cortex-M33）から派生．Step 2 で MVE
#  (Helium) を有効化（-mcpu=cortex-m55 ＝ +mve.fp 既定）．__ARM_FEATURE_MVE が
#  定義され，core_support.S の VPR 退避コードが有効になる．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/mps3_an547_gcc)

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
    TOPPERS_CORTEX_M55
    TOPPERS_ENABLE_TRUSTZONE
    __TARGET_ARCH_THUMB=5
    TOPPERS_FPU_ENABLE       # CPACR で FPU(CP10/CP11)を有効化（core_kernel_impl.c）
    TOPPERS_FPU_CONTEXT      # ディスパッチ時に S16-S31 を退避（core_support.S）
    TOPPERS_FPU_LAZYSTACKING # FPCCR=ASPEN|LSPEN（遅延スタッキング．pico2_arm 等と同一）
)

#
#  コンパイル・リンクオプション（Makefile.targetと同一）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-m55         # Step 2：MVE(Helium) 有効（既定で +mve.fp ＝ __ARM_FEATURE_MVE）
    -mthumb
    -mlittle-endian
    -mfloat-abi=softfp
    -mfpu=fpv5-d16           # Cortex-M55 FPU は FPv5・倍精度（D16）
    -ffunction-sections
    -fdata-sections
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
    -nostartfiles
    -mcpu=cortex-m55
    -mthumb
    -mlittle-endian
    -mfloat-abi=softfp
    -mfpu=fpv5-d16
    -Wl,--print-memory-usage
    -Wl,--gc-sections
)

list(APPEND ASP3_LINK_LIBS c gcc)

set(ASP3_LDSCRIPT ${TARGETDIR}/mps3_an547.ld)

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
    qemu-system-arm -machine mps3-an547 -nographic
    -semihosting-config enable=on,target=native
    -kernel $<TARGET_FILE:asp>
)
