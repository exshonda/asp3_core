# Pico SDK統合

## 項目

Pico SDK統合（AGENTS.md §1 機能追加計画、優先度：中。第一目的「各社SDKとの協調動作」の第1弾）

## 内容

Raspberry Pi Pico SDK と TOPPERS/ASP3 を**協調動作**させる。これまでの
`raspberrypi_pico2_gcc`／`_riscv_gcc`（SDK非依存ベアメタル）と対になる
「**SDK同居**」構成を、**外側のSDKリポジトリ（asp3_pico_sdk）側で管理**する形で実現する。

### 意義（何が嬉しいか）

1. **第一目的の第1弾**：ASP3アプリから pico-sdk のドライバ資産
   （USB CDC・PIO・DMA・Flash・Pico W WiFi等）を直接呼べる。基盤整備
   （高優先13項目）はこのための土台だった。
2. **リポジトリ分離による上流追従の保護**：SDK固有の arch/target を
   asp3_core に入れず外側で持つことで、**asp3_core を純カーネルに保てる**
   （上流マージ・DIVERGENCE_MAP管理が軽くなる）。
3. **submodule構成への移行の実証**：`docs/FILE_PLAN.md` §2 の
   「既存3リポジトリ→asp3_coreをsubmodule参照」構想の最初の実例になる
   （FSP・STM32統合の雛形）。

### 移植元（実証済みの仕組み・確認済み）

`github.com/exshonda/asp3_pico_sdk`：`asp3_pico_sdk.cmake`（`PICO_PLATFORM`→
`ASP3_TARGET` 解決・`asp3_set_pico_sdk_options()`）＋ `target/rp2350-arm-s_pico_sdk/`。
共存の要は **pico-sdkの割込み登録API（`irq_set_exclusive_handler`等8関数）を
リンカ `--wrap` でASP3管理へ誘導**＋ `PICO_RUNTIME_SKIP_INIT_*` でSDKの
ベクタ/IRQ優先度初期化を抑止する点（ASP3がNVICを掌握）。
**RISC-Vは asp3_pico_sdk でも未対応**（ARM-S版を先行）。

## 目標構成（本セッションで合意）

```
asp3_pico_sdk/                  ← 外側リポジトリ（後で _sample からリネーム）
├── asp3_core/                  ← submodule（純カーネル：kernel/ cfg/ syssvc/ arch共通部）
├── arch/                       ← SDK向けチップ依存部（arm_m/rp2350・riscv/rp2350）
├── target/                     ← SDK向けターゲット依存部（ASP3_TARGET_DIRで指定）
├── sample1/                    ← アプリ（ISAで分けない＝PICO_PLATFORMで切替）
│   ├── sample1.c / sample1.cfg
│   └── CMakeLists.txt
└── CMakeLists.txt              ← asp3_pico_sdk.cmake相当（PICO_PLATFORM→target解決）
```

### 設計判断（本セッションの検討結果）

- **arch/target（カーネル依存部）はISAごとに2つ必要**（arm_m/rp2350＋riscv/rp2350）。
  ISAが根本的に違う（割込み＝NVIC vs Xh3irq・ブート＝ARM/RISC-V IMAGE_DEF）。
  ※共通arch（`arch/arm_m_gcc/common`・`arch/riscv_gcc/common`）とcfgエンジン・
  カーネル本体は asp3_core 側（submodule）に残す＝`ASP3_ROOT_DIR` 参照のまま。
- **アプリ（sample1）はISAで分けない＝マージ**。同一ボード(RP2350)でソースは
  ISA非依存。差はツールチェーンと `PICO_PLATFORM` だけ（pico-sdk idiomに一致）。

## asp3_core側の作業（このリポジトリ）

外部リポジトリから arch/target を供給可能にする**最小拡張**：

1. **`ASP3_TARGET_DIR` 変数の導入**（`asp3_core.cmake`・後方互換）
   ```cmake
   if(NOT DEFINED ASP3_TARGET_DIR)
       set(ASP3_TARGET_DIR ${ASP3_ROOT_DIR}/target/${ASP3_TARGET})
   endif()
   # 存在確認とinclude を ASP3_TARGET_DIR 基準に
   ```
   未指定時は従来どおり（既存7ターゲットに影響なし＝CI green維持）
2. **外部target.cmakeの参照規約を明文化**：
   - 外部チップ依存部のincludeは `${CMAKE_CURRENT_LIST_DIR}` 相対で行う
   - 共通arch（`arch/<core>/common/arch.cmake`）・cfgテンプレート
     （core_*.py）は `${ASP3_ROOT_DIR}` 参照（submodule側）でよい
   - cfgテンプレート・Cソースは `ASP3_*_FILES` に**絶対パス**で供給（既存規約）
