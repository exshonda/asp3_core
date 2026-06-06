# trcv_pdq — 優先度データキュー受信（タイムアウト付き）

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = trcv_pdq(ID pdqid, intptr_t *p_data, PRI *p_datapri, TMO tmout);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `pdqid` | `ID` | 対象優先度データキューのID番号 |
| `p_data` | `intptr_t *` | 受信データの格納先 |
| `p_datapri` | `PRI *` | 受信データの優先度の格納先 |
| `tmout` | `TMO` | タイムアウト時間（μs）。`TMO_POL`＝ポーリング、`TMO_FEVR`＝永久待ち |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_data` | `intptr_t` | 受信したデータ（`E_OK` の場合のみ有効） |
| `*p_datapri` | `PRI` | 受信したデータの優先度（`E_OK` の場合のみ有効） |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（データを受信した） |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_ID` | `pdqid` が不正・範囲外 |
| `E_PAR` | `tmout` が不正（`TMO_POL` / `TMO_FEVR` 以外の負値など） |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai`） |
| `E_RASTER` | 待ち状態の間に自タスクへの終了要求（`ras_ter`）があった |
| `E_TMOUT` | タイムアウト時間が経過してもデータを受信できなかった |

## 機能
`pdqid` で指定した優先度データキューから、最も優先度の高いデータを1件取り出し、`*p_data` に
そのデータを、`*p_datapri` にそのデータの優先度を格納する。`rcv_pdq` のタイムアウト付き版。
同一優先度のデータが複数ある場合は、先に送信されたデータが取り出される（FIFO順）。

キューにデータが存在する場合は即座にリターンする。データを取り出したことで送信待ちタスクが
いる場合は、そのタスクが送信したデータをキューに追加してから待ち解除する。

キューが空の場合は、データが送信されるまで自タスクを受信待ち状態に遷移させる。ただし、`tmout`
で指定した時間が経過してもデータが送信されなかった場合は、待ちを解除して `E_TMOUT` を返す。
`tmout` に `TMO_POL`（＝0）を指定した場合は待たずに `E_TMOUT` を返し、`TMO_FEVR`（＝-1）
を指定した場合は `rcv_pdq` と同じく永久に待つ。

## 使用例
```c
void consumer_task(intptr_t exinf) {
    intptr_t data;
    PRI datapri;
    ER ercd = trcv_pdq(JOB_PDQ, &data, &datapri, 5000);   /* 最大5ms待つ */
    if (ercd == E_OK) {
        process_job((Job *)data, datapri);
    } else if (ercd == E_TMOUT) {
        check_watchdog();
    }
}
```

## 関連
- `rcv_pdq` — 優先度データキュー受信（ブロッキング）
- `prcv_pdq` — 受信（ポーリング）
- `tsnd_pdq` — 送信（タイムアウト付き）
- 仕様書 4.4.4 優先度データキュー機能
