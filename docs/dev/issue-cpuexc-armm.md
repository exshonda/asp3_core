# 調査メモ：arm_m で cpuexc1 / cpuexc4 が失敗する件（判断保留中）

> **状態：原因判明・対処方針は未決（ユーザーがゆっくり判断）**。
> 本ファイルは判断のための調査結果の整理。実装はまだ行っていない。
> 関連：`docs/dev/ci.md` 既知事項表（cpuexc行）／CI整備の「要調査」項目。

## 要約（結論を先に）

- `test_cpuexc1` / `test_cpuexc4` は **arm_m 系ターゲット（mps2_an521・pico2_arm・mimxrt685evk）で失敗**する
  （`Unregistered Exception occurs. Excno = 00000003`＝HardFault）。
- 原因は **arm_m の `SIL_LOC_INT()` が PRIMASK（`cpsid i`）で全割込みをロック**するため、
  その区間で発生した CPU 例外（UsageFault）が優先度マスクで取れず **HardFault にエスカレート**すること。
- **この PRIMASK 実装は genuine 上流 TOPPERS/ASP3 arm_m と完全一致**（バイト一致を確認）。
  つまり**我々のバグではなく、上流 arm_m 固有の特性**。
- 実験で「`SIL_LOC_INT` を BASEPRI ロックに変えると cpuexc1/4 は通る」ことは**確認済み**。
  ただしそれは**上流からの乖離**であり、割込みロックの意味論変更・実機再検証を伴う。
- RISC-V（pico2_riscv・polarfire）と Cortex-A（zcu102）では **元から PASS**（後述）。

## 症状（再現手順と出力）

```bash
mkdir CE && cd CE
echo '--preset mps2_an521-qemu' > TARGET_OPTIONS
echo 'timeout 30 qemu-system-arm -machine mps2-an521 -nographic \
  -semihosting-config enable=on,target=native -kernel asp.elf' > TARGET_RUN
python3 <ASP3>/test/testexec.py cpuexc1
```

```
Check point 1 passed.
Check point 2 passed.
Check point 3 passed.
Unregistered Exception occurs.
Excno = 00000003 PC = 100002a0 XPSR = 8100000f basepri = 00000100, p_excinf = ...
```

`Excno = 3` は HardFault。check_point 3 の直後（CPU例外ハンドラ check_point 4 の手前）で落ちる。

## 根本原因（メカニズム）

### テストの構造（`test/test_cpuexc1.c`）
```c
/* alarm1_handler 内 */
check_point(3);
SIL_LOC_INT();            /* ← 全割込みロック */
RAISE_CPU_EXCEPTION;      /* ← udf #0（未定義命令）＝CPU例外 */
check_point(6);
SIL_UNL_INT();
```
- `RAISE_CPU_EXCEPTION` ＝ `Asm("udf #0")`（`arch/arm_m_gcc/common/core_test.h`）。
- `udf #0` は本来 **UsageFault**（`EXCNO_USAGE = 6`）を起こす。
- テストは `DEF_EXC(CPUEXC1, { TA_NULL, cpuexc_handler })` で例外ハンドラを登録
  （`test/test_cpuexc.cfg`）。`CPUEXC1` は `core_test.h` で：
  ```c
  #if __TARGET_ARCH_THUMB >= 4
  #define CPUEXC1 6 /* Usage fault */    ← mps2 は __TARGET_ARCH_THUMB=5 でこちら
  #else
  #define CPUEXC1 3 /* Hard Fault */
  #endif
  ```
  → **UsageFault(6) 用にハンドラを登録**している。

### arm_m の SIL_LOC_INT（`arch/arm_m_gcc/common/core_sil.h`）
```c
Inline bool_t
TOPPERS_disint(void)
{
    uint32_t val;
    Asm("mrs  %0, PRIMASK" : "=r"(val));
    Asm("cpsid i":::"memory");      /* ← PRIMASK=1：実行優先度を 0 に昇格 */
    ...
}
#define SIL_LOC_INT()  ((void)(TOPPERS_locked = TOPPERS_disint()))
```

