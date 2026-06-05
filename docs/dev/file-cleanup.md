# ファイルの削除

## 項目

ファイルの削除（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

完了した機能追加項目（TECSレス・cfgのPython化・.rbツールの.py化・CMake対応）で
不要になったファイルと、今後使用しないファイルを削除し、リポジトリをAIが追いやすい
最小構成にする。

### 削除リストの管理方法

開発項目に紐づく削除ファイルは**各項目の管理ファイル**に「削除対象ファイル」節として
記載し、本ファイルから参照する。項目に紐づかないものは本ファイルに記載する。

| 由来 | 削除リストの記載場所 | 概要 |
|---|---|---|
| TECSレス | `tecs-less.md` | `tecsgen/`・`tecs_kernel/`・TECS版syssvc・各TECSセル（`.cdl`等）・`extension/non_tecs/`（展開済み） |
| cfgのPython化 | `cfg-python.md` | Ruby版エンジン（`cfg/*.rb`）・生成テンプレート `.trb` 一式 |
| .rbツールの.py化 | `rb-tools-python.md` | `configure.rb`・`testexec.rb`・`testcfg.rb`・`utils/*.rb` |
| CMake対応 | `cmake.md` | make版（`sample/Makefile`・`Makefile.target/core/chip`・`configure.py`）。**条件付き＝フェーズ2**（pico2のCMake対応・testcfg/testexecのCMake化が前提） |
| その他 | 本ファイル（下記） | 上流配布用メタデータ・使用しないターゲット等 |

### その他の削除対象（本項目固有）

ユーザー指定：

- `MANIFEST`（ルートおよび全サブディレクトリ。上流パッケージ配布用メタデータ）
- `E_PACKAGE`（同上）
- `target/ct11mpcore_gcc/`
- `target/gr_peach_gcc/`
- `target/macos_xcode/`
- `target/simtimer_ct11mpcore_gcc/`
- `arch/arm_gcc/rza1/`
- `arch/simtimer/`

追加提案（承認済みのものを削除対象とする）：

| # | 候補 | 理由 |
|---|---|---|
| 提1 | `utils/makerelease.rb`・`utils/makerelease.py` | MANIFEST廃止により機能しなくなる（リリースはgit/CIで行う） |
| 提2 | `test/simt_*.c`（8本）＋ `test/testexec.py` の simt系・ct11/gr_peach系エントリ | `arch/simtimer/` 削除によりタイマドライバシミュレータ系テストが実行不能になるため一体で整理 |
| 提3 | `target/zybo_z7_gcc/xilinx_sdk/` | Xilinx SDK（Vivado）プロジェクト資材。本リポジトリの開発フロー（QEMU/SWD）では未使用 |
| 提4 | `sample/Makefile.bak`等のバックアップ・`*.bak` 類（あれば） | 生成残骸 |
| 対象外 | `doc/`（上流テキスト仕様書） | 「ドキュメントMarkdown化」項目でMarkdown版に置換後に削除判断（今回は残置） |
| 対象外 | `extension/`（non_tecs以外：messagebuf/ovrhdr/subprio等） | 拡張パッケージ導入時に使用する可能性があるため残置（non_tecsのみ削除） |
| 対象外 | `target/dummy_gcc/` | cfgテスト（testcfg.py）が使用するため残置 |

### 削除に伴う波及修正

- `testexec.py`：TEST_SPEC から simt系（systim*/drift*/ovrhdr3/simtimer1）・
  arm_cpuexc1等のct11/gr_peach前提エントリの整理（※arm系テストはzyboで使う場合は残す）
- `testcfg.py`：dummy_gcc使用のため影響なし
- 規約修正：「新規ファイルはMANIFESTに追記」（`target/stm32mp257f_dk_arm64_gcc/CLAUDE.md`）の削除
- `UPSTREAM_PRISTINE.txt`・`DIVERGENCE_MAP.md`：削除済みファイルの扱いを記録
  （上流マージ時に削除ファイルが再流入しないよう「削除済み」リストを台帳に追加）
- `docs/building.md`・`AGENTS.md`：ct11/gr_peach等の記述削除

## 実施プラン

1. **削除リストの確定**（本ファイル＋各管理ファイルへの記載）← 今回
2. **フェーズ1の削除実施**（ユーザー承認後）
   - TECS関連（tecs-less.md のリスト）
   - Ruby版cfg＋`.trb`（cfg-python.md のリスト）
   - Ruby版ツール（rb-tools-python.md のリスト）
   - その他（本ファイルのリスト＋承認された提案）
   - 波及修正（testexec.py整理・規約修正・台帳更新）
3. **回帰確認**
   - CMake：posix／m33-qemu／zybo-qemu のビルド＋実行（QEMU）
   - Makefile版：linux／mps2 のビルド＋実行（フェーズ2まで併存のため）
   - testcfg.py（dummy）・testexec.py（mps2で機能テスト数本）
4. **フェーズ2（別途承認）**：make版ビルドファイル＋configure.py の削除
   （前提条件は cmake.md 参照）
5. **記録**：削除済みリストの台帳化（DIVERGENCE_MAP／UPSTREAM_PRISTINE）、
   実施結果記載、README状態更新、コミット・プッシュ

## 実施結果

（完了時に記載）
