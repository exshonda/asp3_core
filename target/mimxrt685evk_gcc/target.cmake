#
#		ターゲット依存部のCMake定義（NXP EVK-MIMXRT685用）
#
#  変数を積み上げ，チップ依存部の chip.cmake をincludeする
#  （Makefile.targetのCMake版に相当）．MCUXpresso SDKは使用しない
#  （ベアメタル移植．SDK統合は外側リポジトリ asp3_mcuxsdk で行う）．
#
#  TOPPERS_ENABLE_TRUSTZONE は定義しないこと：ブートROMがプレーンイメージ
#  （ベクタテーブル インデックス9＝イメージタイプ bit14，target_kernel.py
#  参照）として起動する構成では，EXC_RETURN は 0xFFFFFFBC（ES/S=0）を
#  用いる（genuine ASP3 3.7.0 の MIMXRT685-EVK 移植＝ENABLE_TRUSTZONE=0
#  の動作実績構成を踏襲）．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/mimxrt685evk_gcc)

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
#  コンパイル定義（Makefile.target/chip/coreと同一．
#  FPU_USAGE=FPU_LAZYSTACKING・KERNEL_TIMER=TIM 相当）
#
list(APPEND ASP3_COMPILE_DEFS
    USE_TIM_AS_HRT
    TOPPERS_CORTEX_M33
    __TARGET_ARCH_THUMB=5
    __TARGET_FPU_FPV4_SP
    TOPPERS_FPU_ENABLE
    TOPPERS_FPU_LAZYSTACKING
    TOPPERS_FPU_CONTEXT
)

#
#  コンパイル・リンクオプション（Makefile.target/chip/coreと同一）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=softfp
    -mfpu=fpv5-sp-d16
    -ffunction-sections
    -fdata-sections
    -nostartfiles
)

#
#  flexspi_config（FlexSPI設定ブロック＝flash_config.c）は参照されない
#  ため，--gc-sections で消えないよう --undefined で残す．
#
list(APPEND ASP3_LINK_OPTIONS
    -nostdlib
    -nostartfiles
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=softfp
    -mfpu=fpv5-sp-d16
    -Wl,--print-memory-usage
    -Wl,--gc-sections
    -Wl,--undefined=flexspi_config
)

list(APPEND ASP3_LINK_LIBS c gcc)

if(ENABLE_SAFEG_M)
    set(ASP3_LDSCRIPT ${TARGETDIR}/mimxrt685_safeg.ld)  # 【SAFEG】Secure/NS分割+NSC veneer
else()
    set(ASP3_LDSCRIPT ${TARGETDIR}/mimxrt685.ld)
endif()

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
    ${TARGETDIR}/flash_config.c
)

#
#  実機実行・デバッグターゲット（jlink-run/osdebug等．aspターゲット定義後に取込み）
#
set(ASP3_TARGET_RUN_CMAKE ${TARGETDIR}/run.cmake)

#
#  非TECS版SIOドライバ（チップ依存部のchip.cmakeが供給）
#

#
#  チップ依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/arm_m_gcc/imxrt600/chip.cmake)
