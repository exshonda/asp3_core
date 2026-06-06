# ４．ディレクトリ構成・ファイル構成

**原本**: `doc/user.txt` §4  
**最終更新**: 2026-05-18（上流）→ Markdown化: 2026-06-07

> **asp3_core注（本章の位置付け）**: 本章は**上流ASP3配布パッケージ**のディレクトリ・ファイル構成です。asp3_coreでの現在の構成・変更種別（PRISTINE/EXTENDED/DIVERGED/NEW）は `AGENTS.md` §3 を，ファイル単位の上流乖離は `DIVERGENCE_MAP.md` を正本としてください。主な差異は各節の注に示します。

## 4.1 配布パッケージのディレクトリ構成

| ディレクトリ | 内容 |
|---|---|
| `doc/` | ドキュメント |
| `include/` | アプリケーション向けヘッダファイル |
| `kernel/` | カーネルのソースファイル |
| `tecs_kernel/` | TECSからカーネルを呼び出すためのソースファイル |
| `syssvc/` | システムサービスのヘッダファイル，ソースファイル |
| `library/` | サポートライブラリのソースファイル |
| `target/` | ターゲット依存部 |
| `arch/` | ターゲット依存部の共通部分 |
| `arch/gcc/` | GCC開発環境依存部 |
| `arch/tracelog/` | トレースログ記録のサンプルコード |
| `cfg/` | コンフィギュレータ |
| `cfg/doc/` | コンフィギュレータのドキュメント |
| `utils/` | ユーティリティプログラム |
| `sample/` | サンプルプログラムとMakefile |
| `test/` | テストプログラム |
| `test_cfg/` | コンフィギュレータのテスト |
| `extension/` | 拡張パッケージ |

> **asp3_core注**: asp3_coreでの主な差異：
> - `tecs_kernel/` — **削除済み**（TECSレス化）
> - `cfg/` — Python版コンフィギュレータ（`cfg.py`・`pass1.py`・`pass2.py`・`gen_file.py`・`srecord.py`）に置換（DIVERGED）
> - `arch/` — `arm_m_gcc/`（Cortex-M）・`arm64_gcc/`（AArch64）・`riscv_gcc/`（RISC-V）・`posix_gcc/` 等を拡張（EXTENDED）
> - 追加: `cmake/`（ツールチェーンファイル）・`asp3_core.cmake`・`CMakeLists.txt`・`CMakePresets.json`・`scripts/`（ログパーサ等）・`docs/`（Markdownドキュメント）
> - `sample/Makefile`（テンプレート）— 削除済み（CMake化）

## 4.2 ターゲット非依存部のファイル構成

ターゲット非依存部（テストプログラムと拡張パッケージは除く）の各ファイルの概要は次の通り。

### ルートディレクトリ

| ファイル | 概要 |
|---|---|
| `README.txt` | TOPPERS/ASP3カーネルの簡単な紹介 |
| `configure.rb` | コンフィギュレーションスクリプト（GNU開発環境用） |
| `MANIFEST` | 個別パッケージのファイルリスト |

> **asp3_core注**: `configure.rb`・`MANIFEST` は削除済み。ビルドエントリは `CMakeLists.txt`・`asp3_core.cmake`・`CMakePresets.json` です。

### doc/

| ファイル | 概要 |
|---|---|
| `user.txt` | ユーザーズマニュアル |
| `asp_spec.txt` | TOPPERS/ASP3カーネルの仕様概要 |
| `migration.txt` | TOPPERS/ASP3カーネルへのマイグレーションガイド |
| `extension.txt` | 機能拡張・チューニングガイド |
| `porting.txt` | ターゲット依存部 ポーティングガイド |
| `configurator.txt` | コンフィギュレータ仕様 |
| `design.txt` | 設計メモ |
| `mutex_design.txt` | ミューテックス機能の設計 |
| `inherit_design.txt` | 優先度継承ミューテックス機能の設計 |
| `simtimer.txt` | タイマドライバシミュレータを用いたカーネルのテスト手法 |
| `version.txt` | 変更履歴 |

