# OS Awareness 対応

## 項目

OS Awareness 対応（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

gdb の Python 機構でカーネル状態（タスク・同期/通信オブジェクト・時間イベント・
割込み）を可視化する **OS Awareness を全ターゲットで使用可能**にする。

### 意義（何が嬉しいか）

1. **「printf デバッグの壁」の突破**：RTOS の不具合（デッドロック・優先度逆転・
   スタックオーバーフロー・取りこぼし）は、停止させた瞬間の**カーネル内部状態**
   （レディキュー・待ちキュー・スタック使用量・GIC/PLIC のペンディング）を見るのが
   最短。`atask`/`sem`/`intr` 等の1コマンドで全タスク・全オブジェクトを一覧できる。
2. **エージェントのデバッグループ強化**：QEMU は gdbserver 内蔵（`-s -S`）。
   「QEMUで再現→halt→`atask`で状態取得→仮説→修正」のループをエージェントが
   テキストベースで回せる。構造化ログ（事象の時系列）と相補的な「断面の観測」。
3. **全ターゲット共通のデバッグ体験**：現在は stm32（実機SWD）でのみ使える。
   QEMU 4機種＋POSIX＋pico2 で同じコマンド群が使えれば、ターゲット間の
   挙動比較・移植デバッグ（新ターゲット追加時の定番手順）が揃う。

### 現状（2026-06-06 調査）

FMP3版をASP3に移植した実装が **stm32 ターゲット配下**にある（実機SWDで検証済み）：

| ファイル | 場所 | 役割 |
|---|---|---|
| `gdb_os_aware/os_awareness.py`（839行） | `target/stm32mp257f_dk_arm64_gcc/` | **汎用エンジン**（カーネルシンボル＋kernel_cfg.h解析。コマンド：stask/atask/sem/dtq/pdq/flg/mtx/mpf/cyc/alm/intr） |
| `gdb_os_aware/README.md` | 同上 | 使い方・設計メモ |
| `target_os_awareness.py` | 同上 | ボード層（chipの再エクスポート） |
| `chip_os_awareness.py` | `arch/arm64_gcc/stm32mp2/` | SoC層（GICDベースアドレス） |
| `core_os_awareness.py` | `arch/arm64_gcc/common/` | アーキ層（GICv2レジスタ読出し・inh_table解決） |

**問題点**：
- 汎用エンジンがターゲット依存部に置かれている（**場所が不適切**・他ターゲットから
  使うとstm32配下をsourceすることになる）
- エンジンに **arm64 前提が混入**：`ready_primap` のビット方向
  （arm/arm64は `0x8000>>pri` のMSB詰め、**arm_m/riscvはデフォルトの `1<<pri`**）
- ターゲット層の探索がエンジンと同居前提（`import target_os_awareness`）
- kernel_cfg.h の探索パスが旧構成（ELF同梱/cwd/gen）で、CMakeの
  `build/<preset>/generated/` を見ない
- 割込み状態の層（core/chip/target_os_awareness.py）が arm64+GICv2 のみ

## 実施プラン

1. **エンジンの移動**（ユーザー指示：適切な場所へ）
   - `target/stm32mp257f_dk_arm64_gcc/gdb_os_aware/` →
     **`scripts/gdb_os_aware/`**（AGENTS §3：scripts/=自動化スクリプト[NEW]。
     ホスト側gdbツールでありカーネル/ターゲットのビルド物ではないため）
   - stm32側の参照を更新：`run.cmake`（AWARENESS変数）・`target_user.md`・
     `CLAUDE.md`
