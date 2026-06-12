# メモリ保護（Zephyr相当のセーフティネット型保護）

## 項目

メモリ保護（AGENTS.md §1 機能追加計画、優先度：中）

## 内容

Zephyr が標準提供する**特権分離なしのセーフティネット型メモリ保護**を、
全ターゲットでサポートする：

| Zephyr CONFIG | 内容 | ASP3での扱い |
|---|---|---|
| `CONFIG_HW_STACK_PROTECTION` | スタックリミットHWがあれば使用 | **実装済み**（後述） |
| `CONFIG_MPU_STACK_GUARD` | リミットHWが無い場合MPUでガード | 対象アーキなし＝スコープ外 |
| コード領域の書き込み保護 | FLASH/text = RO（既定有効） | 本項目で実装 |
| SRAMの実行禁止 | RAM = XN（既定有効） | 本項目で実装 |
| `CONFIG_NULL_POINTER_EXCEPTION_DETECTION` | 0番地アクセス検出 | 本項目で実装 |

### 意義（何が嬉しいか）

1. **暴走の早期検出**：ワイルドポインタによるコード破壊・スタック上での
   コード実行（攻撃の典型）・NULL参照を、発生時点のフォルトで捕捉できる。
   発生位置から離れた場所で壊れて死ぬ「追えないバグ」を「その場のCPU例外」に変える。
2. **既存資産との相乗**：フォルトは既存のCPU例外機構→OS Awareness
   （`atask`/レジスタ）・構造化ログで観測でき、エージェントのデバッグループに乗る。
3. **安全規格意識**：ASP3の設計方針（ISO 26262/IEC 61508整合）に沿った
   ランタイム防御層の追加。Zephyrとの機能比較で見劣りしない。

### 前提調査（2026-06-12・本セッション）

- **スタック下限保護は実装済み**：`arch/arm_m_gcc/common/core_support.S` は
  ARMv8-M（Thumb≥5＝mps2/pico2_arm/mimxrt685の全arm_mターゲット）で
  ディスパッチ毎に `psplim ← TCB_stk_top`・非タスクスタックに `msplim ← istk`
  を設定済み。オーバーフローはSTKOF UsageFaultで検出される
  （＝Zephyrの HW_STACK_PROTECTION と同方式）。
  ※スタック上端（アンダーフロー）はZephyr MPU_STACK_GUARDも対象外＝同等。
- **MPU/PMPは完全未使用**（保護目的のコードなし）。arm64はMMU既存で
  `TT_UXN_BIT`/`TT_PXN_BIT`/`MEM_AP_RO_EL1` が**定義済み・未使用**。
- **MPU_STACK_GUARD の対象アーキが現状ない**：arm_mはすべてARMv8-M
  （PSPLIM保有）、v6m/v7mターゲット無し。riscvのPMPは本数制約（8）から
  静的保護と動的ガードの併用が苦しい→将来v7m追加時の課題として記録のみ。

### アーキ別の実現機構

| アーキ | 機構 | 静的保護（text-RO / RAM-XN / NULL） |
|---|---|---|
| arm_m（mps2・pico2_arm・mimxrt685） | **MPU新規**（ARMv8-M：MAIR/RBAR/RLAR・32B粒度・8リージョン） | 共通ドライバ＋ターゲット別リージョン表 |
| arm64（zcu102・stm32mp257） | **MMU既存流用** | `arm64_memory_area[]` を text(RO+X)/data(RW+UXN+PXN) に分割・ldの4KB整列 |
| riscv（polarfire・pico2_riscv） | **PMP新規**（NAPOT/TOR） | FLASH-RO・RAM-XN・0番地ブロック |
| arm A9（zybo） | MMU属性分割 | arm64と同様（QEMU専用扱いなら省略可・優先度低） |
| linux（POSIX） | 対象外（mmap保護はホストOS任せ） | — |

### 変更量見積もり（総計 ~800〜1,000行・5〜7日）

| 項目 | 規模 |
|---|---|
| arm_m MPU共通ドライバ＋3ターゲット表 | ~250行＋30行×3 |
| arm64 memory_area分割＋ld整列 | ~100行 |
| riscv PMP | ~200行 |
| STKOFフォルト識別表示・既存PSPLIMの検証テスト | ~50行 |
| CMakeオプション＋ld差分 | ~50行 |
| 検証テスト `test/protection/`（TAP・test_porting方式） | ~200行 |

**kernel/ は無改変（禁則①セーフ）**：すべて arch[EXTENDED]／target[NEW]／
CMake／ld で完結。**cfg変更も不要**（PSPLIM方式はガード余白が不要）。

## 実施プラン（Phase分割）

1. **Phase 1：arm_m 静的MPU**（mps2で開発→QEMU検証→pico2/mimxrtへ展開）
   - `arch/arm_m_gcc/common/core_mpu.[ch]`（リージョン設定・`chip_initialize`
     経由で起動時1回）＋ターゲット別リージョン表（target層）
   - CMakeオプション `ASP3_MEM_PROTECTION`（**開発中OFF既定→回帰確認後ON既定**）
   - STKOF識別（UsageFaultのCFSR表示にSTKOFビット明示）
2. **Phase 2：arm64**（zcu102 QEMU→stm32mp257はビルド＋実機別PC）
   - memory_area の text/data 分割（既存weakテーブルの既定値変更）・
     `TT_PXN/UXN`・`MEM_AP_RO_EL1` 適用・ld整列
   - zcu102はDDRが0始まり→先頭領域（<TEXT=0x100000）未マップでNULL検出
3. **Phase 3：riscv PMP**（polarfire QEMU→pico2_riscvは実機別PC）
4. **検証テスト**：`test/protection/test_protection.c`（6項目TAP案：
   text書込み→fault／RAM実行→fault／NULL読み・書き→fault／
   スタックオーバーフロー→STKOF／正常動作の継続確認）
5. **回帰**：testexec全件×QEMU3機種（CI）＋実機（別PC）。既定ON化はここで判断
6. **記録**：DIVERGENCE_MAP（arch変更行）・IMPL_INDEX・building.md・README索引

### リスク・確認事項（見積もりに織込み済み）

- **NULL検出とbootROMの衝突**：pico2は0x0にbootROM（SDK統合版はbootROM APIを
  呼ぶ）→ no-access範囲の縮小（0x0–0xFF）かRO化に留めるターゲット別判断
- **リージョン本数**（ARMv8-M=8・Hazard3 PMP=8）：静的3〜4本＋NULL1本に収める
- QEMUのMPU/PMPエミュレーション精度（mps2/icicle-kitは実績あり・要スモーク）
- 既定ON化による既存アプリへの影響（自己書換え等をしていれば顕在化＝それ自体が
  検出成果だが、回帰で確認してから既定を切り替える）

### スコープ外

- タスク間分離・ユーザ/カーネル分離（Zephyr `CONFIG_USERSPACE` 相当）
  ＝上位プロファイル **HRP3** の領分（カーネル本体差し替え級）
- `MPU_STACK_GUARD`（対象アーキなし。将来 v6m/v7m ターゲット追加時に再検討）
- linux（POSIX）ターゲットへの適用

## 実施結果

（完了時に記載）
