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
| CMake対応 | `cmake.md` | make版（`sample/Makefile`・`Makefile.target/core/chip`・`configure.py`）。フェーズ2として実施済み |
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
| ~~提3~~ | ~~`target/zybo_z7_gcc/xilinx_sdk/`~~ | **取り下げ**：`jtag.tcl` が実機実行で使用中のため残置 |
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

（2026-06-06 記載。フェーズ1・フェーズ2とも完了）

### 削除したファイル（計524ファイル変更・約120,000行削除）

| コミット | 由来項目 | 内容 |
|---|---|---|
| `7a94b2a` | TECSレス | `tecsgen/`・`tecs_kernel/`・`extension/non_tecs/`・TECS版syssvc・全TECSセル・tSample2系・`test/*.cdl`（315ファイル・94,349行） |
| `31acf36` | cfgのPython化 | `cfg/*.rb` 5本・`.trb` 全数（v6m系含む）（48ファイル・6,539行） |
| `e6d62d2` | .rbツールの.py化 | `configure.rb`・`testexec.rb`・`testcfg.rb`・`utils/*.rb`・`makerelease.py`（8ファイル・2,591行） |
| `1de1bb4` | 本項目（その他） | `target/{ct11mpcore,gr_peach,macos_xcode,simtimer_ct11mpcore}_gcc/`・`arch/arm_gcc/rza1/`・`arch/simtimer/`・MANIFEST/E_PACKAGE全数・`test/simt_*.c` 8本（149ファイル・16,494行） |

提案の採否：提1（makerelease）採用／提2（simt系整理）採用／
提3（xilinx_sdk）**取り下げ**（`jtag.tcl` が実機実行で使用中）

### 波及修正

- `test/testexec.py`：simt系（TARGET:2）エントリ削除・コメント整理
- `target/mps2_an521_gcc/`・`target/raspberrypi_pico2_gcc/` の `Makefile.target`：
  依存関係定義の `.trb` 参照を `.py` に修正（回帰確認で検出）
- `arch/arm_m_gcc/common/Makefile.core`：v6m分岐の未変換注記
- `target/stm32mp257f_dk_arm64_gcc/CLAUDE.md`：MANIFEST追記規約を削除
- `docs/building.md`・`configure.py`：Ruby版cfg切替の記述削除
- `DIVERGENCE_MAP.md`：**「削除済みファイル」節を追加**（上流マージで復活させないための台帳）

### Git情報

- ベースコミット：`d8ddab5`（削除リスト記載）
- 関連コミット範囲：`7a94b2a`〜`183a90c`（削除4・台帳1・回帰修正1）
- ファイルリスト再現コマンド例：`git diff --stat d8ddab5 183a90c`

### 検証結果（回帰確認）

| テスト | 実施 | 結果 |
|---|---|---|
| CMake posix／m33-qemu／zybo-qemu | ○ | クリーンビルド＋実行OK（バナー確認） |
| Makefile版 linux／mps2_an521 | ○ | linux実行・mps2 build＋check passed |
| testcfg.py（dummy_gcc） | ○ | cfg_all1生成物一致・pass2/pass1エラー一致（make行式差のみ） |
| testexec.py（QEMU mps2） | ○ | sem1／flg1 All check points passed |

### フェーズ2（make版ビルドの削除）

（2026-06-06 実施。前提条件の解消経緯は `cmake.md` 参照）

#### 削除したファイル（15ファイル）

- `configure.py`（Makefileビルドの入口）
- `sample/Makefile`（ビルドテンプレート）
- `kernel/Makefile.kernel` ※**kernel/領域の削除**（DIVERGENCE_MAP「削除済みファイル」節に記録）
- `arch/{arm64_gcc,arm_gcc,arm_m_gcc}/common/Makefile.core`（3本）
- `arch/{arm64_gcc/stm32mp2,arm_gcc/zynq7000,arm_m_gcc/rp2350}/Makefile.chip`（3本）
- `target/{dummy,linux,mps2_an521,raspberrypi_pico2,stm32mp257f_dk_arm64,zybo_z7}_gcc/Makefile.target`（6本）

残置（Makefileだが make版ビルドとは無関係）：

- `target/stm32mp257f_dk_arm64_gcc/minimal_boot/Makefile`（実機ブート資材）
- `target/zybo_z7_gcc/xilinx_sdk/`（`jtag.tcl` 等の実機JTAG資材）

#### 波及修正

- `docs/building.md`：CMake単一系統の手順に全面書き換え
- `AGENTS.md` §4 前提メモ：「2系統」→「CMakeのみ」
- `target/stm32mp257f_dk_arm64_gcc/CLAUDE.md`：クイック手順を CMake（swd-run 等）に更新
- `DIVERGENCE_MAP.md`：configure.rb／Makefile改変行・「Makefile系 CMakeへ置換」行を整理し、
  make版ファイル群を「削除済みファイル」節へ移動（kernel/Makefile.kernel 含む）
- `.gitignore`：`obj/` エントリ削除（Makefileビルド作業ディレクトリ廃止）
- `target/{mps2_an521,raspberrypi_pico2,stm32mp257f_dk_arm64}_gcc/target_user.md`：
  構築・実行・テスト手順を CMake に更新（あわせて TECS 前提の記述
  ＝tUsart セル・tecsgen 同梱等を非 TECS 構成の記述に修正）
- `docs/dev/cmake.md`：前提条件節をフェーズ2実施済みに更新

※テストランナ（testexec.py／testcfg.py）はフェーズ2前にCMakeベース化済みのため影響なし
