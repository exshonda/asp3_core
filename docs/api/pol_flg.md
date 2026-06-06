# pol_flg — イベントフラグ待ち（ポーリング）

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = pol_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `flgid` | `ID` | 対象イベントフラグのID番号 |
| `waiptn` | `FLGPTN` | 待ちビットパターン（0は `E_PAR`） |
| `wfmode` | `MODE` | 待ちモード：`TWF_ORW`（いずれか成立）または `TWF_ANDW`（全ビット成立） |
| `p_flgptn` | `FLGPTN *` | 条件成立時のフラグ値の格納先 |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_flgptn` | `FLGPTN` | 待ち条件成立時のフラグビットパターン |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（待ち条件が成立した） |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態 |
| `E_ID` | `flgid` が不正・範囲外 |
| `E_PAR` | `waiptn` が 0、または `wfmode` が不正 |
| `E_ILUSE` | `TA_WMUL` 属性なしのイベントフラグに既に待ちタスクが存在する |
| `E_TMOUT` | ポーリング失敗（待ち条件が成立していなかった） |

## 機能
`flgid` で指定したイベントフラグの、`waiptn` と `wfmode` で指定した待ち条件が成立して
いるかを調べる（ポーリング）。`wai_flg` と異なり、条件が成立していない場合でも待ち状態
には遷移しない。

`wfmode` の指定：
- `TWF_ORW`（OR待ち）：`waiptn` のいずれかのビットが 1 であれば条件成立
- `TWF_ANDW`（AND待ち）：`waiptn` のすべてのビットが 1 であれば条件成立

待ち条件が成立している場合は、フラグの現在値を `*p_flgptn` に格納して `E_OK` を返す。
イベントフラグに `TA_CLR` 属性が設定されている場合は、条件成立後にすべてのビットを
クリアする。条件が成立していない場合は、待ち状態に入らず即座に `E_TMOUT` を返す。

待ち解除を複数タスクで行うことを許可しない（`TA_WMUL` 属性なしの）イベントフラグに
対して、既に待ちタスクが存在する状態で呼び出すと `E_ILUSE` を返す。

`pol_flg` は、`tmout` に `TMO_POL` を指定した `twai_flg` と等価である。

## 使用例
```c
void poll_task(intptr_t exinf) {
    FLGPTN ptn;
    ER ercd = pol_flg(SYS_FLG, EV_DATA_READY, TWF_ANDW, &ptn);
    if (ercd == E_OK) {
        process_data();
    }
    else if (ercd == E_TMOUT) {
        /* まだデータが揃っていない */
    }
}
```

## 関連
- `wai_flg` — イベントフラグ待ち（ブロッキング）
- `twai_flg` — イベントフラグ待ち（タイムアウト付き）
- `set_flg` / `clr_flg` — ビットのセット / クリア
- 仕様書 4.4.2 イベントフラグ
