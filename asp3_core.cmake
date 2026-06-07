#
#		TOPPERS/ASP3 Core CMakeエントリ
#
#  アプリケーション（または本リポジトリのルートCMakeLists.txt）から
#  includeして使用する．
#
#  - ASP3_ROOT_DIR：ASP3カーネルソースのルート（本ファイルの場所）
#  - ASP3_TARGET：ターゲット名（target/ 配下のディレクトリ名．
#    プリセット/コマンドラインの -DASP3_TARGET=... で指定）
#

set(ASP3_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

#
#  ターゲットの選択と存在確認
#
if(NOT DEFINED ASP3_TARGET)
    message(FATAL_ERROR
        "ASP3_TARGET is not defined. "
        "Use a preset (e.g. --preset mps2_an521-qemu) or -DASP3_TARGET=<target>.")
endif()

if(NOT EXISTS ${ASP3_ROOT_DIR}/target/${ASP3_TARGET}/target.cmake)
    message(FATAL_ERROR
        "target/${ASP3_TARGET}/target.cmake not found. "
        "ASP3_TARGET='${ASP3_TARGET}' is not supported by the CMake build.")
endif()

#
#  非TECS版システムサービスのソース一覧をTARGETに追加するヘルパ関数
#  （ターゲット依存のSIOドライバは ASP3_SYSSVC_TARGET_C_FILES で
#    target.cmake が供給する）
#
function(asp3_add_syssvc TARGET)
    target_sources(${TARGET} PRIVATE
        ${ASP3_ROOT_DIR}/syssvc/syslog.c
        ${ASP3_ROOT_DIR}/syssvc/banner.c
        ${ASP3_ROOT_DIR}/syssvc/serial.c
        ${ASP3_ROOT_DIR}/syssvc/serial_cfg.c
        ${ASP3_ROOT_DIR}/syssvc/logtask.c
        ${ASP3_SYSSVC_TARGET_C_FILES}
    )
    target_sources(${TARGET} PRIVATE
        ${ASP3_ROOT_DIR}/library/log_output.c
        ${ASP3_ROOT_DIR}/library/vasyslog.c
        ${ASP3_ROOT_DIR}/library/t_perror.c
        ${ASP3_ROOT_DIR}/library/strerror.c
    )
endfunction()
