#
#		ツールチェーンファイル（arm-none-eabi-gcc）
#
#  Cortex-M系ターゲット用．-mcpu等のCPU固有フラグはarch.cmake／
#  target.cmakeが ASP3_COMPILE_OPTIONS で供給する．
#
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_NM arm-none-eabi-nm)

#  try_compileはリンク不要のスタティックライブラリで行う
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#  実行ファイルの拡張子（QEMU起動コマンド等が asp.elf を参照する）
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
