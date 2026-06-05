# CRE_FLG — イベントフラグの生成（静的API）

## 種別
〔S〕静的API（システムコンフィギュレーションファイル `.cfg` に記述）

> **構造の正本は `kernel/kernel_api.def`（cfgが読むapi-table）。**
> 意味・エラー・使用例の正本は [統合仕様書 4.4.2 イベントフラグ](https://www.toppers.jp/docs/tech/tgki_spec-370/tgki_spec-370.html)。
> 本ファイルは両者をまとめた人間/AI向けリファレンス。

## 静的API
```
CRE_FLG(flgid, { flgatr, iflgptn });
```

## api-table 定義（構造の正本）
`kernel/kernel_api.def` の該当行：
```
CRE_FLG #flgid* { .flgatr .iflgptn }
```

| 記法 | 意味 |
|---|---|
| `#flgid*` | オブジェクトID定義・キーパラメータ |
| `.flgatr` | 符号無し整数定数式（属性） |
| `.iflgptn` | 符号無し整数定数式（初期フラグパターン） |

## パラメータ
| 位置 | 名称 | C-API型 | 説明 |
|---|---|---|---|
| 第1引数 | `flgid` | `ID` | 生成するイベントフラグのID番号 |
| パケット | `flgatr` | `ATR` | イベントフラグ属性（下表） |
| パケット | `iflgptn` | `FLGPTN` | フラグパターンの初期値（通常 `0x0U`） |

## 属性（flgatr）
| 属性 | 説明 |
|---|---|
| `TA_TFIFO` | 待ちタスクのキューをFIFO順にする（デフォルト） |
| `TA_TPRI` | 待ちタスクのキューを優先度順にする |
| `TA_WMUL` | 複数タスクの同時待ちを許可する |
| `TA_CLR` | 待ち解除時にフラグパターンを全クリアする |

## エラーコード（コンフィギュレータが検出）
| コード | 条件 |
|---|---|
| `E_RSATR` | `flgatr` が不正 |
| `E_ID` | `flgid` が不正・範囲外 |
| `E_OBJ` | `flgid` が重複登録 |

> 静的APIは返値を持たない。エラーはコンフィギュレータが報告する。

## 機能
`flgid` で指定したIDのイベントフラグを静的に生成する。

`TA_WMUL` を指定しない場合、同時に1タスクしか待てない（複数タスクが待とうとすると `E_ILUSE`）。
`TA_CLR` を指定すると、`wai_flg` が待ち解除されたとき自動的に全ビットをクリアする。

## 使用例
```
/* 複数タスクが待てる・解除時クリアなし */
CRE_FLG(SYS_FLG, { TA_TFIFO | TA_WMUL, 0x0U });

/* 単一タスク待ち・解除時全クリア */
CRE_FLG(ONE_FLG, { TA_TFIFO | TA_CLR, 0x0U });
```

## 関連
- [set_flg](set_flg.md) — ビットのセット
- [wai_flg](wai_flg.md) — イベントフラグ待ち
- [clr_flg](clr_flg.md) — ビットのクリア
- `kernel/kernel_api.def` — cfgが読む構造定義
- 仕様書 4.4.2 イベントフラグ / 5.2 静的API一覧
