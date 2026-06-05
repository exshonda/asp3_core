#
#		TOPPERS/ASP3 Core CMakeビルド
#
#  asp3_fsp / asp3_pico_sdk のCMake実装をベースに，SDK非依存の
#  スタンドアロンビルド（サンプルアプリ asp）として構成したもの．
#
#  使い方（例）：
#    cmake --preset m33-qemu -B build/m33-qemu
#    cmake --build build/m33-qemu
#    cmake --build build/m33-qemu --target run    # QEMU実行
#
cmake_minimum_required(VERSION 3.16)

project(asp3_core C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/asp3_core.cmake)

#
#  Pythonインタプリタ（cfg実行用）
#
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(PYTHON_EXE ${Python3_EXECUTABLE} -B)

#
#  cfg（コンフィギュレータ）関連の定義
#
set(CFG_PY ${ASP3_ROOT_DIR}/cfg/cfg.py)

list(APPEND ASP3_SYMVAL_TABLES
    ${ASP3_ROOT_DIR}/kernel/kernel_sym.def
)

list(APPEND ASP3_API_TABLES
    ${ASP3_ROOT_DIR}/kernel/kernel_api.def
)

list(APPEND ASP3_INCLUDE_DIRS
    ${ASP3_ROOT_DIR}/include
)

list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_OMIT_TECS
)

#
#  共通コンパイルオプション（sample/MakefileのCOPTS既定と同一）
#
list(APPEND ASP3_COMPILE_OPTIONS
    -O2 -Wall -g
)

