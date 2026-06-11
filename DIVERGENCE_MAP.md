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
| `syssvc/syslog.c` | 上流 `non_tecs` 由来 | TECSレス版を採用（`T=,EV=`構造化ログは`arch/tracelog/`のトレースログで実現） | `extension/non_tecs/syssvc` の更新に追従 | syssvc(EXTENDED) | 3.7.2 |
| `target/zybo_z7_gcc/`（非TECS化） | 改変＋新規追加 | TECSレス化（`target_serial.{c,h,cfg}`新規・`target_syssvc.h`/`target_kernel_impl.c`/`Makefile.target`/`MANIFEST`改変） | **上流ZYBOターゲットパッケージ由来のため上流更新時要確認** | target | 3.7.2 |
| `arch/arm_gcc/zynq7000/xuartps.[ch]` | `xuartps.c`新規・`xuartps.h`改変（非TECSドライバAPI追加）・`MANIFEST`改変 | 非TECS版SIOドライバ | 上流zynq7000依存部の`xuartps.h`変更時に要確認 | arch | 3.7.2 |
| `target/mps2_an521_gcc/`（非TECS化） | 新規追加（`cmsdk_uart.[ch]`・`target_serial.{c,h,cfg}`）＋改変 | TECSレス化 | （ターゲット自体がNEW・上流衝突なし） | target(NEW) | — |
| `target/stm32mp257f_dk_arm64_gcc/`・`arch/arm64_gcc/stm32mp2/`（非TECS化） | 新規追加（`stm32usart.c`・`target_serial.{c,h,cfg}`）＋改変 | TECSレス化（経緯は`PORTING_ASP3_STM32MP2.md`） | （ターゲット自体がNEW・上流衝突なし） | target/arch(NEW) | — |
| `target/pico2_arm_gcc/`・`arch/arm_m_gcc/rp2350/`（非TECS化） | 新規追加（`rp2350_uart.[ch]`・`chip_serial.{c,h,cfg}`・`target_serial.{h,cfg}`）＋改変 | TECSレス化（経緯は`PORTING.md`）．`rp2350_uart_cls_por()`に送信FIFOドレイン待ちを追加（終了時の末尾出力欠落の修正．`docs/dev/porting-test.md`） | （ターゲット自体がNEW・上流衝突なし） | target/arch(NEW) | — |
| `target/linux_gcc/`・`arch/posix_gcc/` | 上流SVN（3.7.2）から取込み | POSIXシミュレーション環境（3.7.2 tarball未収録のため別途取込み） | 上流posix_gccパッケージの更新に追従 | target/arch | 3.7.2 |
| `target/linux_gcc/target_kernel_impl.c` | 改変（CLIターゲット） | main()のargc/argv化＋`--tap`/`--slog`/`--help`追加（経緯は`docs/dev/cli-target.md`） | 上流posix_gccのmain()変更時に要手動マージ | target | 3.7.2 |
| `arch/posix_gcc/posix_kernel_impl.c` | 改変（CLIターゲット・上流バグ修正） | LOG_INH_ENTER/LEAVEへ渡す変数の修正（上流は未定義変数inhnoを渡しており，トレース有効時にコンパイル不能．上流報告：`docs/dev/upstream-report-tracelog.md`） | 上流で修正されたら上流版を採用して差分解消 | arch | 3.7.2 |
| `kernel/sys_manage.c` | **改変（PRISTINE領域・要注意）** | get_lod()のLOG_GET_LOD_ENTERに渡す変数の修正（上流は未定義変数p_tskidを渡しており，トレース有効時にコンパイル不能．ユーザー承認のうえ1行のみ修正．上流報告：`docs/dev/upstream-report-tracelog.md`） | **上流マージ時は必ず本行を確認**．上流で修正されたら上流版を採用して差分解消 | kernel(PRISTINE+1行) | 3.7.2 |
| `syssvc/test_svc.c`・`test_svc.h` | 上流 `non_tecs` 由来＋改変（CLIターゲット） | TAP出力モード（`test_tap_mode`）を追加（経緯は`docs/dev/cli-target.md`） | `extension/non_tecs/syssvc` の更新時に要手動マージ | syssvc(EXTENDED) | 3.7.2 |
| `arch/tracelog/` | 全面置換（CLIターゲット） | TECS版`tTraceLog.c`を削除し，FMP3 3.3の非TECS版（`trace_log.[ch]`・`trace_dump.c`）をASP3変換して採用．構造化ログ出力`trace_slog.c`・機能コード`trace_fncode.h`を新規追加 | 上流ASP3のtracelog（TECS版）変更はテキストマージ不可・FMP3側の更新は手動反映 | arch | 3.7.2 / FMP3 3.3 |
| `syssvc/qemu_exit.c` | 新規追加 | QEMUセミホスティング終了 | （上流に存在せず・衝突なし） | syssvc(NEW) | — |
| `cfg/cfg.py`・`pass1.py`・`pass2.py`・`gen_file.py`・`srecord.py` | Ruby→Python移植（エンジン，asp3_fsp由来＋1.7.1差分反映） | コンフィギュレータのPython化（Ruby版は残置） | **上流cfg.rb系の挙動変更時はテキスト差分不可・手動再反映（CFG_SPEC_MAP参照）** | cfg | cfg 1.7.1 |
| `test/testexec.py`・`test_cfg/testcfg.py`・`utils/{genrename,applyrename,gentest}.py` | Ruby→Python移植（.rbツール群）＋テストランナはCMakeベース化 | ビルド・テスト・開発フローからRuby依存を除去 | **上流.rbツール変更時は対応.pyへ手動再反映** | build/test | 3.7.2 |
| `kernel/*.py`（kernel.py・kernel_check.py・genoffset.py＋オブジェクト別11本） | 旧`kernel/*.trb`→Python移植（新規追加・既存.trbは未変更） | 生成テンプレートのPython化（kernel/への新規ファイル追加） | **上流kernel/*.trb変更時は対応.pyへ手動再反映** | kernel(テンプレート追加) | 3.7.2 |
| `kernel/kernel_api.def` | 上流同形式（変更なし〜微修正） | 静的API定義（api-table） | 上流と同形式のためテキストマージ可能 | cfg(PRISTINE寄り) | 3.7.0 |
| `arch/*/*.py` 生成テンプレート（core_kernel/core_check/core_offset/gic/chip） | 旧`.trb`→Python移植（arm_m/arm_gcc/arm64_gcc/posix各系列．v6m系は未変換） | offset.h・kernel_cfg生成テンプレートのPython化 | **上流の対応`.trb`変更時はテキスト差分不可・手動再反映** | cfg(テンプレート) | 3.7.2 |
| `target/*/target_kernel.py`・`target_check.py`（全9ターゲット＋dummyのtarget_offset.py） | 旧`.trb`→Python移植 | pass2/pass3テンプレートのPython化 | 上流由来ターゲットは上流.trb変更時に対応.pyへ手動再反映 | target(テンプレート) | 3.7.2 |
| `CMakeLists.txt` | 新規追加（`ASP3_LIBRARY_ONLY` オプション＝外部SDKアプリが add_subdirectory して asp3 lib のみ取り込む構成用。経緯は`docs/dev/pico-sdk-integration.md`） | CMakeビルド | （上流に存在せず・衝突なし） | build(NEW) | — |
| `cmake/` 一式 | 新規追加 | ツールチェーンファイル（arm-none-eabi/a35） | （上流に存在せず・衝突なし） | build(NEW) | — |
| `asp3_core.cmake`・`arch/*/arch.cmake`・`arch/*/*/chip.cmake`・`target/*/target.cmake` | 新規追加（`asp3_core.cmake`：`ASP3_TARGET_DIR`変数で外部SDKターゲットの受け入れ口を追加＝後方互換拡張．経緯は`docs/dev/pico-sdk-integration.md`） | CMakeビルド（変数積み上げ方式） | （上流に存在せず・衝突なし） | build(NEW) | — |
| `arch/riscv_gcc/common/`・`arch/riscv_gcc/polarfire_soc/` | 新規追加 | RISC-V（XLEN抽象＝RV64GC/RV32IMAC）コア依存部＋PolarFire SoCチップ依存部（FMP3のPolarFire SoC移植をASP3変換．経緯は`docs/dev/qemu-target-riscv.md`．RV32対応のSTK_T分岐・PLIC/mtimer除外オプション・target_hrt_*リネーム削除は`docs/dev/pico2-riscv.md`） | 上流がRISC-V対応を追加した場合は統合検討（FMP3側の更新は手動反映） | arch(NEW) | — |
| `target/mps2_an521_gcc/` | 新規追加 | QEMU Cortex-M33ターゲット | （上流に存在せず・衝突なし） | target(NEW) | — |
| `target/rp2350-arm-s_pico_sdk/` | 新規追加 | asp3_pico_sdkから移植 | （上流に存在せず・衝突なし） | target(NEW) | — |
| `arch/riscv_gcc/rp2350/`・`target/pico2_riscv_gcc/` | 新規追加 | RP2350 RISC-V（Hazard3）＝SDK非依存ベアメタル移植（Xh3irq・RISC-V IMAGE_DEF．経緯は`docs/dev/pico2-riscv.md`．RP2350.h/rp2350_uart等はarm_m_gcc/rp2350をパス参照で共有） | （上流に存在せず・衝突なし） | arch/target(NEW) | — |
| `target/stm32mp257f_dk_arm64_gcc/` | 新規追加 | stm32_vscode_aspから移植 | （上流に存在せず・衝突なし） | target(NEW) | — |
| `arch/arm64_gcc/zynqmp/`・`target/zcu102_arm64_gcc/` | 新規追加 | QEMU(xlnx-zcu102)用ARMv8-Aターゲット（FMP3のZynqMP移植をASP3変換．経緯は`docs/dev/qemu-target-a64.md`） | （上流に存在せず・衝突なし．FMP3側の更新は手動反映） | arch/target(NEW) | — |
| `target/polarfire_soc_kit_gcc/` | 新規追加 | QEMU(microchip-icicle-kit)用RISC-Vターゲット（FMP3のPolarFire SoC Kit移植をASP3変換．経緯は`docs/dev/qemu-target-riscv.md`） | （上流に存在せず・衝突なし．FMP3側の更新は手動反映） | target(NEW) | — |

---

## 削除済みファイル（上流マージで復活させないこと）

「ファイルの削除」項目（`docs/dev/file-cleanup.md`）で削除した上流由来ファイル。
上流マージ時にこれらが diff に現れても**取り込まない**。

| 削除対象 | 由来項目 | 備考 |
|---|---|---|
| `tecsgen/`・`tecs_kernel/`・syssvc/arch/targetのTECSセル（`.cdl`・t接頭辞）・`sample/{sample1,tSample2}.cdl`・tSample2系・`test/*.cdl`・`extension/non_tecs/` | TECSレス | `docs/dev/tecs-less.md` の削除リスト参照 |
| `cfg/*.rb`・`kernel/*.trb`・`arch/**/*.trb`・`target/*/*.trb` | cfgのPython化 | v6m系trbも削除（必要時は上流から取得して.py変換） |
| `configure.rb`・`test/testexec.rb`・`test_cfg/testcfg.rb`・`utils/*.rb`・`utils/makerelease.py` | .rbツールの.py化 | |
| `arch/tracelog/tTraceLog.c` | CLIターゲット | TECS版トレースログ．非TECS版`trace_log.c`に置換（`docs/dev/cli-target.md`） |
| `MANIFEST`・`E_PACKAGE`（全数．extension/内は残置）・`target/{ct11mpcore,gr_peach,macos_xcode,simtimer_ct11mpcore}_gcc/`・`arch/arm_gcc/rza1/`・`arch/simtimer/`・`test/simt_*.c` | ファイルの削除 | `docs/dev/file-cleanup.md` 参照 |
| `README.txt` | 監査(2026-06-07) | 上流のトップREADME．案内先のdoc/*.txtがMarkdown化で削除済みとなり陳腐化．README.md が後継 |
| `configure.py`・`sample/Makefile`・`kernel/Makefile.kernel`・`arch/*/common/Makefile.core`・`arch/*/*/Makefile.chip`・`target/*/Makefile.target` | ファイルの削除（フェーズ2＝make版ビルド廃止．ビルドはCMakeのみ） | **`kernel/Makefile.kernel` はkernel/領域だが削除（上流マージで復活させない）**．`target/stm32mp257f_dk_arm64_gcc/minimal_boot/Makefile`（実機ブート資材）・`target/zybo_z7_gcc/xilinx_sdk/`（実機JTAG資材）は残置 |
| `doc/*.txt`（11本：user/porting/configurator/design/mutex_design/inherit_design/asp_spec/extension/non_tecs/migration/simtimer） | ドキュメントMarkdown化 | **Markdown版（`docs/spec/`）が正本**（変換は機械突合で全数検証済み・`docs/dev/docs-markdown.md`）．simtimer.txtのみ機能自体の削除（file-cleanup）に伴い変換せず削除．**`doc/version.txt` のみ残置**（リリース毎に必ず更新＝マージ追従の起点）．**上流マージ時は上流 `doc/*.txt` のdiffを `docs/spec/` の対応ファイルへ手動反映**（対応表は `docs/spec/README.md`） |

---

## 変更種別の凡例

- **TECSレス**：上流の `extension/non_tecs/`（syssvc・ホストターゲット等）を採用。ローカル発明ではなく上流拡張
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
