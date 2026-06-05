# TOPPERS/ASP3 Raspberry Pi Pico 2 ターゲット依存部 利用ガイド

Raspberry Pi **Pico 2**（RP2350, Cortex-M33）向け TOPPERS/ASP3 ターゲット依存部の解説と利用方法．
Raspberry Pi Pico 版（`target/raspberrypi_pico_gcc/target_user.txt`）を基に，RP2350 用へ全面改訂した
もの．

---

## 概要

`raspberrypi_pico2_gcc` 依存部は，Raspberry Pi **Pico 2**（RP2350 の ARM/Cortex-M33 モード）を
サポートする．ASP3 はシングルプロセッサカーネルであり，**Core0 のみ**を使用する（Core1 は
bootROM 内で待機したままとなる）．動作モードは **Secure ステート**（ARMv8-M）である．

- ターゲット略称: `raspberrypi_pico2_gcc`
  （BOARD=`raspberrypi_pico2`, CHIP=`rp2350`, PRC=`arm_m`, TOOL=`gcc`）
- チップ依存部: `arch/arm_m_gcc/rp2350/`

### ボード
動作確認を行ったボード:
- Raspberry Pi **Pico 2**（SoC: RP2350A, Cortex-M33 ×2 / FPU(FPv5-SP) / 4MB QSPI フラッシュ /
  520KB SRAM）

### コンパイラ
動作確認を行った GCC（Arm GNU Toolchain, ベアメタル `arm-none-eabi`）:
- `arm-none-eabi-gcc` **14.3.Rel1**
- https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

### デバッグ環境
- **Raspberry Pi フォークの OpenOCD**（0.12.0+dev）+ **Debugprobe**（CMSIS-DAP）で動作確認．
- ⚠️ upstream OpenOCD 0.12.0 は **RP2350 未サポート**（`target/rp2350.cfg` もフラッシュドライバも
  無い）．必ず Raspberry Pi フォークをビルドして使用する（「実行方法」1.4 節参照）．

### 実行環境
- RP2350 の bootROM が，フラッシュ先頭 4KB 内の **IMAGE_DEF ブロック**（`image_def.S`）を検証して
  本カーネルを **Secure ステート**で直接起動する．
- RP2040 と異なり **boot stage2 は不要**．picotool / UF2 変換も不要で，OpenOCD で ELF を直接
  書き込める．
- クロック初期化（XOSC→PLL, 150MHz）は本依存部の `hardware_init_hook()` が行う．

---

## カーネルのコンフィギュレーション

### カーネルタイマ方式
`target.cmake` の `USE_TIM_AS_HRT` 定義で選択する（既定: `TIM`）．
- `TIM` : RP2350 内蔵 TIMER0 を高分解能タイマ（HRT）として使用（既定・動作確認済み）
- `SYSTICK` : SysTick による方式（未検証）

### TECS
既定で TECS を使用する（シリアルドライバ・システムログは TECS コンポーネント）．

---

## カーネルの使用リソース

カーネルは以下のリソースを使用する．

- **タイマ**: RP2350 内蔵 **TIMER0**（ALARM0, INTNO 16）を高分解能タイマとして使用．
  TIMER0 へは TICKS ブロック経由で 1MHz（clk_ref 12MHz ÷ 12）の tick を供給する
  （`hardware_init_hook()` で設定．未設定だと TIMER0 は歩進しない）．
- **UART**: コンソールに **UART0**（GP0=TX / GP1=RX, INTNO 49, 115200 8N1）を使用する．
  ドライバは ARM PL011 用の TECS コンポーネント `tUsart`（`arch/arm_m_gcc/rp2350/tUsart.c`）．
- **クロック**: XOSC(12MHz) → PLL_SYS（×125 → VCO 1500MHz → /5/2）で **clk_sys = 150MHz**．
  clk_peri は clk_sys を選択（UART のボーレート計算は `CPU_CLOCK_HZ`=150MHz 基準）．
- **メモリ**: QSPI フラッシュ（XIP）と SRAM を使用（次節）．

---

## メモリマップとスタック

### メモリ配置（`rpi_pico2.ld`）
| 領域 | アドレス | サイズ | 備考 |
|---|---|---|---|
| FLASH（.text/.rodata, XIP） | `0x10000000` | 4MB | 先頭にベクタテーブル（`.vector`），続けて IMAGE_DEF ブロック（`.picobin_block`，先頭 4KB 以内必須） |
| RAM（.data/.bss/スタック） | `0x20000000` | 512KB | メイン SRAM（`.data` の初期値は FLASH 側に置き `start.S` がコピー） |

