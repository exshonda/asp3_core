#
#		ターゲット依存部のCMake定義（ARM MPS2-AN505 / QEMU用）
#
#  変数を積み上げ，アーキ依存部の arch.cmake をincludeする
#  （AGENTS.md §11．Makefile.targetのCMake版に相当）．
#
#  AN505 は IoTKit（シングル Cortex-M33）．CPU0 が FPU を実装するため
#  （QEMU armsse.c の iotkit_properties：CPU0_FPU=true），ハードウェア FPU
#  （FPv5-SP・単精度）を有効化する．AN521=SSE-200 では CPU0 に FPU が無く
#  ソフト浮動小数点だったが，AN505 への置換でこの制約が解消した．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/mps2_an505_gcc)

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
    TOPPERS_CORTEX_M33
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
    -mcpu=cortex-m33
    -mthumb
    -mlittle-endian
    -mfloat-abi=softfp
    -mfpu=fpv5-sp-d16
    -ffunction-sections
    -fdata-sections
)

list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
    -nostartfiles
    -mcpu=cortex-m33
    -mthumb
    -mlittle-endian
    -mfloat-abi=softfp
    -mfpu=fpv5-sp-d16
    -Wl,--print-memory-usage
    -Wl,--gc-sections
)

list(APPEND ASP3_LINK_LIBS c gcc)

# 【SAFEG】SafeG-M(ENABLE_SAFEG_M=1)時は Secure/NS 分割＋NSC veneer(.gnu.sgstubs)を
#  含む専用リンカスクリプトを使う。素のASP3(SAFEG=0)は従来の mps2_an505.ld のまま。
if(ENABLE_SAFEG_M)
    set(ASP3_LDSCRIPT ${TARGETDIR}/mps2_an505_safeg.ld)
else()
    set(ASP3_LDSCRIPT ${TARGETDIR}/mps2_an505.ld)
endif()

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
    qemu-system-arm -machine mps2-an505 -nographic
    -semihosting-config enable=on,target=native
    -kernel $<TARGET_FILE:asp>
)
