# CRE_TSK — タスクの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。** パラメータ列・型はそこで宣言される。
> 意味・エラー・使用例の正本は [統合仕様書 4.1 タスク管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。
> 本ファイルは両者をまとめた人間/AI向けリファレンス。

## 静的API
```
CRE_TSK(tskid, { tskatr, exinf, task, itskpri, stksz, stk });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_TSK #tskid* { .tskatr &exinf &task +itskpri .stksz &stk? }
```

| 記法 | 意味 |
|---|---|
| `#tskid*` | オブジェクトID定義・キーパラメータ |
| `.tskatr` | 符号無し整数定数式（属性） |
| `&exinf` `&task` | 一般整数定数式 |
| `+itskpri` | 符号付き整数定数式 |
| `.stksz` | 符号無し整数定数式 |
| `&stk?` | 一般整数定数式・オプション |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `tskid` | `ID` | 生成するタスクのID番号 |
| パケット | `tskatr` | `ATR` | タスク属性（下表） |
| パケット | `exinf` | `intptr_t` | 拡張情報（起動時にパラメータとして渡る） |
| パケット | `task` | `TASK` | タスクのメインルーチンの先頭番地 |
| パケット | `itskpri` | `PRI` | 起動時優先度（1〜`TMAX_TPRI`） |
| パケット | `stksz` | `SIZE` | スタックサイズ（バイト） |
| パケット | `stk` | `STK_T*` | スタック領域先頭番地（省略可。`NULL`=自動割付け） |

## 属性（tskatr）
| 属性 | 説明 |
|---|---|
| `TA_ACT` | 生成時に起動された状態（実行可能状態）にする |
| `TA_NULL` | 属性指定なし（生成時は休止状態） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `tskatr` が不正 |
| `E_PAR` | `task` が NULL / `itskpri` が範囲外 / `stksz` が下限未満 |
| `E_ID` | `tskid` が不正・範囲外 |
| `E_OBJ` | `tskid` が重複登録 |
| `E_NOMEM` | スタック領域が確保できない（自動割付け時） |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`tskid` で指定したIDのタスクを静的に生成する。ASP3は動的生成を持たないため、
タスクはすべてこの静的APIで生成する。

`tskatr` に `TA_ACT` を指定すると生成時に起動された状態になる。指定しない（`TA_NULL`）
場合は休止状態で生成され、`act_tsk` で起動する。`stk` を省略（または `NULL`）すると、
コンフィギュレータが `stksz` バイトのスタックを自動割付けする。

## 使用例
```
/* sample/sample1.cfg より：優先度・スタックは構成定数 */
CRE_TSK(MAIN_TASK, { TA_ACT, 0, main_task, MAIN_PRIORITY, STACK_SIZE, NULL });
CRE_TSK(TASK1,     { TA_NULL, 1, task, MID_PRIORITY, STACK_SIZE, NULL });
```

## 関連
- [act_tsk](act_tsk.md) — 休止状態のタスクを起動する
- `kernel/kernel_api.def` — cfgが読む構造定義（api-table・構造の正本）
- 仕様書 4.1 タスク管理機能 / 5.2 静的API一覧
