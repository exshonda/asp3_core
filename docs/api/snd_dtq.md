# snd_dtq — データキュー送信（ブロッキング）

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = snd_dtq(ID dtqid, intptr_t data);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `dtqid` | `ID` | 対象データキューのID番号 |
| `data` | `intptr_t` | 送信データ（ポインタサイズの整数またはポインタ） |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（データを送信した） |
| `E_ID` | `dtqid` が不正・範囲外 |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_NOEXS`〔D〕 | 対象データキューが未登録（動的生成対応カーネル） |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai` / `ter_tsk`） |

## 機能
`dtqid` で指定したデータキューに `data` を1件送信する。

受信待ちタスクが存在する場合は、先頭タスクに直接 `data` を渡して待ち解除する。

キューに空きがある場合は `data` をキュー末尾に追加して即座にリターンする。

キューが満杯の場合は、空きができるまで自タスクを送信待ち状態に遷移させる。

ポーリング送信は `psnd_dtq`、タイムアウト付きは `tsnd_dtq`、非タスクからの強制送信は `fsnd_dtq`
（満杯時でも待たずにキューイング上書き）を使う。

## 使用例
```c
typedef struct { uint8_t cmd; uint16_t value; } Message;

void producer_task(intptr_t exinf) {
    Message msg = { .cmd = CMD_UPDATE, .value = 42 };
    snd_dtq(MSG_DTQ, (intptr_t)&msg);   /* 満杯なら待つ */
}

void consumer_task(intptr_t exinf) {
    intptr_t data;
    while (true) {
        rcv_dtq(MSG_DTQ, &data);
        Message *msg = (Message *)data;
        process(msg);
    }
}
```

## 関連
- `rcv_dtq` — データキュー受信（ブロッキング）
- `psnd_dtq` — 送信（ポーリング、非タスクからも可）
- `tsnd_dtq` — 送信（タイムアウト付き）
- `fsnd_dtq` — 強制送信（非タスクからも可、満杯時上書き）
- 仕様書 4.4.3 データキュー
