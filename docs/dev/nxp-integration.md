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
0. リポジトリ名：**`asp3_mcuxsdk`** に決定（NXP公式SDKリポジトリ名 mcuxsdk に一致）。
   **作成済み（2026-06-12）**：https://github.com/exshonda/asp3_mcuxsdk
   ＝骨格（README・asp3_core submodule・予定構成）を初期化済み。
   ローカル：`../asp3_mcuxsdk`
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

### Phase A：ベアメタルターゲット追加（2026-06-12 実装・ビルド検証済）

`/home/honda/TOPPERS/FMP3/posix/asp3_3.7/` の genuine ASP3 3.7.0 移植
（`target/mimxrt685evk_gcc`＋`arch/arm_m_gcc/imxrt600`・TECS＋Ruby cfg）を
非TECS＋Python cfg＋CMake構成へ変換し，asp3_core本体に追加した。
ブランチ：`feat/mimxrt685evk`。

#### 追加したファイル

**`arch/arm_m_gcc/imxrt600/`（チップ依存部・18ファイル）**

| ファイル | 由来・変更内容 |
|---|---|
| `IMXRT600.h` | 3.7.0版ほぼそのまま（`FIFOSTAT_TXEMPTY` ビット定義のみ追加） |
| `start_imxrt600.S` | 3.7.0版そのまま（セクションテーブル駆動のDATA/BSS初期化＝XIP用） |
| `imxrt600_usart.[ch]` | **新規**：`tUsart.c`（TECS）のレジスタ操作ロジックを rp2350_uart.[ch] と同構造の非TECSドライバへ変換。クローズ時の送信FIFOドレイン待ちを追加（pico2と同趣旨） |
| `chip_serial.{c,h,cfg}` | **新規**：非TECS SIO 中継層（rp2350の同名ファイルと同形）。`target_fput_log` 含む |
| `chip_kernel.h`・`chip_sil.h`（TBITW_IPRI=3）・`chip_stddef.h`・`chip_cfg1_out.h` | 3.7.0版そのまま |
| `chip_kernel_impl.h`・`chip_syssvc.h` | 3.7.0版から旧logtrace includeを削除（asp3_core方式）。`TNUM_PORT` 4→1 |
| `chip_rename.def`／`chip_rename.h`・`chip_unrename.h` | `INCLUDE "core"` 形式に変更し `utils/genrename.py` で再生成 |
| `chip_os_awareness.py` | **新規**：core層（NVIC）APIの再エクスポート |
| `chip.cmake` | **新規**：Makefile.chipのCMake版。`arch.cmake` include後に `start.S`→`start_imxrt600.S` を差し替え |

**`target/mimxrt685evk_gcc/`（ターゲット依存部・29ファイル）**

| ファイル | 由来・変更内容 |
|---|---|
| `flash_config.[ch]` | 3.7.0版そのまま（NXP BSD-3-Clause・FlexSPI設定ブロック＝`.flash_conf`） |
| `mimxrt685.ld`・`mimxrt685-evk.h`・`target_asm.inc`・`target_sil.h`・`target_stddef.h`・`target_kernel.h`・`target_kernel.cfg`・`target_kernel_impl.[ch]`・`target_cfg1_out.h`・`target_timer.{c,h,cfg}` | 3.7.0版そのまま（CTimer0 HRT・FBB/PMIC/PLL初期化・`raise_int` は3.7.2にも存在） |
| `target_kernel.py`・`target_check.py` | **`.trb`→`.py`変換**：`GenResVectVal`（ベクタ9＝イメージタイプ。プレーンイメージ=bit14）をPython関数化 |
| `target_syssvc.h` | 非TECS用に書き換え：`SIO_USART_BASE`/`SIO_FLEXCOMM_BASE`/`SIO_USART_INDEX`（FC0）・`INTNO_SIO` 等を追加 |
| `target_serial.{h,cfg}` | **新規**（chip層への中継） |
| `target_test.h` | `INTNO1`（CTIMER1 IRQ11＝クロック未供給の空きIRQ・`intno1_clear()`空）を追加 |
| `target_rename.def`／`target_rename.h`・`target_unrename.h` | `INCLUDE "chip"` 形式で再生成 |
| `target_os_awareness.py` | **新規**：chip層APIの再エクスポート |
| `target.cmake`・`presets.json`（プリセット名 `mimxrt685evk`） | **新規**：FPU_LAZYSTACKING相当のdefs・`-Wl,--undefined=flexspi_config`・**`TOPPERS_ENABLE_TRUSTZONE`は未定義**（プレーンイメージ起動＝EXC_RETURN 0xFFFFFFBC の実績構成。stm32h5xxと同論点） |
| `target_user.md` | `target_user.txt`（3.7.0）をMarkdown化・CMake手順に更新（JP22注意・メモリマップ・XIP解説含む） |

**その他**：ルート`CMakePresets.json`にinclude追記。削除ファイルなし
（TECS関連の `tUsart.{c,cdl}`・`target.cdl`・`tSIOPortTarget*`・`.trb`・
`Makefile.*`・`MCUXpresso/` は持ち込まない）。

#### Git情報

- ベースコミット：`96b8be5`
- 関連コミット：`feat/mimxrt685evk` ブランチの `feat(target): add mimxrt685evk` 以降
- ファイルリスト再現：`git diff --stat main feat/mimxrt685evk -- arch/arm_m_gcc/imxrt600 target/mimxrt685evk_gcc`

#### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| ビルド sample1（`--preset mimxrt685evk`） | ○ | 警告ゼロ・リンク成功（FLASH 27KB/SRAM 12KB） |
| ビルド test_porting（+`tap.c`） | ○ | 警告ゼロ・リンク成功 |
| ビルド test_int1（+`test_svc.c`） | ○ | 警告ゼロ・リンク成功 |
| ELF構造検査 | ○ | FCFBタグ@0x08000400・ベクタテーブル@0x08001000・ベクタ9=0x4000（プレーンイメージ）を確認 |
| 実機（test_porting 6/6→sample1→testexec→dlynse較正→OS Awareness） | − | **未実施（次ステップ）** |

QEMUにRT685モデルは無いため，POSIX/QEMU回帰は本ターゲットには適用外
（既存ターゲットへの影響なし＝既存ファイルの変更はCMakePresets.jsonのinclude追記のみ）。

#### DIVERGENCE_MAP との関連

`target/mimxrt685evk_gcc/`・`arch/arm_m_gcc/imxrt600/` のNEW行を追加
（PRISTINE領域への変更なし）。

#### 実機検証の手順メモ（再開点）

1. EVK-MIMXRT685 のJP22（2-3短絡推奨）・J5（MCU-Link USB）接続
2. 書込み：LinkServer（`LinkServer flash MIMXRT685S:EVK-MIMXRT685 load asp.elf`）
   または J-Link / MCUXpresso IDE。**確定した手順を `target_user.md` に反映すること**
3. test_porting（6項目TAP）→ sample1 → testexec → `SIL_DLY_TIM*` 較正確認
4. OS Awareness（target層は実装済・実機gdbで確認）

### Phase B：MCUXpresso SDK統合

未着手（`asp3_mcuxsdk` リポジトリ骨格のみ作成済）。
