# CLIターゲット

## 項目

CLIターゲット（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

POSIXシミュレーション（`target/linux_gcc`＝CMake対応済み・動作確認済み）を、
**エージェントのbuild→run→testループ用CLI**として完成させる。

### 意義（何が嬉しいか）

RTOSの開発・検証は本来「クロスビルド→実機/QEMUへロード→シリアル出力を
人間が目視」というループであり、AIエージェントにとって**遅く・判定が曖昧**。
CLIターゲットはこれを「ホスト上で数秒・機械判定」に変える：

1. **最速のフィードバックループ**：クロスコンパイラもQEMUも実機も不要。
   `cmake --build build/linux && ./build/linux/asp` が数秒で回る。
   カーネルAPIレベルの変更（syssvc・cfg・テスト追加）の一次検証はここで完結し、
   QEMU・実機は段階検証（AGENTS §4のテスト実行順序 1.POSIX→2.QEMU→3.実機）に回す。
2. **合否の機械判定（TAP）**：テスト結果を「ok N / not ok N」で出力し、
   エージェントやCIが**出力文字列の目視解釈なしに**成否を判定できる。
3. **挙動の機械検証（構造化ログ）**：「動いた/落ちた」だけでなく、
   タスク切替・サービスコール列・エラーコードを `T=<時刻>,EV=<イベント>` で
   出力し、期待イベント列（JSON）との照合で**「正しく動いたか」を自動検証**できる。
   実行時の挙動はカーネルログでしか観測できないRTOSにおいて、これが
   エージェントの「テストを書く→走らせる→挙動で確認する」ループの要になる。
4. **CI整備への土台**：ctest化により GitHub Actions から
   `ctest --preset linux` の1コマンドで回帰検知できる（CI整備項目が直接利用）。

### 入れる機能（まとめ）

| 機能 | 用途 |
|---|---|
| `--tap` | テスト合否のTAP出力（エージェント/CIの機械判定） |
| 構造化ログ（`T=,EV=`・トレースログ機構） | 挙動の記録と期待イベント列照合 |
| `test/porting/expected/` ＋ check_events.py | 「正しく動いたか」の自動検証 |
| ctest統合 | `ctest --preset linux` での一括回帰 |
| testexec.py のPOSIX対応確認 | 既存機能テスト群のホスト実行 |

### 現状と差分（2026-06-06 調査）

AGENTS.md には以下が**先行記述**されているが、出力側・実行側が未実装：

| AGENTS.mdの記述 | 現状 |
|---|---|
| §4 `./build/linux/asp --tap`（TAP形式で判定） | `main()` が引数を受けない・TAP出力なし |
| §8 構造化ログ `T=<tick_us>,EV=<event>` | **出力側が未実装**（syslogは通常テキストのみ） |
| §8 `scripts/parse_slog.py`／`check_events.py` | **スクリプトは実装済み**（入力が来れば動く） |
| §8 `test/porting/expected/sample1.json` | `test/porting/` は空 |
| testPresets `linux`（ctest） | プリセットはあるが `add_test` 未登録＝テスト0件 |

### 実現手段の調査結果

- **構造化ログはトレースログ機構で実現可能（kernel/無改変）**：
  `kernel/kernel_impl.h` は `TOPPERS_ENABLE_TRACE` 定義時に
  `arch/tracelog/trace_log.h` を取り込み、カーネル内の `LOG_*` フック
  （タスク状態変化・ディスパッチ・サービスコール出入り等）が有効になる。
  `arch/tracelog/` は存在するが **実装が TECS 版（`tTraceLog.c`）のまま**
  （TECSレス化の残骸）。非TECS版 `trace_log.c` を新規実装し、
  `T=,EV=` 形式で出力すれば §8 のフローが成立する。
- **テストプログラムの合否出力**は `syssvc/test_svc.c` の
  `check_point()`（`Check point %d passed.`）に集約されている。
  TAP化はこの1箇所の変更で全テストに効く。
- linux_gcc の `main()` は `target_kernel_impl.c`（上流SVN取込み品・EXTENDED）
  にあり、argc/argv 化は **DIVERGENCE_MAP 記載のうえで改変可**。

## 実施プラン

1. **コマンドライン引数の導入**（`target/linux_gcc/target_kernel_impl.c`）
   - `main(int argc, char *argv[])` 化し、簡易パーサで以下をグローバルフラグへ：
     - `--tap`：test_svc の出力をTAP形式に
     - `--slog`：構造化ログ（トレース）を有効化（ビルド時有効が前提．後述）
     - `--help`
   - 上流posix_gccパッケージ由来のため **DIVERGENCE_MAP に追記**

