# DIVERGENCE_MAP.md — 上流ASP3との乖離台帳

> 上流TOPPERS/ASP3から**意図的に変更した箇所**をすべて記録する台帳。  
> 手動マージ時、この表と上流diffを照合して影響ファイルを特定する。  
> **上流ファイルに手を入れるたびに、この表へ追記すること。**

現在の上流ベース：`UPSTREAM_VERSION` を参照

---

## 乖離一覧

| ファイル / ディレクトリ | 変更種別 | 理由 | 上流変更時のリスク | 担当レイヤ | 最終確認バージョン |
|---|---|---|---|---|---|
| `configure.rb` | 改変 | 非TECSビルドをデフォルト化（`OMIT_TECS`初期設定＋共通syssvcオブジェクトの自動付与）・CFGデフォルトをPython版cfgに変更 | 上流configure.rb変更時に要確認（変更箇所は【asp3_core変更】コメントでマーク） | build | 3.7.2 |
| `sample/Makefile`・`arch/*/common/Makefile.core`・`target/{dummy_gcc,simtimer_ct11mpcore_gcc}/Makefile.target` | 改変 | 生成テンプレート参照を`.trb`→`.py`に切替 | 上流Makefile変更時に要確認（【asp3_core変更】コメント） | build | 3.7.2 |
| `target/{ct11mpcore,gr_peach,dummy}_gcc/Makefile.target` | 改変 | OMIT_TECS時の非TECS SIOオブジェクト追加 | 上流ターゲットパッケージ更新時に要確認 | target | 3.7.2 |
| `syssvc/serial.c` | 上流 `non_tecs` 由来 | TECSレス版を上流拡張から採用 | `extension/non_tecs/syssvc` の更新に追従 | syssvc(EXTENDED) | 3.7.0 |
| `syssvc/logtask.c` | 上流 `non_tecs` 由来 | 同上 | 同上 | syssvc(EXTENDED) | 3.7.0 |
| `syssvc/banner.c` | 上流 `non_tecs` 由来 | 同上 | 同上 | syssvc(EXTENDED) | 3.7.0 |
| `syssvc/syslog.c` | 上流 `non_tecs` 由来（構造化ログ追加は計画中） | TECSレス版を採用（`T=,EV=`構造化ログは未実施） | `extension/non_tecs/syssvc` の更新に追従 | syssvc(EXTENDED) | 3.7.2 |
| `target/zybo_z7_gcc/`（非TECS化） | 改変＋新規追加 | TECSレス化（`target_serial.{c,h,cfg}`新規・`target_syssvc.h`/`target_kernel_impl.c`/`Makefile.target`/`MANIFEST`改変） | **上流ZYBOターゲットパッケージ由来のため上流更新時要確認** | target | 3.7.2 |
| `arch/arm_gcc/zynq7000/xuartps.[ch]` | `xuartps.c`新規・`xuartps.h`改変（非TECSドライバAPI追加）・`MANIFEST`改変 | 非TECS版SIOドライバ | 上流zynq7000依存部の`xuartps.h`変更時に要確認 | arch | 3.7.2 |
| `target/mps2_an521_gcc/`（非TECS化） | 新規追加（`cmsdk_uart.[ch]`・`target_serial.{c,h,cfg}`）＋改変 | TECSレス化 | （ターゲット自体がNEW・上流衝突なし） | target(NEW) | — |
| `target/stm32mp257f_dk_arm64_gcc/`・`arch/arm64_gcc/stm32mp2/`（非TECS化） | 新規追加（`stm32usart.c`・`target_serial.{c,h,cfg}`）＋改変 | TECSレス化（経緯は`PORTING_ASP3_STM32MP2.md`） | （ターゲット自体がNEW・上流衝突なし） | target/arch(NEW) | — |
| `target/raspberrypi_pico2_gcc/`・`arch/arm_m_gcc/rp2350/`（非TECS化） | 新規追加（`rp2350_uart.[ch]`・`chip_serial.{c,h,cfg}`・`target_serial.{h,cfg}`）＋改変 | TECSレス化（経緯は`PORTING.md`） | （ターゲット自体がNEW・上流衝突なし） | target/arch(NEW) | — |
| `target/linux_gcc/`・`arch/posix_gcc/` | 上流SVN（3.7.2）から取込み＋`Makefile.target`改変 | POSIXシミュレーション環境（3.7.2 tarball未収録のため別途取込み） | 上流posix_gccパッケージの更新に追従 | target/arch | 3.7.2 |
| `syssvc/qemu_exit.c` | 新規追加 | QEMUセミホスティング終了 | （上流に存在せず・衝突なし） | syssvc(NEW) | — |
| `cfg/cfg.py`・`pass1.py`・`pass2.py`・`gen_file.py`・`srecord.py` | Ruby→Python移植（エンジン，asp3_fsp由来＋1.7.1差分反映） | コンフィギュレータのPython化（Ruby版は残置） | **上流cfg.rb系の挙動変更時はテキスト差分不可・手動再反映（CFG_SPEC_MAP参照）** | cfg | cfg 1.7.1 |
| `kernel/*.py`（kernel.py・kernel_check.py・genoffset.py＋オブジェクト別11本） | 旧`kernel/*.trb`→Python移植（新規追加・既存.trbは未変更） | 生成テンプレートのPython化（kernel/への新規ファイル追加） | **上流kernel/*.trb変更時は対応.pyへ手動再反映** | kernel(テンプレート追加) | 3.7.2 |
| `kernel/kernel_api.def` | 上流同形式（変更なし〜微修正） | 静的API定義（api-table） | 上流と同形式のためテキストマージ可能 | cfg(PRISTINE寄り) | 3.7.0 |
| `arch/*/*.py` 生成テンプレート（core_kernel/core_check/core_offset/gic/chip） | 旧`.trb`→Python移植（arm_m/arm_gcc/arm64_gcc/posix各系列．v6m系は未変換） | offset.h・kernel_cfg生成テンプレートのPython化 | **上流の対応`.trb`変更時はテキスト差分不可・手動再反映** | cfg(テンプレート) | 3.7.2 |
| `target/*/target_kernel.py`・`target_check.py`（全9ターゲット＋dummyのtarget_offset.py） | 旧`.trb`→Python移植 | pass2/pass3テンプレートのPython化 | 上流由来ターゲットは上流.trb変更時に対応.pyへ手動再反映 | target(テンプレート) | 3.7.2 |
| `Makefile` 系 | CMakeへ置換 | ビルドシステム近代化 | 上流Makefile変更は参照のみ・取り込まない | build | 3.7.0 |
| `CMakeLists.txt` | 新規追加 | CMakeビルド | （上流に存在せず・衝突なし） | build(NEW) | — |
| `cmake/` 一式 | 新規追加 | ツールチェーン・CMakeモジュール | （上流に存在せず・衝突なし） | build(NEW) | — |
| `arch/riscv_gcc/` | 新規追加 | RP2350 RISC-V対応 | 上流がRISC-V対応を追加した場合は統合検討 | arch(NEW) | — |
| `target/mps2_an521_gcc/` | 新規追加 | QEMU Cortex-M33ターゲット | （上流に存在せず・衝突なし） | target(NEW) | — |
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