### スタック
| スタック | サイズ | 定義 |
|---|---|---|
| 非タスクコンテキスト用（ISTK） | 既定 4KB（`DEFAULT_ISTKSZ=0x1000`） | `target_kernel_impl.h` |
| タスクスタック（TECS 既定） | 2096B（`DefaultTaskStackSize`） | `target.cdl`．`CRE_TSK` 等で個別指定可 |

---

## 割込み管理

割込みは Cortex-M33 の NVIC で管理する．利用者は TOPPERS 標準の静的 API
（`ATT_ISR`/`DEF_INH`/`CFG_INT`）で割込みサービスルーチン・割込みハンドラを登録する．

### 割込み番号（INTNO）の体系
**INTNO = RP2350 の IRQ 番号 + 16**（Cortex-M の例外番号体系）．`TMAX_INTNO = 68`（16 例外 +
52 IRQ）．IRQ 番号は RP2350 データシート §3.2 を参照（主要なものは
`arch/arm_m_gcc/rp2350/RP2350.h` に `RP2350_*_IRQn` として定義済み）．

主要な割込み番号:
| 用途 | IRQ | INTNO | 備考 |
|---|---|---|---|
| TIMER0 ALARM0（カーネルタイマ） | 0 | 16 | `INTPRI_TIMER = -2` |
| UART0（コンソール, SIO） | 33 | 49 | `tSIOPortTarget.cdl` |
| UART1 | 34 | 50 | プロトタイプ定義あり（未検証） |

### 割込み優先度
NVIC の優先度フィールドは 4bit（`TBITW_IPRI=4`, 16 段）．カーネル管理の割込み優先度の範囲は
- `TMIN_INTPRI = -3`（最高位）
- `TMAX_INTPRI = -1`（最低位）

で，値が小さいほど高優先度である．`-3` より高い優先度（`-16`～`-4`）はカーネル管理外割込みと
して使用できる（`chip_kernel.h` の `TMIN_INTPRI` で変更可能）．

### ハンドラ登録の例（静的 API）
```c
/* 割込みサービスルーチン(ISR)を割り付ける例 */
ATT_ISR({ TA_NULL, EXINF, INTNO, isr_func, ISRPRI });
CFG_INT(INTNO, { TA_ENAINT, INTPRI });    /* 属性・優先度の設定 */

/* 割込みハンドラ(INH)を直接定義する例 */
DEF_INH(INHNO, { TA_NULL, inh_func });
CFG_INT(INTNO, { TA_ENAINT, INTPRI });
```

---

## タイマの仕様

- カーネルタイマには RP2350 内蔵 **TIMER0**（ALARM0）を高分解能タイマ（HRT）として使用する．
- カウンタは **1MHz**（1µs 単位）．TICKS ブロック（`TICKS_TIMER0`）で clk_ref(12MHz) を 12 分周
  して生成する（`hardware_init_hook()` で設定）．
- 高分解能タイマのタイマ周期 `TCYC_HRTCNT` は**定義しない**（2^32 [µs]），進み幅
  `TSTEP_HRTCNT = 1`，`HRTCNT_BOUND = 4000000002`（`target_timer.h`）．
- 割込み優先度は `INTPRI_TIMER = TMAX_INTPRI - 1`（= -2）．

---

## システムログ機能

システムログの低レベル出力およびログタスクは UART0 の SIO ポートを使用する（TECS 構成:
`target.cdl` の `SIOPortTarget1` + `PutLogTarget`）．115200bps, 8N1．

---

## ブートの流れについて

RP2350 の bootROM がフラッシュ先頭 4KB 内の **IMAGE_DEF ブロック**を検証し，本カーネルを
**Secure ステート**で直接起動する（boot stage2 / picotool / UF2 は不要）．

```
電源ON / Reset
  └─ BOOTROM（RP2350 内蔵）
       ・QSPI フラッシュ先頭 4KB から IMAGE_DEF ブロック（.picobin_block）を探索・検証
         （EXE / Secure / ARM / RP2350 の最小 20 バイトブロック = image_def.S）
       ・イメージ先頭(0x10000000)をベクタテーブルとみなし，Secure ステートで起動
         （SP=vector[0], PC=vector[1]=_kernel_start）
  └─ ASP3（start.S: _kernel_start）
       ① hardware_init_hook（target_kernel_impl.c）
            ・周辺リセット解除 → XOSC 安定化 → PLL_SYS 設定 → clk_sys = 150MHz
            ・TICKS_TIMER0 設定（TIMER0 へ 1MHz tick 供給）
            ・GP0/GP1 を UART0 に設定（PADS の ISO 解除・IE 設定を含む）
            ・FPU 有効化（CPACR の CP10/CP11）
       ② .data コピー・.bss クリア → sta_ker
       ③ target_initialize → core_initialize（VTOR=_kernel_vector_table 等）
       ④ カーネル起動 → タスク実行
```

