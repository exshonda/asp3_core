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

### 残（本実装）

ドライバ `run_ttsp.py`・静的API系の自動判定・ttg連携（functional 1813件）・ターゲット横断・CI。

## スコープ外 / リスク

- TTSP3 への一切の改変（読み取り専用を厳守）。
- TTSP3 の `ttsp_target_test.h` が定義する割込み番号等が asp3_core 各ターゲットと整合するかは
  ターゲット毎に確認（zybo は実証済）。未対応はSKIP。
- functional の ttg 生成は Ruby 依存（開発コンテナへの ruby 追加要否）。
- SIL/timing テストは別途（本計画は api_test 中心）。