3. **検証用に最小の外部ターゲットでスモーク**（asp3_core単体CIを汚さず）：
   既存ターゲットを一時的に外部ディレクトリへコピーし `ASP3_TARGET_DIR` で
   ビルドが通ることを確認（POSIX or mps2で十分）
4. **PORTING_GUIDE.md に「外部（SDK）ターゲットの置き方」節を追加**
5. 記録：DIVERGENCE_MAP（asp3_core.cmake＝NEW改修）・README索引・本ファイル

> asp3_core側の変更はこの **CMakeエントリ1点の後方互換拡張のみ**。
> SDK固有の arch/target/sample の実装は **asp3_pico_sdk リポジトリ側**で行う
> （本計画のスコープはasp3_core側の受け入れ口整備まで。SDK側実装は別リポジトリ作業）。

## asp3_pico_sdk側の作業（別リポジトリ・参考）

本リポジトリのスコープ外だが、受け入れ口設計の妥当性確認のため記載。

### リポジトリ方針：**A案＝既存sampleを再利用**（新規作成しない）

既存2リポジトリを `../`（asp3_core の隣）にclone済み・実体確認（2026-06-07）：

| clone | 実体 | 移行での役割 |
|---|---|---|
| `../asp3_pico_sdk` | カーネル同梱（arch/cfg/include/kernel/library/syssvc/target＋**独自CMakeLists.txt**＝asp3_core CMakeのpico版fork＋`asp3_pico_sdk.cmake`） | **移植元**。archive（読取専用）して参照点に |
| `../asp3_pico_sdk_sample` | **submodule `asp3`→asp3_pico_sdk 済み**・`sample1/`（`pico_sdk_import.cmake`＋動作するCMakeLists：PICO_BOARD pico2・sdk 2.1.1・toolchain 14_2_Rel1・`asp3_set_pico_sdk_options`＝--wrap） | **再利用の土台**。最終的に `asp3_pico_sdk` へ改名 |

→ submodule・pico_sdk_import・--wrap設定など**動作実績のある資産が sample 側に揃っている**ため、新規作成せず `asp3_pico_sdk_sample` を改修する（A案）。

### 移行手順（sample側＝別リポジトリ作業）

1. **旧 `asp3_pico_sdk` を archive**（または `_legacy` 改名）。改名衝突を回避し移植元として温存
2. **submoduleの差替**：`asp3_pico_sdk_sample` の submodule を
   旧 `asp3_pico_sdk` → **`asp3_core`** へ（`.gitmodules` のurl変更・path `asp3`→`asp3_core`）
3. **pico固有部を sample 側へ移設**（旧 asp3_pico_sdk から）：
   - `asp3_pico_sdk.cmake`（`PICO_PLATFORM`→`ASP3_TARGET`＋**`ASP3_TARGET_DIR`** 解決・
     `asp3_set_pico_sdk_options`＝`irq_*` の--wrap）→ sample repo ルート
   - `target/rp2350-arm-s_pico_sdk` → sample repo の `target/`
   - チップ依存部：asp3_core の `arch/arm_m_gcc/rp2350` を使うか、SDK版を
     sample側 `arch/` に持つかは実装時に判断（共通arch・cfgエンジンは asp3_core 参照のまま）
4. **sample1 CMakeLists の付替**：現状 `include(../asp3/asp3_pico_sdk.cmake)`＋
   `add_subdirectory(${ASP3_CMAKE_DIR})`（＝旧fork CMake）を、
   **asp3_core の正準 CMakeLists ＋ `ASP3_TARGET_DIR`** を使う形へ
   （旧 asp3_pico_sdk の fork CMakeLists.txt は廃止＝asp3_coreに一本化）
5. sample1 を単一フォルダ化（`PICO_PLATFORM` で arm/riscv 切替）
6. **タイマ競合の解決**：SDK alarm（TIMER0）と ASP3 HRT（TIMER0）の調停
7. RISC-V版は後続（asp3_pico_sdk未対応・本リポジトリの
   `raspberrypi_pico2_riscv_gcc` のXh3irq実装を移植元にできる）
8. 検証は実機（pico-sdk必須・CIはビルドまで）
9. 安定後に `asp3_pico_sdk_sample` → `asp3_pico_sdk` へ改名

> 旧 `asp3_pico_sdk` の `CMakeLists.txt` は asp3_core の CMakeLists を pico向けに
> fork したもの（`include(${PROJECT_SOURCE_DIR}/target/${ASP3_TARGET}/target.cmake)`）。
> 新構成では **asp3_core の CMakeLists に一本化**し、pico差分は `asp3_pico_sdk.cmake`
> ヘルパ＋外部 target（`ASP3_TARGET_DIR`）に閉じる＝fork解消が移行のキモ。

