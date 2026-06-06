# ドキュメントMarkdown化

## 項目

ドキュメントMarkdown化（AGENTS.md §1 機能追加計画、優先度：中）

## 内容

上流由来のテキスト仕様書（`doc/*.txt`）とAPIリファレンスをMarkdown化し、
**RAG（検索・引用）とAIエージェントの参照に適した形**に整える。

### 意義（何が嬉しいか）

1. **エージェントの一次資料が引用可能になる**：仕様判断の根拠
   （タスク状態遷移・エラー条件・cfg仕様）は現状 `doc/*.txt`（固定幅・
   全角整形・3,000行級の単一ファイル）にあり、検索性・引用粒度が悪い。
   見出し構造化＋1トピック1ファイル化で「该当節だけ読む」が可能になる。
2. **既存資産の完成**：`docs/api/`（30本・高品質なMarkdown）と
   `docs/errors.md` は整備済み。**残り約50本のサービスコール**を埋めれば
   APIリファレンスが完結する（AGENTS §12「詳細は docs/api/」の実体化）。
3. **廃止機能の記述ノイズ除去**：TECS前提・simtimer等、asp3_coreで
   廃止済みの記述を注記・除去した「本リポジトリの実態に合う版」になる。

### 現状（2026-06-06 調査）

**docs/（Markdown・整備済み）**：`docs/api/` 30本（act_tsk等・表形式で統一）・
`docs/errors.md`・building/porting/dev 各文書

**doc/（上流テキスト・計約17,000行）**：

| ファイル | 行数 | 内容 | 方針案 |
|---|---|---|---|
| `user.txt` | 3,017 | ユーザーズマニュアル（カーネル仕様の中核） | **変換**（章単位に分割） |
| `porting.txt` | 4,761 | 上流の移植ガイド | **変換**（docs/porting/ と相互参照。PORTING_GUIDEはasp3_core固有手順・こちらは依存部仕様の詳細として共存） |
| `configurator.txt` | 1,802 | cfg仕様 | **変換**（cfg-python の CFG_SPEC_MAP と接続） |
| `design.txt`・`mutex_design.txt`・`inherit_design.txt` | 2,821 | 内部設計メモ | **変換**（カーネル内部理解＝デバッグ時のRAG価値大） |
| `asp_spec.txt` | 692 | 仕様概要 | **変換** |
| `extension.txt` | 910 | 拡張パッケージ説明 | **変換**（残置中のextension/に対応する節のみ） |
| `non_tecs.txt` | 623 | 非TECS版説明 | **変換＋注記**（asp3_coreでは既定構成） |
| `migration.txt` | 560 | 旧版からの移行 | 変換（低優先） |
| `version.txt` | 1,557 | 変更履歴 | **残置**（履歴はテキストのまま・マージ追従対象） |
| `simtimer.txt` | 293 | simtimer説明 | **削除**（機能自体を削除済み・file-cleanup追補） |

### 方針

- **変換先は `docs/spec/`**（新設）。`doc/` の原本は**変換検証完了まで残置**し、
  完了後に削除（file-cleanup の方針どおり。DIVERGENCE_MAP「削除済み」節へ記録．
  上流マージ時は上流 `doc/*.txt` のdiffを `docs/spec/` へ手動反映＝cfg.rb→cfg.py
  と同じ運用）
