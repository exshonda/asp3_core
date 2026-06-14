# Raspberry Pi PICO2 ARM（Cortex-M33 / rp2350）実機での TTSP3 適合性テスト

TOPPERS テストスイート **TTSP3** の API 適合性テスト（`api_test`）を **PICO2 実機**
（rp2350, Cortex-M33, ARM）で実行・自動判定する手順と**実機結果**。

- ドライバ（QEMU/実機共通）：`test/ttsp/run_ttsp.py`（`--target pico2_arm_hw`）
- 設計・QEMU 結果（正本）：`docs/dev/ttsp3-conformance.md`（**本書は実機側に特化**）
- **TTSP3 は読み取り専用**（一切改変しない）。実機ロードは本依存部の既存
  `run.cmake` の `run` ターゲット（OpenOCD/CMSIS-DAP）を再利用する。
- 参照実装：STM32MP257F-DK 実機版（`target/stm32mp257f_dk_arm64_gcc/TTSP3_HOWTO.md`）と
  同じ「既存 ninja ターゲット再利用＋UART キャプチャ」方式（`run_hw_openocd_swd`）。
  ninja ターゲットのみ `run`（program verify reset exit）に差し替え。

---

## 1. 方式（OpenOCD / CMSIS-DAP）

QEMU の代わりに **OpenOCD（CMSIS-DAP/Debugprobe）で実機フラッシュに書込み＆実行**し、
UART(`/dev/ttyACM2`)を別途キャプチャして `All check points passed.` で判定する
（系統分類・ttg 生成・判定ロジックは QEMU 版と共通）。

- 実機ロードは本依存部 `run.cmake` の **`run`** ターゲット
  （`openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "program <elf> verify reset exit"`）を
  `ninja -C <build_dir> run` で**そのまま再利用**する（TTSP3 専用のロード資産は追加しない）。
  OpenOCD は完了で自動終了し、ボードは走り続ける。
- `program ... verify reset exit` が毎回フラッシュをクリーンに書き換えて reset するため、
  連続再書込みがそのまま可能。前テストの UART 残骸は**最後のカーネルバナー以降**に限定して
  除去する（`_current_boot`）。

### 構成（asp3_core 側に追加したもの）

| 追加物 | 役割 |
|---|---|
| `target/pico2_arm_gcc/ttsp3/ttsp_target_test.{c,h}` | TTSP ターゲットテスト資産（Cortex-M33＝mps2_an521_gcc と同形。`udf #0` 例外・`get_stk`マクロ=USE_TSKINICTXB）。TTSP3 は本ボード用 ASP ターゲットを同梱しないため asp3_core 側に新設 |
| `target/pico2_arm_gcc/ttsp3/ttsp_target.cfg` | 空（TECS 結線は asp3_core の自動 `tecsgen.cfg` スタブが吸収） |
| `target/pico2_arm_gcc/ttsp3/ttsp_target.py` | ドライバが自動探索する TTSP 定義（`run` 再利用・serial=/dev/ttyACM2） |

ドライバの `TTSP_TARGETS["pico2_arm_hw"]["hw"]` に serial / baud / capture / ninja_target を集約。
`$TTSP_HW_SERIAL` / `$TTSP_HW_CAPTURE` で上書き可。

### 前提

| 必要物 | 本開発機での所在 |
|---|---|
| arm-none-eabi gcc | apt（13.2）。**PATH へ**（`run` が readelf/program 用に使う） |
| OpenOCD（RPi フォーク） | `/usr/local`（rp2350 対応） |
| Ruby（functional yaml の ttg 生成） | apt（3.2） |
| PICO2 実機 + Debugprobe | Debugprobe（`2e8a:000c` CMSIS-DAP）経由で接続。**UART=`/dev/ttyACM2`**（Debugprobe VCP）115200 |

> 注：本開発機では STLINK-V3 が `/dev/ttyACM0`・`ttyACM1` を占有するため、Debugprobe の
> VCP は `/dev/ttyACM2` になる。`udevadm info -q property -n /dev/ttyACM2` で
> `ID_MODEL=Debugprobe...` を確認すること。番号が変わる環境では `$TTSP_HW_SERIAL` で上書き。

---

## 2. 実行

```bash
export TTSP3_ROOT=/home/honda/TOPPERS/ASP3CORE/ttsp3   # この開発機のTTSP3パス（../ttsp3）

# 1件だけ（疎通確認）
python3 test/ttsp/run_ttsp.py --target pico2_arm_hw \
  api_test/ASP/staticAPI/CRE_TSK/CRE_TSK_g-3
# staticAPI 全件（error=cfgのみ／runtime・yaml=実機）
python3 test/ttsp/run_ttsp.py --target pico2_arm_hw --tap api_test/ASP/staticAPI
# 機能テスト1モジュール（代表スポット確認）
python3 test/ttsp/run_ttsp.py --target pico2_arm_hw --tap api_test/ASP/semaphore
```

- **逐次実行のみ**（シリアル1本＝実機1台）。`--jobs` は使わない。
- error 系は cfg がエラーを出してビルド失敗すれば PASS＝**実機不要・QEMU 版と同一**。
- 実機実行は1件あたり ~15–25s（cfg+ビルド＋`program`（フラッシュ書込み）＋完走検出で
  早期打切り）。functional 全 1813 件の実機通しは時間的に非現実的（QEMU 版でカバー済み）。
  実機は staticAPI と代表モジュールのスポット確認に用いる。

### 仕組み（run_hw_openocd_swd）

