# .rbツールの.py化

## 項目

.rbツールの.py化（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

cfg以外に残るRubyツールをPythonに書き換え、ビルド・テスト・開発フローからRuby依存を除去する。
cfgのPython化（`cfg-python.md`）の後続作業。

### 対象の棚卸し

| ファイル | 行数 | 役割 | ビルド/テストフローでの位置 |
|---|---|---|---|
| `configure.rb` | 375 | Makefile生成（ビルドの入口） | **通常ビルドで必須**（cfgのPython化後、ビルドフローに残る唯一のRuby） |
| `test/testexec.rb` | 492 | テストランナ（テストのbuild/exec/clean一括実行） | テスト実行で使用（実機テストでも使用実績あり） |
| `test_cfg/testcfg.rb` | 424 | cfg単体テスト（生成物を期待値と比較） | cfg検証用。**Python版cfgの回帰テストに転用価値が高い** |
| `utils/genrename.rb` | 187 | `*_rename.h`／`*_unrename.h` の生成 | 新ターゲット移植時に使用 |
| `utils/applyrename.rb` | 122 | リネームの適用 | 同上 |
| `utils/gentest.rb` | 545 | テストシナリオ→テストプログラム（C）生成 | テスト追加時のみ（生成済みテストはコミット済み） |
| `utils/makerelease.rb` | 218 | リリースアーカイブ作成 | リリース時のみ |

### 対象外

- `cfg/*.rb`：cfgのPython化で対応済み（Ruby版は残置、削除は「ファイルの削除」項目）
- `tecsgen/`（Ruby 72ファイル）：TECS本体。TECSレス化済みであり「ファイルの削除」項目で削除予定のため移植しない

### 方針

- 既存Rubyツールと同一のCLI（オプション・引数・出力）を維持する（Makefile等からの呼び出し互換）。
- cfgのPython化と同様、Ruby版は残置し（比較・フォールバック用）、削除は「ファイルの削除」項目で行う。
- 変換規範は cfg のPython版（命名・構成）に合わせる。

## 実施プラン

優先度順に変換し、各ステップで動作検証する。

1. **configure.py**（最優先）
   - `configure.rb` と同一CLI（`-T`/`-A`/`-a`/`-S`/`-g` 等＋`変数=値`引数）のPython版を作成。
     asp3_core変更点（非TECSデフォルト・Python版cfgデフォルト）も同一に実装。
   - 検証：linux／mps2_an521／zybo_z7 で `configure.py` 生成のMakefileがRuby版生成と
     **一致**することを確認（diff）。linux実行＋QEMU動作確認。
2. **testcfg.py**
   - cfg単体テストランナをPython化し、**Python版cfgに対して** `test_cfg/` のテストを実行。
   - 検証：テストスイートのpass（期待値との一致）。
3. **testexec.py**
   - テストランナをPython化（build/exec/clean、対象リスト処理）。
   - 検証：QEMU（mps2_an521）で機能テストの一部（例：test_sem等の数本）をbuild→exec。
4. **utils（genrename.py／applyrename.py）**
   - 検証：既存ターゲットの `*_rename.def` から生成し、コミット済み `*_rename.h` と一致することを確認。
5. **utils（gentest.py／makerelease.py）**（優先度低）
   - 検証：gentest は既存テストシナリオから生成し、コミット済みテストプログラムと比較。
     makerelease はアーカイブ生成・展開の動作確認。
6. **記録**
   - `DIVERGENCE_MAP.md`（新規.py追加・上流.rb変更時の再反映ルール）、本ファイルの実施結果、
     `docs/dev/README.md` 状態更新。コミット・プッシュ。

## 実施結果

