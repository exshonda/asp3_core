# NXP MCUXpresso SDK統合（i.MX RT685）

## 項目

NXP MCUXpresso SDK統合（AGENTS.md §1 機能追加計画、優先度：中。第一目的「各社SDKとの協調動作」の**第4弾**＝Pico SDK・FSP・STM32に続く）

## 内容

NXP MCUXpresso SDK と TOPPERS/ASP3 を協調動作させる。対象は
**EVK-MIMXRT685**（i.MX RT685＝Cortex-M33＋HiFi4 DSP。ASP3はCM33のみ使用）。
確立済みのA案（**外側リポジトリで SDK固有部を管理＋asp3_core を submodule＋
`ASP3_TARGET_DIR` で受け入れ**）を踏襲する。

## 現状（2026-06-12 調査）

### 既存移植：genuine ASP3 の MIMXRT685-EVK 移植が手元にある

`/home/honda/TOPPERS/FMP3/posix/asp3_3.7/`（TTSP3/back にも同系）：
`target/mimxrt685evk_gcc` ＋ `arch/arm_m_gcc/imxrt600`（target_user.txt付き・
実機ROM実行の動作実績・gcc 11.3）。ただし：

- **MCUXpresso SDK 未使用**＝ベアメタル自己完結（自前 `IMXRT600.h`・
  `flash_config.c`＝FlexSPI設定ブロック・専用ld・`start_imxrt600.S`・tUsart）。
  MCUXpresso IDE（SDK 2.11.0）はGUIデバッグ用の任意利用にとどまる。
- **旧世代ASP3（3.7.0）対応**＝TECS（tUsart.cdl・target.cdl）＋Ruby cfg（.trb）。
  **現 asp3_core（3.7.2・非TECS・Python cfg）ではそのまま使えない**。

→ 位置づけ：**直接流用不可。非TECS＋Python cfg＋3.7.2規約への変換ベース／
参照資料**（レジスタ定義・flash_config・ld・タイマ/UART実体・割込み番号・
実機手順）。同種変換は stm32mp2・asp3_stm32cube で確立済み（3例目）。

### 他SDK統合との比較（楽な点・特有の点）

| 観点 | NXP/MCUXpresso |
|---|---|
| ツールチェーン | **arm-none-eabi gcc（導入済・既存と同じ）**。SDKもarmgcc公式サポート |
| SDK取得 | **git/zipで取得可**（GitHub: nxp-mcuxpresso）。CubeMX/RASCのような**GUI生成必須ではない**（Config Toolsは任意）→ 生成依存は4SDK中最軽量の見込み |
| ボード特有 | **Flash XIP実行**＝FlexSPI設定ブロック（flash_config）必須・TrustZone（CM33）・HiFi4 DSPは不使用（停止のまま） |
| 実機書込み | オンボードMCU-Link（CMSIS-DAP/J-Link化可）。既存移植のuser.txt手順を踏襲 |

## 実施プラン（2段階）

### Phase A：ベアメタルターゲットを asp3_core 本体に追加
既存移植を変換して `arch/arm_m_gcc/imxrt600`＋`target/mimxrt685evk_gcc` を追加：

1. TECS→非TECS変換（tUsart→`chip_serial.[ch]`系・syssvcは既存非TECS版）・
   Ruby cfg→Python（.trb→.py）・3.7.0→3.7.2規約（TMAX_*）への更新
2. CMake化（target.cmake/chip.cmake・presets.json＝命名規則どおり `mimxrt685evk`）
3. **SDK生成依存がないため CI ビルドジョブ追加可**（CubeMX/RASCと違う利点）
4. 検証：ビルド（CI）→ 実機で test_porting 6/6 → sample1 → testexec → dlynse較正
5. OS Awareness（arm_m共通NVIC層が既存＝target層のみ）・osdebug

### Phase B：MCUXpresso SDK統合（外側リポジトリ）
0. リポジトリ名の決定。候補（既存の `asp3_<SDK名>` 命名に整合）：
   - **`asp3_mcuxpresso`**：SDK正式名。最も説明的だが長い
   - **`asp3_mcuxsdk`**：NXP公式GitHubの現行SDKリポジトリ名（mcuxsdk）に一致
   - **`asp3_mcux`**：最簡潔（SDK自身の略称 MCUX）
1. 外側リポジトリ：asp3_core submodule＋
   `ASP3_TARGET_DIR`＋`ASP3_LIBRARY_ONLY`（確立済みの受け入れ口）
2. SDKのfsl_ドライバ・スタートアップとの調停：割込み登録の主導権
   （picoの`--wrap`方式 or FSP/STM32の「ASP3がNVIC掌握・SDKは素直に従う」方式の
   どちらが要るかをSDKのIRQ APIで判断）・SysTick/OSタイマの競合
3. SDKバージョンは**現行（最新）に合わせる**（STM32と同方針）
4. 実機検証＋移植skill（porting-asp3-to-nxp 等）を外側リポジトリに

> Phase A が完成すればハード知識（XIP・クロック・タイマ・UART）が
> 3.7.2/CMake環境で検証済みになり、Phase B はSDK協調の差分に集中できる
> （pico＝ベアメタル先行→SDK統合、と同じ段取り）。

## 必要な環境

| 要素 | 状況 |
|---|---|
| arm-none-eabi-gcc | 導入済 |
| **EVK-MIMXRT685 実機** | **保有（2026-06-12確認）** |
| MCUXpresso SDK | Phase B。git/zip・現行版に合わせる |
| MCUXpresso IDE / VS Code拡張 | 任意（デバッグGUI） |

## スコープ外

- HiFi4 DSP（マルチコア・別OS連携）
- RT685以外のNXPボード（LPC55S69等＝既存移植はあるが別項目）
- QEMU対応（RT685のQEMUモデルは想定しない）

## リスク・確認事項

- **XIP（Flash実行）**：flash_config（FlexSPI設定）・キャッシュ・XIP中のフラッシュ
  書換え不可制約。既存移植が動作実績を持つ構成を踏襲する
- TrustZone（CM33）：既存移植のSecure設定を踏襲（pico2_arm/mps2 と同様の論点）
- 3.7.0→3.7.2 の規約差（TMAX_*・割込み管理）＝stm32mp2変換で確立した変換規則を適用
- Phase B のSDK startup（ResetISR・SystemInit）と ASP3 ブートの調停

## 実施結果

（完了時に記載）
