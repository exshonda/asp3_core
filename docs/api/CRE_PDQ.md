# CRE_PDQ — 優先度データキューの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.4.4 優先度データキュー](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
CRE_PDQ(pdqid, { pdqatr, pdqcnt, maxdpri, pdq });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_PDQ #pdqid* { .pdqatr .pdqcnt +maxdpri &pdqmb? }
```

| 記法 | 意味 |
|---|---|
| `#pdqid*` | オブジェクトID定義・キーパラメータ |
| `.pdqatr` | 符号無し整数定数式（属性） |
| `.pdqcnt` | 符号無し整数定数式（キュー容量） |
| `+maxdpri` | 符号無し整数定数式（送信優先度の最大値） |
| `&pdqmb?` | 一般整数定数式・オプション（キュー領域） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `pdqid` | `ID` | 生成する優先度データキューのID番号 |
| パケット | `pdqatr` | `ATR` | 優先度データキュー属性（下表） |
| パケット | `pdqcnt` | `uint32_t` | キューの容量（データ数）。0 でバッファなし |
| パケット | `maxdpri` | `PRI` | 送信データ優先度の最大値（1 〜 `TMAX_DPRI`） |
| パケット | `pdqmb` | `void *` | キュー領域の先頭番地（省略可。`NULL` で自動割付け） |

## 属性（pdqatr）
| 属性 | 説明 |
|---|---|
| `TA_TFIFO` | 送信待ちタスクのキューをFIFO順にする（デフォルト） |
| `TA_TPRI` | 送信待ちタスクのキューを優先度順にする |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `pdqatr` が不正 |
| `E_PAR` | `maxdpri` が範囲外 |
| `E_ID` | `pdqid` が不正・範囲外 |
| `E_OBJ` | `pdqid` が重複登録 |
| `E_NOMEM` | キュー領域が確保できない（自動割付け時） |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`pdqid` で指定したIDの優先度データキューを静的に生成する。

優先度データキューはデータに送信優先度（1〜`maxdpri`、値が小さいほど高優先）を付けて送受信する。
受信側は常に最高優先度のデータを先に受け取る。

## 使用例
```
/* 最大優先度16・8件バッファ */
CRE_PDQ(LOG_PDQ, { TA_TFIFO, 8, 16, NULL });
```

## 関連
- `snd_pdq` — 優先度データキュー送信
- `rcv_pdq` — 優先度データキュー受信
- [CRE_DTQ](CRE_DTQ.md) — 優先度なしデータキュー
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.4.4 優先度データキュー / 5.2 静的API一覧
