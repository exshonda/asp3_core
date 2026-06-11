# FSP統合（Renesas RA）

## 項目

FSP統合（AGENTS.md §1 機能追加計画、優先度：中。第一目的「各社SDKとの協調動作」の第2弾＝Pico SDKに続く）

## 内容

Renesas FSP（Flexible Software Package）と TOPPERS/ASP3 を協調動作させる。
Pico SDK統合（`docs/dev/pico-sdk-integration.md`）と同じ「**SDK固有の arch/target/
アプリを外側リポジトリ（asp3_fsp）で管理し、asp3_core は submodule＋`ASP3_TARGET_DIR`
で受け入れる**」方式へ移行する。

### 意義
1. **第一目的の第2弾**：ASP3アプリからFSPのHAL/ドライバ（BSP・各ペリフェラル）を
   使える。Renesas RA（Cortex-M33/M85）への展開。
2. **受け入れ口の汎用性の実証**：Pico統合で整備した `ASP3_TARGET_DIR`／
   `ASP3_LIBRARY_ONLY` が、**別SDK（FSP・別ツールチェーン）でも機能する**ことの確認。
3. submodule構成の2例目（pico に続く）。FSP→STM32 への横展開の足がかり。

### 現状（2026-06-11 調査・`../asp3_fsp` をclone）

旧 asp3_pico_sdk と同じ「**カーネル同梱＋fork CMakeLists**」構成（未submodule化）：

```
asp3_fsp/
├── asp3/                          ← カーネル同梱（fork）
│   ├── arch/arm_m_gcc/{ra6m5_fsp, ra8m2_fsp, common}
│   ├── target/{ek_ra6m5, ek_ra8m2}      ← FSPターゲット依存部
│   ├── asp3_fsp.cmake             ← FSP glue（ほぼスタブ）
│   └── CMakeLists.txt             ← asp3_core CMake の fork
├── ek_ra6m5/sample/               ← EK-RA6M5 アプリ（RASCプロジェクト）
└── ek_ra8m2/                      ← EK-RA8M2 アプリ
```

### Pico統合と同じ（移行が容易）
- **TECSレス済み**（tecsgenなし・プレーンsyssvc）→ asp3_core と親和
- **cfg Python版**（cfg.py/pass1.py/pass2.py）→ asp3_core と同系
- **arm_m**（RA6M5=Cortex-M33／RA8M2=Cortex-M85）→ 既存 arch 系列
- asp3_core側の受け入れ口（`ASP3_TARGET_DIR`・`ASP3_LIBRARY_ONLY`）は**整備済み**
- FSP glue（`asp3_fsp.cmake`）は **picoより軽量**：`asp3_set_fsp_options()` は空
  （pico の `--wrap` による割込み奪取が不要）

### FSP固有の論点（Pico SDKとの相違）
1. **RASC生成依存**：アプリは e2studio / RA Smart Configurator（RASC）プロジェクト
   （`configuration.xml`）。ビルドに要る `ra/`・`ra_gen/`（FSP生成コード）は
   **リポジトリに無く**、configure時に **RASC を実行して生成**する
   （`RASC_EXE_PATH`＝`~/.renesas/platform/sc/ra/fsp_6.2.0/eclipse/rasc.exe`・FSP 6.2.0）。
   → **RASC・VSCode RA拡張は本マシンに導入済み**＝実ビルド検証が可能。
2. **ツールチェーンが LLVM-ARM（clang）＋ picolibc**（`--compiler LLVMARM`・ATfE 21.x）。
   既存の全ターゲット（gcc：arm-none-eabi／aarch64／riscv64）と**異なる**。
   `cmake/` のツールチェーンファイル群に clang 系を追加する必要がある（または
   asp3_fsp 側で保持）。
3. **リンカスクリプト**：RASC生成（`fsp_gen.lld`）＋ ASP3固有セクションの統合
   （sample CMakeLists にコメントあり）。
4. **実機**：EK-RA6M5・EK-RA8M2（Renesas評価ボード）。QEMUモデルは（基本的に）無い。

## 実施プラン

### asp3_core側（このリポジトリ）
受け入れ口は Pico統合で整備済み。FSP特有で追加が要るもののみ：

1. **clang/LLVM-ARM ツールチェーンファイル**：`cmake/toolchain-arm-clang.cmake`（仮）を
   追加するか、FSP固有として asp3_fsp 側に置くかを判断（既存の gcc toolchain と並立）。
   asp3_core を汚さない方針なら **asp3_fsp 側に置き `ASP3_TARGET_DIR` と併用**が筋。
2. 既存ターゲットに影響しないこと（clang追加は新規ファイルのみ＝CI green維持）を確認。
3. PORTING_GUIDE の「外部（SDK）ターゲットの置き方」に **別ツールチェーン併用**の注記を追補。
4. 記録：DIVERGENCE_MAP（必要なら）・README索引・本ファイル。

> Pico統合の経験上、asp3_core側の実変更は最小（受け入れ口は既存）。
> FSP固有実装の主体は **asp3_fsp リポジトリ側**。

