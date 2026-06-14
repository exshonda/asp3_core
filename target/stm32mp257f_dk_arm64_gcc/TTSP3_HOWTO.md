# STM32MP257F-DK（Cortex-A35 / AArch64）実機での TTSP3 適合性テスト

TOPPERS テストスイート **TTSP3** の API 適合性テスト（`api_test`）を **STM32MP257F-DK 実機**
（STM32MP257F, Cortex-A35, AArch64 セキュア EL1）で実行・自動判定する手順と**実機結果**。

- ドライバ（QEMU/実機共通）：`test/ttsp/run_ttsp.py`（`--target stm32mp257f_dk_arm64_hw`）
- 設計・QEMU 結果（正本）：`docs/dev/ttsp3-conformance.md`（**本書は実機側に特化**）
- **TTSP3 は読み取り専用**（一切改変しない）。実機ロードは本依存部の既存
  `run.cmake` の `swd-run` ターゲット（OpenOCD/SWD）を再利用する。
- 参照実装：Zybo Z7 実機版 `target/zybo_z7_gcc/TTSP3_HOWTO.md`（xsct 方式）。本ボードは
  SWD（OpenOCD + GDB）運用のため、実機ローダのみ `openocd_swd` 方式に差し替えている。

---

## 1. 方式（OpenOCD / SWD）

QEMU の代わりに **OpenOCD/SWD で実機にロード＆実行**し、UART(`/dev/ttyACM0`)を別途
キャプチャして `All check points passed.` で判定する（系統分類・ttg 生成・判定ロジックは
QEMU 版と共通）。

- 実機ロードは本依存部の `run.cmake` が定義する **`swd-run`** ターゲット
  （OpenOCD だけで `reset run`→examine/halt→`load_image`→`reg pc`→`resume`→`shutdown`）を
  `ninja -C <build_dir> swd-run` で**そのまま再利用**する（TTSP3 専用のロード資産は追加しない）。
  OpenOCD は完了で自動終了し、ボードは走り続ける。
- `reset run` が毎回 FSBL から再起動（DDR 初期化→landing pad）するため、連続再ロードが
  そのまま可能。前テストの UART 残骸は**最後のカーネルバナー以降に限定**して除去する
  （`_current_boot`）。

### 構成（asp3_core 側に追加したもの）

| 追加物 | 役割 |
|---|---|
| `target/stm32mp257f_dk_arm64_gcc/ttsp3/ttsp_target_test.{c,h}` | TTSP ターゲットテスト資産（AArch64＝zcu102_arm64_gcc と同形。GICv2 SPI=32〜・`svc #0xF000` 例外）。TTSP3 は本ボード用 ASP ターゲットを同梱しないため asp3_core 側に新設 |
| `target/stm32mp257f_dk_arm64_gcc/ttsp3/ttsp_target.cfg` | 空（TECS 結線は asp3_core の自動 `tecsgen.cfg` スタブが吸収） |
| `run_ttsp.py` の `TARGETS["stm32mp257f_dk_arm64_hw"]` ＋ `run_hw_openocd_swd()` | 実機ランナー（`swd-run` 再利用＋UART キャプチャ＋判定） |

ドライバの `TARGETS["stm32mp257f_dk_arm64_hw"]["hw"]` に serial / baud / capture を集約。
`$TTSP_HW_SERIAL` / `$TTSP_HW_CAPTURE` で上書き可。

### 前提

| 必要物 | 本開発機での所在 |
|---|---|
| aarch64-none-elf gcc 14.3 | `/usr/local/tools/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf/bin` を **PATH へ**（`swd-run` が readelf を PATH 経由で使う） |
| OpenOCD 0.12.0 | apt（同梱 `openocd/` で STM32MP25x 設定を供給） |
| Ruby（functional yaml の ttg 生成） | apt（3.2） |
| STM32MP257F-DK 実機 | ST-LINK V3（`0483:3753`）。**UART=`/dev/ttyACM0`** 115200 8N1（ttyACM0=interface01 / ttyACM1=interface04。sample1 で出る方を確定） |
| FSBL + landing pad 入り SD | `target_user.md` §3-4 で作成。ボードを **"Connect using OpenOCD" でハング**させておく |

