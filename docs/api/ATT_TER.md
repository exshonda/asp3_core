# ATT_TER — 終了処理ルーチンの追加（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.11 システム構成管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
ATT_TER({ teratr, exinf, terrtn });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
ATT_TER { .teratr &exinf &terrtn }
```

| 記法 | 意味 |
|---|---|
| `.teratr` | 符号無し整数定数式（属性） |
| `&exinf` | 一般整数定数式（拡張情報） |
| `&terrtn` | 一般整数定数式（終了処理ルーチン先頭番地） |

> `ATT_TER` はオブジェクトIDを持たない（複数登録可能、登録の逆順に実行される）。

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| パケット | `teratr` | `ATR` | 終了処理ルーチン属性（`TA_NULL` のみ） |
| パケット | `exinf` | `intptr_t` | 終了処理ルーチンへの拡張情報 |
| パケット | `terrtn` | `TERRTN` | 終了処理ルーチンの先頭番地 |

## 属性（teratr）
| 属性 | 説明 |
|---|---|
| `TA_NULL` | 属性指定なし（唯一の有効値） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `teratr` が不正 |
| `E_PAR` | `terrtn` が NULL |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`ext_ker` によるカーネル終了時に実行される終了処理ルーチンを追加登録する。

複数の `ATT_TER` を記述した場合、`ATT_INI` と逆順（後から登録したものが先に実行）で呼ばれる。
終了処理ルーチンはカーネルが停止した後の非タスクコンテキストで実行される。

## 使用例
```c
void uart_terminate(intptr_t exinf) {
    /* UART の送信完了を待ってから停止 */
    while (!(UART_SR & UART_TX_EMPTY)) {}
    UART_CR = 0;
}
```
```
ATT_TER({ TA_NULL, 0, uart_terminate });
```

## 関連
- [ATT_INI](ATT_INI.md) — 初期化ルーチンの追加
- `ext_ker` — カーネルの終了
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.11 システム構成管理機能 / 5.2 静的API一覧