> **asp3_core注**: これらのMarkdown版が本ディレクトリ（`docs/spec/`）に順次整備されます（索引は [README.md](README.md)）。`simtimer.txt` は機能自体の削除に伴い削除予定です。

### include/

| ファイル | 概要 |
|---|---|
| `kernel.h` | ASP3カーネルを使用するための定義 |
| `sil.h` | システムインタフェースレイヤを使用するための定義 |
| `t_stddef.h` | TOPPERS共通ヘッダファイル |
| `itron.h` | ITRON仕様共通規定のデータ型・定数・マクロ |
| `t_syslog.h` | システムログ出力を行うための定義 |
| `t_stdlib.h` | 基本的なライブラリ関数を使用するための定義 |
| `queue.h` | キュー操作ライブラリを使用するための定義 |
| `log_output.h` | システムログのフォーマット出力を使用するための定義 |

### kernel/

| ファイル | 概要 |
|---|---|
| `Makefile.kernel` | カーネルのファイル構成の定義 |
| `kernel_impl.h` | カーネル実装用標準ヘッダファイル |
| `kernel_int.h` | kernel_cfg.c用のヘッダファイル |
| `kernel_rename.def` | カーネルの内部識別名のリネーム定義 |
| `kernel_rename.h` | カーネルの内部識別名のリネーム |
| `kernel_unrename.h` | カーネルの内部識別名のリネーム解除 |
| `kernel_api.def` | コンフィギュレータの静的APIテーブル |
| `kernel_sym.def` | コンフィギュレータの値取得シンボルテーブル |
| `kernel.trb` | コンフィギュレータのパス2の生成スクリプト |
| `kernel_check.trb` | コンフィギュレータのパス3の生成スクリプト |
| `genoffset.trb` | オフセットファイル生成用の生成スクリプトのターゲット非依存部 |
| `allfunc.h` | すべての関数をコンパイルするための定義 |
| `check.h` | エラーチェック用マクロ |
| `startup.c` | カーネルの初期化と終了処理 |
| `task.h` / `task.c` / `task.trb` | タスク管理モジュール（定義／本体／生成スクリプト） |
| `taskhook.h` / `taskhook.c` | タスク管理に関連するフックルーチン |
| `wait.h` / `wait.c` | 待ち状態管理モジュール |
| `time_event.h` / `time_event.c` | タイムイベント管理モジュール |
| `task_manage.c` | タスク管理機能 |
| `task_refer.c` | タスクの状態参照機能 |
| `task_sync.c` | タスク付属同期機能 |
| `task_term.c` | タスク終了機能 |
| `semaphore.h` / `semaphore.c` / `semaphore.trb` | セマフォ機能 |
| `eventflag.h` / `eventflag.c` / `eventflag.trb` | イベントフラグ機能 |
| `dataqueue.h` / `dataqueue.c` / `dataqueue.trb` | データキュー機能 |
| `pridataq.h` / `pridataq.c` / `pridataq.trb` | 優先度データキュー機能 |
| `mutex.h` / `mutex.c` / `mutex.trb` | ミューテックス機能 |
| `mempfix.h` / `mempfix.c` / `mempfix.trb` | 固定長メモリプール機能 |
| `time_manage.c` | システム時刻管理機能 |
| `cyclic.h` / `cyclic.c` / `cyclic.trb` | 周期通知機能 |
| `alarm.h` / `alarm.c` / `alarm.trb` | アラーム通知機能 |
| `sys_manage.c` | システム状態管理機能 |
| `interrupt.h` / `interrupt.c` / `interrupt.trb` | 割込み管理機能 |
| `exception.h` / `exception.c` / `exception.trb` | CPU例外管理機能 |