> 補足: ベクタテーブル（`.vector`）はコンフィギュレータ（標準の `core_kernel.trb`）が生成し，
> リンカスクリプトで FLASH 先頭に配置する．IMAGE_DEF ブロックはその直後に置く．

---

## 実行方法

サンプルアプリ `sample1` を例に説明する．パス表記:
- `<ASP3>` : 本 ASP3 カーネルを展開したディレクトリ．
- `<OBJ>`  : 後述の手順で作成するビルドディレクトリ．

### 1. PC（ホスト）環境の構築

#### 1.1 動作確認環境
- OS: **Ubuntu 24.04 LTS** で動作確認．
- 必要ツール: クロスコンパイラ，CMake＋Ninja，Python 3（コンフィギュレータ用），
  OpenOCD（Raspberry Pi フォーク），シリアル端末（picocom 等）．

#### 1.2 クロスコンパイラ（arm-none-eabi-gcc）
```bash
# Arm GNU Toolchain（arm-none-eabi）を展開し PATH を通す
export PATH=/usr/local/tools/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin:$PATH
arm-none-eabi-gcc --version
```

#### 1.3 ビルドツール（CMake／Ninja／Python 3）
```bash
sudo apt-get install -y cmake ninja-build python3     # 未導入なら
```

#### 1.4 OpenOCD（Raspberry Pi フォーク）のビルド・インストール
upstream OpenOCD 0.12.0 は RP2350 未サポートのため，Raspberry Pi フォークをビルドする:
```bash
sudo apt-get install -y libtool automake autoconf pkg-config texinfo \
                        libusb-1.0-0-dev libhidapi-dev libjim-dev
git clone --depth 1 https://github.com/raspberrypi/openocd.git
cd openocd
./bootstrap
./configure --enable-cmsis-dap
make -j$(nproc)
sudo make install            # /usr/local/bin/openocd
openocd --version            # 0.12.0+dev-... と表示されること
ls /usr/local/share/openocd/scripts/target/ | grep rp2350   # rp2350.cfg があること
```

#### 1.5 ボード接続とパーミッション設定
- **Debugprobe** をホスト USB に接続し，Pico 2 と以下のように接続する:
  - 「D」コネクタ（SWD）→ Pico 2 の DEBUG コネクタ
  - 「U」コネクタ（UART）→ Pico 2 の GP0/GP1（**クロス接続**: プローブ RX ← GP0(TX),
    プローブ TX → GP1(RX), GND 共通）
- シリアルコンソールは Debugprobe の UART ブリッジ（`/dev/ttyACM0`, 115200 8N1）を使用する．

**(a) シリアルコンソール(ttyACM)を非 root でアクセス**
```bash
sudo usermod -aG dialout "$USER"     # 再ログインで有効
ls -l /dev/ttyACM0
```

**(b) Debugprobe(CMSIS-DAP) を非 root でアクセスする udev ルール**
```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="000c", MODE="0660", GROUP="plugdev", TAG+="uaccess"' \
  | sudo tee /etc/udev/rules.d/61-debugprobe.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG plugdev "$USER"     # 再ログインで有効
```
> `lsusb` に `Raspberry Pi Debugprobe on Pico (CMSIS-DAP)`（2e8a:000c）が見えること．

### 2. ASP3 のビルド

**(1) ビルドディレクトリの生成**
```bash
cmake --preset raspberrypi_pico2_gcc -B build/raspberrypi_pico2_gcc
```
> ツールチェイン（arm-none-eabi）に PATH を通しておくこと．

**(2) ビルド**
```bash
cmake --build build/raspberrypi_pico2_gcc    # -> asp.elf を生成
```
リンク時に `--print-memory-usage` で FLASH/RAM 使用量が表示される（sample1 で FLASH 約 23KB /
RAM 約 14KB）．

### 3. フラッシュへの書き込みと実行

```bash
ninja -C build/raspberrypi_pico2_gcc run      # OpenOCD で書き込み（program verify reset）→ そのまま実行開始
ninja -C build/raspberrypi_pico2_gcc console  # 別端末: シリアルコンソールを開く（出力確認・キー入力用）
```

**期待される出力（sample1）**:
```
TOPPERS/ASP3 Kernel Release 3.7.2 for RaspberryPi Pico2:ARM Cortex-M33 (...)
...
System logging task is started.
Sample program starts (exinf = 0).
task1 is running (001).   |
task1 is running (002).   |
  ...（カウンタが増加し続ける．'1'〜'3'/'a'〜'z' 等のキー入力で挙動が変わる）
```

