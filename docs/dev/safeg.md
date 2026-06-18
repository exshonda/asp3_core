# SAFEG（ARMv8-M TrustZone デュアルOS）

## 項目

SAFEG（SafeG-M）＝ARMv8-M TrustZone-M を用いたセキュア/非セキュア分離・
デュアルOS実行基盤（優先度：中）。AGENTS.md §1 機能追加計画に追加。

上流 TOPPERS の SafeG（A-profile：ARM TrustZone でのデュアルOS）の Cortex-M
（ARMv8-M TrustZone-M）版を、asp3_core の arm_m 共通アーキへ取り込んだもの。
ASP3 をセキュア側に置き、非セキュア（NS）側で別ファームを起動・隔離する。

> 注：`arch/arm64_gcc/stm32mp2` 等にある「SafeG」記述は A-profile の別文脈。
> 本項目は arm_m（Cortex-M33/RP2350/i.MX RT600）TrustZone-M を対象とする。

## 内容

- **目的**：ASP3 をセキュア側カーネルとして動かし、TrustZone-M（SAU/IDAU・
  NSC）で NS 側を隔離。NS 側に非信頼ファームを置いてもセキュア資源を保護する
  セーフティネット型分離を実現する。
- **設計方針**：上流 SafeG の Cortex-M 取り込み。ローカル発明ではなく上流の
  デュアルOS方式（pendsv/svc/dispatcher のコンテキスト退避にセキュア状態を
  載せる）を arm_m 共通へ移植。既定 OFF（`ENABLE_SAFEG_M`）で素の ASP3 は不変。
- **BTASK**：NS を起動するセキュア側バックグラウンドタスク `_SAFEG_BTASK`
  （id=1＝`TMIN_TSKID`）。id を構造的に保証するため cfg の INCLUDE 順を握れる
  asp3_core 側に CRE_TSK を置く（経緯はコミット `b194fe8`）。
- **kernel/ 無改変**：変更は `arch/arm_m_gcc/`（EXTENDED）と `target/`（NEW）に
  限定。すべて `#ifdef TOPPERS_SAFEG_M` でガードし PRISTINE 領域を侵さない。

## 実施プラン

上流 SafeG-M を M1〜M4 ＋ Phase B の段階で取り込む（着手前計画）。

1. **M1（archコア／プラミング）**：`core_support.S` の pendsv/svc/do_dispatch を
   SafeG-M 版へ載せ替え、`ENABLE_SAFEG_M` ビルドオプションと TrustZone 強制
   （`#error` ガード）、`EXC_RETURN_S/_NESTED` を追加。
2. **M2（arch依存部の充足）**：deactivate 群・usagefault・SAU/SCB_NS レジスタ・
   `launch_ns`・割込み優先度シフト（IIPM/IPM）・`set_*_ns` 命令・rename 同期・
   ベクタ #6 差し替え・imxrt600 chip 定数 swap・`-mcmse`。
3. **M3（an505 セキュアボード層）**：`_SAFEG_BTASK` cfg 生成・`TOPPERS_NS_VTOR`・
   SAU 設定・CMSE import lib 配線・region 方式 ld・target.cmake 分岐。
4. **M4（他ターゲット展開）**：RP2350(pico2)・i.MX RT685(mimxrt685evk) の
   セキュアボード層（SAU/NS_VTOR/region ld/target.cmake）。
5. **Phase B（NS連携の実証）**：NS 専用 UART1 をセキュア側がブリングアップして
   SAU 開放（RP2350）。

## 実施結果

**完了**（2026-06-17）。M1〜M4＋Phase B を main に取り込み済み。既定 OFF のため
素の ASP3 ビルドは不変、`-DENABLE_SAFEG_M=ON` で有効化する。

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
| `arch/arm_m_gcc/common/core_support.S` | pendsv/svc/do_dispatch/dispatcher の SafeG-M 載せ替え（`#ifdef TOPPERS_SAFEG_M` 7サイト）＋ deactivate/usagefault 群を末尾に自己完結追加（M1/M2） |
| `arch/arm_m_gcc/common/arm_m.h` | TrustZone 強制 `#error` ガード・`EXC_RETURN_S/_NESTED`・SAU/SCB_NS/AIRCR/NSACR/ITNS/FPCCR_NS 等を純追加（`#ifdef`下、M1/M2） |
| `arch/arm_m_gcc/common/core_kernel_impl.c/.h` | `core_initialize`（Deep sleep禁止/NSACR/AIRCR.PRIS/ITNS全NS化）・`config_int`・`launch_ns`・`IIPM_ENAALL`/IPMシフト swap（M2） |
| `arch/arm_m_gcc/common/core_insn.h` | `set_msp_ns`/`set_control_ns`/`set_faultmask_ns`/`is_secure` 追加（M2）＋ `set_control_ns` に ISB 追加（fix） |
| `arch/arm_m_gcc/common/core_rename.{def,h}`・`core_unrename.h` | `launch_ns`/`deactivate_nonsecure_interrupts`/`usagefault_handler` 識別子追加（M2） |
| `arch/arm_m_gcc/common/core_kernel.py` | ベクタ #6（UsageFault）を SAFEG時 `_kernel_usagefault_handler` へ（生成Cに `#ifdef` 出力、M2） |
| `arch/arm_m_gcc/common/core_kernel.cfg` | `_SAFEG_BTASK`（id=1）の CRE_TSK と設計意図コメント（M3） |
| `arch/arm_m_gcc/common/arch.cmake` | `option(ENABLE_SAFEG_M OFF)`→`-DTOPPERS_SAFEG_M`＋TrustZone強制・`-mcmse`・CMSE implib（M1/M2/M3） |
| `arch/arm_m_gcc/imxrt600/chip_kernel.h`・`chip_sil.h` | `TMIN_INTPRI`/`TBITW_IPRI` を `#ifdef/#else` swap（common impl.h と同期、M2） |
| `target/mps2_an505_gcc/{target.cmake,target_kernel.cfg,target_kernel_impl.c/.h}` | an505 セキュアボード層：SAU 設定・`TOPPERS_NS_VTOR`・test app 分岐（M3） |
| `target/pico2_arm_gcc/{target.cmake,target_kernel.cfg,target_kernel_impl.c/.h}` | RP2350 セキュアボード層：SAU/NS_VTOR/region・NS 専用 UART1 ブリングアップ＋ACCESSCTRL(0xACCE)＋RESET_DONE 上限待ち（M4/B） |
| `target/mimxrt685evk_gcc/{target.cmake,target_kernel.cfg,target_kernel_impl.c/.h}` | i.MX RT685 セキュアボード層：SAU/NS_VTOR=0x8400000 分岐（M4） |
| `CMakeLists.txt` | SAFEG ビルド配線 |
| `DIVERGENCE_MAP.md` | arch/target の SAFEG 改変行を【SAFEG】タグで台帳化 |

