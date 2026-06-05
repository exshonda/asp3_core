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

整備済みのものはリンク、未整備は仕様書の該当節を参照して生成する。

### タスク管理（4.1）
- [act_tsk](act_tsk.md) — タスクの起動
- can_act — タスク起動要求のキャンセル
- chg_pri — タスク優先度の変更
- get_pri — タスク優先度の参照
- get_inf — 拡張情報の参照
- ref_tsk — タスク状態の参照
- get_tst — タスク状態の参照（簡易）

### タスク付属同期（4.2）
- [slp_tsk](slp_tsk.md) / tslp_tsk — 起床待ち / タイムアウト付き
- [wup_tsk](wup_tsk.md) — タスクの起床
- can_wup — 起床要求のキャンセル
- rel_wai — 待ち状態の強制解除
- sus_tsk / rsm_tsk — 強制待ち / 再開
- [dly_tsk](dly_tsk.md) — 時間経過待ち

### タスク終了（4.3）
- ext_tsk — 自タスクの終了
- ras_ter — タスク終了要求
- dis_ter / ena_ter / sns_ter — 終了要求拒否の制御・参照
- ter_tsk — タスクの強制終了

### セマフォ（4.4.1）
- [sig_sem](sig_sem.md) — セマフォ資源の返却
- [wai_sem](wai_sem.md) — セマフォ資源の獲得
- pol_sem — 獲得（ポーリング）
- twai_sem — 獲得（タイムアウト付き）
- ref_sem — セマフォ状態の参照

### イベントフラグ（4.4.2）
- [set_flg](set_flg.md) / [clr_flg](clr_flg.md) — ビットのセット / クリア
- [wai_flg](wai_flg.md) / pol_flg / twai_flg — 待ち / ポーリング / タイムアウト付き
- ref_flg — 状態の参照

### データキュー（4.4.3）
- [snd_dtq](snd_dtq.md) / psnd_dtq / tsnd_dtq / fsnd_dtq — 送信各種
- [rcv_dtq](rcv_dtq.md) / prcv_dtq / trcv_dtq — 受信各種
- ref_dtq — 状態の参照

### 優先度データキュー（4.4.4）
- snd_pdq / rcv_pdq 系

### ミューテックス（4.4.5）
- loc_mtx / ploc_mtx / tloc_mtx / unl_mtx / ref_mtx

### 固定長メモリプール（4.5.1）
- get_mpf / pget_mpf / tget_mpf / rel_mpf / ref_mpf

### 時間管理（4.6）
- [get_tim](get_tim.md) — システム時刻の参照（ms）
- get_utm — 性能評価用システム時刻の参照
- fch_hrt — 高分解能タイマの参照
- sta_cyc / stp_cyc / ref_cyc — 周期通知
- [sta_alm](sta_alm.md) / stp_alm / ref_alm — アラーム通知

### システム状態管理（4.7）
- rot_rdq — レディキューの回転
- get_did — 自タスクの所属ドメインID参照
- loc_cpu / unl_cpu — CPUロック / 解除
- dis_dsp / ena_dsp — ディスパッチ禁止 / 許可
- sns_ctx / sns_loc / sns_dsp / sns_dpn / sns_ker — 状態参照
- ext_ker — カーネルの終了

### 割込み管理（4.9）
- dis_int / ena_int — 割込み禁止 / 許可
- chg_ipm / get_ipm — 割込み優先度マスクの変更 / 参照

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
- [CRE_TSK](CRE_TSK.md) — タスクの生成 ← 整備済み（正本 `kernel/kernel_api.def`）
- [CRE_SEM](CRE_SEM.md) — セマフォの生成
- [CRE_FLG](CRE_FLG.md) — イベントフラグの生成
- [CRE_DTQ](CRE_DTQ.md) — データキューの生成
- [CRE_PDQ](CRE_PDQ.md) — 優先度データキューの生成
- [CRE_MTX](CRE_MTX.md) — ミューテックスの生成
- [CRE_MPF](CRE_MPF.md) — 固定長メモリプールの生成
- [CRE_CYC](CRE_CYC.md) — 周期通知の生成
- [CRE_ALM](CRE_ALM.md) — アラーム通知の生成

### 定義・登録系
- [DEF_INH](DEF_INH.md) — 割込みハンドラの定義
- [CFG_INT](CFG_INT.md) — 割込み要求ラインの設定
- [ATT_ISR](ATT_ISR.md) — 割込みサービスルーチンの追加
- [DEF_EXC](DEF_EXC.md) — CPU例外ハンドラの定義
- [ATT_INI](ATT_INI.md) — 初期化ルーチンの追加
- [ATT_TER](ATT_TER.md) — 終了ルーチンの追加
- [DEF_SVC](DEF_SVC.md) — 拡張サービスコールの定義

---

## 整備方針

- まず**使用頻度の高いコール**（act_tsk, wup_tsk, slp_tsk, dly_tsk, sig_sem, wai_sem, set_flg, wai_flg, snd_dtq, rcv_dtq, sta_alm, get_tim）から整備する。
- **静的APIは `kernel/kernel_api.def`（api-table）を構造の正本として整備する**（CRE_TSK ＝ その1行が雛形）。`.cfg` を書く全作業の前提になるため。
- 整備済みの例（サービスコール：act_tsk / wai_sem / dly_tsk、静的API：CRE_TSK）をテンプレートとして、残りはAIが仕様書および `kernel_api.def` から生成する。
- AIに生成依頼する際は「仕様書の該当節と `kernel/kernel_api.def` を参照し、テンプレートに沿って作成せよ」と指示する。
