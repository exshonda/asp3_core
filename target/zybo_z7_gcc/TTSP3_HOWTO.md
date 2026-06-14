# Zybo Z7（Cortex-A9）実機での TTSP3 適合性テスト

TOPPERS テストスイート **TTSP3** の API 適合性テスト（`api_test`）を **Zybo Z7 実機**
（XC7Z020, Cortex-A9, Zynq-7000）で実行・自動判定する手順と**実機結果**。

- ドライバ（QEMU/実機共通）：`test/ttsp/run_ttsp.py`（`--target zybo_z7_hw`）
- 設計・QEMU 結果（正本）：`docs/dev/ttsp3-conformance.md`（**本書は実機側に特化**）
- **TTSP3 は読み取り専用**（一切改変しない）。実機ロード資産は本フォルダ
  `target/zybo_z7_gcc/ttsp3/` に置く。
- 移植元：FMP3 `target/zybo_z7_gcc/TTSP3_HOWTO.md` の **A 方式（xsct）**。ASP3 は
  シングルプロセッサのため Cortex-A9 **#0 のみ**を扱うよう簡略化。

---

## 1. 方式（xsct / Vitis）

QEMU の代わりに **xsct(Vitis) で実機にロード＆実行**し、UART(`/dev/ttyUSB1`)を別途
キャプチャして `All check points passed.` で判定する（系統分類・ttg 生成・判定ロジックは
QEMU 版と共通）。

- `rst -system` が毎回コア/キャッシュをクリーンにするため、openocd 方式（B）で必要だった
  **キャッシュ正規化スタブ・SMP 回避は不要**（ASP3 はシングルコア）。連続再ロードがそのまま可能。
- xsct に **hw_server を自動起動**させる（`xsct script.tcl` を直接実行。手動 attach だと
  FT2232 を列挙しないことがある）。

### 構成ファイル

| ファイル | 役割 |
|---|---|
| `ttsp3/jtag.tcl` | xsct 起動スクリプト（`rst -system`→`ps7_init`→`dow`→`con`→`exit`）。引数 `<ps7_init.tcl> <ELF>`。Cortex-A9 #0 のみ |
| `xilinx_sdk/zybo_z7_hw/ps7_init.tcl` | DDR/クロック初期化（ボード同梱・既存） |

ドライバの `TARGETS["zybo_z7_hw"]["hw"]` に serial / baud / vitis_settings / capture を集約。
`$TTSP_HW_SERIAL` / `$TTSP_HW_CAPTURE` で上書き可。

### 前提

| 必要物 | 本開発機での所在 |
|---|---|
| Vitis 2024.2（xsct/hw_server） | `/usr/local/tools/Vitis/2024.2`（`settings64.sh` を source） |
| arm-none-eabi gcc | apt（13.2） |
| Ruby（functional yaml の ttg 生成） | apt（3.2） |
| Zybo Z7 実機（JP5=JTAG） | FT2232（`0403:6010`, Digilent）。JTAG=`/dev/ttyUSB0`, **UART=`/dev/ttyUSB1`** 115200 |

---

## 2. 実行

```bash
export TTSP3_ROOT=/home/honda/TOPPERS/ASP3CORE/ttsp3   # この開発機のTTSP3パス（../ttsp3）
# staticAPI 全件（error=cfgのみ／runtime・yaml=実機）
python3 test/ttsp/run_ttsp.py --target zybo_z7_hw --tap api_test/ASP/staticAPI
# 機能テスト1モジュール
python3 test/ttsp/run_ttsp.py --target zybo_z7_hw --tap api_test/ASP/semaphore
# 1件だけ
python3 test/ttsp/run_ttsp.py --target zybo_z7_hw api_test/ASP/staticAPI/CRE_TSK/CRE_TSK_g-3
```

