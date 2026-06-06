# ref_dtq — データキュー状態の参照

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = ref_dtq(ID dtqid, T_RDTQ *pk_rdtq);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `dtqid` | `ID` | 対象データキューのID番号 |
| `pk_rdtq` | `T_RDTQ *` | 状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_rdtq->stskid` | `ID` | 送信待ち行列の先頭タスクのID番号（待ちタスクなしのとき `TSK_NONE`） |
| `pk_rdtq->rtskid` | `ID` | 受信待ち行列の先頭タスクのID番号（待ちタスクなしのとき `TSK_NONE`） |
| `pk_rdtq->sdtqcnt` | `uint_t` | データキュー管理領域に格納されているデータの数 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態 |
| `E_ID` | `dtqid` が不正・範囲外 |

## 機能
`dtqid` で指定したデータキューの状態を参照し、`pk_rdtq` で指定したパケットに格納する。

格納される情報は、送信待ち行列の先頭タスクのID番号（`stskid`）、受信待ち行列の先頭タスクの
ID番号（`rtskid`）、およびデータキュー管理領域に格納されているデータの数（`sdtqcnt`）である。
待ち行列にタスクが存在しない場合、対応するID番号には `TSK_NONE` が格納される。

## 使用例
```c
void monitor_task(intptr_t exinf) {
    T_RDTQ rdtq;
    if (ref_dtq(MSG_DTQ, &rdtq) == E_OK) {
        syslog(LOG_NOTICE, "dtq: count=%d stskid=%d rtskid=%d",
               rdtq.sdtqcnt, rdtq.stskid, rdtq.rtskid);
    }
}
```

## 関連
- `snd_dtq` — データキュー送信（ブロッキング）
- `rcv_dtq` — データキュー受信（ブロッキング）
- `ini_dtq` — データキューの再初期化
- `CRE_DTQ` — データキューの生成（静的API）
- 仕様書 4.4.3 データキュー機能
