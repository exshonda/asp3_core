# DEF_EXC — CPU例外ハンドラの定義（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.10 CPU例外管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
DEF_EXC(excno, { excatr, exchdr });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
DEF_EXC .excno* { .excatr &exchdr }
```

| 記法 | 意味 |
|---|---|
| `.excno*` | CPU例外ハンドラ番号・キーパラメータ |
| `.excatr` | 符号無し整数定数式（属性） |
| `&exchdr` | 一般整数定数式（ハンドラ先頭番地） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `excno` | `EXCNO` | CPU例外ハンドラ番号（プロセッサ依存） |
| パケット | `excatr` | `ATR` | CPU例外ハンドラ属性（`TA_NULL` のみ） |
| パケット | `exchdr` | `EXCHDR` | CPU例外ハンドラの先頭番地 |

## 属性（excatr）
| 属性 | 説明 |
|---|---|
| `TA_NULL` | 属性指定なし（唯一の有効値） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `excatr` が不正 |
| `E_PAR` | `excno` が範囲外、または `exchdr` が NULL |
| `E_OBJ` | `excno` が重複定義 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`excno` で指定した CPU 例外ハンドラ番号に `exchdr` を登録する。

CPU 例外（ハードフォルト・バスフォルト・UsageFault等）が発生すると `exchdr` が呼ばれる。
ハンドラはタスクコンテキスト・非タスクコンテキストのいずれで例外が発生した場合も非タスクコンテキストで実行される。

ARM Cortex-M では `excno` にシステム例外番号（例：`EXCNO_HARDFAULT`）を指定する（ターゲット依存マクロを使う）。

## 使用例
```c
void hardfault_handler(void *p_excinf) {
    /* ハードフォルト発生時の処理 */
    syslog(LOG_EMERG, "HardFault");
    ext_ker();
}
```
```
DEF_EXC(EXCNO_HARDFAULT, { TA_NULL, hardfault_handler });
```

## 関連
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.10 CPU例外管理機能 / 5.2 静的API一覧
