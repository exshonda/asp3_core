# ref_sem — セマフォの状態参照

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = ref_sem(ID semid, T_RSEM *pk_rsem);
```

`T_RSEM` の定義（`include/kernel.h`）：

```c
typedef struct t_rsem {
    ID      wtskid;     /* セマフォの待ち行列の先頭のタスクのID番号 */
    uint_t  semcnt;     /* セマフォの現在の資源数 */
} T_RSEM;
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `semid` | `ID` | 対象セマフォのID番号 |
| `pk_rsem` | `T_RSEM *` | 状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_rsem->wtskid` | `ID` | 待ち行列の先頭タスクのID番号（待ちタスクがなければ `TSK_NONE`） |
| `pk_rsem->semcnt` | `uint_t` | 現在の資源数 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態 |
| `E_ID` | `semid` が不正・範囲外 |

## 機能
`semid` で指定したセマフォの現在の状態を参照し、`pk_rsem` で指定したパケットに格納する。

- `wtskid`：セマフォの待ち行列の先頭タスクのID番号。待ち行列が空の場合は `TSK_NONE` を
  格納する。
- `semcnt`：セマフォの現在の資源数。

## 使用例
```c
void diag_task(intptr_t exinf) {
    T_RSEM rsem;
    if (ref_sem(RESOURCE_SEM, &rsem) == E_OK) {
        /* rsem.semcnt = 残り資源数, rsem.wtskid = 待ち先頭タスク */
    }
}
```

## 関連
- `wai_sem` / `pol_sem` / `twai_sem` — セマフォ資源の獲得
- `sig_sem` — セマフォ資源の返却
- `ini_sem` — セマフォの再初期化
- 仕様書 4.4.1 セマフォ
