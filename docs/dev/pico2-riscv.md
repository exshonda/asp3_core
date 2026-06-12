# RISC-V Hazard3ターゲット（pico2-riscv）

> **注（その後の改称）**：本書中の `raspberrypi_pico2_riscv_gcc` は現 `pico2_riscv_gcc`、
> `raspberrypi_pico2_gcc` は現 `pico2_arm_gcc`（プリセットは `pico2_riscv` / `pico2_arm`）。
> 以下のプラン・実施結果は当時の名称のまま（履歴記録）。

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

0. **test_porting（移植検証テスト・最初に実施）**：`docs/dev/porting-test.md` で
   整備済みの6項目TAP（`-DASP3_APPLDIR=test/porting -DASP3_APPLNAME=test_porting`
   でビルド）。**①syslog=ブート/UART・②tick=TICKS/TIMER0・⑥alarm=Xh3irq割込み経路**
   の切り分けに直結。6/6 passed を確認してから次へ
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

（2026-06-06・実機接続PC（Ubuntu 24.04・PICO2＋Debugprobe）で実施）

### 追加したファイル

- **`arch/riscv_gcc/rp2350/`（新規・13ファイル）**：Xh3irqチップ依存部
  - `xh3irq_kernel_impl.h`：Xh3irq制御（窓方式CSR・割込み許可/禁止/
    probe/強制・優先度設定・PREEMPT操作）
  - `chip_kernel_impl.[ch]`：チップ初期化（mtvec・Xh3irq・MEIEのみ許可）・
    `initialize_interrupt`・t_set_ipm/t_get_ipm・ras_int/clr_int（meifa）
  - `chip_support.S`：トラップベクタ・irc_begin_int（meinext UPDATE=1で
    claim）・irc_end_int（優先度スタックのソフトウェアpop）・irc_*_exc
  - `chip_kernel.h`（TMIN_INTPRI=-15）・`chip_asm.inc`・`chip_sil.h`
    （RP2350アトミックエイリアス）・`chip_stddef.h`・`chip_kernel.py`
    （INTNO_VALID=1〜52）・`chip_rename.def/.h`・`chip_unrename.h`・
    `chip.cmake`
- **`target/raspberrypi_pico2_riscv_gcc/`（新規・27ファイル）**：
  ARM版をベースに，`image_def.S`（RISC-V EXE 0x1101＋ENTRY_POINT item
  0x44）・`rpi_pico2.ld`（エントリコード先頭・sdata/gp配置）・
  `target_timer.[ch]`（TIMER0流用・割込み強制をNVIC ISPR→meifaに置換）・
  `target_kernel_impl.c`（hardware_init_hook流用・FPU部削除）・
  `run.cmake`（rp2350-riscv.cfg・gdb-multiarch）・`presets.json` ほか

### 変更したファイル（既存）

| ファイル | 内容 |
|---|---|
| `arch/riscv_gcc/common/core_kernel.h` | RV32対応：`TOPPERS_STK_T` をXLENで分岐（RV32は`__int128`非対応のためaligned(16)構造体） |
| `arch/riscv_gcc/common/arch.cmake` | PLIC/mtimerを `ASP3_RISCV_OMIT_PLIC_MTIMER` で除外可能に |
| `arch/riscv_gcc/common/core_rename.def/.h`・`core_unrename.h` | `target_hrt_*` のリネームを削除（ARM系の慣例に統一．kernel_cfg.cの参照と不整合になるため．polarfireはビルドで整合確認済み） |
| `arch/arm_m_gcc/rp2350/rp2350_uart.c`・`RP2350.h` | `cls_por`に送信FIFOドレイン待ち（FR.BUSY）を追加（終了時の末尾出力欠落の修正．ARM版共通） |
| `target/raspberrypi_pico2_gcc/presets.json`・`CMakePresets.json` | 予約プリセット`rp2350-riscv_pico_sdk`を削除し`raspberrypi_pico2_riscv`を追加 |

### 要確認事項の結果

1. **IMAGE_DEF**：RISC-V EXE＝IMAGE_TYPE 0x1101．既定エントリは
   イメージ先頭だが，pico-sdk同様 **ENTRY_POINT item（0x44・
   entry＋初期SP）** を付与（SPはstart.Sが直ちにistkptへ差替え）
