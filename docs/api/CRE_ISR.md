# CRE_ISR — 割込みサービスルーチンの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。** パラメータ列・型はそこで宣言される。
> 意味・エラー・使用例の正本は [統合仕様書 4.9 割込み管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。
> 本ファイルは両者をまとめた人間/AI向けリファレンス。

## 静的API
```
CRE_ISR(isrid, { isratr, exinf, intno, isr, isrpri });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_ISR #isrid* { .isratr &exinf .intno &isr +isrpri }
```

| 記法 | 意味 |
|---|---|
| `#isrid*` | オブジェクトID定義・キーパラメータ |
| `.isratr` | 符号無し整数定数式（属性） |
| `&exinf` | 一般整数定数式（拡張情報） |
| `.intno` | 符号無し整数定数式（割込み番号） |
| `&isr` | 一般整数定数式（ISRの先頭番地） |
| `+isrpri` | 符号付き整数定数式（ISR優先度） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `isrid` | `ID` | 生成する割込みサービスルーチンのID番号 |
| パケット | `isratr` | `ATR` | 割込みサービスルーチン属性（下表） |
| パケット | `exinf` | `intptr_t` | 拡張情報（ISRにパラメータとして渡る） |
| パケット | `intno` | `INTNO` | ISRを付加する割込み番号（プロセッサ依存） |
| パケット | `isr` | `ISR` | 割込みサービスルーチンの先頭番地 |
| パケット | `isrpri` | `PRI` | ISR優先度（同一割込み番号内での実行順序。1〜`TMAX_ISRPRI`） |

> `ISR` 型は `void (*)(EXINF exinf)`（`include/kernel.h`）。

## 属性（isratr）
| 属性 | 説明 |
|---|---|
| `TA_NULL` | 属性指定なし（唯一の有効値） |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `isratr` が不正 |
| `E_PAR` | `intno` が範囲外 / `isr` が NULL / `isrpri` が範囲外 |
| `E_ID` | `isrid` が不正・範囲外 |
| `E_OBJ` | `isrid` が重複登録 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`isrid` で指定したIDの割込みサービスルーチン（ISR）を静的に生成し、`intno` で指定した
割込み番号に付加する。

割込みサービスルーチン方式は、複数のISRを一つの割込み番号に
付加でき、`isrpri`（ISR優先度）の昇順に実行する（μITRON4.0の `ATT_ISR` に相当。
ASP3ではISRをID付きオブジェクトとして生成する `CRE_ISR` に置き換えられた）。これは割込みハンドラを直接管理する
`DEF_INH`（割込みハンドラ方式）に対する高位の抽象であり、SDK・ドライバとの協調がしやすい。

`CRE_ISR` を使う割込み番号には、`CFG_INT` で割込み要求ラインの設定（トリガ種別・優先度）が
必要である。

`exinf` はISRが起動されるときにパラメータとして渡される拡張情報である。

## 使用例
```c
void intno1_isr(intptr_t exinf) {
    /* 割込みサービスルーチン本体 */
}
```
```
/* sample/sample1.cfg より */
CFG_INT(INTNO1, { INTNO1_INTATR, INTNO1_INTPRI });
CRE_ISR(INTNO1_ISR, { TA_NULL, 0, INTNO1, intno1_isr, 1 });
```

## 関連
- [CFG_INT](CFG_INT.md) — 割込み要求ラインの設定（CRE_ISR と組み合わせて使う）
- [DEF_INH](DEF_INH.md) — 割込みハンドラの定義（割込みハンドラ方式・代替手段）
- `kernel/kernel_api.def` — cfgが読む構造定義（api-table・構造の正本）
- 仕様書 4.9 割込み管理機能 / 5.2 静的API一覧
