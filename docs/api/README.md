# docs/api — カーネルAPIリファレンス

> ASP3カーネルのサービスコールを **1コール=1ファイル** のMarkdownで記述する。  
> RAG・AIエージェントの検索精度を上げるための構造化リファレンス。  
> 正式仕様は [TOPPERS第3世代カーネル統合仕様書 Release 3.7.0](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html) を正とする。

---

## 記述テンプレート

各 `docs/api/<service_call>.md` は以下の構成で記述する：

```markdown
# <service_call> — <一言説明>

## 種別
〔T〕タスクコンテキスト専用 / 〔I〕非タスク専用 / 〔TI〕両方 / 〔S〕静的API

## C言語API
\`\`\`c
ER ercd = <service_call>(<params>);
\`\`\`

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|

## エラーコード
| コード | 条件 |
|---|---|

## 機能
（動作の説明）

## 使用例
\`\`\`c
\`\`\`

## 関連
- 関連するサービスコールへのリンク
- 仕様書の該当節
```

> 種別記号：〔T〕は非タスクコンテキストから呼ぶと `E_CTX`、〔I〕はタスクコンテキストから呼ぶと `E_CTX`。

---

## サービスコール一覧（ASP3）

**全95本整備済み**（`include/kernel.h` の宣言と全数一致。2026-06確認）。

### タスク管理（4.1）
- [act_tsk](act_tsk.md) — タスクの起動
- [can_act](can_act.md) — タスク起動要求のキャンセル
- [chg_pri](chg_pri.md) — タスク優先度の変更
- [get_pri](get_pri.md) — タスク優先度の参照
- [get_inf](get_inf.md) — 自タスクの拡張情報の参照
- [ref_tsk](ref_tsk.md) — タスク状態の参照
- [get_tst](get_tst.md) — タスク状態の参照（簡易）

### タスク付属同期（4.2）
- [slp_tsk](slp_tsk.md) / [tslp_tsk](tslp_tsk.md) — 起床待ち / タイムアウト付き
- [wup_tsk](wup_tsk.md) — タスクの起床
- [can_wup](can_wup.md) — 起床要求のキャンセル
- [rel_wai](rel_wai.md) — 待ち状態の強制解除
- [sus_tsk](sus_tsk.md) / [rsm_tsk](rsm_tsk.md) — 強制待ち / 再開
- [dly_tsk](dly_tsk.md) — 時間経過待ち

### タスク終了（4.3）
- [ext_tsk](ext_tsk.md) — 自タスクの終了
- [ras_ter](ras_ter.md) — タスク終了要求
- [dis_ter](dis_ter.md) / [ena_ter](ena_ter.md) / [sns_ter](sns_ter.md) — 終了要求拒否の制御・参照
- [ter_tsk](ter_tsk.md) — タスクの強制終了

### セマフォ（4.4.1）
- [sig_sem](sig_sem.md) — セマフォ資源の返却
- [wai_sem](wai_sem.md) — セマフォ資源の獲得
- [pol_sem](pol_sem.md) — 獲得（ポーリング）
- [twai_sem](twai_sem.md) — 獲得（タイムアウト付き）
- [ini_sem](ini_sem.md) — セマフォの再初期化
- [ref_sem](ref_sem.md) — セマフォ状態の参照

### イベントフラグ（4.4.2）
- [set_flg](set_flg.md) / [clr_flg](clr_flg.md) — ビットのセット / クリア
- [wai_flg](wai_flg.md) / [pol_flg](pol_flg.md) / [twai_flg](twai_flg.md) — 待ち / ポーリング / タイムアウト付き
- [ini_flg](ini_flg.md) — イベントフラグの再初期化
- [ref_flg](ref_flg.md) — 状態の参照