（2026-06-05 記載・完了）

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
| `configure.rb`・`configure.py`・`sample/Makefile`・各`Makefile.target`/`Makefile.chip` | `OMIT_DEFAULT_SYSSVC`（非TECS時のsyssvcオブジェクト自動付与の抑止）を追加。素のカーネルビルド（cfgテスト等）を可能にする |
| `cfg/cfg.py` | スクリプト実行時の`__main__`/`cfg`モジュール二重化で`error_flag`が伝播せず、エラー時も終了コード0になるバグを修正 |
| `cfg/pass2.py` | 重複エラーメッセージの引用符をRuby版と同形式（`` `X' ``）に修正 |
| `kernel/interrupt.py` | 無効な`intno`のCRE_ISRでKeyError crashするバグを修正 |
| `target/stm32mp257f_dk_arm64_gcc/Makefile.target` | 古い「TECS構成のみサポート」コメントを削除 |

### 追加したファイル

| ファイル | 対応するRuby版 | 検証結果 |
|---|---|---|
| `configure.py` | `configure.rb`（375行） | 全8ターゲット＋オプション網羅ケースで生成Makefileがバイト一致。linuxビルド・実行確認 |
| `test_cfg/testcfg.py` | `test_cfg/testcfg.rb`（424行） | dummy_gccで全9テストが期待値一致（makeのバージョン表記差のみ）。pass3チェック56エラー含む |
| `test/testexec.py` | `test/testexec.rb`（492行） | QEMU mps2-an521で機能テスト6本（sem1/sem2/flg1/dtq1/task1/hrt1）build→exec全pass。非TECS用に`tecsgen.cfg`スタブ自動生成を追加 |
| `utils/genrename.py` | `utils/genrename.rb`（187行） | 既存14個の`*_rename.def`から再生成した`*_rename.h`/`*_unrename.h`がコミット済みと一致（CRLF既存ファイルは改行差のみ） |
| `utils/applyrename.py` | `utils/applyrename.rb`（122行） | kernel/の3ソースへの適用結果がRuby版と完全一致 |
| `utils/gentest.py` | `utils/gentest.rb`（545行） | **全46テストソース**でRuby版と出力完全一致（HOOK %d書式・Ruby版typoの出力互換含む） |
| `utils/makerelease.py` | `utils/makerelease.rb`（218行） | zyboターゲットMANIFESTでtar.gz/zip生成・アーカイブ内容がRuby版と一致 |

### 削除したファイル

なし（Ruby版は全て残置。削除は別項目「ファイルの削除」で実施。`tecsgen/` は対象外＝同項目で削除予定）

### Git情報

- ベースコミット：`507c007`（rb-tools計画記載）
- 関連コミット範囲：`9ccd72e`（configure.py）〜本コミット（testcfg+cfg修正 `e460cb4`／testexec `dcfdb93`／rename系 `d2c1b95`／gentest+makerelease `4227cd5`）
- ファイルリスト再現コマンド例：`git diff --stat 507c007 HEAD`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| configure.py（Makefile一致） | ○ | 8ターゲット＋オプションケースでバイト一致 |
| testcfg.py（cfgテストスイート） | ○ | 9/9 期待値一致（make表記差のみ）。**Python版cfgの回帰テストとして機能** |
| testexec.py（QEMU mps2-an521） | ○ | 機能テスト6本 All check points passed |
| genrename/applyrename/gentest/makerelease | ○ | Ruby版出力と一致（上記表参照） |

### DIVERGENCE_MAP との関連

- `.py`ツール群の行を追加（上流`.rb`変更時は対応`.py`へ手動再反映）
- `configure.rb`行は既存（OMIT_DEFAULT_SYSSVC追加も同行でカバー）

### 残課題・備考

- ビルド・テストフローはPythonのみで完結（Ruby不要）になった。Ruby版・`tecsgen/`の削除は「ファイルの削除」項目で実施
- testcfg/testexecの実行にはTARGET_OPTIONSの作成が必要（testcfg.py冒頭コメント参照）

## 削除対象ファイル（「ファイルの削除」項目で実施）

Python化の完了に伴い不要となるRuby版：

- `configure.rb`
- `test/testexec.rb`
- `test_cfg/testcfg.rb`
- `utils/genrename.rb`・`utils/applyrename.rb`・`utils/gentest.rb`
- `utils/makerelease.rb`（MANIFEST廃止（file-cleanup.md）に伴い `makerelease.py` も削除）
