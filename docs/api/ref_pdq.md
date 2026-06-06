# ref_pdq — 優先度データキュー状態の参照

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = ref_pdq(ID pdqid, T_RPDQ *pk_rpdq);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `pdqid` | `ID` | 対象優先度データキューのID番号 |
| `pk_rpdq` | `T_RPDQ *` | 状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_rpdq->stskid` | `ID` | 送信待ち行列の先頭タスクのID番号（待ちタスクなしのとき `TSK_NONE`） |
| `pk_rpdq->rtskid` | `ID` | 受信待ち行列の先頭タスクのID番号（待ちタスクなしのとき `TSK_NONE`） |
| `pk_rpdq->spdqcnt` | `uint_t` | 優先度データキュー管理領域に格納されているデータの数 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態 |
| `E_ID` | `pdqid` が不正・範囲外 |

## 機能
`pdqid` で指定した優先度データキューの状態を参照し、`pk_rpdq` で指定したパケットに格納する。

格納される情報は、送信待ち行列の先頭タスクのID番号（`stskid`）、受信待ち行列の先頭タスクの
ID番号（`rtskid`）、および優先度データキュー管理領域に格納されているデータの数（`spdqcnt`）
である。待ち行列にタスクが存在しない場合、対応するID番号には `TSK_NONE` が格納される。

## 使用例
```c
void monitor_task(intptr_t exinf) {
    T_RPDQ rpdq;
    if (ref_pdq(JOB_PDQ, &rpdq) == E_OK) {
        syslog(LOG_NOTICE, "pdq: count=%d stskid=%d rtskid=%d",
               rpdq.spdqcnt, rpdq.stskid, rpdq.rtskid);
    }
}
```

## 関連
- `snd_pdq` — 優先度データキュー送信（ブロッキング）
- `rcv_pdq` — 優先度データキュー受信（ブロッキング）
- `ini_pdq` — 優先度データキューの再初期化
- `CRE_PDQ` — 優先度データキューの生成（静的API）
- 仕様書 4.4.4 優先度データキュー機能
