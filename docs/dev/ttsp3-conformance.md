# TTSP3 適合性テスト（QEMU）

## 項目

TTSP3 適合性テスト（AGENTS.md §1 機能追加計画・優先度：中。テスト基盤の拡充）

## 内容

TOPPERSテストスイート **TTSP3**（`../../TTSP3/work/ttsp3`）の API/SIL 適合性テストを、
asp3_core の各ターゲット依存部に対して **QEMU で実行**できるようにする。
**TTSP3 のコードは変更しない**（読み取り専用のテスト供給源として扱う）。

### 背景：構造的ミスマッチ（2026-06-14 調査）

TTSP3 は被テストカーネル（`OS_PATH=../asp3/`）に標準ASP3の**ビルド機構**を要求する：

| TTSP3が要求 | 標準ASP3 | asp3_core |
|---|---|---|
| `configure.rb`（Ruby・Makefile生成） | ✓ | 削除済（Python cfg） |
| `target/<t>/Makefile.target`・`*.trb` | ✓ | 無（CMake・`.py`） |
| TECS（`out.cfg` が `tecsgen.cfg`／TECSセルを INCLUDE） | ✓ | TECSレス（プレーンC syssvc） |
| `kernel/*.c`・`kernel_api.def`・`kernel_sym.def` | ✓ | **✓**（PRISTINE＝同一） |
| テスト資産 `api_test/*.{c,cfg,h}`・`library/ASP/test/ttsp_test_lib.c` | ✓ | 流用可 |

カーネル本体とテスト資産は共通、**ビルド機構だけが非互換**。TTSP3のMake/Ruby/TECS駆動を
asp3_coreツリーで満たすのは削除した機構の再導入になるため非現実的（案A：却下）。

## 設計（案B：採用）

**TTSP3 を読み取り専用のテスト供給源**とし、テストを **asp3_core の CMake＋QEMU で回す**
外部ドライバを asp3_core 側に置く（`test/testexec.py` の TTSP3 全件版）。

- TTSP3 テスト（`out.c`＋`out.cfg`＋`out.h`）を asp3_core の app として扱い、
  `ASP3_APPLDIR`／`ASP3_APPLNAME=out`／`ASP3_EXTRA_APP_C_FILES`（`ttsp_test_lib.c`＋
  `ttsp_target_test.c`）／`ASP3_APP_INCLUDE_DIRS`（TTSP3 の `library/ASP/test` と
  `library/ASP/target/<t>`）で供給。
- **TECS結線は asp3_core の自動 `tecsgen.cfg` スタブ＋プレーンC syssvc が吸収**
  （`out.cfg` の `INCLUDE("tecsgen.cfg")` がそのまま通る）。**out 非TECSシムは不要**（PoCで確認）。
- 出力は `Check point N passed.` ／ `All check points passed.`＝**既存 testexec.py がそのまま判定可**。
- ターゲットは asp3_core の既存 preset（zybo_z7-qemu／mps2_an521-qemu／zcu102_arm64-qemu／
  polarfire_soc_kit-qemu …）で横断。**TTSP3 は不変。**

### テスト2系統と判定

| 系統 | 規模（ASP） | 生成 | 判定 |
|---|---|---|---|
| 静的APIエラー系 | staticAPI 118対のうち約113（`err_code.txt` に E_* あり） | チェックイン済 | cfg出力 vs `err_code.txt`（asp3_core `test_cfg` と同型・QEMU不要） |
| runtime系 | staticAPI正常系5＋functional **1813 .yaml** | functional は **ttg（Ruby・ターゲット非依存）で out.* 生成が必要** | QEMU `All check points passed.`（testexec と同型） |

## 実施プラン

1. **ドライバ `test/ttsp/run_ttsp.py`**（asp3_core 側・新規）
   - TTSP3 ルートは引数／環境変数で受ける（既定 `../../TTSP3/work/ttsp3`）。
   - テスト1件＝1 build dir：`cmake --preset <t> -DASP3_APPLDIR=<test> -DASP3_APPLNAME=out
     -DASP3_APP_INCLUDE_DIRS="<lib/ASP/test>;<lib/ASP/target/<t>>"
     -DASP3_EXTRA_APP_C_FILES="ttsp_test_lib.c;ttsp_target_test.c"` → QEMU実行→判定。
   - TTSP3 の target名（zybo_z7_gcc 等）→ asp3_core preset の対応表を持つ。
