# PORTING_ASP3_STM32MP2.md — ASP3 への ARM64/STM32MP2 移植記録

最終更新: 2026-06-03

## 概要

TOPPERS/ASP3 3.7.2（シングルプロセッサ）に，AArch64（Cortex-A35）の
コア依存部（`arch/arm64_gcc/common`），STM32MP2 チップ依存部
（`arch/arm64_gcc/stm32mp2`），STM32MP257F-DK ターゲット依存部
（`target/stm32mp257f_dk_arm64_gcc`）を新規追加した移植の記録．

- **移植元**: TOPPERS/FMP3 3.3（`fmp3_3.3_svn`）の arm64 依存部一式
  （STM32MP257F-DK で StepS/StepM 実機検証済みのもの）．
- **変換規範**: FMP3 と ASP3 3.7.2 の両方に存在する arm 依存部
  （`arch/arm_gcc/common`）の実差分から「FMP3→ASP3 変換規則」を抽出して適用．
- **3.7.2 規約**: `TNUM_INHNO/TNUM_INTNO/TNUM_EXCNO` は使わず
  **`TMAX_*`（配列サイズ `TMAX_*+1`）** 規約で作成（asp3_3.7.1→3.7.2 の
  arm 依存部の変更を反映）．スプリアス割込み判定も `TMAX_INHNO`＋`b.hi`．
- 経緯・検証ログ: `baremetal/.steering/20260603-asp3-arm64-port/`．

## 実行条件（FMP3 の DK 移植と同一）

- TF-A（BAREMETAL_IMAGE_LOADER）が EL3 で起動 → `start.S` がセキュア EL1 へ
  ドロップ（`TOPPERS_TZ_S`）．EL3/EL2 フック（`target_el3_initialize`/
  `target_el2_initialize`→chip）は ASP3 arm に無い本依存部固有の移植仕様．
- GICv2（GIC-400）．セキュア(Group0)割込みを **IRQ 配送**
  （`GIC_NO_FIQ_IN_SECURE`，`Makefile.chip` で定義）．
- タイマ: Secure Physical Timer（CNTPS，INTID 29）．`CNTFRQ_EL0` 実行時読出し．
- コンソール: USART2（115200 8N1，ST-LINK VCP）．
- メモリ: TEXT=`0x88001000`（SWD）/ DATA=`0x90000000`（`.data` は ROM から
  `start.S` がコピー）．
- MMU: 静的テーブル `arm64_memory_area[]`（weak）方式のみ
  （FMP3 で 2026-06-03 に確立した方式を標準化．`target_mmu_init()` 経路は無い）．

## FMP3 → ASP3 の主な変換内容

### コア依存部（arch/arm64_gcc/common）

| 項目 | 変換 |
|---|---|
| TPCB | 廃止．`uint32_t excpt_nest_count` をグローバル化．`istkpt` は kernel_cfg 生成の const を参照 |
| ハンドラテーブル | `p_inh_table[prcid]` 等の間接参照 → 単一 `inh_table[TMAX_INHNO+1]` / `exc_table[TMAX_EXCNO+1]` / `intcfg_table[TMAX_INTNO+1]`（core_kernel.trb はASP3 arm の書式） |
| core_support.S | `my_pcb`/`PCB_*` オフセット全廃→グローバル直接参照．`dispatch_and_migrate`/`exit_and_migrate`/IPI 分岐/`force_unlock_spin` 削除．`start_dispatch` は ASP3 流（タスク1のスタックへ切替）．アイドルは専用スタック無し（dispatcher_1） |
| start.S | マスタ判定・`start_sync`・slave_wait・`target_mprc_initialize` 削除．EL ドロップ・bss/data 初期化・hook は維持 |
| GIC | **GICv2 のみ**（v3/v4 コードは持ち込まず，`#error` ガード）．SGI 送信はself-target（`0x02000000\|intno`）．`config_int`/`initialize_interrupt` は ASP3 形式（prcid/affinity 無し）．`SIL_LOC_SPN`→`SIL_LOC_INT` |
| core_timer | `timer_cval[TNUM_PRCID]`→単一．`target_hrt_set_event(HRTCNT)`/`target_hrt_raise_event(void)`（prcid 引数削除）．`INHNO_TIMER_PRC*` 削除．32bit HRTCNT（`HRTCNT_BOUND=4000000002U`，`TSTEP_HRTCNT=1U`，TCYC 無し） |
| core_sil.h | スピンロック節（`TOPPERS_sil_loc_spn` 等）・`sil_get_pid` 削除 |
| arm64.c | MMU 管理の per-プロセッサ配列（`tt_pri_prc*`/`tt[TNUM_PRCID]`/`mmap[TNUM_PRCID]`）を単一化 |

