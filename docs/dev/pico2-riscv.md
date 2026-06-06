# RISC-V Hazard3ターゲット（pico2-riscv）

## 項目

RISC-V Hazard3ターゲット（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

Raspberry Pi PICO2（RP2350）の **RISC-Vコア（Hazard3×2・RV32IMAC系）** 向け
ターゲットを、**SDK非依存のベアメタル移植**として追加する
（ARM版 `target/raspberrypi_pico2_gcc` と同じ方針・同じボード）。

### 意義（何が嬉しいか）

1. **1ボードで2 ISA**：PICO2はARM（Cortex-M33）とRISC-V（Hazard3）を
   切替可能。本対応でAGENTSの対象ターゲット表（ARMv8-M/ARMv8-A/RISC-V）が
   実機レベルで完成する。
2. **RV32の実証**：`arch/riscv_gcc/common`（PolarFire＝RV64GCで検証済み）の
   **XLEN抽象が実機RV32で通る**ことを実証。RISC-V系列の適用範囲が広がる。
3. **PolarFire移植の知見が新鮮なうち**に、riscv系2例目で移植パターンを固める
   （他のTOPPERS系RTOSへの横展開資料としての価値）。

### 検証済みの好材料（2026-06-06・開発機で確認）

| 項目 | 結果 |
|---|---|
| ツールチェーン | apt の `gcc-riscv64-unknown-elf`（13.2）に **rv32imac/ilp32 マルチリブあり**（`-print-multi-lib`で確認）→ **PolarFireと同一パッケージで足りる** |
| `arch/riscv_gcc/common` のRV32対応 | **XLEN抽象済み**（`core_asm.inc` の `lx`/`sx` が `__riscv_xlen` で lw/sw に切替・start.S/core_support.S に ld/sd 直書きゼロ） |
| FPU | start.S の FPU初期化は **`#ifdef __riscv_flen` ガード済み**（Hazard3はFPU無し→RV32IMACで素通り） |
| 流用資産（ARM版pico2） | `RP2350.h`（**IRQ番号はARM/RISC-Vで共通**）・`rp2350_uart.[ch]`（MMIO・ISA非依存）・`target_kernel_impl.c` の `hardware_init_hook`（TICKS/PADS ISO/UARTクロック＝MMIO）・TIMER0 HRTロジック・`rpi_pico2.ld`（メモリマップ）・swd-debug.gdb／run.cmake の構成 |

### ARM版との主要差分（新規に作るもの）

| 項目 | 内容 |
|---|---|
| **割込みコントローラ** | NVICではなく **Hazard3のXh3irq**（カスタムCSR：`meiea`(0xbe0)/`meipa`(0xbe1)/`meifa`(0xbe2)/`meipra`(0xbe3)/`meinext`(0xbe4)/`meicontext`(0xbe5)・16bitウィンドウ方式・優先度16段階）。**PLICではない**ため `plic_kernel_impl` は使えず、`xh3irq_kernel_impl`（chip層）を新規実装。外部割込みはmcause=11で受け、`meinext` でディスパッチ（PolarFireのchip_support.S＝PLIC claim/completeパターンの置換に相当） |
| **ブート** | `image_def.S` の IMAGE_DEF ブロックを **RISC-V EXE タイプ**に（ARM版はARM Secure）。RISC-Vイメージではベクタテーブルでなく**エントリポイント**から実行（bootROMがARCHSELを自動切替） |
| **トラップ入口** | mtvec＋`arch/riscv_gcc/common` のトラップ枠組み（PolarFireで検証済み）。タイマはMTI（mip.MTIP）ではなく**TIMER0のIRQ（外部割込み）**を使う想定（ARM版とロジック共有のため） |
| **ブートハート** | `TOPPERS_BOOT_HARTID=0`（PolarFireは1）。core1はbootROMが待機させる（ARM版と同じ） |

## 実施プラン

> **本項目は実機接続PC（別セッション）での実施を前提**とする。
> QEMUにRP2350/Hazard3のモデルは無く、検証は実機のみ。
> 以下に環境構築含め必要な情報をすべて記載する。

### 0. 環境構築（実機接続PC・**Ubuntu 24.04**）

