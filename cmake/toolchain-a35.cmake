#
#		ツールチェーンファイル（AArch64ベアメタル：Cortex-A35用）
#
#  既定は aarch64-none-elf．別のプレフィックスを使う場合は
#  -DA35_TOOLCHAIN_PREFIX=aarch64-linux-gnu- 等で上書きできる
#  （開発機にベアメタル用ツールチェーンが無い場合のコンパイル確認用．
#    その場合glibcの制約で最終リンクはできないことがある）．
#
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(NOT DEFINED A35_TOOLCHAIN_PREFIX)
    set(A35_TOOLCHAIN_PREFIX aarch64-none-elf-)
endif()

set(CMAKE_C_COMPILER ${A35_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${A35_TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${A35_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${A35_TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_NM ${A35_TOOLCHAIN_PREFIX}nm)

#  try_compileはリンク不要のスタティックライブラリで行う
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#  実行ファイルの拡張子
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