2. **TAP出力**（`syssvc/test_svc.c`）
   - `check_point()`／`check_finish()`／`check_assert()` 失敗時を実行時フラグで切替：
     - 通常：`Check point %d passed.`（従来どおり＝既定）
     - TAP：`ok %d` ／ 失敗 `not ok %d - ...`、`check_finish()` で `1..N` と
       `# All check points passed.`
   - フラグはweakシンボル（`test_tap_mode`）にし、linux_gcc の `--tap` が設定。
     他ターゲットは従来出力のまま（QEMU側の testexec.py 互換を維持）
   - syssvc は EXTENDED（上流 non_tecs 由来）のため変更箇所に
     【asp3_core変更】コメントを付け、DIVERGENCE_MAP の syssvc 行を更新

3. **構造化ログ＝トレースログの非TECS実装**（`arch/tracelog/`・**FMP3ベース**）
   - **FMP3の `arch/tracelog/` 一式をASP3変換して流用**
     （`/home/honda/TOPPERS/fmp3_pfsoc/fmp3_3.3/arch/tracelog/`）：
     - `trace_log.h`（712行・**LOG_\*フック250定義＝全サービスコール網羅**）
     - `trace_log.c`（236行・リングバッファ＋trace_wri_log）
     - `trace_dump.c`（**2,187行・全イベントのデコード表**＝新規に書かずに済む）
   - FMP3→ASP3変換（軽微・prcid依存は計45箇所）：
     - `SIL_LOC_SPN`→`SIL_LOC_INT`・プロセッサID欄/引数の削除・`pcb.h` include削除
     - FMP3専用フック（mact_tsk等のマイグレーション系）のcaseは削除
     - ASP3カーネル側のLOG_\*呼出しと突合（未定義フックはカーネル側の
       `#ifndef`ガードで空になるため安全）
   - **TECS版 `tTraceLog.c` と既存の最小版 `trace_log.h`（13定義）は置換・削除**
   - 出力形式：trace_dump の人間可読出力に加えて **`T=,EV=` 形式の出力関数を追加**
     （`scripts/parse_slog.py` の既存仕様に一致させる）。出力タイミングは
     リングバッファ＋終了時ダンプ（上流流・既定）と逐次出力モードの両対応を検討
   - CMakeオプション `ASP3_ENABLE_TRACE=ON` で `TOPPERS_ENABLE_TRACE` を定義し
     `trace_log.c`／`trace_dump.c` をリンク（既定OFF＝オーバヘッドなし）

4. **期待イベント列の整備**（`test/porting/expected/`）
   - `sample1.json` を作成（起動〜タスク切替の主要列＋forbidden=ERR）
   - `./build/linux/asp --slog | parse_slog.py --json | check_events.py` の
     一連フローを通す

5. **ctest統合**（ルート `CMakeLists.txt`）
   - `enable_testing()`＋linux_gcc ビルド時に機能テストを `add_test` 登録
     （テストアプリのビルドは現状のビルド毎フォルダ方式と整合させるため、
     方式は実装時に検討：①testexec.py をctestから呼ぶ／②テストごとに
     ExternalProject／③sample1+check_events のスモークのみ登録）
   - `ctest --preset linux` で回る状態に（CI整備項目への受け渡し点）

6. **testexec.py のPOSIX動作確認**
   - `TARGET_OPTIONS=--preset linux`・`TARGET_RUN=timeout 20 ./asp` で
     機能テストが回ることを確認（ext_ker での自動終了＝POSIXは exit() 確認）

7. **検証・記録**
   - 回帰：QEMU mps2（test_svc変更の影響確認＝testexec数本）
   - DIVERGENCE_MAP（linux_gcc・syssvc/test_svc.c・arch/tracelog）・
     README索引・本ファイル実施結果・AGENTS §4/§8の記述と実体の一致確認

### スコープ外

- 全カーネルイベントの網羅的トレース（最小セットで開始し必要に応じ拡張）
- JUnit XML出力（CI整備項目で必要になれば追加）
- macOS対応（上流 macos_xcode は削除済み．Linuxのみ）

### リスク・確認事項

- `LOG_*` フックの種類と引数は上流仕様（`doc/`・tracelogサンプル）に従う。
  フック自体はkernel/に既存のため**禁則①に抵触しない**
- test_svc.c のTAP化はQEMU系ターゲットの既存テストフロー
  （testexec.py が `All check points passed.` を目視確認する運用）を壊さないこと
