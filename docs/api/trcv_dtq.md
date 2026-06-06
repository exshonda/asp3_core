# trcv_dtq — データキュー受信（タイムアウト付き）

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = trcv_dtq(ID dtqid, intptr_t *p_data, TMO tmout);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `dtqid` | `ID` | 対象データキューのID番号 |
| `p_data` | `intptr_t *` | 受信データの格納先 |
| `tmout` | `TMO` | タイムアウト時間（μs）。`TMO_POL`＝ポーリング、`TMO_FEVR`＝永久待ち |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_data` | `intptr_t` | 受信したデータ（`E_OK` の場合のみ有効） |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（データを受信した） |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_ID` | `dtqid` が不正・範囲外 |
| `E_PAR` | `tmout` が不正（`TMO_POL` / `TMO_FEVR` 以外の負値など） |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai`） |
| `E_RASTER` | 待ち状態の間に自タスクへの終了要求（`ras_ter`）があった |
| `E_TMOUT` | タイムアウト時間が経過してもデータを受信できなかった |

## 機能
`dtqid` で指定したデータキューから先頭の1件を取り出し、`*p_data` に格納する。
`rcv_dtq` のタイムアウト付き版。

キューにデータが存在する場合は即座にリターンする。データを取り出したことで送信待ちタスクが
いる場合は、そのタスクが送信したデータをキューに追加してから待ち解除する。

キューが空の場合は、データが送信されるまで自タスクを受信待ち状態に遷移させる。ただし、
`tmout` で指定した時間が経過してもデータが送信されなかった場合は、待ちを解除して `E_TMOUT`
を返す。`tmout` に `TMO_POL`（＝0）を指定した場合は待たずに `E_TMOUT` を返し、
`TMO_FEVR`（＝-1）を指定した場合は `rcv_dtq` と同じく永久に待つ。

## 使用例
```c
void consumer_task(intptr_t exinf) {
    intptr_t data;
    while (true) {
        ER ercd = trcv_dtq(MSG_DTQ, &data, 5000);   /* 最大5ms待つ */
        if (ercd == E_OK) {
            process((Message *)data);
        } else if (ercd == E_TMOUT) {
            check_watchdog();
        }
    }
}
```

## 関連
- `rcv_dtq` — データキュー受信（ブロッキング）
- `prcv_dtq` — 受信（ポーリング）
- `tsnd_dtq` — 送信（タイムアウト付き）
- 仕様書 4.4.3 データキュー機能