### データキュー（4.4.3）
- [snd_dtq](snd_dtq.md) / [psnd_dtq](psnd_dtq.md) / [tsnd_dtq](tsnd_dtq.md) / [fsnd_dtq](fsnd_dtq.md) — 送信 / ポーリング / タイムアウト付き / 強制
- [rcv_dtq](rcv_dtq.md) / [prcv_dtq](prcv_dtq.md) / [trcv_dtq](trcv_dtq.md) — 受信 / ポーリング / タイムアウト付き
- [ini_dtq](ini_dtq.md) — データキューの再初期化
- [ref_dtq](ref_dtq.md) — 状態の参照

### 優先度データキュー（4.4.4）
- [snd_pdq](snd_pdq.md) / [psnd_pdq](psnd_pdq.md) / [tsnd_pdq](tsnd_pdq.md) — 送信 / ポーリング / タイムアウト付き
- [rcv_pdq](rcv_pdq.md) / [prcv_pdq](prcv_pdq.md) / [trcv_pdq](trcv_pdq.md) — 受信 / ポーリング / タイムアウト付き
- [ini_pdq](ini_pdq.md) — 優先度データキューの再初期化
- [ref_pdq](ref_pdq.md) — 状態の参照

### ミューテックス（4.4.5）
- [loc_mtx](loc_mtx.md) / [ploc_mtx](ploc_mtx.md) / [tloc_mtx](tloc_mtx.md) — ロック / ポーリング / タイムアウト付き
- [unl_mtx](unl_mtx.md) — ロック解除
- [ini_mtx](ini_mtx.md) — ミューテックスの再初期化
- [ref_mtx](ref_mtx.md) — 状態の参照

### 固定長メモリプール（4.5.1）
- [get_mpf](get_mpf.md) / [pget_mpf](pget_mpf.md) / [tget_mpf](tget_mpf.md) — 獲得 / ポーリング / タイムアウト付き
- [rel_mpf](rel_mpf.md) — 返却
- [ini_mpf](ini_mpf.md) — 固定長メモリプールの再初期化
- [ref_mpf](ref_mpf.md) — 状態の参照

### 時間管理（4.6）
- [get_tim](get_tim.md) — システム時刻の参照（μs）
- [set_tim](set_tim.md) — システム時刻の設定
- [adj_tim](adj_tim.md) — システム時刻の調整
- [fch_hrt](fch_hrt.md) — 高分解能タイマの参照（μs）
- [sta_cyc](sta_cyc.md) / [stp_cyc](stp_cyc.md) / [ref_cyc](ref_cyc.md) — 周期通知の開始 / 停止 / 参照
- [sta_alm](sta_alm.md) / [stp_alm](stp_alm.md) / [ref_alm](ref_alm.md) — アラーム通知の開始 / 停止 / 参照

### システム状態管理（4.7）
- [rot_rdq](rot_rdq.md) — タスクの優先順位の回転
- [get_tid](get_tid.md) — 実行状態のタスクIDの参照
- [get_lod](get_lod.md) / [get_nth](get_nth.md) — 実行できるタスク数 / 優先順位指定のタスクID参照
- [loc_cpu](loc_cpu.md) / [unl_cpu](unl_cpu.md) — CPUロック / 解除
- [dis_dsp](dis_dsp.md) / [ena_dsp](ena_dsp.md) — ディスパッチ禁止 / 許可
- [sns_ctx](sns_ctx.md) / [sns_loc](sns_loc.md) / [sns_dsp](sns_dsp.md) / [sns_dpn](sns_dpn.md) / [sns_ker](sns_ker.md) — 状態参照
- [ext_ker](ext_ker.md) — カーネルの終了

### 割込み管理（4.9）
- [dis_int](dis_int.md) / [ena_int](ena_int.md) — 割込み禁止 / 許可（ターゲット定義）
- [clr_int](clr_int.md) / [ras_int](ras_int.md) / [prb_int](prb_int.md) — 要求クリア / 要求 / 要求チェック（ターゲット定義）
- [chg_ipm](chg_ipm.md) / [get_ipm](get_ipm.md) — 割込み優先度マスクの変更 / 参照

### CPU例外管理（4.10）
- [xsns_dpn](xsns_dpn.md) — CPU例外発生時のディスパッチ保留状態の参照