1. `stty` で UART を raw 設定し、`timeout <capture> cat /dev/ttyACM2` を背景起動。
2. `ninja -C <build_dir> run` を同期実行（OpenOCD が `program verify reset exit`。
   フラッシュ書込み→reset→起動。ボードは走り続ける）。
3. UART ログを **最後のカーネルバナー以降**に限定して `All check points passed.` を検出。
   検出で早期に cat を停止（`_current_boot`）。

---

## 3. 実機結果（PICO2 ARM）

> 環境：本開発機（Ubuntu 24.04）＋ PICO2 実機（Debugprobe）。arm-none-eabi 13.2 /
> OpenOCD（RPi フォーク）/ Ruby 3.2。**TTSP3・asp3_core 本体（kernel/・syssvc/・cfg/・
> target 既存部）とも変更なし**＝TTSP ターゲット資産（`target/pico2_arm_gcc/ttsp3/`）と
> ドライバの ninja ターゲット・パラメータ化のみ。実機ロードは既存 `run.cmake` の `run` を再利用。

| 範囲 | 件数 | 結果 | 日付 |
|---|---|---|---|
| `api_test/ASP/staticAPI` 全件（error 113＋runtime 5＋yaml 20） | 138 | **PASS 110・FAIL 19・SKIP 9** | 2026-06-15 |
| `api_test/ASP/semaphore`（functional・代表スポット） | 117 | **PASS 73・FAIL 0・SKIP 44** | 2026-06-15 |

```
== TTSP3 [pico2_arm_hw] PASS=110 FAIL=19 SKIP=9 / 138 ==   # staticAPI
== TTSP3 [pico2_arm_hw] PASS=73  FAIL=0  SKIP=44 / 117 ==  # semaphore（functional 代表）
```

- **functional 代表 `semaphore` は FAIL 0**（実行できた全件 PASS）。SKIP 44 は HW タイマ早送り
  （`gain_tick`）等に依存する timeout 系・`_ntc`/`_ten` 変種で、Cortex-M33＝mps2 と同じ SKIP 規則。
  zybo（全 HW 対応）が SKIP 0 で通すのに対し、arm_m では HW 依存分を SKIP し純カーネル系をカバーする。

### staticAPI の FAIL / SKIP の仕分け

- **SKIP 9（HW 依存・既知）**：`int_raise`（CFG_INT_d-1/d-2・CRE_ISR_d・DEF_INH_e）・
  `gain_tick`（CRE_ALM_f・CRE_CYC_h/i-1/i-2）・`cpuexc_raise`（DEF_EXC_d）。割込み/例外/早送り
  HW を呼ぶテストで、Cortex-M33＝mps2 と同じ SKIP 規則（`unsupported_hw`）。

- **FAIL 19（ターゲット依存の cfg エラーコード差・既知）**：すべて error/runtime 系で、
  割込み/例外/ICS/通知の静的 API に集中する。TTSP3 の `err_code.txt` は参照ターゲット
  （zybo＝arm）基準であり、arm_m（Cortex-M33）とは割込み/例外モデル・属性の有効ビットが
  異なるため、cfg のエラーコードが一致しない（**機能適合性とは別問題**）。**同じ arm_m の
  mps2_an521（QEMU）と同種の差**（mps2 は 28–29 件）。
  - 通知属性 E_PAR 未検出：`CRE_ALM_d-1..5`・`CRE_CYC_d-1..5`（10件）
  - 割込み/例外/ICS：`CFG_INT_b-2-1`（E_PAR↔E_OBJ）・`DEF_EXC_a`（E_RSATR）・`DEF_EXC_b`・
    `DEF_ICS_b`・`DEF_INH_c`（E_PAR）
  - 属性/パラメータ：`ATT_INI_b`・`ATT_TER_b`・`CRE_TSK_b`（E_PAR 未検出）
  - `DEF_ICS_d-2`（runtime）のみビルド不成立（mps2 と一致）
  - これらは静的 API 検証の**ターゲット依存差**として記録する。kernel/・arch/ は不変とする。

---

## 4. トラブルシュート

| 症状 | 対処 |
|---|---|
| OpenOCD が Debugprobe を掴んだまま／次回 `Unsupported DTM` 等 | `pkill -x openocd`（`-f` は使わない＝コマンド文字列に当たり自分のシェルを殺す） |
| UART が無出力 | `/dev/ttyACM2` が Debugprobe VCP か `udevadm` で確認（STLINK 併設時は番号がずれる）。`$TTSP_HW_SERIAL` で上書き |
| `program` が readelf/openocd を見つけられない | arm-none-eabi を **PATH** へ。OpenOCD は RPi フォーク（rp2350 対応）を使う |
| 前テストの残骸で誤 PASS | バナー以降限定で対策済（`_current_boot`）。capture を延ばす場合は `$TTSP_HW_CAPTURE` |
| staticAPI error 系が FAIL | §3 既知のターゲット依存 cfg エラーコード差（mps2 と同種） |

---

## 同梱物とライセンス

| 同梱物 | 内容 | ライセンス |
|---|---|---|
| `target/pico2_arm_gcc/ttsp3/ttsp_target_test.{c,h}` | TTSP ターゲットテスト資産（Cortex-M33・mps2_an521_gcc と同形） | TOPPERS ライセンス（新規作成） |
| `target/pico2_arm_gcc/ttsp3/ttsp_target.cfg` | 空（自動 tecsgen.cfg スタブが結線を吸収） | TOPPERS ライセンス |

> 実機ロードは本依存部の既存 `run.cmake`（`run`）を再利用する（TTSP3 専用のロード資産は
> 追加しない）。OpenOCD 設定・接続の詳細は `target_user.md` を参照。
