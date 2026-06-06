# ref_alm — アラーム通知の状態参照

## 種別
〔T〕タスクコンテキスト専用（非タスクコンテキストから呼び出すと `E_CTX`）

## C言語API
```c
ER ercd = ref_alm(ID almid, T_RALM *pk_ralm);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `almid` | `ID` | 対象アラームのID番号 |
| `pk_ralm` | `T_RALM *` | アラーム通知の現在状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_ralm->almstat` | `STAT` | アラーム通知の動作状態（`TALM_STP`＝停止中／`TALM_STA`＝動作中） |
| `pk_ralm->lefttim` | `RELTIM` | 通知時刻までの相対時間（マイクロ秒）。動作中のときのみ有効 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態で呼び出した |
| `E_ID` | `almid` が不正・範囲外 |

## 機能
`almid` で指定したアラーム通知の現在状態を参照し、`*pk_ralm` に格納して返す。

`almstat` にはアラーム通知の動作状態が格納される。
- `TALM_STA`（動作中）：このとき `lefttim` に、アラームハンドラが起動されるまでの相対時間（マイクロ秒）が格納される。
- `TALM_STP`（停止中）：このとき `lefttim` の値は無効である。

`T_RALM` 構造体は次のメンバを持つ。

```c
typedef struct t_ralm {
    STAT    almstat;    /* アラーム通知の動作状態 */
    RELTIM  lefttim;    /* 通知時刻までの相対時間 */
} T_RALM;
```

## 使用例
```c
void monitor_task(intptr_t exinf) {
    T_RALM ralm;

    if (ref_alm(CMD_TIMEOUT_ALM, &ralm) == E_OK) {
        if (ralm.almstat == TALM_STA) {
            syslog(LOG_INFO, "alarm fires in %u us", (unsigned int)ralm.lefttim);
        }
        else {
            syslog(LOG_INFO, "alarm stopped");
        }
    }
}
```

## 関連
- `sta_alm` — アラーム通知の開始
- `stp_alm` — アラーム通知の動作停止
- 仕様書 4.6.4 アラーム通知
