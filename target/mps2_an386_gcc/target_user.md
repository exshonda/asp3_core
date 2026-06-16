# TOPPERS/ASP3 ターゲット依存部（ARM MPS2-AN386 / QEMU 用）

ARM MPS2+ FPGA ボードの AN386（レガシー CMSDK，シングル Cortex-M4，非
TrustZone）イメージを，QEMU の `mps2-an386` マシン上で動作させるための
ターゲット依存部である．AN505（`mps2_an505_gcc`，Cortex-M33/IoTKit）の依存部を
ベースに，ボード固有部（メモリマップ・CMSDK UART 番地・割込み番号・クロック）
を全面的に差し替えて構成している．本ターゲットの目的は，`arch/arm_m_gcc/common`
の `__TARGET_ARCH_THUMB == 4`（ARMv7-M）経路を QEMU で常時検証することである．

AN386 は QEMU の `hw/arm/mps2.c`（レガシー CMSDK 系）でモデル化され，AN505/AN521
の `hw/arm/mps2-tz.c`（IoTKit/SSE）とは別系統である．したがってメモリマップ・
UART 番地・割込み番号は AN505 と大きく異なる．

## 構築と実行

GNU Arm Embedded ツールチェイン（`arm-none-eabi-gcc`）と QEMU
（`qemu-system-arm`，8.2 以降）が必要である．

```sh
cmake --preset mps2_an386-qemu -B build/mps2_an386-qemu
cmake --build build/mps2_an386-qemu
ninja -C build/mps2_an386-qemu run   # QEMU(mps2-an386) で実行．UART0 が標準入出力に接続される
```

`run` ターゲットは次を実行する（終了は Ctrl-A X）：

```
qemu-system-arm -machine mps2-an386 -nographic \
    -semihosting-config enable=on,target=native -kernel asp.elf
```

サンプルプログラムの対話コマンド（`a`,`A`,`1`〜`3`,`d`,`t` 等）は，標準入力
から UART0 経由で受け付ける．

## 設計上の要点

### コア / 開発環境
- プロセッサコア：Cortex-M4（ARMv7-M，非 TrustZone）．
- TrustZone を持たないため `TOPPERS_ENABLE_TRUSTZONE` は定義しない．
  PSPLIM 等の `__TARGET_ARCH_THUMB >= 5` 経路は arch 共通部で自動的に無効化
  される．
- チップ依存部は持たない（`target.cmake` がコア共通の `arch.cmake` を直接イン
  クルード）．チップ層が提供していた定義（`TMIN_INTPRI`，`TBITW_IPRI`，
  エンディアン，`sil_orw` 等のビット操作，CMSIS 相当のレジスタ定義）は
  ボード依存部（`target_*.h`，`mps2_an386.h`）に取り込んでいる．

### FPU
- **ハードウェア FPU（FPv4-SP・単精度）を有効化する**．QEMU の `cortex-m4`
  は FPv4-SP-D16 を実装する（`hw/arm/tcg/cpu-v7m.c` の `cortex_m4_initfn`：
  `mvfr0 = 0x10110021`，`mvfr1 = 0x11000011`，D レジスタ 16 本・単精度）．
  CPACR の CP10/CP11 を有効化すれば FP 命令を実行できる．`target.cmake` で
  以下を設定する：
  - コンパイル定義：`TOPPERS_FPU_ENABLE`（`core_kernel_impl.c` が CPACR・FPCCR
    を設定）／`TOPPERS_FPU_CONTEXT`（`core_support.S` がディスパッチ時に
    `S16-S31` を退避）／`TOPPERS_FPU_LAZYSTACKING`（FPCCR=ASPEN|LSPEN）．
  - コンパイル/リンクオプション：`-mfloat-abi=softfp -mfpu=fpv4-sp-d16`．
- FPv4-SP は ARMv7-M（Cortex-M4）の FPU で，AN505 の FPv5-SP（ARMv8-M/Cortex-M33）
  とは別系統だが，S16-S31 のコンテキスト退避・遅延スタッキングの扱いは同型で
  ある．