- **RAG対応の変換規則**：
  - 1章（または1トピック）＝1ファイル・先頭にH1＋原本の章番号を保持
    （上流diffとの照合用トレーサビリティ）
  - 固定幅の表・箇条書きをMarkdownの表・リストへ。コードは ``` フェンス
  - **内容は原文に忠実**（意訳しない）。asp3_coreでの差異
    （TECS廃止・CMake化等）は原文を消さず **「asp3_core注」ブロック**で付記
- **APIリファレンスは既存30本のフォーマットを踏襲**して残り（約50本）を追加
  （種別・C言語API・パラメータ・エラーコード・機能・関連の構成）

## 実施プラン

1. **docs/api/ の完成**（独立して価値があるため先行）
   - 全サービスコールの棚卸し（`kernel/kernel_api.def`＋user.txtの関数一覧から
     リスト化）→ 未作成分を既存フォーマットで作成
   - 出典は user.txt／統合仕様書の記述に忠実。各ファイルに対応する
     カーネルソース（`kernel/task.c` 等）への参照行を付ける
   - `docs/api/README.md` の索引を全数に更新
2. **docs/spec/ への変換（優先順）**：user.txt → configurator.txt →
   design系3本 → asp_spec/non_tecs/extension → porting.txt → migration
   - 各ファイル変換後に**機械チェック**：原本との対応確認（章立ての網羅・
     数値/識別子の一致をスクリプトで突合．`scripts/` に検証スクリプトを追加）
3. **simtimer.txt の削除**（file-cleanup 追補・DIVERGENCE_MAP記録）
4. **参照の張り替え**：AGENTS §13・README・docs内の `doc/*.txt` 参照を
   `docs/spec/` へ。`doc/` 原本の削除はユーザー確認のうえ実施
5. **記録**：README索引・本ファイル実施結果・DIVERGENCE_MAP

### スコープ外

- TOPPERS第3世代カーネル統合仕様書（外部docx・500頁級）の全文変換
  — APIリファレンス＋user.txt変換で実用上の参照は賄える。必要節が
  出てきたら個別に追加
- 英訳・対訳化
- RAGインデックス自体の構築（チャンク分割・埋め込み）— 別ツール側の責務

### リスク・確認事項

- **変換の忠実性**：仕様文書のため意訳・脱落が致命的。機械突合
  （識別子・数値・章網羅）＋スポット目視を変換単位ごとに行う
- **上流マージ負担**：doc/原本削除後は上流diffの手動反映が発生
  （version.txtを残すのはこのため＝リリース毎に必ず更新されるファイル）
- 規模：変換対象 約15,000行＝複数セッションに分割
  （1セッション＝1〜3ファイル目安。索引の状態欄で進捗管理）

## 実施結果

**全手順完了（2026-06-07）**。総括：

- **手順1**：APIリファレンス完成 — サービスコール95本＋静的API16本＝**111本＋README**
  （正本 `kernel.h`/`kernel_api.def` と1対1・機械突合済み）
- **手順2**：仕様文書のMarkdown化完了 — 変換対象10本（約15,000行）→ `docs/spec/`
  **24ファイル＋README**。全変換を `scripts/check_spec_conversion.py` で機械突合し
  **欠落ゼロ**（user.txt=911・configurator=516・design系=420・asp_spec等=619・
  porting=982・migration=158 トークン）
- **手順3**：`doc/simtimer.txt` 削除（機能自体削除済みのため変換せず・file-cleanup追補）
- **手順4**：参照の張り替え（docs/spec内の旧参照・`docs/api/get_tim.md`・
  stm32 `target_user.md` の計6箇所→`docs/spec/`へ）と **`doc/*.txt` 原本11本の削除**
  （ユーザー承認済み）。**`doc/version.txt` のみ残置**（マージ追従の起点）。
  原本は `upstream` ブランチ・削除前コミットから取得可能（README検証節に再実行手順を記載）
- **手順5**：記録 — DIVERGENCE_MAP「削除済み」節・file-cleanup.md・README索引を最終化

**上流マージ時の運用**（以後恒久）：上流 `doc/*.txt` のdiffは `docs/spec/` の対応
ファイルへ手動反映する（cfg.rb→cfg.pyと同じ運用。対応表は `docs/spec/README.md`）。

（以下は手順ごとの中間記録＝経緯）

### 手順1：docs/api/ の完成（2026-06-07 完了）

**棚卸し**：`include/kernel.h` の extern 宣言＝サービスコール95本、
`kernel/kernel_api.def`＝静的API 16本が正本。完成形は **111本＋README** で、
全ファイルが正本と1対1対応（`comm` で機械突合・差分ゼロを確認）。

**作成**：未整備の82本（サービスコール80本＋静的API `CRE_ISR`・`DEF_ICS`）を
既存フォーマットで新規作成。エラーコード・種別（〔T〕/〔I〕/〔TI〕）はカーネル
ソースの `CHECK_*` マクロ・実装分岐から導出（推測記述なし）。

**削除**（asp3_coreに存在しないAPIの誤doc・ユーザー確認済み）：
- `docs/api/ATT_ISR.md` — ASP3では `CRE_ISR`（ID付きオブジェクト生成）に置換済み
- `docs/api/DEF_SVC.md` — 拡張サービスコールはHRP3の機能（`cal_svc` も不存在）

**既存docの修正**（検証パスで検出した仕様誤り）：
- `get_tim.md` — 単位ms→**μs**（ASP3はSYSTIM/RELTIM/TMOすべてμs。
  `doc/asp_spec.txt` (4-7)）・種別〔TI〕→**〔T〕**（`CHECK_TSKCTX_UNL`）・
  ティック更新→ティックレスの記述修正・不存在の `get_utm` 言及除去
- ブロッキング系19本 — `E_RASTER`（待ち中の `ras_ter`）の漏れを追加、
  `E_RLWAI` の原因から `ter_tsk` 併記を除去（強制終了であり復帰しない）
- `AGENTS.md` §12・`DEF_INH.md`・`CFG_INT.md` — `ATT_ISR`/`DEF_SVC` 参照を
  `CRE_ISR` へ張り替え
- `README.md` — 索引を全111本のリンクに更新。不存在の `get_utm`・`get_did` を
  削除し `get_tid`/`get_lod`/`get_nth`・ini_*系・CPU例外管理（`xsns_dpn`）等を追加

**検証**：生成84本全数を3並列の独立検証（プロトタイプ＝kernel.h／種別＝CHECK_*
マクロ／エラーコード網羅性／時間単位／不存在機能への言及）で突合し、検出された
E_RASTER漏れ6本を修正。README索引⇔ファイルのリンク切れゼロを機械確認。

**Git情報**：ベース `e12e68d`。ファイルリスト再現：
`git diff --stat HEAD -- docs/api/ AGENTS.md`（コミット後は該当コミット参照）

### 手順2：docs/spec/ への変換（2026-06-07 Phase1 完了）

**Phase1 — user.txt 前半2章 完成**

**作成ファイル**：
- `docs/spec/README.md` — 全体索引・変換ルール・参照関係（全仕様文書の計画テーブル付き）
- `docs/spec/01_overview.md` — user.txt §1（1.1〜1.6：ASP3カーネル概要）
- `docs/spec/02_target_overview.md` — user.txt §2（2.1〜2.3：ターゲット依存部）

**変換規則の実装**：
- H1（`#`）に原本章番号（１．２．）を保持（トレーサビリティ）
- H2（`##`）に節番号（1.1, 1.2等）
- Markdown表・リスト・コードフェンスへ変換
- asp3_core注を各所に付記（TECS廃止・CMake化・対応ターゲット・参照リンク更新等）

**asp3_core注の内容**：
- 01_overview.md: TECS廃止、asp3_core目的・対応ターゲット説明、docs/OVERVIEW.md参照
- 02_target_overview.md: 対応ターゲット実績、Gitリポジトリ構成、移植ガイド参照

**検証**：原本との対応確認（章立て網羅 ✅、本文忠実 ✅、参照更新 ✅）

**Phase2 — user.txt §3〜§8 完成（2026-06-07）**

**作成ファイル**：
- `docs/spec/03_quickstart.md` — §3（3.1〜3.5：開発環境・TECSジェネレータ・構築手順・ライブラリ化・分離ビルド）
- `docs/spec/04_directory_structure.md` — §4（4.1〜4.2：配布パッケージ構成・全ファイルリスト）
- `docs/spec/05_configuration_script.md` — §5（configure.rbオプション24個・パラメータ3個・処理内容）
- `docs/spec/06_makefile.md` — §6（6.1 変数定義A〜I・6.2 コンパイルオプション）
- `docs/spec/07_configurator.md` — §7（cfgオプション12個・タイマドライバ組込み）
- `docs/spec/08_system_services.md` — §8（8.1〜8.5：syslog・serial・logtask・histogram・banner。
  サービスコール仕様・結合関係図・TECSセル記述を完全保持）
- `scripts/check_spec_conversion.py` — **機械突合スクリプト**（新規。原本の識別子・
  数値・節番号が変換後Markdownに全数含まれるかを照合）

**asp3_core注の方針**（Phase2で確立）：
- 廃止された仕組み（configure.rb・Makefile・TECSセル記述）の章は，冒頭に
  「本章の位置付け」注を置き，**上流→asp3_coreの対応表**を付す（原文は照合用に全文保持）
- 組込み手順（8.1.8等）は非TECS版の実手順（`INCLUDE("syssvc/*.cfg")`）を付記
  （syssvc/の実ファイル・cfg.py --helpの実出力で確認済み・推測記述なし）

**検証**：`check_spec_conversion.py` で01〜08の全8章を機械突合 —
識別子464個・数値4個・節番号42個（計510トークン）**欠落ゼロ**。

**Phase3 — user.txt §9〜§13 完成（2026-06-07）＝ user.txt 全13章の変換完了**

**作成ファイル**：
- `docs/spec/09_support_libraries.md` — §9（9.1〜9.3：strerror・キュー操作6関数・ログ出力3関数。PRISTINE＝そのまま有効）
- `docs/spec/10_test_program.md` — §10（10.1〜10.7：test_svcチェック関数7個・機能テスト35本・
  simtテスト7本・perfテスト6本・cfgテスト8本・testexec仕様）
- `docs/spec/11_usage_notes.md` — §11（11.1〜11.9：実行時間特性5項目・assert・syslog設定・
  ID管理・リネーム・トレースログ・初期化フック・rodata・カーネルログ）
- `docs/spec/12_reference.md` — §12（12.1〜12.7：利用条件・サポート・バグ報告・ML）
- `docs/spec/13_reference_detail.md` — §13（13.1 サービスコール早見表・13.2 静的API16本・
  13.3 版歴14件。詳細はdocs/api/へ誘導）

**Phase3の主なasp3_core注**（実地確認に基づく）：
- 10.7: testexec.py（CMake版）の実仕様 — TARGET_OPTIONS=CMake引数1行・TARGET_RUN省略時は
  ./aspホスト実行・TEST_SPEC辞書・test_svc.c自動付与（testexec.py冒頭コメントで確認）
- 10.4: simtimer機能削除済み＝simtテストは実行不可（target/・arch/にsimtimer該当なし・
  testexec.pyに対応エントリなしを確認。test/simt_*.cfgはデータのみ残置）
- 11.5: applyrename.py/genrename.py（.py化済み・utils/で確認）
- 11.6: トレースログはTECSセル方式→slog方式（ASP3_ENABLE_TRACE・--slog・AGENTS §8）に変更
- 12: 改変版の位置付け・バグ報告窓口の使い分け（GitHub Issues vs TOPPERS ML）
- 13: docs/api/（111本）を正本として誘導

**検証**：機械突合（個別5章＋**全文一括** 行157-3017 vs 全13ファイル）—
識別子823個・数値7個・節番号81個（計911トークン）**欠落ゼロ**。
検証パスで検出した1件（11.5の `_kernel_` がMarkdownエスケープで不一致）を
コードスパン表記に修正済み。

**configurator.txt 変換（2026-06-07 完了）**

- `docs/spec/configurator_spec.md` — configurator.txt 全文（1,802行）。
  生成仕様（kernel_cfg.h §3・kernel_cfg.c §4）・静的API16本・通知ハンドラ生成（4.6）・
  エラー条件のNGKI/ASPS要件番号を全数保持。
- asp3_core注：**Python版cfg（cfg.py）が実装すべき生成仕様の正本**としての位置付けを冒頭に明記
  （生成スクリプト対応：パス2=kernel.py・パス3=kernel_check.py・CFG_SPEC_MAP参照）
- 検証：機械突合（行56-1803）— 識別子456個・節番号60個（計516トークン）**欠落ゼロ**

**design系3本 変換（2026-06-07 完了）**

- `docs/spec/design_overview.md` — design.txt 全文（1,126行）。実装設計方針（ASPR0001〜0011）・
  システム状態の実装（kerflg/enadsp/dspflg）・スケジューリング（p_runtsk/p_schedtsk）・
  ディスパッチャ構造（dispatch/ret_int/exit_and_dispatch等）・アイドル処理の4課題・
  xsns_dpn・エラー3分類・CHECKマクロのgoto論証・volatile制約・strict-aliasing・依存関係
- `docs/spec/design_mutex.md` — mutex_design.txt 全文（652行）。優先度上限のみの標準構成。
  要素優先度の分析・MTXCB/TCB拡張・mutex_calc_priority/change_priority等の実装コード
- `docs/spec/design_inherit.md` — inherit_design.txt 全文（1,043行）。優先度継承拡張
  （extension/inherit/に対応・本リポジトリに残置を確認）。推移的優先度継承の再帰展開
  （goto transitive_inheritance）＝カーネル内再帰禁止ルールの実例である旨を注記
- 検証：機械突合 design=157・mutex=119・inherit=144（計420トークン）**欠落ゼロ**

**asp_spec / non_tecs / extension 変換（2026-06-07 完了）**

- `docs/spec/asp3_spec_overview.md` — asp_spec.txt 全文（692行）。μITRON4.0との差分
  （導入(1-1)〜(1-6)・廃止(2-1)〜(2-2)・独自拡張(3-1)〜(3-15)・修正(4-1)〜(4-17)）・
  JSP/ASP1比較・拡張パッケージ一覧（extension/実体との対応表を付加）。TECS節は非該当を注記
- `docs/spec/non_tecs_services.md` — non_tecs.txt 全文（623行）。
  **asp3_coreの既定構成**（extension/non_tecsは本体へ統合済み＝不存在を確認）。
  §3（SIOドライバAPI 12関数・低レベル出力・各種マクロ）は**新ターゲット移植時の正本仕様**
- `docs/spec/extension_guide.md` — extension.txt 全文（910行）。エラーチェック省略・
  mutex完全除去・非タスクコンテキスト/CPUロック化・拡張パッケージ9種（acre_*/del_*等の
  追加API全数）・特殊レジスタ・CPU例外直接呼出し。
  **冒頭に禁則①との関係を明記**（本書の改造はすべてPRISTINE変更＝要ユーザー確認・
  DIVERGENCE_MAP記録）。原文の誤記1件（「oCPUロック状態」の衍字）を修正しHTMLコメントで記録
