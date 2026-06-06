# CFG_INT — 割込み要求ラインの設定（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.9 割込み管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
CFG_INT(intno, { intatr, intpri });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CFG_INT .intno* { .intatr +intpri }
```

| 記法 | 意味 |
|---|---|
| `.intno*` | 割込み要求ライン番号・キーパラメータ |
| `.intatr` | 符号無し整数定数式（属性） |
| `+intpri` | 符号付き整数定数式（割込み優先度） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `intno` | `INTNO` | 割込み要求ライン番号（プロセッサ依存） |
| パケット | `intatr` | `ATR` | 割込み要求ライン属性（下表） |
| パケット | `intpri` | `PRI` | 割込み優先度（負値。`TMIN_INTPRI` 〜 `TMAX_INTPRI`） |

## 属性（intatr）
| 属性 | 説明 |
|---|---|
| `TA_ENAINT` | 初期状態で割込みを許可する |
| `TA_EDGE` | エッジトリガ |
| `TA_LEVEL` | レベルトリガ（デフォルト） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `intatr` が不正 |
| `E_PAR` | `intno` が範囲外、または `intpri` が範囲外 |
| `E_OBJ` | `intno` が重複設定 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`intno` で指定した割込み要求ラインのトリガ種別と優先度を設定する。

`DEF_INH` や `CRE_ISR` で割込みハンドラを登録する際に必ず `CFG_INT` を組み合わせて使う。
`TA_ENAINT` を指定しない場合、割込みは初期状態で禁止（`ena_int` で有効化が必要）。

割込み優先度は負値で表し、絶対値が大きいほど高優先度（例：-5 は -3 より高優先）。

## 使用例
```
/* UART割込み：エッジトリガ・優先度-5・初期有効 */
CFG_INT(INTNO_UART, { TA_ENAINT | TA_EDGE, -5 });

/* GPIO割込み：レベルトリガ・優先度-3・初期禁止 */
CFG_INT(INTNO_GPIO, { TA_LEVEL, -3 });
```

## 関連
- [DEF_INH](DEF_INH.md) — 割込みハンドラの定義
- [CRE_ISR](CRE_ISR.md) — 割込みサービスルーチンの生成
- [dis_int](dis_int.md) / [ena_int](ena_int.md) — 割込みの禁止・許可
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.9 割込み管理機能 / 5.2 静的API一覧