### 4. SWD（OpenOCD + GDB）によるデバッグ

OpenOCD と gdb を起動できる CMake ターゲット（`target/raspberrypi_pico2_gcc/run.cmake`）を
用意している（`ninja -C build/raspberrypi_pico2_gcc <ターゲット>` で実行）:

| ターゲット | 動作 | OpenOCD |
|---|---|---|
| `run`       | OpenOCD だけで書き込み＆実行（gdb 不要．最も手軽） | 完了で自動終了 |
| `swd-debug` | OpenOCD を自動起動し gdb（`swd-debug.gdb`）でロード＆デバッグ（1 端末で完結） | **gdb 終了で自動終了** |
| `openocd`   | OpenOCD だけを前面起動（別端末の `gdb` と対で使う） | Ctrl-C で終了 |
| `gdb`       | gdb だけを起動しロード＆デバッグ（OpenOCD は別端末で起動済みのこと） | （触らない） |
| `console`   | UART コンソール(UART0)を開く（別端末で使う） | （触らない） |

```bash
ninja -C build/raspberrypi_pico2_gcc console    # 端末C: シリアルコンソールを開いておく（出力を見る）
ninja -C build/raspberrypi_pico2_gcc run        # 単に動かすだけ（最も手軽．gdb 不要）
ninja -C build/raspberrypi_pico2_gcc swd-debug  # 1 端末でソースデバッグ（break 設定後 continue）

# 2 端末に分けたい場合（OpenOCD を起動しっぱなしにして gdb を繰り返したいとき）
ninja -C build/raspberrypi_pico2_gcc openocd    # 端末1: OpenOCD を起動して放置（gdb サーバ :3333）
ninja -C build/raspberrypi_pico2_gcc gdb        # 端末2: gdb を起動（再ビルド後はこれを再実行するだけ）
```
- `swd-debug.gdb` は `:3333` に接続し，**reset init → load（フラッシュ書き込み）→ reset init**
  までを行い **halt のまま待機**する．`break` 設定後に `continue` で bootROM から実行を開始する．
- 上書き可能なキャッシュ変数: `OCD_IF`/`OCD_TGT`/`OCD_SPEED`，
  `TTY`（既定 `/dev/ttyACM0`），`BAUD`（既定 `115200`）．
  例: `cmake -B build/raspberrypi_pico2_gcc -DTTY=/dev/ttyACM1`

手動で同等のことを行う場合:
```bash
# 書き込みのみ（run ターゲット相当）
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
  -c "adapter speed 5000" -c "program asp.elf verify reset exit"

# gdb デバッグ（OpenOCD 起動後に）
arm-none-eabi-gdb asp.elf
(gdb) target extended-remote localhost:3333
(gdb) monitor reset init
(gdb) load
(gdb) monitor reset init
(gdb) break main_task
(gdb) continue
```

---

## 注意事項・既知の事項

- **Secure ステートと `TOPPERS_ENABLE_TRUSTZONE`**: RP2350 の bootROM は ARM イメージを **Secure
  ステート**で起動する．このため `target.cmake` で **`TOPPERS_ENABLE_TRUSTZONE`** を定義し，
  Secure ステート用の `EXC_RETURN`（`0xfffffffd`）を使用している．未定義にすると Non-secure 用の
  `EXC_RETURN`（`0xffffffbc`）が使われ，**最初の例外リターンで INVPC UsageFault となり起動しない**
  （TrustZone による Secure/Non-secure 分割を行うという意味ではない．カーネルは Secure のみで
  動作する）．
- **TIMER0 と TICKS ブロック**: RP2350 では TIMER0 のカウントソースが TICKS ブロック経由に
  なった（RP2040 は watchdog の tick）．`TICKS_TIMER0_CYCLES/CTRL` を設定しないと TIMER0 が
  歩進せず，`target_hrt_initialize()` でハングする．本依存部は `hardware_init_hook()` で設定済み．
- **PADS の ISO ビット**: RP2350 ではリセット後にパッドがアイソレーション状態となる．
  ISO ビットをクリアしないと UART 信号が出力されない（`hardware_init_hook()` で解除済み）．
- **アトミックアクセスエイリアスと PPB**: `chip_sil.h` の `sil_orw()`/`sil_clrw()` は RP2350 の
  アトミックアクセスエイリアス（+0x1000/+0x2000/+0x3000）を使用しており，**M33 の PPB 領域
  （NVIC/SCB/CPACR 等, 0xE000xxxx）には適用されない**．PPB レジスタの read-modify-write は
  `sil_rew_mem()`/`sil_wrw_mem()` で直接行うこと（FPU 有効化の CPACR 設定はこの理由で
  `hardware_init_hook()` で直接行っている）．
