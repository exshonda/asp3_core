# get_nth — 指定した優先順位のタスクIDの参照

## 種別
〔T〕タスクコンテキスト専用（非タスクコンテキストから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = get_nth(PRI tskpri, uint_t nth, ID *p_tskid);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `tskpri` | `PRI` | 対象とするタスク優先度（`TMIN_TPRI`〜`TMAX_TPRI`＝1〜16）。`TPRI_SELF`（=0）で自タスクのベース優先度 |
| `nth` | `uint_t` | 優先順位（0 が先頭。指定優先度のレディキュー内で何番目かを0起算で指定） |
| `p_tskid` | `ID *` | タスクIDを格納する領域へのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_tskid` | `ID` | 指定した優先順位のタスクのID番号。該当タスクがないときは `TSK_NONE`（=0） |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態で呼び出した |
| `E_PAR` | `tskpri` が不正・範囲外（`TPRI_SELF` 以外を指定した場合のみチェック） |

## 機能
`tskpri` で指定した優先度の実行できるタスクのうち、`nth` で指定した優先順位（0が先頭）の
タスクのID番号を読み出し、`p_tskid` が指す領域に格納する。`nth` が0のとき、指定優先度の
レディキューの先頭（次にディスパッチされうるタスク）のID番号を返す。

`nth` が指定優先度の実行できるタスク数以上の場合は、該当するタスクがないため
`TSK_NONE`（=0）を格納する。

`tskpri` に `TPRI_SELF`（=0）を指定すると、自タスクのベース優先度を対象とする。

## 使用例
```c
void scheduler_task(EXINF exinf) {
    ID tskid;
    uint_t i;
    for (i = 0U; ; i++) {
        get_nth(MID_PRIORITY, i, &tskid);
        if (tskid == TSK_NONE) {
            break;
        }
        syslog(LOG_NOTICE, "nth=%d -> task %d", i, tskid);
    }
}
```

## 関連
- `get_lod` — 指定優先度の実行できるタスク数の参照
- `rot_rdq` — レディキューの回転
- 仕様書 4.7 システム状態管理機能
