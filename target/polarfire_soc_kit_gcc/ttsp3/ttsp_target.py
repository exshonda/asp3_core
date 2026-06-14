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
}
