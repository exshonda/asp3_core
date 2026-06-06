# get_tim — システム時刻の参照

## 種別
〔T〕タスクコンテキスト専用

## C言語API
```c
ER ercd = get_tim(SYSTIM *p_systim);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `p_systim` | `SYSTIM *` | システム時刻の格納先（`SYSTIM` = `uint64_t`） |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_systim` | `SYSTIM` | 現在のシステム時刻（マイクロ秒） |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストまたはCPUロック状態で呼び出した |

## 機能
カーネルが管理するシステム時刻を `*p_systim` に格納して返す。

`SYSTIM` は `uint64_t` であり、単位は**マイクロ秒**（ASP3でμITRON4.0のミリ秒から変更。
`doc/asp_spec.txt` (4-7)）。カーネル起動からの経過時間に `set_tim` による設定値を
加味した値を表す。64ビットのため、オーバーフローまで約58万年であり実用上考慮不要。

ASP3はティックレスカーネルであり、システム時刻は高分解能タイマ（`fch_hrt` と同じ
タイムベース）から参照のたびに計算される。分解能は高分解能タイマに準じる（通常 1μs）。

`set_tim` でシステム時刻を設定しても、相対時間で動作するタイムイベント
（`dly_tsk`・周期通知・アラーム通知）の発生タイミングには影響しない。

## 使用例
```c
void benchmark_task(intptr_t exinf) {
    SYSTIM t0, t1;

    get_tim(&t0);
    do_heavy_work();
    get_tim(&t1);

    syslog(LOG_INFO, "elapsed: %llu us", (unsigned long long)(t1 - t0));
}
```

## 関連
- [set_tim](set_tim.md) — システム時刻の設定
- [adj_tim](adj_tim.md) — システム時刻の調整
- [fch_hrt](fch_hrt.md) — 高分解能タイマの参照
- 仕様書 4.6 時間管理機能