### 設計判断・注意点

1. **割込み優先度マッピングは FMP3 実機検証値を踏襲**:
   `GIC_PRI_LEVEL=32`（stm32mp2.h）＋TZ_S で `GIC_PRI_SHIFT=3`，
   `INT_IPM(ipm)=(ipm+15)<<3`（PMR アイドル値 120）．
   ASP3 arm の `(GIC_PRI_LEVEL-1)` 式・SAFEG ベースの定義とは**異なる**
   （FMP3 式 `(GIC_PRI_LEVEL>>1)-1` を維持）．TOPPERS-only（NS 側なし）の
   本ボードでは実害なく，実機検証済みの値を優先した．
2. **`enable_smp()`（CPUECTLR.SMPEN）は維持**: シングルコアでも Shareable
   メモリのキャッシュ有効化に必須（FMP3 移植時の実機知見）．
3. **FMP3 の潜在バグ修正**: `gic_support.S` の `irc_end_exc`（GICv2 経路）が
   保存した優先度マスクを PMR に**書き戻さず読んでいた**のを `str` に修正．
4. `gicd_initialize()` は `core_initialize()` 内（`gic_init()` の前）で呼ぶ
   （FMP3 ではマスタ専用の `core_mprc_initialize()` にあった）．

### チップ依存部（arch/arm64_gcc/stm32mp2）

- `chip_mprc_initialize`（Core1=a35_1 の EL3 直接起動）を削除．
  `chip_initialize(void)` 化（キャッシュ無効化＋`core_initialize()`）．
- `SMP_CACHE_BYTES` 削除．`chip_serial.cfg` の `CLASS(CLS_SERIAL)` 解除．
- `chip_kernel.trb`: `$INTNO_VALID = [*(0..415)]`（ASP3 形式，prcid エンコード無し）．
- `GIC_NO_FIQ_IN_SECURE` は `Makefile.chip` で定義（FMP3 と同じ）．

### ターゲット依存部（target/stm32mp257f_dk_arm64_gcc）

- `target_initialize(void)`/`target_exit()` 化．`USE_THREAD_ID_PCB`・
  プロセッサID 関数群（`get_my_prcidx` 等）・`target_asm.inc`・`target_ipi.*`・
  `target_class.trb`・`sil_get_pid` を削除．
- `TNUM_PRCID`/`PRC*`/`CLS_*` の定義を削除（`target_kernel.h`）．
- SWD デバッグ環境（`openocd/`・`minimal_boot/`・`swd-debug.gdb`・
  Makefile.target の swd-run/swd-debug/console/osdebug）は FMP3 から流用
  （ELF 名は `asp`）．

### 非 TECS 版システムサービス（廃止済み）

移植当初は TECS オミット構成（-w）用に，FMP3 由来の非 TECS 版
syslog/serial/logtask/banner（`nt_syssvc/`）とそれを取り込む `tecsgen.cfg`
スタブ，SIO ドライバ（chip_serial/stm32usart.c）を同梱していたが，
**2026-06-04 に非 TECS 構成のサポートを廃止し，これらのファイルを削除した**
（TECS 対応の節を参照）．`stm32usart.h` はレジスタ定義のみ残し，
tUsart セルが使用する．

## ビルド・実行

```bash
mkdir <OBJ> && cd <OBJ>
ruby <ASP3>/configure.rb -T stm32mp257f_dk_arm64_gcc   # TECS 構成のみサポート
make            # -> asp (ELF) ※コンパイラ警告ゼロ
make swd-run    # SWD でロード＆実行（FMP3 と同じ OpenOCD 設定）
make console    # UART コンソール
```

## 検証（2026-06-03，DK 実機）

- ビルド: コンパイラ警告ゼロ（リンカ RWX 警告のみ＝無害）．
  `asp` 346KB / `asp.bin` 52KB．
