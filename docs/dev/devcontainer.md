# devcontainer / Docker

## 項目

devcontainer / Docker（AGENTS.md §1 機能追加計画、優先度：中）

## 内容

ツールチェーン・QEMU・Pythonを**ピン留めした開発コンテナ**を整備し、
開発機／CI／エージェント環境の再現性を確保する。

### 意義（何が嬉しいか）

1. **環境乖離の解消**：開発機はQEMU **11.0**へ更新済みだが、
   CI（ubuntu-24.04ランナー）はapt版**8.2**。
   ※この乖離による最大の実害（polarfireのCI build-only格下げ）は、
   **polarfireジョブを ubuntu:26.04 コンテナ（QEMU 10.2）で実行する対処で
   解消済み**（`82bd565`・ci.md参照）。ただしジョブ毎に異なるOSイメージが
   混在する状態であり、**全ジョブ／開発機／エージェントを単一のピン留め
   イメージに統一する**意義は残る。
2. **「開発機でしか通らない」の根絶**：ツールチェーン4系統
   （host gcc／arm-none-eabi／aarch64-linux-gnu／riscv64-unknown-elf）＋
   aarch64-none-elf（ARM公式tarball）＋CMake/Ninja/Python/cppcheckの
   バージョンが1つのDockerfileに明文化される。CIのapt installの列挙より
   強い再現性（aptはdistro更新で動く）。
3. **エージェント実行環境の配布**：Claude Code等のエージェントが
   devcontainer内で作業すれば、新しいマシン・他の開発者・サンドボックス環境でも
   `git clone → devcontainer起動` だけでbuild→run→testループが揃う。
   静的解析ツール（開発機に未導入のcppcheck等）も常備される。
4. **stm32フルリンクの標準化**：aarch64-none-elfをイメージに同梱し、
   どの環境でも全7ターゲットがリンクまで通る。

### 前提の整理（2026-06-06 調査）

| 項目 | 状況 |
|---|---|
| Docker | 開発機にv29.5・サーバ動作確認済み（イメージのローカルビルド・検証が可能） |
| `.devcontainer/` | 未作成 |
| ピン留め対象（開発機の現状） | gcc 13.3／arm-none-eabi 13.2.1／aarch64-linux-gnu 13.3／riscv64-unknown-elf 13.2／CMake 3.28.3／Ninja 1.11.1／Python 3.12.3／**QEMU 11.0.0**（ローカルビルド） |
| CIとの乖離 | QEMU（11.0 vs 8.2）・cppcheck（開発機に無し）・aarch64-none-elf（CIはtarball+cache） |
| イメージ配布先 | GHCR（`ghcr.io/exshonda/asp3_core-dev`）。privateリポジトリのGHCRはActionsの`GITHUB_TOKEN`でpush/pull可 |

### 再検討：ベースを ubuntu:26.04 にする（2026-06-06・実地検証済み）

**ubuntu:26.04 をベースにすると QEMU ソースビルド（最重量ステージ）が丸ごと
不要になる**ことを、開発機の Docker で実地検証した：

| 検証項目 | 結果 |
|---|---|
| apt の QEMU | **10.2.1**（icicle-kit直接ブート対応＝CIのpolarfireジョブで実績あり） |
| ツールチェーン | host gcc 15.2／arm-none-eabi **14.2**／aarch64-linux-gnu 15.2／riscv64-unknown-elf 14.2／CMake **4.2**／Python **3.14**／cppcheck 2.19 |
| ビルド | **linux・mps2・zcu102・polarfire の4プリセットすべてBUILD OK**（CMake 4でも`cmake_minimum_required 3.16`はそのまま通る） |
| QEMU実行 | mps2・zcu102・polarfire の sample1 バナー確認（icicle-kit含む） |
| ⚠️ パッケージ名変更 | 26.04では riscv が `qemu-system-misc` から分離→ **`qemu-system-riscv`**（CIのpolarfireジョブと同じ） |

これにより：**Dockerfileはaptインストール＋tarball展開のみのシングルステージ**
（イメージビルド数分・BuildKitキャッシュ不要級）になり、CIのpolarfireジョブ
（ubuntu:26.04）とベースOSも統一される。

### 構成（ubuntu:26.04ベース・シングルステージ）

```
.devcontainer/
├── devcontainer.json   ← VS Code / Claude Code 用（image参照・clangd拡張・
│                          postCreateでcompile_commands.jsonリンク）
└── Dockerfile          ← ubuntu:26.04ベース・シングルステージ
                           apt一式（QEMU 10.2含む）＋aarch64-none-elf tarball
.github/workflows/container.yml ← Dockerfile変更時にGHCRへbuild&push
```

