#
#		ターゲット依存部のCMake定義（PolarFire SoC Kit /
#		QEMU microchip-icicle-kit用）
#
#  変数を積み上げ，チップ依存部の chip.cmake をincludeする．
#

set(TARGETDIR ${ASP3_ROOT_DIR}/target/polarfire_soc_kit_gcc)

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
#  QEMU／実機の切り替え（既定：QEMU）
#
#  OFF（実機Icicle Kit）にすると TOPPERS_USE_QEMU を定義しない：
#    - target_exit のセミホスティング終了を行わない
#  実機向けのロード手段（HSS/SoftConsole資材）は未整備（実機対応時に
#  整備する）．
#
option(POLARFIRE_QEMU "Build for QEMU microchip-icicle-kit (OFF: real Icicle Kit)" ON)

#
#  コンパイル定義
#
#  QEMUが-kernelでELFを直接ロードするため，.dataのROM化コピーは不要
#  （リンカスクリプトはVMA==LMA）．
#
list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_OMIT_DATA_INIT
)

if(POLARFIRE_QEMU)
    list(APPEND ASP3_COMPILE_DEFS TOPPERS_USE_QEMU)
endif()

#
#  リンクオプション
#
list(APPEND ASP3_LINK_OPTIONS
    -Wl,--build-id=none
)

set(ASP3_LDSCRIPT ${TARGETDIR}/polarfire_soc_kit.ld)

#
#  ターゲット依存部のソース
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
)

#
#  チップ依存部のインクルード（非TECS版SIOドライバはchip側で追加）
#
include(${ASP3_ROOT_DIR}/arch/riscv_gcc/polarfire_soc/chip.cmake)

#
#  QEMUによる実行（cmake --build <dir> --target run．QEMUビルド時のみ）
#
#  qemu-system-riscv64 がPATHにない場合は
#  -DQEMU_SYSTEM_RISCV64=/path/to/qemu-system-riscv64 で指定する．
#
if(POLARFIRE_QEMU)
    set(QEMU_SYSTEM_RISCV64 qemu-system-riscv64
        CACHE STRING "Path to qemu-system-riscv64")
    set(ASP3_RUN_COMMAND
        ${QEMU_SYSTEM_RISCV64} -machine microchip-icicle-kit -nographic
        -semihosting-config enable=on,target=native
        -bios none -kernel $<TARGET_FILE:asp>
    )

    #  osdebug用のgdb接続手順の差し替え（root CMakeLists.txtが参照）．
    #
    #  icicle-kitはマルチクラスタ構成（E51クラスタ＋U54クラスタ）で，QEMUの
    #  gdbstubはクラスタ毎に別プロセスとして公開する．既定の
    #  `gdb asp.elf -ex "target remote :1234"` ではE51（FPUなし）のtarget
    #  descriptionとRV64GCのELFが衝突し "bfd requires flen 8, but target has
    #  flen 0" で接続に失敗するため，ELFなしで接続→U54クラスタ（inferior 2）に
    #  attach→fileの順とする（ASP3はhart1=U54で動作する）．
    set(ASP3_OSDEBUG_GDB_CONNECT
        "-ex 'target extended-remote :1234' -ex 'add-inferior' -ex 'inferior 2' -ex 'attach 2' -ex 'file $<TARGET_FILE:asp>'")
endif()
