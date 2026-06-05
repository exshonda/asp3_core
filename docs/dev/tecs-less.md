# TECSレス

## 項目

TECSレス（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

システムサービス（syssvc）をプレーンCで実装し、tecsgen への依存を除去する。
コンポーネント記述（.cdl）とアダプタ層の間接構造が消えることで、AIがコードを追いやすくなる。

### 方針

- 上流ASP3の **`extension/non_tecs/` 拡張**（プレーンC版 syssvc / arch / target / sample）を採用する。
  上流由来のため、上流マージ時も extension 側の変更に追従すればよい。

### 現状

- `syssvc/` は**TECS版のまま**：t接頭辞のコンポーネント実装（`tSysLog.c`・`tSerialPortMain.c`・`tLogTaskMain.c` 等）と `.cdl` ファイルで構成。
- 上流のプレーンC版は `extension/non_tecs/syssvc/` に存在：
  `syslog.c` / `serial.c`・`serial_cfg.c` / `logtask.c` / `banner.c` / `histogram.c` / `test_svc.c` ＋ 各 `.cfg`。
- ルートに TECS 依存の本体である `tecsgen/`（ジェネレータ）・`tecs_kernel/`（TECS用カーネルヘッダ）が存在。

### 関連記述

- `docs/asp3_derivative_plan.md` §3：「syssvcをプレーンCで実装。tecsgenへの依存を除去。AI向けにコードの間接層が消え追いやすくなる」
- `docs/asp3_derivative_plan.md` §8.1：`syssvc/serial.c` の TECS-less化（DIVERGENCE_MAP 記載例）
- `docs/OVERVIEW.md`：「TECSレス版システムサービス（上流 `extension/non_tecs/syssvc` 由来。ローカル発明ではなく上流拡張を利用）」

## 実施プラン

- extension/non_tecs にTECSレスに対応したファイルがあるのでそれらのファイルを各ディレクトリにコピーする(同じファイルがあれば上書き)．
  - TECS版ファイル（`syssvc/t*.c`・`.cdl`，`tecsgen/`，`tecs_kernel/` 等）は残置する（条件ディレクティブで共存可能）．削除は別項目「ファイルの削除」で実施する．
- configure.rb を変更して，非TECSビルドをデフォルトとする．
  - `-w` 相当（TECSジェネレータをスキップし `TOPPERS_OMIT_TECS` を定義）と `-S` 相当（非TECS版syssvcオブジェクト `syslog.o banner.o serial.o logtask.o`＋ターゲットSIO の追加）をデフォルト化する．
  - configure.rb は上流由来ファイルのため，変更を `DIVERGENCE_MAP.md` に記録する．
- すでに上記でサポートされるlinux_gccについて，./obj/obj_linux でビルドして動作するか確認する
- 次のサポートされていないターゲットについてコードを変更して非TECS版とする
  - mps2_an521_gcc
  - zybo_z7_gcc
  - stm32mp257f_dk_arm64_gcc
  - raspberrypi_pico2_gcc
  - 注意：上流の非TECS対応archは `arm_gcc`/`posix_gcc`/`simtimer` のみ．mps2_an521・pico2（`arch/arm_m_gcc`）と stm32mp257f（`arch/arm64_gcc`）は **arch側のSIO関連も新規作成が必要**．zybo_z7 は `arch/arm_gcc` が対応済みのため target依存部のみで済む見込み．
- それぞれ ./obj/obj_xxx でビルドできることを確認する
- QEMUで実行できるターゲット（mps2_an521_gcc/zybo_z7_gcc）は，QEMUで実行可能か確認する．
- 非TECS化で変更した上流由来ファイル（4ターゲットの target/・arch/ 配下）を `DIVERGENCE_MAP.md` に記録する（上流マージ時の衝突対策）．
- QEMUまでの確認が終わったらプッシュし，実機（raspberrypi_pico2_gcc / stm32mp257f_dk_arm64_gcc）が接続されている機器で動作確認する．


## 実施結果

（完了時に記載）
