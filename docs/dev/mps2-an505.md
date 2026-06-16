# QEMU Cortex-M33 ターゲットの mps2-an505 化（FPU 対応）

## 項目

QEMU 用 Cortex-M33（ARMv8-M）ターゲットを `mps2-an521`（SSE-200）から
`mps2-an505`（IoTKit）へ**置き換え**、ハードウェア FPU を有効化する。
AGENTS.md §1「機能追加計画」の「QEMUターゲット」系・「メモリ保護／FPU 検証」に関連
（M33 の FPU コンテキスト退避を QEMU 上で検証可能にする）。

## 内容

### 動機

旧 `mps2-an521` は QEMU で SSE-200（デュアル Cortex-M33）をモデル化するが、
ブートコアである **CPU0 に FPU が実装されていない**（QEMU `hw/arm/armsse.c` の
`sse200_properties`：`CPU0_FPU = false`，FPU を持つのは CPU1 のみ）。このため
CPACR の CP10/CP11 を有効化しても FP 命令は NOCP UsageFault となり、ソフトウェア
浮動小数点でしかビルドできなかった（64bit 値移動に使われる `vldr`/`vstr` で確実に
フォールトする）。

FPU を使うには、(A) `mps2-an521` で CPU1 を AMP 起動する（CPU0 が CPUWAIT を解放・
INITSVTOR1 設定・自身は WFI パーク。シングルコア RTOS には過剰）、(B) QEMU の
`-global sse-200.CPU0_FPU=true` で CPU0 に FPU を生やす（実機 AN521 から乖離する
ハック）、の2案があるが、いずれも不自然。

### 採用

`mps2-an505`（IoTKit）は **シングル Cortex-M33 で CPU0 が FPU を実装する**のが
ハードウェアの公式デフォルト（`iotkit_properties`：`CPU0_FPU = true`，
`num_cpus = 1`，`cpuwait_rst = 0`）。FPU の有効化に AMP も `-global` ハックも不要で、
**実機 AN505 と整合した正直な構成**になる。

さらに QEMU `hw/arm/mps2-tz.c` で **AN505 と AN521 はメモリマップ・RAM・oscclk・
init_svtor・UART 番地・IRQ 番号・割込み数（EXP_NUMIRQ=64）がすべて同一**
（ソース中コメント "AN521 is the same as AN505 here"）。違いは「SSE タイプ
（IoTKit vs SSE-200）＝シングル／FPU の有無」だけ。よって移植は既存 an521 ターゲット
ディレクトリのほぼクローンで済み、ハード定数の変更は不要。

## 実施プラン

1. `target/mps2_an521_gcc/` を `target/mps2_an505_gcc/` へ `git mv`。`mps2_an521.{h,ld}`
   も改名。ディレクトリ内の識別子を一括置換（`an521`→`an505`，`AN521`→`AN505`）。
2. `target.cmake` に FPU を追加：定義 `TOPPERS_FPU_ENABLE`／`TOPPERS_FPU_CONTEXT`／
   `TOPPERS_FPU_LAZYSTACKING`、フラグ `-mfloat-abi=softfp -mfpu=fpv5-sp-d16`
   （`pico2_arm_gcc`・`mimxrt685evk_gcc` と同一レシピ）。QEMU 起動を `mps2-an505` へ。
3. ボード説明（`mps2_an505.h`・`target_user.md`）を SSE-200／デュアル → IoTKit／
   シングル＋FPU 有効に修正。
4. ビルド配線（`CMakePresets.json` の include・presets.json・各種 docs・CI・
   テストスクリプト・TTSP3 資産）の参照を an505 へ更新。日付付き実施結果は注記方式で保持。
5. POSIX を介さず QEMU(mps2-an505) で build→sample→testexec→TTSP3 staticAPI を検証し、
   an521 ベースラインと比較して退行ゼロを確認。

