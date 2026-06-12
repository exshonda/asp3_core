# TOPPERS/ASP3 MIMXRT685-EVK ターゲット依存部 利用ガイド

NXP **EVK-MIMXRT685**（i.MX RT685, Cortex-M33）向け TOPPERS/ASP3 ターゲット依存部の解説と
利用方法．genuine ASP3 3.7.0 の `target/mimxrt685evk_gcc`（TECS＋Ruby cfg）を，asp3_core の
非TECS＋Python cfg＋CMake 構成へ変換したもの（`target_user.txt` 3.7.0 版が原本）．

---

## 概要

`mimxrt685evk_gcc` 依存部は，NXP **EVK-MIMXRT685** ボード（SoC: i.MX RT685＝MIMXRT685S）を
サポートする．i.MX RT685 は Cortex-M33＋HiFi4 DSP のヘテロ構成だが，ASP3 は **CM33 のみ**を
使用する（DSP は停止のまま）．MCUXpresso SDK は使用しない（ベアメタル自己完結．SDK 統合は
外側リポジトリ `asp3_mcuxsdk` で行う）．

- ターゲット略称: `mimxrt685evk_gcc`
  （BOARD=`mimxrt685evk`, CHIP=`imxrt600`, PRC=`arm_m`, TOOL=`gcc`）
- チップ依存部: `arch/arm_m_gcc/imxrt600/`
- 実行方式: **フラッシュ書込み実行（ROM実行／XIP）のみ**サポート

### ボード
動作確認を行ったボード:
- NXP **EVK-MIMXRT685**（MIMXRT685-EVK．i.MX RT685: Cortex-M33 300MHz / FPU(FPv5-SP) /
  4.5MB SRAM / 外付け 64MB Octal SPI フラッシュ（FlexSPI 接続，0x08000000 にマップ））
- https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/i-mx-rt600-evaluation-kit:MIMXRT685-EVK

### メモリマップ

```
0x08000000 - 0x08800000 : 外付けフラッシュ（リンカスクリプトでは8MB分を使用）
0x20000000 - 0x20480000 : 内蔵SRAM（4.5MB）
0x40000000 - 0x40160000 : I/O領域
```

変更する場合は `mimxrt685.ld` を修正する．

### XIP実行とブートイメージ

ブートROMは外付けフラッシュ先頭の **FlexSPI設定ブロック**（オフセット0x400，
`flash_config.c`＝`.flash_conf` セクション）を読んでフラッシュを設定し，0x1000 に置かれた
ベクタテーブルからイメージを起動する．

- `flash_config.c/h` は NXP 提供（BSD-3-Clause）の設定ブロックをそのまま使用
- ベクタテーブルのインデックス9（オフセット0x24）は**イメージタイプ**：プレーンイメージ
  （TZ-M設定なし）として bit14 をセットする（`target_kernel.py` の `GenResVectVal`）
- `TOPPERS_ENABLE_TRUSTZONE` は**定義しない**（プレーンイメージ起動の動作実績構成）
- スタートアップ（`start_imxrt600.S`）はリンカスクリプトのセクションテーブルを用いた
  テーブル駆動で DATA/BSS を初期化する（XIP のため DATA はフラッシュ→SRAM コピーが必要）

### クロック・電源初期化

`target_kernel_impl.c` の `hardware_init_hook()` が行う：

1. **FBB（forward body-bias）モード遷移**＝RAM 上にコピーしたコードでディープスリープに
   入り PMIC 割込みで復帰（コア電圧 1.13V 設定を含む）
2. **PMIC（PCA9420）** へ I2C（Flexcomm15）で SW1=1.15V を設定
3. **メインPLL** を 24MHz XTAL から 300MHz に設定（`CPU_CLOCK_HZ`）
4. FlexSPI クロック＝メインクロック/3，frg_pll＝50MHz（`FRG_PLL_HZ`）
5. **キャッシュ**を有効化（FlexSPI 領域＝write-back）
6. デバッグコンソールの IOCON（P0_1=TX, P0_2=RX, FC0）

---

## カーネルのコンフィギュレーション

### カーネルタイマ方式
`target.cmake` の `USE_TIM_AS_HRT` 定義で選択する（既定: `TIM`）．
- `TIM` : CTIMER0 を高分解能タイマ（HRT）として使用（既定・動作実績あり）．
  プリスケーラで 1MHz（1μs）を生成する 32bit アップカウンタ＋マッチ割込み
- `SYSTICK` : SysTick による方式（未検証）

### シリアル（SIO）
チップ内蔵の Flexcomm USART を使用する（`arch/arm_m_gcc/imxrt600/imxrt600_usart.c`）．
- デバッグコンソール: **Flexcomm0 USART0**（オンボード MCU-Link の VCOM に接続）
- 設定: 115200bps・8bit・パリティなし・ストップ1bit
- ポート数: 1（`TNUM_SIOP`／`TNUM_PORT`）

### 割込み
- 割込み優先度のビット幅 `TBITW_IPRI`=3（`TMIN_INTPRI`=-7）
- `TMAX_INTNO` = 59+16（`mimxrt685-evk.h`）

---

## ビルド方法

```bash
cmake --preset mimxrt685evk -B build/mimxrt685evk
cmake --build build/mimxrt685evk
```

