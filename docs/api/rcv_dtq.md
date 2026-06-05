# rcv_dtq — データキュー受信（ブロッキング）

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = rcv_dtq(ID dtqid, intptr_t *p_data);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `dtqid` | `ID` | 対象データキューのID番号 |
| `p_data` | `intptr_t *` | 受信データの格納先 |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_data` | `intptr_t` | 受信したデータ |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（データを受信した） |
| `E_ID` | `dtqid` が不正・範囲外 |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_NOEXS`〔D〕 | 対象データキューが未登録（動的生成対応カーネル） |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai` / `ter_tsk`） |

## 機能
`dtqid` で指定したデータキューから先頭の1件を取り出し、`*p_data` に格納する。

キューにデータが存在する場合は即座にリターンする。キューが空の場合は、データが送信されるまで
自タスクを受信待ち状態に遷移させる。

データを取り出したことで送信待ちタスクがいる場合は、そのタスクが送信したデータをキューに
追加してから待ち解除する。

ポーリング受信は `prcv_dtq`、タイムアウト付きは `trcv_dtq` を使う。

## 使用例
```c
typedef struct { uint8_t cmd; uint16_t value; } Message;

void consumer_task(intptr_t exinf) {
    intptr_t data;
    while (true) {
        ER ercd = rcv_dtq(MSG_DTQ, &data);
        if (ercd == E_OK) {
            Message *msg = (Message *)data;
            process(msg);
        }
    }
}
```

## 関連
- `snd_dtq` — データキュー送信（ブロッキング）
- `prcv_dtq` — 受信（ポーリング）
- `trcv_dtq` — 受信（タイムアウト付き）
- 仕様書 4.4.3 データキュー
