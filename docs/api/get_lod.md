# get_lod — 指定優先度の実行できるタスク数の参照

## 種別
〔T〕タスクコンテキスト専用（非タスクコンテキストから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = get_lod(PRI tskpri, uint_t *p_load);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `tskpri` | `PRI` | 対象とするタスク優先度（`TMIN_TPRI`〜`TMAX_TPRI`＝1〜16）。`TPRI_SELF`（=0）で自タスクのベース優先度 |
| `p_load` | `uint_t *` | タスク数を格納する領域へのポインタ |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |
| `*p_load` | `uint_t` | 指定優先度の実行できる状態のタスクの数 |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した／CPUロック状態で呼び出した |
| `E_PAR` | `tskpri` が不正・範囲外（`TPRI_SELF` 以外を指定した場合のみチェック） |

## 機能
`tskpri` で指定した優先度における、実行できる状態（実行可能状態および実行状態）にある
タスクの数を読み出し、`p_load` が指す領域に格納する。指定優先度のレディキューに
つながれているタスクの数を数える。

`tskpri` に `TPRI_SELF`（=0）を指定すると、自タスクのベース優先度を対象とする。

## 使用例
```c
void scheduler_task(EXINF exinf) {
    uint_t load;
    if (get_lod(HIGH_PRIORITY, &load) == E_OK && load > 0U) {
        syslog(LOG_NOTICE, "%d runnable task(s) at high priority", load);
    }
}
```

## 関連
- `get_nth` — 指定優先順位のタスクIDの参照
- `rot_rdq` — レディキューの回転
- 仕様書 4.7 システム状態管理機能
