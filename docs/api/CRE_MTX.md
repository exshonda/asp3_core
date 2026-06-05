# CRE_MTX — ミューテックスの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.4.5 ミューテックス](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
CRE_MTX(mtxid, { mtxatr, ceilpri });
```

`TA_CEILING` 属性なしの場合、`ceilpri` は省略可：
```
CRE_MTX(mtxid, { mtxatr });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_MTX #mtxid* { .mtxatr +ceilpri? }
```

| 記法 | 意味 |
|---|---|
| `#mtxid*` | オブジェクトID定義・キーパラメータ |
| `.mtxatr` | 符号無し整数定数式（属性） |
| `+ceilpri?` | 符号付き整数定数式・オプション（上限優先度） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `mtxid` | `ID` | 生成するミューテックスのID番号 |
| パケット | `mtxatr` | `ATR` | ミューテックス属性（下表） |
| パケット | `ceilpri` | `PRI` | 優先度上限（`TA_CEILING` 指定時のみ必須） |

## 属性（mtxatr）
| 属性 | 説明 |
|---|---|
| `TA_TPRI` | 待ちタスクのキューを優先度順にする |
| `TA_INHERIT` | 優先度継承プロトコルを使用する |
| `TA_CEILING` | 優先度上限プロトコルを使用する（`ceilpri` 必須） |

> `TA_INHERIT` と `TA_CEILING` は排他。いずれも指定しない場合は優先度逆転の防止なし。

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `mtxatr` が不正、または `TA_CEILING` なしで `ceilpri` 指定 |
| `E_PAR` | `ceilpri` が範囲外（`TA_CEILING` 時） |
| `E_ID` | `mtxid` が不正・範囲外 |
| `E_OBJ` | `mtxid` が重複登録 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`mtxid` で指定したIDのミューテックスを静的に生成する。

ミューテックスはセマフォと異なり、ロックしたタスクのみがロック解除できる（オーナーシップ保証）。
優先度逆転を防ぎたい場合は `TA_INHERIT`（優先度継承）または `TA_CEILING`（優先度上限）を指定する。

## 使用例
```
/* 優先度継承ミューテックス */
CRE_MTX(SHARED_MTX, { TA_TPRI | TA_INHERIT });

/* 優先度上限ミューテックス（上限優先度3） */
CRE_MTX(CEIL_MTX, { TA_TPRI | TA_CEILING, 3 });
```

## 関連
- `loc_mtx` — ミューテックスのロック
- `unl_mtx` — ミューテックスのロック解除
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.4.5 ミューテックス / 5.2 静的API一覧
