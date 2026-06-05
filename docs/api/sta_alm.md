# sta_alm — アラーム通知の開始

## 種別
〔TI〕タスクコンテキスト・非タスクコンテキストの両方から呼出し可能

## C言語API
```c
ER ercd = sta_alm(ID almid, RELTIM almtim);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `almid` | `ID` | 対象アラームのID番号 |
| `almtim` | `RELTIM` | 起動時間（マイクロ秒、相対時間）。0 を指定すると即時起動 |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_ID` | `almid` が不正・範囲外 |
| `E_CTX`〔s〕 | CPUロック状態で呼び出した |
| `E_NOEXS`〔D〕 | 対象アラームが未登録（動的生成対応カーネル） |
| `E_PAR` | `almtim` が `TMAX_RELTIM`（4,000,000,000）を超える |

## 機能
`almid` で指定したアラームを、現在時刻から `almtim` マイクロ秒後にアラームハンドラを
1回だけ起動するように設定して動作開始する（ワンショット動作）。

対象アラームが既に動作中の場合は、起動時刻を新しい値に上書き再設定する（前の設定は破棄される）。

アラームハンドラが起動された後、アラームは停止状態に戻る（再起動するには再度 `sta_alm` が必要）。

`almtim` に 0 を指定すると、次のカーネルティックで即座に起動する。

アラームハンドラは非タスクコンテキストで実行される。ハンドラからタスクを起こすには
`wup_tsk`・`sig_sem`・`set_flg`・`psnd_dtq` など〔I〕または〔TI〕のサービスコールを使う。

## 使用例
```c
/* アラームハンドラ（CFGで登録） */
void timeout_handler(intptr_t exinf) {
    set_flg(SYS_FLG, EV_TIMEOUT);
}

void command_task(intptr_t exinf) {
    FLGPTN ptn;

    send_command();
    sta_alm(CMD_TIMEOUT_ALM, 100000U);   /* 100ms タイムアウト設定 */

    wai_flg(SYS_FLG, EV_RESPONSE | EV_TIMEOUT, TWF_ORW, &ptn);
    stp_alm(CMD_TIMEOUT_ALM);            /* タイムアウト前に解除した場合キャンセル */

    if (ptn & EV_TIMEOUT) { handle_timeout(); }
    else { handle_response(); }
}
```

## 関連
- `stp_alm` — アラームの停止
- `ref_alm` — アラーム状態の参照
- `sta_cyc` — 周期通知の開始（繰り返し起動）
- 仕様書 4.6 時間管理機能
