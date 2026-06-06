# QEMUターゲット(RISC-V)

## 項目

QEMUターゲット(RISC-V)（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

RISC-V（RV64GC）のQEMUターゲットを追加し、RISC-V系のカーネル検証を
ハードなしで行えるようにする。**asp3_core 初の RISC-V アーキテクチャ対応**
（`arch/riscv_gcc/` は新規系列）。

### 方針

- **移植元はマルチコア版 TOPPERS/FMP3 の PolarFire SoC 移植**：
  `fmp3_polafire_soc_kit_gcc-20241224.zip`（fmp3_3.3ベース．
  **開発機に展開済み：`/home/honda/TOPPERS/fmp3_pfsoc/fmp3_3.3/`**）
  - `arch/riscv_gcc/common/`（コア依存部：start.S・core_support.S・PLIC・mtimer）
  - `arch/riscv_gcc/polarfire_soc/`（チップ依存部：MMUART・chip_serial＝**元から非TECS**）
  - `target/polarfire_soc_kit_gcc/`（Microchip PolarFire SoC Icicle Kit）
- **QEMUマシンは `microchip-icicle-kit`**（QEMU 11.0で確認．開発機にriscv64版を
  ローカルビルド済み：`~/qemu/qemu-11.0.0/build-riscv/qemu-system-riscv64`）
- FMP3→ASP3変換は STM32MP2/ZCU102 で確立した規則
  （`arch/arm64_gcc/stm32mp2/PORTING_ASP3_STM32MP2.md`）を適用する。
  **ただし arm64 と異なりコア依存部の変換から行う**（riscv は初変換）。

### 検討結果（2026-06-06）

#### QEMU microchip-icicle-kit の適合性 → **サポート可能**

| 項目 | FMP3の前提 | QEMU 11.0 のモデル | 適合 |
|---|---|---|---|
| CLINT（MSIP/MTIMECMP/MTIME） | 0x02000000/0x02004000/0x0200BFF8 | 0x02000000（同配置） | ✓ |
| PLIC | 0x0C000000・182割込み | 0x0C000000 | ✓ |
| MMUART0（コンソール） | 0x20000000・INTNO 90 | 0x20000000 → serial0（stdio） | ✓ |
| mtime周波数 | 1MHz（µs直結） | `clint-timebase-freq` 既定 **1MHz** | ✓ |
| DDR | 0x80000000（低位1GB） | DRAM_LO 0x80000000+1GB | ✓ |
| L2-LIM | 0x08000000（FMP3実機はここで実行） | 0x08000000+32MB | ✓ |
| セミホスティング | −（テスト終了用に使用予定） | RISC-V semihosting対応 | ✓ |

#### ブートフロー（QEMUソース hw/riscv/microchip_pfsoc.c で確認）

- `-bios none -kernel asp.elf` で起動可能：QEMUがeNVM（0x20220000）に
  **リセットベクタROMを生成し，全ハートがELFエントリへジャンプ**する。
- ハート構成は **E51×1（hart0・RV64IMAC）＋ U54×4（hart1-4・RV64GC）固定**。
  FMP3実機ではSDK(HSS)が e51()/u54_*() に振り分けるが（sdk_entry.c），
  QEMUでは**全5ハートが同一エントリに来る**。
- **⚠️ E51はFPU無し**：FMP3のstart.SはFPU初期化（fscsr）をハート判定より
  前に行うため，E51が実行すると不正命令例外になる。ASP3版start.Sでは
  **mhartidによるパーキング（hart1以外をwfiループへ）を最初に行う**こと。

#### FMP3→ASP3 変換規模（約3,500行＋trb変換）

| 層 | 主なファイル | 変換内容 |
|---|---|---|
| core（新規変換） | start.S(228)・core_support.S(1263)・core_kernel_impl.c(244)・plic_kernel_impl.c(166)・mtimer.c(109) | TPCB/my_pcb廃止・IPI/migrate削除・`msi_ipi.c`削除・3.7.2 TMAX_*規約・`sil_get_pid`削除（mtimer.cのprcid引数除去）・ハートパーキング追加 |
| 生成テンプレート | core_kernel.trb(209)・core_check.trb(92)・core_offset.trb(33)・plic_kernel.trb(79)・chip/target trb | **Ruby→Python変換**（arm64の`*.py`を規範に） |
| chip | chip_kernel_impl.c(119)・chip_serial.c(167)・mmuart.c(347)・chip_support.S(345) | ほぼ流用（chip_serialは元から非TECS）．CLASS解除 |
| target | target_kernel_impl.c等 | ASP3形へ（zcu102の構成を規範に）．**sdk/（3.8MB・HSS/HAL）は持ち込まない**（QEMUでは不要．実機SoftConsole資材は実機対応時に判断） |

#### 環境

