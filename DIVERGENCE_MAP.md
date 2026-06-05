# DIVERGENCE_MAP.md — 上流ASP3との乖離台帳

> 上流TOPPERS/ASP3から**意図的に変更した箇所**をすべて記録する台帳。  
> 手動マージ時、この表と上流diffを照合して影響ファイルを特定する。  
> **上流ファイルに手を入れるたびに、この表へ追記すること。**

現在の上流ベース：`UPSTREAM_VERSION` を参照

---

## 乖離一覧

| ファイル / ディレクトリ | 変更種別 | 理由 | 上流変更時のリスク | 担当レイヤ | 最終確認バージョン |
|---|---|---|---|---|---|
| `syssvc/serial.c` | 上流 `non_tecs` 由来 | TECSレス版を上流拡張から採用 | `extension/non_tecs/syssvc` の更新に追従 | syssvc(EXTENDED) | 3.7.0 |
| `syssvc/logtask.c` | 上流 `non_tecs` 由来 | 同上 | 同上 | syssvc(EXTENDED) | 3.7.0 |
| `syssvc/banner.c` | 上流 `non_tecs` 由来 | 同上 | 同上 | syssvc(EXTENDED) | 3.7.0 |
| `syssvc/syslog.c` | `non_tecs` 由来＋構造化ログ追加 | TECSレス版に構造化ログ（`T=,EV=`）を付加 | **構造化ログ部分が局所改変・要確認** | syssvc(DIVERGED) | 3.7.0 |
| `syssvc/qemu_exit.c` | 新規追加 | QEMUセミホスティング終了 | （上流に存在せず・衝突なし） | syssvc(NEW) | — |
| `cfg/cfg.py`・`pass1.py`・`pass2.py` | Ruby→Python移植（エンジン） | コンフィギュレータのPython化 | **上流cfg.rb系の挙動変更時はテキスト差分不可・手動再反映（CFG_SPEC_MAP参照）** | cfg | cfg 1.7.0 |
| `kernel/kernel_api.def` | 上流同形式（変更なし〜微修正） | 静的API定義（api-table） | 上流と同形式のためテキストマージ可能 | cfg(PRISTINE寄り) | 3.7.0 |
| `arch/*/common/core_offset.py` 他 `*.py` 生成テンプレート | 旧`.trb`→Python移植 | offset.h・kernel_cfg生成テンプレートのPython化 | **上流の対応`.trb`変更時はテキスト差分不可・手動再反映** | cfg(テンプレート) | 3.7.0 |
| `target/*/target_kernel.py`・`target_check.py` | 旧`.trb`→Python移植 | pass2/pass3テンプレートのPython化 | 新規ターゲットでは新規作成（衝突なし） | target(テンプレート) | — |
| `Makefile` 系 | CMakeへ置換 | ビルドシステム近代化 | 上流Makefile変更は参照のみ・取り込まない | build | 3.7.0 |
| `CMakeLists.txt` | 新規追加 | CMakeビルド | （上流に存在せず・衝突なし） | build(NEW) | — |
| `cmake/` 一式 | 新規追加 | ツールチェーン・CMakeモジュール | （上流に存在せず・衝突なし） | build(NEW) | — |
| `arch/riscv_gcc/` | 新規追加 | RP2350 RISC-V対応 | 上流がRISC-V対応を追加した場合は統合検討 | arch(NEW) | — |
| `target/mps2_an521_gcc/` | 新規追加 | QEMU Cortex-M33ターゲット | （上流に存在せず・衝突なし） | target(NEW) | — |
| `target/linux_gcc/` | 改変（上流linux_gccベース） | CLIターゲット・TAP対応 | 上流linux_gcc変更時に要確認 | target | 3.7.0 |
| `target/rp2350-arm-s_pico_sdk/` | 新規追加 | asp3_pico_sdkから移植 | （上流に存在せず・衝突なし） | target(NEW) | — |
| `target/rp2350-riscv_pico_sdk/` | 新規追加 | RP2350 RISC-V | （上流に存在せず・衝突なし） | target(NEW) | — |
| `target/stm32mp257f_dk_arm64_gcc/` | 新規追加 | stm32_vscode_aspから移植 | （上流に存在せず・衝突なし） | target(NEW) | — |

---

## 変更種別の凡例

- **TECSレス**：上流の `extension/non_tecs/`（syssvc・ホストターゲット等）を採用。ローカル発明ではなく上流拡張。局所改変は syslog への構造化ログ付加のみ
- **新規追加**：上流に存在しないファイル（衝突なし・上流マージで無視可）
- **改変**：上流ファイルをベースに変更（マージ時に要確認）
- **全面置換**：上流とは別実装（テキストマージ不可・専用台帳で管理）

---

## マージ時の参照手順（AGENTS.md §10 と対応）

1. `git diff upstream main` で全乖離を把握
2. 上流の新バージョンdiffを取得
3. 本表の各行と照合し、「変更種別＝改変/全面置換/TECS-less化」の行に上流変更が当たるか確認
4. 当たる場合は「最終確認バージョン」を見て差分を手動マージ、確認後にバージョン更新
5. 「新規追加」の行は上流マージの影響を受けない
6. cfg関連は `docs/CFG_SPEC_MAP.md` を併用