### なぜ HardFault に昇格するか（ARMv8-M 仕様）
1. `SIL_LOC_INT()` が **PRIMASK=1** → 実行優先度が **0** に昇格。
2. カーネルは UsageFault の優先度を **0** に設定している
   （`core_kernel_impl.c` `set_exc_int_priority(EXCNO_USAGE, 0)`。CPUロック中でも
   取れるように…という意図のコメント付き）。
3. ところが PRIMASK による実行優先度 0 に対し、UsageFault 優先度 0 は
   **「厳密に高い（数値的に小さい）」ではない**ため**プリエンプトできず取れない**。
4. 取れない同期フォルトは **HardFault にエスカレート**（ARMアーキの priority escalation）。
5. HardFault(Excno 3) 用の ASP3 ハンドラは未登録 → 「Unregistered Exception」。

> 補足：カーネルの **CPUロック（`loc_cpu`）は BASEPRI** を使う（`TIPM_LOCK=TMIN_INTPRI`）。
> BASEPRI ロックなら優先度0フォルトは取れる。**食い違いは「SILの全割込みロックが
> PRIMASK」である点に限られる**。

## 上流との関係（重要）

genuine 上流 TOPPERS/ASP3 arm_m の `core_sil.h` を **3系統独立に参照**し、
いずれも `SIL_LOC_INT = cpsid i（PRIMASK）` で**現行とバイト一致**：

| 参照元 | パス |
|---|---|
| pristine import | `/home/honda/TOPPERS/asp3core/base/asp3/arch/arm_m_gcc/common/core_sil.h` |
| TOPPERSテスト環境 | `/home/honda/TOPPERS/TTSP3/work/asp3_3.7/arch/arm_m_gcc/common/core_sil.h` |
| FMP3同梱asp3 | `/home/honda/TOPPERS/FMP3/posix/asp3_3.7/arch/arm_m_gcc/common/core_sil.h` |

→ **本件は上流 arm_m 固有の特性**。`core_test.h`（これも genuine TOPPERS 製）が
Thumb≥4 で `CPUEXC1=6` を期待することと、SIL が PRIMASK である事実は
上流内部でも食い違っており、**cpuexc1/4 は PRIMASK ベースの arm_m SIL では
そもそも成立しない**（＝上流でも arm_m では通らない／除外されている可能性が高い）。

## 他アーキとの対比（なぜ他は通るか）

| ターゲット | SIL_LOC_INT の機構 | CPU例外の扱い | cpuexc1/4 |
|---|---|---|---|
| mps2_an521・pico2_arm（arm_m） | **PRIMASK**（cpsid i） | UsageFault→HardFault昇格 | **失敗** |
| zcu102（arm64・Cortex-A） | DAIF（IRQ/FIQマスク） | 同期例外は別経路で常に取れる | PASS |
| pico2_riscv（Hazard3）・polarfire（RISC-V） | mstatus.MIE 等 | 同期例外（trap）は割込みマスクと独立に常に取れる | PASS |

RISC-V/Cortex-A では**同期例外（CPU例外）が割込みマスクと独立に必ず取れる**ため、
SIL_LOC_INT 中でもハンドラに入る。Cortex-M は「フォルトも優先度体系の一部」で
PRIMASK がそれを止めるため、構造的に異なる。

## 実験で確認した事実（2026-06-11）

`TOPPERS_disint` を一時的に **BASEPRI ロック（`IIPM_LOCK = INT_IPM(TMIN_INTPRI)`。
mps2 では `0xA0`）** に書き換えて検証：

```
cpuexc1: All check points passed.
cpuexc4: All check points passed.
```

→ **BASEPRI ロックにすれば両テストは通る**ことを確認（原因の確証）。
実験パッチは検証後に**完全に元へ戻し済み**（`core_sil.h` は上流一致のまま）。