生成物: `build/mimxrt685evk/asp.elf`

テストプログラムのビルド（例: 移植検証テスト）:

```bash
cmake --preset mimxrt685evk -B build/mimxrt685evk -DASP3_APP=test_porting
cmake --build build/mimxrt685evk
```

---

## 実行方法（実機）

### 接続
EVK-MIMXRT685 のデバッグ用 USB（J5）と PC を接続する．オンボードのデバッグプローブ
（LPC-Link2）は CMSIS-DAP（LinkServer）または J-Link ファームウェアで動作し，
同じ USB で VCOM（デバッグシリアル 115200bps，Linux では `/dev/ttyACM0`）も提供される．

**実機検証済みの構成は J-Link ファームウェア**（lsusb で `1366:0105 SEGGER J-Link`）．
ホスト側に SEGGER J-Link Software が必要：

```bash
# MCUXpresso Installer 経由（components: JLink）または segger.com から deb を取得して
sudo dpkg -i JLink_Linux_V930_x86_64.deb
# 接続済みプローブに udev ルールを適用（または抜き差し）
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### 書込み（実機検証済みの手順）

**JLinkExe**（コマンドファイル方式）：

```bash
cd build/mimxrt685evk
printf 'loadfile asp.elf\nr\ng\nqc\n' > flash.jlink
JLinkExe -device MIMXRT685S_M33 -if SWD -speed 4000 -autoconnect 1 -NoGui 1 \
         -CommandFile flash.jlink
```

`loadfile` でフラッシュ書込み（比較→消去→書込み＝差分のみ），`r`（リセット）→
`g`（実行）．CMake ターゲットでも同じことができる：

```bash
ninja -C build/mimxrt685evk jlink-run    # 書込み→リセット→実行
ninja -C build/mimxrt685evk console     # シリアルコンソール（picocom等）
ninja -C build/mimxrt685evk jlink-debug # J-Link GDB Server＋gdbでロード＆デバッグ
ninja -C build/mimxrt685evk osdebug     # 同上＋OS-awareness（gdb-multiarch）
```

その他の書込み手段（未検証）：

1. **LinkServer**（CMSIS-DAP ファームウェアの場合）:
   `LinkServer flash MIMXRT685S:EVK-MIMXRT685 load build/mimxrt685evk/asp.elf`
2. **MCUXpresso IDE**: GUI からのデバッグ実行（旧 3.7.0 移植の `MCUXpresso/` 手順）

### VCOM 利用時の注意

VCOM（`/dev/ttyACM0`）を読むプロセスは**同時に1つだけ**にすること．複数のリーダ
（`cat`・`picocom` 等）が同じ tty を開くとバイトを奪い合い，出力が欠落・断片化して
「文字化け」「無出力」に見える（実機検証時にこれで原因究明に手間取った）．
`fuser -v /dev/ttyACM0` で残留プロセスを確認できる．

### テスト実行（ボードランナ）

`scripts/ci/run_board_mimxrt685evk.sh` が UART キャプチャ→J-Link 書込み→完走マーカ
待ちを行う（`test/testexec.py` の TARGET_RUN 用．ボードと tty を共有するため
**テストバッチの並行実行は禁止**）：

```bash
scripts/ci/run_testexec.py --options "--preset mimxrt685evk" \
    --run "bash $PWD/scripts/ci/run_board_mimxrt685evk.sh 120" \
    --workdir build/testexec-mimxrt685evk task1 sem1 flg1
```

### ジャンパ JP22（重要）
JP22 の中央 2 番ピンは SoC の LDO_ENABLE ピンに接続されている．デフォルト（オープン）では
フローティングとなり，データシートの禁止事項に当たる．**2-3 ピンを短絡**し，外部 PMIC から
コア電圧を供給する設定を推奨する（本依存部は PMIC を 1.15V に設定する）．

---

## 検証状況

すべて実機（EVK-MIMXRT685・J-Link 書込み）で確認（2026-06-12）：

| 項目 | 状態 |
|---|---|
| ビルド（警告ゼロ） | CI で確認 |
| test_porting（6項目） | **6/6 passed** |
| sample1 | バナー・task1 周期実行・`r`（rot_rdq で task1→2→3 切替）確認 |
| testexec（機能テスト35件） | **33/35 PASS**（cpuexc10 は SKIP=not necessary．cpuexc1/cpuexc4 は arm_m 共通の既知 FAIL＝上流由来，`docs/dev/issue-cpuexc-armm.md`） |
| sil_dly_nse 較正 | `SIL_DLY_TIM1`=23 / `SIL_DLY_TIM2`=16（実機 test_dlynse で較正・全項目 OK）|
| OS Awareness | `osdebug`（J-Link GDB Server＋gdb-multiarch）で atask/stask/sem/cyc/intr（NVIC ena/pend 含む）を確認 |

---

## 既知の制約

- ROM実行（XIP）のみ．RAM実行リンカスクリプトは未提供
- XIP 中はフラッシュ書換え不可（フラッシュドライバは対象外）
- HiFi4 DSP・TrustZone（Secure実行）は未サポート
- オーバランハンドラ（`TOPPERS_TARGET_SUPPORT_OVRHDR`）未サポート