---

## 2. 実行

```bash
export PATH=/usr/local/tools/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf/bin:$PATH
export TTSP3_ROOT=/home/honda/TOPPERS/ASP3CORE/ttsp3   # この開発機のTTSP3パス（../ttsp3）

# 1件だけ（疎通確認）
python3 test/ttsp/run_ttsp.py --target stm32mp257f_dk_arm64_hw \
  api_test/ASP/staticAPI/CRE_TSK/CRE_TSK_g-3
# staticAPI 全件（error=cfgのみ／runtime・yaml=実機）
python3 test/ttsp/run_ttsp.py --target stm32mp257f_dk_arm64_hw --tap api_test/ASP/staticAPI
# 機能テスト1モジュール（代表スポット確認）
python3 test/ttsp/run_ttsp.py --target stm32mp257f_dk_arm64_hw --tap api_test/ASP/semaphore
```

- **逐次実行のみ**（シリアル1本＝実機1台）。`--jobs` は使わない。
- error 系は cfg がエラーを出してビルド失敗すれば PASS＝**実機不要・QEMU 版と同一**。
- 実機実行は1件あたり ~15–30s（cfg+ビルド＋`swd-run` の `reset run` 待ち ~8s＋完走検出で
  早期打切り）。functional 全 1813 件の実機通しは時間的に非現実的（QEMU 版でカバー済み）。
  実機は staticAPI と代表モジュールのスポット確認に用いる。

### 仕組み（run_hw_openocd_swd）

1. `stty` で UART を raw 設定し、`timeout <capture> cat /dev/ttyACM0` を背景起動。
2. `ninja -C <build_dir> swd-run` を同期実行（OpenOCD が `reset run`→load→`resume`→`shutdown`。
   ボードは走り続ける）。
3. UART ログを **最後のカーネルバナー以降**に限定して（`reset run` で再出力される TF-A/
   landing pad ログ・前テストの残骸を除去）`All check points passed.` を検出。検出で早期に
   cat を停止（`_current_boot`）。

---

## 3. 実機結果（STM32MP257F-DK）

> 環境：本開発機（Ubuntu 24.04）＋ STM32MP257F-DK 実機。aarch64-none-elf 14.3 / OpenOCD 0.12 /
> Ruby 3.2。**TTSP3・asp3_core 本体（kernel/・syssvc/・cfg/・target 既存部）とも変更なし**＝
> TTSP ターゲット資産（`target/stm32mp257f_dk_arm64_gcc/ttsp3/`）とドライバの実機ランナー
> 追加のみ。実機ロードは既存 `run.cmake` の `swd-run` を再利用。

| 範囲 | 件数 | 結果 | 日付 |
|---|---|---|---|
| `api_test/ASP/staticAPI` 全件（error 113＋runtime 5＋yaml 20） | 138 | **PASS 128・FAIL 1・SKIP 9** | 2026-06-15 |
| `api_test/ASP/semaphore`（functional・代表スポット） | 117 | （実行中／別記） | 2026-06-15 |

```
== TTSP3 [stm32mp257f_dk_arm64_hw] PASS=128 FAIL=1 SKIP=9 / 138 ==
```

### staticAPI の FAIL / SKIP の仕分け

- **SKIP 9（HW 依存・既知）**：`int_raise`（CFG_INT_d-1/d-2・CRE_ISR_d・DEF_INH_e）・
  `gain_tick`（CRE_ALM_f・CRE_CYC_h/i-1/i-2）・`cpuexc_raise`（DEF_EXC_d）。割込み/例外/早送り
  HW を呼ぶテストで、AArch64＝zcu102/mps2/polarfire と同じ SKIP 規則（`unsupported_hw`）。