- RISC-Vツールチェーン：開発機に未導入。**`gcc-riscv64-unknown-elf`（apt）の
  インストールが必要**（要sudo）
- QEMU：`qemu-system-riscv64` 未導入のため QEMU 11.0.0 ソースから
  riscv64-softmmu をローカルビルド済み（上記パス）

#### 工数見積

ARMv8-A（ZCU102）は検証済みコア層の流用で約1時間だったのに対し，
RISC-Vは**コア層のFMP3→ASP3変換から**のため1〜2日規模。
変換規則自体は確立済みで，リスクは初適用のデバッグ
（コンテキストスイッチ・PLIC優先度マスク・例外入口）。

## 実施プラン

> 再開時の参照物：移植元＝`/home/honda/TOPPERS/fmp3_pfsoc/fmp3_3.3/`，
> QEMU＝`/home/honda/qemu/qemu-11.0.0/build-riscv/qemu-system-riscv64`，
> 変換規則＝`arch/arm64_gcc/stm32mp2/PORTING_ASP3_STM32MP2.md`，
> ターゲット構成の規範＝`arch/arm64_gcc/zynqmp/`＋`target/zcu102_arm64_gcc/`
> （chip.cmake・presets.json・QEMU/実機切替オプションの形）

1. **環境**：`sudo apt-get install -y gcc-riscv64-unknown-elf`（ユーザー実施）
2. **コア依存部 `arch/riscv_gcc/common/`**：FMP3→ASP3変換
   - start.S：ハートパーキング（mhartid≠1をwfi）→FPU初期化の順に修正．
     マスタ同期（start_sync/slave_wait）削除
   - core_support.S：my_pcb/PCBオフセット→グローバル直接参照，
     dispatch_and_migrate/IPI分岐削除，ASP3流 start_dispatch
   - core_kernel_impl.c・plic_kernel_impl.c：単一ハート化（テーブル単一化）
   - mtimer.c：prcid引数・sil_get_pid除去（MTIMECMPはhart1固定）
   - msi_ipi.[ch] は持ち込まない
   - core_*.trb・plic_kernel.trb → .py（arm64版.pyを規範に）
3. **チップ依存部 `arch/riscv_gcc/polarfire_soc/`**：流用＋ASP3形
   （chip_serial.cfg の CLASS解除・chip.cmake 新規・rv64gc/lp64d/medany）
4. **ターゲット依存部 `target/polarfire_soc_kit_gcc/`**：zcu102構成を規範に作成
   （DDR 0x80000000リンク・target_exit=セミホスティング（QEMU時）・
   POLARFIRE_QEMU オプションでQEMU/実機切替・presets.json：
   `polarfire_soc_kit`／`polarfire_soc_kit-qemu`）
5. **検証**：sample1（バナー・タスク切替）→ testexec（sem1等）
6. **記録**：DIVERGENCE_MAP・README索引・本ファイル実施結果

## 実施結果

（2026-06-06 記載。QEMU検証まで完了）

### 追加したファイル

**コア依存部 `arch/riscv_gcc/common/`**（26ファイル・新規系列）

| ファイル | 由来・変換内容 |
|---|---|
| `start.S` | FMP3から変換：**ハートパーキング追加**（mhartid≠TOPPERS_BOOT_HARTID(=1)をwfiループへ．E51のFPU無し対策でFPU初期化より前）・マスタ同期（start_sync/slave_wait）削除・sp初期化を`istkpt`直接参照に・sbss/bssクリアを統合 |
| `core_support.S` | FMP3から変換：my_pcb/PCBオフセット全廃→グローバル直接参照（p_runtsk/p_schedtsk/excpt_nest_count/istkpt）・dispatch_and_migrate/exit_and_migrate/force_unlock_spin削除・start_dispatchをASP3流（タスク1のスタックへ切替）・アイドルは専用スタック無し・**FMP3の2つの潜在バグを修正**（下記） |
| `core_kernel_impl.c` | TPCB廃止→`excpt_nest_count`グローバル化．start_sync/スピンロック/ジャイアントロック削除．default_int_handler等のprcid表示除去 |
| `core_kernel_impl.h` | ASP3形：sense_context(void)・lock_cpu/unlock_cpu(mstatus.MIE)・単一テーブル`inh_table[TMAX_INHNO+1]`/`exc_table[TMAX_EXCNO+1]`/`intcfg_table[TMAX_INTNO+1]`（**3.7.2 TMAX_\*規約**）・`TMAX_EXCNO=15`定義追加・LOCK/TPCB/idstkpt削除 |
| `plic_kernel_impl.c/.h` | 単一ハート化：plic_target_cidx_table廃止（自コンテキストのみ操作）・plic_config_intのprcid/affinity/SILスピンロック除去・`initialize_interrupt`はINTINIB（iprcid無し）で巡回 |
| `mtimer.c/.h` | prcid引数・sil_get_pid除去．mtimecmpは`MTIMER_MTIMECMP_SELF`（チップ依存部がブートハートに固定）．HRTCNT_BOUND=4000000002U |
| `riscv.h`・`riscv_insn.h` | ほぼ流用（riscv_tas_uint32のみ削除） |
| `core_sil.h` | SILスピンロック節（sil_loc_spn等）削除 |
| `core_asm.inc` | my_pcb/my_istkptマクロ削除 |
| `core_kernel.h`・`core_stddef.h`・`core_syssvc.h` | FMP3そのまま |
| `core_test.h` | `CPUEXC1=EXCNO_IINST`（未定義命令）に変更（FMP3のCPUEXC1_PRC\*を削除） |
| `core_kernel.py`・`core_check.py`・`core_offset.py`・`plic_kernel.py` | **trb→Python変換**（arm64の.pyを規範．PCBオフセット節削除・単一テーブル生成） |
| `core_sym.def`・`core_rename.def`（＋genrename生成）・`core_cfg1_out.h`・`arch.cmake` | ASP3形で新規作成 |

