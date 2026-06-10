# TOPPERS/ASP3 ターゲット依存部（ARM MPS2-AN521 / QEMU 用）

ARM MPS2+ FPGA ボードの AN521（SSE-200，デュアル Cortex-M33）イメージを，
QEMU の `mps2-an521` マシン上で動作させるためのターゲット依存部である．
RaspberryPi Pico2（`pico2_arm_gcc`，Cortex-M33）の依存部をベースに，
チップ依存部を持たない自己完結型のボード依存部として構成している．

## 構築と実行

GNU Arm Embedded ツールチェイン（`arm-none-eabi-gcc`）と QEMU
（`qemu-system-arm`，11.0 以降）が必要である．

```sh
cmake --preset mps2_an521-qemu -B build/mps2_an521-qemu
cmake --build build/mps2_an521-qemu
ninja -C build/mps2_an521-qemu run   # QEMU(mps2-an521) で実行．UART0 が標準入出力に接続される
```

`run` ターゲットは次を実行する（終了は Ctrl-A X）：

```
qemu-system-arm -machine mps2-an521 -nographic \
    -semihosting-config enable=on,target=native -kernel asp.elf
```

サンプルプログラムの対話コマンド（`a`,`A`,`1`〜`3`,`d`,`t` 等）は，標準入力
から UART0 経由で受け付ける．

## 設計上の要点

### コア / 開発環境
- プロセッサコア：Cortex-M33（ARMv8-M）．
- リセット後は Secure ステートで起動するため `TOPPERS_ENABLE_TRUSTZONE` を定義．
- チップ依存部は持たない（`target.cmake` がコア共通の `arch.cmake` を直接イン
  クルード）．チップ層が提供していた定義（`TMIN_INTPRI`，`TBITW_IPRI`，
  エンディアン，`sil_orw` 等のビット操作，CMSIS 相当のレジスタ定義）は
  ボード依存部（`target_*.h`，`mps2_an521.h`）に取り込んでいる．

### FPU
- **使用しない（FPU関連の定義・コンパイルオプションを設定しない）**．QEMU の SSE-200 では，QEMU が
  実行するコア（CPU0）に FPU が実装されていない
  （`hw/arm/armsse.c` の `sse200_properties`：`CPU0_FPU = false`，CPU1 のみ
  FPU を持つ）．このため CPACR の CP10/CP11 を有効化しても無視され，FP 命令
  は NOCP UsageFault となる．ハードウェア FPU を有効にすると，64bit 値の
  移動に `vldr`/`vstr` が使われた箇所で確実にフォールトするため，ソフトウェ
  ア浮動小数点でビルドする．

### 割込み優先度ビット幅
- `TBITW_IPRI = 3`．実機 SSE-200 は 3bit（8 レベル）を実装する．QEMU は
  8bit を実装するが，3bit で設定した値は優先度レジスタの上位 3bit に置かれ
  るため，QEMU・実機の双方で正しく動作する．

### メモリマップ（リンカスクリプト `mps2_an521.ld`）
SSE-200 のメモリマップ（`hw/arm/armsse.c`）に基づく．
- `FLASH 0x10000000, 4MB`：コード用 SSRAM（`0x00000000` のエイリアス）．
  CPU はリセット時に init-svtor（=`0x10000000`）からベクタテーブルを読むた
  め，ベクタ（`.vector`）を FLASH 先頭に配置する．
- `RAM   0x38000000, 4MB`：ボード SSRAM（`0x28000000` のエイリアス）．

### 高分解能タイマ（HRT）
- **SysTick によるイベント駆動 HRT**（`USE_TIM_AS_HRT` を定義，
  `target_timer.[hc]`）．次のタイムイベントまでの相対時間を SysTick のリロー
  ド値に設定し，区間終端（0 到達）で割込みを発生させる．これにより，タイム
  イベントの無い時刻に `signal_time()` が呼ばれることがない（周期ティック方
  式で生じる「no time event is processed in hrt interrupt」の頻発を回避）．
- SysTick はプロセッサクロック（QEMU の mainclk = 20MHz；SSE は systick
  refclk を配線しないため CLKSOURCE はプロセッサクロックを使用）で駆動する．
  HRT のカウント値（us）は SysTick カウント（20MHz）を 20 で除して得る．
