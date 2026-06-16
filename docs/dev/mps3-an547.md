# QEMU Cortex-M55 ターゲット（mps3-an547 / ARMv8.1-M＋MVE テスト）

## 項目

QEMU 用 Cortex-M55（ARMv8.1-M）ターゲット `mps3-an547` を新設し、あわせて
**MVE(Helium) の VPR レジスタ退避を共通 arch へ復活**させる。
AGENTS.md §1「機能追加計画」の「QEMUターゲット(ARMv8.1-M)」。

目的：
1. ARMv8.1-M（Cortex-M55/SSE-300）経路を QEMU で検証する。
2. **MVE(Helium) のコンテキスト退避（VPR）** を QEMU 上で常時検証可能にする。これは
   asp3_fsp の RA8M2(Cortex-M85) 対応で実装済みだが、submodule 移行時に共通 arch へ
   未マージのまま先送りされた積み残し（[`fsp-integration.md`](fsp-integration.md) L190-191）の解消を兼ねる。

## 内容

### 対象 QEMU マシン

`mps3-an547`（`qemu-system-arm -M mps3-an547`。**マシン名は `mps3-`**）。QEMU の
`hw/arm/mps2-tz.c`（`mps2-an505`/`an521` と同系統）でモデル化される
**Cortex-M55・SSE-300・TrustZone・MPS3** ボード。CPU0 は FPU を実装する。

### MVE は「実装済みだが現行 asp3_core 未マージ」

`arch/arm_m_gcc/common/core_support.S` の現行版には MVE/VPR 退避が**無い**（grep 0件）。
一方 **asp3_fsp@`30bf318`（2026-05-22・submodule 化前の bundled fork）には実装済み**
（`asp3_change.md` Step5「EK-RA8M2 FPU/MVE 有効化」・EK-RA8M2 実機検証済み）。3箇所：

| 箇所 | 挿入位置 | コード |
|---|---|---|
| `do_dispatch` 保存 | `vpush {s16-s31}` 直後 | `vmrs r12, VPR` / `push {r12}` |
| `dispatcher` 復帰 | `vpop {s16-s31}` 直前 | `pop {r12}` / `vmsr VPR, r12` |
| `return_to_thread_fp` 復帰 | `ldmfd r0!,{r4-r11}` 直後 | `ldmfd r0!,{r12}` / `vmsr VPR, r12` |

いずれも `#ifdef __ARM_FEATURE_MVE`（外側 `#ifdef TOPPERS_FPU_CONTEXT`）でガード。
VPR はスタック退避で **TCB/TINIB 構造の変更不要**。現行 arch の退避構造
（`vpush/vpop {s16-s31}`・`return_to_thread_fp` の `ldmfd r0!,{r4-r11}`）は当時と同型の
ため、**3ハンクの移植で復活可能**。非MVEターゲットは `#ifdef` で無影響。

### QEMU から確定したボードパラメータ（`hw/arm/mps2-tz.c`）

| 項目 | 値 | 出典 |
|---|---|---|
| CPU | Cortex-M55（ARMv8.1-M、SSE-300、TZ、FPU） | `mps3tz_an547_class_init`：`armsse_type = TYPE_SSE300` |
| init-svtor | `0x00000000` | `mmc->init_svtor = 0x00000000` |
| SRAM_ADDR_WIDTH | 21 | `mmc->sram_addr_width = 21` |
| メモリ（`an547_raminfo`） | ITCM `0x00000000`(512KB・armsse.c)／sram `0x01000000`(2MB)／DTCM `0x20000000`(512KB)／sram2 `0x21000000`(4MB)／QSPI(ROM) `0x28000000`(8MB)／DDR `0x60000000` | `an547_raminfo[]`・armsse.c「SSE-300 has an ITCM at 0x0000_0000」 |
| ブート位置（ld） | init-svtor=0 が指す **ITCM(0x0)** にベクタ＋コード．データは sram2(0x21000000)．※「コード 0x01000000」は sram 領域の意でブートアドレスではない（実装で判明） | armsse.c:1227-1238 |
| UART | UART0 `0x49303000`・IRQ{rx=33,tx=34,combined=43} | `an547_ppcs` apb_ppcexp2 "uart0" |
| FPU | CPU0_FPU=true（SSE-300） | `sse300_properties`：`CPU0_FPU=true` |

`mps2-an505`(IoTKit) との違い：SSE-300＋MPS3 のメモリマップ（コード `0x01000000`・
init_svtor `0x00000000`）と `an547_ppcs` の UART 番地、CPU が M55。**構造は an505 の
延長**（SSE/TZ/FPU 同系）で、メモリマップと UART 番地を差し替える。

## 実施プラン

> 着手前。an547 ＝ an505 の延長。MVE は arch への独立変更として段階検証する。