- トレース有効時の出力量（dly_tsk周期で毎回SVC_ENTER等）→ イベント選別を
  ビルド時マクロで調整可能にする

## 実施結果

（2026-06-06 実施・完了）

実施プランどおり、①コマンドライン引数（--tap/--slog/--help）、②TAP出力、
③構造化ログ＝トレースログの非TECS実装（FMP3ベース）、④期待イベント列、
⑤ctest統合、⑥testexec.pyのPOSIX確認、⑦QEMU回帰を実施した。

### 設計上の決定事項（プランからの具体化）

- **構造化ログの出力方式**：トレースモードに `TRACE_SLOG`（0x08）を新設し、
  リングバッファに記録せず `trace_wri_log()` から `trace_slog_print()`
  （`trace_slog.c`・新規）で**逐次出力**する方式とした。POSIXでは
  timeout killでもログが残ることを優先（リングバッファ＋終了時ダンプの
  上流方式は `trace_dump.c` として併存．QEMU/実機はATT_INI/ATT_TERで利用可）。
- **イベント語彙**（`trace_slog.c` 冒頭コメントに一覧）：
  `TSK_STAT`（ID・STAT）／`TSK_RUN`（ディスパッチ先）／`DSP_ENTER`／
  `SVC_ENTER`（API・A1〜A5）／`SVC_LEAVE`（API・RET・V1/V2）／
  `ERR`（RETが-99〜-1のとき追加出力）／`INH_*`・`ISR_*`・`CYC_*`・`ALM_*`・`EXC_*`。
  AGENTS.md §8の例は実装に合わせて更新した。
- **機能コード**：ASP3に存在しないため、FMP3の `kernel_fncode.h` を
  `arch/tracelog/trace_fncode.h` としてトレースログ専用に取込んだ
  （FMP3との差分管理のためFMP3専用コードも残置）。
- **TAPフラグの結線**：`test_svc.c` が `bool_t test_tap_mode = false;` を定義し、
  linux_gccの `main()` が **weak参照**で存在時のみ設定（test_svc.cをリンク
  しないsample1ビルドでもリンク可能）。
- **ctestの登録範囲**：プラン⑤の方式③を採用（sample1スモーク2件：実行確認＋
  slog照合）。linux_gccかつ既定アプリのビルドでのみ登録し、testexec.pyの
  OBJ-*ビルドには影響しない。機能テスト一括はtestexec.pyを使用。

### トレース有効時に顕在化した上流の潜在バグ（2件・上流報告対象）

| 箇所 | 内容 | 対処 |
|---|---|---|
| `kernel/sys_manage.c`（get_lod） | `LOG_GET_LOD_ENTER` に存在しない変数 `p_tskid` を渡す | 上流報告の方針決定に伴い，**ユーザー承認のうえ禁則①の例外としてカーネルを1行修正**（`tskpri` に変更．当初のtrace_log.h側回避マクロは撤去） |
| `arch/posix_gcc/posix_kernel_impl.c`（int_entry） | `LOG_INH_ENTER/LEAVE` に存在しない変数 `inhno` を渡す | EXTENDED層のため `p_my_intrcb->p_inhinib->inhno` に**修正**（【asp3_core変更】コメント付与） |

2件とも上流（TOPPERSプロジェクト）への報告対象。報告文面・修正案diffは
**`upstream-report-tracelog.md`** を参照。上流で修正された場合は上流版を
採用して本リポジトリ側の差分を解消する。

また、mps2_an521・rp2350の依存部に残っていた旧ASP系
`logtrace/trace_config.h` のinclude（トレース有効時にビルド不能）を除去した。

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
| `target/linux_gcc/target_kernel_impl.c` | main()のargc/argv化．--tap/--slog/--help追加（--slogはTRACE_SLOGでtrace_initialize） |
| `syssvc/test_svc.c`・`test_svc.h` | TAP出力モード（test_tap_mode）追加．既定は従来出力 |
| `arch/tracelog/trace_log.h` | TECS版→FMP3ベース非TECS版へ全面置換（LOG_*フック網羅・TRACE_SLOG追加・FMP3専用フック削除） |
| `arch/posix_gcc/posix_kernel_impl.c` | LOG_INH_ENTER/LEAVEの引数修正（上流潜在バグ） |
| `kernel/sys_manage.c` | get_lod()のLOG_GET_LOD_ENTERの引数修正（上流潜在バグ・**PRISTINE領域の1行修正**） |
| `target/mps2_an521_gcc/target_kernel_impl.h`・`arch/arm_m_gcc/rp2350/chip_kernel_impl.h`・`chip_syssvc.h` | 旧`logtrace/trace_config.h` includeの除去 |
| `CMakeLists.txt` | `ASP3_ENABLE_TRACE` オプション（既定OFF）＋ctest登録（sample1.run／sample1.slog_check） |
| `target/linux_gcc/presets.json` | linuxプリセットで `ASP3_ENABLE_TRACE=ON` |
| `scripts/parse_slog.py` | SIGPIPE対策（head等とのパイプ接続用） |
| `.gitignore` | `test/OBJ-*/`・`test/TARGET_OPTIONS`・`test/TARGET_RUN` を追加 |
| `AGENTS.md` | §4（ctest追記）・§8（--slog前提のコマンド・実際のイベント語彙）を実体に一致 |
| `DIVERGENCE_MAP.md` | arch/tracelog・test_svc・linux_gcc/posix_gcc改変・tTraceLog.c削除を記録 |
| `docs/dev/README.md` | 索引の状態更新 |

