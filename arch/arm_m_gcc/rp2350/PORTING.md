# TOPPERS/ASP3 → Raspberry Pi Pico 2 (RP2350) 移植メモ

最終更新: 2026-06-05

## 目的
TOPPERS/ASP3 (Release 3.7.2) を Raspberry Pi **Pico 2**
(RP2350: Cortex-M33 ×2 / FPU(FPv5-SP) / 4MB QSPI フラッシュ / 520KB SRAM) へ移植する。
ASP3 はシングルプロセッサカーネルのため **Core0 のみ**を使用（シングルコア実行）。
ビルドは ASP3 標準の Makefile + configure.rb（Ruby 版コンフィギュレータ）。cmake は使わない。

## 成果サマリ — **完了 (2026-06-04)**

**ビルド成功 + 実機でシングルコア完全動作**。bootROM(IMAGE_DEF)→Secure ステート起動→
クロック 150MHz→カーネル起動→LOGTASK/タスク/HRT タイマ/シリアル受信割込みが連続動作する
ことを確認（sample1 のバナー・周期タスク出力・キー入力応答）。

実機 UART(UART0/Debugprobe 経由 ttyACM0) 出力:
```
TOPPERS/ASP3 Kernel Release 3.7.2 for RaspberryPi Pico2:ARM Cortex-M33 (Jun  4 2026, ...)
...
System logging task is started.
Sample program starts (exinf = 0).
task1 is running (001).   |
task1 is running (002).   |
  ... (連続。'r' キー入力で rot_rdq → task2 へ切替も確認)
```

- ビルドサイズ（sample1, TECS 構成）: FLASH 約 22.8KB / RAM 約 13.5KB
- 書き込み・デバッグ: Raspberry Pi フォーク OpenOCD + Debugprobe(CMSIS-DAP)

## 移植方針・参考にしたもの

| 参考 | 使い方 |
|---|---|
| `target/raspberrypi_pico_gcc` + `arch/arm_m_gcc/rp2040`（初代 Pico, CM0+） | ディレクトリ構成・Makefile・クロック初期化（XOSC→PLL）・tUsart(PL011) の雛形。**ベースとして複製** |
| `target/nucleo_h563zi_gcc`（CM33, .trb） | CM33 ターゲットの正規の構成（`CORE_TYPE=CORTEX_M33`, `FPU_LAZYSTACKING`, trb の書き方, `TBITW_IPRI` の定義場所） |
| exshonda 版 `asp3_pico_sdk`（pico-sdk + cmake の RP2350 移植） | `RP2350.h`（レジスタ定義）・`chip_*.h`・`target_timer.*`（TIMER0 ALARM0 HRT）は**ほぼ流用**。pico-sdk ランタイム統合部（`__wrap_*`・Python 版 cfg・`target_serial.c`）は**捨てて標準方式に置換** |
| metebalci/rp2350-bare-metal-build | cmake を使わないビルドの先例。IMAGE_DEF ブロック（boot stage2 不要）の知見 |
| pico-sdk（移植作業時のみ一時クローンして参照） | `picobin.h` / `embedded_start_block.inc.S`（IMAGE_DEF の正確なビットフィールド）・`hardware_regs`（レジスタフィールド確認）の**リファレンスとして使用**。ビルド自体は自己完結の `RP2350.h` を使い **pico-sdk に依存しない**ため，**ツリーには同梱しない**（検証完了後に削除済み） |

## 追加・変更したファイル

### chip 依存部 `arch/arm_m_gcc/rp2350/`（サンプル+rp2040 を基に作成）
- `RP2350.h` — レジスタ定義（CLOCKS/RESETS/XOSC/PLL/UART/TICKS/TIMER0/PADS/IO_BANK0/
  SIO/M33 PPB）。exshonda 版をそのまま流用（自己完結・pico-sdk 非依存）。
