# QEMUターゲット(ARMv8-A)

## 項目

QEMUターゲット(ARMv8-A)（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

ARMv8-A（AArch64）のQEMUターゲットを追加し、Cortex-A系のカーネル検証を
ハードなしで行えるようにする。

### 方針

- **QEMUマシンは `xlnx-zcu102`**（Xilinx ZynqMP / Quad Cortex-A53）を使用する。
  QEMU本体に対応が入っており（`hw/arm/xlnx-zcu102.c`）、GIC-400（GICv2）・
  Cadence UART・Generic Timer が実機相当にモデル化されている。
- **移植元はマルチコア版 TOPPERS/FMP3 の ZynqMP 移植**：
  - 公式パッケージ：`fmp3_zcu102_arm64_gcc-20230914.zip`
    （`target/zcu102_arm64_gcc` + `arch/arm64_gcc/zynqmp`）
  - 本開発機に同系の FMP3 3.3 arm64 ツリーあり：
    `/home/honda/cadence/XtensaDualOS/ARM/fmp3_3.3/`
    （`arch/arm64_gcc/zynqmp` + `target/kr260_arm64_gcc`。KR260 も同じ ZynqMP）
- **FMP3→ASP3 の変換規則は確立済み**：STM32MP2 移植
  （`arch/arm64_gcc/stm32mp2/PORTING_ASP3_STM32MP2.md`）で抽出・実機検証済みの
  規則を再適用する。コア依存部 `arch/arm64_gcc/common/` は**変更不要**（検証済み）。

### ZynqMP の主要ハードウェア資源（FMP3 zynqmp.h より）

| 資源 | 値 |
|---|---|
| GIC-400 (GICv2) | GICD=0xF9010000, GICC=0xF9020000, 割込み352本 |
| Cadence UART | UART0=0xFF000000 (IRQ 53), UART1=0xFF010000 (IRQ 54) |
| Generic Timer | CNTPS INTID 29（stm32mp2 と同一）。CNTFRQ 実行時読出し |
| DDR (QEMU) | 0x00000000〜（低位2GB） |

## 実施プラン

1. **チップ依存部 `arch/arm64_gcc/zynqmp/`**（FMP3 zynqmp からASP3変換）
   - `zynqmp.h`：定数そのまま（マルチコア起動レジスタ定義は残置可）
   - `chip_kernel_impl.c`：`chip_mprc_initialize`（セカンダリコア起動）削除→
     `chip_initialize(void)` 化。`chip_el3_initialize` の
     System Timestamp Generator 初期化は **QEMUでは未実装MMIOの恐れがあるため
     `TOPPERS_USE_QEMU` でガード**（CNTFRQはQEMUが設定済みの値を実行時読出し）
   - `chip_kernel_impl.h`・`chip_kernel.h`・`chip_kernel.py`（INTNO_VALID=0..351）・
     `chip_timer.h`・`chip_stddef.h`・`chip_rename.def`
   - SIOドライバ：`xuartps.[ch]`（FMP3 arm64版＝Cadence UART）＋
     `chip_serial.[ch]`（非TECS、rp2350 の chip_serial 方式）
   - `chip.cmake`
2. **ターゲット依存部 `target/zcu102_arm64_gcc/`**（kr260 + stm32mp257f_dk からASP3形）
   - `zcu102.h`（クロック・ボーレート。Xtensaワールド間割込み定義は持ち込まない）
   - `zcu102.ld`（DDR 0x00000000 配置）・`target_mmu_config.h`（DDR＋デバイス領域）
   - `target_kernel_impl.c/h`（EL3/EL2フック→chip委譲、`target_exit` は
     セミホスティングでQEMU終了）
   - `target_kernel.h/.cfg/.py`・`target_check.py`・`target_serial.cfg/h`・
     `target_syssvc.h`・`target_timer.cfg/h`・`target_stddef.h`・`target_sil.h`・
     `target_test.h`・`target_rename.def`・`target.cmake`
3. **CMakePresets** に `a64-qemu` を追加（toolchain-a35.cmake 流用。
   QEMU実行は `-M xlnx-zcu102,secure=on -semihosting`）
4. **検証**：ビルド（aarch64クロス）→ QEMUで sample1 バナー・タスク切替 →
   testexec スポット（sem1等）
5. **記録**：DIVERGENCE_MAP・docs/dev/README・本ファイル実施結果

### リスク

- QEMU `-kernel`（ELF）起動時のEL：`secure=on` でEL3開始（実機FSBL相当）。
  EL3→セキュアEL1ドロップは start.S の既存処理がそのまま使える見込み
- QEMUの CNTFRQ は実行時読出しで吸収（stm32mp2 と同方式）
- 実機ZCU102は当面スコープ外（QEMU専用として整備、実機は将来）

## 実施結果

（2026-06-06 記載。QEMU検証まで完了）

### 追加したファイル

**チップ依存部 `arch/arm64_gcc/zynqmp/`**（17ファイル）

