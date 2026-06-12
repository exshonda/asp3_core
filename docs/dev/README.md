# docs/dev/ — 機能追加の実施記録

機能追加項目ごとの経緯・手順書を置くディレクトリ。
ルールの正本は `AGENTS.md` §1「機能追加の実施ルール」を参照。

> 目的：変更点を明らかにし、他のTOPPERS系RTOSで同様の作業をする際の参考とすること。

- 実施プランは**着手前**に書き、実施結果は**完了時**に書く。
- `DIVERGENCE_MAP.md`（ファイル単位の上流乖離台帳）とは役割分担し、PRISTINE領域に変更が及んだ項目は相互にリンクする。

## 索引

項目名は `AGENTS.md` §1「機能追加計画」の表と一致させる。

| 項目 | ファイル | 状態 |
|---|---|---|
| TECSレス | `tecs-less.md` | 完了 |
| cfgのPython化 | `cfg-python.md` | 完了 |
| .rbツールの.py化 | `rb-tools-python.md` | 完了 |
| CMake対応 | `cmake.md` | 完了 |
| ファイルの削除 | `file-cleanup.md` | 完了 |
| QEMUターゲット(ARMv8-A) | `qemu-target-a64.md` | 完了 |
| QEMUターゲット(RISC-V) | `qemu-target-riscv.md` | 完了 |
| CLIターゲット | `cli-target.md` | 完了 |
| CI整備 | `ci.md` | 完了 |
| OS Awareness 対応 | `os-awareness.md` | 完了 |
| 移植検証テスト | `porting-test.md` | 完了（polarfire QEMUのみCIで確認） |
| RISC-V Hazard3ターゲット | `pico2-riscv.md` | 完了（dlynse較正・testexec 36/36・OS Awareness実機確認まで完了） |
| ドキュメントMarkdown化 | `docs-markdown.md` | 完了 |
| devcontainer / Docker | `devcontainer.md` | 完了 |
| Pico SDK統合 | `pico-sdk-integration.md` | 完了（asp3_core側＝`ASP3_TARGET_DIR`＋`ASP3_LIBRARY_ONLY`＋pico2ターゲット改称／SDK側＝submodule移行・ARM-S/RISC-V とも PICO2実機動作確認・タイマ競合定量検証（ALARM0 vs ALARM3＝競合なし）／GitHub再編済。SDK統合版でのOS Awareness確認等は後続） |
| FSP統合 | `fsp-integration.md` | 実施中（asp3_core側＝PORTING_GUIDE注記＋CMSIS協調のCPACR/FPCCR修正(473d97e)。SDK側＝asp3_fsp submodule化・EK-RA6M5/EK-RA8M2 を RASC6.2.0+ATfE clang でビルド検証完了。実機動作確認は後続） |
| STM32 HAL統合 | `stm32-integration.md` | 完了（外側リポジトリ asp3_stm32cube＝A案submodule化＋非TECS+Python cfg化。NUCLEO-H563ZI/H533RE 実機検証済み＝test_porting 6/6・testexec。H533REのVTOR整列が重要知見） |
| skillパッケージ | `skill-package.md` | 計画中 |

状態：計画中 → 実施中 → 完了（各項目の進行に合わせて更新する）

### 調査メモ（機能追加項目とは別の課題）

| 課題 | ファイル | 状態 |
|---|---|---|
| arm_mでcpuexc1/4が失敗する件（SIL_LOC_INT=PRIMASKによるHardFault昇格・上流固有） | `issue-cpuexc-armm.md` | 原因判明・対処未決（ユーザー判断待ち） |

## テンプレート

各.mdは以下の構成で作成する。

```markdown
# <項目名>

## 項目

（AGENTS.md §1 機能追加計画の項目名・優先度）

## 内容

（何を・なぜ行うか。上流ASP3からの変更観点）

## 実施プラン

（着手前に記載。手順・影響範囲・リスク）

## 実施結果

（完了時に記載）

### 変更したファイル

| ファイル | 変更内容概要 |
|---|---|
|  |  |

### 追加したファイル

### 削除したファイル

### Git情報

- ベースコミット：
- 関連コミット範囲：
- ファイルリスト再現コマンド例：`git diff --stat upstream main -- <paths>`

### 検証結果

| テスト | 実施 | 結果 |
|---|---|---|
| POSIX | ○/− |  |
| QEMU (mps2-an521) | ○/− |  |
| 実機 | ○/− |  |

### DIVERGENCE_MAP との関連

（PRISTINE領域に変更が及んだ場合、該当エントリへのリンク）
```