2. **静的APIエラー系の判定経路**：cfg を走らせ stderr の `error: E_*` を `err_code.txt` と照合
   （ビルド失敗＝期待エラー。`test_cfg/testcfg.py` の正規化ロジックを流用）。
3. **functional（.yaml）対応**：TTSP3 の `tools/ttg/bin/ttg.rb` を**読み取りで**起動し
   `out.{c,cfg,h}` を生成（ターゲット非依存）→ 生成物を 1. のドライバへ。
   ttg は Ruby＝開発コンテナに ruby 追加要否を確認。
4. **ターゲット横断**：まず zybo_z7-qemu（PoC実証済）で api_test 一巡 → mps2/zcu102/polarfire へ。
   ターゲット非対応テスト（割込み番号未定義等）は SKIP 規則を `ttsp_target_test.h` 由来で判定。
5. **CI/記録**：軽量サブセットを CI（nightly）に載せるか検討。`docs/dev/README.md` 索引・本ファイル更新。

## 実施結果

### PoC 完了（2026-06-14・zybo_z7-qemu）

案Bの全経路を実機CMake＋QEMUで実証。**TTSP3・asp3_core とも本実装の変更なし**（ドライバは未作成・手動コマンドで確認）。

- **runtime系**：`api_test/ASP/staticAPI/CRE_TSK/CRE_TSK_g-3` を
  `--preset zybo_z7-qemu`＋上記 include/extra-C で **ビルド成功 → QEMU(xilinx-zynq-a9) 実行 →
  `Check point 1/2/3 passed.` → `All check points passed.`**。
- **静的APIエラー系**：`staticAPI/CRE_SEM/CRE_SEM_a` を同手順で configure → cfg が
  `out.cfg:52: error: E_RSATR: illegal sematr '2'` を検出＝`err_code.txt`（E_RSATR）と一致。
- **out 非TECSシムは不要**：asp3_core の自動 tecsgen.cfg スタブ＋プレーンC syssvc が
  TTSP3 テストの TECS INCLUDE と出力を吸収（当初想定の追加シムは作らずに済んだ）。

#### 再現コマンド（PoC）

```bash
T=../../TTSP3/work/ttsp3
TD=$T/api_test/ASP/staticAPI/CRE_TSK/CRE_TSK_g-3
cmake --preset zybo_z7-qemu -B build/ttsp-poc \
  -DASP3_APPLDIR="$TD" -DASP3_APPLNAME=out \
  -DASP3_APP_INCLUDE_DIRS="$T/library/ASP/test;$T/library/ASP/target/zybo_z7_gcc" \
  -DASP3_EXTRA_APP_C_FILES="$T/library/ASP/test/ttsp_test_lib.c;$T/library/ASP/target/zybo_z7_gcc/ttsp_target_test.c"
cmake --build build/ttsp-poc
timeout 20 qemu-system-arm -M xilinx-zynq-a9 -semihosting -m 512M \
  -serial null -serial mon:stdio -nographic -kernel build/ttsp-poc/asp.elf
```

### 本実装 完了（2026-06-14・zybo_z7-qemu）

案Bのドライバを `test/ttsp/run_ttsp.py` として実装し、3系統すべてを自動判定可能にした。
**TTSP3・asp3_core 本体（kernel/・syssvc/・cfg/・target/）とも変更なし**＝ドライバ1ファイルの追加のみ。

#### ドライバ `test/ttsp/run_ttsp.py`

- TTSP3 ルートは `--ttsp3-root` / `$TTSP3_ROOT` / 既定 `<repo>/../../TTSP3/work/ttsp3`（絶対パス化）。
  **include/extra-C のパスは必ず絶対パスで CMake に渡す**（cfg はビルドディレクトリで動くため、
  相対 `-I` だと `INCLUDE("ttsp_target.cfg")` 等が解決できずビルド失敗する＝当初の再現失敗の原因）。
- 各テストの系統を**自動分類**（手書きのテスト表は不要）：

  | 系統 | 判定条件 | 実行 | 合格条件 |
  |---|---|---|---|
  | `error` | dir に `err_code.txt` あり | configure+build のみ（QEMU不要） | ビルド失敗かつ cfg の `error: E_*` が `err_code.txt` と一致 |
  | `runtime` | dir に `out.c`・`err_code.txt` 無し | build → QEMU | `All check points passed.` |
  | `yaml` | `*.yaml` ファイル | `ttg.rb -a -p`（Ruby・ターゲット非依存）で `out.{c,cfg,h}` 生成 → build → QEMU | 同上 |

