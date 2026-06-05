# CRE_CYC — 周期通知の生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.6 時間管理機能](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
CRE_CYC(cycid, { cycatr, { nfymode, par1, par2, par3, par4 }, cyctim, cycphs });
```

第2パラメータは `CRE_ALM` と同じ**通知方式のネスト構造**。その後に周期・位相が続く。

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_CYC #cycid* { .cycatr { .nfymode &par1 &par2? &par3? &par4? } .cyctim .cycphs }
```

| 記法 | 意味 |
|---|---|
| `#cycid*` | オブジェクトID定義・キーパラメータ |
| `.cycatr` | 符号無し整数定数式（属性。`TA_STA`=生成時に動作開始 / `TA_NULL`） |
| `{ .nfymode &par1 &par2? &par3? &par4? }` | 通知方式のネスト構造（`CRE_ALM` と共通） |
| `.cyctim` | 符号無し整数定数式（周期、μs） |
| `.cycphs` | 符号無し整数定数式（起動位相、μs） |

## パラメータ
| 位置 | 名称 | 説明 |
|---|---|---|
| 第1引数 | `cycid` | 生成する周期通知のID番号（`ID`） |
| パケット | `cycatr` | 周期通知属性（`TA_STA` / `TA_NULL`） |
| パケット | `{ nfymode, par1.. }` | 周期到来時の**通知方式**（`CRE_ALM` の表を参照） |
| パケット | `cyctim` | 通知周期（μs） |
| パケット | `cycphs` | 起動位相（μs） |

## 通知方式（nfymode）
`CRE_ALM` と共通：`TNFY_HANDLER` / `TNFY_ACTTSK` / `TNFY_WUPTSK` / `TNFY_SIGSEM` / `TNFY_SETFLG` / `TNFY_SNDDTQ` 等。詳細は [CRE_ALM](CRE_ALM.md) の通知方式表を参照。

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `cycatr` が不正 |
| `E_PAR` | `nfymode`/par が不正、`cyctim` が 0 または範囲外 |
| `E_ID` | `cycid` が不正・範囲外 |
| `E_OBJ` | `cycid` が重複登録 |

## 機能
`cycid` で指定した周期通知を静的に生成する。`cycphs` 経過後を最初の通知時刻として、
以降 `cyctim` 周期で `nfymode` の通知を繰り返す。`TA_STA` 指定時は生成時に動作開始、
非指定時は `sta_cyc(cycid)` で開始する。

## 使用例
```
/* sample1.cfg より：2秒周期でハンドラ通知、位相0 */
CRE_CYC(CYCHDR1, { TA_NULL, { TNFY_HANDLER, 0, cyclic_handler }, 2000000, 0 });
```

## 関連
- `sta_cyc` / `stp_cyc` — 周期通知の動作開始 / 停止
- [CRE_ALM](CRE_ALM.md) — 同じ通知方式構造を使うアラーム通知
- `kernel/kernel_api.def`（構造の正本）／ 仕様書 4.6 時間管理機能
