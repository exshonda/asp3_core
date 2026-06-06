# 移植検証テスト（test/porting）

## 項目

移植検証テスト（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

新ターゲット移植時の**最初の動作確認**として、カーネル基本機能6項目を
TAP形式で機械判定する `test/porting/` を整備する。
プロトタイプ（AI提案）が旧ツリーにあり、これを取込み・修正して実体化する：
**`/home/honda/TOPPERS/asp3core/asp3_core_org/test/porting/`**
（tap.[ch]＝最小TAPフレームワーク・test_porting.c＝テスト本体・
test_porting.cfg／test_porting_cfg.h・README.md。計約290行）

### 意義（何が嬉しいか）

1. **新ポートの「最初の灯」を機械判定**：sample1（目視）・testexec全件（重い）の
   前段として、6項目のTAPで合否が出る。項目の並びが**故障切り分けの順序**に
   なっている：
   ①syslog_output（ブート・UART）→②tick_timer_basic（タイマ歩進）→
   ③task_create_activate（ディスパッチャ）→④⑤sem/flg（カーネル本体）→
   ⑥alarm_handler（**タイマ割込み経路**）
2. **PORTING_GUIDE Step 8-2 の実体化**：ガイドが先行記述している
   `test/porting/` の移植確認テスト（期待TAP出力例まで一致）が実在する状態になる
3. **直近の利用先＝pico2-riscv**：Xh3irq（新割込みコントローラ）・新ブートの
   切り分けに①②⑥が直接効く。以降のポート（FSP統合等）でも定番手順になる

### プロトタイプからの修正ポイント（取込み時に対応）

| 箇所 | 問題 | 対処 |
|---|---|---|
| `tap_done()` | `qemu_exit(int)` 依存（arm32セミホスティング前提・全ターゲットでは通用しない） | 基本は **`# N/N passed` のパース判定**（run_testexec.py方式・全ターゲット共通）＋ `ext_ker()`。終了コードは linux（ネイティブ）でのみ活用（ctest直結） |
| `test_porting.cfg` | 優先度・スタックサイズ等の定数 | 現行の test/ 慣例（target_test.h）と突合 |
| `expected/sample1.json` | 旧ツリー版 | 現リポジトリ版（CLIターゲットで作成済み）を正とし、重複は取込まない |
| ビルド方法 | README記載の `--target test_porting` | 専用ターゲットは作らず、既存の `ASP3_APPLNAME/APPLDIR` 機構を使用（`-DASP3_APPLDIR=test/porting -DASP3_APPLNAME=test_porting`）。READMEを実手順に更新 |

## 実施プラン

1. **取込み・修正**：`asp3_core_org/test/porting/` → `test/porting/`
   （上記修正ポイントを反映。tap.[ch] は test/porting/ 配下に置き
   syssvc/test_svc.c とは独立のまま）
2. **既存5実行ターゲットで検証**（テスト自体の検証）：
   linux（ネイティブ・ctest登録＝終了コード判定）＋
   QEMU 4機種（mps2／zybo／zcu102／polarfire）で **6/6 passed** を確認
3. **ドキュメント整合**：
   - `PORTING_GUIDE.md` Step 8-2 を実手順（APPLDIR方式のコマンド）に更新
   - `docs/dev/pico2-riscv.md` の実機検証手順を
     「**test_porting → sample1 → testexec**」の順に更新（本コミットで実施済み）
   - `docs/building.md` テストランナ節に追記
4. **CI**（任意・小）：linuxジョブのctestに test_porting が乗る（2の登録で自動）。
   QEMUスモークの置換/追加は効果と時間を見て判断
5. **記録**：README索引・本ファイル実施結果

### スコープ外

- テスト項目の追加（割込み応答時間等の性能系）— 必要になったら拡張
- 実機ターゲット（pico2/stm32）での実行確認 — 各ポートの実機検証時に実施

## 実施結果

（完了時に記載）