- `Makefile.chip` — rp2040 版を複製し **`CORE_TYPE = CORTEX_M33`**。
- `chip_sil.h` — RP2350 のアトミックアクセスエイリアス（xor:+0x1000 / set:+0x2000 /
  clear:+0x3000）を使う `sil_orw` 等。**`TBITW_IPRI=4` を追加**（nucleo_h563zi は
  `stm32h5xx_stm32cube/chip_sil.h` で定義しており同じ流儀。未定義だと
  `core_kernel_impl.h` がコンパイルエラー）。
- `tUsart.c/.cdl` — rp2040 版（PL011）を `RP2040_`→`RP2350_` 置換のみで流用
  （RP2350 の UART も同じ PL011 でレジスタ互換）。
- `chip_kernel.h`（`TMIN_INTPRI=-3`）, `chip_kernel_impl.h`, `chip_stddef.h`,
  `chip_syssvc.h`, `chip_rename.*`, `chip_unrename.h`, `chip_cfg1_out.h`, `MANIFEST`
  — サンプル/rp2040 版をほぼそのまま流用。

### target 依存部 `target/raspberrypi_pico2_gcc/`（raspberrypi_pico_gcc を複製して改変）
- `Makefile.target` — `BOARD=raspberrypi_pico2`, `CHIP=rp2350`,
  **`FPU_USAGE=FPU_LAZYSTACKING`**, **`ENABLE_TRUSTZONE=1`（重要・後述）**,
  `KERNEL_TIMER=TIM`。`bs2_default_padded_checksummed.o`（boot stage2）を廃止し
  `image_def.o` に置換。SWD デバッグ支援ターゲット
  （run/openocd/gdb/swd-debug/console）を追加。
- `image_def.S`（**新規**） — RP2350 bootROM 用 **IMAGE_DEF 最小ブロック（20 バイト）**。
  `.picobin_block` セクション。形式は pico-sdk `picobin.h` に基づく
  （START=0xffffded3 / IMAGE_TYPE item 0x42,size1,**0x1021**(EXE|Secure|ARM|RP2350) /
  LAST item / link=0(単一ブロックループ) / END=0xab123579）。
- `rpi_pico2.ld` — rpi_pico.ld を基に FLASH 4MB / RAM 512KB。`.boot2` を削除し
  **`KEEP(*(.vector))` → `KEEP(*(.picobin_block))`** の順で配置（順序が重要・後述）。
- `target_kernel_impl.c` — `hardware_init_hook()` を RP2350 用に全面書換え（後述の
  発見 2〜4・6 を全て反映）。`boot2_image` 参照を `image_def_block` 参照に置換（発見 5）。
- `target_timer.c/.h` — exshonda 版を流用（TIMER0 ALARM0 を HRT に使用。1MHz,
  `INTNO_TIMER=16`, `HRTCNT_BOUND=4000000002`, `TCYC_HRTCNT` 未定義, `TSTEP_HRTCNT=1`）。
- `target_kernel.trb` — **`IncludeTrb("core_kernel.trb")` の 1 行のみ**（rp2040 の
  `core_kernel_v6m.trb` から変更。CM33 のベクタテーブル生成は標準 trb で完結し，
  exshonda 版の Python cfg 固有処理（`isr_irqN` エイリアス・`.ram_vector_table`）は
  pico-sdk ランタイム統合用であり**不要**）。`target_check.trb` も同様。
- `rpi_pico.h` — `CPU_CLOCK_HZ=150000000`, `TMAX_INTNO=(52+16)=68`（exshonda 版流用）。
- `tSIOPortTarget.cdl` — `RP2350_UART0_BASE` / `RP2350_UART0_IRQn(33)+16=49`。
- `swd-debug.gdb`（新規）, `target_user.md`（新規・利用ガイド）, ほか `target_*` 一式。

### 共通部の変更: **なし**
`arch/arm_m_gcc/common` / `kernel` / `cfg` 等は一切変更していない（CM33 対応・FPU・
TrustZone 分岐・ベクタ生成は全て既存の共通部で対応済みだった）。