> **asp3_core注**: asp3_coreでは生成スクリプト `.trb`（Ruby）は **`.py`（Python）に置換**されています（`task.py`・`kernel.py`・`kernel_check.py`・`genoffset.py` 等）。`Makefile.kernel` は削除済み（CMake化）。`.c`/`.h`/`.def` ファイルは上流そのまま（PRISTINE・編集禁止）です。

### tecs_kernel/

| ファイル | 概要 |
|---|---|
| `kernel.cdl` | カーネルオブジェクトのコンポーネント記述ファイル |
| `tecs_kernel.h` | カーネルオブジェクトのコンポーネント化のためのヘッダファイル |
| `init_tecs.c` | TECSの初期化処理 |
| `tKernel_inline.h` | カーネル操作のインライン関数 |
| `tTask.c` / `tTask_inline.h` | タスク操作（本体／インライン関数） |
| `tSemaphore_inline.h` | セマフォ操作のインライン関数 |
| `tEventflag_inline.h` | イベントフラグ操作のインライン関数 |
| `tDataqueue_inline.h` | データキュー操作のインライン関数 |
| `tPriorityDataqueue_inline.h` | 優先度データキュー操作のインライン関数 |
| `tMutex_inline.h` | ミューテックス操作のインライン関数 |
| `tFixedSizeMemoryPool_inline.h` | 固定長メモリプール操作のインライン関数 |
| `tCyclicNotifier.c` / `tCyclicNotifier_inline.h` | 周期通知操作 |
| `tAlarmNotifier.c` / `tAlarmNotifier_inline.h` | アラーム通知操作 |
| `tInterruptRequest_inline.h` | 割込み要求ライン操作のインライン関数 |
| `tISR.c` | 割込みサービスルーチン操作 |
| `tInitializeRoutine.c` | 初期化ルーチン操作 |
| `tTerminateRoutine.c` | 終了処理ルーチン操作 |

> **asp3_core注**: `tecs_kernel/` はディレクトリごと**削除済み**（TECSレス化。`docs/dev/tecs-less.md`）。

### syssvc/

| ファイル | 概要 |
|---|---|
| `syslog.h` | システムログ機能を使用するための定義 |
| `tSysLog.cdl` / `tSysLog.c` | システムログ機能（コンポーネント記述／本体） |
| `tSysLogAdapter.cdl` / `tSysLogAdapter.c` | システムログ機能のアダプタ |
| `serial.h` | シリアルインタフェースドライバを使用するための定義 |
| `tSerialPort.cdl` / `tSerialPortMain.c` | シリアルインタフェースドライバ |
| `tSerialAdapter.cdl` / `tSerialAdapter.c` | シリアルインタフェースドライバのアダプタ |
| `tLogTask.cdl` / `tLogTaskMain.c` | システムログタスク |
| `test_svc.h` | テストプログラム用サービスを使用するための定義 |
| `tTestService.cdl` / `tTestService.c` | テストプログラム用サービス |
| `tTestServiceAdapter.cdl` / `tTestServiceAdapter.c` | テストプログラム用サービスのアダプタ |
| `histogram.h` | 実行時間分布集計サービスを使用するための定義 |
| `tHistogram.cdl` / `tHistogram.h` / `tHistogram.c` | 実行時間分布集計サービス |
| `tHistogramAdapter.cdl` / `tHistogramAdapter.c` | 実行時間分布集計サービスのアダプタ |
| `tPutLogSIOPort.cdl` / `tPutLogSIOPort.c` | 簡易SIOドライバによるシステムログの低レベル出力 |
| `tBanner.cdl` / `tBannerMain.c` | カーネル起動メッセージ出力 |

