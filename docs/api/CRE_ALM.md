# CRE_ALM — アラーム通知の生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.6 時間管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
CRE_ALM(almid, { almatr, { nfymode, par1, par2, par3, par4 } });
```

第2パラメータは**通知方式を表すネスト構造**（`T_NFYINFO` 相当）であり、単純なハンドラ番地ではない。

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_ALM #almid* { .almatr { .nfymode &par1 &par2? &par3? &par4? } }
```

| 記法 | 意味 |
|---|---|
| `#almid*` | オブジェクトID定義・キーパラメータ |
| `.almatr` | 符号無し整数定数式（属性、`TA_NULL` のみ） |
| `{ .nfymode &par1 &par2? &par3? &par4? }` | 通知方式のネスト構造（`.nfymode`＝符号無し、`&par*`＝一般、par2以降は省略可） |

## パラメータ
| 位置 | 名称 | 説明 |
|---|---|---|
| 第1引数 | `almid` | 生成するアラームのID番号（`ID`） |
| パケット | `almatr` | アラーム属性（`ATR`、`TA_NULL` のみ） |
| パケット | `{ nfymode, par1.. }` | タイムイベント発生時の**通知方式**（下表） |

## 通知方式（nfymode と par の対応）
| `nfymode` | par1 | par2 | par3 | 動作 |
|---|---|---|---|---|
| `TNFY_HANDLER` | exinf | ハンドラ番地 | — | タイムイベントハンドラを起動 |
| `TNFY_ACTTSK` | tskid | — | — | タスクを起動 |
| `TNFY_WUPTSK` | tskid | — | — | タスクを起床 |
| `TNFY_SIGSEM` | semid | — | — | セマフォを返却 |
| `TNFY_SETFLG` | flgid | setptn | — | イベントフラグをセット |
| `TNFY_SNDDTQ` | dtqid | data | — | データキューに送信 |

（エラー通知 `T_ENFYINFO` を使う拡張形もあるが、基本は上記）

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `almatr` が不正 |
| `E_PAR` | `nfymode` が不正、または par が通知方式に対し不正 |
| `E_ID` | `almid` が不正・範囲外 |
| `E_OBJ` | `almid` が重複登録 |

## 機能
`almid` で指定したアラーム通知を静的に生成する。アラームは `sta_alm(almid, almtim)` で
起動され、相対時間 `almtim` 経過後に **`nfymode` で指定した通知**を1回行う。
最も一般的な `TNFY_HANDLER` では、`par1` を拡張情報、`par2` をハンドラ番地として、
タイムイベントハンドラが呼び出される。

## 使用例
```
/* sample1.cfg より：ハンドラ通知 */
CRE_ALM(ALMHDR1, { TA_NULL, { TNFY_HANDLER, 0, alarm_handler } });

/* タスク起床で通知する例 */
CRE_ALM(ALM_WAKE, { TA_NULL, { TNFY_WUPTSK, MAIN_TASK } });
```

## 関連
- [sta_alm](sta_alm.md) — アラームの動作開始 / `stp_alm` — 停止
- `CRE_CYC` — 周期通知（同じ通知方式構造を使う）
- `kernel/kernel_api.def`（構造の正本）／ 仕様書 4.6 時間管理機能