- 実機: バナー（TOPPERS/ASP3 Kernel Release 3.7.2），logging task 起動，
  task1 周期実行（Generic Timer 動作），シリアル入力で 'a'（act_tsk）・
  'r'（rot_rdq）により task1→task2→task3 の切替を確認．
- シリアル受信のメモリプローブ（spcb の rcv_write_ptr / echo 送信数）で
  ISR→バッファ→タスクの全経路の動作を確認．

## 追記（2026-06-04）

- `Makefile.target` の swd-run/swd-debug 用 `ENTRY_PC` を，`TEXT_START_ADDRESS`
  固定から **ELF のエントリポイントの自動取得**（readelf）に変更（HRP3 移植から
  のバックポート．本ターゲットは固定リンカスクリプトの `ENTRY(start)` により
  両者が一致するため動作は不変だが，ELF を正とすることで配置変更に頑健になる）．
- `target/.../target_timer.h` の `HRT_CNT_TO_HRTCNT(cnt)`／
  `HRT_HRTCNT_TO_CNT(hrtcnt)` マクロの引数が無括弧で，式を渡すと
  演算子優先順位により誤展開する潜在バグを修正（`(cnt)`／`(hrtcnt)` に）．
  ASP3 の既存の呼出しは単一変数渡しのため動作への影響はない（HRP3 移植時に
  タイムウィンドウタイマの残り時間計算で顕在化して発見．FMP3 ツリー
  （fmp3_3.3_svn）も同日修正済み）．

## 既知の制限・未実施

- SD 直接ブートは未検証（SWD 実行のみ）．
- `TOPPERS_SUPPORT_OVRHDR`（オーバランハンドラ）は未検証．
- GICv3/v4 は非対応（必要なら FMP3 版から追補可能）．

## TECS 対応（2026-06-03 追記）

当初は TECS オミット構成のみだったが，TECS 構成に対応し，その後
**TECS 構成のみのサポート**とした（2026-06-04 に非 TECS 構成を廃止．
`nt_syssvc/`・`tecsgen.cfg` スタブ・`chip_serial.{c,h,cfg}`・`stm32usart.c`・
`target_serial.{cfg,h}` を削除し，`target_kernel_impl.c`/`target_syssvc.h`/
`Makefile.target` の TOPPERS_OMIT_TECS 分岐を撤去）．
ステアリング記録: `baremetal/.steering/20260603-asp3-tecs/`．

- 新規セル:
  - `arch/arm64_gcc/stm32mp2/tUsart.{cdl,c}` — STM32MP2 用 USART SIO ドライバセル．
    `arch/arm_m_gcc/stm32h5xx_stm32cube/tUsart` を基に HAL 依存を除去（レジスタ
    マップは STM32H5 と同一）．**オープン時の再初期化はしない**（TF-A が
    115200 8N1・FIFO 有効で初期化済み．クロックツリー取得手段が無いため．
    stm32usart.c と同方針）．bps 属性なし．
  - `target/.../target.cdl`，`tSIOPortTarget.cdl`（USART2/INTNO 147），
    `tSIOPortTargetMain_inline.h`（nucleo_h563zi 版を規範）．
- 共通部の変更（1 箇所）: `arch/arm64_gcc/common/core_kernel.h` の
  `TOPPERS_STK_T (__int128)` を `#ifndef TECSGEN` ガード．
  **tecsgen の C パーサが __int128 を解釈できない**ため，tecsgen の
  ヘッダ解析時のみ int64_t に見せる（生成コードには影響しない）．
- `target_kernel_impl.c`: 低レベル出力の初期化を TECS/非 TECS で分岐
  （TECS 時は `tPutLogSIOPort_initialize()`，zybo と同形）．
- `tecsgen.cfg`: `INCLUDE("tecsgen.cfg")` は `-I./gen` 経由で tecsgen 生成版が
  解決される（非 TECS 廃止後はスタブ不要となり削除）．

### 検証（2026-06-03，DK 実機）

- TECS 構成: コンパイラ警告ゼロでビルド（`asp` 375KB）．実機で tBanner 経由の
  バナー・tLogTask・task1 周期実行・シリアル入力（'r'）による task1→2→3 切替を確認．
- 非 TECS 構成（-w）: 回帰ビルド警告ゼロ．
- OS-awareness: TECS 構成 ELF で atask が動作（TECS 生成のオブジェクト名
  `SEMID_tSemaphore_SerialPort1_ReceiveSemaphore` 等も正しく表示）．

