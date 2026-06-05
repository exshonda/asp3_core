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

（2026-06-05 記載．実機確認のみ未了）

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
| `configure.rb` | 非TECSビルドをデフォルト化（`OMIT_TECS`初期設定・共通syssvcオブジェクト自動付与・`banner.o`含む）．TECS構成は引数 `OMIT_TECS=`（空値）で選択 |
| `syssvc/serial.h`・`syssvc/syslog.h` | non_tecs展開による上書き（`TOPPERS_OMIT_TECS`条件ディレクティブ付き，上流ファイル） |
| `sample/sample1.cfg` | 同上（非TECS用INCLUDEに切替わる条件ディレクティブ） |
| `arch/arm_gcc/common/uart_pl011.h`・`arch/arm_gcc/rza1/scif.h`・`arch/simtimer/sim_timer_cntl.h` | 同上（non_tecs展開による上書き） |
| `target/ct11mpcore_gcc/`・`target/dummy_gcc/`・`target/gr_peach_gcc/` | 同上（`target_kernel_impl.c`・`target_syssvc.h`の上書き＋`target_serial.*`追加） |
| `target/mps2_an521_gcc/` | 非TECS化：`target_syssvc.h`（SIO/割込み設定）・`target_kernel_impl.c`（sio初期化・低レベル出力）・`Makefile.target`（OMIT_TECS時オブジェクト追加） |
| `target/zybo_z7_gcc/` | 非TECS化：同上＋`MANIFEST`更新（**上流ZYBOパッケージ由来**） |
| `arch/arm_gcc/zynq7000/xuartps.h`・`MANIFEST` | 非TECSドライバAPI追加 |
| `target/stm32mp257f_dk_arm64_gcc/`・`arch/arm64_gcc/stm32mp2/` | 非TECS化：同上＋`MANIFEST`・`PORTING_ASP3_STM32MP2.md`・`CLAUDE.md`更新 |
| `target/raspberrypi_pico2_gcc/`・`arch/arm_m_gcc/rp2350/` | 非TECS化：`target_syssvc.h`・`Makefile.chip`・`MANIFEST`・`PORTING.md`更新 |
| `target/linux_gcc/Makefile.target` | OMIT_TECS時に`posix_serial.o`追加 |
| `DIVERGENCE_MAP.md`・`.gitignore`・`docs/dev/README.md` | 台帳記録・obj/除外・状態更新 |

### 追加したファイル

- non_tecs展開（上流由来）：`syssvc/{syslog,banner,serial,serial_cfg,logtask,histogram,test_svc}.c`＋各`.cfg/.h`，`arch/posix_gcc/posix_serial.*`・`posix_sigio.*`，`arch/arm_gcc/common/uart_pl011.c`，`arch/arm_gcc/rza1/{chip_serial,scif}.*`，`target/{linux_gcc,macos_xcode}/`（非TECS3点），各target/の`target_serial.*`，`doc/non_tecs.txt`
- 上流SVN取込み（3.7.2）：`arch/posix_gcc/`（カーネル実装部一式），`target/linux_gcc/`（完全版．3.7.2 tarball未収録のため`asp3_3.7.zip`＝同一バージョンのSVN作業コピーから取込み）
- 新規作成（非TECS SIOドライバ）：
  - `target/mps2_an521_gcc/cmsdk_uart.{c,h}`・`target_serial.{c,h,cfg}`
  - `arch/arm_gcc/zynq7000/xuartps.c`，`target/zybo_z7_gcc/target_serial.{c,h,cfg}`
  - `arch/arm64_gcc/stm32mp2/stm32usart.c`，`target/stm32mp257f_dk_arm64_gcc/target_serial.{c,h,cfg}`
  - `arch/arm_m_gcc/rp2350/rp2350_uart.{c,h}`・`chip_serial.{c,h,cfg}`，`target/raspberrypi_pico2_gcc/target_serial.{h,cfg}`

### 削除したファイル

なし（TECS版ファイルは残置．条件ディレクティブで共存．削除は別項目「ファイルの削除」で実施）

### Git情報

- ベースコミット：`d8c0b7e`（Import gen files＝ASP3 3.7.2取込み直後）
- 関連コミット範囲：`6d3604e`〜`2c8f9f0`（docs 1・upstream取込み2・configure.rb 2・ターゲット4）
- ファイルリスト再現コマンド例：`git diff --stat d8c0b7e HEAD -- syssvc arch target configure.rb`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| POSIX（linux_gcc, obj/obj_linux） | ○ | ビルド警告なし・sample1動作（バナー・logtask・task1周期実行） |
| QEMU mps2-an521（mps2_an521_gcc） | ○ | ビルド警告なし・sample1動作・シリアル入力`r`でtask1→2→3切替（SIO割込み経路確認） |
| QEMU xilinx-zynq-a9（zybo_z7_gcc） | ○ | 同上 |
| ビルドのみ（raspberrypi_pico2_gcc） | ○ | arm-none-eabi-gcc 警告ゼロ・configuration check passed |
| コンパイルのみ（stm32mp257f_dk_arm64_gcc） | △ | 全オブジェクト警告ゼロ（aarch64-linux-gnu代用）．最終リンクは aarch64-none-elf 必須のため実機側マシンで実施 |
| 実機（pico2 / STM32MP257F-DK） | − | 未実施．プッシュ後に実機接続機器で確認 |

### DIVERGENCE_MAP との関連

- `configure.rb`・`target/zybo_z7_gcc/`・`arch/arm_gcc/zynq7000/xuartps.[ch]`・各非TECS化・`target/linux_gcc/`＋`arch/posix_gcc/`取込みを `DIVERGENCE_MAP.md` に記録済み
- `syssvc/syslog.c` の行を実態（構造化ログ未実施・上流non_tecs由来）に修正
