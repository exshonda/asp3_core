# FILE_PLAN.md — フォルダ構成 & Git開発方法

> 作成日：2026-06-04  
> 対象：TOPPERS/ASP3 Core  
> 開発体制：1名・上流ASP3手動マージ追従

---

## 1. フォルダ構成

### 設計方針

- **トップレベルを上流ASP3と同じ構造にする**  
  `git diff upstream main` で自分の全変更が即座に見える状態を維持する。
- **変更種別をディレクトリ単位で明示する**  
  PRISTINE / DIVERGED / EXTENDED / NEW の4種でラベル管理。
- **新規追加ディレクトリは上流が絶対に触らない名前にする**  
  `cmake/`・`test/`・`scripts/`・`docs/` は上流との衝突ゼロ。

### ディレクトリツリー

```
asp3_core/                              ← リポジトリルート
│
│  ── ルートファイル ──────────────────────────────────────────
├── AGENTS.md                          ← 全ツール共通エージェント誘導
├── CLAUDE.md                          ← Claude Code向けポインタ
├── .clangd                            ← クロスコンパイルincludepath補正
├── .clinerules                        ← Cline向け（集約元はAGENTS.md）
├── .cursorrules                       ← Cursor向け（同上）
├── CMakeLists.txt                     ← トップレベルCMake
├── CMakePresets.json                  ← ターゲット×ツールチェーン定義
├── UPSTREAM_VERSION                   ← 上流ASP3バージョン・最終マージ日
├── DIVERGENCE_MAP.md                  ← 上流乖離台帳【マージ作業の起点】
├── UPSTREAM_PRISTINE.txt              ← 上流から変更していないファイル一覧
│
│  ── [PRISTINE] 上流ASP3コア ─────────────────────────────────
├── kernel/                            ← 上流コア。直接編集禁止
│   ├── *.c                            ←   カーネル本体（task.c 等）
│   ├── kernel_api.def                 ←   静的API定義（api-table・テキストDSL）
│   └── kernel_sym.def                 ←   シンボル定義
├── include/                           ← 上流ヘッダ。直接編集禁止
├── library/                           ← 上流ライブラリ。直接編集禁止
│
│  ── [EXTENDED] 上流ベース＋追加 ────────────────────────────
├── arch/
│   ├── arm_m_gcc/                     ← Cortex-M（上流ベース）
│   │   ├── common/                    ←   core_kernel_impl.c / core_support.S
│   │   │                                  core_sym.def / core_offset.py(旧.trb)
│   │   └── <chip>/                    ←   chip依存＋ arch.cmake
│   ├── arm64_gcc/                     ← Cortex-A35/AArch64（chip: stm32mp2）
│   └── riscv_gcc/                     ← NEW: RP2350 RISC-V (Hazard3)
│
├── sample/                            ← 上流サンプル＋派生サンプル
│
│  ── [DIVERGED] 上流から意図的に変更 ────────────────────────
├── syssvc/                            ← TECSレス版システムサービス
│   ├── serial.c                       ←   TECSなしプレーンC実装
│   ├── syslog.c                       ←   構造化ログ対応（T=,EV=形式）
│   ├── logtask.c
│   ├── banner.c
│   └── qemu_exit.c                    ←   NEW: セミホスティング終了
│
├── cfg/                               ← Python化済みコンフィギュレータ（汎用エンジン）
│   ├── cfg.py                         ←   エントリ・多パス制御
│   ├── pass1.py                       ←   api-table読込・構文解析
│   ├── pass2.py                       ←   コード生成
│   ├── gen_file.py                    ←   ファイル生成補助
│   └── srecord.py                     ←   Sレコード処理
│   # 静的API定義(kernel_api.def)・生成テンプレート(*.py)は
│   # kernel/ や arch/・target/ 側に置く（cfg/はエンジンのみ）
│
│  ── [NEW] 新規追加（上流との衝突なし） ─────────────────────
├── target/                            ← 全ターゲット依存部
│   ├── mps2_an521_gcc/                ←   QEMU Cortex-M33 (mps2-an521)
│   │   ├── target_config.h / target_kernel.h / target_kernel_impl.h
│   │   ├── target_kernel_impl.c / target_timer.c / target_serial.c
│   │   ├── target_kernel.cfg          ←   ターゲット静的構成
│   │   ├── target_kernel.py           ←   pass2生成テンプレート（旧.trb）
│   │   ├── target_check.py            ←   pass3チェック（旧.trb）
│   │   └── target.cmake               ←   変数積み上げ＋arch.cmake include
│   ├── linux_gcc/                     ←   ホスト(Linux)シミュレーション（上流 non_tecs 由来）
│   ├── rp2350-arm-s_pico_sdk/         ←   asp3_pico_sdkから移植
│   ├── rp2350-riscv_pico_sdk/         ←   NEW: RP2350 RISC-V
│   └── stm32mp257f_dk_arm64_gcc/           ←   stm32_vscode_aspから移植
│
├── asp3_core.cmake                     ← エントリ（asp3_pico_sdk.cmake 相当）
│                                         ASP3_ROOT_DIR設定・TARGET選択・ヘルパ関数
├── cmake/                             ← ツールチェーンファイルのみ
│   ├── toolchain-arm-none-eabi.cmake  ←   コンパイラフラグ(-mcpu等)はここ
│   ├── toolchain-riscv.cmake
│   └── toolchain-a35.cmake
│   # ビルドロジック本体（libasp3.a・cfg 3パス）はルート CMakeLists.txt
│   # ターゲット/アーキ固有は target.cmake / arch.cmake（変数積み上げ方式）
│
├── test/                              ← テストスイート
│   ├── tap/                           ←   TAPランナ実装（C）
│   │   ├── tap.h
│   │   └── tap.c
│   ├── cases/                         ←   テストケース
│   │   ├── test_task.c
│   │   ├── test_semaphore.c
│   │   └── test_eventflag.c
│   └── expected/                      ←   期待イベント列（CI比較用）
│       └── sample1.json
│
├── scripts/                           ← 自動化スクリプト
│   ├── parse_slog.py                  ←   構造化ログパーサ
│   └── check_events.py               ←   期待イベント列との比較
│
├── docs/                              ← ドキュメント（読み物はすべてここ）
│   ├── asp3_derivative_plan.md        ←   プロジェクト全体計画
│   ├── FILE_PLAN.md                   ←   本ファイル（構成・Git運用）
│   ├── errors.md                      ←   E_* エラーコード辞書
│   ├── api/                           ←   サービスコール・静的API別（RAG対応）
│   │   ├── README.md                  ←     索引・テンプレート・方針
│   │   ├── act_tsk.md  wai_sem.md  dly_tsk.md
│   │   └── CRE_TSK.md                 ←     静的API（正本は kernel_api.def）
│   └── porting/                       ←   移植関連
│       ├── PORTING_GUIDE.md           ←     移植手順（Step式）
│       ├── IMPL_INDEX.md              ←     実装参照インデックス
│       └── target_spec.yaml.template  ←     ハードウェア記述テンプレート
│
├── skills/                            ← skillパッケージ参照
│   └── README.md                      ←   別リポジトリへのポインタ
│
└── .github/
    └── workflows/
        └── ci.yml                     ← GitHub Actions CI
```