対象OSは **Ubuntu 24.04**（ネイティブ実行。実機書込み＝USB/SWDのため
開発コンテナは使わない）。本計画の事前検証も Ubuntu 24.04 の apt
パッケージで実施済みであり、**以下のバージョンで動作確認済みの構成**となる：
gcc-riscv64-unknown-elf **13.2.0**（rv32imac/ilp32マルチリブ同梱）・
CMake 3.28・Ninja 1.11・Python 3.12。

ARM版pico2で構築済みの環境（Debugprobe配線・udev・OpenOCD RPiフォーク・
picocom）をそのまま使う。詳細手順は
**`target/raspberrypi_pico2_gcc/target_user.md` §1**（1.4=OpenOCD・1.5=配線/udev）。
追加で必要なのは以下のみ：

```bash
# RISC-Vツールチェーン（rv32マルチリブ同梱・PolarFireと同一パッケージ．24.04のaptで13.2.0）
sudo apt-get install -y gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf
riscv64-unknown-elf-gcc -print-multi-lib | grep rv32imac   # rv32imac/ilp32 があること

# OpenOCDのRISC-V対応確認（RPiフォークに同梱のはず）
ls /usr/local/share/openocd/scripts/target/ | grep rp2350   # rp2350-riscv.cfg があること
```

- リポジトリは main を pull（本計画と全依存ファイルが入っていること）
- 参照資料（実装時に必ず確認）：
  - **RP2350 Datasheet**（https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf）
    §3.8 Hazard3（Xh3irq CSRの正確なエンコーディング・ウィンドウ方式）、
    §5 bootROM（IMAGE_DEFのRISC-V EXEタイプ・エントリ規約）
  - **pico-sdk**（https://github.com/raspberrypi/pico-sdk・**参照のみ＝コードは持ち込まない**）：
    `src/rp2_common/hardware_irq/`（Xh3irqの使い方）・
    `src/rp2_common/pico_crt0/crt0_riscv.S`（ブート・IMAGE_DEF）・
    `src/common/boot_picobin_headers/include/boot/picobin.h`（IMAGE_DEFエンコード）
  - ARM版の経緯：`arch/arm_m_gcc/rp2350/PORTING.md`・
    `target/raspberrypi_pico2_gcc/target_user.md`（TICKS/PADS ISO等のハマりどころ）
  - PolarFire移植（riscv 1例目）：`docs/dev/qemu-target-riscv.md`

### 1. チップ依存部 `arch/riscv_gcc/rp2350/`（新規）

- `chip.cmake`：`-march=rv32imac_zicsr_zifencei -mabi=ilp32 -mcmodel=medany`・
  `-nostdlib`等（PolarFire chip.cmakeを規範に）。
  **ARM版rp2350の流用ファイルはパス参照**（コピーしない）：
  `ASP3_INCLUDE_DIRS` に `arch/arm_m_gcc/rp2350` を追加して `RP2350.h` を共有、
  `ASP3_SYSSVC_TARGET_C_FILES` に `arch/arm_m_gcc/rp2350/rp2350_uart.c` を指定
  （chip_serial.c は新規（PolarFire/mmuart版のuart呼出し置換）または
  ARM版 `chip_serial.c` の流用可否を実装時に判断）
- `xh3irq_kernel_impl.[ch]`＋`chip_support.S`：Xh3irq制御
  （disable/enable/probe/config_int＝meiea/meipra、割込み入口＝meinext読出し→
  `_kernel_inh_table[intno]`、優先度マスク＝meicontext．
  PolarFireの`chip_support.S`（PLIC claim/complete）の置換実装）
- `chip_kernel_impl.[ch]`・`chip_kernel.h`・`chip_kernel.py`
  （INTNO_VALID＝0..51・RP2350のIRQ本数）・`chip_timer.h`・
  `chip_stddef.h`・`chip_rename.def`（genrename）
- `TOPPERS_BOOT_HARTID=0`

### 2. ターゲット依存部 `target/raspberrypi_pico2_riscv_gcc/`（新規）

ARM版 `target/raspberrypi_pico2_gcc` をベースに：