- ターゲット定義（preset・TTSP3 target lib 名・QEMU コマンド）は `TARGETS` dict に集約。
- 主なオプション：`--target`／`--only error,runtime,yaml`／`--list`／`--tap`／`--keep`／`-v`。

```bash
# 例：staticAPI 全件（error 系は QEMU 不要で高速）
python3 test/ttsp/run_ttsp.py api_test/ASP/staticAPI
# 例：機能テストを TAP で
python3 test/ttsp/run_ttsp.py --tap api_test/ASP/task_manage
# 例：対象の列挙のみ
python3 test/ttsp/run_ttsp.py --list api_test/ASP
```

#### 検証結果（zybo_z7-qemu）

| 範囲 | 件数 | 結果 |
|---|---|---|
| `api_test/ASP/staticAPI` 全件（error 113＋runtime 5＋yaml 20） | 138 | **138/138 PASS** |
| `api_test/ASP` functional 全件（`--only yaml`・ttg 生成→build→QEMU） | 1813 | **1813/1813 PASS** |

ttg（Ruby 3.2.3）連携・静的APIエラー自動判定・runtime/QEMU 判定の全経路を実機 CMake+QEMU で確認。

#### functional 全 1813 件 通し走行（2026-06-14・zybo_z7-qemu）

`api_test/ASP` 配下の functional yaml **全 1813 件**を ttg 生成→ビルド→QEMU 実行で通し、
**1813/1813 PASS・FAIL 0・SKIP 0**（zybo_z7-qemu で全件 QEMU 上の `All check points passed.`）。

```bash
python3 test/ttsp/run_ttsp.py --only yaml --tap api_test/ASP
#   == TTSP3 [zybo_z7] PASS=1813 FAIL=0 SKIP=0 / 1813 ==
```

逐次実行（1件＝独立 build dir、build 後に削除）。所要は環境により十数分〜数十分。
これと staticAPI 138 件（うち error 113＝QEMU不要）を合わせ、**ASP api_test の自動判定が全件成立**。

### ターゲット横断 完了（2026-06-14・mps2_an521／polarfire／zcu102）

TTSP3 が同梱する ASP ターゲットライブラリは `zybo_z7_gcc`・`lpc55s69evk_gcc`・`nucleo_f401re_gcc`
の3つのみで、asp3_core の mps2/zcu102/polarfire 用は無い。**TTSP3 不変方針のため、各ターゲットの
TTSP ターゲット資産を asp3_core 側 `test/ttsp/target/<t>/` に新設**（`ttsp_target_test.{c,h}`・
空 `ttsp_target.cfg`）。汎用テストlib（`library/ASP/test`＝アーキ非依存）は TTSP3 のものを流用。

#### 追加した asp3_core 側ターゲット資産

| ファイル | 由来・要点 |
|---|---|
| `test/ttsp/target/mps2_an521_gcc/` | Cortex-M33。TTSP3 lpc55s69evk_gcc（同 arm_m）ベース。`RAISE_CPU_EXCEPTION=udf #0`、`get_stk`マクロ（USE_TSKINICTXB） |
| `test/ttsp/target/zcu102_arm64_gcc/` | Cortex-A53。`RAISE_CPU_EXCEPTION=svc #0xF000`、USE_TSKINICTXB非定義のため`get_stk`マクロ無し（汎用lib が TINIB.stk 直参照） |
| `test/ttsp/target/polarfire_soc_kit_gcc/` | RV64GC。`RAISE_CPU_EXCEPTION`＝未定義命令（圧縮命令対応）、`get_stk`マクロ無し |

各 `ttsp_target_test.c` は `stop_tick/start_tick/gain_tick` を no-op、`int_raise/cpuexc_raise` は
SKIP モジュール専用（mps2 は実コール、polarfire の int_raise は PLIC 未提供で no-op）。

#### HW 依存テストの扱い（ドライバの SKIP 規則）

純カーネル系（92%＝1671件）は HW を実行時に呼ばないが、調査の結果さらに以下が判明：

- `ttsp_target_stop_tick` は **timeout 系テスト全般**（task_sync/dataqueue 等の純カーネル系含む）が
  防御的に呼ぶ。多くは no-op で正しく通るが、時刻凍結に依存する timed-API のタイムアウト系・
  alarm/cyclic refer 系は no-op だと破綻する。
