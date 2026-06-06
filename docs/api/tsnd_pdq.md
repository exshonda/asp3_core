# tsnd_pdq — 優先度データキュー送信（タイムアウト付き）

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = tsnd_pdq(ID pdqid, intptr_t data, PRI datapri, TMO tmout);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `pdqid` | `ID` | 対象優先度データキューのID番号 |
| `data` | `intptr_t` | 送信データ（ポインタサイズの整数またはポインタ） |
| `datapri` | `PRI` | 送信データの優先度（`TMIN_DPRI`〜生成時の `maxdpri`、値が小さいほど高優先度） |
| `tmout` | `TMO` | タイムアウト時間（μs）。`TMO_POL`＝ポーリング、`TMO_FEVR`＝永久待ち |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（データを送信した） |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_ID` | `pdqid` が不正・範囲外 |
| `E_PAR` | `tmout` が不正、または `datapri` が範囲外（`TMIN_DPRI` 未満または `maxdpri` 超過） |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai`） |
| `E_RASTER` | 待ち状態の間に自タスクへの終了要求（`ras_ter`）があった |
| `E_TMOUT` | タイムアウト時間が経過しても送信できなかった |

## 機能
`pdqid` で指定した優先度データキューに、データ優先度 `datapri` を付けて `data` を1件送信する。
`snd_pdq` のタイムアウト付き版。

受信待ちタスクが存在する場合は、先頭タスクに直接 `data` と `datapri` を渡して待ち解除する。

キューに空きがある場合は、`data` を `datapri` の順位に従ってキューに挿入して即座にリターンする。
同一優先度のデータの中では、先に送信されたデータが先に取り出される（FIFO順）。

キューが満杯の場合は、空きができるまで自タスクを送信待ち状態に遷移させる。ただし、`tmout`
で指定した時間が経過しても空きができなかった場合は、待ちを解除して `E_TMOUT` を返す。
`tmout` に `TMO_POL`（＝0）を指定した場合は待たずに `E_TMOUT` を返し、`TMO_FEVR`（＝-1）
を指定した場合は `snd_pdq` と同じく永久に待つ。

## 使用例
```c
void producer_task(intptr_t exinf) {
    intptr_t data = make_data();
    ER ercd = tsnd_pdq(JOB_PDQ, data, 1, 2000);   /* 優先度1で最大2ms待つ */
    if (ercd == E_TMOUT) {
        handle_full();
    }
}
```

## 関連
- `snd_pdq` — 優先度データキュー送信（ブロッキング）
- `psnd_pdq` — 送信（ポーリング、非タスクからも可）
- `trcv_pdq` — 受信（タイムアウト付き）
- 仕様書 4.4.4 優先度データキュー機能
