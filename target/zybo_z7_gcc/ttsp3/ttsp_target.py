#
#  Zybo Z7（Cortex-A9）TTSP3 ターゲット定義
#
#  test/ttsp/run_ttsp.py が target/*/ttsp3/ttsp_target.py を自動探索して
#  TARGETS を組み立てる（中央の定義表は持たない＝ターゲット追加時に共有
#  ドライバを編集しない）。パスは asp3_core ルート相対。
#
#  zybo は TTSP3 同梱の ASP ターゲットライブラリ（ttsp_target="zybo_z7_gcc"）を
#  流用するため、asp3 側テスト資産（ttsp_target_test.*）は持たない。実機は
#  xsct(Vitis) でロード＆実行する（jtag.tcl は本フォルダ）。全 HW 対応＝SKIP なし。
#
TTSP_TARGETS = {
    "zybo_z7": {
        "preset": "zybo_z7-qemu",
        "ttsp_target": "zybo_z7_gcc",
        "lib": None,
        "qemu": ("qemu-system-arm -M xilinx-zynq-a9 -semihosting -m 512M "
                 "-serial null -serial mon:stdio -nographic -kernel {elf}"),
        "unsupported_hw": set(),   # zybo は全 HW 対応＝SKIP なし
        "soft_hw": set(),
        "skip_modules": set(),
    },
    "zybo_z7_hw": {
        "preset": "zybo_z7",            # 実機ビルド（ZYBO_QEMU=OFF）
        "ttsp_target": "zybo_z7_gcc",   # TTSP3 同梱の ASP ターゲットライブラリ
        "lib": None,
        "qemu": None,
        "hw": {
            "kind": "xsct_zybo",
            "jtag_tcl": "target/zybo_z7_gcc/ttsp3/jtag.tcl",
            "ps7_init": "target/zybo_z7_gcc/xilinx_sdk/zybo_z7_hw/ps7_init.tcl",
            "serial": "/dev/ttyUSB1",   # FT2232 ch.B（$TTSP_HW_SERIAL で上書き可）
            "baud": 115200,
            "vitis_settings": "/usr/local/tools/Vitis/2024.2/settings64.sh",
            "capture": 35,              # UART キャプチャ上限（秒．早期検出で短縮）
        },
        "unsupported_hw": set(),
        "soft_hw": set(),
        "skip_modules": set(),
    },
}
