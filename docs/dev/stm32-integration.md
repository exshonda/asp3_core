# STM32 HAL統合（STM32Cube / Cortex-M）

## 項目

STM32 HAL統合（AGENTS.md §1 機能追加計画、優先度：中。第一目的「各社SDKとの協調動作」の第3弾＝Pico SDK・FSPに続く）

## 内容

STM32Cube HAL（STM32CubeMX生成）と TOPPERS/ASP3 を協調動作させる。
移植元は `../stm32_vscode_asp`（NUCLEO-H563ZI＝STM32H563・Cortex-M33）。
Pico SDK／FSP統合と同じく **SDK固有部を外側リポジトリで管理＋asp3_core を
submodule＋`ASP3_TARGET_DIR` で受け入れる**方式を目指す。

> 注：asp3_core には既に `target/stm32mp257f_dk_arm64_gcc`（STM32MP2・Cortex-A35・
> ベアメタル）があるが、本項目は **STM32CubeMX HAL を使う Cortex-M（STM32H5系）**で別物。

## 現状（2026-06-11 調査・`../stm32_vscode_asp` をclone）

```
stm32_vscode_asp/
├── asp3/                          ← カーネル同梱（★旧世代：TECS＋Ruby cfg）
│   ├── arch/arm_m_gcc/{stm32h5xx_stm32cube, common}
│   ├── target/stm32cubemx         ← STM32Cubeターゲット依存部
│   ├── tecsgen/                   ← ★TECS同梱
│   ├── cfg/*.rb                   ← ★Ruby版cfg
│   ├── syssvc/*.cdl               ← ★TECSセル
│   └── asp3_stm32cubemx.cmake     ← STM32 glue
├── nucleo_h563zi/                 ← アプリ（STM32CubeMXプロジェクト）
│   ├── H563ZI.ioc                 ← CubeMX設定
│   ├── Core/ startup_stm32h563xx.s
│   └── cmake/{gcc-arm-none-eabi.cmake, stm32cubemx/}
```

### 他2つ（Pico/FSP）との決定的な違い
- **移植元が旧世代asp3**：**TECS（tecsgen同梱）＋ Ruby cfg（cfg.rb）**。
  Pico/FSPは既にTECSレス＋Python cfgだったが、**STM32はそうではない**。
  → 同梱asp3はそのまま使えず、**asp3_core（TECSレス＋Python cfg）へ移植**が要る。

### 移植が楽な点
- **ツールチェーンが arm-none-eabi (gcc)**（`cmake/gcc-arm-none-eabi.cmake`）＝
  既存ターゲット（mps2/pico/zybo）と同じ。**FSPのclangと違い新ツールチェーン不要**。
- **arm_m**（STM32H563＝Cortex-M33）＝既存arch系列。
- **TECSレス変換の前例がある**：`target/stm32mp257f_dk_arm64_gcc`／
  `arch/arm64_gcc/stm32mp2` は STM32 を TECS版→非TECS版へ変換済み
  （`PORTING_ASP3_STM32MP2.md`）。同じ要領で stm32h5xx_stm32cube を変換できる。
- asp3_core側の受け入れ口（`ASP3_TARGET_DIR`・`ASP3_LIBRARY_ONLY`）は整備済み。

### STM32固有の論点
- **STM32CubeMX生成依存**：`H563ZI.ioc` から CubeMX が `Core/`・HALドライバを生成
  （FSPのRASCに相当）。`add_subdirectory(cmake/stm32cubemx)` で取り込む。
- **割込みベクタ／startup**：`startup_stm32h563xx.s`（CubeMX生成）と ASP3 の
  ベクタ・例外の調停（NVIC掌握）。pico の `--wrap` 相当の検討が要るか確認。
- **実機**：NUCLEO-H563ZI（ST-LINK内蔵）。OpenOCD/STM32CubeProgrammerで書込み。

## 必要な環境（ユーザー確認事項）

| 要素 | 用途 | 状況 |
|---|---|---|
| **STM32CubeMX**（＋HAL/CMSIS） | `.ioc`→HAL/Core生成（FSPのRASC相当） | **要確認・要導入**（VSCode STM32拡張＝stm32-for-vscode 経由でも可）。**バージョンは導入時点の最新（現行）に合わせる**（移植元の旧バージョンに固定せず、CubeMX/HAL/CMSISは現行版で再生成） |
| arm-none-eabi-gcc | ビルド（gcc系） | **導入済**（既存ターゲットで使用中） |
| OpenOCD / STM32CubeProgrammer | NUCLEO-H563ZI 書込み・デバッグ | 実機検証時に要確認 |
| NUCLEO-H563ZI 実機 | 動作確認 | 実機接続PCで |