2. **エンジンのターゲット非依存化**
   - **primap方向のアーキ抽象**：`core_os_awareness.py` に
     `primap_bit(pri)` を持たせ、エンジンはアーキ層から取得
     （未提供時は両方向を試しタスク状態と整合する方を採用、等のフォールバック）
   - **ターゲット層の自動発見**：ELFと同じビルドフォルダの `CMakeCache.txt` から
     `ASP3_TARGET` を読み、`target/<ASP3_TARGET>/target_os_awareness.py` を
     sys.path 解決（明示指定用に gdb コマンド/環境変数も用意）
   - kernel_cfg.h 探索に `build/<preset>/generated/` を追加
   - 32bit ターゲット（arm_m/arm/riscv32は将来）でのポインタ幅・型幅の確認
3. **各アーキ/ターゲットの割込み層を追加**（stm32の3層構造を踏襲）
   - `arch/arm64_gcc/zynqmp/chip_os_awareness.py`（GICD=0xF9010000）＋
     `target/zcu102_arm64_gcc/target_os_awareness.py`
   - `arch/arm_m_gcc/common/core_os_awareness.py`（**NVIC**：ISER/ISPR・
     inh_table）＋ mps2／pico2（rp2350）の chip/target 層
   - `arch/arm_gcc/common/core_os_awareness.py`（GICv2＝arm64版の流用）＋
     zynq7000 chip層（GICD=0xF8F01000）＋ zybo target層
   - `arch/riscv_gcc/common/core_os_awareness.py`（**PLIC**：enable/pending・
     inh_table）＋ polarfire chip/target 層
   - linux_gcc：割込み層なし（`intr` は登録情報のみ表示に劣化）。
     タスク等のコマンドは**ネイティブgdbでそのまま動く**ことを確認
4. **QEMUでの接続手段の整備**
   - 各QEMUターゲットの `target.cmake`/`run.cmake` に **`osdebug`** ターゲットを
     追加（QEMU `-s -S` 起動＋`gdb-multiarch -ex "target remote :1234"
     -ex "source scripts/gdb_os_aware/os_awareness.py"`。stm32のosdebugと同名・
     同体験に）
   - linux は `gdb ./asp -ex source ...` の手順をREADMEに記載
5. **検証**（各ターゲットで halt → コマンド確認）
   - POSIX／QEMU 4機種：sample1 を halt し `stask`/`atask`/`sem`/`cyc`/`intr` の
     出力確認（レディキュー・待ちタスク・スタック使用量・割込みena/pend）
   - stm32実機：回帰（移動後のosdebugが従来どおり動くこと）→ 実機側PCで実施
   - pico2実機：osdebug追加の確認は実機側PCで実施（スコープ：層の実装まで）
6. **記録**：IMPL_INDEX（OS Awareness行）・README索引・docs/building.md
   （osdebugの表へ追記）・stm32 README.mdの「適用範囲」更新・本ファイル実施結果

### スコープ外

- gdbスクリプトの機能拡張（新コマンド追加・トレースバッファ表示等）
- VS Code デバッグ統合（launch.json）
- FMP3側への逆移植

### リスク・確認事項

- **gdb-multiarch のアーキ網羅**：arm/aarch64/riscv64 すべて対応（Ubuntu版で
  確認済みの組合せは aarch64。arm_m/riscv は要確認）
- QEMUのgdbserver経由で**メモリマップドレジスタ（NVIC/GIC/PLIC）が読めるか**
  はマシン依存（読めない場合は intr の ena/pend 列を省く既存の劣化動作）
- arm_m のNVIC読出しはセキュア/非セキュアエイリアス（mps2はSecure動作）に注意
- エンジンの arm64前提が他にもないか（移植時にコメント・定数を総点検）

## 実施結果

（2026-06-06 実施。ブランチ `feat/os-awareness`）

プランどおり実施。プランからの主な差分・追加判明事項：

1. **TINIB のスタック表現のアーキ差**（プランの「ポインタ幅・型幅の確認」で発覚）：
   標準（`stk`/`stksz`）のほか、arm_m は `tskinictxb.stk_top/stk_bottom`、
   posix は `tskinictxb.stksz` のみ。エンジンの `_tinib_stack()` が3形態を吸収。
