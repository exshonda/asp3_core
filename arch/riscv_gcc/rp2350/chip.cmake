#
#		チップ依存部のCMake定義（RP2350 RISC-V（Hazard3）用）
#
#  target.cmake からincludeされる（Makefile.chipのCMake版に相当）．
#
#  ARM版RP2350チップ依存部（arch/arm_m_gcc/rp2350）のISA非依存ファイル
#  （RP2350.h・rp2350_uart.[ch]・chip_serial.[ch]＝MMIOとena_int/dis_int
#  のみ使用）をパス参照で共有する（コピーしない）．同名の chip_*.h は
#  本ディレクトリを先にインクルードパスへ置くことでRISC-V版が使われる．
#

set(CHIPDIR ${ASP3_ROOT_DIR}/arch/riscv_gcc/rp2350)
set(RP2350_ARM_CHIPDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc/rp2350)

list(APPEND ASP3_INCLUDE_DIRS
    ${CHIPDIR}
    ${RP2350_ARM_CHIPDIR}
)

#
#  Hazard3×2（RV32IMAC＋Zicsr/Zifencei．FPU無し）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -march=rv32imac_zicsr_zifencei
    -mabi=ilp32
    -mcmodel=medany
    -msmall-data-limit=8
    -mstrict-align
    -mno-save-restore
    -fsigned-char
    -ffunction-sections
)

list(APPEND ASP3_LINK_OPTIONS
    -march=rv32imac_zicsr_zifencei
    -mabi=ilp32
)

list(APPEND ASP3_ARCH_C_FILES
    ${CHIPDIR}/chip_kernel_impl.c
    ${CHIPDIR}/chip_support.S
)

#
#  非TECS版SIOドライバ（ARM版RP2350のドライバを共有．ISA非依存）
#
list(APPEND ASP3_SYSSVC_TARGET_C_FILES
    ${RP2350_ARM_CHIPDIR}/chip_serial.c
    ${RP2350_ARM_CHIPDIR}/rp2350_uart.c
)

#
#  PLIC・Machine Timerは使用しない（割込みコントローラはXh3irq，
#  高分解能タイマはTIMER0（ターゲット依存部）を使用）
#
set(ASP3_RISCV_OMIT_PLIC_MTIMER ON)

#
#  コア依存部のインクルード
#
include(${ASP3_ROOT_DIR}/arch/riscv_gcc/common/arch.cmake)

#
#  ベアメタルリンクのため libc は使用しない（PolarFireと同様）．
#  コンパイラが生成する memcpy/memset 等は libc_stub.c（ISA非依存・
#  PolarFire依存部からパス参照）が提供する．
#
list(REMOVE_ITEM ASP3_LINK_LIBS c)
list(APPEND ASP3_ARCH_C_FILES
    ${ASP3_ROOT_DIR}/arch/riscv_gcc/polarfire_soc/libc_stub.c
)
