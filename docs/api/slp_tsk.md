# slp_tsk / tslp_tsk — 起床待ち / タイムアウト付き起床待ち

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = slp_tsk(void);
ER ercd = tslp_tsk(TMO tmout);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `tmout` | `TMO` | タイムアウト時間（マイクロ秒）。`TMO_POL`（=0）でポーリング、`TMO_FEVR`（=-1）で無限待ち |

`slp_tsk` はパラメータなし（`tslp_tsk(TMO_FEVR)` と等価）。

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（`wup_tsk` で起床した） |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai` / `ter_tsk`） |
| `E_TMOUT` | タイムアウトした（`tslp_tsk` のみ。`TMO_POL` 指定時も含む） |
| `E_PAR` | `tmout` が不正（`tslp_tsk` のみ） |

## 機能
自タスクを起床待ち状態（WAITING）に遷移させる。`wup_tsk` によって起床要求が来ると
実行可能状態（READY）に戻り、`E_OK` を返す。

`wup_tsk` による起床要求がキューイングされている場合は、待ち状態に入らず即座に `E_OK` を返す
（キューイング数を1減じる）。

`tslp_tsk` は `tmout` で指定した時間（マイクロ秒）が経過するとタイムアウトして `E_TMOUT` を返す。
`TMO_POL`（0）を指定すると起床要求がなければ即座に `E_TMOUT` を返す（ポーリング）。
`TMO_FEVR`（-1）を指定すると `slp_tsk` と同じく無限待ちになる。

`dly_tsk` と異なり、`wup_tsk` による起床で解除される。

## 使用例
```c
/* 無限待ち */
void worker_task(intptr_t exinf) {
    while (true) {
        slp_tsk();          /* wup_tsk で起床するまで待つ */
        process_request();
    }
}

/* タイムアウト付き（500ms） */
void polling_task(intptr_t exinf) {
    while (true) {
        ER ercd = tslp_tsk(500000U);
        if (ercd == E_OK) {
            handle_event();
        } else {    /* E_TMOUT */
            do_periodic_check();
        }
    }
}
```

## 関連
- `wup_tsk` — タスクの起床（`slp_tsk` の待ちを解除する）
- `can_wup` — キューイングされた起床要求のキャンセル
- `dly_tsk` — 時間経過待ち（起床要求では解除されない）
- 仕様書 4.2 タスク付属同期機能