### Step 1：ボード新設（スカラ FP まで）

1. `target/mps3_an547_gcc/` を `mps2_an505_gcc` を雛形に新設：
   - `mps3_an547.h`：UART ベース・IRQ（`an547_ppcs` から）、クロック、`TBITW_IPRI`。
   - `mps3_an547.ld`：コード `0x01000000`(2MB)・データ `0x21000000`(4MB)・init-svtor `0x00000000`。
   - 流用：`cmsdk_uart.*`／`target_timer.c`(SysTick)／`target_serial.*`／arch 共通部。
2. `target.cmake`：`-mcpu=cortex-m55`、`__TARGET_ARCH_THUMB=5`、`TOPPERS_CORTEX_M55`、
   `TOPPERS_ENABLE_TRUSTZONE`、FPU（`-mfpu=fpv5-d16` 系・`TOPPERS_FPU_*`）。
   QEMU 実行：`qemu-system-arm -machine mps3-an547 …`。
3. presets/`CMakePresets.json`/CI/TTSP3 資産を追加。
4. 検証：build → QEMU sample → testexec → TTSP3 staticAPI（**MVE 無効ビルドで先に成立**）。

### Step 2：MVE(Helium) VPR 退避の復活（共通 arch 変更）

1. **asp3_fsp@`30bf318` の3ハンクを `arch/arm_m_gcc/common/core_support.S` へ移植**
   （上記表の挿入位置。`#ifdef __ARM_FEATURE_MVE` ガード維持）。
   - 取得元：`git -C ../asp3_fsp show 30bf318:asp3/arch/arm_m_gcc/common/core_support.S`。
   - 現行行と照合し、`vpush/vpop {s16-s31}`・`ldmfd r0!,{r4-r11}` の各アンカーに挿入。
2. an547 を `-mcpu=cortex-m55+mve`（または `-march=armv8.1-m.main+mve.fp`）で再ビルドし、
   `__ARM_FEATURE_MVE` が立つことを確認。
3. **MVE コンテキスト退避の検証**：複数タスクが Q0-Q7／VPR を使い分ける testexec 相当の
   小テストで、ディスパッチ跨ぎに VPR/Q が破壊されないことを確認（QEMU）。
4. **回帰**：非MVEターゲット（an505・an386・pico2_arm・mimxrt685）の build を確認
   （`#ifdef` ガードで無影響であること）。
5. **DIVERGENCE_MAP 追記**：`core_support.S` は arch(EXTENDED)。MVE 追加を記録し、
   [`fsp-integration.md`](fsp-integration.md) の積み残し項目と相互リンク。

### リスク・確認事項

- `core_support.S` は PRISTINE 隣接（arch・EXTENDED）。**MVE 変更は Step 2 として独立に
  検証**し、Step 1（ボード）と分離する。
- `an547_ppcs` の UART 番地・IRQ を実装時に正確に読む（an505 の `0x40200000` とは別）。
- M55 の FPU 種別（`fpv5-d16` か `fpv5-sp-d16` か）と MVE 有効化フラグの整合
  （`+mve` vs `+mve.fp`）。
- RA8M2(M85) も同じ MVE コードを使うため、復活後は asp3_fsp 側で submodule bump すれば
  M85 でも MVE 退避が有効化される（後続）。

## 実施結果

### Step 1（ボード新設・スカラ FP まで）：完了（2026-06-16）

MVE は未着手（Step 2）．`target/mps3_an547_gcc/` を `mps2_an505_gcc` から派生
して新設し，SSE-300/Cortex-M55/MPS3 のメモリマップ・UART 番地・FPU 種別を
確定した．arch 共通部（`core_support.S` 等）は無変更．

#### 確定したボードパラメータ（QEMU 11.0.1 ソース根拠）