## 実施結果

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
| `target/mps2_an505_gcc/`（旧 `mps2_an521_gcc/`） | `git mv` で改名。`mps2_an505.{h,ld}` へ改名。全ファイルの識別子を an505 化 |
| `target/mps2_an505_gcc/target.cmake` | FPU 定義3種＋`-mfloat-abi=softfp -mfpu=fpv5-sp-d16` を追加。QEMU マシン名を `mps2-an505` に。IoTKit/FPU の説明コメント追記 |
| `target/mps2_an505_gcc/target_kernel_impl.c` | 早期 FPU 有効化フックの裸 `CPACR`（arm_m.h が CMSIS 衝突回避のため未定義）を `CPACR_BASE` に修正（an521 時代は未コンパイルの死蔵コードだった） |
| `target/mps2_an505_gcc/mps2_an505.h`・`target_user.md`・`presets.json` | 「SSE-200／デュアル」→「IoTKit／シングル」、FPU 節を「無効」→「有効（FPv5-SP）」に書き換え |
| `CMakePresets.json`・`asp3_core.cmake`・`CMakeLists.txt` | include パス・例示プリセット名を an505 へ |
| `.github/workflows/{ci,nightly}.yml`・`scripts/ci/run_testexec.py`・`test/testexec.py` | M33 QEMU ジョブ・testexec・TTSP3 を an505 へ。`mps2` を含むジョブ名/成果物名は据え置き |
| `arch/arm_m_gcc/common/core_os_awareness.py` | コメント中の例示を an505 へ（EXTENDED・挙動変更なし） |
| `docs/`・`README.md`・`test/*/README.md`・`scripts/gdb_os_aware/README.md`・`target/pico2_arm_gcc/`（TTSP3 相互参照） | 現行向け参照を an505 へ。日付付き実施結果（`docs/dev/*.md`・`asp3_derivative_plan.md`・`ttsp3-conformance.md` 横断表）は原文保持＋注記方式 |

### 追加したファイル

- `docs/dev/mps2-an505.md`（本ファイル）

### 削除したファイル

- なし（`target/mps2_an521_gcc/` は削除ではなく `git mv` による改名）

### 据え置いたファイル

- `UPSTREAM_VERSION`：`MPS2_AN521依存部` は上流 ASP3 SVN の**上流側パッケージ名**を記述する
  ものであり、本リポジトリのターゲット改名とは独立（上流には AN521 が存在し続ける）。
  an505 ターゲットはこの上流 AN521 由来の an521 ターゲットから派生したもの。

### Git情報

- ベースコミット：`952d03e`
- 作業ブランチ：`feat/mps2-an505-replace-an521`
- ファイルリスト再現コマンド例：
  - `git diff --stat 952d03e -- target/mps2_an505_gcc`
  - `git log --oneline --follow -- target/mps2_an505_gcc/mps2_an505.ld`（an521 からの改名追跡）

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| POSIX | − | （M33 固有変更のため対象外） |
| QEMU (mps2-an505) build | ○ | `cmake --preset mps2_an505-qemu` → build OK。`objdump` で `vldr/vstr d7`（64bit FP）等の FP 命令が生成される（hard-float codegen） |
| QEMU (mps2-an505) sample | ○ | サンプルプログラム完走。旧 an521 では NOCP UsageFault になる FP 命令が実 FPU で実行される |
| QEMU (mps2-an505) testexec | ○ | task1/sem1/flg1/tmevt1/hrt1 = **5/5 PASS** |
| QEMU (mps2-an505) TTSP3 staticAPI | ○ | **PASS 100・FAIL 29・SKIP 9 / 138**。FAIL の件数・内訳は an521 ベースライン（FAIL 29・全 staticAPI 系）と**完全一致＝退行なし** |
| 実機 | − | AN505 は FPGA イメージ。本対応は QEMU 検証用 |

### FPU レシピの要点（再現用）

- 定義：`TOPPERS_FPU_ENABLE`（`core_kernel_impl.c` が CPACR・FPCCR 設定）／
  `TOPPERS_FPU_CONTEXT`（`core_support.S` が `S16-S31` 退避）／
  `TOPPERS_FPU_LAZYSTACKING`（`arm_m.h` の `FPCCR_INIT` 選択に必須。これが無いと
  `FPCCR_INIT undeclared` でビルド不成立）。
- フラグ：`-mfloat-abi=softfp -mfpu=fpv5-sp-d16`（Cortex-M33 は単精度 FPv5-SP）。

### DIVERGENCE_MAP との関連

- `target/mps2_an505_gcc/`（旧 `mps2_an521_gcc/`）は NEW（上流衝突なし）。
  DIVERGENCE_MAP.md の該当エントリを an505 へ更新済み。
- 関連：[`ttsp3-conformance.md`](ttsp3-conformance.md)（横断結果に an505 行・置換注記を追記）。