- **そのまま流用（コピー・ISA非依存）**：`hardware_init_hook` の内容
  （RESETS/TICKS/PADS ISO/UARTクロック）・`target_timer.c`（TIMER0 HRT・
  `USE_TIM_AS_HRT`）・`target_syssvc.h`・`target_serial.cfg`・`rpi_pico.h`
- **変更**：`image_def.S`（**RISC-V EXEタイプ**のIMAGE_DEFブロック．picobin.h
  のエンコードを参照して作成）・`rpi_pico2.ld`（ENTRYと先頭配置をRISC-V流に：
  ベクタテーブルでなくエントリコード＋IMAGE_DEF）・`target_kernel_impl.h`
  （SIL_DLY_TIM＝実機で要較正．初期値はARM版の46/33から開始）
- `target.cmake`・`run.cmake`：ARM版を規範に。OpenOCDは
  `target/rp2350-riscv.cfg`（要確認②）。preset名は命名規則どおり
  **`raspberrypi_pico2_riscv`**（presets.json はARM版と同居＝
  `target/raspberrypi_pico2_gcc/presets.json` でなく新ターゲット側に作成し、
  ルートCMakePresets.jsonにinclude追記）。
  **予約プリセット `rp2350-riscv_pico_sdk` は削除**（SDK統合版は将来の
  SDK統合項目で別途。AGENTS §4・building.md の記述も更新）

### 3. ビルド確認（開発機でも可）

```bash
cmake --preset raspberrypi_pico2_riscv -B build/raspberrypi_pico2_riscv
cmake --build build/raspberrypi_pico2_riscv   # 警告ゼロ
# ELFのアーキ確認
riscv64-unknown-elf-objdump -f build/raspberrypi_pico2_riscv/asp.elf | grep riscv
```

### 4. 実機検証（実機接続PC）

```bash
ninja -C build/raspberrypi_pico2_riscv console   # 端末C
ninja -C build/raspberrypi_pico2_riscv run       # OpenOCD書込み→実行
```

1. sample1：バナー（`<RISC-V Hazard3>`等の表記）・task1周期実行・
   `a`/`r`でタスク切替・`z`（CPU例外）
2. testexec（ARM版実績＝36本中34本PASS）：
   `TARGET_OPTIONS=--preset raspberrypi_pico2_riscv`・TARGET_RUNは実機のため
   ARM版PORTING.mdのテスト環境の作り方を踏襲
3. `dlynse` で SIL_DLY_TIM を較正
4. OS Awareness：`target_os_awareness.py`（riscv common＝Xh3irq用に
   chip層を追加）→ `osdebug` 確認（任意）

### 5. 記録

- DIVERGENCE_MAP（arch/riscv_gcc/rp2350・target/raspberrypi_pico2_riscv_gcc＝NEW）
- IMPL_INDEX（Xh3irq・RISC-V IMAGE_DEF・RV32実証）
- building.md対応表・AGENTS §4コマンド例・README索引・本ファイル実施結果
- CI：build-onlyジョブ追加（ツールチェーンはpolarfireジョブと同一）

### 要確認事項（実装セッションで最初に潰す）

1. **IMAGE_DEFのRISC-V EXEエンコード**とエントリ規約（picobin.h／crt0_riscv.S／
   datasheet §5．bootROMがエントリへ直接ジャンプするか・スタック初期値の扱い）
2. **OpenOCDのRISC-V書込み**：`rp2350-riscv.cfg` の有無と
   `program asp.elf verify reset` の可否（ARMコア経由でのフラッシュ書込みに
   なる可能性あり．RPiフォークのREADME参照）
3. Xh3irqの**優先度とASP3のintpriモデルの対応**（meipra＝16段階．
   CPUロックはmstatus.MIEで単純化し、優先度マスクはmeicontext系を使うか
   実装時に判断＝PolarFireはPLIC閾値方式だった）
4. ARM版 `chip_serial.c`（arm_m_gcc/rp2350）がそのまま使えるか
   （ena_int/dis_int経由なら非依存のはず）

### スコープ外

- pico-sdk統合版（`rp2350-arm-s_pico_sdk`等）＝SDK統合項目
- core1（マルチコア）・TrustZone相当（Hazard3にはない）
- QEMU対応（Hazard3モデルが存在しない）

## 実施結果

（完了時に記載）
