#
#  Zybo Z7 (Cortex-A9) 実機ロード＆実行スクリプト（xsct 用・ASP3 シングルコア）
#
#  TTSP3 適合性テストの実機ランナー（test/ttsp/run_ttsp.py の zybo_z7_hw）から
#  起動される．FMP3 の target/zybo_z7_gcc/xilinx_sdk/jtag.tcl を基に，ASP3 は
#  シングルプロセッサのため Cortex-A9 #0 のみを扱うよう簡略化したもの．
#
#  使い方:  xsct jtag.tcl <ps7_init.tcl> <ELF>
#
#  rst -system で毎回クリーンにリセットするため，openocd 方式で必要だった
#  キャッシュ正規化スタブは不要（TTSP3_HOWTO.md A 方式）．con の後すぐ exit して
#  ボードは走らせたまま xsct を終了し，UART は別途キャプチャして判定する．
#
set ps7_init_tcl [lindex $argv 0]
set load_obj     [lindex $argv 1]

connect -url tcp:127.0.0.1:3121
source $ps7_init_tcl

#  APU をクリーンにリセット（コア／キャッシュ）
targets -set -nocase -filter {name =~"APU*"} -index 0
rst -system
after 1000

#  DDR／クロック初期化（ネイティブ ps7_init）
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"} -index 0
ps7_init
ps7_post_config

#  Core0 へロード
targets -set -nocase -filter {name =~ "ARM*#0"} -index 0
dow $load_obj
configparams force-mem-access 0

#  実行（Core0 のみ．ASP3 は Core1 を使用しない）
targets -set -nocase -filter {name =~ "ARM*#0"} -index 0
con

#  UART フラッシュ待ち → 切断して終了（ボードは走り続ける）
after 500
disconnect
exit
