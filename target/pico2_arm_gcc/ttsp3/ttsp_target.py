#
#  Raspberry Pi PICO2 ARM（Cortex-M33 / rp2350）TTSP3 ターゲット定義
#
#  test/ttsp/run_ttsp.py が自動探索する（パスは asp3_core ルート相対）。
#  TTSP3 は本ボード用 ASP ターゲットを同梱しないため、asp3 側テスト資産
#  （ttsp_target_test.{c,h}・ttsp_target.cfg）を本フォルダに置く（lib＝本フォルダ）。
#  Cortex-M33＝mps2_an521 と同形の SKIP 規則。
#
#  実機は OpenOCD（CMSIS-DAP/Debugprobe）で書込み＆実行する（既存 run.cmake の
#  `run` ターゲットを再利用＝`program ... verify reset exit`。フラッシュへ書込み後
#  reset で起動。ボードは走り続ける）。UART(Debugprobe VCP=/dev/ttyACM2) を別途
#  キャプチャし "All check points passed." で判定する（_current_boot で最後のバナー
#  以降に限定）。program は毎回クリーンに reset するため連続再書込みがそのまま可能。
#
TTSP_TARGETS = {
    "pico2_arm_hw": {
        "preset": "pico2_arm",                 # 実機ビルド（QEMU 非対応ターゲット）
        "ttsp_target": None,
        "lib": "target/pico2_arm_gcc/ttsp3",
        "qemu": None,
        "hw": {
            "kind": "openocd_swd",             # OpenOCD 経由（ninja ターゲット再利用）
            "ninja_target": "run",             # run.cmake の run（program verify reset exit）
            "serial": "/dev/ttyACM2",          # Debugprobe VCP（$TTSP_HW_SERIAL で上書き可）
            "baud": 115200,
            "capture": 30,                     # UART キャプチャ上限（秒．flash 書込み込み）
        },
        "unsupported_hw": {"ttsp_target_gain_tick", "ttsp_int_raise",
                           "ttsp_cpuexc_raise"},
        "soft_hw": {"ttsp_target_stop_tick", "ttsp_target_start_tick"},
        "skip_modules": {"interrupt", "exception"},
    },
}
