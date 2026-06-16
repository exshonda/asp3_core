#
#  polarfire_soc_kit（U54 / RV64GC）TTSP3 ターゲット定義
#
#  test/ttsp/run_ttsp.py が自動探索する（パスは asp3_core ルート相対）。
#  TTSP3 は本ターゲットの ASP ライブラリを同梱しないため、asp3 側テスト資産
#  （ttsp_target_test.{c,h}・ttsp_target.cfg）を本フォルダに置く（lib＝本フォルダ）。
#
TTSP_TARGETS = {
    "polarfire_soc_kit": {
        "preset": "polarfire_soc_kit-qemu",
        "ttsp_target": None,
        "lib": "target/polarfire_soc_kit_gcc/ttsp3",
        "qemu": ("qemu-system-riscv64 -machine microchip-icicle-kit -nographic "
                 "-semihosting-config enable=on,target=native -bios none "
                 "-kernel {elf}"),
        "unsupported_hw": {"ttsp_target_gain_tick", "ttsp_int_raise",
                           "ttsp_cpuexc_raise"},
        "soft_hw": {"ttsp_target_stop_tick", "ttsp_target_start_tick"},
        "skip_modules": {"interrupt", "exception"},
    },
    #
    #  実機 Discovery Kit（SoftConsole openocd + gdb，reset init なし）
    #
    #  asp3 は mpfs_hal の system_startup を持たず HSS の MSS IO/クロック初期化に
    #  依存するため，reset init せずにロードする（HSS の設定を保持）．起動時の
    #  CLINT MSIP クリアは chip_initialize で実施済み．詳細は TTSP3_HOWTO.md．
    #
    "polarfire_soc_kit_hw": {
        "preset": "polarfire_soc_kit",            # 実機ビルド（POLARFIRE_QEMU=OFF）
        "cmake_extra": ["-DPOLARFIRE_DISCOVERY=ON"],  # Discovery=MMUART1/L2-LIM
        "ttsp_target": None,
        "lib": "target/polarfire_soc_kit_gcc/ttsp3",
        "qemu": None,
        "hw": {
            "kind": "openocd_gdb_riscv",
            "serial": "/dev/ttyUSB1",   # MMUART1（FlashPro5 FT4232 if1）．$TTSP_HW_SERIAL で上書き
            "baud": 115200,
            "capture": 30,
            "gdb_port": 3333,
            #  SoftConsole 同梱 gdb．$TTSP_RISCV_GDB で上書き
            "gdb": ("/home/honda/Microchip/SoftConsole-v2022.2-RISC-V-747/"
                    "riscv-unknown-elf-gcc/bin/riscv64-unknown-elf-gdb"),
        },
        "unsupported_hw": {"ttsp_target_gain_tick", "ttsp_int_raise",
                           "ttsp_cpuexc_raise"},
        "soft_hw": {"ttsp_target_stop_tick", "ttsp_target_start_tick"},
        "skip_modules": {"interrupt", "exception"},
    },
}