**チップ依存部 `arch/riscv_gcc/polarfire_soc/`**（20ファイル）

| ファイル | 由来・変換内容 |
|---|---|
| `polarfire_soc.h` | FMP3流用＋`TOPPERS_BOOT_HARTID=1`定義追加 |
| `chip_kernel_impl.c` | `chip_mprc_initialize`を`chip_initialize(void)`へ統合（mtvec設定→plic_global_initialize→mtimer→plic_context_initialize→mie許可→core_initialize） |
| `chip_kernel_impl.h` | `get_my_plic_cidx()`を定数化（PLIC_BOOT_CIDX=1）・VALID_INTNO(intno)単一引数化・TMAX_INHNO/TMAX_INTNO=182 |
| `chip_support.S` | trap_vector_table流用．irc_\*のPLICコンテキストをhart1固定・割込みハンドラテーブルを`inh_table`直接参照・msi_handler削除（MSIはdefault_int_handlerへ）・ローカル割込み番号は`0x10000\|mcause` |
| `chip_asm.inc` | my_pid/my_pidx削除．my_cidxは定数 |
| `chip_serial.c`・`mmuart.c`・`chip_serial.h`・`mmuart.h` | FMP3そのまま（元から非TECS） |
| `chip_serial.cfg` | `CLASS(CLS_SERIAL)`解除 |
| `chip_sil.h` | sil_get_pid削除 |
| `chip_timer.h` | `MTIMER_MTIMECMP_SELF`定義追加 |
| `chip_kernel.h`・`chip_stddef.h` | FMP3そのまま |
| `chip_kernel.py` | INTNO_VALID＝1..182（trb→Python） |
| `chip_rename.def`（＋genrename生成）・`chip.cmake`・`libc_stub.c` | 新規（libc_stubはzynqmpと同方式．**aptのriscv64-unknown-elfはlibc非同梱**のため） |

**ターゲット依存部 `target/polarfire_soc_kit_gcc/`**（22ファイル・zcu102構成を規範）

| ファイル | 内容 |
|---|---|
| `target.cmake` | `POLARFIRE_QEMU`オプション（既定ON）でQEMU/実機切替・`TOPPERS_OMIT_DATA_INIT`定義（ELF直接ロードのためROM化不要）・run＝QEMU実行（`-DQEMU_SYSTEM_RISCV64=`でパス指定可） |
| `polarfire_soc_kit.ld` | DDR 0x80000000リンク（VMA==LMA）・`__global_pointer$`・sbss/sdataをbss/dataに統合 |
| `polarfire_soc_kit.h` | Icicle Kit（QEMU）：CORE_CLK_MHZ=600・MTIMER_FREQ_MHZ=1・USE_UART0・SIL_DLY_TIM=70/44（実機値．実機対応時に要再較正） |
| `target_kernel_impl.c` | target_initialize(void)化（UART0クロック/リセット→sio_initialize→target_fput_initialize）・**target_exit＝RISC-Vセミホスティング SYS_EXIT**（TOPPERS_USE_QEMU時） |
| `target_kernel_impl.h`・`target_kernel.h` | TNUM_PRCID/PRC\*/CLS_\*削除・TSTEP_HRTCNT=1U |
| `target_syssvc.h` | 非TECS（INTNO_SIO=90/MMUART0・115200 8N1） |
| `target_kernel.cfg` | タイマ登録なし（MTIはローカル割込みのためチップ依存部で直接処理） |
| `target_kernel.py`・`target_check.py`・`target_rename.def`（＋生成）・`presets.json` | zcu102と同形（`polarfire_soc_kit`／`polarfire_soc_kit-qemu`） |
| `target_serial.{h,cfg}`・`target_timer.h`・`target_stddef.h`・`target_sil.h`・`target_test.h`・`target_asm.inc`・`target_cfg1_out.h` | FMP3流用 |