| ファイル | 由来・内容 |
|---|---|
| `zynqmp.h` | FMP3そのまま（GIC=0xF9010000/0xF9020000・352本・UART・STG・APU起動レジスタ定義） |
| `xuartps.[ch]`・`chip_serial.[ch]` | FMP3そのまま（**FMP3 arm64版は元から非TECS**．Cadence UARTドライバ） |
| `chip_serial.cfg` | FMP3から `CLASS(CLS_SERIAL)` を解除 |
| `chip_kernel_impl.c` | ASP3形に変換：`chip_mprc_initialize`（セカンダリコア起動）削除→`chip_initialize(void)`化。STG初期化＋CNTFRQ設定は **`TOPPERS_USE_QEMU` 時スキップ**（QEMUは0xFF260000未実装・CNTFRQは実行時読出しで吸収） |
| `chip_kernel_impl.h`・`chip_kernel.h`・`chip_timer.h/.cfg`・`chip_stddef.h`・`chip_rename.def/.h`・`chip_unrename.h` | stm32mp2/FMP3から流用（リネームはgenrename生成） |
| `chip_kernel.py` | INTNO_VALID＝0..351 |
| `chip.cmake` | cortex-a53・`GIC_NO_FIQ_IN_SECURE`・`-nostdlib -static -Wl,--no-dynamic-linker`・`-fno-stack-protector -fno-pic`・**libc非リンク**（glibc系ツールチェーン対策） |
| `libc_stub.c` | 新規：memcpy/memset/memmove/memcmp のweak実装（libc非リンクのため） |

**ターゲット依存部 `target/zcu102_arm64_gcc/`**（22ファイル）

| ファイル | 由来・内容 |
|---|---|
| `zcu102.h` | DDR_ADDR=0x0・**DDR_SIZE=128MB**（QEMU既定）・UARTボーレート定数 |
| `zcu102.ld` | stm32mp257f_dk.ld と同一（配置は `-Ttext/-Tdata` で指定） |
| `target_kernel_impl.c` | MMUテーブル＝DDR(0x0,128MB)＋デバイス(0xF9000000,112MB)。低レベル出力はxuartps。**`target_exit` はセミホスティング SYS_EXIT でQEMU終了** |
| `target_syssvc.h` | UART0（0xFF000000, IRQ53）＝QEMUコンソール |
| `target.cmake` | TEXT=0x00100000／DATA=0x04000000・`TOPPERS_TZ_S`・**`ZCU102_QEMU` オプション（既定ON）**・RUN_COMMAND=`qemu-system-aarch64 -M xlnx-zcu102,secure=on -semihosting...`（QEMU時のみ） |
| その他（kernel.h/cfg/py・check.py・timer・serial・test・stddef・sil・rename等） | stm32mp257f_dk／kr260から流用 |

**その他**：`CMakePresets.json` に `a64-qemu`／`run-a64-qemu` を追加

### 設計のポイント・ハマりどころ

1. **QEMUは `secure=on` でEL3開始**（実機FSBL相当）→ start.S の既存
   EL3→セキュアEL1ドロップ（TOPPERS_TZ_S）がそのまま動作。
2. **QEMU xlnx-zcu102 の既定DDRは128MB**（0x0–0x07FFFFFF）。当初
   DATA=0x10000000 に置いたところRAM外で，スタックへのstr/ldrが消失
   →`SCR_EL3` 書き戻しが0化→eret不正（Illegal Execution state, 0x200ループ）。
   `info mtree` でRAMサイズを確認し128MB内に再配置して解決。
3. **glibc系ツールチェーン（aarch64-linux-gnu）でのベアメタルリンク**：
   `-nostdlib -static -Wl,--no-dynamic-linker` ＋ `-fno-stack-protector` が必要。
   libcを外しmemcpy等は `libc_stub.c` で供給。
   aarch64-none-elf 環境ではプリセットそのまま，無い環境は
   `-DA35_TOOLCHAIN_PREFIX=aarch64-linux-gnu-` を付与。
4. コア依存部 `arch/arm64_gcc/common/` は**無変更**（stm32mp2移植の検証済み資産）。

### QEMU／実機の切り替え（条件コンパイル）

CMakeオプション `ZCU102_QEMU`（既定ON）で `TOPPERS_USE_QEMU` の定義を切り替える。
実機ZCU102向けは `-DZCU102_QEMU=OFF` でconfigureする：

| 切替箇所 | QEMU（既定） | 実機（`-DZCU102_QEMU=OFF`） |
|---|---|---|
| STG初期化＋CNTFRQ設定（`chip_kernel_impl.c`） | スキップ（QEMUは0xFF260000未実装） | 実行（FMP3と同一） |
| `target_exit`（`target_kernel_impl.c`） | セミホスティングSYS_EXITでQEMU終了 | 無限ループ |
| DDRマップ（`zcu102.h`） | 128MB（QEMU既定RAM） | 低位2GB |
| `run` ターゲット | QEMU起動 | 定義なし（書込み手段は実機検証時に整備） |

両構成のビルドとバイナリ差分（STGコード有無・hlt #0xf000 有無）を確認済み。
**実機での動作確認は未実施**（FSBLからのロード・JTAG等の実行手段とあわせて今後）。

### Git情報

- ベースコミット：`7e0c457`
- ファイルリスト再現：`git diff --stat 7e0c457 HEAD -- arch/arm64_gcc/zynqmp target/zcu102_arm64_gcc CMakePresets.json`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| ビルド（aarch64-linux-gnu 13.3） | ○ | 警告はリンカRWXのみ（無害） |
| QEMU sample1 | ○ | バナー・logging task・task1周期実行・`a`/`r`入力で task2/3 切替 |
| testexec（QEMU） | ○ | **task1/sem1/flg1/dtq1/mutex1/tmevt1 All check points passed・hrt1 正常完了（カウンタ逆行なし・cyclic 203回）** |
| `ext_ker` でのQEMU自動終了 | ○ | セミホスティング SYS_EXIT 動作（testexecが自動進行） |
| 実機 ZCU102 | − | ビルドのみ確認（`-DZCU102_QEMU=OFF`．実行は将来・上記切り替え表参照） |

### DIVERGENCE_MAP との関連

- `arch/arm64_gcc/zynqmp/`・`target/zcu102_arm64_gcc/` を新規追加行として記載
  （上流ASP3に存在せず衝突なし．移植元はFMP3）
