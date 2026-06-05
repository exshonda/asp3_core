# cfgのPython化

## 項目

cfgのPython化（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

Rubyベースのコンフィギュレータ（cfg）をPythonで再実装する。
宣言的スペック（データ駆動）設計を推奨とし、AIがコード生成ロジックを追いやすい構造にする。
あわせてビルド時のRuby依存（cfg実行・.trbテンプレート評価）を除去する。

### 方針

cfgは2層に分けて扱う（`docs/asp3_derivative_plan.md` §8.1 CFG_SPEC_MAP節）：

- **層① 静的API定義（api-table）＝ `kernel/kernel_api.def`・`*_sym.def`**
  接頭辞DSL（`#`=ID, `.`=符号無し, `+`=符号付き, `&`=一般, `^`=ポインタ, `$`=文字列,
  後置 `*`=キー `?`=オプション `...`=リスト）のテキスト。**上流と同形式のまま維持**し、
  Python版エンジンがこれをそのまま読む（YAML等への二重定義は持たない）。
  上流の静的API追加はこのファイルのテキストマージで追従できる。
- **層② エンジン＝ `cfg.py` / `pass1.py` / `pass2.py`（Ruby→Python移植）**
  api-tableを解釈してコード生成する汎用エンジン。テキスト差分が効かない
  **DIVERGED** 部分であり、上流 cfg.rb 系の挙動変更時は CFG_SPEC_MAP の
  対応表（最終確認バージョン）で手動再反映する。
- **生成テンプレート（旧 `.trb`）**：パス2/3でRubyスクリプトとして評価される
  `target_kernel.trb`・`target_check.trb`（target）、`core_offset.trb` 等（arch）も
  Python化の対象（`target_kernel.py`・`target_check.py`・`core_offset.py`）。

### 現状

- `cfg/` は**上流Ruby版のまま**：`cfg.rb`（751行）・`pass1.rb`（986行）・
  `pass2.rb`（552行）・`GenFile.rb`（106行）・`SRecord.rb`（235行）、計約2,630行。
- ビルドはMakefileから `cfg.rb` を3パスで起動している（TECSレス化後のビルドで動作確認済み）：
  - パス1：`.cfg`＋api-table → `cfg1_out.c`（静的APIの抽出・パラメータ評価）
  - パス2：`cfg1_out` 実行結果＋ `.trb` → `kernel_cfg.c/h`・`offset.h` 等の生成
  - パス3：ロードモジュール（シンボル・ROMイメージ）＋ `target_check.trb` → 構成チェック
- 各 `target/<name>/` に `target_kernel.trb`・`target_check.trb`、各archに
  `core_offset.trb` 等の `.trb` テンプレートが存在する。
- `DIVERGENCE_MAP.md` には `cfg/cfg.py`・`pass1.py`・`pass2.py`（エンジン）と
  `*.py` 生成テンプレートの行が**計画として記載済み**（実体は未作成）。

### 参照実装

- [asp3_pico_sdk](https://github.com/exshonda/asp3_pico_sdk)（AGENTS.md §14）に
  Python版cfgの先行実装あり。RP2350移植時には採用を見送った経緯がある
  （`arch/arm_m_gcc/rp2350/PORTING.md` 参照）ため、流用範囲は実施プランで検討する。

### 関連記述

- `docs/asp3_derivative_plan.md` §3：「RubyベースのコンフィギュレータをPythonで再実装。宣言的スペック（データ駆動）設計推奨」
- `docs/asp3_derivative_plan.md` §8.1：CFG_SPEC_MAP節（層①／層②の分離とマージ運用）
- `AGENTS.md` §7：cfg（Pythonコンフィギュレータ）の構成と運用
- `DIVERGENCE_MAP.md`：cfgエンジン・生成テンプレートの乖離行（計画）

## 実施プラン

（着手時に記載）

## 実施結果

（完了時に記載）
