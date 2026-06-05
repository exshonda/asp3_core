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

（完了時に記載）
