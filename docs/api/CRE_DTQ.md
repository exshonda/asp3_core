# CRE_DTQ — データキューの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.4.3 データキュー](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。
> 本ファイルは両者をまとめた人間/AI向けリファレンス。

## 静的API
```
CRE_DTQ(dtqid, { dtqatr, dtqcnt, dtq });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_DTQ #dtqid* { .dtqatr .dtqcnt &dtqmb? }
```

| 記法 | 意味 |
|---|---|
| `#dtqid*` | オブジェクトID定義・キーパラメータ |
| `.dtqatr` | 符号無し整数定数式（属性） |
| `.dtqcnt` | 符号無し整数定数式（キュー容量） |
| `&dtqmb?` | 一般整数定数式・オプション（キュー領域） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `dtqid` | `ID` | 生成するデータキューのID番号 |
| パケット | `dtqatr` | `ATR` | データキュー属性（下表） |
| パケット | `dtqcnt` | `uint32_t` | データキューの容量（データ数）。0 でバッファなし（直接受渡し専用） |
| パケット | `dtqmb` | `void *` | データキュー領域の先頭番地（省略可。`NULL` で自動割付け） |

## 属性（dtqatr）
| 属性 | 説明 |
|---|---|
| `TA_TFIFO` | 送信待ちタスクのキューをFIFO順にする（デフォルト） |
| `TA_TPRI` | 送信待ちタスクのキューを優先度順にする |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `dtqatr` が不正 |
| `E_ID` | `dtqid` が不正・範囲外 |
| `E_OBJ` | `dtqid` が重複登録 |
| `E_NOMEM` | データキュー領域が確保できない（自動割付け時） |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`dtqid` で指定したIDのデータキューを静的に生成する。

`dtqcnt` に 0 を指定するとバッファなし（`snd_dtq` は受信待ちタスクがいないと即ブロック）。
`dtqmb` を省略または `NULL` にすると、コンフィギュレータが `dtqcnt` 個分の領域を自動割付けする。

## 使用例
```
CRE_DTQ(MSG_DTQ, { TA_TFIFO, 8, NULL });    /* 8件バッファ・自動割付け */
CRE_DTQ(CMD_DTQ, { TA_TPRI,  0, NULL });    /* バッファなし・直接受渡し */
```

## 関連
- [snd_dtq](snd_dtq.md) — データキュー送信
- [rcv_dtq](rcv_dtq.md) — データキュー受信
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.4.3 データキュー / 5.2 静的API一覧
