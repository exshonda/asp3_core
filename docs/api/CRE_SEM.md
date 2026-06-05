# CRE_SEM — セマフォの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。** パラメータ列・型はそこで宣言される。
> 意味・エラー・使用例の正本は [統合仕様書 4.4.1 セマフォ](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。
> 本ファイルは両者をまとめた人間/AI向けリファレンス。

## 静的API
```
CRE_SEM(semid, { sematr, isemcnt, maxsem });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_SEM #semid* { .sematr .isemcnt .maxsem }
```

| 記法 | 意味 |
|---|---|
| `#semid*` | オブジェクトID定義・キーパラメータ |
| `.sematr` | 符号無し整数定数式（属性） |
| `.isemcnt` | 符号無し整数定数式（初期資源数） |
| `.maxsem` | 符号無し整数定数式（最大資源数） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `semid` | `ID` | 生成するセマフォのID番号 |
| パケット | `sematr` | `ATR` | セマフォ属性（下表） |
| パケット | `isemcnt` | `uint32_t` | セマフォ資源数の初期値（0 ≤ isemcnt ≤ maxsem） |
| パケット | `maxsem` | `uint32_t` | セマフォ資源数の最大値（1 以上） |

## 属性（sematr）
| 属性 | 説明 |
|---|---|
| `TA_TFIFO` | 待ちタスクのキューをFIFO順にする（デフォルト） |
| `TA_TPRI` | 待ちタスクのキューを優先度順にする |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `sematr` が不正 |
| `E_PAR` | `isemcnt > maxsem` または `maxsem` が 0 |
| `E_ID` | `semid` が不正・範囲外 |
| `E_OBJ` | `semid` が重複登録 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`semid` で指定したIDのセマフォを静的に生成する。

初期資源数 `isemcnt` と最大資源数 `maxsem` を設定する。
`TA_TPRI` を指定すると、資源待ちタスクが優先度順にキューイングされる。
`TA_TFIFO`（または指定なし）はFIFO順。

## 使用例
```
CRE_SEM(UART_SEM,     { TA_TFIFO, 0, 1 });   /* バイナリセマフォ（初期0） */
CRE_SEM(RESOURCE_SEM, { TA_TFIFO, 1, 1 });   /* 相互排除セマフォ（初期1） */
CRE_SEM(POOL_SEM,     { TA_TPRI,  4, 4 });   /* カウンタセマフォ（4資源） */
```

## 関連
- [wai_sem](wai_sem.md) — セマフォ資源の獲得
- [sig_sem](sig_sem.md) — セマフォ資源の返却
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.4.1 セマフォ / 5.2 静的API一覧
