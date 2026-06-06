# tslp_tsk — タイムアウト付き起床待ち

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = tslp_tsk(TMO tmout);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `tmout` | `TMO` | タイムアウト時間（マイクロ秒）。`TMO_POL`（=0）でポーリング、`TMO_FEVR`（=-1）で無限待ち |

`tmout` には `0`（`TMO_POL`）以上 `TMAX_RELTIM` 以下の値、または `TMO_FEVR`（-1）を指定できる。
`tslp_tsk(TMO_FEVR)` は `slp_tsk()` と等価。

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（`wup_tsk` で起床した） |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態（`CHECK_DISPATCH`） |
| `E_PAR` | `tmout` が不正（`TMAX_RELTIM` 超かつ `TMO_FEVR` でない。`VALID_TMOUT`） |
| `E_TMOUT` | タイムアウトした（`TMO_POL` 指定で起床要求がなかった場合を含む） |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai`） |
| `E_RASTER` | 自タスクに終了要求がある状態で呼び出した（`ras_ter` 後） |

## 機能
自タスクを起床待ち状態（WAITING）に遷移させ、`wup_tsk` による起床要求が来ると
実行可能状態（READY）に戻して `E_OK` を返す。

`wup_tsk` による起床要求がキューイングされている場合は、待ち状態に入らず即座に `E_OK` を返す
（キューイング数を1減じる）。

`tmout` で指定した時間（マイクロ秒）が経過すると、待ち状態を解除して `E_TMOUT` を返す。
`TMO_POL`（0）を指定した場合は、起床要求がキューイングされていなければ待ち状態に入らず
即座に `E_TMOUT` を返す（ポーリング）。`TMO_FEVR`（-1）を指定した場合は `slp_tsk` と同じく
無限待ちとなる。

`dly_tsk` と異なり、`wup_tsk` による起床で待ちが解除される。

## 使用例
```c
/* タイムアウト付き（500ms） */
void polling_task(intptr_t exinf) {
    while (true) {
        ER ercd = tslp_tsk(500000U);
        if (ercd == E_OK) {
            handle_event();         /* wup_tsk で起床した */
        } else {    /* E_TMOUT */
            do_periodic_check();    /* 500ms経過した */
        }
    }
}
```

## 関連
- `slp_tsk` — 起床待ち（`tslp_tsk(TMO_FEVR)` と等価）
- `wup_tsk` — タスクの起床（`tslp_tsk` の待ちを解除する）
- `can_wup` — キューイングされた起床要求のキャンセル
- `dly_tsk` — 時間経過待ち（起床要求では解除されない）
- 仕様書 4.2 タスク付属同期機能
