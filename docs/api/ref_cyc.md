# ref_cyc — 周期通知の状態参照

## 種別
〔T〕タスクコンテキスト専用（非タスクコンテキストから呼び出すと `E_CTX`）

## C言語API
```c
ER ercd = ref_cyc(ID cycid, T_RCYC *pk_rcyc);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `cycid` | `ID` | 対象周期通知のID番号 |
| `pk_rcyc` | `T_RCYC *` | 周期通知の現在状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_rcyc->cycstat` | `STAT` | 周期通知の動作状態（`TCYC_STP`＝停止中／`TCYC_STA`＝動作中） |
| `pk_rcyc->lefttim` | `RELTIM` | 次回通知時刻までの相対時間（マイクロ秒）。動作中のときのみ有効 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態で呼び出した |
| `E_ID` | `cycid` が不正・範囲外 |

## 機能
`cycid` で指定した周期通知の現在状態を参照し、`*pk_rcyc` に格納して返す。

`cycstat` には周期通知の動作状態が格納される。
- `TCYC_STA`（動作中）：このとき `lefttim` に、次回の通知が行われるまでの相対時間（マイクロ秒）が格納される。
- `TCYC_STP`（停止中）：このとき `lefttim` の値は無効である。

`T_RCYC` 構造体は次のメンバを持つ。

```c
typedef struct t_rcyc {
    STAT    cycstat;    /* 周期通知の動作状態 */
    RELTIM  lefttim;    /* 次回通知時刻までの相対時間 */
} T_RCYC;
```

## 使用例
```c
void monitor_task(intptr_t exinf) {
    T_RCYC rcyc;

    if (ref_cyc(SAMPLE_CYC, &rcyc) == E_OK) {
        if (rcyc.cycstat == TCYC_STA) {
            syslog(LOG_INFO, "next sample in %u us", (unsigned int)rcyc.lefttim);
        }
        else {
            syslog(LOG_INFO, "cyclic notification stopped");
        }
    }
}
```

## 関連
- `sta_cyc` — 周期通知の動作開始
- `stp_cyc` — 周期通知の動作停止
- 仕様書 4.6.3 周期通知