### 追加したファイル

- `arch/tracelog/trace_log.c` — FMP3 3.3をASP3変換（prcid削除・SIL_LOC_SPN→SIL_LOC_INT・TRACE_SLOG分岐追加）
- `arch/tracelog/trace_dump.c` — FMP3 3.3をASP3変換（プロセッサID欄・マイグレーション系デコード削除）
- `arch/tracelog/trace_slog.c` — 新規．T=,EV=形式の逐次出力（機能コード→API名・引数個数の対応表を内蔵）
- `arch/tracelog/trace_fncode.h` — FMP3 `kernel_fncode.h` のトレースログ専用取込み
- `test/porting/expected/sample1.json` — sample1の期待イベント列（sequence＋forbidden=ERR/EXC_ENTER）
- `docs/dev/upstream-report-tracelog.md` — 上流報告メモ（報告文面・修正案diff・対応状況）

### 削除したファイル

- `arch/tracelog/tTraceLog.c`（TECS版トレースログ．trace_log.cに置換）

### Git情報

- ベースコミット：`8469d30`
- 関連コミット範囲：（コミット時に記載）
- ファイルリスト再現コマンド例：
  `git diff --stat 8469d30 HEAD -- arch/tracelog syssvc/test_svc.c syssvc/test_svc.h target/linux_gcc arch/posix_gcc/posix_kernel_impl.c CMakeLists.txt scripts/parse_slog.py test/porting`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| POSIX（sample1実行・--help・--slog） | ○ | 起動・タスク動作・構造化ログ出力OK |
| POSIX（--slog → parse_slog.py → check_events.py） | ○ | `ok - sample1: sequence matched (10 events)`／`no forbidden events` |
| POSIX（ctest --preset linux） | ○ | 2/2 passed（sample1.run／sample1.slog_check） |
| POSIX（testexec.py --tap：task1/sem1/flg1/dtq1/mutex3/tmevt1） | ○ | 全テスト `1..N`＋`# All check points passed.` |
| QEMU (mps2-an521)（sample1起動・testexec task1/sem1） | ○ | 従来出力（`Check point N passed.`）のまま全合格＝testexec互換維持 |
| QEMU (mps2-an521)（ASP3_ENABLE_TRACE=ONビルド） | ○ | クロスコンパイル・リンク成功（トレースのリングバッファ利用は今後） |
| raspberrypi_pico2（ビルド） | ○ | logtrace include除去後のコンパイル確認 |
| 実機 | − | （対象外．ビルド確認のみ） |

### DIVERGENCE_MAP との関連

- `arch/tracelog/`（全面置換）・`syssvc/test_svc.c`・`target/linux_gcc/target_kernel_impl.c`・`arch/posix_gcc/posix_kernel_impl.c` の各行を追加
- **kernel/（PRISTINE）への変更あり**：`kernel/sys_manage.c` 1行（get_lodの上流バグ修正．ユーザー承認済・上流報告対象＝`upstream-report-tracelog.md`）。DIVERGENCE_MAPに専用行を追加済み

### 残課題（スコープ外・他項目への引き継ぎ）

- CI整備：`ctest --preset linux` をGitHub Actionsに組込み（CI整備項目で実施）
- QEMU/実機ターゲットでのリングバッファモード運用（ATT_INI/ATT_TER登録例の整備）
- --slog出力とsyslogコンソール出力は同一ストリームに混在する（トレース行自体は
  割込みロック下で一行単位に出力されるためparse_slog.pyの解析は破綻しない．
  syslog行は分断されうる）
