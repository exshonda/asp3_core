# STM32MP257F-DK 用 OpenOCD 設定（TOPPERS/FMP3）

TOPPERS/FMP3 を STM32MP257F-DK 上で **SWD デバッグ**するための OpenOCD 設定一式．
upstream OpenOCD 0.12 には STM32MP25x のボード設定が無い（mp13x/mp15x のみ）ため，本ターゲット
依存部に同梱している．設定は STMicroelectronics 提供の OpenOCD スクリプト（GPL-2.0-or-later）を
ベースに，**ベアメタル FSBL + gdb 完結**のための改変を加えたもの．

> 利用手順は `../target_user.md` の「実行方法」4・5 章を参照．本 README は**設定の構成と根拠，
> および本開発で得た知見**をまとめる．

---

## ファイル構成

### 同梱（このディレクトリ）
| ファイル | 役割 |
|---|---|
| `openocd.cfg`          | トップ設定．自分のディレクトリを検索パスに追加し，ボード設定を読み込む |
| `board/stm32mp25x_dk.cfg` | DK ボード設定（interface→transport→target→reset_config の順に構成） |
| `target/stm32mp25x.cfg`   | STM32MP25x SoC のターゲット定義（DAP/AP/コア/CTI/reset イベント）．**改変あり** |

### upstream OpenOCD 同梱版を使用（同梱しない）
| ファイル | 入手元 |
|---|---|
| `interface/stlink-dap.cfg` | OpenOCD 同梱（ST-LINK V2/V3, VID 0x0483 を網羅．STLINK-V3=0x3753 含む） |
| `target/swj-dp.tcl`        | OpenOCD 同梱（SWJ-DP 共通） |
| `mem_helper.tcl`           | OpenOCD 同梱（`mmw` 等のヘルパ） |

→ **OpenOCD 0.12 以降**が必要．`openocd.cfg` は自ディレクトリを検索パス先頭に足すので，
同梱の board/target が優先され，未同梱の interface/tcl は upstream から解決される．

---

## 起動方法

```bash
cd <FMP3>/target/stm32mp257f_dk_arm64_gcc/openocd
openocd -c "set EN_CA35_1 0" -f openocd.cfg
# Listening on port 3333 (a35_0/SMP), 4444 (telnet), 6666 (tcl)
```
- **gdb で完結**するソースデバッグ手順は `../swd-debug.gdb`（`gdb <fmp> -x ../swd-debug.gdb`）．
  単に動かすだけなら gdb 不要で `make swd-run`（OpenOCD だけでロード＆実行）が使える
  （`../target_user.md` 5 章）．
- `EN_CA35_1 0` は SMP（2 コア）運用で必須（後述）．シングルコアでも付けて問題ない．

---

## 設定の根拠

### SoC / デバッグトポロジ
- STM32MP257F は **Cortex-A35 ×2 / Cortex-M33 / Cortex-M0+**．デバッグは CoreSight DAP 経由．
- AP マップ: `ap0`(APB, ap-num 0), `axi`(AXI, ap-num 4), `ap2`(CM0+, ap-num 2), `ap8`(CM33, ap-num 8)．
- コア debug base / CTI:
  - `a35_0`: dbgbase `0x80210000`, CTI `0x80220000`
  - `a35_1`: dbgbase `0x80310000`, CTI `0x80320000`
  - `m33` : CTI `0xe0042000` / `m0p`: CTI `0xf0000000`
- `target create ... -defer-examine`: 各コアは起動時に即 examine せず，イベントで examine する
  （リセット直後はコアが未起動でアクセスできないことがあるため）．
- **`target smp $_CHIPNAME.a35_0 $_CHIPNAME.a35_1`**: 2 つの A35 を SMP グループにする．
  そのため **gdb ポートは a35_0 と a35_1 で共有（:3333）**．m33→:3334, m0p→:3335．

### トランスポート / リセット
- `interface/stlink-dap.cfg` + `transport select dapdirect_swd`（ST-LINK を DAP ラッパとして使用）．
- `reset_config srst_only`: SRST 信号でリセット．`reset run` で SoC を再起動し，FSBL に DDR 初期化
  から landing pad 起動までを行わせる（JTAG/halt リセットでは FSBL が走らず DDR 未初期化になる）．

### `EN_CA35_x` フラグ（コアの examine 制御）
- 既定は全コア examine（`EN_CA35_0/1`, `EN_CM33`, `EN_CM0P` = 1）．
- **`EN_CA35_1 0` の用途**: FMP3 の SMP（StepM）では Core1(a35_1) を **SoC 側**（`chip_mprc_initialize`
  の `CA35SYSCFG.VBAR_CR` + `RCC.C1P1RSTCSETR`）で起動する．OpenOCD が a35_1 を examine して
  halt 保持すると Core1 が走り出せないため，`EN_CA35_1 0` で OpenOCD に a35_1 を触らせない．
  起動ログから `a35_1: hardware has 6 breakpoints` の行が消えていれば解放成功．

---

## 本開発で得られた知見・改変点