#
#  生成されるCコードの出力先
#
set(CFG_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(CFG1_OUT_FILE ${CFG_GEN_DIR}/cfg1_out.c)
set(CFG1_OUT cfg1_out)
set(OFFSET_H_FILE ${CFG_GEN_DIR}/offset.h)
set(CFG1_OUT_SYMS_FILE ${CFG_GEN_DIR}/cfg1_out.syms)
set(CFG1_OUT_SREC_FILE ${CFG_GEN_DIR}/cfg1_out.srec)
set(CFG1_OUT_DB_FILE ${CFG_GEN_DIR}/cfg1_out.db)
set(CFG2_OUT_DB_FILE ${CFG_GEN_DIR}/cfg2_out.db)
file(MAKE_DIRECTORY ${CFG_GEN_DIR})

#
#  アプリケーション（サンプル）の定義
#
if(NOT DEFINED ASP3_APPLNAME)
    set(ASP3_APPLNAME sample1)
endif()
if(NOT DEFINED ASP3_APPLDIR)
    set(ASP3_APPLDIR ${ASP3_ROOT_DIR}/sample)
endif()
set(ASP3_APP_CFG_FILE ${ASP3_APPLDIR}/${ASP3_APPLNAME}.cfg)
list(APPEND ASP3_APP_INCLUDE_DIRS ${ASP3_APPLDIR})

#
#  ターゲット依存部の情報を読み込み
#
#  target.cmake は以下の変数を積み上げ，アーキ依存部の arch.cmake を
#  includeする（AGENTS.md §11）：
#    ASP3_CFG_FILES / ASP3_KERNEL_CFG_TRB_FILES / ASP3_CHECK_TRB_FILES /
#    ASP3_OFFSET_TRB_FILES / ASP3_SYMVAL_TABLES / ASP3_INCLUDE_DIRS /
#    ASP3_COMPILE_DEFS / ASP3_COMPILE_OPTIONS / ASP3_LINK_OPTIONS /
#    ASP3_LINK_LIBS / ASP3_LDSCRIPT / ASP3_ARCH_C_FILES /
#    ASP3_TARGET_C_FILES / ASP3_SYSSVC_TARGET_C_FILES / ASP3_RUN_COMMAND
#
include(${ASP3_ROOT_DIR}/target/${ASP3_TARGET}/target.cmake)

list(APPEND ASP3_CFG_FILES
    ${ASP3_APP_CFG_FILE}
)

list(APPEND ASP3_INCLUDE_DIRS
    ${ASP3_ROOT_DIR}
    ${CFG_GEN_DIR}
)

#
#  リンカスクリプトの指定
#
if(DEFINED ASP3_LDSCRIPT)
    list(APPEND ASP3_LINK_OPTIONS -Wl,-T,${ASP3_LDSCRIPT})
endif()

#
#  cfgコマンドライン引数の組み立て
#
foreach(path IN LISTS ASP3_SYMVAL_TABLES)
    list(APPEND CFG_SYMVAL_TABLES "--symval-table" ${path})
endforeach()

foreach(path IN LISTS ASP3_API_TABLES)
    list(APPEND CFG_API_TABLES "--api-table" "${path}")
endforeach()

foreach(path IN LISTS ASP3_OFFSET_TRB_FILES)
    list(APPEND CFG_OFFSET_TRB_FILES "-T" ${path})
endforeach()

foreach(path IN LISTS ASP3_KERNEL_CFG_TRB_FILES)
    list(APPEND CFG_KERNEL_CFG_TRB_FILES "-T" "${path}")
endforeach()

foreach(path IN LISTS ASP3_CHECK_TRB_FILES)
    list(APPEND CFG_CHECK_TRB_FILES "-T" ${path})
endforeach()

foreach(path IN LISTS ASP3_INCLUDE_DIRS ASP3_APP_INCLUDE_DIRS)
    list(APPEND CFG_INCLUDE_DIRS "-I${path}")
endforeach()

set(CFG_GEN_H_FILES ${CFG_GEN_DIR}/kernel_cfg.h)
set(CFG_GEN_C_FILES ${CFG_GEN_DIR}/kernel_cfg.c)

#
#  カーネルのコンフィギュレーションファイルの生成
#
#  パス1：.cfg＋api-table → cfg1_out.c
#
add_custom_command(
    OUTPUT ${CFG1_OUT_FILE} ${CFG1_OUT_DB_FILE}
    WORKING_DIRECTORY ${CFG_GEN_DIR}
    COMMAND ${PYTHON_EXE} ${CFG_PY} --pass 1 --kernel asp
            ${CFG_INCLUDE_DIRS} ${CFG_API_TABLES} ${CFG_SYMVAL_TABLES}
            -M ${CFG_GEN_DIR}/cfg1_out_c.d ${ASP3_CFG_FILES}
    DEPENDS ${CFG_PY} ${ASP3_SYMVAL_TABLES} ${ASP3_API_TABLES} ${ASP3_CFG_FILES}
    COMMENT "Running cfg pass 1 to generate cfg1_out.c"
)

#
#  cfg1_outのビルド（静的APIパラメータの値の取り出し用）
#
#  ASP3_START_FILES（start.S等）はライブラリ外で先頭にリンクする
#  （Makefile版のSTART_OBJSに相当．arm_gcc系で使用）
add_executable(${CFG1_OUT} ${ASP3_START_FILES} ${CFG1_OUT_FILE})
target_include_directories(${CFG1_OUT}
    PRIVATE ${ASP3_INCLUDE_DIRS} ${ASP3_APP_INCLUDE_DIRS}
    PRIVATE ${ASP3_ROOT_DIR}/kernel    # ASP3_START_FILES（start.S）用
)
target_compile_definitions(${CFG1_OUT} PRIVATE ${ASP3_COMPILE_DEFS})
target_compile_options(${CFG1_OUT} PRIVATE ${ASP3_COMPILE_OPTIONS})
#  cfg1_outは全シンボルの値をシンボルテーブルから取り出すため，
#  セクションガベージコレクションは行わない（Makefileビルドと同じ）
set(CFG1_LINK_OPTIONS ${ASP3_LINK_OPTIONS})
list(REMOVE_ITEM CFG1_LINK_OPTIONS "-Wl,--gc-sections")
target_link_options(${CFG1_OUT} PRIVATE ${CFG1_LINK_OPTIONS})
target_link_libraries(${CFG1_OUT} PRIVATE ${ASP3_LINK_LIBS})

add_custom_command(
    OUTPUT ${CFG1_OUT_SYMS_FILE}
    COMMAND ${CMAKE_NM} -n $<TARGET_FILE:${CFG1_OUT}> > ${CFG1_OUT_SYMS_FILE}
    DEPENDS ${CFG1_OUT}
    COMMENT "Generating cfg1_out.syms"
)

add_custom_command(
    OUTPUT ${CFG1_OUT_SREC_FILE}
    COMMAND ${CMAKE_OBJCOPY} -O srec -S $<TARGET_FILE:${CFG1_OUT}> ${CFG1_OUT_SREC_FILE}
    DEPENDS ${CFG1_OUT}
    COMMENT "Generating cfg1_out.srec"
)

#
#  パス2（-O）：offset.h の生成
#  （offset.hを使わないターゲット（POSIX等）ではスキップ）
#
if(ASP3_OFFSET_TRB_FILES)
    add_custom_command(
        OUTPUT ${OFFSET_H_FILE}
        WORKING_DIRECTORY ${CFG_GEN_DIR}
        COMMAND ${PYTHON_EXE} ${CFG_PY} --pass 2 -O --kernel asp
                ${CFG_INCLUDE_DIRS} ${CFG_OFFSET_TRB_FILES}
                --rom-symbol ${CFG1_OUT_SYMS_FILE} --rom-image ${CFG1_OUT_SREC_FILE}
        DEPENDS ${CFG_PY} ${ASP3_OFFSET_TRB_FILES}
                ${CFG1_OUT_SYMS_FILE} ${CFG1_OUT_SREC_FILE} ${CFG1_OUT_DB_FILE}
        COMMENT "Running cfg pass 2 to generate offset.h"
    )
else()
    set(OFFSET_H_FILE "")
endif()

#
#  パス2：kernel_cfg.c/h の生成
#
add_custom_command(
    OUTPUT ${CFG_GEN_H_FILES} ${CFG_GEN_C_FILES} ${CFG2_OUT_DB_FILE}
    WORKING_DIRECTORY ${CFG_GEN_DIR}
    COMMAND ${PYTHON_EXE} ${CFG_PY} --pass 2 --kernel asp
            ${CFG_INCLUDE_DIRS} ${CFG_KERNEL_CFG_TRB_FILES}
            --rom-symbol ${CFG1_OUT_SYMS_FILE} --rom-image ${CFG1_OUT_SREC_FILE}
    DEPENDS ${CFG_PY} ${ASP3_KERNEL_CFG_TRB_FILES}
            ${CFG1_OUT_SYMS_FILE} ${CFG1_OUT_SREC_FILE} ${CFG1_OUT_DB_FILE}
    COMMENT "Running cfg pass 2 to generate kernel_cfg.c/h"
)

add_custom_target(generate_cfg_gen_files
    DEPENDS ${OFFSET_H_FILE} ${CFG_GEN_H_FILES} ${CFG_GEN_C_FILES}
)

#
#  カーネルライブラリ（libasp3.a）
#
add_library(asp3 STATIC
    ${ASP3_ARCH_C_FILES}
    ${ASP3_TARGET_C_FILES}
    ${ASP3_ROOT_DIR}/kernel/alarm.c
    ${ASP3_ROOT_DIR}/kernel/cyclic.c
    ${ASP3_ROOT_DIR}/kernel/dataqueue.c
    ${ASP3_ROOT_DIR}/kernel/eventflag.c
    ${ASP3_ROOT_DIR}/kernel/exception.c
    ${ASP3_ROOT_DIR}/kernel/interrupt.c
    ${ASP3_ROOT_DIR}/kernel/mempfix.c
    ${ASP3_ROOT_DIR}/kernel/mutex.c
    ${ASP3_ROOT_DIR}/kernel/pridataq.c
    ${ASP3_ROOT_DIR}/kernel/semaphore.c
    ${ASP3_ROOT_DIR}/kernel/startup.c
    ${ASP3_ROOT_DIR}/kernel/sys_manage.c
    ${ASP3_ROOT_DIR}/kernel/task.c
    ${ASP3_ROOT_DIR}/kernel/task_manage.c
    ${ASP3_ROOT_DIR}/kernel/task_refer.c
    ${ASP3_ROOT_DIR}/kernel/task_sync.c
    ${ASP3_ROOT_DIR}/kernel/task_term.c
    ${ASP3_ROOT_DIR}/kernel/taskhook.c
    ${ASP3_ROOT_DIR}/kernel/time_event.c
    ${ASP3_ROOT_DIR}/kernel/time_manage.c
    ${ASP3_ROOT_DIR}/kernel/wait.c
    ${CFG_GEN_C_FILES}
)
add_dependencies(asp3 generate_cfg_gen_files)

target_include_directories(asp3
    PUBLIC ${ASP3_INCLUDE_DIRS} ${ASP3_APP_INCLUDE_DIRS}
    PRIVATE ${ASP3_ROOT_DIR}/kernel
)
target_compile_definitions(asp3
    PRIVATE ALLFUNC
    PUBLIC ${ASP3_COMPILE_DEFS}
)
target_compile_options(asp3 PUBLIC ${ASP3_COMPILE_OPTIONS})

#
#  パス3：構成チェック（POST_BUILD）
#
list(APPEND PASS3_ARGS ${PYTHON_EXE} ${CFG_PY} --pass 3 --kernel asp -O)
list(APPEND PASS3_ARGS ${CFG_INCLUDE_DIRS} ${CFG_CHECK_TRB_FILES})

function(asp3_cfg_check TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        WORKING_DIRECTORY ${CFG_GEN_DIR}
        COMMAND ${CMAKE_NM} -n $<TARGET_FILE:${TARGET}> > ${TARGET}.syms
        COMMAND ${CMAKE_OBJCOPY} -O srec -S $<TARGET_FILE:${TARGET}> ${TARGET}.srec
        COMMAND ${PASS3_ARGS} --rom-symbol ${TARGET}.syms --rom-image ${TARGET}.srec
        COMMENT "Running cfg pass 3 to check configuration"
    )
endfunction()

#
#  サンプルアプリケーション（asp）
#
add_executable(asp
    ${ASP3_START_FILES}
    ${ASP3_APPLDIR}/${ASP3_APPLNAME}.c
)
asp3_add_syssvc(asp)
target_include_directories(asp
    PRIVATE ${ASP3_ROOT_DIR}/kernel    # ASP3_START_FILES（start.S）用
)
target_link_libraries(asp PRIVATE asp3 ${ASP3_LINK_LIBS})
target_link_options(asp PRIVATE ${ASP3_LINK_OPTIONS})
asp3_cfg_check(asp)

#
#  実行ターゲット（QEMU等．target.cmakeがASP3_RUN_COMMANDを定義した場合）
#
if(DEFINED ASP3_RUN_COMMAND)
    add_custom_target(run
        COMMAND ${ASP3_RUN_COMMAND}
        DEPENDS asp
        USES_TERMINAL
        COMMENT "Running ${ASP3_TARGET}"
    )
endif()
