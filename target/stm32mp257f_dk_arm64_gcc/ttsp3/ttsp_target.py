#
#  STM32MP257F-DK（Cortex-A35 / AArch64）TTSP3 ターゲット定義
#
#  test/ttsp/run_ttsp.py が自動探索する（パスは asp3_core ルート相対）。
#  TTSP3 は本ターゲットの ASP ライブラリを同梱しないため、asp3 側テスト資産
#  （ttsp_target_test.{c,h}・ttsp_target.cfg）を本フォルダに置く（lib＝本フォルダ）。
#
#  実機は OpenOCD/SWD で実機ロード＆実行する（既存 run.cmake の `swd-run`
#  ターゲットを再利用＝OpenOCD だけで reset→load→resume→shutdown．ボードは
#  走り続ける）。UART(/dev/ttyACM0) を別途キャプチャし "All check points passed."
#  で判定する（_current_boot で最後のバナー以降に限定）。AArch64＝zcu102_arm64 と
#  同形の SKIP 規則。前提：FSBL + landing pad 入り SD でボードが
#  "Connect using OpenOCD" でハングしていること（target_user.md §4-5）。
#
TTSP_TARGETS = {
    "stm32mp257f_dk_arm64_hw": {
        "preset": "stm32mp257f_dk_arm64",     # 実機ビルド（QEMU 非対応ターゲット）
        "ttsp_target": None,
        "lib": "target/stm32mp257f_dk_arm64_gcc/ttsp3",
        "qemu": None,
        "hw": {
            "kind": "openocd_swd",
            "serial": "/dev/ttyACM0",   # USART2/ST-LINK VCP（$TTSP_HW_SERIAL で上書き可）
            "baud": 115200,
            "capture": 45,              # UART キャプチャ上限（秒．swd-run 起動 ~8s 込み）
        },
        "unsupported_hw": {"ttsp_target_gain_tick", "ttsp_int_raise",
                           "ttsp_cpuexc_raise"},
        "soft_hw": {"ttsp_target_stop_tick", "ttsp_target_start_tick"},
        "skip_modules": {"interrupt", "exception"},
    },
}
