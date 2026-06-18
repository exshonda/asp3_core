# CFG_SPEC_MAP — cfg 上流追従の対応台帳

> **役割**：Python コンフィギュレータ（`cfg/`）の上流 ASP3（Ruby 版 cfg）への
> 追従を管理する台帳。上流 cfg が変わったとき「テキストマージで足りるのか／
> 移植版へ手動再反映が要るのか」を層ごとに判定するための正本。
> （本ファイルは `docs/asp3_derivative_plan.md` §8.1 から独立・現役化したもの。
> 計画文書側は歴史記録として凍結。）

関連：[`DIVERGENCE_MAP.md`](../../DIVERGENCE_MAP.md)（ファイル単位の乖離台帳）／
[`cfg-python.md`](cfg-python.md)（Python化の実施記録）／
AGENTS.md §7（cfg の運用ルール）・§10（上流マージ支援手順）。

---

## cfg は3層に分かれる（層ごとにマージ難易度が異なる）

### 層① 静的API定義 ＝ `kernel/kernel_api.def`（api-table）

各静的APIのパラメータ列・型を宣言するテキスト DSL。**上流と同形式のため
テキストマージ可能**（PRISTINE に近い扱い）。上流が静的APIを追加・変更しても
このファイルの差分で追える。

```
CRE_TSK #tskid* { .tskatr &exinf &task +itskpri .stksz &stk? }
```

接頭辞DSL：`#`=ID, `.`=符号無し, `+`=符号付き, `&`=一般, `^`=ポインタ,
`$`=文字列、後置 `*`=キー `?`=オプション `...`=リスト。
**構造の正本は `kernel/kernel_api.def`**（YAML 等の二重定義は持たない）。

### 層② cfgエンジン ＝ `cfg.py` / `pass1.py` / `pass2.py` ほか（Ruby→Python移植）

api-table を解釈してコード生成する汎用エンジン。**ここが「テキスト差分が効かない」
DIVERGED 部分**。上流の `cfg.rb` / `pass1.rb` / `pass2.rb` の変更は、移植版へ
手作業で反映する必要がある。

| 上流（Ruby） | 移植版（Python） | 役割 | 最終確認バージョン |
|---|---|---|---|
| `cfg.rb` | `cfg.py` | エントリ・多パス制御 | cfg 1.7.1 |
| `pass1.rb` | `pass1.py` | api-table読込・構文解析 | cfg 1.7.1 |
| `pass2.rb` | `pass2.py` | コード生成 | cfg 1.7.1 |
| `GenFile.rb` | `gen_file.py` | 生成ファイル管理 | cfg 1.7.1 |
| `SRecord.rb` | `srecord.py` | Sレコード読込 | cfg 1.7.1 |

### 層③ 生成テンプレート（旧 `.trb` → `.py`）

`kernel/*.trb`・`arch/*/*.trb`・`target/*/target_*.trb` の Python 版（`.py`）を
併設している（移植元：asp3_fsp、2026-06-05 導入）。
代表：`core_offset.py`（arch）・`target_kernel.py`・`target_check.py`（target）。
上流の `.trb` 変更時は対応する `.py` へ手動再反映する（[`DIVERGENCE_MAP.md`](../../DIVERGENCE_MAP.md) 参照）。

---

## 上流 cfg が変わったときの判定フロー

```
上流cfgの変更を検知
   ├─ 静的APIの追加・変更だけ？
   │     → 層①：kernel_api.def の差分マージで済む（テキストマージ可）
   └─ エンジン／テンプレートの挙動変更？
         → 層②/③：上記対応表と「最終確認バージョン」で
            移植版（.py）の更新箇所を特定し、手動再反映
```

マージ支援ワークフロー（AGENTS.md §10）のステップ6「cfgに関わる変更は
CFG_SPEC_MAP を参照する」が、この判定にあたる。

関連 skill（構想）：`cfg-diff`（入力＝上流cfg変更＋CFG_SPEC_MAP／
出力＝Python実装の更新要否と対象箇所リスト）。

---

## 更新ルール

- 上流 cfg を追従して移植版（`.py`）を再反映したら、上の対応表の
  **「最終確認バージョン」を更新**する。
- 層①（api-table）に静的APIを追加したら、`kernel/kernel_api.def` の行追加のみ
  （本ファイルの更新は不要）。
- ファイル単位の乖離（どのファイルを改変したか）は [`DIVERGENCE_MAP.md`](../../DIVERGENCE_MAP.md) 側に記録する。
