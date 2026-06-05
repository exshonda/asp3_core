# CLAUDE.md — TOPPERS/ASP3 STM32MP257F-DK ターゲット依存部（開発メモ）

## 全体像

TOPPERS/ASP3 3.7.2 への STM32MP257F-DK（AArch64/Cortex-A35）移植．
**FMP3 の同ボード移植（`fmp3_3.3_svn`）を移植元**とし，FMP3↔ASP3 の
arm 依存部の実差分から抽出した変換規則で arm64 依存部を新規作成した
（2026-06-03，StepA/StepB 完了・実機検証済み）．

- 設計・変換規則・検証記録: `../../arch/arm64_gcc/stm32mp2/PORTING_ASP3_STM32MP2.md`
- 利用者向け: `target_user.md`
- ステアリング記録: `baremetal/.steering/20260603-asp3-arm64-port/`
  （requirements/design/tasklist＋実機 UART ログ）

## ビルド・実行（クイック）

```bash
cmake --preset stm32mp257f_dk_arm64_gcc -B build/stm32mp257f_dk_arm64_gcc   # 非TECS（TECSは廃止済み）
cmake --build build/stm32mp257f_dk_arm64_gcc     # -> asp.elf（コンパイラ警告ゼロであること）
ninja -C build/stm32mp257f_dk_arm64_gcc swd-run  # 実機実行（FSBL/minimal_boot 入り SD で起動済みのこと）
ninja -C build/stm32mp257f_dk_arm64_gcc console  # / osdebug / swd-debug / openocd / gdb
```

（Makefile版ビルドは廃止．aarch64-none-elf が無い開発機では
`-DA35_TOOLCHAIN_PREFIX=aarch64-linux-gnu-` でコンパイル確認のみ可）

## 把握しておくべき移植のポイント

- **3.7.2 規約**: `TMAX_INHNO/TMAX_INTNO/TMAX_EXCNO`＋配列 `TMAX_*+1`．
  3.7.1 以前の `TNUM_*` 規約で書かないこと．
- **GICv2 のみ**・`GIC_NO_FIQ_IN_SECURE`（IRQ 配送）・優先度は FMP3 実機検証値
  （`GIC_PRI_LEVEL=32`/SHIFT=3/`INT_IPM=(ipm+15)<<3`）を踏襲．ASP3 arm の
  SAFEG 式とは意図的に異なる．
- **EL3/EL2 フック**（`target_el3_initialize` 等）は ASP3 arm に無い本依存部固有
  の仕様（STM32MP2 のブートに必須）．`enable_smp()`（CPUECTLR.SMPEN）も
  シングルコアで必須（キャッシュ）．
- **非 TECS 構成を再サポート**（2026-06-05〜．asp3_core の TECSレス化に伴う）:
  非 TECS の SIO ドライバは `stm32usart.{c,h}`（chip 依存部，TF-A 初期化済み
  前提で再初期化しない）＋ `target_serial.{c,h,cfg}`．上流 `extension/non_tecs`
  版 syssvc を使用．TECS 構成（`tUsart` セル等の `.cdl`）はその後の TECSレス化で
  削除済み．`core_kernel.h` の `#ifndef TECSGEN` ガード（`__int128` 対策）は
  当時の名残として残置．
  経緯: 2026-06-04 に FMP3 由来の非 TECS 構成（nt_syssvc）を廃止→
  2026-06-05 に上流 non_tecs 版で再導入（PORTING_ASP3_STM32MP2.md 参照）．
- **MMU は静的テーブルのみ**（`arm64_memory_area[]` weak）．
- sample1 の入力テスト時は `a`（act_tsk）→ **`r`（rot_rdq）** が必要
  （同優先度タスクは r 無しでは切り替わらない）．

## 現状

- **StepA（ビルド）/ StepB（実機 sample1）完了**（2026-06-03）．
  タイマ・シリアル入出力・タスク切替を DK 実機で確認．
- **ランタイムテスト完了（2026-06-05）: 機能テスト 36/36 PASS**（DK 実機，
  当時の Ruby 版 testexec.rb＋`<ASP3>/TEST-EXEC/`．現在は `test/testexec.py`）．cpuexc は arm_m と異なり全件 PASS
  （arm64 は同期例外のためマスク中エスカレートの制限なし）．テストで
  `SIL_DLY_TIM1/2` を 70/44→**12/10** に修正（dlynse 実測），swd-run の
  halt レース対策（起動待ち 6000ms＋examine/halt 間 settle）を実施．
  詳細は PORTING_ASP3_STM32MP2.md「ランタイムテストの実施」．
- StepC（gdb OS-awareness の ASP3 移植）: `gdb_os_aware/`（本依存部内） —
  状況は tasklist（steering）を参照．
- 未検証: SD 直接ブート / OVRHDR．性能評価（perf）・simt 系テストは未実施．

## 運用規約

- `target_user.md` は TOPPERS スタイル句読点（「，」「．」）．
- 設計に影響する変更は PORTING_ASP3_STM32MP2.md に記録．
- FMP3 ツリー（`fmp3_3.3_svn`）は参照のみ（本移植から改変しない）．
