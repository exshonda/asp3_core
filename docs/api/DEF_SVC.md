# DEF_SVC — 拡張サービスコールの定義（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.8 拡張サービスコール管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
DEF_SVC(fncd, { svcatr, svcrtn });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
DEF_SVC #fncd* { .svcatr &svcrtn }
```

| 記法 | 意味 |
|---|---|
| `#fncd*` | 機能コード・キーパラメータ |
| `.svcatr` | 符号無し整数定数式（属性） |
| `&svcrtn` | 一般整数定数式（ルーチン先頭番地） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `fncd` | `FN` | 機能コード（負値。ターゲット依存範囲） |
| パケット | `svcatr` | `ATR` | 拡張サービスコール属性（`TA_NULL` のみ） |
| パケット | `svcrtn` | `SVCRTN` | 拡張サービスコールルーチンの先頭番地 |

## 属性（svcatr）
| 属性 | 説明 |
|---|---|
| `TA_NULL` | 属性指定なし（唯一の有効値） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `svcatr` が不正 |
| `E_PAR` | `fncd` が範囲外、または `svcrtn` が NULL |
| `E_OBJ` | `fncd` が重複定義 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`fncd` で指定した機能コードに拡張サービスコールルーチン `svcrtn` を定義する。

拡張サービスコールは非タスクコンテキスト（割込みハンドラ等）からタスクコンテキストの処理を
呼び出すための仕組み。`cal_svc(fncd, ...)` で呼ぶと、`svcrtn` がタスクコンテキストで実行される。

ASP3 ではほとんど使われない。割込みハンドラからタスクへの通知には通常 `wup_tsk` / `sig_sem` / `set_flg` を使う。

## 使用例
```c
ER_UINT my_service(intptr_t par1, intptr_t par2, ID cdmtskid) {
    /* タスクコンテキストで実行される */
    return E_OK;
}
```
```
DEF_SVC(TFN_MY_SERVICE, { TA_NULL, my_service });
```

## 関連
- `cal_svc` — 拡張サービスコールの呼出し
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.8 拡張サービスコール管理機能 / 5.2 静的API一覧