### 割込み優先度ビット幅
- `TBITW_IPRI = 3`．Cortex-M4（AN386）は `__NVIC_PRIO_BITS = 3`＝3bit（8 レベル）
  を実装する．QEMU は 8bit を実装するが，3bit で設定した値は優先度レジスタの
  上位 3bit に置かれるため，QEMU・実機の双方で正しく動作する．

### メモリマップ（リンカスクリプト `mps2_an386.ld`）
AN386（レガシー CMSDK）のメモリマップ（`hw/arm/mps2.c` の `case FPGA_AN386`）に
基づく．
- `FLASH 0x00000000, 4MB`：SSRAM1（コード／ベクタ用．`0x00400000` にミラー）．
  Cortex-M4 はリセット時に VTOR=0 で `0x00000000` からベクタテーブルを読むた
  め，ベクタ（`.vector`）を FLASH 先頭に配置する．
- `RAM   0x20000000, 4MB`：SSRAM2&3（データ用）．

### 高分解能タイマ（HRT）
- **SysTick によるイベント駆動 HRT**（`USE_TIM_AS_HRT` を定義，
  `target_timer.[hc]`）．次のタイムイベントまでの相対時間を SysTick のリロー
  ド値に設定し，区間終端（0 到達）で割込みを発生させる．SysTick は M-profile
  共通のコア内タイマであり，AN386/AN505 で実装は共通である．
- SysTick はプロセッサクロック（QEMU の AN386 sysclk = 25MHz）で駆動する．
  HRT のカウント値（us）は SysTick カウント（25MHz）を 25 で除して得る．
- SysTick は 24bit のため 1 区間の最大は約 0.67 秒（`HRTCNT_BOUND`）．これ
  より長い待ちはカーネルが分割して再設定する．
- `target_hrt_get_current`/`set_event`/`raise_event` は，システムサービスや
  カーネルが局所的に `_kernel_` プレフィクスへリネームして参照するため，
  ヘッダではインライン関数（薄い転送）とし，実体は `target_timer.c` に置く．

### シリアル（CMSDK APB UART）
- UART0（`0x40004000`）を使用する（`cmsdk_uart.[ch]`，`target_serial.[ch]`）．
- AN386 の UART には combined 割込みが無く，TX / RX が個別の NVIC 線に割り
  当てられる（`hw/arm/mps2.c` の `uartirq[] = {0, 2, 4, 18, 20}` が RX 番号，
  TX は常にその +1）．UART0 は RX=IRQ0（INTNO 16）／TX=IRQ1（INTNO 17）．
  SIO ドライバは受信通知と送信可能通知の双方を要するため，同一の ISR
  （`sio_isr`）を両割込みに登録する（`target_serial.cfg`）．ISR は INTSTATUS を
  読んで両要因を処理・クリアするため，どちらの線で起動されても正しく動作する．

## テストプログラムの実行

`scripts/ci/run_testexec.py` を用いてテストをビルド・実行できる．

```sh
python3 scripts/ci/run_testexec.py \
    --options "--preset mps2_an386-qemu" \
    --run "timeout 60 qemu-system-arm -machine mps2-an386 -nographic -semihosting-config enable=on,target=native -kernel asp.elf" \
    --workdir build/testexec-an386 \
    task1 sem1 flg1 tmevt1 hrt1
```

テスト終了（`ext_ker`）で QEMU を終了させるため，`target_exit()` は
`TOPPERS_USE_QEMU` 定義時にセミホスティング `SYS_EXIT` を発行する
（`target.cmake` で既定定義）．

## 動作確認

QEMU 8.2.2 ＋ arm-none-eabi-gcc で確認済み．

- サンプルプログラム（`sample1`）：起動メッセージ表示，各タスクの周期実行，
  `dly_tsk` による時間待ち，UART からのコマンド入力が正しく動作する．
- カーネルの機能テスト（`testexec`）：`task1`，`sem1`，`flg1`，`tmevt1`，
  `hrt1` が「All check points passed」で合格する．
- `int1`/`cpuexc*` は ARM-M 系の HW 割込み／CPU 例外を要し，他の arm_m_gcc
  ターゲットと同様の対応状況である．
