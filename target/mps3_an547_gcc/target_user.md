# TOPPERS/ASP3 ターゲット依存部（ARM MPS3-AN547 / QEMU 用）

ARM MPS3 FPGA ボードの AN547（SSE-300，シングル Cortex-M55，ARMv8.1-M）
イメージを，QEMU の `mps3-an547` マシン上で動作させるためのターゲット依存部
である．`mps2_an505_gcc`（IoTKit/Cortex-M33）から派生し，チップ依存部を持た
ない自己完結型のボード依存部として構成している．

> **Step 1 の範囲**：本ターゲットはスカラ FP まで（FPv5-D16）を有効化する．
> MVE(Helium) のコンテキスト退避は別ステップ（共通 arch への変更）で追加する
> ため，本 Step では `-mcpu=cortex-m55+nomve` として MVE を無効化している．

## 構築と実行

GNU Arm Embedded ツールチェイン（`arm-none-eabi-gcc`）と QEMU
（`qemu-system-arm`，`mps3-an547` をサポートする版＝8.2 以降）が必要である．

```sh
cmake --preset mps3_an547-qemu -B build/mps3_an547-qemu
cmake --build build/mps3_an547-qemu
ninja -C build/mps3_an547-qemu run   # QEMU(mps3-an547) で実行．UART0 が標準入出力に接続される
```

`run` ターゲットは次を実行する（終了は Ctrl-A X）：

```
qemu-system-arm -machine mps3-an547 -nographic \
    -semihosting-config enable=on,target=native -kernel asp.elf
```

サンプルプログラムの対話コマンド（`a`,`A`,`1`〜`3`,`d`,`t` 等）は，標準入力
から UART0 経由で受け付ける．

## 設計上の要点

### コア / 開発環境
- プロセッサコア：Cortex-M55（ARMv8.1-M，SSE-300）．
- リセット後は Secure ステートで起動するため `TOPPERS_ENABLE_TRUSTZONE` を定義．
- チップ依存部は持たない（`target.cmake` がコア共通の `arch.cmake` を直接イン
  クルード）．チップ層が提供していた定義（`TMIN_INTPRI`，`TBITW_IPRI`，
  エンディアン，`sil_orw` 等のビット操作，CMSIS 相当のレジスタ定義）は
  ボード依存部（`target_*.h`，`mps3_an547.h`）に取り込んでいる．

### FPU
- **ハードウェア FPU（FPv5・倍精度＝FPv5-D16）を有効化する**．SSE-300 は CPU0
  が FPU を実装する（`hw/arm/armsse.c` の `sse300_properties`：
  `CPU0_FPU = true`）ため，CPACR の CP10/CP11 を有効化すれば FP 命令を実行で
  きる．`target.cmake` で以下を設定する：
  - コンパイル定義：`TOPPERS_FPU_ENABLE`（`core_kernel_impl.c` が CPACR・FPCCR
    を設定）／`TOPPERS_FPU_CONTEXT`（`core_support.S` がディスパッチ時に
    `S16-S31` を退避）／`TOPPERS_FPU_LAZYSTACKING`（FPCCR=ASPEN|LSPEN．遅延
    スタッキング．これが無いと `FPCCR_INIT undeclared` でビルド不成立）．
  - コンパイル/リンクオプション：`-mfloat-abi=softfp -mfpu=fpv5-d16`．Cortex-M55
    の FPU は倍精度対応（QEMU `cpu-v7m.c`：`MVFR0=0x10110221`＝単精度/倍精度
    とも実装）のため `fpv5-d16`（an505 の `fpv5-sp-d16` ＝単精度とは異なる）．
  - `objdump -d` で `vpush/vpop {s16-s31}`・`vldr d7` 等の FP 命令が生成される
    ことを確認済み（`d7` ＝倍精度レジスタ参照）．

### 割込み優先度ビット幅
- `TBITW_IPRI = 3`．実機 AN547（SSE-300）の NVIC は 3bit（8 レベル）を実装する．
  QEMU は 8bit を実装するが，3bit で設定した値は優先度レジスタの上位 3bit に
  置かれるため，QEMU・実機の双方で正しく動作する．

### メモリマップ（リンカスクリプト `mps3_an547.ld`）
SSE-300/AN547 のメモリマップ（`hw/arm/armsse.c`・`hw/arm/mps2-tz.c`
`an547_raminfo[]`・`mps3tz_an547_class_init`）に基づく．AN505 とは init-svtor
とメモリマップが異なる．
- `FLASH 0x00000000, 512KB`：**ITCM**（SSE-300 は ITCM を `0x0` に配置．
  `armsse.c`「The SSE-300 has an ITCM at 0x0000_0000」）．CPU はリセット時に
  init-svtor（=`0x00000000`，`mps3tz_an547_class_init`）から初期 SP/PC（ベクタ
  テーブル）を読むため，ベクタ（`.vector`）とコードを ITCM 先頭に配置する．
