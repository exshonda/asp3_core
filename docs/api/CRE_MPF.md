# CRE_MPF — 固定長メモリプールの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.5.1 固定長メモリプール](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。

## 静的API
```
CRE_MPF(mpfid, { mpfatr, blkcnt, blksz, mpf });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_MPF #mpfid* { .mpfatr .blkcnt .blksz &mpf? &mpfmb? }
```

| 記法 | 意味 |
|---|---|
| `#mpfid*` | オブジェクトID定義・キーパラメータ |
| `.mpfatr` | 符号無し整数定数式（属性） |
| `.blkcnt` | 符号無し整数定数式（ブロック数） |
| `.blksz` | 符号無し整数定数式（ブロックサイズ） |
| `&mpf?` | 一般整数定数式・オプション（メモリプール領域） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `mpfid` | `ID` | 生成する固定長メモリプールのID番号 |
| パケット | `mpfatr` | `ATR` | メモリプール属性（下表） |
| パケット | `blkcnt` | `uint32_t` | メモリブロック数（1 以上） |
| パケット | `blksz` | `uint32_t` | メモリブロックのサイズ（バイト、1 以上） |
| パケット | `mpf` | `MPF_T *` | メモリプール領域の先頭番地（省略可。`NULL` で自動割付け） |

## 属性（mpfatr）
| 属性 | 説明 |
|---|---|
| `TA_TFIFO` | 待ちタスクのキューをFIFO順にする（デフォルト） |
| `TA_TPRI` | 待ちタスクのキューを優先度順にする |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `mpfatr` が不正 |
| `E_PAR` | `blkcnt` または `blksz` が 0 |
| `E_ID` | `mpfid` が不正・範囲外 |
| `E_OBJ` | `mpfid` が重複登録 |
| `E_NOMEM` | メモリプール領域が確保できない（自動割付け時） |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`mpfid` で指定したIDの固定長メモリプールを静的に生成する。

ASP3 では動的メモリ確保（`malloc` 等）を使わないため、固定長メモリプールが動的なバッファ割付けの
代替手段となる。`blkcnt` 個の `blksz` バイトブロックをプールとして管理する。

## 使用例
```
#define MSG_POOL_CNT  8
#define MSG_SIZE      64

CRE_MPF(MSG_MPF, { TA_TFIFO, MSG_POOL_CNT, MSG_SIZE, NULL });
```

## 関連
- `get_mpf` — メモリブロックの獲得
- `rel_mpf` — メモリブロックの返却
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.5.1 固定長メモリプール / 5.2 静的API一覧
