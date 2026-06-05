# ATT_INI — 初期化ルーチンの追加（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.11 システム構成管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
ATT_INI({ iniatr, exinf, inirtn });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
ATT_INI { .iniatr &exinf &inirtn }
```

| 記法 | 意味 |
|---|---|
| `.iniatr` | 符号無し整数定数式（属性） |
| `&exinf` | 一般整数定数式（拡張情報） |
| `&inirtn` | 一般整数定数式（初期化ルーチン先頭番地） |

> `ATT_INI` はオブジェクトIDを持たない（複数登録可能、登録順に実行される）。

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| パケット | `iniatr` | `ATR` | 初期化ルーチン属性（`TA_NULL` のみ） |
| パケット | `exinf` | `intptr_t` | 初期化ルーチンへの拡張情報 |
| パケット | `inirtn` | `INIRTN` | 初期化ルーチンの先頭番地 |

## 属性（iniatr）
| 属性 | 説明 |
|---|---|
| `TA_NULL` | 属性指定なし（唯一の有効値） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `iniatr` が不正 |
| `E_PAR` | `inirtn` が NULL |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
カーネル起動時（スケジューラが動き出す前）に実行される初期化ルーチンを追加登録する。

複数の `ATT_INI` を記述すると、`.cfg` への記述順に実行される。
初期化ルーチンはタスクコンテキスト前の非タスクコンテキストで実行されるため、
`loc_cpu` 等の状態操作は使えるが、タスク依存のサービスコールは使えない。

ハードウェア初期化・ロガー初期化・共有データ初期化などに使う。

## 使用例
```c
void uart_initialize(intptr_t exinf) {
    /* UART ハードウェアの初期化 */
    UART_CR = UART_ENABLE | UART_115200;
}
```
```
ATT_INI({ TA_NULL, 0, uart_initialize });
```

## 関連
- [ATT_TER](ATT_TER.md) — 終了処理ルーチンの追加
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.11 システム構成管理機能 / 5.2 静的API一覧