## スコープ外

- FSP統合・STM32統合（同じ受け入れ口＝`ASP3_TARGET_DIR` で後日同様に）
- asp3_pico_sdk リポジトリ側の実装そのもの（別リポジトリ作業）
- asp3_core を各SDKリポジトリのsubmoduleに登録する運用切替（FILE_PLAN §2）

## リスク・確認事項

- **タイマ競合**（SDKのTIMER0 alarm と ASP3 HRT）— SDK側実装の最大論点
- 外部target.cmakeが `ASP3_ROOT_DIR` と `CMAKE_CURRENT_LIST_DIR` を取り違えると
  パス解決が壊れる → 規約を明文化＋スモークで担保
- pico-sdk のバージョンピン（devcontainer/CIへの追加可否はSDK側で判断）

## 実施結果

### asp3_core側（受け入れ口整備）— 完了（2026-06-10）

外部リポジトリ（各社SDK）が `target/` を本リポジトリ外に置けるよう、CMakeエントリに
**後方互換の `ASP3_TARGET_DIR` 変数**を導入した。SDK固有の arch/target/sample の実装は
asp3_pico_sdk リポジトリ側の作業（本リポジトリのスコープ外）。

#### 変更したファイル

| ファイル | 変更内容 |
|---|---|
| `asp3_core.cmake` | `ASP3_TARGET_DIR` を導入。未定義時は `${ASP3_ROOT_DIR}/target/${ASP3_TARGET}`（従来動作）を既定とし、存在確認を `${ASP3_TARGET_DIR}/target.cmake` 基準へ変更。外部指定時のエラーメッセージに `-DASP3_TARGET_DIR=<dir>` の案内を追加 |
| `CMakeLists.txt` | `target.cmake` の include を `${ASP3_ROOT_DIR}/target/${ASP3_TARGET}/...` → `${ASP3_TARGET_DIR}/...` へ変更（解決は `asp3_core.cmake` に一元化） |
| `docs/porting/PORTING_GUIDE.md` | Step 6 に「外部（SDK）ターゲットの置き方（`ASP3_TARGET_DIR`）」節を追加。参照規約（チップ依存部＝`CMAKE_CURRENT_LIST_DIR` 相対／共通arch・cfgテンプレート＝`ASP3_ROOT_DIR`／`ASP3_*_FILES`＝絶対パス）を明文化 |
| `DIVERGENCE_MAP.md` | `asp3_core.cmake` 行に `ASP3_TARGET_DIR`（外部SDKターゲット受け入れ口＝後方互換拡張）を追記 |
| `docs/dev/README.md` | Pico SDK統合の状態を「計画中」→「実施中（asp3_core側 完了）」へ更新 |

追加・削除ファイル：なし（既存ファイルの後方互換拡張のみ）。

#### Git情報

- ベースコミット：`c6a0079`（main）
- ブランチ：`feat/pico-sdk-target-dir`
- ファイルリスト再現：`git diff --stat c6a0079 feat/pico-sdk-target-dir -- asp3_core.cmake CMakeLists.txt docs/porting/PORTING_GUIDE.md DIVERGENCE_MAP.md docs/dev/`

#### 検証結果（POSIX）

1. **既存プリセット無変更（CI green維持）**：
   - `linux`（POSIX CLI）— ビルドOK・`asp` 実行で `task1 is running` 確認。
   - `mps2_an521-qemu`（Cortex-M33）— ビルドOK・`asp.elf` 生成。QEMU(mps2-an521) 実行で起動確認。
2. **外部ターゲットスモーク**：`target/linux_gcc` を `/tmp/asp3_ext_target/linux_gcc` へコピーし、
   チップ依存部 `TARGETDIR` を `${CMAKE_CURRENT_LIST_DIR}` へ書き換え（共通arch `arch/posix_gcc` は
   `${ASP3_ROOT_DIR}` のまま）。`-DASP3_TARGET=linux_gcc -DASP3_TARGET_DIR=/tmp/asp3_ext_target/linux_gcc`
   でconfigure・ビルド・実行が成功（外部の `target.cmake` が読まれ、チップ依存部は外部・共通archは
   submodule側から正しく解決されることを確認）。

> 実機検証（タイマ競合の解決等）は asp3_pico_sdk リポジトリ側の作業。本リポジトリのスコープは
> 受け入れ口整備まで。

### asp3_core側（ライブラリ専用モード）— 完了（2026-06-10）

