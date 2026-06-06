# rsm_tsk — 強制待ち状態からの再開

## 種別
〔T〕タスクコンテキスト専用（非タスクから呼ぶと `E_CTX`）

## C言語API
```c
ER ercd = rsm_tsk(ID tskid);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `tskid` | `ID` | 再開対象タスクのID番号（`TSK_SELF` 不可） |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了（対象タスクの強制待ち状態を解除した） |
| `E_CTX` | 非タスクコンテキスト／CPUロック状態（`CHECK_TSKCTX_UNL`） |
| `E_ID` | `tskid` が不正・範囲外（`VALID_TSKID` 不成立。`TSK_SELF` も不可） |
| `E_OBJ` | 対象タスクが強制待ち状態（SUSPENDED）でない |

## 機能
`tskid` で指定したタスクが強制待ち状態（SUSPENDED）にある場合、その強制待ち状態を解除する
（`sus_tsk` の対となるサービスコール）。

- 対象タスクが強制待ち状態のみ（待ち要因を持たない SUSPENDED）の場合、実行できる状態
  （RUNNABLE）へ遷移させる。再開によって対象タスクの方が実行すべき優先度になった場合は
  ディスパッチが起こる。
- 対象タスクが二重待ち状態（WAITING かつ SUSPENDED）の場合、強制待ち状態のみを解除し、
  待ち状態（WAITING）に戻す。

対象タスクが強制待ち状態でない場合は `E_OBJ` を返す。ASP3では強制待ち状態の多重化を行わない
ため、1回の `rsm_tsk` で強制待ち状態が完全に解除される。

`tskid` に `TSK_SELF`（＝0）は指定できない（自タスクは強制待ち状態にないため）。

## 使用例
```c
void supervisor_task(intptr_t exinf) {
    sus_tsk(WORKER_TASK);       /* 一時停止 */
    adjust_parameters();
    rsm_tsk(WORKER_TASK);       /* 再開 */
}
```

## 関連
- `sus_tsk` — 強制待ち状態への移行（`rsm_tsk` の対）
- 仕様書 4.2 タスク付属同期機能
