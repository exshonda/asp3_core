# QEMUターゲット(RISC-V)

## 項目

QEMUターゲット(RISC-V)（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

RISC-V（RV64GC）のQEMUターゲットを追加し、RISC-V系のカーネル検証を
ハードなしで行えるようにする。**asp3_core 初の RISC-V アーキテクチャ対応**
（`arch/riscv_gcc/` は新規系列）。

### 方針

- **移植元はマルチコア版 TOPPERS/FMP3 の PolarFire SoC 移植**：
  `fmp3_polafire_soc_kit_gcc-20241224.zip`（fmp3_3.3ベース．
  **開発機に展開済み：`/home/honda/TOPPERS/fmp3_pfsoc/fmp3_3.3/`**）
  - `arch/riscv_gcc/common/`（コア依存部：start.S・core_support.S・PLIC・mtimer）
  - `arch/riscv_gcc/polarfire_soc/`（チップ依存部：MMUART・chip_serial＝**元から非TECS**）
  - `target/polarfire_soc_kit_gcc/`（Microchip PolarFire SoC Icicle Kit）
- **QEMUマシンは `microchip-icicle-kit`**（QEMU 11.0で確認．開発機にriscv64版を
  ローカルビルド済み：`~/qemu/qemu-11.0.0/build-riscv/qemu-system-riscv64`）
- FMP3→ASP3変換は STM32MP2/ZCU102 で確立した規則
  （`arch/arm64_gcc/stm32mp2/PORTING_ASP3_STM32MP2.md`）を適用する。
  **ただし arm64 と異なりコア依存部の変換から行う**（riscv は初変換）。

### 検討結果（2026-06-06）

#### QEMU microchip-icicle-kit の適合性 → **サポート可能**

| 項目 | FMP3の前提 | QEMU 11.0 のモデル | 適合 |
|---|---|---|---|
| CLINT（MSIP/MTIMECMP/MTIME） | 0x02000000/0x02004000/0x0200BFF8 | 0x02000000（同配置） | ✓ |
| PLIC | 0x0C000000・182割込み | 0x0C000000 | ✓ |
| MMUART0（コンソール） | 0x20000000・INTNO 90 | 0x20000000 → serial0（stdio） | ✓ |
| mtime周波数 | 1MHz（µs直結） | `clint-timebase-freq` 既定 **1MHz** | ✓ |
| DDR | 0x80000000（低位1GB） | DRAM_LO 0x80000000+1GB | ✓ |
| L2-LIM | 0x08000000（FMP3実機はここで実行） | 0x08000000+32MB | ✓ |
| セミホスティング | −（テスト終了用に使用予定） | RISC-V semihosting対応 | ✓ |

#### ブートフロー（QEMUソース hw/riscv/microchip_pfsoc.c で確認）

- `-bios none -kernel asp.elf` で起動可能：QEMUがeNVM（0x20220000）に
  **リセットベクタROMを生成し，全ハートがELFエントリへジャンプ**する。
- ハート構成は **E51×1（hart0・RV64IMAC）＋ U54×4（hart1-4・RV64GC）固定**。
  FMP3実機ではSDK(HSS)が e51()/u54_*() に振り分けるが（sdk_entry.c），
  QEMUでは**全5ハートが同一エントリに来る**。
- **⚠️ E51はFPU無し**：FMP3のstart.SはFPU初期化（fscsr）をハート判定より
  前に行うため，E51が実行すると不正命令例外になる。ASP3版start.Sでは
  **mhartidによるパーキング（hart1以外をwfiループへ）を最初に行う**こと。

#### FMP3→ASP3 変換規模（約3,500行＋trb変換）

| 層 | 主なファイル | 変換内容 |
|---|---|---|
| core（新規変換） | start.S(228)・core_support.S(1263)・core_kernel_impl.c(244)・plic_kernel_impl.c(166)・mtimer.c(109) | TPCB/my_pcb廃止・IPI/migrate削除・`msi_ipi.c`削除・3.7.2 TMAX_*規約・`sil_get_pid`削除（mtimer.cのprcid引数除去）・ハートパーキング追加 |
| 生成テンプレート | core_kernel.trb(209)・core_check.trb(92)・core_offset.trb(33)・plic_kernel.trb(79)・chip/target trb | **Ruby→Python変換**（arm64の`*.py`を規範に） |
| chip | chip_kernel_impl.c(119)・chip_serial.c(167)・mmuart.c(347)・chip_support.S(345) | ほぼ流用（chip_serialは元から非TECS）．CLASS解除 |
| target | target_kernel_impl.c等 | ASP3形へ（zcu102の構成を規範に）．**sdk/（3.8MB・HSS/HAL）は持ち込まない**（QEMUでは不要．実機SoftConsole資材は実機対応時に判断） |

#### 環境

- RISC-Vツールチェーン：開発機に未導入。**`gcc-riscv64-unknown-elf`（apt）の
  インストールが必要**（要sudo）
- QEMU：`qemu-system-riscv64` 未導入のため QEMU 11.0.0 ソースから
  riscv64-softmmu をローカルビルド済み（上記パス）

#### 工数見積

ARMv8-A（ZCU102）は検証済みコア層の流用で約1時間だったのに対し，
RISC-Vは**コア層のFMP3→ASP3変換から**のため1〜2日規模。
変換規則自体は確立済みで，リスクは初適用のデバッグ
（コンテキストスイッチ・PLIC優先度マスク・例外入口）。

## 実施プラン

> 再開時の参照物：移植元＝`/home/honda/TOPPERS/fmp3_pfsoc/fmp3_3.3/`，
> QEMU＝`/home/honda/qemu/qemu-11.0.0/build-riscv/qemu-system-riscv64`，
> 変換規則＝`arch/arm64_gcc/stm32mp2/PORTING_ASP3_STM32MP2.md`，
> ターゲット構成の規範＝`arch/arm64_gcc/zynqmp/`＋`target/zcu102_arm64_gcc/`
> （chip.cmake・presets.json・QEMU/実機切替オプションの形）

1. **環境**：`sudo apt-get install -y gcc-riscv64-unknown-elf`（ユーザー実施）
2. **コア依存部 `arch/riscv_gcc/common/`**：FMP3→ASP3変換
   - start.S：ハートパーキング（mhartid≠1をwfi）→FPU初期化の順に修正．
     マスタ同期（start_sync/slave_wait）削除
   - core_support.S：my_pcb/PCBオフセット→グローバル直接参照，
     dispatch_and_migrate/IPI分岐削除，ASP3流 start_dispatch
   - core_kernel_impl.c・plic_kernel_impl.c：単一ハート化（テーブル単一化）
   - mtimer.c：prcid引数・sil_get_pid除去（MTIMECMPはhart1固定）
   - msi_ipi.[ch] は持ち込まない
   - core_*.trb・plic_kernel.trb → .py（arm64版.pyを規範に）
3. **チップ依存部 `arch/riscv_gcc/polarfire_soc/`**：流用＋ASP3形
   （chip_serial.cfg の CLASS解除・chip.cmake 新規・rv64gc/lp64d/medany）
4. **ターゲット依存部 `target/polarfire_soc_kit_gcc/`**：zcu102構成を規範に作成
   （DDR 0x80000000リンク・target_exit=セミホスティング（QEMU時）・
   POLARFIRE_QEMU オプションでQEMU/実機切替・presets.json：
   `polarfire_soc_kit`／`polarfire_soc_kit-qemu`）
5. **検証**：sample1（バナー・タスク切替）→ testexec（sem1等）
6. **記録**：DIVERGENCE_MAP・README索引・本ファイル実施結果

## 実施結果

（完了時に記載）
