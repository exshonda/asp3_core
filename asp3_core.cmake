#
#		TOPPERS/ASP3 Core CMakeエントリ
#
#  アプリケーション（または本リポジトリのルートCMakeLists.txt）から
#  includeして使用する．
#
#  - ASP3_ROOT_DIR：ASP3カーネルソースのルート（本ファイルの場所）
#  - ASP3_TARGET：ターゲット名（target/ 配下のディレクトリ名．
#    プリセット/コマンドラインの -DASP3_TARGET=... で指定）
#  - ASP3_TARGET_DIR：ターゲット依存部（target.cmake）のディレクトリ．
#    未指定なら ${ASP3_ROOT_DIR}/target/${ASP3_TARGET}（従来動作）．
#    外部リポジトリ（各社SDK）が target/ を本リポジトリ外に置く場合，
#    -DASP3_TARGET_DIR=<外部の絶対パス> で供給する（後方互換拡張）．
#    参照規約は docs/porting/PORTING_GUIDE.md「外部（SDK）ターゲットの置き方」．
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

#
#  ターゲット依存部のディレクトリ（ASP3_TARGET_DIR）
#
#  未指定時は本リポジトリ内の target/${ASP3_TARGET} を既定とする
#  （既存ターゲット＝CMakePresets.json は無変更で従来どおり動作）．
#
if(NOT DEFINED ASP3_TARGET_DIR)
    set(ASP3_TARGET_DIR ${ASP3_ROOT_DIR}/target/${ASP3_TARGET})
endif()

if(NOT EXISTS ${ASP3_TARGET_DIR}/target.cmake)
    message(FATAL_ERROR
        "${ASP3_TARGET_DIR}/target.cmake not found. "
        "ASP3_TARGET='${ASP3_TARGET}' is not supported by the CMake build "
        "(set -DASP3_TARGET_DIR=<dir> for an external/SDK target).")
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
