# ARM-M 性能評価：DWT CYCCNT 時間源（USE_ARM_DWT_PMCNT）

ARM-M コア依存部で、実行時間分布集計サービス（`syssvc/histogram`）の時間源を
**DWT の CYCCNT サイクルカウンタ**にする設定。`arm_gcc` の `USE_ARM_PMCNT`
（PMCCNTR）/ `riscv_gcc` の cycle CSR と同じ枠組みで、ARM-M にだけ欠けていた
`core_syssvc.h` を新設して実装した。

## 何が嬉しいか

`histogram` の既定の時間源は `fch_hrt()`（マイクロ秒分解能の HRT）。コンテキスト
スイッチやサービスコールは数百サイクル＝サブマイクロ秒で、μs 分解能では量子化
されてしまう。DWT CYCCNT はコアクロック 1 サイクル単位で計数するため、これを
時間源にするとサイクル精度のヒストグラムが採れる。

実測（Raspberry Pi Pico2 / RP2350・Cortex-M33・150MHz）：`perf0` の計測オーバヘッドが
DWT では **140 ns**（fch_hrt では 0〜4 μs に量子化）。

## 仕組み（layered include）

```
syssvc/histogram.c ──(既存)──> #include "target_syssvc.h"   ← #ifndef HISTTIM より前
target/<board>/target_syssvc.h ──> #include "core_syssvc.h"  ← 全 ARM-M ターゲット
arch/arm_m_gcc/common/core_syssvc.h ─> #if USE_ARM_DWT_PMCNT: HISTTIM/HIST_GET_TIM を
                                        DWT CYCCNT に上書き（arm_m_get_pmcnt＝生サイクル）
target/<board>/target_syssvc.h ──────> #if USE_ARM_DWT_PMCNT: HIST_CONV_TIM を
                                        ナノ秒変換に上書き（CPU_CLOCK_HZ 依存）
arch/arm_m_gcc/common/core_kernel_impl.c ─> core_initialize() で #if USE_ARM_DWT_PMCNT:
                                        arm_m_pmcnt_initialize()（DEMCR.TRCENA 等）
arch/arm_m_gcc/common/arm_m.h ─────────> DWT/DEMCR レジスタ定義（_ADDR 命名）
```

- **オプトイン**：`USE_ARM_DWT_PMCNT` を定義したビルドでのみ有効。未定義の通常
  ビルドでは `core_syssvc.h` は何も定義せず、DWT も有効化しない（＝副作用なし・
  既定の fch_hrt のまま）。`arm_gcc` の `USE_ARM_PMCNT` と同じ作法。
- **v7-M 以降専用**：DWT/CYCCNT は ARMv6-M（Cortex-M0/M0+）に無いので
  `__TARGET_ARCH_THUMB >= 4` でガード。
- **QEMU は非対応**：QEMU は DWT CYCCNT を実装せず常に 0 を返す（`NOCYCCNT=0`
  でも計数しない）。よって QEMU ターゲットでは `USE_ARM_DWT_PMCNT` を指定しない
  こと（指定すると全計測が 0 になる）。本機能の検証は実機（Cortex-M33 等）で行う。
- CYCCNT は full 32bit のフリーランカウンタなので、`histogram.c` の end-begin
  （符号無し 32bit 減算）だけでラップは正しく扱える。
- **計測単位はナノ秒**。`core_syssvc.h` は生サイクルを返し、サイクル→ns 変換係数は
  コアクロック依存のため **ターゲット依存部（`target_syssvc.h`）の `HIST_CONV_TIM`**
  で与える（`ns = cycles * 1000 / (CPU_CLOCK_HZ[MHz])`）。`HIST_MAX_TIME`（既定 1000）
  を超える区間は histogram の `over`（`> 1000`）に集計される＝ns 粒度では 1μs 未満の
  高速処理の計測に向く。

## 使い方

性能評価プログラム（`perf0`〜`perf5` 等、`syssvc/histogram` を使うアプリ）を
`-DUSE_ARM_DWT_PMCNT` 付きでビルドするだけ。アプリ側の変更は不要（DWT の有効化は
`core_initialize()` が行う）。例（実機 pico2 で perf0）：

```bash
cmake -S . -B build/perf0-pico2 --preset pico2_arm \
  -DASP3_APPLDIR=$PWD/test -DASP3_APPLNAME=perf0 \
  "-DASP3_EXTRA_APP_C_FILES=$PWD/syssvc/test_svc.c;$PWD/syssvc/histogram.c" \
  "-DASP3_EXTRA_COMPILE_DEFS=USE_ARM_DWT_PMCNT"
cmake --build build/perf0-pico2
# OpenOCD(CMSIS-DAP) で program verify reset exit → UART に cycle 単位の分布が出る
```

単体でサイクル数を測りたい場合は、`core_syssvc.h` の `arm_m_get_pmcnt(&t)` を
前後で読んで差分を取る（同じく `USE_ARM_DWT_PMCNT` が必要）。

## 変更ファイル

| ファイル | 種別 | 内容 |
|---|---|---|
| `arch/arm_m_gcc/common/core_syssvc.h` | 新規 | ARM-M の syssvc コア依存部（他アーキと同型）。`USE_ARM_DWT_PMCNT` 時に `HISTTIM`/`HIST_GET_TIM` を DWT CYCCNT に設定 |
| `arch/arm_m_gcc/common/arm_m.h` | 追加 | DWT/DEMCR レジスタのアドレス・ビットマクロ（`DEMCR_ADDR`/`DWT_CTRL_ADDR`/`DWT_CYCCNT_ADDR`/`DWT_LAR_ADDR` ほか。`_ADDR` 命名で CMSIS 衝突回避） |
| `arch/arm_m_gcc/common/core_kernel_impl.c` | 追加 | `arm_m_pmcnt_initialize()` と `core_initialize()` からの呼び出し（ともに `#if USE_ARM_DWT_PMCNT && __TARGET_ARCH_THUMB>=4` ガード） |
| `target/{mps2_an505,mps2_an386,mps3_an547,pico2_arm,mimxrt685evk}_gcc/target_syssvc.h` | 追加 | `#include "core_syssvc.h"`（全 ARM-M ターゲットで取り込み。QEMU 3 種はフラグ非指定＝fch_hrt のまま）。実機 2 種（pico2/mimxrt685）は `HIST_CONV_TIM`（cycles→ns・`CPU_CLOCK_HZ` 依存）も定義 |

`syssvc/histogram.c`・`histogram.h` は**無改変**（時間源差し替えは histogram が元々
持つ `#ifndef HISTTIM` フックを arch 側で上書きする方式のため）。

## 検証

| 環境 | 結果 |
|---|---|
| linux / 全 ARM-M サンプル（フラグ off） | ビルド回帰なし（DWT ブロックは未コンパイル） |
| QEMU mps2-an505（`perf0`・fch_hrt） | 既定の HRT ヒストグラム（0〜4 μs）＝回帰なし |
| 実機 PICO2（`perf0`・`-DUSE_ARM_DWT_PMCNT`） | ナノ秒ヒストグラム（計測オーバヘッド **140 ns** が 9999/10000。外れ値 1 件は >1000ns＝over） |

## DIVERGENCE_MAP との関連

`arm_m.h`・`core_kernel_impl.c`（ともに arch=EXTENDED）への追加は `#ifdef` ガード付き
純追加でテキストマージ可。`DIVERGENCE_MAP.md` の該当エントリ参照。`core_syssvc.h` は
他アーキ（arm_gcc/riscv/arm64）に既存の同名ファイルを ARM-M に補完した新規ファイル。