2. **CB 配列シンボルの不完全型問題**：halt 位置のコンパイル単位によっては
   `_kernel_semcb_table` 等が extern 宣言（`SEMCB []`）に解決され `sizeof` が失敗
   →待ちオブジェクト逆引き・待ちキュー表示が静かに劣化していた。要素数は
   `_kernel_tmax_<obj>id` を正とし、配列は `gdb.lookup_global_symbol` で定義側を
   引く方式に変更（既存 stm32 でも起き得た潜在バグの修正）。
3. **TA_ACT 誤値の修正**：エンジンが FMP3 由来の `TA_ACT=0x02` を使っており
   ASP3（`include/kernel.h`）の `0x01` と不一致。stask の attr 列が常に `-` に
   なっていたのを修正。
4. **zcu102×QEMU 8.2 の不具合**：gdbstub 経由の GIC 領域（0xF9010000）読出しで
   **QEMU 自体が segfault**（UART 領域は読める。QEMU 11 の zynq では発生せず）。
   ターゲット層で GICD 読出しを既定無効とし、`ASP3_OSA_GICREAD=1` でオプトイン。
5. **polarfire（icicle-kit）の gdb 接続**：マルチクラスタ構成（E51＋U54）のため
   既定の `gdb asp.elf -ex "target remote :1234"` は flen 不一致
   （E51 に FPU 無し）で失敗。`target extended-remote` →`add-inferior`→
   `inferior 2`→`attach 2`→`file` の手順が必要で、`ASP3_OSDEBUG_GDB_CONNECT`
   変数（target.cmake で上書き）として osdebug に組み込んだ。
6. **posix の intr**：INTINIB が無いため、INHINIB 表（`_kernel_inhinib_table`）
   から登録情報のみ表示するフォールバックを追加（プランの「劣化」より一歩改善）。
7. **osdebug の実装位置**：各 target.cmake への個別追加ではなく、root
   CMakeLists.txt で `ASP3_RUN_COMMAND` が qemu-system 系の場合に汎用定義
   （将来の QEMU ターゲットにも自動適用）。

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
| `scripts/gdb_os_aware/os_awareness.py` | （移動元 `target/stm32mp257f_dk_arm64_gcc/gdb_os_aware/`）ターゲット非依存化：依存部自動発見（CMakeCache.txt／env／osa-target コマンド）・primap_bit 抽象＋キュー走査フォールバック・kernel_cfg.h 探索に `generated/` 追加・TINIB 3形態対応・CB 要素数を tmax 基準に・lookup_global_symbol 化・TA_ACT=0x01 修正・posix 用 INHINIB フォールバック |
| `scripts/gdb_os_aware/README.md` | 全ターゲット共通版に全面改訂（層構造・公開API・発見手順・既知の制約） |
| `CMakeLists.txt` | QEMU ターゲット汎用の `osdebug` 追加（`ASP3_OSDEBUG_GDB_CONNECT` で接続手順差し替え可） |
| `arch/arm64_gcc/common/core_os_awareness.py` | `primap_bit`（0x8000>>pri）追加 |
| `arch/arm64_gcc/stm32mp2/chip_os_awareness.py` | `primap_bit` 再エクスポート |
| `target/stm32mp257f_dk_arm64_gcc/target_os_awareness.py` | 同上 |
| `target/stm32mp257f_dk_arm64_gcc/run.cmake` | AWARENESS パスを `scripts/gdb_os_aware/` へ |
| `target/stm32mp257f_dk_arm64_gcc/target_user.md`・`CLAUDE.md` | エンジン移動の参照更新 |
| `target/polarfire_soc_kit_gcc/target.cmake` | `ASP3_OSDEBUG_GDB_CONNECT`（U54 クラスタへの attach 手順） |
| `docs/building.md` | osdebug（QEMU）の節を追加 |
| `docs/porting/IMPL_INDEX.md` | 「デバッグ支援（gdb OS-awareness）」節を追加 |
| `docs/dev/README.md` | 索引の状態更新 |