- 検証：機械突合 asp_spec=148・non_tecs=137・extension=334（計619トークン）**欠落ゼロ**
  （extensionの誤記修正は検証が検出→意図的修正としてコメント記録）

**porting.txt 変換（2026-06-07 完了）＝最大の1本（4,761行）を3分割**

- `docs/spec/porting_01_common.md` — §1〜§5（行52〜1416）。共通事項（ディレクトリ構成・命名・
  ヘッダ記述ルール・クリティカルセクション制約）・システム構築環境（Makefile変数）・
  TOPPERS共通定義・SIL・カーネルAPI。原本目次（全節）を本分冊に全文保持（分冊間トレーサビリティ）
- `docs/spec/porting_02_kernel_impl.md` — §6（行1417〜3777）。カーネル実装依存部の全仕様：
  システム状態管理・割込み関連・ディスパッチャ（dispatcher/dispatch/start_dispatch/
  exit_and_dispatch/activate_context）・割込み/CPU例外出入口処理の擬似コード全文・
  起動終了・チューニング・トレースログ・リネーム・高分解能/オーバランタイマドライバ・
  動的メモリ管理（TLSF例）
- `docs/spec/porting_03_cfg_syssvc.md` — §7〜§10（行3778〜4761）。cfg設定ファイル
  （$INTNO_VALID等の変数・関数）・システムサービス（TECS版原文＋非TECS対応表）・
  ドキュメント要件・パッケージ記述・ファイル一覧（asp3_core対応表を付加）