## 対処の選択肢

### 案① SIL_LOC_INT を BASEPRI ロックに変更（cpuexc1/4 を通す）
- **内容**：`TOPPERS_disint` を Thumb≥4 で `cpsid i`→`BASEPRI=IIPM_LOCK` に。
- **利点**：cpuexc1/4 が通る。`core_test.h` の `CPUEXC1=6` 期待と整合。
- **欠点／リスク**：
  - **上流 TOPPERS/ASP3 arm_m からの意図的乖離**（DIVERGENCE_MAP管理対象が増える）。
  - **`SIL_LOC_INT`（全割込みロック）の意味論変更**：BASEPRI ロックは
    *カーネル管理外*の高優先度割込み（BASEPRIレベルより上）を**ロックしない**。
    「全割込みロック」の語義（PRIMASK＝本当に全部止める）に反する。
  - `SIL_PRE_LOC` が `bool_t TOPPERS_locked` 固定 → BASEPRI の旧値を保持できず、
    **ネスト時の正しい save/restore に別設計が要る**（実験パッチは未対応）。
  - mps2・pico2_arm の**実機での割込み挙動の再検証**が必要。
- **影響範囲**：arm_m 全ターゲット（mps2_an521・pico2_arm・mimxrt685evk〔2026-06-12 実機で同一症状を確認〕・将来のarm_m）。

### 案② 上流準拠のまま「制限」として明文化・除外（推奨）
- **内容**：`core_sil.h` は上流忠実のまま。arm_m で cpuexc1/4 を
  「上流arm_m固有の制限（PRIMASKベースSILではSIL_LOC_INT中のCPU例外がHardFault昇格）」
  として**意図的除外**と記録（dlynse・polarfire int1 と同じ扱い）。CI/nightlyの
  arm_m テストリストから除外済みの状態を、理由付きで確定させる。
- **利点**：上流追従の方針（禁則①の精神）に沿う。battle-tested な割込みロック機構を
  2テストのために変えない。低リスク。
- **欠点**：arm_m で cpuexc1/4 は永続的に非対象（機能テスト1.5本ぶん）。

### 案③（折衷・将来）テスト側で arm_m の挙動に合わせる
- `CPUEXC1` を arm_m かつ「SIL_LOC_INT=PRIMASK」の構成では **3（HardFault）** とみなし、
  HardFault ハンドラで CPU 例外を捕捉する作り。ただし `core_test.h` は genuine TOPPERS
  ファイルであり、テスト本体の改変は上流テストとの乖離になる。**非推奨**（記録のみ）。

## 担当者の見解（提案）

**案②** を推す。理由：
- 本プロジェクトの根幹方針は**上流追従**であり、`core_sil.h` は上流とバイト一致。
- ①は「修正」ではなく「上流からの割込みロック意味論の乖離」で、実機影響と
  ネスト対応の追加設計を伴う。得られるのは cpuexc1/4 の2本のみ。
- 上流 TOPPERS 自身、arm_m ではこのテストが PRIMASK SIL と食い違っており、
  arm_m では成立しないテストと整理するのが自然。

ただし最終判断はユーザーに委ねる（本ファイルはそのための資料）。

## 判断後にやること（メモ）

- **②に決めた場合**：`docs/dev/ci.md` の cpuexc 行を「上流準拠の制限・意図的除外」で確定。
  nightly.yml / ci.yml のコメントを更新（既に除外済み・理由を明文化）。本ファイルの状態を「確定（②）」に。
- **①に決めた場合**：`feat/sil-basepri-lock` ブランチで
  `TOPPERS_disint`/`TOPPERS_enaint` を BASEPRI 化（ネスト save/restore の設計含む）→
  DIVERGENCE_MAP に「arch/arm_m_gcc/common/core_sil.h 改変（上流乖離・理由）」を追加 →
  mps2・pico2_arm の全 testexec 回帰＋実機確認 → cpuexc1/4 を CI/nightly に追加。
