# wai_flg — イベントフラグ待ち

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `flgid` | `ID` | 対象イベントフラグのID番号 |
| `waiptn` | `FLGPTN` | 待ちビットパターン（0は `E_PAR`） |
| `wfmode` | `MODE` | 待ちモード：`TWF_ORW`（いずれか成立）または `TWF_ANDW`（全ビット成立） |
| `p_flgptn` | `FLGPTN *` | 待ち解除時のフラグ値の格納先 |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_flgptn` | `FLGPTN` | 待ち条件成立時のフラグビットパターン |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（待ち条件が成立した） |
| `E_ID` | `flgid` が不正・範囲外 |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_PAR` | `waiptn` が 0、または `wfmode` が不正 |
| `E_NOEXS`〔D〕 | 対象イベントフラグが未登録（動的生成対応カーネル） |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai` / `ter_tsk`） |

## 機能
`flgid` で指定したイベントフラグの、`waiptn` と `wfmode` で指定した待ち条件が成立するまで
自タスクを待ち状態に遷移させる。

`wfmode` の指定：
- `TWF_ORW`（OR待ち）：`waiptn` のいずれかのビットが 1 であれば条件成立
- `TWF_ANDW`（AND待ち）：`waiptn` のすべてのビットが 1 であれば条件成立

待ち条件が既に成立している場合は、待ち状態に入らず即座に `E_OK` を返す。

待ち解除時、フラグの現在値を `*p_flgptn` に格納する。
イベントフラグに `TA_CLR` 属性が設定されている場合は、待ち解除後にすべてのビットをクリアする。

ポーリングは `pol_flg`、タイムアウト付きは `twai_flg` を使う。

## 使用例
```c
#define EV_UART_RX   0x01U
#define EV_TIMER     0x02U
#define EV_ERROR     0x04U

void monitor_task(intptr_t exinf) {
    FLGPTN ptn;
    /* UARTまたはエラーのいずれかを待つ */
    ER ercd = wai_flg(SYS_FLG, EV_UART_RX | EV_ERROR, TWF_ORW, &ptn);
    if (ercd == E_OK) {
        if (ptn & EV_ERROR) { handle_error(); }
        if (ptn & EV_UART_RX) { handle_uart(); }
    }
}
```

## 関連
- `set_flg` — ビットのセット
- `clr_flg` — ビットのクリア
- `pol_flg` — イベントフラグ待ち（ポーリング）
- `twai_flg` — イベントフラグ待ち（タイムアウト付き）
- 仕様書 4.4.2 イベントフラグ