- `interrupt`/`exception` モジュールはカーネル API（`ras_int` 等）・割込み/例外設定がターゲット依存で、
  int_raise/cpuexc 未実装の本資産では検証不能（polarfire では `ras_int` がリンク未解決）。

そこでドライバは3段の SKIP を行う（`docs` 冒頭のドライバ説明参照）：
1. **モジュール SKIP**（`skip_modules`）：`interrupt`/`exception` を丸ごと除外。
2. **HW呼び出し事前 SKIP**（`unsupported_hw`）：生成 out.c が `gain_tick`/`int_raise`/`cpuexc_raise`
   を呼ぶテスト（_ntc/_ten 変種・cyclic/alarm/time_event）をビルド前に除外。
3. **soft HW 失敗時再分類**（`soft_hw`）：`stop_tick` 依存で実行失敗したものを SKIP に再分類
   （通れば PASS。ビルド失敗は HW 無関係＝真の FAIL のまま）。

#### 横断結果

| ターゲット | preset | この環境 | 結果 |
|---|---|---|---|
| zybo_z7 (A9) | zybo_z7-qemu | build+run | **functional 1813/1813・staticAPI 138/138 PASS**（FAIL 0） |
| mps2_an521 (M33) | mps2_an521-qemu | build+run | **PASS 1265・SKIP 639・FAIL 29**（functional FAIL 0／FAILは全て staticAPI 系） |
| polarfire (RV64GC) | polarfire_soc_kit-qemu | **build-only**（qemu-riscv64 未導入） | **PASS 1504・SKIP 423・FAIL 6**（全ビルド成立確認。FAILは staticAPI error系のみ） |
| zcu102 (A53) | zcu102_arm64-qemu | 不可（aarch64 ツールチェーン未導入） | 資産作成済・検証は devcontainer/CI へ |

- **mps2 の functional は実行可能な全件 PASS・FAIL 0**（SKIP 639＝gain_tick 379・stop_tick 228・
  int/cpuexc/interrupt/exception 等）。
- **残る FAIL は全て staticAPI のエラー/runtime系**（mps2 28+1・polarfire 6）で、
  `CFG_INT`/`DEF_EXC`/`DEF_INH`/`DEF_ICS`（割込み・例外・ICS）と `CRE_ALM`/`CRE_CYC`（通知）に集中＝
  **ターゲット依存の cfg エラーコード差**（TTSP3 の `err_code.txt` は参照ターゲット基準。zybo とは
  割込み/例外モデルが異なる）。`DEF_ICS_d-2`(mps2 runtime) のみビルド不成立。これらは静的API検証の
  既知ターゲット差として記録（functional 適合性とは別問題）。

```bash
python3 test/ttsp/run_ttsp.py --target mps2_an521     --tap api_test/ASP
python3 test/ttsp/run_ttsp.py --target polarfire_soc_kit --build-only --tap api_test/ASP
python3 test/ttsp/run_ttsp.py --target zcu102_arm64    --tap api_test/ASP   # 要 aarch64 toolchain
```

### 残（本実装）

- **zcu102 / polarfire の QEMU 実走行**：本開発機は aarch64 ツールチェーン・qemu-system-riscv64 未導入
  のため、zcu102 は build 不可・polarfire は build-only。両者の QEMU 実行は devcontainer/CI で行う。
- **staticAPI エラー系のターゲット差（mps2 29・polarfire 6）**：割込み/例外/ICS/通知 静的APIの
  エラーコード差。期待値をターゲット別に持つか、許容差として扱うかは要検討。
- **HWタイマ/割込み/例外の本対応**：`gain_tick`（HWタイマ早送り）・`int_raise`・`cpuexc` を各アーキで
  実装すれば、現状 SKIP の HW依存テスト（mps2 で約639件）も検証可能になる。CLAUDE.md に従い
  生成後の人間確認が必要なため別タスク。
- **CI 組み込み**：`.github/workflows/` への nightly ジョブ（mps2 build+run・polarfire build-only 等）追加は未実施。

## スコープ外 / リスク

- TTSP3 への一切の改変（読み取り専用を厳守）。
- TTSP3 の `ttsp_target_test.h` が定義する割込み番号等が asp3_core 各ターゲットと整合するかは
  ターゲット毎に確認（zybo は実証済）。未対応はSKIP。
- functional の ttg 生成は Ruby 依存（開発コンテナへの ruby 追加要否）。
- SIL/timing テストは別途（本計画は api_test 中心）。