## ビルド・実行方法（要点）
```bash
cd asp3_3.7 && mkdir OBJ_pico2 && cd OBJ_pico2
ruby ../configure.rb -T raspberrypi_pico2_gcc \
     -V /usr/local/tools/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin
make            # -> asp (ELF)
make run        # OpenOCD(RPiフォーク)で書き込み＆実行
make console    # UART コンソール (/dev/ttyACM0 115200)
```
詳細は `target/raspberrypi_pico2_gcc/target_user.md` を参照。

## 実機での確立事項 / ハマりどころ

### ★重要な発見 1: Secure ステートには `ENABLE_TRUSTZONE=1` が必須（INVPC で即死）

**症状**: 書き込み・リセット後に UART 無出力。openocd で halt すると
`xPSR=0x69000006`（例外番号 6 = UsageFault がアクティブ）, `CFSR=0x00040000`
（UFSR の **INVPC**）, PC は `target_exit` の無限ループ
（INVPC → デフォルト例外ハンドラ → カーネル終了，という経路）。

**原因**: `arm_m.h` は
```c
#if __TARGET_ARCH_THUMB >= 5 && ! defined(TOPPERS_ENABLE_TRUSTZONE)
#define EXC_RETURN  0xffffffbc   /* Non-secure ステート用 */
#else
#define EXC_RETURN  0xfffffffd   /* Secure ステート用 */
#endif
```
であり，`ENABLE_TRUSTZONE=0` だと **Non-secure 用の EXC_RETURN(0xffffffbc, S=0/ES=0)**
が使われる。nucleo_h563zi（TZEN 無効の STM32H5）はこれで良いが，**RP2350 の bootROM は
ARM イメージを Secure ステートで起動する**ため，最初の例外リターンで EXC_RETURN の
整合性チェックに引っかかり INVPC UsageFault となる。

**対策**: `Makefile.target` で **`ENABLE_TRUSTZONE = 1`**（→ `-DTOPPERS_ENABLE_TRUSTZONE`，
EXC_RETURN=0xfffffffd）。exshonda 版の arch.cmake が `TOPPERS_ENABLE_TRUSTZONE` を
定義していたのはこのためだった。「TrustZone で Secure/NS 分割する」のではなく
「**Secure 単独動作のための EXC_RETURN 選択**」である点に注意。

### ★重要な発見 2: TIMER0 は TICKS ブロックを設定しないと歩進しない

RP2040 では TIMER のカウントソースは watchdog の tick だったが，**RP2350 では独立した
TICKS ブロック（0x40108000）経由**に変わった。未設定だと TIMER0 が止まったままで，
`target_hrt_initialize()` の `while (target_hrt_get_current() < 1) ;` で**無限ループ**する。

対策（`hardware_init_hook()`）:
```c
sil_wrw_mem(RP2350_TIMER0_SOURCE, RP2350_TIMER0_SOURCE_TICK);
sil_wrw_mem(RP2350_TICKS_TIMER0_CYCLES, 12);   /* clk_ref 12MHz / 12 = 1MHz */
sil_wrw_mem(RP2350_TICKS_TIMER0_CTRL, RP2350_TICKS_TIMER0_CTRL_ENABLE);
while ((sil_rew_mem(RP2350_TICKS_TIMER0_CTRL) & RP2350_TICKS_TIMER0_CTRL_RUNNING) == 0) ;
```

### ★重要な発見 3: PADS の ISO ビット解除（RP2350 固有）

RP2350 はリセット後にパッドが**アイソレーション状態**（PADS_BANK0 の ISO=bit8 がセット）
であり，FUNCSEL を設定しただけでは**信号がピンに出ない**。GP0/GP1 の ISO クリアと
IE セットを `hardware_init_hook()` で行う（RP2040 には無い手順）。

### ★重要な発見 4: アトミックアクセスエイリアスは M33 PPB に効かない

`chip_sil.h` の `sil_orw()`/`sil_clrw()` は RP2350 のアトミックエイリアス
（アドレス +0x2000/+0x3000 への書き込み）で実装されているが，これは APB/AHB
ペリフェラル用であり **M33 の PPB（NVIC/SCB/CPACR 等, 0xE000xxxx）には存在しない**
（書き込みは無害に無視される）。

