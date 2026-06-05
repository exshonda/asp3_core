#
#		アーキテクチャ依存部のCMake定義（POSIXシミュレーション用）
#
#  target.cmake からincludeされる．
#

set(POSIXDIR ${ASP3_ROOT_DIR}/arch/posix_gcc)

list(APPEND ASP3_INCLUDE_DIRS
    ${POSIXDIR}
    ${ASP3_ROOT_DIR}/arch/gcc
)

list(APPEND ASP3_ARCH_C_FILES
    ${POSIXDIR}/posix_kernel_impl.c
    ${POSIXDIR}/interrupt_sim.c
    ${POSIXDIR}/thread_ctrl.c
    ${POSIXDIR}/posix_timer_itimer.c
)