> STM32CubeMX（または同等の生成）が無いと `Core/`・HALが生成できずビルド不可。
> Pico（git clone完結）と違い、FSP同様**ツール生成依存**。CIはビルド対象外見込み。

## 実施プラン

### Step 0：移植元の素性確認（最初に）
- 同梱asp3（TECS/Ruby）と現 asp3_core の乖離を把握。**TECS版syssvc/SIOを
  非TECS版へ変換**する範囲を確定（stm32mp2変換を規範に）。
- `arch/arm_m_gcc/stm32h5xx_stm32cube`・`target/stm32cubemx` のうち、
  TECS依存（.cdl・t*.c）と純依存部（レジスタ定義・SIO実体）を仕分け。

### Step 1：asp3_core への arch/chip 追加（TECSレス・Python cfg化）
- `arch/arm_m_gcc/stm32h5xx_stm32cube`（または外側リポジトリ）に、
  非TECS版 SIO（`chip_serial.[ch]`＋HAL UARTラッパ）・`chip_kernel.py`等を作成
  （pico2_arm／stm32mp2の非TECS構成を規範）。
- cfgテンプレートは `.trb`/Ruby→`.py`（既存変換済みターゲットを規範）。

### Step 2：ターゲット依存部（外側リポジトリ stm32_vscode_asp 側）
- A案（Pico/FSPと同型）：asp3_core を submodule 参照、SDK固有 arch/target を
  外側で `ASP3_TARGET_DIR` 供給、fork CMakeを asp3_core正準へ。
- CubeMX生成フロー（`.ioc`→Core/HAL）＋ gcc toolchain を維持。

### Step 3：検証（実機 NUCLEO-H563ZI）
- CubeMX生成 → gccビルド → sample1（バナー・タスク切替）→ testexec。
- TECSレス変換の正しさを test_porting（6項目）で機械判定。

### Step 4：記録
- DIVERGENCE_MAP（stm32h5xx_stm32cube追加・必要なら）・README索引・本ファイル。

## スコープ外
- TECS構成のサポート（asp3_coreはTECSレス方針＝非TECS版に変換して取り込む）
- STM32全シリーズ（まずH5＝H563の1ボード）
- QEMU対応（NUCLEO-H563ZIのQEMUモデルは想定しない）
- CIでのビルド（CubeMX生成依存＝GitHubランナーに無い。実機環境で検証）

## リスク・確認事項
- **TECS→非TECS変換の工数**：3SDK中最大。ただし stm32mp2 で同種変換の前例・
  知見あり（PORTING_ASP3_STM32MP2.md）。
- **CubeMX生成の再現性**：`.ioc`→生成物のツールバージョン依存。**バージョンは現行（最新）に合わせる方針**（移植元のバージョンには固定しない）。`.ioc` を現行CubeMXで開き直して再生成→差分確認。生成物コミット可否も判断。
- 同梱asp3（旧世代）の他の差分（Ruby cfgテンプレート・arm_m chip実装）の現 asp3_core 比較。
- startup/ベクタの調停（CubeMX startup と ASP3 のNVIC/例外）。

## 実施結果

（2026-06-11 実施。Step 0〜Step 1ビルド検証まで完了。実機検証は次ステップ）

### 方針（確定）

A案（FSP/Pico と同型）：外側リポジトリ `stm32_vscode_asp` を **asp3_core サブモジュール
構成**へ変換し、STM32固有の arch（チップ層）/target を外側に残置、`ASP3_TARGET_DIR`
で asp3_core に供給する。**asp3_core 本体は無変更**（受け入れ口は Pico/FSP で整備済み）。
作業はすべて外側リポジトリ `stm32_vscode_asp`（ブランチ `feat/stm32-h5`）で実施。

### Step 0：移植元の素性確認（仕分け結果）

移植元 `../stm32_vscode_asp/asp3`（旧世代asp3：TECS＋Ruby cfg 同梱）のうち、STM32固有部
（`arch/arm_m_gcc/stm32h5xx_stm32cube`・`target/stm32cubemx`）は **ほぼ非TECS化済み**で、
残るTECS/Ruby依存は `.trb`×2 のみだった：

