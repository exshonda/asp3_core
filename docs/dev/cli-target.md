# CLIターゲット

## 項目

CLIターゲット（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

POSIXシミュレーション（`target/linux_gcc`＝CMake対応済み・動作確認済み）を、
**エージェントのbuild→run→testループ用CLI**として完成させる。

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

（完了時に記載）
