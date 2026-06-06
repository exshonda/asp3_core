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

### 構成

```
.devcontainer/
├── devcontainer.json   ← VS Code / Claude Code 用（image参照・clangd拡張・
│                          postCreateでcompile_commands.jsonリンク）
└── Dockerfile          ← ubuntu:24.04ベース・マルチステージ
                           stage1: QEMUソースビルド（arm/aarch64/riscv64）
                           stage2: ツールチェーン一式＋stage1のQEMU＋
                                   aarch64-none-elf tarball＋解析ツール
.github/workflows/container.yml ← Dockerfile変更時にGHCRへbuild&push
```

- **QEMUのピン**：11.0.0をソースビルド（3ターゲット
  `arm-softmmu,aarch64-softmmu,riscv64-softmmu`）。開発機の検証済み
  バージョンと一致させ、icicle-kit直接ブート対応を確保
- **タグ運用**：`ghcr.io/...:latest`＋日付タグ（`:20260606`）。
  CI・devcontainer.jsonは日付タグを参照（暗黙の更新を防ぐ）

## 実施プラン

1. **Dockerfile作成**（`.devcontainer/Dockerfile`）
   - ubuntu:24.04・apt（バージョン明示）：build-essential, cmake, ninja-build,
     python3, gcc-arm-none-eabi＋libnewlib, gcc-aarch64-linux-gnu＋
     libc6-dev-arm64-cross, gcc-riscv64-unknown-elf＋picolibc,
     cppcheck, clang-tidy, dtc, git, gh
   - QEMU 11.0.0 マルチステージビルド（--target-list=3種・最小構成
     `--disable-gtk --disable-sdl` 等でサイズ抑制）
   - ARM公式 aarch64-none-elf tarball を /opt へ展開・PATH追加
   - 非rootユーザ（vscode）・ワークスペース権限
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

- **イメージサイズ**：ツールチェーン4系統＋QEMU3種で2〜3GB級になる見込み。
  CIのpull時間（GHCR→ランナー）が毎ジョブ発生するため、現行（apt install
  約1分）との比較で採否を判断（イメージはdevcontainer用とし、CIは従来方式の
  まま＋QEMUだけイメージから取り出す折衷もあり得る）
- QEMU 11のビルド依存（ninja/meson/glib等）はstage1のみに閉じる
- GHCR privateイメージのpullはdevcontainer利用時に `gh auth token` 等での
  docker loginが必要（手順をdevcontainer.json/READMEに記載）

## 実施結果

（完了時に記載）