**porting.txt の主なasp3_core注**（実地確認に基づく）：
- 各分冊冒頭：`docs/porting/PORTING_GUIDE.md`（asp3_core固有手順）との共存・役割分担を明記
- §2: Makefile.target→`target.cmake` の**変数対応表**（COPTS→ASP3_COMPILE_OPTIONS等・
  `target/mps2_an521_gcc/target.cmake` で確認）
- §2.5/§7: .trb→.py（`core_offset.py`・`kernel/genoffset.py`・`target_kernel.py`・
  `target_check.py`＝ASP3_*_TRB_FILES で確認）
- §6.12: genrename.rb→`utils/genrename.py`。§6.11.2: TECSセル→slog方式（arch/tracelog/）
- §6.15: 動的メモリ管理は拡張パッケージ専用＝**禁則②（AGENTS §2）との関係を注記**
- §8: TECS→非TECSの**対応表**（sSIOPort→sio_*関数・siSIOCBR→sio_irdy_snd/rcv・
  tPutLog→target_fput_log・target.cdl→target_syssvc.hマクロ。syssvc/serial.c・
  target_serial.h・banner.c で実在確認）。正本は non_tecs_services.md §3 へ誘導
- §9.2: makerelease.rb削除済み（utils/に不存在を確認）・§10.1: ファイル一覧のasp3_core対応表
- 原本の誤記3件をHTMLコメントで記録：(6-4-3-3)→(6-4-4-3)修正・(7-2-1-10)
  $INHNO→$INTNO_FIX_NONKERNEL修正・(7-2-1-7)のTMIN_INTPRI重複は原文ママ＋注記

