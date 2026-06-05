# clr_flg — イベントフラグのビットクリア

## 種別
〔TI〕タスクコンテキスト・非タスクコンテキストの両方から呼出し可能

## C言語API
```c
ER ercd = clr_flg(ID flgid, FLGPTN clrptn);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `flgid` | `ID` | 対象イベントフラグのID番号 |
| `clrptn` | `FLGPTN` | クリアパターン。**0のビット**が対象フラグのビットをクリアする（AND マスク） |

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
`flgid` で指定したイベントフラグのビットパターンに、`clrptn` を AND 演算する。
`clrptn` の **0 のビット**に対応するフラグビットがクリアされる（1 のビットはそのまま）。

全ビットをクリアするには `clrptn = 0x0U`、何もしないには `clrptn = ~0x0U`（全ビット1）を指定する。

`clr_flg` はフラグをクリアするだけで、待ちタスクを解除しない。
また `set_flg` とは逆に `clrptn` に 0 を持つビットの位置をクリアするため、引数の意味に注意すること。

## 使用例
```c
#define EV_DATA_READY   0x01U
#define EV_ERROR        0x02U

void after_processing(void) {
    /* EV_DATA_READY ビットだけをクリアし、EV_ERROR は残す */
    clr_flg(SYS_FLG, ~EV_DATA_READY);
}

void reset_all_events(void) {
    clr_flg(SYS_FLG, 0x0U);    /* 全ビットクリア */
}
```

## 関連
- `set_flg` — ビットのセット
- `wai_flg` — イベントフラグ待ち
- 仕様書 4.4.2 イベントフラグ
