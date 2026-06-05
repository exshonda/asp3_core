# IMPL_INDEX.md — 実装参照インデックス

> 新ターゲット移植時に「この機能の実装はどのターゲットを参照すればよいか」の索引。  
> `PORTING_GUIDE.md` の各Step実施時に併せて参照する。  
> **新ターゲットで新たな実装パターンを確立したら、この表に追記すること。**

---

## アーキテクチャ依存部（arch/）

| 機能 | 参照arch | ファイル | 備考 |
|---|---|---|---|
| Cortex-M コンテキストスイッチ | `arm_m_gcc/common` | `core_support.S` | PendSV使用 |
| Cortex-M 例外/割込みエントリ | `arm_m_gcc/common` | `core_support.S`, `core_kernel.h` | NVIC |
| Cortex-A コンテキストスイッチ | `arm64_gcc/common` | `core_support.S` | — |
| Cortex-A 例外ベクタ・EL設定 | `arm64_gcc/common` | `core_support.S` | EL1/EL3初期化 |
| RISC-V コンテキストスイッチ | `riscv_gcc/common` | `core_support.S` | mret/csr |
| RISC-V トラップエントリ | `riscv_gcc/common` | `core_support.S` | mcause判定 |

---

## ターゲット依存部（target/）

| 実装したいもの | 参照ターゲット | ファイル | パターン |
|---|---|---|---|
| SysTickによるHRTタイマ | `mps2_an521_gcc` | `target_timer.c` | SysTickダウンカウンタ |
| NVIC割込みプライオリティ設定 | `mps2_an521_gcc` | `target_config.h` | prio_bits考慮 |
| セミホスティングシリアル | `mps2_an521_gcc` | `target_serial.c` | bkpt #0xAB / SYS_WRITEC |
| QEMUクリーン終了 | `syssvc/qemu_exit.c` | — | SYS_EXIT (0x18) |
| Pico SDK統合タイマ | `rp2350-arm-s_pico_sdk` | `target_timer.c` | add_repeating_timer_us |
| Pico SDK UART | `rp2350-arm-s_pico_sdk` | `target_serial.c` | uart_putc_raw |
| Pico SDK ブート連携 | `rp2350-arm-s_pico_sdk` | `target_kernel.c` | stdio_init / Core1停止 |
| RP2350 RISC-V起動 | `rp2350-riscv_pico_sdk` | `target_kernel.c` | Hazard3固有 |
| GICv3初期化 | `stm32mp257f_dk_arm64_gcc` | `target_kernel.c` | GICD/GICR/ICC |
| Cortex-A35 MMU/キャッシュ初期化 | `stm32mp257f_dk_arm64_gcc` | `target_kernel.c` | EL1ページテーブル |
| Cortex-A35 アーキテクチャタイマ | `stm32mp257f_dk_arm64_gcc` | `target_timer.c` | CNTP_TVAL_EL0 |
| Cortex-A35 UART | `stm32mp257f_dk_arm64_gcc` | `target_serial.c` | USART |
| POSIXホストシミュレーション | `linux_gcc` | 全ファイル | signal/timerエミュレーション |
| ZynqMP（Cortex-A53）EL3初期化 | `arch/arm64_gcc/zynqmp` | `chip_kernel_impl.c` | SCR/SMPEN/STG（QEMUはSTGスキップ） |
| Cadence UART（ARM64） | `arch/arm64_gcc/zynqmp` | `xuartps.[ch]`・`chip_serial.c` | 非TECS SIO |
| QEMU AArch64ベアメタル実行 | `zcu102_arm64_gcc` | `target.cmake`・`target_kernel_impl.c` | xlnx-zcu102,secure=on／SYS_EXIT終了／glibc系ツールチェーン対策 |

---

## システムサービス（syssvc／TECSレス版）

| 機能 | ファイル | 備考 |
|---|---|---|
| シリアルドライバ | `syssvc/serial.c` | `target_serial.c` を呼ぶ |
| syslog（構造化ログ） | `syssvc/syslog.c` | `T=,EV=` 形式出力 |
| ログタスク | `syssvc/logtask.c` | — |
| バナー表示 | `syssvc/banner.c` | 起動時表示 |
| QEMU終了 | `syssvc/qemu_exit.c` | セミホスティングSYS_EXIT |

---

## タイマ実装パターンの選択指針

| 条件 | 推奨パターン | 参照 |
|---|---|---|
| Cortex-M でSysTickが空いている | SysTick直接制御 | `mps2_an521_gcc` |
| SDKがタイマAPIを提供（Pico等） | SDKラッパー経由 | `rp2350-arm-s_pico_sdk` |
| Cortex-A | アーキテクチャタイマ（CNTP） | `stm32mp257f_dk_arm64_gcc` |
| RISC-V | CLINT mtime/mtimecmp | `rp2350-riscv_pico_sdk` |

---

## シリアル実装パターンの選択指針

| 条件 | 推奨パターン | 参照 |
|---|---|---|
| QEMU・デバッガ接続あり | セミホスティング | `mps2_an521_gcc` |
| SDKがUART APIを提供 | SDKラッパー経由 | `rp2350-arm-s_pico_sdk` |
| メモリマップドUART直接 | レジスタポーリング | `stm32mp257f_dk_arm64_gcc` |