**検証**：機械突合 分冊毎（515＋445＋254）＋**全文一括**（行52-4761 vs 全3ファイル＝
識別子825・数値31・節番号126 計982トークン）**欠落ゼロ**。

**migration.txt 変換（2026-06-07 完了）＝手順2の変換対象すべて完了**

- `docs/spec/migration_guide.md` — migration.txt 全文（560行）。ASP1との構築環境/カーネル仕様の
  違い（時間単位ms→μs・通知機能・タスク例外/メールボックス廃止・E_RASTER等）・
  μITRON4.0からの移行（データ型/属性の置換表・cfg記述変更・ID自動割付け・sta_tsk代替・
  周期ハンドラ非互換・TLSFによるmalloc/free実現）
- **主なasp3_core注**：
  - 冒頭：構築環境の章（configure.rb/Makefile/TECS前提）はasp3_coreに不適用，
    カーネル仕様の章はそのまま適用，という読み方を明示
  - TECS節に**「逆向きに読むこと」注**：原文が「削除せよ」とする
    `INCLUDE("syssvc/*.cfg")` 4行こそ asp3_coreの標準の組込み方法（非TECS版が既定）
  - 非タスクコンテキスト化の節：extension_guide.md参照＋**禁則①**（PRISTINE変更＝要確認）
  - TLSF節：**アプリ側ライブラリ**であり禁則②（カーネル内動的確保禁止）に抵触しない旨を注記
  - 原本目次の漏れ1件（「非タスクコンテキスト〜サービスコール」節）を補いHTMLコメントで記録
