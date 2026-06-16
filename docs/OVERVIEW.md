# OVERVIEW.md — TOPPERS/ASP3 Core 構想まとめ

> **TOPPERS/ASP3 Core**（リポジトリ：`asp3_core`）の全体像。  
> 詳細は各ドキュメント（`AGENTS.md` / `docs/dev/README.md` 等）を参照。

---

## 1. ASP3からの変更点

### カーネル基盤（既存3リポジトリで実現済み → asp3_core に統合）
- TECSレス版システムサービス（上流 `extension/non_tecs/syssvc` 由来。ローカル発明ではなく上流拡張を利用）
- コンフィギュレータ cfg のPython化（Ruby→Python移植）
  - エンジン（`cfg.py`/`pass1.py`/`pass2.py`）＝汎用。静的API定義は `kernel_api.def`（api-table）から読む
  - 生成テンプレートも `.trb`→`.py`（`core_offset.py`/`target_kernel.py`/`target_check.py`）
- CMake対応（**上流ASP3はMakefileベース**=`configure.rb`／`Makefile.target`。CMakeは `asp3_pico_sdk` 由来。`libasp3.a` を生成、cfg 3パスをCMakeネイティブ配線、4層の変数積み上げ方式。ターゲット選択は `CMakePresets.json`。非Pico上流ターゲットはMakefile→CMake移植が必要）
- ARMv8-M（Cortex-M33／RP2350）ベアメタルポート
- ARMv8-A（Cortex-A35／STM32MP257F-DK）ベアメタルポート
- OS awareness（GDB）

### 新規に追加・統合するもの
- 各社SDK協調（Pico SDK／Renesas FSP／STM32 HAL）を1つの共通カーネル基盤に統合
  （既存3リポジトリは submodule として本体を参照する形に再編）
- 対象ターゲット追加：RISC-V Hazard3（RP2350）、ホスト(Linux)シミュレータ（上流 `extension/non_tecs/target/linux_gcc`／mac は `macos_xcode`）、QEMU（mps2-an505）
- 構造化ログ（`T=<tick>,EV=<event>` 形式）

### 維持する設計制約
- `kernel/` コアは PRISTINE（直接編集禁止）
- 静的割付けのみ・動的生成（malloc）禁止（ISO 26262／IEC 61508 整合）
- ソース識別子プレフィックスは `asp3` のまま

---

## 2. AI向けの記述の追加

### エントリ・規約ファイル
- `AGENTS.md`：全AIツール共通の正本（single source of truth）
- `CLAUDE.md` / `.clinerules` / `.cursorrules`：各ツールから AGENTS.md への薄いポインタ

### 手動マージ支援（上流追従の台帳）
- `DIVERGENCE_MAP.md`：上流からの乖離点を記録（cfgエンジン／定義／生成テンプレートを区別）
- `UPSTREAM_VERSION` / `UPSTREAM_PRISTINE.txt`：マージ基準点の管理
- CFG_SPEC_MAP（計画書内）：cfgエンジン（DIVERGED）と定義（テキストマージ可）の追跡

### ドキュメント（RAG・AI参照向け）
- `docs/api/`：サービスコール＋静的API（ASP3に絞り込み＋派生差分。静的APIの構造正本は `kernel_api.def`）
- `docs/porting/`：`PORTING_GUIDE.md`（Step式）、`IMPL_INDEX.md`（参照すべき既存ターゲット索引）、`target_spec.yaml.template`
- `docs/errors.md`：`E_*` エラーコード辞書

### 自己検証ループ（AIが正否を機械確認できる仕組み）
- 構造化ログ＋`scripts/parse_slog.py`・`check_events.py`（期待イベント列との照合）
- `test/porting/`：TAPフレームワーク（POSIX/QEMUでハードなし検証、`tap_done()`→終了コード）
- `compile_commands.json` ＋ `.clangd`（クロスコンパイルのコード知能）

### CI・品質ゲート（生成コードの自動検証）
- ビルド/テストマトリクス：POSIX・QEMU(mps2-an505)・各実機ターゲットを自動ビルドし、TAPテストを実行
- **静的解析のCI統合**：`clang-tidy` / `cppcheck`（MISRA-C:2012サブセット相当）。生成コードがこれを通過することを必須とする
- 上記「自己検証ループ」をCIが自動実行し、回帰を検知（AI生成コードの品質ゲート）

### Claude Code Skills（別リポジトリで配布）
- build / flash / debug / cfg生成 の各skill
- `merge-review`（上流diff＋`DIVERGENCE_MAP` → 影響箇所抽出）、`cfg-diff`（上流cfg変更＋CFG_SPEC_MAP → Python実装の更新要否）
- asp3_core本体とは別リポジトリで管理し、本体から参照（ESLA文脈での配布も想定）

---

## 3. その他

### 名称（決定）
- ブランド名 **TOPPERS/ASP3 Core**、リポジトリ **`asp3_core`**、識別子 `asp3`
- 「Core」＝各SDK協調版が乗る共通基盤の意。検討経緯：ASP3-Plus は Ubiquitous AI「TOPPERS-Pro/ASP3」と意味衝突で不採用
- **TOPPERS名称規則 第4条**：「TOPPERS/」を冠する正式名は運営委員会の事前承認＋公式成果物と非誤認が条件。
  承認前は `asp3_core` ＋「based on TOPPERS/ASP3」で進行可

### 開発・運用
- 開発環境を **Docker で提供**：toolchain（arm-none-eabi-gcc / riscv-gcc / a35-gcc）・QEMU・静的解析ツールを同梱し、ローカルとCIで同一環境を再現
- 上流追従は手動マージ
- Git：`upstream`（マージ基準）＋ `main` の2ブランチ最小構成、必要時のみフィーチャーブランチ
- 人間/AI分担：人間がPhase 0（upstream確立＋最初の1ターゲットを緑＋ハード/メモリ/割込みの正しさ）、
  AIが横展開（他ターゲット・docs・テスト・CI）
- フォルダ構成は上流ASP3をミラー（git diff容易）。区分：PRISTINE／EXTENDED／DIVERGED／NEW

### ライセンス・適合領域
- TOPPERSライセンス（改変版である旨を明示）
- 安全関連：ISO 26262／IEC 61508／ISO/PAS 8800／MISRA-C／AUTOSAR を意識