| 項目 | 値 | 出典（hw/arm/mps2-tz.c, armsse.c, cpu-v7m.c） |
|---|---|---|
| CPU | Cortex-M55（ARMv8.1-M, SSE-300, TZ, FPU） | mps3tz_an547_class_init:1429 / armsse.c:556 TYPE_SSE300 |
| init-svtor | `0x00000000` | mps2-tz.c:1441 `mmc->init_svtor = 0x00000000` |
| SRAM_ADDR_WIDTH | 21 | mps2-tz.c:1443 |
| numirq (EXP_NUMIRQ) | 96 | mps2-tz.c:1439 → TMAX_INTNO=(128+16)=144 |
| sysclk(MAINCLK=cpuclk) | 32MHz | mps2-tz.c:1432 ／ armsse.c:994 cpuclk←MAINCLK |
| systick clock | プロセッサクロック(32MHz) | armsse.c:995「SSE subsystems do not wire up a systick refclk」 |
| メモリ：ITCM | `0x00000000`, 512KB | armsse.c:1227-1238「The SSE-300 has an ITCM at 0x0000_0000」 |
| メモリ：sram | `0x01000000`, 2MB | an547_raminfo[] mps2-tz.c:276 |
| メモリ：DTCM | `0x20000000`, 512KB | armsse.c:1228,1238 |
| メモリ：sram 2 | `0x21000000`, 4MB | an547_raminfo[] mps2-tz.c:283 ／ armsse.c:560 sram_bank_base |
| メモリ：QSPI(ROM) | `0x28000000`, 8MB | an547_raminfo[] mps2-tz.c:290 |
| メモリ：DDR | `0x60000000` | an547_raminfo[] mps2-tz.c:297 |
| UART0 base | `0x49303000` | an547_ppcs apb_ppcexp2 mps2-tz.c:1079 |
| UART0 IRQ | rx=33, tx=34, combined=43 | an547_ppcs uart0={33,34,43}（make_uart は rx,tx,combined 順） |
| FPU | FPv5-D16（倍精度） | cpu-v7m.c:217 MVFR0=0x10110221（SP/DP 実装）→ -mfpu=fpv5-d16 |
| TBITW_IPRI | 3 | 実機 SSE-300 NVIC=3bit（an505 と同方針．QEMU は 8bit 実装で上位 3bit 使用） |

> **リンカスクリプトの要点（要人間確認）**：init-svtor=0 のため，ベクタ＋コードは
> init-svtor が指す **ITCM(0x0, 512KB)** に置く．`an547_raminfo[]` の sram は
> `0x01000000` だがブートアドレスではない（dev 文書の「コード 0x01000000」は
> sram 領域の意．実際のリセットベクタ位置は ITCM=0x0）．データは sram 2
> (`0x21000000`, 4MB) に配置．FLASH 使用 23KB / 512KB．

#### 追加したファイル

- `target/mps3_an547_gcc/`（一式．`mps2_an505_gcc` から `cp -r` 後，
  `mps2_an505.h`→`mps3_an547.h`，`mps2_an505.ld`→`mps3_an547.ld` を改名し，
  識別子・コメントを AN547/M55/SSE-300/MPS3 へ更新）．主な実体変更ファイル：
  `mps3_an547.h`（UART base/IRQ・CPU_CLOCK_HZ=32MHz・TMAX_INTNO=144），
  `mps3_an547.ld`（ITCM 0x0 起動・sram2 データ），`target.cmake`
  （`-mcpu=cortex-m55+nomve`・`-mfpu=fpv5-d16`・`TOPPERS_CORTEX_M55`・
  FPU レシピ），`target_syssvc.h`・`target_sil.h`・`target_timer.h`・
  `target_test.h`・`target_user.md`・`presets.json`．

#### 変更したファイル（target 外）

| ファイル | 変更内容概要 |
|---|---|
| `CMakePresets.json` | include に `target/mps3_an547_gcc/presets.json` を追記 |
| `docs/dev/mps3-an547.md` | 本実施結果（Step 1）を記載 |

#### Git情報

- ベースコミット：`2fe37d7`（feat/mps3-an547）
- 関連コミット範囲：（未コミット＝ワーキングツリー．ユーザーレビュー後にコミット）
- ファイルリスト再現：`git status --short` ／ `ls target/mps3_an547_gcc/`
- 参照（Step 2 用）：`git -C asp3_fsp show 30bf318:asp3/arch/arm_m_gcc/common/core_support.S`（MVE 原典）

#### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| QEMU (mps3-an547) build（MVE無効） | ○ | PASS（FLASH 23KB/512KB・RAM 12KB/4MB） |
| QEMU (mps3-an547) sample | ○ | PASS（バナー「for ARM MPS3-AN547」・task1 周期実行） |
| testexec スモーク（task1/sem1/flg1/tmevt1/hrt1） | ○ | **5/5 PASS** |
| objdump FP 命令 | ○ | `vpush/vpop {s16-s31}`・`vldr d7`・`vstr` 生成．MVE 命令なし |
| MVE VPR 退避（+mve ビルド・QEMU） | − | Step 2 |
| 非MVE 回帰（an505/an386/pico2/mimxrt685） | − | Step 2（本 Step は arch 無変更のため回帰なし） |
| TTSP3 staticAPI | − | Step 1 範囲外（ttsp3 資産は番地/CPU を M55 へ更新済み・未実行） |

### DIVERGENCE_MAP との関連

- `target/mps3_an547_gcc/` は NEW。
- `arch/arm_m_gcc/common/core_support.S`（EXTENDED）に MVE VPR 退避を追加＝要 DIVERGENCE_MAP 追記。
- 関連：[`mps2-an505.md`](mps2-an505.md)（v8-M 先行）・[`fsp-integration.md`](fsp-integration.md)（MVE 原典・RA8M2 積み残し）。
