# ref_mpf — 固定長メモリプールの状態参照

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`／CPUロック状態でも `E_CTX`）

## C言語API
```c
T_RMPF rmpf;
ER ercd = ref_mpf(ID mpfid, T_RMPF *pk_rmpf);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `mpfid` | `ID` | 対象固定長メモリプールのID番号 |
| `pk_rmpf` | `T_RMPF *` | 状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_rmpf->wtskid` | `ID` | メモリプールの待ち行列の先頭タスクのID番号（待ちタスクがなければ `TSK_NONE`） |
| `pk_rmpf->fblkcnt` | `uint_t` | 割付け可能な空き固定長メモリブロックの数 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態（`CHECK_TSKCTX_UNL`） |
| `E_ID` | `mpfid` が不正・範囲外 |

## 機能
`mpfid` で指定した固定長メモリプールの状態を参照し、`pk_rmpf` で指定したパケットに格納する。

`wtskid` にはメモリプールの待ち行列の先頭タスク（次にメモリブロックを獲得するタスク）の
ID番号が返される。待ちタスクがない場合は `TSK_NONE` が返される。`fblkcnt` には、その時点で
割付け可能な空き固定長メモリブロックの数が返される。

## 使用例
```c
void monitor(intptr_t exinf) {
    T_RMPF rmpf;
    if (ref_mpf(WORK_MPF, &rmpf) == E_OK) {
        syslog(LOG_NOTICE, "free blocks=%d, waiting=%d",
               rmpf.fblkcnt, rmpf.wtskid);
    }
}
```

## 関連
- `get_mpf` / `rel_mpf` — 固定長メモリブロックの獲得 / 返却
- `CRE_MPF` — 固定長メモリプールの生成（静的API）
- 仕様書 4.5.1 固定長メモリプール機能