### ディレクトリ変更種別の定義

| 種別 | 意味 | 上流マージ時の扱い |
|---|---|---|
| **PRISTINE** | 上流ASP3をそのまま使用 | 上流版で上書き更新（人手不要） |
| **EXTENDED** | 上流をベースに追加あり | 上流変更を確認し必要分を取り込む |
| **DIVERGED** | 意図的に上流と異なる実装 | DIVERGENCE_MAPに従い要確認 |
| **NEW** | 本派生型で新規追加 | 上流との衝突なし |

---

## 2. Git開発方法

### 2.1 ブランチ構成

1名開発者のため最小限の構成にする。**必須は2本のみ**。

```
upstream  ←─ ASP3タルボールをそのままインポートするブランチ
   │            （上流との差分確認の基準点）
   │
main      ←─ 全ての作業ブランチ（常にビルド・テストが通る状態を維持）
   │
feat/*    ←─ 機能ブランチ（必要な場合のみ・短命）
```

### 2.2 初回セットアップ

```bash
# リポジトリ作成
git init asp3_core && cd asp3_core

# upstreamブランチを作成してASP3タルボールを取り込む
git checkout --orphan upstream
# ASP3タルボールを展開してファイルをコピー後：
git add -A
git commit -m "Import TOPPERS/ASP3 3.7.0"

# mainブランチをupstreamから作成
git checkout -b main upstream

# 自分の変更（TECS-less / Python cfg / CMake等）を加えていく
# syssvc/ cfg/ cmake/ target/ docs/ 等を追加・修正
git add -A
git commit -m "feat: initial asp3_core setup (TECS-less, Python cfg, CMake)"

# upstreamとmainの差分 = 自分の全変更（常にこれが見える状態を保つ）
git diff upstream main
```

### 2.3 新規ASP3リリース時のマージ手順

```bash
# 1. upstreamブランチに新バージョンを取り込む
git checkout upstream
# 新ASP3タルボールを展開してファイルを上書きコピー後：
git add -A
git commit -m "Import TOPPERS/ASP3 3.8.0"

# 2. 上流の変更箇所を確認（= DIVERGENCE_MAPとの照合起点）
git diff upstream~1 upstream          # 上流での変更内容
git diff upstream main -- kernel/     # kernel/への影響
git diff upstream main -- syssvc/     # syssvc/への影響

# 3. AIにDIVERGENCE_MAP + 上流diffを渡して影響箇所を抽出
#    「このdiffでDIVERGENCE_MAPのどの行が影響を受けるか列挙せよ」

# 4. mainに必要な変更を手動で取り込む
git checkout main
# UPSTREAM_PRISTINEのファイルは git checkout upstream -- <file> で更新
# DIVERGENCEファイルは差分を確認しながら手動マージ
git add -A
git commit -m "chore(upstream): merge ASP3 3.8.0"

# 5. UPSTREAM_VERSIONを更新
echo "ASP3_UPSTREAM_VERSION = 3.8.0" > UPSTREAM_VERSION
echo "ASP3_LAST_MERGED      = $(date +%Y-%m-%d)" >> UPSTREAM_VERSION
git add UPSTREAM_VERSION
git commit -m "chore: update UPSTREAM_VERSION to 3.8.0"

# 6. CI（POSIX + QEMU）で回帰確認
cmake --preset posix -B build/posix && cmake --build build/posix
./build/posix/asp --tap
```