---

## 静的API一覧（ASP3）

> **静的APIはサービスコールより重要度が高い。** ASP3は動的生成を持たないため、
> オブジェクトはすべて静的APIで生成される。AIが `.cfg` を書く際の必須情報。

### 正本は kernel_api.def（api-table）

静的APIの構造（パラメータ列・型・順序）は、Python cfg が読む **api-table の `.def` ファイル**
（`kernel/kernel_api.def`）に既に宣言されている。cfgエンジン（`cfg.py` / `pass1.py` / `pass2.py`）は
このファイルを `--api-table` で読み込んで解釈する。したがって：

```
kernel/kernel_api.def    ← 構造の正本（cfg が --api-table で読む）
        ↓ 参照
docs/api/<STATIC_API>.md ← 構造 + 意味・エラー・例 をまとめた人間/AI可読リファレンス
```

`.def` は単純なテキストDSL（`#`=ID定義, `.`=符号無し, `+`=符号付き, `&`=一般, `^`=ポインタ,
`$`=文字列、後置 `*`=キー, `?`=オプション, `...`=リスト）で、**上流と同形式のためテキストマージ可能**。
別途YAML等の二つ目の表現を持つ必要はない。docs/api は `.def` の構造に、仕様書由来の意味・エラー・
使用例を加えたものとして記述する。

> マージ追従の観点：cfgエンジン（cfg.py等）はRuby→Python移植のDIVERGED部分で個別追従が必要だが、
> 静的API定義（`kernel_api.def`）は上流と同形式のためテキストマージできる。`docs/CFG_SPEC_MAP.md` 参照。

### 生成系（オブジェクト生成）
- [CRE_TSK](CRE_TSK.md) — タスクの生成（正本 `kernel/kernel_api.def`）
- [CRE_SEM](CRE_SEM.md) — セマフォの生成
- [CRE_FLG](CRE_FLG.md) — イベントフラグの生成
- [CRE_DTQ](CRE_DTQ.md) — データキューの生成
- [CRE_PDQ](CRE_PDQ.md) — 優先度データキューの生成
- [CRE_MTX](CRE_MTX.md) — ミューテックスの生成
- [CRE_MPF](CRE_MPF.md) — 固定長メモリプールの生成
- [CRE_CYC](CRE_CYC.md) — 周期通知の生成
- [CRE_ALM](CRE_ALM.md) — アラーム通知の生成
- [CRE_ISR](CRE_ISR.md) — 割込みサービスルーチンの生成

### 定義・登録系
- [DEF_INH](DEF_INH.md) — 割込みハンドラの定義
- [CFG_INT](CFG_INT.md) — 割込み要求ラインの設定
- [DEF_EXC](DEF_EXC.md) — CPU例外ハンドラの定義
- [DEF_ICS](DEF_ICS.md) — 非タスクコンテキスト用スタック領域の定義
- [ATT_INI](ATT_INI.md) — 初期化ルーチンの追加
- [ATT_TER](ATT_TER.md) — 終了ルーチンの追加

> `kernel/kernel_api.def` の全16エントリと1対1対応（2026-06確認）。
> μITRON4.0の `ATT_ISR` はASP3では `CRE_ISR`（ISRをID付きオブジェクトとして生成）に置き換えられている。
> `DEF_SVC`（拡張サービスコール）はHRP3の機能でありASP3には存在しない。

---

## 整備方針

- **全数整備済み**：サービスコール95本（`include/kernel.h` と全数一致）＋静的API16本（`kernel/kernel_api.def` と1対1対応）。
- エラーコード・種別（〔T〕〔I〕〔TI〕）はカーネルソースの `CHECK_*` マクロ・実装から導出し、推測で書かない。
- 静的APIは `kernel/kernel_api.def`（api-table）を構造の正本とする。
- 新規サービスコール・静的APIが上流マージで追加された場合は、`include/kernel.h` / `kernel_api.def` との突合（`comm` で差分確認）→ 既存フォーマットで追記する。
