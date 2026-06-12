# skillパッケージ

## 項目

skillパッケージ（AGENTS.md §1 機能追加計画、優先度：中。**全項目中、最後に完了**）

## 内容

AIコーディングツール（Claude Code等）向けの skill を整備する。
当初計画は「build/flash/debug/cfg生成skill（別リポジトリ）」だったが、
実装の進展に伴い**性格と置き場所を変更**して実現した：

- **性格**：build/flash/debug の操作skillではなく、**各SDKへの移植ガイドskill**
  （新ボード対応・環境特有の落とし穴・ブリングアップ手順）。
  操作系（build/run/test/debug）は AGENTS.md §4・docs/building.md・
  CMakeターゲット（run/osdebug等）・scripts/ci/ が既に担っており、
  skill化の価値は「移植ノウハウの再利用」にあると判断。
- **置き場所**：専用の別リポジトリではなく、**各SDK統合リポジトリ内**
  （`.claude/skills/`）。移植対象のコード・実例ボードと同居させることで
  skill が参照する実体とズレない。

## 実施結果

（2026-06-12 完了。実装は各SDKリポジトリ側）

| リポジトリ | skill | 内容 |
|---|---|---|
| `asp3_fsp` | `.claude/skills/porting-asp3-to-renesas-ra` | Renesas RA 新ボード移植手順（FSP＋RASC＋LLVM/clang 環境特有の落とし穴・FSPバージョン更新・警告ゼロ化・CLIビルド整備。実例＝EK-RA6M5/EK-RA8M2） |
| `asp3_stm32cube` | `.claude/skills/porting-asp3-to-stm32` | STM32 新ボード移植手順（CubeMX＋HAL＋gcc 環境特有の落とし穴＝ベクタテーブル整列・TrustZone・リンカGC。実例＝NUCLEO-H563ZI/H533RE） |
| `asp3_pico_sdk` | **なし（意図的）** | ベアメタル版（pico2_arm/pico2_riscv）が asp3_core 本体にあり、移植手順は PORTING_GUIDE.md＋実装例で足りるため skill 不要と判断 |

- asp3_core 本体には skill を置かない（移植の正本は `docs/porting/PORTING_GUIDE.md`
  ＋ `IMPL_INDEX.md`。SDK固有のノウハウのみ外側リポジトリの skill が担う）。
- 各skillの実装・検証の経緯は各リポジトリのコミット履歴を参照
  （例：asp3_fsp `4a54b4a`＝submodule構成への更新）。

### 当初計画からの変更点（横展開の参考）

| 当初計画 | 実際 | 理由 |
|---|---|---|
| build/flash/debug/cfg生成 skill | 移植ガイド skill | 操作系は CMake ターゲット・scripts/・AGENTS で完結しており、AIに必要なのは「環境特有の落とし穴」の知識だった |
| 別リポジトリ（skill専用） | 各SDKリポジトリ内 `.claude/skills/` | 参照する実体（コード・ボード実例）との同居で陳腐化を防ぐ |
