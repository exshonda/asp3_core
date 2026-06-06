# DEF_INH — 割込みハンドラの定義（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.9 割込み管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
DEF_INH(inhno, { inhatr, inthdr });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
DEF_INH .inhno* { .inhatr &inthdr }
```

| 記法 | 意味 |
|---|---|
| `.inhno*` | 割込みハンドラ番号・キーパラメータ |
| `.inhatr` | 符号無し整数定数式（属性） |
| `&inthdr` | 一般整数定数式（ハンドラ先頭番地） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `inhno` | `INHNO` | 割込みハンドラ番号（プロセッサ依存） |
| パケット | `inhatr` | `ATR` | 割込みハンドラ属性（`TA_NULL` のみ） |
| パケット | `inthdr` | `INTHDR` | 割込みハンドラの先頭番地 |

## 属性（inhatr）
| 属性 | 説明 |
|---|---|
| `TA_NULL` | 属性指定なし（唯一の有効値） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `inhatr` が不正 |
| `E_PAR` | `inhno` が範囲外、または `inthdr` が NULL |
| `E_OBJ` | `inhno` が重複定義 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`inhno` で指定した割込みハンドラ番号に `inthdr` を登録する。

`DEF_INH` はカーネルが割込みベクタを直接管理するモデル（割込みハンドラ方式）。
割込みハンドラは非タスクコンテキストで実行され、カーネル管理の割込み禁止状態になる前に呼ばれる。

`CRE_ISR`（割込みサービスルーチン方式）と組み合わせるのではなく、どちらか一方を使う。
`DEF_INH` は低レイテンシが求められるハンドラや、`CRE_ISR` で対応できない割込みに使う。

## 使用例
```c
void uart_isr(void) {
    /* 割込みハンドラ本体（カーネルスタック上で動作） */
    uint8_t ch = UART_RDR;
    psnd_dtq(UART_DTQ, (intptr_t)ch);
}
```
```
DEF_INH(INHNO_UART, { TA_NULL, uart_isr });
CFG_INT(INTNO_UART, { TA_ENAINT | TA_EDGE, -5 });
```

## 関連
- [CFG_INT](CFG_INT.md) — 割込み要求ラインの設定（DEF_INH と組み合わせて使う）
- [CRE_ISR](CRE_ISR.md) — 割込みサービスルーチン方式（代替手段）
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.9 割込み管理機能 / 5.2 静的API一覧