- **逐次実行のみ**（シリアル1本＝実機1台）。`--jobs` は使わない。
- error 系は cfg がエラーを出してビルド失敗すれば PASS＝**実機不要・QEMU 版と同一**。
- 実機実行は1件あたり ~15s（xsct 起動 + `rst -system` + ロード + 完走検出で早期打切り）。
  functional 全 1813 件の実機通しは時間的に非現実的（QEMU 版でカバー済み）。実機は
  staticAPI と代表モジュールのスポット確認に用いる。

### 仕組み（run_hw）

1. `stty` で UART を raw 設定し、`timeout <capture> cat /dev/ttyUSB1` を背景起動。
2. `source settings64.sh; xsct ttsp3/jtag.tcl <ps7_init> <ELF>` を実行（con 後すぐ exit。
   ボードは走り続ける）。
3. UART ログを **最後のカーネルバナー以降**に限定して（`rst -system` が効くまで残る前テストの
   出力を除去）`All check points passed.` を検出。検出で早期に cat を停止（`_current_boot`）。

---

## 3. 実機結果（Zybo Z7）

> 環境：本開発機（Ubuntu 24.04）＋ Zybo Z7 実機。Vitis 2024.2 / arm-none-eabi 13.2 /
> Ruby 3.2。**TTSP3・asp3_core 本体（kernel/・syssvc/・cfg/・target 既存部）とも変更なし**＝
> 実機ロード資産（`ttsp3/jtag.tcl`）とドライバの実機ランナー追加のみ。

| 範囲 | 件数 | 結果 | 日付 |
|---|---|---|---|
| `api_test/ASP/staticAPI` 全件（error 113＋runtime 5＋yaml 20） | 138 | **138/138 PASS**（FAIL 0・SKIP 0） | 2026-06-15 |
| `api_test/ASP/semaphore`（functional・全 yaml） | 117 | **117/117 PASS**（FAIL 0・SKIP 0） | 2026-06-15 |

```
== TTSP3 [zybo_z7_hw] PASS=138 FAIL=0 SKIP=0 / 138 ==   # staticAPI
== TTSP3 [zybo_z7_hw] PASS=117 FAIL=0 SKIP=0 / 117 ==   # semaphore（functional 代表）
```

- **QEMU 版の staticAPI 138/138 と完全一致**。zybo は TTSP3 の参照ターゲットのため、mps2/
  polarfire で target 依存差により FAIL していた staticAPI エラー系（`CFG_INT`/`DEF_ICS`/
  `CRE_ALM`/`CRE_CYC` 等）も全 PASS、`DEF_ICS_d-2`（mps2 でビルド不成立）も実機 runtime で PASS。
- **functional 代表モジュール `semaphore` を実機で全 117 件 PASS**（SKIP 0＝zybo は全 HW 対応）。
  zybo は全 HW 対応のため `gain_tick`/`int_raise`/`cpuexc` の SKIP も無く、`_ntc`/`_ten` 変種も
  含めて実走行する。他モジュールも同様に `--target zybo_z7_hw` でスポット確認可能。

---

## 4. トラブルシュート

| 症状 | 対処 |
|---|---|
| `[zynq.cpu0] not halted` / hw_server が掴んだまま | `pkill -x hw_server`（`-f` は使わない＝コマンド文字列に当たり自分のシェルを殺す） |
| UART が無出力 | ポート番号（`/dev/ttyUSB1`）を `udevadm info` で確認。`$TTSP_HW_SERIAL` で上書き |
| 前テストの残骸で誤 PASS | バナー以降限定で対策済（`_current_boot`）。capture を延ばす場合は `$TTSP_HW_CAPTURE` |
| xsct が見つからない | `source /usr/local/tools/Vitis/2024.2/settings64.sh`。所在は `hw["vitis_settings"]` |

---

## 同梱物とライセンス

| 同梱物 | 内容 | ライセンス |
|---|---|---|
| `ttsp3/jtag.tcl` | xsct 実機ロード（ASP3 シングルコア版。FMP3 `xilinx_sdk/jtag.tcl` を基に作成） | TOPPERS ライセンス |
| `xilinx_sdk/zybo_z7_hw/ps7_init.tcl` | DDR/クロック初期化（ボード同梱・既存） | （Xilinx 由来） |
