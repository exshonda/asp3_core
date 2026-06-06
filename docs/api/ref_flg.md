# ref_flg — イベントフラグの状態参照

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = ref_flg(ID flgid, T_RFLG *pk_rflg);
```

`T_RFLG` の定義（`include/kernel.h`）：

```c
typedef struct t_rflg {
    ID      wtskid;     /* イベントフラグの待ち行列の先頭のタスクのID番号 */
    FLGPTN  flgptn;     /* イベントフラグの現在のビットパターン */
} T_RFLG;
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `flgid` | `ID` | 対象イベントフラグのID番号 |
| `pk_rflg` | `T_RFLG *` | 状態を格納するパケットへのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `pk_rflg->wtskid` | `ID` | 待ち行列の先頭タスクのID番号（待ちタスクがなければ `TSK_NONE`） |
| `pk_rflg->flgptn` | `FLGPTN` | 現在のビットパターン |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態 |
| `E_ID` | `flgid` が不正・範囲外 |

## 機能
`flgid` で指定したイベントフラグの現在の状態を参照し、`pk_rflg` で指定したパケットに
格納する。

- `wtskid`：イベントフラグの待ち行列の先頭タスクのID番号。待ち行列が空の場合は
  `TSK_NONE` を格納する。
- `flgptn`：イベントフラグの現在のビットパターン。

## 使用例
```c
void diag_task(intptr_t exinf) {
    T_RFLG rflg;
    if (ref_flg(SYS_FLG, &rflg) == E_OK) {
        /* rflg.flgptn = 現在のビットパターン, rflg.wtskid = 待ち先頭タスク */
    }
}
```

## 関連
- `set_flg` / `clr_flg` — ビットのセット / クリア
- `wai_flg` / `pol_flg` / `twai_flg` — イベントフラグ待ち
- `ini_flg` — イベントフラグの再初期化
- 仕様書 4.4.2 イベントフラグ