### 1. gdb だけで完結させるための reset イベント改変（★本設定の肝）
オリジナルの ST 設定は，reset 時に **FSBL wrapper ハンドシェイク**（`_handshake_with_wrapper`,
DBGMCU `0x80010004`）と `_enable_debug`（ST デバッグ有効化レジスタ書き込み）を行う．これらは
ST の Linux/FSBL 環境前提で，**ベアメタル FSBL では失敗**し，reset イベントが a35_0 の
examine 前に中断する．結果，reset 後に a35_0 が未 examine となり，**gdb-attach が
"Target not examined yet" で拒否**される（このため当初は telnet 運用だった）．

改変（`target/stm32mp25x.cfg` 内にコメントで明示）:
- `_handshake_with_wrapper` を **no-op（即 return）** 化．
- `reset-deassert-pre` / `examine-end` イベントの各手順を **`catch` で包む**．

→ ST デバッグ処理が失敗しても，**a35_0 の `arp_examine`/`arp_halt` まで必ず到達**するようになり，
gdb だけで reset→load→実行が完結する．

### 2. a35_0 の examine タイミング
**reset 直後の examine は早すぎて失敗する**（CA35 がまだ起動しきっていない）．正しい順序は
**`reset run` → 数秒待つ（FSBL の DDR 初期化 + landing pad 起動）→ `arp_examine` → `arp_halt`**．
telnet・gdb いずれの運用でも同じ．`swd-debug.gdb` はこの待ちを `shell sleep 3` で入れている．

### 3. `Failed to write memory at 0x80000fe4` 警告は無害
reset 時の ST デバッグ有効化（存在しない FSBL wrapper 向け）の書き込み失敗．上記 `catch` で
握りつぶしており，動作に影響しない．

### 4. IWDG1（独立ウォッチドッグ）は FSBL で無効化が必要
DK は IWDG1（32 秒）が有効で，SWD で停止すると約 32 秒でリセットされデバッグ不能になる
（さらに A35 の examine 自体が `Could not initialize the APB-AP` で不安定化する）．
**SWD デバッグ用の FSBL は `fdts/stm32mp257f-dk.dts` の `&iwdg1` を `status="disabled"` にして
再ビルド**する（`../target_user.md` 3 章）．製品では戻すこと．

### 5. 先頭ページ（0x88000000–0x88001000）への resume は不可
FSBL は bm-fw を 0x88000000 にロード・entry する．landing pad はこの先頭ページに居るため，
SWD で **そこへ PC を載せて resume すると即例外**になる（観測済み）．FMP3 を SWD ロードする際は
**2 ページ目以降（`TEXT_START=0x88001000`）**でビルドし，`reg pc 0x88001000` で実行する．
SD 直接ブート時は FSBL の entry に合わせて `0x88000000` でビルドする．

### 6. ロードのコヒーレンシ
OpenOCD の `load`/`load_image` は DDR(LMA)へ直接書く．**前のアプリがキャッシュ/MMU 有効で
走行中にロードするとコヒーレンシが壊れる**ため，必ず `reset run` で landing pad（MMU/キャッシュ
OFF で停止）状態にしてからロードする．landing pad（同梱 `../minimal_boot`）は MMU/キャッシュを
一切操作しないので安全．

---

## トラブルシュート

| 症状 | 対処 |
|---|---|
| gdb 接続が `Remote connection closed` / `Target not examined yet` | reset 後に a35_0 が未 examine．`reset run`→待ち→`monitor ... arp_examine` の順にする（本設定の改変で通常は回避される） |
| `Could not initialize the APB-AP` 連発 / examine 不安定 | FSBL の IWDG1 が有効．無効化版 FSBL を使う（知見 4） |
| `a35_1 halted ... EL3H` が多数出る | SMP の 2 コア目報告で無害．SMP 起動を妨げるなら `EN_CA35_1 0` |
| ロード後 resume で即クラッシュ | 先頭ページに entry がある．`TEXT_START=0x88001000` でビルド（知見 5） |
| `Failed to write memory at 0x80000fe4` | 無害（知見 3） |
| HW ブレークポイント枯渇 / 挙動不審 | ボードを電源 OFF/ON（CPU debug レジスタの古い BP が reset で消えないことがある） |
| 前アプリの DDR 残留（古い rodata 文字列が延々出る）/ a35_0 が examine 未済に戻る | 部分ロードや halt 系コマンドの繰り返しで起きる．**`monitor reset run` で FSBL から再起動**（UART に BL2 ログ→landing pad が出てクリーン化）→ **OpenOCD を再起動**（`pkill openocd` 後に起動し直すと a35_0 を再 examine）→ ロードし直すのが確実 |
| `monitor reg pc` で PC を設定したのに resume すると別の場所から走る | gdb の `set $pc` は resume 時まで反映されないため `monitor resume` と相性が悪い．PC 設定は **`monitor reg pc <addr>`（ハードウェアへ直接）** を使う（同梱スクリプトはこれを採用） |

---

## ライセンス
`board/stm32mp25x_dk.cfg`・`target/stm32mp25x.cfg`・`openocd.cfg` は **GPL-2.0-or-later**
（STMicroelectronics 由来の OpenOCD 設定をベースに改変）．改変箇所は各ファイル内にコメントで明示．