- **FAIL 1（ターゲット依存の cfg 差・既知）**：`CRE_TSK/CRE_TSK_a`（E_RSATR 期待）。
  本テストは `tskatr=8`（0x08）を「不正ビット」とみなし E_RSATR を期待するが、**arm64 依存部では
  `TARGET_TSKATR = TA_FPU = 0x08`**（`arch/arm64_gcc/common/core_kernel.h`/`core_kernel_impl.h`）
  であり、cfg のシンボル抽出時にも `TARGET_TSKATR=0x08` が解決される（生成 `cfg1_out.elf` の
  `TOPPERS_cfg_TARGET_TSKATR` の .rodata 実値＝`0x08`）。よって `tskatr=8`(=TA_FPU) は**有効な
  タスク属性**となり、cfg は正しく E_RSATR を出さずビルドが成功する＝TTSP3 の期待（参照ターゲット
  基準）と不一致になる。
  - 対照：TTSP3 参照ターゲット zybo（arm）では同じ cfg 抽出で `TOPPERS_cfg_TARGET_TSKATR=0`
    （.rodata 実値＝`0`）となるため `tskatr=8` は不正＝E_RSATR を検出し PASS する（QEMU で確認）。
  - 位置づけ：TTSP3 の `err_code.txt` が参照ターゲット基準であることによる**ターゲット依存の
    静的 API エラーコード差**（mps2/polarfire の staticAPI 差と同種）。arm64 では TA_FPU を
    正しく有効属性と認識している＝**機能適合性とは別問題**。同じ arm64 の zcu102_arm64 でも
    同様の差が出る見込み（`TARGET_TSKATR=TA_FPU`）。kernel/・arch/ は不変とする。

---

## 4. トラブルシュート

| 症状 | 対処 |
|---|---|
| `swd-run` が `readelf` を見つけられない | aarch64-none-elf を **PATH** へ（`run_ttsp.py` の実行環境に）。`A35_TOOLCHAIN_PREFIX` は相対名のため PATH 必須 |
| OpenOCD が ST-LINK を掴んだまま | `pkill -x openocd`（`-f` は使わない＝コマンド文字列に当たり自分のシェルを殺す） |
| UART が無出力 | `/dev/ttyACM0`/`ttyACM1` の別を確認（ttyACM0=interface01）。`$TTSP_HW_SERIAL` で上書き |
| `Connect using OpenOCD` のまま進まない | SD（FSBL+landing pad）未挿入/未準備。`target_user.md` §4-5 を参照 |
| 前テストの残骸で誤 PASS | バナー以降限定で対策済（`_current_boot`）。capture を延ばす場合は `$TTSP_HW_CAPTURE` |
| `Failed to write memory at 0x80000fe4` 警告 | ベアメタル FSBL に ST デバッグ待ち合わせが無いことによるもので**無害**（同梱 OpenOCD 設定で catch 済み） |

---

## 同梱物とライセンス

| 同梱物 | 内容 | ライセンス |
|---|---|---|
| `target/stm32mp257f_dk_arm64_gcc/ttsp3/ttsp_target_test.{c,h}` | TTSP ターゲットテスト資産（AArch64・zcu102_arm64_gcc と同形） | TOPPERS ライセンス（新規作成） |
| `target/stm32mp257f_dk_arm64_gcc/ttsp3/ttsp_target.cfg` | 空（自動 tecsgen.cfg スタブが結線を吸収） | TOPPERS ライセンス |

> 実機ロードは本依存部の既存 `run.cmake`（`swd-run`）・`openocd/`・`minimal_boot/` を再利用する
> （TTSP3 専用のロード資産は追加しない）。それらの所在・ライセンスは `target_user.md`
> 「同梱物とライセンス」を参照。
