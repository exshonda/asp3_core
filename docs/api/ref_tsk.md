# ref_tsk — タスクの状態参照

## 種別
〔T〕タスクコンテキスト専用（非タスクコンテキストから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = ref_tsk(ID tskid, T_RTSK *pk_rtsk);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `tskid` | `ID` | 対象タスクのID番号。`TSK_SELF`（=0）で自タスク |
| `pk_rtsk` | `T_RTSK *` | タスク状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*pk_rtsk` | `T_RTSK` | 対象タスクの状態（下記） |

`T_RTSK` のメンバ：

| メンバ | 型 | 説明 |
|---|---|---|
| `tskstat` | `STAT` | タスク状態（`TTS_RUN`／`TTS_RDY`／`TTS_WAI`／`TTS_SUS`／`TTS_WAS`／`TTS_DMT`） |
| `tskpri` | `PRI` | 現在優先度（休止状態では未定義） |
| `tskbpri` | `PRI` | ベース優先度（休止状態では未定義） |
| `tskwait` | `STAT` | 待ち要因（`TTW_SLP`／`TTW_DLY`／`TTW_SEM`／`TTW_FLG`／`TTW_SDTQ`／`TTW_RDTQ`／`TTW_SPDQ`／`TTW_RPDQ`／`TTW_MTX`／`TTW_MPF`。待ち状態のときのみ有効） |
| `wobjid` | `ID` | 待ち対象オブジェクトのID（待ち要因が同期・通信オブジェクトのときのみ有効） |
| `lefttmo` | `TMO` | タイムアウトするまでの残り時間。無限待ちのときは `TMO_FEVR` |
| `actcnt` | `uint_t` | 起動要求キューイング数（0または1） |
| `wupcnt` | `uint_t` | 起床要求キューイング数（0または1） |
| `raster` | `bool_t` | タスク終了要求フラグ |
| `dister` | `bool_t` | タスク終了禁止状態 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態で呼び出した |
| `E_ID` | `tskid` が不正・範囲外（`TSK_SELF` 以外を指定した場合のみチェック） |

## 機能
`tskid` で指定したタスクの状態を読み出し、`pk_rtsk` が指すパケットに格納する。

対象タスクが休止状態（DORMANT）の場合は、`tskstat` に `TTS_DMT` が格納され、
`tskpri`・`tskbpri`・`tskwait`・`wobjid`・`lefttmo`・`wupcnt`・`raster`・`dister` は
有効な値を返さない（`actcnt` は格納される）。

対象タスクが待ち状態でない場合、`tskwait`・`wobjid`・`lefttmo` は有効な値を返さない。
`tskwait` が待ち要因のオブジェクト（セマフォ等）でない（`TTW_SLP`／`TTW_DLY`）の場合、
`wobjid` は有効な値を返さない。

`TSK_SELF`（=0）を指定すると自タスクを対象とする。

## 使用例
```c
void monitor_task(EXINF exinf) {
    T_RTSK rtsk;
    if (ref_tsk(WORKER_TASK, &rtsk) == E_OK) {
        if (rtsk.tskstat == TTS_WAI && rtsk.tskwait == TTW_SEM) {
            syslog(LOG_NOTICE, "waiting for sem %d", rtsk.wobjid);
        }
    }
}
```

## 関連
- `get_tst` — タスク状態の参照（簡易版。状態のみ取得）
- `get_pri` — タスク優先度の参照
- 仕様書 4.1 タスク管理機能