> **asp3_core注**: asp3_coreの `syssvc/` は**非TECS版**（上流 `extension/non_tecs/syssvc` 由来・プレーンC）に置換済みです。現在の構成：
>
> | ファイル | 概要 |
> |---|---|
> | `syslog.{h,c,cfg}` | システムログ機能 |
> | `serial.{h,c,cfg}` / `serial_cfg.c` | シリアルインタフェースドライバ |
> | `logtask.{h,c,cfg}` | システムログタスク |
> | `banner.{h,c,cfg}` | カーネル起動メッセージ出力 |
> | `histogram.{h,c}` | 実行時間分布集計サービス |
> | `test_svc.{h,c}` | テストプログラム用サービス |
>
> `.cdl`（コンポーネント記述）は `.cfg`（コンフィギュレーションファイル）に置き換わっています。使用方法は [08_system_services.md](08_system_services.md) の各asp3_core注を参照。

### library/

| ファイル | 概要 |
|---|---|
| `log_output.c` | システムログのフォーマット出力 |
| `strerror.c` | エラーメッセージ文字列を返す関数 |
| `t_perror.c` | エラーメッセージの出力 |
| `vasyslog.c` | 可変数引数のシステムログライブラリ |

> **asp3_core注**: `library/` は上流そのまま（PRISTINE）です。

### arch/gcc/・arch/tracelog/

| ファイル | 概要 |
|---|---|
| `arch/gcc/tool_stddef.h` | t_stddef.hの開発環境依存部（GCC用） |
| `arch/tracelog/trace_log.h` | トレースログ機能のヘッダファイル |
| `arch/tracelog/tTraceLog.cdl` / `tTraceLog.c` | トレースログ機能 |

> **asp3_core注**: `arch/tracelog/` は構造化ログ（slog）対応版に変更されています（`trace_slog.c`。フォーマットは `AGENTS.md` §8）。TECSコンポーネント記述（`.cdl`）は削除済み。

### cfg/

| ファイル | 概要 |
|---|---|
| `MANIFEST` | 個別パッケージのファイルリスト |
| `cfg.rb` | コンフィギュレータ本体 |
| `pass1.rb` | コンフィギュレータのパス1の処理 |
| `pass2.rb` | コンフィギュレータのパス2の処理 |
| `GenFile.rb` | GenFileクラスの定義 |
| `SRecord.rb` | SRecordクラスの定義 |
| `cfg/doc/cfg_user.txt` | コンフィギュレータ ユーザーズマニュアル |

> **asp3_core注**: Python版に置換済み（DIVERGED）：`cfg.py`・`pass1.py`・`pass2.py`・`gen_file.py`・`srecord.py`。上流cfg.rbとの仕様対応は `docs/asp3_derivative_plan.md` の CFG_SPEC_MAP節 で管理しています。

### utils/

| ファイル | 概要 |
|---|---|
| `applyrename.rb` | ファイルにリネームを適用 |
| `genrename.rb` | リネームヘッダファイルの生成 |
| `gentest.rb` | テストプログラムの生成 |
| `makerelease.rb` | 配布パッケージの生成 |

> **asp3_core注**: Python版に置換済み：`applyrename.py`・`genrename.py`・`gentest.py`。`makerelease.rb` は削除済み（tarball配布を行わないため）。

### sample/

| ファイル | 概要 |
|---|---|
| `Makefile` | 標準のMakefile（GNU開発環境用）のテンプレート |
| `sample1.h` / `sample1.c` / `sample1.cfg` / `sample1.cdl` | サンプルプログラム(1) |
| `tSample2.h` / `tSample2.c` / `tSample2.cfg` / `tSample2.cdl` | サンプルプログラム(2) |

> **asp3_core注**: 現在は `sample1.{h,c,cfg}` のみ。`Makefile`テンプレート・`.cdl`・TECS版サンプル `tSample2` は削除済みです。

---

**原本との対応確認**:
- 章立て: ✅ 4.1〜4.2 完全網羅
- ファイルリスト: ✅ 原本の全ファイル・説明を表形式で保持
- asp3_core注: ✅ 現リポジトリとの差分（削除・置換・追加）を各節に付記