2. **OpenOCD**：RPiフォークの `target/rp2350-riscv.cfg` で
   `program asp.elf verify reset exit` がそのまま動作（フラッシュ
   書込み・verify・リセットOK）
3. **優先度モデル**：meipra（4bit・16段階）に内部表現1〜15を対応付け，
   マスクはmeicontext.PREEMPT（=内部表現+1）．**重要な知見**：Hazard3は
   **MEIトラップ入口で優先度スタックをハードウェアがpush**
   （PREEMPT←受付け割込み優先度+1・PPREEMPT←旧PREEMPT・MRETEIRQ←1）し，
   mret時にMRETEIRQ=1ならpopする．ASP3はディスパッチでmretを通らず
   タスクへ戻る経路があるため，**popをソフトウェア（irc_end_int）に
   一本化**した（csrwで入口前の値を書き戻す方式は不可＝書き戻す値自体が
   push後の値になる．実機で全割込みマスク固着として顕在化）
4. **ARM版chip_serial.c**：そのまま流用可（ena_int/dis_int経由＝ISA非依存）

### テストにより発見・修正した問題

1. **`__idata_start = .;` 方式のROM化コピー元ずれ**（notify1で発覚）：
   `.data : ALIGN(8)` のLMAはアラインで繰り上がるため，.text終端を
   コピー元とすると4バイトずれて初期値付き変数（test_notify1の
   `count_variable = 1`等）が壊れる．→ `__idata_start = LOADADDR(.data)`
   に修正．あわせてリンカスクリプトをLINK_DEPENDSに登録
   （CMakeLists.txt．-Wl,-Tは依存にならず.ld編集が再リンクされないため）
2. **SIL_DLY_TIM較正**：上記dlynse較正の行を参照

### 検証結果（すべて実機）

| 項目 | 結果 |
|---|---|
| ビルド | 警告ゼロ・rv32 ELF・IMAGE_DEFは先頭4KB内（0x94） |
| test_porting | **6/6 passed**（修正済みカーネルで再確認込み） |
| sample1 | バナー`<RISC-V Hazard3>`・task1周期実行・`a`（act_tsk）・`r`×2（task1→2→3切替）・`3`+`z`（CPU例外→ハンドラ→復帰）すべてOK |
| dlynse較正 | SIL_DLY_TIM1/2＝**40/13**で全項目OK（NG=0）．呼出実測46ns・ループ実測はビルド（XIPフェッチ整列）で13.3〜20.5ns変動→理論下限（6cyc/2cyc @150MHz）基準で設定 |
| testexec全件 | **36/36 PASS**（cpuexc10はSKIP＝not necessary）．ARM版実績（34/36）で既知FAILだったcpuexc1/cpuexc4もRISC-Vでは**PASS**．ランナは`scripts/ci/run_board_pico2.sh`（UARTキャプチャ→OpenOCD書込み→完走マーカ待ち．ARM版と共用可） |
| OS Awareness | `chip_os_awareness.py`（Xh3irq）を実装し実機で確認（atask/stask/intr．窓方式CSR＝meiea/meipaはOpenOCDの`riscv exec_progbuf`で`csrrs s0,csr,s0`を1命令実行して読出し）．`--target osdebug`をrun.cmakeに追加 |
| シリアル | RX/TX割込み駆動で動作（logtask経由） |
| polarfire回帰 | ビルド・リンクOK（QEMU実行はCI＝ピン留めコンテナで確認） |

### Git情報

- ベースコミット：`7f8213b`
- ファイルリスト再現：
  `git diff --stat <base> main -- arch/riscv_gcc target/raspberrypi_pico2_riscv_gcc arch/arm_m_gcc/rp2350 CMakePresets.json target/raspberrypi_pico2_gcc/presets.json`

### 残課題

なし（testexec全件・dlynse較正・OS Awareness・polarfire QEMUのCI確認
まですべて完了．スコープ外＝SDK統合版・core1・QEMU対応は本ファイル
冒頭の「スコープ外」を参照）