**その他**：`cmake/toolchain-riscv64.cmake`（新規）・ルート`CMakePresets.json`（include追記）

### FMP3→ASP3変換で発見したFMP3の潜在バグ（ASP3版で修正）

1. **割込み/例外フレーム破棄の符号誤り**（core_support.S）：p_runtsk==NULL
   （アイドルへ戻る）経路でスクラッチレジスタフレームを捨てる際，
   `addi sp, sp, -(TNUM_...)` と**負値でspを下げていた**．FMP3はアイドル専用
   スタック（idstkpt）に切り替えるため顕在化しないが，アイドル専用スタックを
   持たないASP3変換ではスタックリークになるため正値に修正．
2. **割込み入口のFPU保存/復帰でfa3（f13）がfs3になっていた誤記**
   （core_support.S）：スロット11にfs3を保存・復帰しておりfa3が保存されない
   （FPU使用タスクで割込みを跨いでf13が破壊される）．fa3に修正．

### 設計メモ

- **割込みモデル**：PLIC（MEI）はclaim/completeで割込み番号を取得し
  `inh_table[intno]`を引く．MTI（タイマ）はPLICを経由しないローカル割込みで，
  `irc_begin_int`が`target_hrt_handler`へ直接分岐（優先度は内部表現1＝外部-1
  相当）．割込み優先度マスクはPLIC閾値レジスタ＋mie.MTIE/MSIEの組（FMP3の
  実機検証済み方式を踏襲）．
- **QEMUブート**：`-bios none -kernel asp.elf`．QEMUがeNVM(0x20220000)に
  リセットベクタを生成し**全5ハート**（E51×1＋U54×4）がELFエントリへ来る．
  start.S先頭（FPU初期化前）でhart1以外をwfiパーキング．
- **MMUARTのQEMUモデル**：16550互換（0x00-0x1F）＋拡張レジスタは素のRAM
  （0x20-0xFFF）のため，FMP3ドライバのDLR/DMR/FCR等への書込みは無害．
  LSR/RBR/THR/IERは16550として正しく動作する．
- ras_intはPLICにセットペンディング機構が無いためサポートしない
  （FMP3と同じ．TOPPERS_TARGET_SUPPORT_ENA_INT/DIS_INT/PRB_INTのみ）．

### Git情報

- ベースコミット：`844756d`（docs(dev): pin resume pointers for QEMU RISC-V work）
- ファイルリストの再現：
  `git diff --stat upstream main -- arch/riscv_gcc target/polarfire_soc_kit_gcc cmake/toolchain-riscv64.cmake`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| ビルド（riscv64-unknown-elf-gcc 13.2 / apt） | ○ | QEMU・実機両プリセットともコンパイラ警告ゼロ（リンカRWX警告のみ＝無害）．asp.elf 286KB |
| QEMU sample1 | ○ | バナー・logging task・task1周期実行（mtimerタイマ動作）・`a`/`r`入力でtask2/3切替（MMUART受信割込み）・`z`でCPU例外ハンドラ→復帰・`Q`で終了 |
| testexec（QEMU・機能テスト34本） | ○ | **全件PASS**：task1/sem1/sem2/flg1/dtq1/pdq1/mpf1/mutex1〜8/notify1/suspend1/sysman1/sysstat1/tmevt1/raster1/raster2/exttsk が「All check points passed.」，hrt1 正常完了，cpuexc1〜10 PASS（cpuexc10は「not necessary」正常終了） |
| `ext_ker`でのQEMU自動終了 | ○ | RISC-VセミホスティングSYS_EXIT動作（testexecが自動進行） |
| int1 | − | ras_int非サポート（PLICにセットペンディング無し）のため対象外 |
| dlynse | − | QEMUは命令実行時間が実時間と無関係のため対象外（実機対応時に実施・SIL_DLY_TIM再較正） |
| 回帰（POSIX／QEMU mps2-an521） | ○ | sample1動作確認（既存ターゲットへの影響なし．変更はルートCMakePresets.jsonのinclude追記のみ） |
| 実機 Icicle Kit | − | ビルドのみ確認（`POLARFIRE_QEMU=OFF`．実機ロード手段＝HSS/SoftConsole資材は実機対応時に整備） |

### DIVERGENCE_MAP との関連

- `arch/riscv_gcc/common/`・`arch/riscv_gcc/polarfire_soc/`・
  `target/polarfire_soc_kit_gcc/` を新規追加行として記載
  （上流ASP3に存在せず衝突なし．移植元はFMP3）．