## ランタイムテストの実施（2026-06-05）

`test/testexec.rb` により，標準のランタイムテスト（機能テスト 36 件）を DK 実機で実施した．
方式は rp2350 移植（`arch/arm_m_gcc/rp2350/PORTING.md`）のテスト実施に倣う．

### テスト環境（`<ASP3>/TEST-EXEC/`）

- `TARGET_OPTIONS`: `-T stm32mp257f_dk_arm64_gcc`（TECS 構成・既定）
- `TARGET_RUN`: `bash ../run_test.sh` — UART キャプチャを**書き込みより先に**開始し
  （起動直後のバナー取りこぼし防止），`make swd-run` でロード後，完了パターンか
  エラー（`## ...`／`Unregistered exception`）の検出まで待つ（タイムアウト 90 秒，
  swd-run 失敗時 1 回リトライ）．
- 完了パターンは `All check points passed.` に加え，`check_finish(0)` で終わる
  テスト用に以下を判定する:
  - cpuexc10: `This test program is not necessary.`（本構成では対象外＝正常終了）
  - hrt1: `high resolution timer count test finishes.`
  - dlynse: 完了メッセージなし．17 測定（fitting 14 + boundary 3）が出揃った時点で
    NG の有無により判定．

> ⚠️ テストバッチを**並行実行してはならない**（ボードと /dev/ttyACM0 を共有するため）．
> 必ず 1 件ずつ逐次実行する．

### 結果（36 テスト全件 PASS）

| テスト | 結果 |
|---|---|
| task1 / sem1 / sem2 / flg1 / dtq1 / pdq1 / mpf1 | PASS |
| mutex1〜mutex8 | PASS（8件） |
| notify1 / suspend1 / sysman1 / sysstat1 / tmevt1 | PASS |
| raster1 / raster2 / exttsk / int1 / hrt1 | PASS |
| dlynse | PASS（SIL_DLY_TIM 修正後，17 測定全て OK） |
| cpuexc1〜cpuexc10 | PASS（10件．cpuexc10 は「not necessary」正常終了） |

**cpuexc が全件 PASS する点は arm_m（rp2350）と異なる**．arm_m では PRIMASK=1 中の
UsageFault が HardFault にエスカレートするアーキ仕様により cpuexc1/cpuexc4 が FAIL
する（rp2350 PORTING.md「既知の制限」）が，arm64 の CPU 例外発生方法は同期例外
（`svc #0xF000`，`core_test.h`）であり，割込みマスク状態と無関係に配送されるため，
この制限自体が存在しない．

### テストにより発見・修正した問題

1. **swd-run の halt レース**（`target/.../Makefile.target`）
   `reset run → sleep 3000 → arp_examine → arp_halt` の連続実行では，FSBL（LPDDR4
   トレーニング）の起動時間ばらつきと examine 直後の halt 要求消失により
   `Error: not halted` で失敗することがある（UART には landing pad の
   `Connect using OpenOCD` 到達済みでも発生）．→ 起動待ちを **6000ms** に延長し，
   examine/halt の間と halt 後に **`sleep 500` + `arp_poll`** を挿入して解消．

2. **`SIL_DLY_TIM1/2` が実機と不一致**（dlynse で発覚，`target_kernel_impl.h`）
   旧値 70/44（移植元の値）では全測定が遅延不足（NG）．dlynse の
   "for fitting parameters" 出力から実コストを逆算：初回 12ns・ループ毎 10ns．
   → **12/10** に修正し，17 測定全て OK（遅延 ≧ 要求）を確認．

### 残課題

- ~~FMP3 側（`fmp3_3.3_svn`）の `SIL_DLY_TIM1/2` も 70/44 のまま~~
  → **解決済み（2026-06-05）**: FMP3 側のランタイムテスト実施時に dlynse で実測し，
  同一値（12/10）へ修正済み（FMP3 側 `PORTING_STM32MP2.md`「ランタイムテストの実施」参照）．
- 性能評価（perf0〜5）・タイマドライバシミュレータ系（simt_*）・拡張パッケージ系の
  テストは未実施（rp2350 と同じく機能テスト 36 件のみ）．

## 非 TECS 対応の再導入（2026-06-05 追記）