共通部 `core_kernel_impl.c` の FPU 有効化が `sil_orw((uint32_t*)CPACR, ...)` を使って
おり**空振りする**。exshonda 版で顕在化しなかったのは pico-sdk のランタイムが先に
CPACR を設定していたため。本移植では `hardware_init_hook()` で
`sil_wrw_mem(RP2350_M33_CPACR, sil_rew_mem(...) | CPACR_FPU_ENABLE)` と**直接 RMW**で
設定する（共通部は変更しない）。grep の結果，共通部でエイリアス系 sil マクロを
PPB に使うのはこの 1 箇所のみ。

### ★重要な発見 5: `image_def.o` はシンボル参照でリンク強制が必要

`image_def.o` は `libkernel.a` に入るため，どこからも参照されないと**リンカが
アーカイブから取り出さず**，IMAGE_DEF ブロックが ELF に入らない（`KEEP()` は
「取り込まれたオブジェクトのセクションを gc から守る」だけで，アーカイブ抽出は
強制しない）。→ ボードは無反応（bootROM がイメージを見つけられない）。

対策: `target_kernel_impl.c` で
```c
extern volatile int image_def_block;
void hardware_init_hook(void) {
    if (image_def_block) ;   /* Enforce image_def_block to be linked */
    ...
```
（rp2040 版の `boot2_image` 参照と同じ手法。objdump で
`d3deffff ... 42012110 ... 793512ab` がフラッシュ先頭 4KB 内に入ったことを確認する）

### ★重要な発見 6: クロック分周レジスタのフィールド位置が RP2040 と異なる

`CLK_REF_DIV` / `CLK_SYS_DIV` の整数部 `INT` が **RP2040: bit8〜 → RP2350: bit16〜**
に変わっている（pico-sdk `hardware_regs/clocks.h` で確認:
`CLOCKS_CLK_SYS_DIV_INT_LSB = 16`）。RP2040 版の `1 << 8` をそのまま使うと分周比が
異常値になる。本移植は `1 << 16` に修正済み。
PLL は 12MHz × **125**（VCO 1500MHz, 範囲 750–1600MHz 内）÷5 ÷2 = **150MHz**。

### ★重要な発見 7: ベクタテーブルと IMAGE_DEF ブロックの配置順

bootROM は IMAGE_DEF を**先頭 4KB から探索**するが，ベクタテーブル項目を持たない
最小 IMAGE_DEF の場合，**イメージ先頭（0x10000000）をベクタテーブルとみなして**
SP=vector[0] / PC=vector[1] で起動する。よってリンカスクリプトは
```
KEEP(*(.vector))          /* 先頭: コンフィギュレータ生成のベクタテーブル */
KEEP(*(.picobin_block))   /* その直後（4KB 以内ならどこでも可） */
```
の順とする（pico-sdk の `memmap_default.ld` も vectors → embedded_block の順）。

### その他の確立事項

- **ベクタテーブル生成**: 標準の `core_kernel.trb` が CM33 用の
  `_kernel_vector_table[]`（`.vector` セクション）を完全生成する。
  `nm` で `_kernel_vector_table` が 0x10000000 に来ることを確認。
  exshonda 版 Python cfg の RAM ベクタ・`isr_irqN` エイリアスは pico-sdk の
  crt0/ランタイム統合用であり，SDK 非使用なら不要（「捨てて標準に戻す」が正解）。
- **TBITW_IPRI=4**: RP2350 の NVIC 優先度は 4bit 実装。`chip_sil.h` で定義
  （未定義だとビルドエラー。定義場所は nucleo_h563zi の流儀に合わせた）。
- **boot stage2 / picotool / UF2 はすべて不要**: IMAGE_DEF ブロックがあれば
  OpenOCD で ELF を直接書き込んで起動できる（`program asp verify reset exit`）。
- **OpenOCD**: upstream 0.12.0 は RP2350 未対応（`rp2350.cfg` もフラッシュドライバも
  無い）。**Raspberry Pi フォーク**（`github.com/raspberrypi/openocd`, 0.12.0+dev）を
  ビルドして使用する。Debugprobe(2e8a:000c) 用の udev ルールも必要
  （openocd 同梱の `contrib/60-openocd.rules` には Debugprobe のエントリが**無い**）。