- **カウント値の構成**：区間の開始時刻 `hrt_base`[us] と区間内オフセット
  `(RVR - SYST_CVR)` から `get_current = hrt_base + offset/20` を求める．区間
  がまるごと 1 回経過したことは「割込みが発生したこと」が表すため，割込み
  ハンドラで `hrt_base` に 1 区間分を加算する．これにより，区間中にカウント
  値の読出しが一度も行われない場合（`dly_tsk` 等）でも経過時間を正しく計上
  できる（ダウンカウンタが 1 周して同値に戻るため，端点だけの観測では経過が
  見えない問題を回避）．`get_current` は単調性を保証するためのクランプを行う．
  `raise_event` は手動の割込み保留と区間終端を区別するため，最小区間で張り直
  して直ちに割込みを起こす．
- SysTick は 24bit のため 1 区間の最大は約 0.84 秒（`HRTCNT_BOUND`）．これ
  より長い待ちはカーネルが分割して再設定する．
- `target_hrt_get_current`/`set_event`/`raise_event` は，システムサービスや
  カーネルが局所的に `_kernel_` プレフィクスへリネームして参照するため，
  ヘッダではインライン関数（薄い転送）とし，実体は `target_timer.c` に置く．
- 周期ティック方式（`USE_TIM_AS_HRT` を外しコア依存部の `core_timer` を使用）も
  選択可能だが，タイムイベントの無いティックでも `signal_time()` が呼ばれる．

### シリアル（CMSDK APB UART）
- UART0（`0x40200000`）を使用する（`cmsdk_uart.[ch]`，`target_serial.[ch]`）．
- 送受信割込みは rx / tx / combined の 3 本があり（`hw/arm/mps2-tz.c` の
  `make_uart`），combined 割込み（NVIC IRQ 42）1 本で送受信を処理する．

## テストプログラムの実行

`test/testexec.py` を用いて，テスト毎のディレクトリ（`TEST-<名前>`）を作って
ビルド・実行できる．本ターゲット用に，実行ディレクトリ（例：`TEST/`）へ次の
2 ファイルを用意する．

- `TARGET_OPTIONS`（1 行目＝CMake の configure 引数）：

  ```
  --preset mps2_an521-qemu
  ```

- `TARGET_RUN`（実行コマンド．QEMU で起動し，テスト終了時に
  セミホスティングで QEMU が終了する）：

  ```
  timeout 30 qemu-system-arm -machine mps2-an521 -nographic -semihosting-config enable=on,target=native -kernel asp.elf
  ```

実行例（`TEST/` で）：

```sh
python3 ../test/testexec.py task1 sem1 tmevt1 # 指定テストを構築・実行
python3 ../test/testexec.py all               # 全テスト
```

テスト終了（`ext_ker`）で QEMU を終了させるため，`target_exit()` は
`TOPPERS_USE_QEMU` 定義時にセミホスティング `SYS_EXIT` を発行する
（`target.cmake` で既定定義）．

## 動作確認

QEMU 11.0.0 ＋ arm-none-eabi-gcc 13.2.1 で確認済み．

- サンプルプログラム（`sample1`）：起動メッセージ表示，各タスクの周期実行，
  `dly_tsk` による時間待ち，UART からのコマンド入力（`a`/`A`/`t`/`d` 等）が
  正しく動作する．
- カーネルの機能テストプログラム（`test/`）：`task1`，`sem1`/`sem2`，`flg1`，
  `dtq1`，`pdq1`，`mpf1`，`mutex1`〜`mutex8`，`sysman1`，`sysstat1`，
  `suspend1`，`exttsk`，`notify1`，`raster1`/`raster2`，`hrt1`，`tmevt1` が
  すべて「All check points passed」（`hrt1` はカウンタ逆行 0）で合格する．
- `dlynse`（`sil_dly_nse` のビジーウェイト校正テスト）は，QEMU がサイクル精度
  でないため（ビジーループ速度とホスト依存で，仮想 20MHz の HRT と一致しない）
  正確には合格しない．`int1`/`cpuexc*` は `target_test.h` にテスト用割込み
  番号等の定義が必要で，ARM-M 系ターゲットでは未対応（本ターゲットも同様）．