- `RAM   0x21000000, 4MB`：sram 2（`an547_raminfo[]` の "sram 2"）．データを配置．
- その他（未使用）：sram `0x01000000`(2MB)，DTCM `0x20000000`(512KB)，
  QSPI(ROM) `0x28000000`(8MB)，DDR `0x60000000`．

### 高分解能タイマ（HRT）
- **SysTick によるイベント駆動 HRT**（`USE_TIM_AS_HRT` を定義，
  `target_timer.[hc]`）．次のタイムイベントまでの相対時間を SysTick のリロー
  ド値に設定し，区間終端（0 到達）で割込みを発生させる．
- SysTick はプロセッサクロック（QEMU の MAINCLK = cpuclk = 32MHz；SSE は
  systick refclk を配線しないため CLKSOURCE はプロセッサクロックを使用）で
  駆動する．HRT のカウント値（us）は SysTick カウント（32MHz）を 32 で除して
  得る（`HRT_CLOCKS_PER_US = CPU_CLOCK_HZ / 1000000 = 32`）．
- **カウント値の構成**：区間の開始時刻 `hrt_base`[us] と区間内オフセット
  `(RVR - SYST_CVR)` から `get_current = hrt_base + offset/32` を求める．区間
  がまるごと 1 回経過したことは「割込みが発生したこと」が表すため，割込み
  ハンドラで `hrt_base` に 1 区間分を加算する．これにより，区間中にカウント
  値の読出しが一度も行われない場合（`dly_tsk` 等）でも経過時間を正しく計上
  できる．`get_current` は単調性を保証するためのクランプを行う．
- SysTick は 24bit のため 1 区間の最大は約 0.52 秒（`HRTCNT_BOUND`）．これ
  より長い待ちはカーネルが分割して再設定する．

### シリアル（CMSDK APB UART）
- UART0（`0x49303000`）を使用する（`cmsdk_uart.[ch]`，`target_serial.[ch]`）．
  AN505 の `0x40200000` とは異なる（`an547_ppcs`：apb_ppcexp2 の "uart0"）．
- 送受信割込みは rx / tx / combined の 3 本があり（`hw/arm/mps2-tz.c` の
  `make_uart`．`an547_ppcs` uart0 = {rx=33, tx=34, combined=43}），combined
  割込み（NVIC IRQ 43）1 本で送受信を処理する．

## テストプログラムの実行

`test/testexec.py` を用いて，テスト毎のディレクトリを作ってビルド・実行できる．
本ターゲット用の実行ディレクトリへ次の 2 ファイルを用意する．

- `TARGET_OPTIONS`（1 行目＝CMake の configure 引数）：

  ```
  --preset mps3_an547-qemu
  ```

- `TARGET_RUN`（実行コマンド．QEMU で起動し，テスト終了時に
  セミホスティングで QEMU が終了する）：

  ```
  timeout 60 qemu-system-arm -machine mps3-an547 -nographic -semihosting-config enable=on,target=native -kernel asp.elf
  ```

テスト終了（`ext_ker`）で QEMU を終了させるため，`target_exit()` は
`TOPPERS_USE_QEMU` 定義時にセミホスティング `SYS_EXIT` を発行する
（`target.cmake` で既定定義）．

## 動作確認

QEMU 8.2.2 ＋ arm-none-eabi-gcc 13.2.1 で確認済み（Step 1）．

- サンプルプログラム（`sample1`）：起動メッセージ
  「TOPPERS/ASP3 Kernel ... for ARM MPS3-AN547」表示，各タスクの周期実行，
  `dly_tsk` による時間待ちが正しく動作する．
- `scripts/ci/run_testexec.py` のスモーク 5 項目（`task1`,`sem1`,`flg1`,`tmevt1`,
  `hrt1`）が **5/5 PASS**．
- `objdump -d` で FP 命令（`vpush/vpop {s16-s31}`，`vldr d7`，`vstr`）が生成され，
  MVE 命令は含まれないことを確認済み．
- `dlynse`（`sil_dly_nse` のビジーウェイト校正テスト）は，QEMU がサイクル精度
  でないため正確には合格しない．`int1`/`cpuexc*` は ARM-M 系ターゲットの既知
  事情（本ターゲットも同様）．