- **周辺リセットの除外対象**: 一括リセット時に **PLL_SYS / PADS_QSPI / IO_QSPI を
  除外**する（XIP 実行中のフラッシュアクセスを壊さないため。RP2040 版と同じ）。

## デバッグ手法（今回有効だったもの）

- **openocd で halt → `xPSR`/`CFSR(0xE000ED28)`/`HFSR` を読む**: 例外番号と
  フォールト種別（今回 UFSR の INVPC）が一発で分かる。PC を `addr2line -f -e asp`
  に通すと停止位置のシンボル（今回 `target_exit` = デフォルトハンドラ経由の
  カーネル終了と判明）。
- **周辺レジスタの実機ダンプで段階切り分け**: UART 無出力時に
  UART0_CR/FR/IBRD・GPIO_CTRL・PADS・CLK_PERI_CTRL をダンプ →
  「FR=TX FIFO empty（送信済み）」からチップ側は正常・**配線問題**と確定できた。
  openocd の複数 `mdw` は最後の 1 つしか stdout に出ないことがあるため，
  tcl スクリプトで `echo "[capture {mdw 0x...}]"` とすると確実。
- **静的検証**: `objdump -s -j .text asp` で IMAGE_DEF ブロックのマーカ
  （`d3deffff`/`793512ab`）の有無・位置を確認。`nm -n` でベクタテーブル位置を確認。

## メモリマップ（現状）
- FLASH(XIP) 0x10000000（4MB）: `.vector` → `.picobin_block` → `.text`/`.rodata` →
  `.data` 初期値
- RAM 0x20000000（512KB）: `.data`（start.S がコピー）/ `.bss` / スタック
- VTOR は `core_initialize()` が `_kernel_vector_table`（=0x10000000）に設定

## 検証（2026-06-04）
- gcc 14.3（arm-none-eabi）で警告なくビルド（sample1, TECS, cfg チェックパス）。
- 実機（Pico 2 + Debugprobe）で sample1 のバナー・task1 周期出力・キー入力
  （`2`/`r` による rot_rdq・タスク切替）・リセット後の再起動を確認。
- `make run` / `make swd-debug`（reset init → load → halt 待機）/ `make console` の
  デバッグ支援ターゲットも実機で動作確認。

## ランタイムテストの実施（2026-06-05）

`test/testexec.rb` により，標準のランタイムテストを実機で実施した．

### テスト環境（`<ASP3>/TEST-EXEC/`）

TARGET_OPTIONS:
```
-T raspberrypi_pico2_gcc -V /usr/local/tools/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin
```

TARGET_RUN（要点）: UART キャプチャを**書き込みより先に**開始し（起動直後のバナー取り
こぼし防止），OpenOCD で書き込み後，完了メッセージ（`All check points passed.` 等）か
エラー（`Unregistered Exception` 等）を検出するまで待つ．

> ⚠️ テストバッチを**並行実行してはならない**（ボードと /dev/ttyACM0 を共有するため，
> 別テストの出力を拾って誤判定する）．必ず 1 バッチずつ逐次実行する．

### 結果（36 テスト中 34 PASS）

| テスト | 結果 |
|---|---|
| task1 / sem1 / sem2 / flg1 / dtq1 / pdq1 / mpf1 | PASS |
| mutex1〜mutex8 | PASS（8件） |
| int1 / hrt1 / notify1 / suspend1 / sysman1 / sysstat1 / tmevt1 | PASS |
| raster1 / raster2 / exttsk | PASS |
| dlynse | PASS（パラメータ調整後，17 測定全て OK） |
| cpuexc2 / cpuexc3 / cpuexc5〜cpuexc10 | PASS（8件） |
| **cpuexc1 / cpuexc4** | **FAIL（既知の制限・後述）** |

### テストにより発見・修正した問題