### 2.4 機能ブランチを切る基準

**切る条件（いずれかを満たす場合）**：
- 複数セッション（日）にまたがる作業
- 既存の動作ターゲットを壊すリスクがある
- 新規ターゲット追加
- テストインフラ・CIの大幅変更

**mainに直接コミットする条件**：
- ドキュメント更新（AGENTS.md / DIVERGENCE_MAP 等）
- 小規模バグ修正
- コメント・整形・設定の微調整
- CIの小修正

```bash
# 機能ブランチの例
git checkout -b target/pico2-riscv        # RISC-Vターゲット追加
git checkout -b feat/qemu-mps2-an521      # QEMUターゲット整備
git checkout -b feat/tap-test-runner      # TAPテストランナ実装
git checkout -b feat/structured-log       # 構造化ログ実装
git checkout -b feat/os-awareness         # GDB OS awareness追加

# 完了したらmainにマージ
git checkout main
git merge --no-ff feat/tap-test-runner
git branch -d feat/tap-test-runner
```

### 2.5 コミットメッセージ規則

```
<type>(<scope>): <summary>

type:
  feat     新機能
  fix      バグ修正
  docs     ドキュメントのみの変更
  test     テスト追加・修正
  chore    ビルド・CI・依存関係等（コード変更なし）
  refactor リファクタリング

scope（省略可）:
  target   ターゲット依存部
  syssvc   システムサービス
  cfg      Pythonコンフィギュレータ
  cmake    ビルドシステム
  upstream 上流マージ関連
  ci       GitHub Actions

例：
  feat(target): add mps2-an521 QEMU target
  feat(syssvc): add structured log T=,EV= format
  fix(cfg): correct CRE_TSK parameter count
  docs: update DIVERGENCE_MAP for syssvc changes
  chore(upstream): merge ASP3 3.8.0
  test: add semaphore TAP test cases
  chore(ci): add QEMU mps2-an521 test job
```

### 2.6 既存3リポジトリの移行方針

初回セットアップ時に、既存派生リポジトリのターゲット実装を `target/` に移植する。  
移行後は元リポジトリが本カーネルをsubmoduleとして参照する構成に切り替える。

```
# 移行後の構成イメージ

asp3_pico_sdk/                  ← 既存リポジトリ（アプリ層のみ残す）
├── asp3_core/                   ←   submodule: 本カーネル
├── src/                        ←   アプリケーション
└── CMakeLists.txt

asp3_fsp/
├── asp3_core/                   ←   submodule: 本カーネル
├── src/
└── CMakeLists.txt

stm32_vscode_asp/
├── asp3_core/                   ←   submodule: 本カーネル
├── src/
└── CMakeLists.txt
```

submoduleの追加：

```bash
# 例：asp3_pico_sdkを移行後
cd asp3_pico_sdk
git submodule add https://github.com/<org>/asp3_core asp3_core
git submodule update --init
```

### 2.7 タグ・マイルストーン

```bash
# 主要マイルストーンにタグを打つ
git tag -a v0.1.0 -m "Initial: TECS-less + Python cfg + CMake + M33/A35"
git tag -a v0.2.0 -m "QEMU mps2-an521 + POSIX CLI + TAP test runner"
git tag -a v0.3.0 -m "RISC-V (RP2350) + structured log + OS awareness"
git tag -a v1.0.0 -m "Stable: all targets + CI + docs + skills package"
```

---

## 3. 開発フロー全体像

```
【初回セットアップ】
  ASP3タルボール → upstream branch
        ↓ merge
  main branch（＋TECS-less / Python cfg / CMake）

【日常開発】
  main または feat/* branch で作業
        ↓
  POSIX ビルド＆テスト（ハードなし・最速）
        ↓
  QEMU mps2-an521 テスト（M33確認）
        ↓
  実機確認（RP2350 / STM32MP257F-DK）
        ↓
  commit → CI自動実行（GitHub Actions）

【上流マージ時】
  新ASP3タルボール → upstream branch 更新
        ↓
  git diff upstream main → AI + DIVERGENCE_MAPで影響箇所抽出
        ↓
  手動マージ → CI確認 → UPSTREAM_VERSION更新
```

---

## 4. .gitignore（参考）

```gitignore
# ビルド成果物
build/
*.elf
*.bin
*.hex
*.uf2
*.map

# Python
__pycache__/
*.pyc
.venv/

# compile_commands（生成物・symlinkはコミットしない）
compile_commands.json

# エディタ
.vscode/settings.json
.idea/
*.swp

# OS
.DS_Store
Thumbs.db
```
