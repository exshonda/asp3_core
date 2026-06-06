# chg_ipm — 割込み優先度マスクの変更

## 種別
〔T〕タスクコンテキスト専用

## C言語API
```c
ER ercd = chg_ipm(PRI intpri);
```

## パラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `intpri` | `PRI` | 設定する割込み優先度マスク（`TMIN_INTPRI` 〜 `TIPM_ENAALL`） |

## リターンパラメータ
| 名称 | 型 | 説明 |
|---|---|---|
| `ercd` | `ER` | 正常終了（`E_OK`）またはエラーコード |

## エラーコード
| コード | 条件 |
|---|---|
| `E_OK` | 正常終了 |
| `E_CTX` | 非タスクコンテキストから呼び出した、またはCPUロック状態で呼び出した |
| `E_PAR` | `intpri` が範囲外（`TMIN_INTPRI` 〜 `TIPM_ENAALL` の範囲外） |

> 範囲判定は `TMIN_INTPRI <= intpri && intpri <= TIPM_ENAALL`（`TIPM_ENAALL` は 0）。

## 機能
割込み優先度マスクを `intpri` で指定した値に変更する。

割込み優先度マスクを設定すると、それより低い優先度（値が大きい側）の割込みがマスク
（禁止）される。`intpri` に `TIPM_ENAALL`（＝0、全割込み許可）を指定すると、すべての
割込みが許可された状態になる。`intpri` は割込み優先度なので負値で表し、`TMIN_INTPRI`
（最高優先度）から `TIPM_ENAALL`（0）までの範囲で指定する。

`TIPM_ENAALL` 以外を指定した場合、ディスパッチ保留状態となる（`dspflg = false`）。
`TIPM_ENAALL` を指定したときに、より高い優先度のタスクが実行可能であればディスパッチが
起こる。

タスクコンテキスト専用であり、CPUロック状態でも呼び出せない（いずれの場合も `E_CTX`）。

## 使用例
```c
void task(intptr_t exinf) {
    chg_ipm(TMIN_INTPRI);   /* すべてのカーネル管理割込みをマスク */
    /* 高優先度の割込みも止めたい臨界区間 */
    chg_ipm(TIPM_ENAALL);   /* 全割込み許可に戻す */
}
```

## 関連
- `get_ipm` — 割込み優先度マスクの参照（対操作）
- `loc_cpu` / `unl_cpu` — CPUロック / 解除（より強い割込み禁止）
- `dis_int` / `ena_int` — 個別割込みの禁止 / 許可
- 仕様書 4.9 割込み管理機能