SDKアプリ（pico_stdlib をリンクする最終実行ファイル）が asp3_core を
`add_subdirectory` して `asp3` ライブラリだけを取り込めるよう、`CMakeLists.txt` に
**`ASP3_LIBRARY_ONLY` オプション**を追加した（既定OFF＝従来どおり `asp` 実行ファイルまでビルド）。

| ファイル | 変更内容 |
|---|---|
| `CMakeLists.txt` | `option(ASP3_LIBRARY_ONLY ... OFF)` を追加。ON のとき `asp` 実行ファイル・サンプル・ctest登録・実行/デバッグターゲットを作らず、`asp3` ライブラリ・cfg生成・ヘルパ関数（`asp3_add_syssvc` / `asp3_cfg_check`）のみを公開（`cfg1_out` は offset.h 生成に必要なため維持） |

- ブランチ：`feat/library-only`（ベース `9a15203` → コミット `5619f92`、CI 全9ジョブ green を確認して main へ ff マージ）。
- 検証：`linux`（OFF）で `asp` 生成を確認。`mps2_an521_gcc` を `-DASP3_LIBRARY_ONLY=ON` で
  configure・ビルドし `libasp3.a` 生成・`asp` 非生成を確認。

### SDK側（asp3_pico_sdk_sample／別リポジトリ）— ビルド検証 完了（2026-06-10）

A案（既存 `asp3_pico_sdk_sample` を再利用）で、旧 `asp3_pico_sdk`（カーネル同梱fork）への
依存を解消し、純カーネルの asp3_core を submodule 参照する構成へ移行。**ローカル作業まで完了・
GitHub操作（旧repoのarchive／sample→asp3_pico_sdk 改名／push）は保留**（ユーザー確認待ち）。

実施内容（`../asp3_pico_sdk_sample` ブランチ `feat/asp3-core-submodule`・コミット `f843f52`）：

| 区分 | 内容 |
|---|---|
| submodule差替 | `.gitmodules` の `asp3`（asp3_pico_sdk）→ `asp3_core`（`asp3_core.git`・main `5619f92`） |
| pico固有部の移設 | 旧 asp3_pico_sdk から `asp3_pico_sdk.cmake`（`PICO_PLATFORM`→`ASP3_TARGET`/`ASP3_TARGET_DIR`／`ASP3_CORE_DIR`／`irq_*` の `--wrap`）、`target/rp2350-arm-s_pico_sdk`、`arch/arm_m_gcc/rp2350`（チップ）・`arch/gcc` を sample 側へ |
| パス規約適用 | `target.cmake`／`arch.cmake` を外部ターゲット規約に修正（チップ/ターゲット＝`CMAKE_CURRENT_LIST_DIR` 相対、共通arch＝`ASP3_ROOT_DIR`）。`cfg1_out` 用に `-nostdlib -nostartfiles`＋`c gcc nosys` を `target.cmake` で供給 |
| CMake一本化 | `sample1/CMakeLists.txt` の fork CMake（`add_subdirectory(asp3_pico_sdk fork)`）を廃止し、`add_subdirectory(asp3_core, ASP3_LIBRARY_ONLY=ON)` ＋ `asp3_add_syssvc()` ＋ `asp3_set_pico_sdk_options()` に一本化 |

検証（ビルドまで・doc「CIはビルドまで」方針）：

- pico-sdk 2.1.1 ＋ arm-none-eabi-gcc 13.2.1 で `PICO_PLATFORM=rp2350-arm-s` を
  configure・ビルドし、`sample1_pico_sdk.elf`／`.uf2`／`.bin` の生成を確認
  （ARM EXEC・entry `0x1000014d`＝RP2350 flash XIP・text 約37KB）。
- cfg 3パス（`cfg1_out.c`→`cfg1_out.elf`→`offset.h`→`kernel_cfg.c/h`）が pico クロスビルド上で
  実行されることを確認（asp3_core の Python cfg エンジンがそのまま動作）。
- ASP3カーネル（`sta_ker`／`dispatcher_*`／`logtask_main`）と pico-sdk 割込み登録APIの
  `--wrap` 誘導（`__wrap_irq_set_exclusive_handler`）が同一ELFにリンクされることを確認。

> 未完（後続）：①GitHub操作（旧 asp3_pico_sdk の archive／`asp3_pico_sdk_sample`→`asp3_pico_sdk`
> 改名／push）はユーザー確認待ちで保留。②実機検証とタイマ競合（SDK alarm TIMER0 vs ASP3 HRT）の
> 調停は実機（pico-sdk必須）で別途。③RISC-V（Hazard3）版（`raspberrypi_pico2_riscv_gcc` を移植元）。
