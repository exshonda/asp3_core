# get_pri — タスク優先度（現在優先度）の参照

## 種別
〔T〕タスクコンテキスト専用（非タスクコンテキストから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = get_pri(ID tskid, PRI *p_tskpri);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `tskid` | `ID` | 対象タスクのID番号。`TSK_SELF`（=0）で自タスク |
| `p_tskpri` | `PRI *` | 現在優先度を格納する領域へのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_tskpri` | `PRI` | 対象タスクの現在優先度（`TMIN_TPRI`〜`TMAX_TPRI`＝1〜16） |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態で呼び出した |
| `E_ID` | `tskid` が不正・範囲外（`TSK_SELF` 以外を指定した場合のみチェック） |
| `E_OBJ` | 対象タスクが休止状態（DORMANT）である |

## 機能
`tskid` で指定したタスクの現在優先度を読み出し、`p_tskpri` が指す領域に格納する。
現在優先度は、ミューテックスの優先度継承・優先度上限プロトコルによる引き上げを反映した
実効的な優先度である。

対象タスクが休止状態（DORMANT）の場合は現在優先度が定義されないため `E_OBJ` を返す。

`TSK_SELF`（=0）を指定すると自タスクを対象とする。

## 使用例
```c
void main_task(EXINF exinf) {
    PRI pri;
    ER  ercd = get_pri(TSK_SELF, &pri);
    if (ercd == E_OK) {
        syslog(LOG_NOTICE, "current priority = %d", pri);
    }
}
```

## 関連
- `chg_pri` — タスク優先度（ベース優先度）の変更
- `ref_tsk` — タスク状態の参照（`tskpri`／`tskbpri` を取得）
- 仕様書 4.1 タスク管理機能
