# twai_flg — イベントフラグ待ち（タイムアウト付き）

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = twai_flg(ID flgid, FLGPTN waiptn, MODE wfmode,
                   FLGPTN *p_flgptn, TMO tmout);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `flgid` | `ID` | 対象イベントフラグのID番号 |
| `waiptn` | `FLGPTN` | 待ちビットパターン（0は `E_PAR`） |
| `wfmode` | `MODE` | 待ちモード：`TWF_ORW`（いずれか成立）または `TWF_ANDW`（全ビット成立） |
| `p_flgptn` | `FLGPTN *` | 待ち解除時のフラグ値の格納先 |
| `tmout` | `TMO` | タイムアウト時間（μs）。`TMO_POL`（=0）でポーリング、`TMO_FEVR`（=-1）で永久待ち |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_flgptn` | `FLGPTN` | 待ち条件成立時のフラグビットパターン |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（待ち条件が成立した） |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態／ディスパッチ禁止状態 |
| `E_ID` | `flgid` が不正・範囲外 |
| `E_PAR` | `waiptn` が 0、`wfmode` が不正、または `tmout` が不正 |
| `E_ILUSE` | `TA_WMUL` 属性なしのイベントフラグに既に待ちタスクが存在する |
| `E_TMOUT` | タイムアウト時間内に待ち条件が成立しなかった |
| `E_RLWAI` | 待ち状態が強制解除された（`rel_wai`） |
| `E_RASTER` | タスク終了要求を受け付けている状態で呼び出した |

## 機能
`flgid` で指定したイベントフラグの、`waiptn` と `wfmode` で指定した待ち条件が成立する
まで自タスクを待ち状態に遷移させる。`wai_flg` にタイムアウト機能を加えたサービスコール
である。

`wfmode` の指定：
- `TWF_ORW`（OR待ち）：`waiptn` のいずれかのビットが 1 であれば条件成立
- `TWF_ANDW`（AND待ち）：`waiptn` のすべてのビットが 1 であれば条件成立

待ち条件が既に成立している場合は、待ち状態に入らず即座に `E_OK` を返す。成立していない
場合は待ち状態に遷移し、条件が成立するか `tmout` で指定した時間が経過するまで待つ。
タイムアウトすると `E_TMOUT` を返す。

待ち解除時、フラグの現在値を `*p_flgptn` に格納する。イベントフラグに `TA_CLR` 属性が
設定されている場合は、待ち解除後にすべてのビットをクリアする。

`tmout` に `TMO_POL`（=0）を指定すると待ち状態に入らずポーリング動作となり（`pol_flg`
と等価）、`TMO_FEVR`（=-1）を指定するとタイムアウトを行わない（`wai_flg` と等価）。

待ち解除を複数タスクで行うことを許可しない（`TA_WMUL` 属性なしの）イベントフラグに
対して、既に待ちタスクが存在する状態で呼び出すと `E_ILUSE` を返す。

## 使用例
```c
void monitor_task(intptr_t exinf) {
    FLGPTN ptn;
    /* 最大100ms（100000μs）待つ */
    ER ercd = twai_flg(SYS_FLG, EV_UART_RX | EV_ERROR, TWF_ORW, &ptn, 100000);
    if (ercd == E_OK) {
        if (ptn & EV_ERROR) { handle_error(); }
        if (ptn & EV_UART_RX) { handle_uart(); }
    }
    else if (ercd == E_TMOUT) {
        handle_timeout();
    }
}
```

## 関連
- `wai_flg` — イベントフラグ待ち（永久待ち）
- `pol_flg` — イベントフラグ待ち（ポーリング）
- `set_flg` / `clr_flg` — ビットのセット / クリア
- 仕様書 4.4.2 イベントフラグ
