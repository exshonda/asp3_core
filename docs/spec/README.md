# TOPPERS/ASP3 仕様書（Markdown版）

> **Source Reference**: Converted from upstream `doc/*.txt` with asp3_core notes.  
> **Conversion Status**: In progress (手順2 from docs/dev/docs-markdown.md)

## 概要

本ディレクトリは，TOPPERS/ASP3カーネルの上流由来テキスト仕様書をMarkdown化したものです。
原本の内容に忠実に変換し，asp3_coreとの差異（TECS廃止、CMake化など）は **asp3_core注** ブロックで付記します。

## ファイル一覧と状態

> **状態凡例**：🟢 完了 / 🟡 進行中 / ⚪ 計画中

### user.txt（ユーザーズマニュアル）→ 3分割

| ファイル | 内容 | 対応元 | 状態 | 備考 |
|---------|------|--------|------|------|
| `01_overview.md` | TOPPERS/ASP3カーネル概要 | user.txt §1 | 🟢 | 1.1〜1.6: 位置付け・仕様・マイグレーション・既知問題など |
| `02_target_overview.md` | ターゲット依存部概要 | user.txt §2 | 🟢 | 2.1〜2.3: 簡易/個別パッケージなど |
| `03_quickstart.md` | クイックスタートガイド | user.txt §3 | 🟢 | 3.1〜3.5: 開発環境・TECSジェネレータ・ビルド・ライブラリ化など |
| `04_directory_structure.md` | ディレクトリ構成 | user.txt §4 | 🟢 | 4.1〜4.2: パッケージ・ファイル構成 |
| `05_configuration_script.md` | コンフィギュレーションスクリプト | user.txt §5 | 🟢 | configure.rbは削除済み（CMake対応表を注記） |
| `06_makefile.md` | Makefile修正方法 | user.txt §6 | 🟢 | **asp3_core注**: CMake対応表を注記（Makefile版は廃止） |
| `07_configurator.md` | コンフィギュレータ | user.txt §7 | 🟢 | cfg.py（Python版）とのCLI差異を注記 |
| `08_system_services.md` | システムサービス | user.txt §8 | 🟢 | 8.1〜8.5: ログ・シリアル・ログタスク・histogram・banner。非TECS版の組込み手順を注記 |
| `09_support_libraries.md` | サポートライブラリ | user.txt §9 | 🟢 | 9.1〜9.3: strerror・キュー操作・ログ出力。PRISTINE（そのまま有効） |
| `10_test_program.md` | テストプログラム | user.txt §10 | 🟢 | 10.1〜10.7: test_svc・機能/性能テスト・testexec。testexec.py（CMake版）の差異を注記 |
| `11_usage_notes.md` | 使用上の注意とヒント | user.txt §11 | 🟢 | 11.1〜11.9: 実行時間特性・ID管理・リネーム。slog方式への変更を注記 |
| `12_reference.md` | 参考情報 | user.txt §12 | 🟢 | 12.1〜12.7: 利用条件・サポート窓口。改変版の位置付けを注記 |
| `13_reference_detail.md` | リファレンス | user.txt §13 | 🟢 | 13.1〜13.3: サービスコール/静的API早見表・版歴。詳細は docs/api/ |

### 他の仕様文書

| ファイル | 内容 | 対応元 | 状態 |
|---------|------|--------|------|
| `configurator_spec.md` | cfg仕様詳細（kernel_cfg.c/h 生成仕様・エラー条件） | configurator.txt | 🟢 |
| `design_overview.md` | カーネル内部設計（設計方針・ディスパッチャ・CHECKマクロ等） | design.txt | 🟢 |
| `design_mutex.md` | ミューテックス設計（優先度上限・標準構成） | mutex_design.txt | 🟢 |
| `design_inherit.md` | 優先度継承設計（extension/inherit/・推移的継承） | inherit_design.txt | 🟢 |
| `asp3_spec_overview.md` | ASP3仕様概要 | asp_spec.txt | ⚪ |
| `extension_guide.md` | 拡張パッケージガイド | extension.txt | ⚪ |
| `non_tecs_services.md` | 非TECS版システムサービス | non_tecs.txt | ⚪ |
| `porting_guide_upstream.md` | 上流ポーティングガイド | porting.txt | ⚪ |
| `migration_guide.md` | マイグレーション | migration.txt | ⚪ |
| `version_history.md` | 変更履歴 | version.txt | ⚪ |

## 変換ルール

### 見出し構造
- 原本の章番号（１．２．など）を H1（`#`）として保持
- 節番号（1.1, 1.2など）を H2（`##`）として保持
- 細分項目を H3, H4（`###`, `####`）で表現

**例**：
```markdown
# １．TOPPERS/ASP3カーネルの概要

## 1.1 TOPPERS/ASP3カーネルの位置付け

### 背景
...
```

### Markdown化
- 固定幅文字列の表 → Markdown表（`|`区切り）
- 箇条書き → リスト（`-`, `*`）
- コード例 → ` ``` ` フェンス

### asp3_core注
asp3_coreでの差異を以下フォーマットで付記：

```markdown
> **asp3_core注**: CMake化により Makefile は廃止済み。
> 詳細は `docs/building.md` §2 を参照。
```

## 参照関係

- `docs/api/` — API仕様の詳細（95サービスコール + 16静的API）
- `docs/errors.md` — エラーコード一覧
- `docs/porting/` — asp3_core固有の移植ガイド（本ファイルの porting_guide_upstream.md とは別）
- `docs/building.md` — ビルド手順（CMake中心）
- `AGENTS.md` — 開発規約

## 検証（機械突合）

各章の変換後，`scripts/check_spec_conversion.py` で原本との突合を行う
（識別子・3桁以上の数値・節番号が変換後Markdownにすべて含まれるかをチェック）。

```bash
python3 scripts/check_spec_conversion.py doc/user.txt <開始行>:<終了行> docs/spec/<変換先>.md
# 例: python3 scripts/check_spec_conversion.py doc/user.txt 1063:1729 docs/spec/08_system_services.md
```

## 変換進捗

- **手順1（2026-06-07 完了）**: docs/api/ 完成 → 95サービスコール + 16静的API 100% 対応
- **手順2（進行中）**: docs/spec/ 変換
  - Phase 1（2026-06-07 完了）: 01_overview.md, 02_target_overview.md
  - Phase 2（2026-06-07 完了）: 03_quickstart.md ～ 08_system_services.md
  - Phase 3（2026-06-07 完了）: 09_support_libraries.md ～ 13_reference_detail.md
    — **user.txt 全13章の変換完了**（全章機械突合・911トークン欠落ゼロ）
  - configurator.txt（2026-06-07 完了）: configurator_spec.md（機械突合516トークン欠落ゼロ）
  - design系3本（2026-06-07 完了）: design_overview.md・design_mutex.md・design_inherit.md
    （機械突合 計420トークン欠落ゼロ）
  - 次: asp_spec/non_tecs/extension → porting.txt → migration
- **手順3（計画中）**: simtimer.txt 削除、参照張り替え

---

**最終更新**: 2026-06-07（手順2 Phase 3 完了＝user.txt 全章変換済み）
