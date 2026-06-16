# QEMU Cortex-M4 ターゲット（mps2-an386 / ARMv7-M テスト）

## 項目

QEMU 用 Cortex-M4（ARMv7-M）ターゲット `mps2-an386` を新設する。
AGENTS.md §1「機能追加計画」の「QEMUターゲット(ARMv7-M)」。

目的：これまでの QEMU M-profile 検証は ARMv8-M（Cortex-M33＝`mps2-an505`）のみだった。
**ARMv7-M（非TrustZone・FPv4-SP FPU）経路** を QEMU で常時検証できるようにし、
`arch/arm_m_gcc/common` の `__TARGET_ARCH_THUMB == 4` 系パスの回帰を担保する。

## 内容

### 対象 QEMU マシン

`mps2-an386`（`qemu-system-arm -M mps2-an386`）。QEMU の **`hw/arm/mps2.c`**
（レガシー CMSDK 系。`mps2-an505`/`an521` の `hw/arm/mps2-tz.c`＝IoTKit/SSE とは
別系統）でモデル化される **Cortex-M4・非 TrustZone** ボード。

### アーキ対応は既存（作業はボード依存部が主体）

`arch/arm_m_gcc/common` は上流 TOPPERS ARM-M 由来で `__TARGET_ARCH_THUMB` により
アーキ版数を分岐する多アーキ設計：
- `== 3` → ARMv6-M、`>= 4` → ARMv7-M、`>= 5` → ARMv8-M。
- PSPLIM・TrustZone 例外復帰ビット等は `#if __TARGET_ARCH_THUMB >= 5` で囲われ、
  v7-M では自動無効化される。

したがって **arch 依存部の変更は原則不要**。作業の主体はボード依存部
（メモリマップ・CMSDK UART 番地・割込み番号・クロック）と `target.cmake`。

### QEMU から確定したボードパラメータ（`hw/arm/mps2.c`）

| 項目 | 値 | 出典 |
|---|---|---|
| CPU | Cortex-M4（ARMv7-M、非TZ） | `mps2_an386_class_init`：`default_cpu_type = cortex-m4` |
| SYSCLK | 25 MHz | `#define SYSCLK_FRQ 25000000` |
| 割込み数 | `num-irq = 32` | `case FPGA_AN386` |
| コード/ベクタ | SSRAM1 `0x00000000`（4MB、`0x00400000` にミラー） | `make_ram(ssram1, 0x0, 0x400000)` |
| データ RAM | SSRAM2&3 `0x20000000`（4MB） | `make_ram(ssram23, 0x20000000, 0x400000)` |
| リセットベクタ | `0x00000000`（VTOR リセット値 0） | ARMv7-M 既定 |
| UART | CMSDK-APB-UART `0x40004000`(UART0)/`0x40005000`(UART1)… | `uartbase[] = {0x40004000, ...}` |
| UART IRQ | `uartirq[i] + 1`（TX/RX 個別） | `sysbus_connect_irq(..., uartirq[i] + 1)`（実装時に正確な番号を確定） |

`mps2-an505`（IoTKit）との違い：**UART 番地が `0x40004000`（セキュアエイリアス
`0x40200000` ではない）／`num-irq=32`（96 ではない）／TrustZone なし**。よって
an505 のクローンにはならず、ボード依存部は新規に起こす（ただし下記は流用可）。

## 実施プラン

> 着手前。手順は `docs/porting/PORTING_GUIDE.md` の Step 順に従う。

1. **target ディレクトリ新設** `target/mps2_an386_gcc/`。`mps2_an505_gcc` を雛形に、
   ボード固有部のみ差し替える：
   - `mps2_an386.h`：CPU クロック 25MHz、UART ベース `0x40004000`、UART IRQ 番号、
     `TBITW_IPRI`（an386 の NVIC 優先度ビット幅を QEMU/実機 AN386 から確認）。
   - `mps2_an386.ld`：FLASH=SSRAM1 `0x00000000`（4MB）、RAM=SSRAM2&3 `0x20000000`（4MB）。
     init-svtor 相当はリセットベクタ `0x00000000`。
   - 流用：`cmsdk_uart.{c,h}`（同 IP・番地のみ変更）／`target_timer.c`（SysTick は
     M-profile 共通）／`target_serial.*`／arch 共通部。
2. **`target.cmake`**：
   - `-mcpu=cortex-m4 -mthumb`、`__TARGET_ARCH_THUMB=4`、`TOPPERS_CORTEX_M4`。
   - **TrustZone 定義を付けない**（`TOPPERS_ENABLE_TRUSTZONE` 無し）。
   - FPU（FPv4-SP）：`-mfloat-abi=softfp -mfpu=fpv4-sp-d16` ＋
     `TOPPERS_FPU_ENABLE`/`TOPPERS_FPU_CONTEXT`/`TOPPERS_FPU_LAZYSTACKING`
     （an505 と同型。v7-M でも S16-S31 退避は同じ）。QEMU `cortex-m4` の FPU 有無を
     起動時に確認（無ければ soft-float にフォールバック）。
   - QEMU 実行コマンド：`qemu-system-arm -machine mps2-an386 …`。
3. **presets/配線**：`target/mps2_an386_gcc/presets.json`（`mps2_an386-qemu`）、
   ルート `CMakePresets.json` の include 追記、`.github/workflows/ci.yml`（M-profile
   build+testexec ジョブ追加）。
4. **TTSP3 資産**：`target/mps2_an386_gcc/ttsp3/`（M33 と同型の SKIP 規則。
   `RAISE_CPU_EXCEPTION=udf #0`）。
5. **検証**（`docs/building.md`・`§4` 順）：
   - build → QEMU sample 完走 → testexec（task/sem/flg/tmevt/hrt）→ TTSP3 staticAPI。
   - **FP 命令が ARMv7-M で実行されること**（FPv4-SP の `vldr/vstr`・コンテキスト退避）。
   - 既存 an505（v8-M）の回帰確認（arch 共通部を触る場合のみ）。

### リスク・確認事項

- **UART IRQ 番号の確定**：`hw/arm/mps2.c` の `uartirq[]`＋1 のオフセットを実装時に精査。
- **NVIC 優先度ビット幅**（`TBITW_IPRI`）：AN386 実機の実装ビット数を確認（QEMU は広めに
  実装するため上位ビット配置で吸収可）。
- **FPU の有無**：QEMU `cortex-m4` モデルが FPU を持つか。持たない場合は soft-float で
  立ち上げ、FP 検証は別途。

## 実施結果

（完了時に記載）

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
|  |  |

### 追加したファイル

### Git情報

- ベースコミット：（着手時に記載）
- 関連コミット範囲：
- ファイルリスト再現コマンド例：`git diff --stat <base> -- target/mps2_an386_gcc`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| QEMU (mps2-an386) build | ○/− |  |
| QEMU (mps2-an386) sample | ○/− |  |
| QEMU (mps2-an386) testexec | ○/− |  |
| TTSP3 staticAPI | ○/− |  |
| 既存 an505 回帰 | ○/− |  |

### DIVERGENCE_MAP との関連

- `target/mps2_an386_gcc/` は NEW（上流衝突なし）。arch 共通部に変更が及んだ場合は
  該当エントリを追記しリンクする。
- 関連：[`mps2-an505.md`](mps2-an505.md)（M33/v8-M の先行ターゲット）。
