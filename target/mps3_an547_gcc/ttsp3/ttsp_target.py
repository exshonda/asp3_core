#
#  mps3_an547（Cortex-M55）TTSP3 ターゲット定義
#
#  test/ttsp/run_ttsp.py が自動探索する（パスは asp3_core ルート相対）。
#  TTSP3 は本ターゲットの ASP ライブラリを同梱しないため、asp3 側テスト資産
#  （ttsp_target_test.{c,h}・ttsp_target.cfg）を本フォルダに置く（lib＝本フォルダ）。
#
TTSP_TARGETS = {
    "mps3_an547": {
        "preset": "mps3_an547-qemu",
        "ttsp_target": None,
        "lib": "target/mps3_an547_gcc/ttsp3",
        "qemu": ("qemu-system-arm -M mps3-an547 -cpu cortex-m55 -kernel {elf} "
                 "-semihosting -semihosting-config enable=on,target=native "
                 "-nographic"),
        "unsupported_hw": {"ttsp_target_gain_tick", "ttsp_int_raise",
                           "ttsp_cpuexc_raise"},
        "soft_hw": {"ttsp_target_stop_tick", "ttsp_target_start_tick"},
        "skip_modules": {"interrupt", "exception"},
    },
}
