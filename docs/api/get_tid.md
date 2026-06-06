# get_tid — 実行状態のタスクIDの参照

## 種別
〔TI〕タスクコンテキスト・非タスクコンテキストの両方から呼出し可能

## C言語API
```c
ER ercd = get_tid(ID *p_tskid);
ER ercd = iget_tid(ID *p_tskid);    /* 非タスクコンテキスト用エイリアス */
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `p_tskid` | `ID *` | タスクIDを格納する領域へのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_tskid` | `ID` | 実行状態のタスクのID番号。該当タスクがないときは `TSK_NONE`（=0） |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | CPUロック状態で呼び出した |

## 機能
実行状態のタスク（自タスク）のID番号を読み出し、`p_tskid` が指す領域に格納する。

非タスクコンテキストから呼び出した場合は、割り込まれたタスク（実行状態のタスク）の
ID番号を返す。実行状態のタスクが存在しない場合（アイドル中など）は `TSK_NONE`（=0）を
格納する。

`iget_tid` は非タスクコンテキストから呼び出す場合のエイリアスである（実体は `get_tid`）。

## 使用例
```c
void some_handler(EXINF exinf) {
    ID tskid;
    iget_tid(&tskid);
    if (tskid != TSK_NONE) {
        syslog(LOG_NOTICE, "interrupted task = %d", tskid);
    }
}
```

## 関連
- `ref_tsk` — タスク状態の参照
- `sns_ctx` — コンテキストの参照
- 仕様書 4.7 システム状態管理機能
