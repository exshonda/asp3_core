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

（完了時に記載）