### asp3_fsp側（別リポジトリ作業・Pico A案と同型）
1. submodule化：`asp3/asp3_core`（純カーネル）を submodule 参照、
   bundled `asp3/` の fork CMakeLists を asp3_core 正準＋`ASP3_TARGET_DIR` へ。
2. SDK固有 arch/target（`arm_m_gcc/{ra6m5_fsp,ra8m2_fsp}`・`target/{ek_ra6m5,ek_ra8m2}`）を
   外側で保持し `ASP3_TARGET_DIR` で供給。
3. アプリ（ek_ra6m5/ek_ra8m2 sample）の RASC生成フロー＋clangツールチェーンを維持。
4. **本マシンで実ビルド検証**（RASC導入済み）：RASC生成 → clangビルド →
   （可能なら）実機 EK-RA6M5/RA8M2 で sample1・testexec。
5. submodule の asp3_core を最新 main に追従。

## スコープ外
- STM32統合（同じ受け入れ口で後日）
- FSPの全ペリフェラルドライバのASP3連携（まず sample1 起動＝BSP＋SIO）
- QEMU対応（RA評価ボードのQEMUモデルは想定しない）
- CIでのFSPビルド（RASC＋clang＝GitHubランナーに無い。pico実機同様 CI対象外
  ＝環境のあるマシンで検証）

## リスク・確認事項
- **RASC生成の再現性**：`configuration.xml` から生成される `ra/`・`ra_gen/` が
  ツールバージョン（FSP 6.2.0）に依存。生成物はコミットしない方針か要判断。
- **clang/picolibc と ASP3 の相性**：crt0/_start の扱い（sample CMakeにATfE 21.x の
  crt0引き込み回避コメントあり）。既存gccターゲットと別系統のため切り分け必須。
- asp3_fsp の bundled `asp3/` が現 asp3_core main からどれだけ乖離しているか
  （submodule化時に差分を確認）。

## 実施結果

### asp3_core側 — 完了（2026-06-11）

受け入れ口（`ASP3_TARGET_DIR`・`ASP3_LIBRARY_ONLY`）は Pico統合で整備済み。FSP特有の追加は
clang/LLVM-ARM ツールチェーンだが、**asp3_core を汚さない方針で asp3_fsp 側に置く**と判断
（プラン推奨どおり）。asp3_core 側の実変更は doc のみ：

- `docs/porting/PORTING_GUIDE.md`「外部ターゲット」節に **別ツールチェーン併用**の指針を追補
  （ツールチェーンファイルは外部SDK側／asp3 lib・cfg1_out は最上位ツールチェーンを継承＝asp3_core非依存／
  cfg1_out が SDK生成ヘッダに依存する場合は `add_subdirectory(asp3_core)` 後に `add_dependencies(cfg1_out …)`）。
- コミット `5415b79`（branch `feat/fsp-porting-guide-note` → CI 全9 green → main）。既存ターゲット無影響。

### asp3_fsp側 — 調査結果（2026-06-11・中間／実ビルドは未）

`../asp3_fsp`（clone）を調査。現状は旧 asp3_pico_sdk と同型の「カーネル同梱＋fork CMakeLists」。

**ツール（本マシン導入済み）**
- RASC：`~/.renesas/platform/sc/ra/fsp_6.4.0/eclipse/rasc`（Linux ネイティブ・ヘッドレス起動可）
- LLVM-ARM：ATfE **21.1.1**（`~/.renesas/platform/arm-llvm/21.1.1/ATfE-21.1.1-Linux-x86_64/bin/clang`）

**移行で対応が要る点（CMake一本化の実装メモ）**
- `target/ek_ra6m5/target.cmake`：`${PROJECT_SOURCE_DIR}` 基準を **外部規約**へ
  （チップarch `arch/arm_m_gcc/ra6m5_fsp` と target は `CMAKE_CURRENT_LIST_DIR` 相対、共通arch
  `arch/arm_m_gcc/common` は `ASP3_ROOT_DIR`＝submodule）。
- target.cmake 末尾の `add_dependencies(${CFG1_OUT} generate_content_…)` は、canonical asp3_core では
  `cfg1_out` が target.cmake include 後に生成されるため**ここでは未生成**。→ サンプル CMakeLists の
  `add_subdirectory(asp3_core)` 後に移すこと（PORTING_GUIDE の注記どおり）。
- FSP生成ヘッダ（`ra/`・`ra_cfg/`・`ra_gen/`）依存：cfg1_out も FSP ヘッダを要するため RASC 生成に依存させる。
- `ek_ra6m5/sample/Config.cmake` が **Windows前提**（`rasc.exe`・`python.exe`・既定 `fsp_6.2.0`）。
  Linux では `RASC_EXE_PATH=…/fsp_6.4.0/eclipse/rasc`・`PYTHON_EXE=python3` 等の上書きが要る。
- リンカ：`cmake/asp3_sections.lld`（ASP3固有 `.vector`/`.empty.*` を FLASH 配置）＋ `-nostartfiles`
  （ATfE の picolibc crt0 回避）は移行後も維持。