- 検証：機械突合（行54-560）— 識別子150個・数値8個（計158トークン）**欠落ゼロ**

**手順2の総括**：変換対象10本すべて完了（user.txt 13章＝13ファイル・configurator・design系3本・
asp_spec/non_tecs/extension・porting 3分冊・migration＝計24ファイル＋README）。
残置はversion.txt（計画どおり＝マージ追従対象のため原本のまま）。

### 手順3〜5：simtimer.txt削除・参照張り替え・doc/原本削除・記録（2026-06-07 完了）

**手順3**：`doc/simtimer.txt` を削除（機能自体削除済み＝変換対象外。file-cleanup.md の
「対象外」行を「実施済み」へ更新）。

**手順4**：
- 参照の張り替え（6箇所）：`docs/spec/01_overview.md`（extension_guide.mdへの誘導追加）・
  `07_configurator.md`（configurator_spec.mdへ）・`10_test_program.md`（simtimer削除済みへ）・
  `04_directory_structure.md`（整備完了・原本削除済みへ）・`docs/api/get_tim.md`
  （asp_spec.txt→asp3_spec_overview.md）・`target/stm32mp257f_dk_arm64_gcc/target_user.md`
  （user.txt→03_quickstart.md 3.3節）。AGENTS.md・README.md・OVERVIEW.md 等に
  doc/*.txt 参照がないことを確認済み（docs/spec/*.mdの「原本:」行は出自記録として保持）
- **doc/*.txt 原本11本を削除**（ユーザー承認済み・`version.txt` のみ残置）。
  原本は `upstream` ブランチ（`git show upstream:doc/user.txt`）・削除前コミットから取得可能

**手順5**：DIVERGENCE_MAP「削除済み」節へ doc/*.txt 行を追加・file-cleanup.md 更新・
docs/spec/README.md（検証節に照合再実行手順・索引🟢化）・本ファイル総括を記載。

**Git情報**：ベース（本セッション開始時）= e12e68d。Phase1= dc15adb・Phase2= a7e15d7・
Phase3= 1d0ea7d・configurator= c3e2b6b・design系= 98a3641・asp_spec等= 14bd966・
porting= 54a3de6・migration= e33a80a。
リスト再現: `git diff --stat e12e68d HEAD -- docs/ doc/ scripts/ DIVERGENCE_MAP.md`
