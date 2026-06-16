#
#		アーキテクチャ依存部のCMake定義（ARM-Mコア共通）
#
#  target.cmake からincludeされる．CPU固有のフラグ（-mcpu等）と
#  コア種別の定義（TOPPERS_CORTEX_M33等）はtarget.cmake側で
#  ASP3_COMPILE_OPTIONS／ASP3_COMPILE_DEFS に積む．
#

set(ARCHDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc)

list(APPEND ASP3_SYMVAL_TABLES
    ${ARCHDIR}/common/core_sym.def
)

list(APPEND ASP3_OFFSET_TRB_FILES
    ${ARCHDIR}/common/core_offset.py
)

list(APPEND ASP3_INCLUDE_DIRS
    ${ARCHDIR}/common
    ${ASP3_ROOT_DIR}/arch/gcc
)

list(APPEND ASP3_ARCH_C_FILES
    ${ARCHDIR}/common/core_kernel_impl.c
    ${ARCHDIR}/common/core_support.S
    ${ARCHDIR}/common/start.S
)

#
#  【SAFEG】SafeG-M（デュアルOSモニタ）有効化オプション（既定 OFF＝素 ASP3）
#  ENABLE_SAFEG_M=1 で -DTOPPERS_SAFEG_M を付与し，TrustZone を強制する(M0 §11.2)．
#  既定 OFF のため素 ASP3 ビルドは不変．
#
option(ENABLE_SAFEG_M "Enable SafeG-M dual-OS monitor (implies TrustZone)" OFF)
if(ENABLE_SAFEG_M)
    list(APPEND ASP3_COMPILE_DEFS TOPPERS_SAFEG_M TOPPERS_ENABLE_TRUSTZONE)
    # 【SAFEG】Secure gate / cmse_nonsecure_call(launch_ns) のため -mcmse 必須．
    #  CMSE import lib(secure_nsclib.o) の生成は NS ビルド連携(M3)で最終実行ファイルに
    #  対してのみ付与する(cfg1_out 等 cmse entry を持たない補助ELFで失敗するため)．
    list(APPEND ASP3_COMPILE_OPTIONS -mcmse)
    list(APPEND ASP3_LINK_OPTIONS -mcmse)
    message(STATUS "[SAFEG] SafeG-M enabled: -DTOPPERS_SAFEG_M -mcmse (+TrustZone forced)")
endif()