### 追加したファイル

- `arch/arm64_gcc/zynqmp/chip_os_awareness.py`（GICD=0xF9010000）
- `arch/arm_gcc/common/core_os_awareness.py`（GICv2・primap=MSB詰め）
- `arch/arm_gcc/zynq7000/chip_os_awareness.py`（GICD=0xF8F01000）
- `arch/arm_m_gcc/common/core_os_awareness.py`（NVIC ISER/ISPR・SysTick・`_kernel_exc_tbl`・primap=LSB）
- `arch/arm_m_gcc/rp2350/chip_os_awareness.py`（core パススルー）
- `arch/riscv_gcc/common/core_os_awareness.py`（PLIC IEM/IPEND・`_kernel_inh_table`・primap=LSB）
- `arch/riscv_gcc/polarfire_soc/chip_os_awareness.py`（PLIC_BASE=0x0C000000・cidx=1）
- `target/{zcu102_arm64,zybo_z7,mps2_an521,raspberrypi_pico2,polarfire_soc_kit}_gcc/target_os_awareness.py`

### 削除したファイル

- `target/stm32mp257f_dk_arm64_gcc/gdb_os_aware/`（`scripts/gdb_os_aware/` へ移動）

### Git情報

- ベースコミット：`a5f2a40`（docs(dev): add OS Awareness 対応 plan）
- 関連コミット範囲：`feat/os-awareness` ブランチ（`git log a5f2a40..feat/os-awareness`）
- ファイルリスト再現コマンド例：
  `git diff --stat a5f2a40 feat/os-awareness -- scripts/gdb_os_aware arch target CMakeLists.txt docs`

### 検証結果

各ターゲットで sample1 を halt（`b task` → continue）し、
`stask`/`atask`/`sem`/`cyc`/`intr` の出力を確認：

| ターゲット | 実施 | 結果 |
|---|---|---|
| POSIX（linux, ネイティブgdb） | ○ | タスク状態・レディキュー（LSB）・待ちキュー・SEM/CYC OK。stk番地なし（仕様）。intr は INHINIB フォールバックで ISR 名解決まで OK |
| QEMU mps2-an521（M33） | ○ | 全コマンド OK。NVIC ena/pend・SysTick(15)・IRQ58 の ISR 解決 OK。osdebug 実行確認 |
| QEMU xlnx-zcu102（A53） | ○ | タスク系 OK（primap MSB 解決）。GIC ena/pend は QEMU 8.2 不具合のため既定無効（劣化表示を確認・`ASP3_OSA_GICREAD=1` で有効化可） |
| QEMU xilinx-zynq-a9（zybo, A9） | ○ | 全コマンド OK（QEMU 11）。GICD ena/pend 取得 OK。osdebug 実行確認 |
| QEMU microchip-icicle-kit（RV64GC） | ○ | ubuntu:26.04 コンテナ（CI 同等、QEMU 10.2）で全コマンド OK。PLIC ena 取得 OK。inferior 2 attach 手順で接続 |
| stm32 実機（移動後の osdebug 回帰） | ○ | 実機接続PCで実施（2026-06-06）。atask/stask/intr 全コマンドOK（GICのena/pend動的取得・halt中のタイマpend表示も正） |
| pico2 実機 | − | ARM側はスコープ＝層の実装まで（SWD用osdebug未定義）。**RISC-V側（pico2-riscv）はXh3irq層＋osdebugを実装し実機確認済み**（`docs/dev/pico2-riscv.md`） |

回帰：`ctest --preset linux` 2/2 PASS。
linux／QEMU 4 プリセット＋pico2 のビルド（`cmake --build`）通過。

### DIVERGENCE_MAP との関連

なし（PRISTINE 領域への変更なし。kernel/・include/・library/ は未変更）。
