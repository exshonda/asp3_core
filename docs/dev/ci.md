# CI整備

## 項目

CI整備（AGENTS.md §1 機能追加計画、優先度：高）

## 内容

GitHub Actions で**全ターゲットのビルド＋POSIX/QEMUでのテスト実行**を自動化する
（リポジトリ：`github.com/exshonda/asp3_core`）。

### 意義（何が嬉しいか）

1. **手動マージ後のリグレッション検知**（主目的）：本リポジトリは上流ASP3を
   手動マージで追従する。マージ・機能追加のたびに7ターゲットを手元で全件回すのは
   現実的でなく、「pushすれば全ターゲットの build→run→test が自動で走る」状態が
   上流追従の安全網になる。
2. **エージェントの作業完了条件の客観化**：AGENTS §4「検証の鉄則」（ビルドが
   通ることを確認してから報告）を、ローカル確認＋**CI green** の二段に格上げできる。
   テスト結果はTAP/JUnit形式で出力し、エージェントが解釈・自動修正に使える。
3. **環境の再現性**：GitHub-hostedランナー（ubuntu-24.04）＝開発機と同じ
   Ubuntu 24.04 系。apt のツールチェーン・QEMUバージョンがCI定義に明文化され、
   「開発機でしか通らないビルド」を検出できる。
4. **静的解析の常設**：AGENTS §6 が先行記述している clang-tidy / cppcheck の
   チェックをCIに実装し、MISRA-C意識の品質ゲートを実体化する。

### 前提の整理（2026-06-06 調査）

| 項目 | 状況 |
|---|---|
| `.github/workflows/` | **未作成**（AGENTS §11 が `ci.yml` を先行参照） |
| ランナー | `ubuntu-24.04`＝開発機と同系（CMake 3.28・QEMU 8.2） |
| arm-none-eabi | apt `gcc-arm-none-eabi`（13.2）✓ mps2/zybo/pico2 |
| aarch64 | apt `gcc-aarch64-linux-gnu`（13.3）✓ zcu102（-nostdlib対応済み）．**stm32の最終リンクは aarch64-none-elf が必要**→ARM公式tarballをDL＋`actions/cache` |
| riscv64 | apt `gcc-riscv64-unknown-elf`（13.2）✓ polarfire |
| QEMU | apt `qemu-system-arm`（mps2/zybo/zcu102 ✓ 開発機で8.2検証済み）・`qemu-system-misc`（riscv64）．**⚠️ microchip-icicle-kit は開発機ではQEMU 11で検証**：8.2での動作（-bios none・CLINT 1MHz）は**CI初回実行で要確認**．NGなら build-only に切替（または QEMU をソースビルド＋cache） |
| テスト実行基盤 | POSIX：ctest（JUnit出力可）＋testexec.py（ネイティブ・高速）／QEMU：testexec.py（セミホスティング自動終了）／cfg：testcfg.py（dummy・ホスト）— すべて整備済み |
| 合否判定 | `--tap`（POSIX）・「All check points passed.」（testexec）・check_events.py（slog照合）— CLIターゲット項目で整備済み |
| gh CLI | 開発機に未導入（CI結果の取得に導入推奨：`sudo apt-get install gh`） |

### ジョブ設計

**ワークフロー `ci.yml`**（push: main / pull_request / workflow_dispatch）

| ジョブ | ツールチェーン | 内容 |
|---|---|---|
| `posix` | （ホストgcc） | build → `ctest --preset linux --output-junit` → **testexec.py 全機能テスト**（ネイティブで高速）→ testcfg.py（dummy） |
| `mps2-qemu` | arm-none-eabi＋qemu-arm | build → testexec.py スモーク（task1/sem1/flg1/tmevt1/hrt1） |
| `zybo-qemu` | 同上 | build → sample1スモーク（バナーgrep・タスク切替） |
| `zcu102-qemu` | aarch64-linux-gnu＋qemu-aarch64 | build → sample1スモーク → testexec.py スモーク（sem1） |
| `polarfire-qemu` | riscv64-unknown-elf＋qemu-misc | build → sample1スモーク（**icicle-kit/QEMU8.2の検証を兼ねる**） |
| `pico2` | arm-none-eabi | **build only**（実機ターゲット） |
| `stm32mp257` | aarch64-none-elf（DL+cache） | **build only**（実機ターゲット・リンクまで） |
| `static-analysis` | cppcheck（＋clang-tidy） | EXTENDED/NEW領域（syssvc/ target/ arch/ cfg/ scripts/）。**kernel/・include/・library/（PRISTINE）は対象外** |