asp3_core の機能追加計画「TECSレス」（AGENTS.md §1）に伴い，非 TECS 構成の
サポートを再導入した．2026-06-04 に廃止した FMP3 由来の `nt_syssvc/` 構成
とは異なり，**上流 ASP3 の `extension/non_tecs/` 版システムサービス**
（syssvc/syslog.c 等）を用いる．configure.rb は非 TECS がデフォルトと
なった（OMIT_TECS 初期設定・共通 syssvc オブジェクト自動付与）．

- 新規ファイル:
  - `arch/arm64_gcc/stm32mp2/stm32usart.c` — 非 TECS 版 USART SIO ドライバ．
    tUsart.c のロジックを uart_pl011.c（上流非 TECS ドライバ）の構造で実装．
    TF-A 初期化済み前提・再初期化しない方針は tUsart.c と同一．
    ドライバ API 宣言は `stm32usart.h` に TOPPERS_OMIT_TECS ガードで追加．
  - `target/.../target_serial.{c,h,cfg}` — sio_* インタフェース
    （ct11mpcore_gcc の非 TECS 版を規範．USART2 / INTNO 147）．
- 変更: `target_syssvc.h`（OMIT_TECS 時の TARGET_NAME・SIO 設定），
  `target_kernel_impl.c`（OMIT_TECS 時 sio_initialize＋target_fput_log），
  `Makefile.target`（OMIT_TECS 時 SYSSVC_COBJS に target_serial.o stm32usart.o）．
- TECS 構成は引き続きビルド可能（条件ディレクティブで共存）．

### 検証（2026-06-05）

- 非 TECS 構成: 全オブジェクトのコンパイルをコンパイラ警告ゼロで確認
  （開発機は aarch64-none-elf 未導入のため aarch64-linux-gnu で代用．
  glibc 静的リンクがベアメタル非対応のため開発機では最終リンク不可）．
- 実機確認: 実機接続PC（aarch64-none-elf あり）でリンクし，DK 実機で
  sample1 の動作を確認（2026-06-05）．

## エントリ時キャッシュ/MMU状態の正規化（2026-06-12 追記）

キャッシュ／MMU が有効な状態でエントリすると起動に失敗する問題
（リセット無しの SWD 再ロードで再現：旧実行が EL1 で MMU/D/I 有効のまま
halt → load_image → resume すると無出力ハングし，以後デバッグハルトも
不能になる）への対策として，`hardware_init_hook` をターゲット依存部で実装した
（エントリ時のキャッシュ状態はブートローダ構成・デバッグ運用などボード
ごとの事情で異なるため，chip ではなく target 依存部に置く）．

- 機構（正規化ルーチン本体）はコア依存部 `common/core_support.S` の
  `arm64_normalize_cache_state_el1` として共通化（DC CISW 全レベル掃引 →
  SCTLR_EL1.{M,C,I} クリア → IC IALLU → TLBI VMALLE1，各段 DSB/ISB．
  ロード／ストア不発行の leaf アセンブラ）．仕様・前提条件・再現方法は
  `arch/arm64_gcc/docs/entry-cache-normalization.md` を参照．
- 本ターゲットの `target_start_hook.S` は start.S（共通部）の weak 定義
  `hardware_init_hook` を上書きし，`b arm64_normalize_cache_state_el1` の
  tail-call で呼び出すのみ（実行するか否かのポリシーはターゲット依存部
  が決める）．パス1（cfg1_out，libasp3.a 非リンク）用に空の弱定義を同居．
- 変更: `target.cmake` — weak 上書きを確実にするため libasp3.a ではなく
  `ASP3_START_FILES`（直接リンク）に追加．
- zcu102 は weak デフォルト（何もしない）のまま＝挙動不変（QEMU 回帰済み）．
- 設計の経緯・根拠・再現手順は外側ワークスペースの
  `docs/step1_analysis.md`（stm32mp2_init/work）を参照．

### 検証（2026-06-12，DK 実機）

- 正常系（`swd-run`＝リセットあり・キャッシュ無効エントリ）: sample1 動作．
- リセット無し再ロード（EL1H・MMU/D/I 有効で halt→load→resume）:
  **2 回連続で正常起動**（従来は無出力ハング）．
- 制約: 旧実行と異なるイメージの再ロードは，本フック到達前の命令フェッチが
  stale I-cache から行われる可能性が残る（同一イメージなら問題なし．
  異なるイメージはロード側での I-cache 無効化が必要）．