### 追加したファイル

- `target/mps2_an505_gcc/mps2_an505_safeg.ld`（region 方式リンカスクリプト）
- `target/pico2_arm_gcc/rpi_pico2_safeg.ld`
- `target/mimxrt685evk_gcc/mimxrt685_safeg.ld`

### 削除したファイル

なし。

### Git情報

- ベースコミット：`184e8c6`（SAFEG 着手直前）
- 関連コミット範囲：`0ed9c5c`（M1）〜 `a564514`（Phase B 完成）の13コミット
- ファイルリスト再現コマンド例：
  ```bash
  git log --reverse --format='%ad %h %s' --date=short 184e8c6..a564514 | grep -i safeg
  git diff --stat 184e8c6 a564514 -- arch/arm_m_gcc target CMakeLists.txt
  ```

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| POSIX | − | 対象外（arm_m TrustZone 専用機能） |
| QEMU (mps2-an505) | ○ | M3：SAU/NS_VTOR・`_SAFEG_BTASK` 起動経路をビルド＋QEMU で確認 |
| 実機 | ○ | RP2350(pico2)：Phase B で NS 専用 UART1 をセキュア側がブリングアップ・SAU 開放まで確認（B 完成）。i.MX RT685：セキュアボード層配線 |

> 既定 OFF（`ENABLE_SAFEG_M`）であり、素の ASP3 回帰（an505 testexec 等）に影響しない。

### DIVERGENCE_MAP との関連

`arch/arm_m_gcc/common/`（core_support.S・arm_m.h・core_kernel_impl.c/.h・
core_insn.h・rename系・core_kernel.py・arch.cmake）と imxrt600 chip 定数は
すべて【SAFEG】タグで `DIVERGENCE_MAP.md` に登録済み（`#ifdef TOPPERS_SAFEG_M`
ガードで上流不変）。`core_support.S` の pendsv #3/svc #4 の `#else` 経路が
最脆弱＝上流変更時に要再確認、と台帳に明記。

## コードレビュー指摘 (F1-F4) と対応状況（2026-06-18）

SAFEG-M コードレビューで挙がった指摘 F1-F4 の対応記録（詳細メモは別途 NSBIT.md）。

| # | 重大度 | 指摘 | 対応 |
|---|---|---|---|
| **F1** | 中 | NS優先度ビット縮小の表現がチップ間で非対称（imxrt600 のみ SAFEG時 TBITW 3→2、an505/rp2350 は据置） | **解決**。不変条件 `TBITW_IPRI(SAFEG)=物理__NVIC_PRIO_BITS-1`（最上位1本を PRIS の NS 分割へ予約）を `core_kernel_impl.h` に明文化し、an505 `3→2`・rp2350 `4→3` を `#ifdef` で追加（imxrt600 に統一）。据置だと `INT_IPM` の LSB が未実装優先度ビットに落ち隣接優先度が衝突する潜在バグ。**実機 rp2350 A〜D1 9/9 で非回帰確認**。 |
| **F2** | 低 | `core_support.S` のサイト間レジスタ暗黙契約（asm・宣言不可・上流マージで破綻し得る） | **解決(コメント)**。契約1: `r6`=basepri を dispatcher_0 #7→svc_handler #4（svc #0 跨ぎ）、契約2: `r3`=BTASK判定を svc #5a→#5b（ldmfd 跨ぎ）。各生産/消費サイトに対サイトと生存条件を明記。コード不変。 |
| **F3** | 低 | マクロ命名の非対称：C側ガードは `TOPPERS_SAFEG_M`（ENABLE 無し、`TOPPERS_ENABLE_TRUSTZONE` 規約と不整合）、CMake は `ENABLE_SAFEG_M` | **現状維持（記録のみ）**。整合させるなら `TOPPERS_ENABLE_SAFEG_M` だが全 `#ifdef` サイトに波及するため見送り。`arch.cmake` で `ENABLE_SAFEG_M→ -DTOPPERS_SAFEG_M -DTOPPERS_ENABLE_TRUSTZONE`。 |
| **F4** | 低 | `core_kernel.cfg` の BTASK(id=1) 生成は cfg-pass の cpp が `-DTOPPERS_SAFEG_M`（`arch.cmake` 由来）を受けることに依存 | **確認済（記録のみ）**。an505(QEMU) id=1 ＋ rp2350 実機 9/9 で動作確認。cfg plumbing 変更時に BTASK 生成が静かに変わる結合だけ留意（id=1 保証は `core_kernel.cfg` の最初の CRE_TSK＝§「_SAFEG_BTASK 配置」参照）。 |
