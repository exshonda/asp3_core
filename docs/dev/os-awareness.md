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

（完了時に記載）
