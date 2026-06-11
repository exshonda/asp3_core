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

（完了時に記載）