- **QEMUのピン**：apt版10.2.1（ソースビルド廃止。開発機の11.0との差は
  検証済み機能の範囲では影響なし）
- **arm64コンパイラは aarch64-none-elf（ARM公式tarball）に一本化**：
  - aptには26.04でもベアメタル版（gcc-aarch64-none-elf／aarch64用newlib）が
    **存在しない**（linux-gnu系のみ。実地確認済み）
  - バージョンは**CIのstm32ジョブと同一の 13.3.rel1** にピン
    （`arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf`）
  - `toolchain-a35.cmake` の既定プレフィックスが aarch64-none-elf のため、
    コンテナ内では **`A35_TOOLCHAIN_PREFIX` オーバーライド不要**＝
    zcu102・stm32 ともプリセット素のままで通る（stm32はフルリンク）
  - `gcc-aarch64-linux-gnu`（apt）は**イメージに入れない**（2系統混在を回避）。
    zcu102 chip.cmake のglibc系対策（-nostdlib・libc_stub等）は
    「tarballが無い素の環境向けフォールバック」として残置
- **タグ運用**：`ghcr.io/...:latest`＋日付タグ（`:20260606`）。
  CI・devcontainer.jsonは日付タグを参照（暗黙の更新を防ぐ）

## 実施プラン

1. **Dockerfile作成**（`.devcontainer/Dockerfile`）
   - **ubuntu:26.04**・apt（バージョン明示）：build-essential, cmake, ninja-build,
     python3, gcc-arm-none-eabi＋libnewlib,
     gcc-riscv64-unknown-elf＋picolibc,
     **qemu-system-arm, qemu-system-riscv**（26.04の分割名に注意）,
     cppcheck, clang-tidy, dtc, git, gh, gdb-multiarch
     （**gcc-aarch64-linux-gnu は入れない**＝arm64はnone-elfに一本化）
   - ARM公式 aarch64-none-elf **13.3.rel1** tarball を /opt へ展開・PATH追加
     （share/doc削除でサイズ節減）
   - 非rootユーザ（vscode）・ワークスペース権限
   - ツールチェーンが一段新しくなる（gcc15/arm-none-eabi14.2等）ため、
     **全ターゲットの警告ゼロ確認**をローカル検証に含める
2. **ローカル検証**（開発機のDockerで）
   - イメージビルド → コンテナ内で：
     linux（ctest＋testexec数本）／mps2・zcu102・polarfire 各QEMUスモーク／
     pico2・stm32 build（**stm32はフルリンク**）／testcfg／cppcheck
   - **polarfireのQEMU実行がコンテナ内で通ること**
3. **devcontainer.json作成**
   - image参照（GHCR日付タグ）・clangd拡張・
     postCreateCommand（compile_commands.jsonリンク等）
4. **GHCR publishワークフロー**（`.github/workflows/container.yml`）
   - paths: `.devcontainer/**` のpush時のみbuild&push（QEMUビルドが
     重いためBuildKitレイヤキャッシュ＝`cache-from: gha`を使用）
5. **CIのコンテナ化**（`ci.yml`/`nightly.yml` を `container:` 実行に切替）
   - apt install手順を削除しイメージ参照に置換（セットアップ時間も短縮）
   - polarfireの暫定対処（ubuntu:26.04コンテナ・QEMU 10.2）を本イメージに
     統一（CIへの復帰自体は実施済み）
   - ランナー上のdockerイメージpullが毎回入るため、所要時間の前後を計測し
     ci.mdに記録（イメージサイズ次第では従来方式の維持も判断）
6. **検証**：feat/devcontainer ブランチでActions確認（container.yml→
   イメージpush→ci.yml green→polarfire実行確認）
7. **記録**：AGENTS（§4前提メモにdevcontainer言及・必要なら）・
   docs/building.md（コンテナでのビルド手順節）・README索引・本ファイル実施結果

### スコープ外

- 実機書込み（USBパススルー）・SWDデバッグのコンテナ化（ホスト側で実施）
- マルチアーキイメージ（arm64ホスト対応）。当面 amd64 のみ
- Codespaces最適化（動けばよい・チューニングはしない）

### リスク・確認事項

- **イメージサイズ**：ツールチェーン4系統＋QEMU（apt）で1.5〜2.5GB級の見込み
  （ソースビルド廃止でステージング不要に）。
  CIのpull時間（GHCR→ランナー）が毎ジョブ発生するため、現行（apt install
  約1分）との比較で採否を判断（イメージはdevcontainer用とし、CIは従来方式の
  まま＋QEMUだけイメージから取り出す折衷もあり得る）
- GHCR privateイメージのpullはdevcontainer利用時に `gh auth token` 等での
  docker loginが必要（手順をdevcontainer.json/READMEに記載）

## 実施結果

（完了時に記載）
