# TTSP3 ドライバ `test/ttsp/run_ttsp.py`（asp3_core 固有）

TTSP3（TOPPERSテストスイート）を**改変せず読み取り専用**で使い、asp3_core の CMake+QEMU で
回す外部ドライバ。TTSP3 のテスト資産（`api_test/ASP/.../out.{c,cfg,h}` ＋ テストライブラリ）を
asp3_core の「アプリ」としてビルドし QEMU 実行する（Strategy B）。経緯は
`docs/dev/ttsp3-conformance.md`、利用マニュアルは `test/ttsp/README.md`。

## 基本コマンド

```bash
# functional 全件（ttg で out.* 生成→build→QEMU）
python3 test/ttsp/run_ttsp.py --target mps2_an505 --tap api_test/ASP
# 静的API
python3 test/ttsp/run_ttsp.py --target mps2_an505 --tap api_test/ASP/staticAPI
# functional だけ／error 系だけ等、系統を絞る
python3 test/ttsp/run_ttsp.py --target mps2_an505 --tap --only yaml api_test/ASP
```

## オプション

| オプション | 意味 |
|---|---|
| `--target <name>` | 対象ターゲット（mps2_an386 / mps2_an505 / mps3_an547 / zcu102_arm64 / zybo_z7 / polarfire_soc_kit / *_hw＝実機 など） |
| `paths...` | TTSP3ルート相対のテストパス（既定 `api_test/ASP`） |
| `--only <list>` | `error,runtime,yaml` のうち実行する系統（カンマ区切り） |
| `--tap` | TAP形式（`ok`/`not ok`/`1..N`）で出力 |
| `--build-dir <dir>` | ビルド/生成ディレクトリ（**並列実行時はターゲットごとに別dirを指定**） |
| `--build-only` | ビルド成功でPASS（QEMU実行をskip。qemu未導入アーキのbuild検証・CI用） |
| `--ttsp3-root <dir>` | TTSP3 ソースの場所（既定は環境内のTTSP3） |
| `--list` | 対象テストの一覧表示 |
| `--keep` | 生成物を残す |
| `-v` | 詳細出力 |

## テスト系統（class）

| 系統 | 実体 | 判定 |
|---|---|---|
| `error` | `err_code.txt` あり（cfgエラー系） | cfg出力 vs `err_code.txt`（QEMU不要） |
| `runtime` | `out.c` あり・`err_code.txt` 無し | QEMU `All check points passed.` |
| `yaml`（functional） | `.yaml` から ttg(Ruby) で `out.*` 生成 | build→QEMU |

## 並列実行（サブエージェント/バックグラウンド）

ドライバ自体は逐次。**ターゲットごとに別 `--build-dir` を指定すれば完全に独立**で並列安全
（生成・ビルドとも `--build-dir` 配下に隔離される）。例：

```bash
python3 test/ttsp/run_ttsp.py --target mps2_an505 --tap --build-dir build/ttsp-an505 --only yaml api_test/ASP &
python3 test/ttsp/run_ttsp.py --target mps2_an386 --tap --build-dir build/ttsp-an386 --only yaml api_test/ASP &
python3 test/ttsp/run_ttsp.py --target mps3_an547 --tap --build-dir build/ttsp-an547 --only yaml api_test/ASP &
```

> 同一 build-dir を複数で共有するとレースで flaky になる。必ず別 build-dir。

## 結果（PASS/FAIL/SKIP）の読み方

末尾サマリ：`== TTSP3 [<target>] PASS=.. FAIL=.. SKIP=.. / .. ==`。

- **SKIP**：未対応HW依存（`ttsp_target_gain_tick`/`stop_tick`＝HWタイマ制御、`ttsp_int_raise`、
  `ttsp_cpuexc_raise` 等）。想定内。
- **FAIL**：functional は重く見る（FAIL=0 が gate）。staticAPI のFAILは「ターゲット非依存の
  cfgエンジン差」なら既知差分（複数ターゲットでFAIL内訳が一致するかで判定）。
- 終了コードは FAIL があると非ゼロ（CIで利用）。

TTSP3 の概念・FAIL切り分けの一般論は skill `toppers-kernel-debug` の `conformance-ttsp3.md`。
新ターゲット追加時の手順は `test/ttsp/README.md`（`target/<name>/ttsp3/ttsp_target.py` を置く）。