- **IMAGE_DEF ブロックのリンク**: `image_def.o` は `libkernel.a` 内にあるため，シンボル
  `image_def_block` への参照（`target_kernel_impl.c`）でリンクを強制している（RP2040 版の
  `boot2_image` と同じ手法）．参照を消すとブロックがリンクされず**ボードが起動しなくなる**．
- **Core1（2 個目の Cortex-M33）**: ASP3 はシングルプロセッサカーネルのため使用しない．
  Core1 は bootROM 内で待機したままであり，停止処理は行っていない．
- **pico-sdk は不要**: レジスタ定義は自己完結の `RP2350.h` を使用しており，カーネルの
  ビルド・実行に pico-sdk は必要ない（移植時に IMAGE_DEF ブロックの形式・レジスタ
  フィールドの確認用リファレンスとして参照したのみ．必要なら
  `github.com/raspberrypi/pico-sdk` の `picobin.h` / `hardware_regs` を参照）．
- **BOOTSEL ブート（参考・未検証）**: IMAGE_DEF ブロックを含むため，`picotool load asp.elf` →
  `picotool reboot`（BOOTSEL モード）でも書き込めると考えられるが未検証．動作確認済みの方法は
  OpenOCD（`run` ターゲット）である．
- **ランタイムテスト**: `test/testexec.py`（実施当時は Ruby 版 testexec.rb）による標準テスト 36 件中 34 件 PASS を確認済み
  （2026-06-05，テスト環境の作り方と詳細は `arch/arm_m_gcc/rp2350/PORTING.md` 参照）．
  **cpuexc1 / cpuexc4 は FAIL するが既知の制限**（割込みロック PRIMASK=1 中の CPU 例外が
  HardFault にエスカレートする ARMv7-M/v8-M のアーキテクチャ仕様によるもので，arm_m
  依存部を使う全ターゲット共通）．
- **UART1**: SIOドライバ（`rp2350_uart.[ch]`）は UART0 の 1 ポート構成（`TNUM_SIOP = 1`）．
  UART1 を使用する場合はポート定義の追加に加え，GPIO のファンクション設定が UART0
  （GP0/GP1）分しか行われていないため `hardware_init_hook()` への追加が必要（未検証）．

---

## 同梱物とライセンス

| 同梱物 | 内容 | ライセンス |
|---|---|---|
| `image_def.S` | RP2350 bootROM 用 IMAGE_DEF 最小ブロック（新規作成．形式は pico-sdk の `picobin.h` に基づく） | TOPPERS ライセンス（新規作成） |
| `swd-debug.gdb` | gdb だけで reset→load→ソースデバッグを行う自己完結スクリプト | TOPPERS ライセンス（新規作成） |
| その他のターゲット依存部ファイル | `raspberrypi_pico_gcc`（RP2040 版）および `asp3_pico_sdk`（exshonda 版 RP2350 移植）を基に作成 | TOPPERS ライセンス |

外部から取得するもの:
- **OpenOCD（Raspberry Pi フォーク）**: `github.com/raspberrypi/openocd`（1.4 節）．

---

## 変更履歴

- 2026/06/05
  - ランタイムテスト（testexec.rb）36 件を実機実施（34 PASS / cpuexc1・4 は既知の制限）．
  - `-fdata-sections` を削除（コンフィギュレータのパス3チェック対応）．
  - `SIL_DLY_TIM1/2` を実測値（46/33）にフィッティング（dlynse 全測定 OK）．
  - `target_test.h` に int1 テスト用の `INTNO1`（TIMER0 ALARM1）と `intno1_clear()` を追加．

- 2026/06/04
  - Raspberry Pi Pico 2（RP2350）ターゲット依存部の初版．RP2040 版（raspberrypi_pico_gcc）
    および exshonda 版 RP2350 移植（asp3_pico_sdk）を基に作成．
  - boot stage2 を IMAGE_DEF ブロックに置き換え（bootROM 直接起動）．クロック初期化
    （150MHz）・TICKS ブロック・PADS ISO 解除・FPU(CPACR) 設定を `hardware_init_hook()` に実装．
  - Secure ステート動作のため `ENABLE_TRUSTZONE=1`（EXC_RETURN 対応）．
  - sample1 をシングルコアで実機動作確認（バナー・周期タスク・シリアル受信割込み）．
  - SWD デバッグ支援 make ターゲット（run / openocd / gdb / swd-debug / console）を追加．