**ワークフロー `nightly.yml`**（schedule＋workflow_dispatch）

- QEMU 3ターゲット（mps2／zcu102／polarfire）で **testexec.py 全件**
  （PR毎に回すには重いため夜間に分離。mps2は36本×約30秒≒20分規模）

### 設計方針

- **スモークとフルの2段構え**：PR/pushは「全ターゲットbuild＋各QEMUスモーク」で
  10分以内を目標。全件テストはnightly（および手動dispatch）
- **結果はartifacts化**：テストログ（testexecのOBJ-\*出力・slogのjsonl・
  ctestのJUnit XML）をアップロードし、失敗時にエージェント/人間が解析できる形に
- **キャッシュ**：aarch64-none-elf tarball（〜200MB）のみ`actions/cache`。
  aptパッケージはランナー既載＋apt installで十分高速
- **fail-fast無効**：ターゲット独立で全件の成否を見せる（回帰の全体像把握）
- **concurrency**：同一ブランチの古い実行をキャンセル

## 実施プラン

1. **`ci.yml` 骨格＋posixジョブ**（最小で green にする）
   - checkout → build → ctest（JUnit）→ testexec.py全件 → testcfg.py →
     artifacts（JUnit・ログ）
2. **QEMUジョブ追加**（mps2 → zybo → zcu102 → polarfire の順）
   - 各スモークの合否判定スクリプトは `scripts/ci/` に置く
     （バナーgrep・「All check points passed.」カウント等．ローカルでも実行可能に）
   - **polarfireはQEMU 8.2での初回検証を兼ねる**：NGの場合は build-only に
     落とし、対処（QEMUソースビルド＋cache等）を本ファイルに記録
3. **build-onlyジョブ**（pico2／stm32mp257）
   - stm32：ARM公式 aarch64-none-elf tarball のDL＋cache＋フルリンク
4. **static-analysisジョブ**
   - cppcheck（apt）：syssvc/ target/ arch/ の C ソース（PRISTINE除外・
     `--error-exitcode=1` は当面warning運用＝non-blocking から開始）
   - clang-tidy は posix の compile_commands.json を使い対象を絞って段階導入
5. **nightly.yml**（QEMU全件テスト）
6. **検証**：ブランチ `feat/ci` でActions実行を確認（AGENTS §9 のブランチ基準
   「テスト/CI大幅変更」に該当）→ green後にmainへ
   - 開発機に gh CLI が無いため、実行結果の確認はWeb UIまたは
     `gh run watch`（gh導入後）。**CI検証はpush駆動**になる点に注意
7. **記録**：AGENTS §6（静的解析の実体化を反映）・§11手順4のci.yml実体化確認・
   README索引・本ファイル実施結果。READMEバッジ追加（任意）

### スコープ外

- 実機テストの自動化（セルフホストランナー）
- devcontainer / Docker によるツールチェーンピン留め（derivative planに記載
  あり。CIがaptバージョンを明文化するため当面不要。必要になったら別項目）
- JUnit集約ダッシュボード・カバレッジ計測

### リスク・確認事項

- **microchip-icicle-kit の QEMU 8.2 動作**（上記）— 計画段階での最大の不確定要素
- GitHub Actions の無料枠（privateリポジトリの場合は分数消費に注意．
  nightlyの頻度・対象で調整）
- testexec.py はテスト毎に cmake configure を回すため、CI上の実行時間は
  ローカルより伸びる可能性（必要なら configure 共有等の高速化は別途）

## 実施結果

（完了時に記載）