| 区分 | ファイル | 扱い |
|---|---|---|
| 非TECS済・流用可 | `target_serial.c`（HAL UART・plain `sio_*`）・`target_timer.c`（HAL TIM2=HRT/TIM5=割込み）・`target_kernel_impl.c`（TECS呼出は `#ifndef TOPPERS_OMIT_TECS` ガード内） | そのまま流用 |
| plain cfg | `target_kernel.cfg`・`target_serial.cfg`・`target_timer.cfg` | そのまま |
| 要 .trb→.py | `target_kernel.trb`・`target_check.trb` | Python化（下記） |
| chip層ヘッダ | `chip_kernel.h`/`chip_sil.h`(`TBITW_IPRI=4`)/`chip_kernel_impl.h` 他 | FSP(ra8m2)と**バイト一致**＝流用 |
| 破棄（asp3_coreに存在） | 同梱 `syssvc/*.cdl`・`tecsgen/`・Ruby `cfg/*.rb`・`arch/.../common/*.trb` | submodule化で削除 |

### Step 1〜2：移植実装

- **submodule化**：bundled `asp3/`（606ファイル）の純カーネル部を削除し `asp3/asp3_core`
  （submodule＝`https://github.com/exshonda/asp3_core.git`）へ。STM32固有部のみ残置。
- **chip層 `asp3/arch/arm_m_gcc/stm32h5xx_stm32cube/`**：ヘッダ群は流用。`arch.cmake` のみ
  書換（`core_offset.trb`→`.py`、`ARCHDIR/CHIPDIR` 規約、`start.S` 非include＝CubeMX
  startup がリセットを握るため、存在しない `chip_serial.c` 参照を除去）。
- **target層 `asp3/target/stm32cubemx/`**：
  - `target_kernel.trb`→`target_kernel.py`：**Pattern B**＝arm_m 共通 `core_kernel.py` を
    使わず `kernel/kernel.py` を直include し、ベクタテーブルを **`Reset_Handler`/`&_estack`**
    （CubeMX startup 由来）に向けて生成（`_kernel_exc_tbl`・`_kernel_bitpat_cfgint` も生成）。
    FSP `target_kernel.py` と同型。`_estack`/`Reset_Handler` は既存ヘッダ宣言を使用（重複宣言しない）。
  - `target_check.trb`→`target_check.py`（`core_check.py` を include）。
  - `target.cmake`：FSP型（`ARCHDIR`=submodule、`CHIPDIR`=外側）。M33＋fpv5-sp-d16 フラグ。
    **`-Wl,--no-gc-sections` を追加**（下記理由）。
- **glue `asp3/asp3_stm32cubemx.cmake`**：`asp3_fsp.cmake` 同型（`ASP3_CORE_DIR`/
  `ASP3_ROOT_DIR`/`ASP3_TARGET_DIR`＋`asp3_set_stm32_options`）。
- **アプリ `nucleo_h563zi/CMakeLists.txt`**：fork廃止→`ASP3_LIBRARY_ONLY`＋
  `add_subdirectory(asp3_core)`＋`asp3_add_syssvc`＋`ASP3_APPLDIR/APPLNAME`。

### CubeMX 生成（headless 自動化）

NUCLEO 環境はGUI（xrdp）が不調だったため、`xvfb-run` + `xdotool`（要 `sudo apt install
xvfb xdotool`）で **完全headless自動化**した：CubeMX 6.17 を仮想ディスプレイ起動 →
`config load`/`project generate` → 出現ダイアログを screenshot（`xwd`→PNG）で確認しつつ
クリック：①移行ダイアログ→**Migrate**（現行最新＝CubeMX 6.17＋**FW_H5 V1.6.0**）→
②FW未導入→**Download**→③ライセンス→**I agree**＋**Finish**。FW_H5 V1.6.0（242MB）DL/導入後、
`Core/Drivers/cmake/`＋リンカスクリプトを生成。`.ioc` は移行で V1.6.0 に更新（CubeMXが書換）。
`swmgr install <名前>` は当版では firmware 名解決不可（"Unknown pack"）＝`project generate`
の自動DL経路が正解、という知見。

### ビルドで解決した3点（移植知見）