1. **`-fdata-sections` でコンフィギュレータのパス3が E_SYS**（notify1 で発覚）
   kernel_cfg.c が生成する通知ハンドラの `intptr_t *const ..._p_evar` が定数伝播で
   未参照となり `--gc-sections` に回収され，パス3のシンボル値検査が
   `E_SYS: symbol not found` で失敗する．→ `Makefile.target` から `-fdata-sections` を
   削除（`-ffunction-sections` + `--gc-sections` は維持）．

2. **`SIL_DLY_TIM1/2` が実機と不一致**（dlynse で発覚）
   exshonda 版から流用した 79/50 では全測定が遅延不足（実測が要求の約 2/3）．
   dlynse の "for fitting parameters" 出力から実コストを逆算：初回 46ns・
   ループ毎 33.3ns（=150MHz で 5 サイクル）．→ `rpi_pico.h` を **46/33** に修正し，
   全 17 測定 OK（遅延 ≧ 要求）を確認．

3. **int1 用の割込み定義が未整備**
   `target_test.h` に `INTNO1`（TIMER0 ALARM1 = IRQ1+16=17，カーネルタイマと別の
   ALARM を流用），`INTNO1_INTATR`/`INTNO1_INTPRI`，および割込みフラグクリア用の
   `intno1_clear()`（`TIMER0_INTR` の ALARM_1 ビットをクリア）を追加．

### 既知の制限: cpuexc1 / cpuexc4（arm_m 共通・修正不能）

**割込みロック状態（`SIL_LOC_INT()` = PRIMASK=1）で発生させた CPU 例外**のテスト．
ARMv7-M/v8-M では PRIMASK=1 中は構成可能優先度の例外（UsageFault, 優先度 0）が
プリエンプトできず，**HardFault（例外 3）にエスカレートする**（アーキテクチャ仕様）．
テストハンドラは `CPUEXC1`=6（UsageFault，`core_test.h`）に登録されるため，
エスカレートした例外はデフォルトハンドラに落ち
`Unregistered Exception occurs. Excno = 00000003` で終了する．

- CPUロック状態（basepri）やロック解除状態の cpuexc2/3/5〜10 は UsageFault が
  正常配送されて PASS する．
- 本制限は **arm_m 依存部を使う全ての ARMv7-M/v8-M ターゲットに共通**で，本移植固有
  ではない．ベクタテーブル生成（core_kernel.trb）・テスト本体とも共通部のため，
  ターゲット依存部の範囲では修正できない（HardFault からの再ディスパッチを共通部に
  実装すれば解決可能）．

## 残課題 / 未検証
- **UART1**: cdl にプロトタイプはあるが GPIO ファンクション設定が未実装（未検証）。
- **SYSTICK タイマ方式**（`KERNEL_TIMER=SYSTICK`）: 未検証（既定は TIM=TIMER0 HRT）。
- **cpuexc1/cpuexc4**: PRIMASK 中の CPU 例外エスカレートによる FAIL（上記「既知の制限」参照）。
- **BOOTSEL/picotool での書き込み**: IMAGE_DEF があるので可能と考えられるが未検証。
- **Core1**: 未使用・未停止（bootROM 内で待機）。省電力化するなら PSM での停止を検討。
- **TrustZone の本格利用**（Secure/Non-secure 分割, `-mcmse` のコールゲート等）: 対象外。
  現状は「Secure 単独動作のための `ENABLE_TRUSTZONE=1`」のみ。
- ~~**TOPPERS_OMIT_TECS（-w）ビルド**: 非 TECS 用の `chip_serial.h`/`target_serial.c` を
  用意していないため不可（rp2040 版も同様）。~~
  → **解決済み（2026-06-05）**: asp3_core の TECSレス化に伴い，非 TECS 版 SIO ドライバ
  （`rp2350_uart.{c,h}`・`chip_serial.{c,h,cfg}`，上流 `extension/non_tecs` の
  uart_pl011／rza1 chip_serial 構成を規範）を追加。configure.rb は非 TECS が
  デフォルトとなった（TECS 構成は `OMIT_TECS=` 空指定でビルド可能）。
