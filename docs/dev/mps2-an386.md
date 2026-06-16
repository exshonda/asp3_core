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

`mps2_an505_gcc` を雛形にボード固有部（メモリマップ・CMSDK UART 番地/IRQ・
クロック・CPU・FPU 種別）を全面的に差し替えて新設した．arch 共通部
（`arch/arm_m_gcc/common`）は無改変で，`__TARGET_ARCH_THUMB == 4`（ARMv7-M）
経路がそのまま動作することを確認した．

### 確定ボードパラメータ（`hw/arm/mps2.c`／QEMU 11.0.1 ソース）

| 項目 | AN386 値 | 根拠（行） | AN505 との差分 |
|---|---|---|---|
| CPU | cortex-m4（ARMv7-M・非TZ） | `mps2.c:501-512`（`mps2_an386_class_init`） | M33（ARMv8-M・TZ） |
| SYSCLK | 25 MHz | `mps2.c:104`（`SYSCLK_FRQ 25000000`） | 20 MHz |
| num-irq | 32 → `TMAX_INTNO=32+16=48` | `mps2.c:227-228`（`case FPGA_AN386`） | 92 → 124+16 |
| FLASH（ld ORIGIN/LENGTH） | `0x00000000` / 4MB（SSRAM1） | `mps2.c:205-208`（`make_ram(ssram1,0x0,0x400000)`） | `0x10000000`/4MB |
| RAM（ld ORIGIN/LENGTH） | `0x20000000` / 4MB（SSRAM2&3） | `mps2.c:209-211` | `0x38000000`/4MB |
| リセットベクタ | `0x00000000`（VTOR=0） | ARMv7-M 既定 | init-svtor=`0x10000000` |
| UART0 base | `0x40004000` | `mps2.c:290`（`uartbase[]`） | `0x40200000`（secエイリアス） |
| UART0 IRQ | RX=IRQ0→INTNO16／TX=IRQ1→INTNO17 | `mps2.c:294,308-309`（`uartirq[]={0,2,...}`，TX=`uartirq[i]+1`） | combined IRQ42→INTNO58 |
| TBITW_IPRI | 3（`__NVIC_PRIO_BITS`） | Cortex-M4 既定（QEMU は 8bit 実装・上位3bit に配置で吸収） | 3（同） |
| FPU | FPv4-SP-D16（単精度・16本） | `cpu-v7m.c:105-118`（`cortex_m4_initfn`：`mvfr0=0x10110021`，`mvfr1=0x11000011`） | FPv5-SP-D16 |

**AN505 との別系統点**：AN386 は `hw/arm/mps2.c`（レガシー CMSDK），AN505 は
`hw/arm/mps2-tz.c`（IoTKit/SSE）．UART に combined 割込みが無く TX/RX が個別
NVIC 線（→同一 ISR を両線に登録）／TrustZone なし（`TOPPERS_ENABLE_TRUSTZONE`
未定義）．

### 追加したファイル（`target/mps2_an386_gcc/` 一式）

ボード固有を書き換えたファイル：`mps2_an386.h`（旧 `mps2_an505.h`：UART base
`0x40004000`・RX/TX 個別 IRQ・`CPU_CLOCK_HZ=25000000`・`TMAX_INTNO=48`）／
`mps2_an386.ld`（旧 `mps2_an505.ld`：FLASH `0x00000000`・RAM `0x20000000`）／
`target.cmake`（`-mcpu=cortex-m4 -mfpu=fpv4-sp-d16`・`__TARGET_ARCH_THUMB=4`・
`TOPPERS_CORTEX_M4`・TZ 定義なし・FPU 3 定義・QEMU `-machine mps2-an386`）／
`target_sil.h`（`TBITW_IPRI=3`・Cortex-M4 コメント）／`target_syssvc.h`
（`TARGET_NAME="ARM MPS2-AN386"`・`INTNO_SIO_RX=16`/`INTNO_SIO_TX=17`）／
`target_serial.{c,cfg}`（TX/RX 両割込みに同一 ISR を登録・open/close で両線を
ena/dis）／`target_test.h`（`INTNO1`=予備 IRQ30→INTNO46）／`presets.json`
（`mps2_an386-qemu`）／`ttsp3/`（`ttsp_target.py`・`ttsp_target_test.{c,h}`：
M4 化・`TTSP_INTNO_A〜F` を SIO 線 16/17 と衝突しない IRQ24〜29→INTNO40〜45 に
変更）．その他（`cmsdk_uart.{c,h}`・`target_timer.{c,h,cfg}`・`target_kernel*`・
`target_stddef.h`・`target_os_awareness.py` 等）は AN505 由来の機構をそのまま流用
（識別子・コメントを AN386/CM4/ARMv7-M/非TZ へ更新）．

### 配線（target 外）

- `CMakePresets.json`：`target/mps2_an386_gcc/presets.json` を include に追記．
- `.github/workflows/ci.yml`：`mps2-an386-qemu` ジョブ（build＋testexec
  `task1 sem1 flg1 int1 tmevt1 hrt1`）を追加．
- `DIVERGENCE_MAP.md`：NEW 2 行追記．`docs/dev/README.md`：状態を完了に更新．

### Git情報

- ベースコミット：`2fe37d7`（ブランチ `feat/mps2-an386`）
- ファイルリスト再現コマンド例：`git diff --stat 2fe37d7 -- target/mps2_an386_gcc`
  ／追加分は `git status --short target/mps2_an386_gcc`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| QEMU (mps2-an386) build | ○ | FLASH 22224B/RAM 12464B・警告なし |
| QEMU (mps2-an386) sample | ○ | バナー "ARM MPS2-AN386"・task1 周期実行 |
| QEMU (mps2-an386) testexec | ○ | 6/6 PASS（task1/sem1/flg1/int1/tmevt1/hrt1） |
| FP 命令（objdump） | ○ | `vldr`/`vstr`・ディスパッチャに `vpush`/`vpop {s16-s31}`（FPv4-SP 退避） |
| TTSP3 staticAPI（error+runtime） | ○ | PASS 91/FAIL 29（an505 と同一・退行なし） |
| 既存 an505 回帰 | ○ | build OK・sample OK（arch 共通部 無改変） |

### DIVERGENCE_MAP との関連

- `target/mps2_an386_gcc/` は NEW（上流衝突なし）．arch 共通部は無改変のため
  PRISTINE/EXTENDED への波及なし．
- 関連：[`mps2-an505.md`](mps2-an505.md)（M33/v8-M の先行ターゲット）．
