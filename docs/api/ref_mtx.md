# ref_mtx — ミューテックスの状態参照

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`／CPUロック状態でも `E_CTX`）

## C言語API
```c
T_RMTX rmtx;
ER ercd = ref_mtx(ID mtxid, T_RMTX *pk_rmtx);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `mtxid` | `ID` | 対象ミューテックスのID番号 |
| `pk_rmtx` | `T_RMTX *` | 状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_rmtx->htskid` | `ID` | ミューテックスをロックしているタスクのID番号（誰もロックしていなければ `TSK_NONE`） |
| `pk_rmtx->wtskid` | `ID` | ミューテックスの待ち行列の先頭タスクのID番号（待ちタスクがなければ `TSK_NONE`） |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態（`CHECK_TSKCTX_UNL`） |
| `E_ID` | `mtxid` が不正・範囲外 |

## 機能
`mtxid` で指定したミューテックスの状態を参照し、`pk_rmtx` で指定したパケットに格納する。

`htskid` にはミューテックスを現在ロックしているタスクのID番号が返される。どのタスクも
ロックしていない場合は `TSK_NONE` が返される。`wtskid` にはミューテックスの待ち行列の
先頭タスク（次にロックを獲得するタスク）のID番号が返される。待ちタスクがない場合は
`TSK_NONE` が返される。

## 使用例
```c
void monitor(intptr_t exinf) {
    T_RMTX rmtx;
    if (ref_mtx(SHARED_MTX, &rmtx) == E_OK) {
        syslog(LOG_NOTICE, "holder=%d, waiting=%d", rmtx.htskid, rmtx.wtskid);
    }
}
```

## 関連
- `loc_mtx` / `unl_mtx` — ミューテックスのロック / 解除
- `CRE_MTX` — ミューテックスの生成（静的API）
- 仕様書 4.4.5 ミューテックス機能
