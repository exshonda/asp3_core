# set_flg — イベントフラグのビットセット

## 種別
〔TI〕タスクコンテキスト・非タスクコンテキストの両方から呼出し可能

## C言語API
```c
ER ercd = set_flg(ID flgid, FLGPTN setptn);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `flgid` | `ID` | 対象イベントフラグのID番号 |
| `setptn` | `FLGPTN` | セットするビットパターン（OR演算でセット） |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_ID` | `flgid` が不正・範囲外 |
| `E_CTX`〔s〕 | CPUロック状態で呼び出した |
| `E_NOEXS`〔D〕 | 対象イベントフラグが未登録（動的生成対応カーネル） |

## 機能
`flgid` で指定したイベントフラグのビットパターンに、`setptn` で指定したビットを OR 演算で
セットする（既にセットされているビットはそのまま）。

ビットをセットした後、待ち条件（`wai_flg` / `pol_flg` / `twai_flg` の `waiptn` と `wfmode`）
を満たしたタスクをすべて待ち解除する（イベントフラグはブロードキャスト型）。
ただしイベントフラグに `TA_CLR` 属性が設定されている場合は、先頭タスク1件のみ解除し、
フラグをクリアする。

`setptn` に 0 を指定するとフラグは変化しないが、エラーにもならない。

## 使用例
```c
#define EV_DATA_READY   0x01U
#define EV_ERROR        0x02U

/* 割込みハンドラからイベント通知 */
void dma_complete_isr(void) {
    set_flg(SYS_FLG, EV_DATA_READY);
}

void process_task(intptr_t exinf) {
    FLGPTN ptn;
    wai_flg(SYS_FLG, EV_DATA_READY, TWF_ORW, &ptn);
    process_data();
}
```

## 関連
- `clr_flg` — ビットのクリア
- `wai_flg` — イベントフラグ待ち（ブロッキング）
- `pol_flg` — イベントフラグ待ち（ポーリング）
- `twai_flg` — イベントフラグ待ち（タイムアウト付き）
- 仕様書 4.4.2 イベントフラグ