**実ビルドの現ブロッカー（要ユーザー判断）**
- `ek_ra6m5/ek_ra8m2` の `configuration.xml` は **FSP 6.2.0**（`#FSPVersion#=6.2.0`・各コンポーネント V6.2.0）。
  導入済み RASC は **6.4.0** のみ → RASC `--generate` が「Failed to locate component … (V6.2.0) in any
  software packs」で失敗（`ra/`・`ra_gen/` 未生成）。生成失敗時の GUI エラー表示で GTK クラッシュ（副次）。
- 解消には①プロジェクトを **FSP 6.4.0 へ移行**（VSCode RA拡張で開くと移行／RASC migrate）か、
  ②**FSP 6.2.0 パックを導入**（Renesas pack）のいずれかが必要。いずれも Renesas GUI/パック操作＝ユーザー領域。

### asp3_fsp側 submodule化＋CMake一本化 — EK-RA6M5 ビルド検証 完了（2026-06-11）

FSP 6.2.0 パック導入後、A案（Pico同型）で移行・**EK-RA6M5 を実ビルド検証**（`../asp3_fsp`
ブランチ `feat/asp3-core-submodule`・コミット `28ba8a6`・**未push**＝ユーザーが push）。

| 区分 | 内容 |
|---|---|
| submodule化 | bundled `asp3/`（fork）の純カーネル部を削除し `asp3/asp3_core`（submodule＠`473d97e`）へ。FSP固有部（`asp3_fsp.cmake`・`arch/arm_m_gcc/{ra6m5_fsp,ra8m2_fsp}`・`target/{ek_ra6m5,ek_ra8m2}`）のみ残置 |
| CMake一本化 | `asp3_fsp.cmake` で `ASP3_TARGET_DIR`/`ASP3_CORE_DIR`/`ASP3_ROOT_DIR` 解決。sample は fork CMake 廃止→`add_subdirectory(asp3_core, ASP3_LIBRARY_ONLY)`＋`asp3_add_syssvc`。`target.cmake`/`arch.cmake` を外部規約へ |
| RASCフラグ伝播 | `RASC_CMAKE_C_FLAGS`/`_DEFINITIONS` を `ASP3_COMPILE_OPTIONS`/`_DEFS` へ、`${CMAKE_SOURCE_DIR}`（`bsp_linker_info.h` 等）を include へ。cfg1_out は最小リンク（`--target=arm-none-eabi -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -nostartfiles -nostdlib`＝FSP実リンカスクリプト不要） |
| cfg1_out依存 | `add_dependencies(cfg1_out generate_content_<proj>)` を sample の add_subdirectory 後へ移設 |

**asp3_core側の必須修正（CMSIS協調）— main `473d97e`**：
`arch/arm_m_gcc/common/arm_m.h` の bareな `#define CPACR`/`FPCCR` を廃止（CMSIS の
レジスタ構造体メンバ名と衝突し `core_cm33.h` でコンパイル不能）。`CPACR_BASE`（既存）＋
`FPCCR_ADDR`（新規）に置換、`core_kernel_impl.c` の FPU有効化2箇所を追随。
mps2_an521/pico2_arm 無影響を確認・CI 全9 green・DIVERGENCE_MAP 記録。

**検証（EK-RA6M5・RASC 6.2.0＋ATfE clang 21.1.1）**：
- 環境：`ARM_TOOLCHAIN_PATH=~/.renesas/platform/arm-llvm/21.1.1/ATfE-21.1.1-Linux-x86_64/bin`、
  `RASC_EXE_PATH=~/.renesas/platform/sc/ra/fsp_6.2.0/eclipse/rasc`、`-DCMAKE_TOOLCHAIN_FILE=cmake/llvm.cmake`。
- RASC `--generate` で `ra/`・`ra_cfg/`・`ra_gen/` 生成（Linux ヘッドレス可）→ clang ビルドで
  `asp3_fsp.elf`/`.hex`/`.bin` 生成（text 30,770 / data 292 / bss 13,684）。
- cfg 3パス（cfg1_out→offset.h→kernel_cfg.c/h）実行、ASP3カーネル（`_kernel_*`/`sta_ker`）と
  FSP（`R_BSP_*`/`R_IOPORT_Open`）が同一ELFにリンクされることを確認。
- RASC は終了時に GUI エラーダイアログ表示で GTK クラッシュするが生成物には影響なし（無害）。

> 未完（後続）：①EK-RA8M2（Cortex-M85）は同型に CMake 移行済みだが**実ビルド未検証**
> （RASC生成＋clangビルドの確認）。②実機（EK-RA6M5/RA8M2）での sample1 動作確認。
> ③`Config.cmake` の Windows前提（`rasc.exe`/`python.exe`/既定6.2.0）の Linux/OS非依存化
> （現状は `-DRASC_EXE_PATH=` 等の上書きで対応）。④STM32統合（同じ受け入れ口で後日）。