1. **cfg1_out が空＝`'TOPPERS_magic_number' is not found`**：CubeMXツールチェーン
   （`cmake/gcc-arm-none-eabi.cmake`）が `CMAKE_EXE_LINKER_FLAGS` に `-Wl,--gc-sections` を
   **グローバル付与**。asp3_core側の gc-sections 除去はツールチェーンのグローバル付与には
   効かず、cfg1_out（オフセット抽出用ELF）の未参照シンボルがGCで消失。`target.cmake` の
   `ASP3_LINK_OPTIONS` に **`-Wl,--no-gc-sections`**（後勝ち）を追加して解消。最終exeはCubeMX側でGC有効のまま。
2. **`_estack` 型衝突**：`target_kernel.py` の `extern uint32_t _estack;` が
   `target_kernel_impl.h` の `extern int _estack;` と衝突。重複 extern を削除（既存宣言を使用）。
3. **`EXC_RETURN_PREFIX` 再定義warning**：CMSIS `core_cm33.h`（`0xFF000000UL`）と arm_m.h
   （`0xff000000`）。非致命・FSP（M85）と同様。未対処（warningのまま）。

### ベクタ/startup 調停（確認済み）

`H563ZI.elf` では `0x08000000` に CubeMX `g_pfnVectors`（`.isr_vector`）、ASP3
`_kernel_vector_table` は `.vector`（非flash先頭）に配置。実行時は ASP3 の
`core_initialize()`（`core_kernel_impl.c`）が **`SCB->VTOR=_kernel_vector_table`** を設定する
ため整合（`Reset_Handler`→`main()`→`sta_ker()`→`target_initialize()`→`core_initialize()`）。
リンカスクリプト変更は不要。

### 変更したファイル（外側リポジトリ `stm32_vscode_asp`・ブランチ `feat/stm32-h5`）

| ファイル | 変更 |
|---|---|
| `.gitmodules`／`asp3/asp3_core` | 追加（submodule＝asp3_core `9f54ac8`） |
| `asp3/`（bundled kernel 571ファイル） | 削除（kernel/include/library/cfg/syssvc/tecsgen/tecs_kernel/test/test_cfg/utils/doc/arch/{gcc,tracelog,arm_m_gcc/common} 等） |
| `asp3/arch/arm_m_gcc/stm32h5xx_stm32cube/arch.cmake` | 書換（.py化・ARCHDIR/CHIPDIR・start.S非include） |
| `asp3/arch/arm_m_gcc/stm32h5xx_stm32cube/chip_*.h` | 流用（無変更） |
| `asp3/target/stm32cubemx/target_kernel.py`・`target_check.py` | 新規（.trb→.py） |
| `asp3/target/stm32cubemx/{target_kernel,target_check}.trb` | 削除 |
| `asp3/target/stm32cubemx/target.cmake` | 書換（FSP型・M33/fpv5-sp-d16・`-Wl,--no-gc-sections`） |
| `asp3/asp3_stm32cubemx.cmake` | 書換（FSP glue 同型） |
| `nucleo_h563zi/CMakeLists.txt` | submodule＋LIBRARY_ONLY＋asp3_add_syssvc へ |
| `nucleo_h563zi/{H563ZI.ioc,Core,Drivers,cmake,*.ld,CMakePresets.json}` | CubeMX 6.17＋FW_H5 V1.6.0 で再生成（`Drivers/cmake/*.ld` は `.gitignore` 済） |

asp3_core 本体：**無変更**（docs のみ：本ファイル・README索引）。

### Git情報

- asp3_core：ベース `9f54ac8`（submodule pin）。本体コード変更なし。
- 外側 `stm32_vscode_asp`：ブランチ `feat/stm32-h5`（main からの差分が全実装）。
  - ファイルリスト再現：`git -C ../stm32_vscode_asp diff --stat main feat/stm32-h5`

### 検証結果

- **ビルド（実機ターゲット）**：NUCLEO-H563ZI / STM32H563（Cortex-M33）。
  `cmake --preset Debug`（arm-none-eabi-gcc 13.2.1 / Ninja）→ **`H563ZI.elf` リンク成功**。
  FLASH 61,344 B (2.93%) / RAM 15,344 B (2.34%)。warning は `EXC_RETURN_PREFIX` 再定義のみ。
- **POSIX/QEMU**：対象外（CubeMX HAL 依存・QEMUモデル無し。スコープ外）。
- **実機（フラッシュ→バナー/タスク切替・testexec）**：**未実施**（NUCLEO-H563ZI 接続環境での次ステップ）。
  ベクタ調停は静的に確認済み（上記）。CI はビルド対象外（CubeMX生成依存）。
