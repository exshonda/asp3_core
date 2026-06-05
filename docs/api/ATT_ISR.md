# ATT_ISR — 割込みサービスルーチンの追加（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.9 割込み管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
ATT_ISR({ isratr, exinf, intno, isr, isrpri });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
ATT_ISR { .isratr &exinf .intno &isr +isrpri }
```

| 記法 | 意味 |
|---|---|
| `.isratr` | 符号無し整数定数式（属性） |
| `&exinf` | 一般整数定数式（拡張情報） |
| `.intno` | 符号無し整数定数式（割込み要求ライン番号） |
| `&isr` | 一般整数定数式（ISR先頭番地） |
| `+isrpri` | 符号付き整数定数式（ISR優先度） |

> `ATT_ISR` はオブジェクトIDを持たない（複数のISRを同一割込みに登録可能）。

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| パケット | `isratr` | `ATR` | ISR属性（`TA_NULL` のみ） |
| パケット | `exinf` | `intptr_t` | ISRへの拡張情報 |
| パケット | `intno` | `INTNO` | 割込み要求ライン番号 |
| パケット | `isr` | `ISR` | ISRの先頭番地 |
| パケット | `isrpri` | `PRI` | ISR優先度（1 〜 `TMAX_ISRPRI`、値が小さいほど高優先） |

## 属性（isratr）
| 属性 | 説明 |
|---|---|
| `TA_NULL` | 属性指定なし（唯一の有効値） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `isratr` が不正 |
| `E_PAR` | `intno` が範囲外、`isr` が NULL、または `isrpri` が範囲外 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`intno` で指定した割込み要求ラインに ISR（割込みサービスルーチン）を追加登録する。

`DEF_INH` と異なり、複数の ISR を同一の割込みに登録できる（`isrpri` の昇順に呼ばれる）。
ISR はカーネルが提供する割込みハンドラから呼ばれるため、`DEF_INH` より若干オーバーヘッドがある。

`CFG_INT` と組み合わせて使う。

## 使用例
```c
void uart_rx_isr(intptr_t exinf) {
    uint8_t ch = UART_RDR;
    psnd_dtq(UART_DTQ, (intptr_t)ch);
}
```
```
ATT_ISR({ TA_NULL, 0, INTNO_UART, uart_rx_isr, 1 });
CFG_INT(INTNO_UART, { TA_ENAINT | TA_EDGE, -5 });
```

## 関連
- [DEF_INH](DEF_INH.md) — 割込みハンドラの直接定義（代替手段）
- [CFG_INT](CFG_INT.md) — 割込み要求ラインの設定
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.9 割込み管理機能 / 5.2 静的API一覧
