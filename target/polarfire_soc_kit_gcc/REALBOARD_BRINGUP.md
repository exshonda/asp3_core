# PolarFire SoC Discovery Kit 実機ブリングアップ（作業記録）

TOPPERS/ASP3 Core を **PolarFire SoC Discovery Kit**（FlashPro5 `1514:2008`，
RV64GC，U54）実機で JTAG 起動し，最終的に TTSP3 を回すための実機ブリングアップ記録．
**2026-06-16：sample1 実機完動**（バナー＋タスク＋タイマ割込み）．要点は次の2つ：

1. **`reset init` をしない**でロードする（HSS の MSS IO/クロック設定を保持．§4 参照）．
2. **起動時に CLINT MSIP をクリア**する（`chip_initialize`．§4a 参照）．

> 注：asp3 の polarfire ターゲットは元々 QEMU 専用で，`target.cmake` が実機ロード手段を
> 「実機対応時に整備する」と先送りしていた．本書はその実機対応の進行中記録．

---

## 1. 環境・接続

- ボード：**Discovery Kit**（Icicle ではない）．JTAG/UART とも FlashPro5（FT4232，`1514:2008`）．
  - if0=JTAG，**if1=MMUART1=コンソール**（FMP3 実績）．udev 適用後 `/dev/ttyUSB1`（採番は接続順で動く）．
- ツール：SoftConsole-v2022.2（`/home/honda/Microchip/SoftConsole-v2022.2-RISC-V-747`）
  の openocd + `riscv64-unknown-elf-gdb`．
- openocd 起動：
  ```sh
  SC=/home/honda/Microchip/SoftConsole-v2022.2-RISC-V-747
  LD_LIBRARY_PATH=$SC/openocd/bin:$SC/fpServer/lib \
    $SC/openocd/bin/openocd -s $SC/openocd/share/openocd/scripts \
    -c "set DEVICE MPFS" -f board/microsemi-riscv.cfg
  # → found 5 harts / Listening on port 3333（TCL RPC=6666, telnet=4444）
  ```
- **FMP3 は同手順で完動**（`/tmp/fmp_pf/fmp`，`thread apply all set $pc=_start; continue`，
  コンソール=ttyUSB1）＝ボード/openocd/gdb/UART 経路は健全．これが切り分けの基準．

---

## 2. 実施済みの変更（asp3_core 側・実機ブリングアップの足場）

| ファイル | 変更 |
|---|---|
| `polarfire_soc_kit_lim.ld`（新規） | 実機用リンカ．全セクションを **L2-LIM（0x08000000）** に配置（QEMU 版は DDR 0x80000000）．JTAG デバッグ＝lim-debug 構成 |
| `target.cmake` | `POLARFIRE_QEMU=OFF` 時に L2-LIM ld を選択．`POLARFIRE_DISCOVERY` オプション追加（ON で `POLARFIRE_BOARD_DISCOVERY` 定義） |
| `polarfire_soc_kit.h` | `POLARFIRE_BOARD_DISCOVERY` 時に **USE_UART1（MMUART1）** ＋ターゲット名 "Discovery Kit" |

ビルド：`cmake --preset polarfire_soc_kit -B build/polarfire_soc_kit_hw -DPOLARFIRE_DISCOVERY=ON && cmake --build build/polarfire_soc_kit_hw`
（Entry=0x08000000，~63KB＝L2-LIM 512KB に余裕で収まる）

ロード（gdb）：
```
monitor reset init
load
thread apply all set $pc=start   # mhartid=1 の U54 が起動，他は park_hart
continue
```

---

## 3. 確認できた事実（実機，クリーンな openocd）

カーネルは **mhartid=1 の U54 で正常起動**し，以下を全通過：

1. `start.S`：mhartid 判定通過（BOOT_HARTID=1）→ sp/gp → bss クリア → data コピー
2. `sta_ker` → `target_initialize` → `_kernel_chip_initialize`
   （**mtvec 設定・PLIC global/context・Machine Timer・MIE 許可・core_initialize 全 OK**）
3. SYSREG で **MMUART1 サブブロッククロック有効化**（CLOCK_CR=0x41）＋リセット解除
4. `sio_initialize` → `target_fput_initialize` → `mmuart_opn_por`（**MMUART1 baud 設定**）
5. **`target_fput_log` 到達**：banner 文字（`\n`,`T`,…）を THR(0x20100000) へ実際に書込み

→ **起動・カーネル・出力経路（THR 書込み）まで完動**．

### MMUART1 レジスタ（asp3 設定後＝FMP3 動作時と一致）
LCR=0x03，DLR=0x20（DLAB=1 で読む），DMR=0x01（分周比 288），DFR=0x18，MM0=0x80．
`SIO0_BAUD=81`/`SIO0_FBAUDDIV=24`（FMP3 と同一値）．**baud は正しい**．
（注：DLR は DLAB ゲート．DLAB=0 で読むと 0x00 に見えるので誤読注意）

---

## 4. 解決策①：`reset init` をしない（HSS の MSS IO/クロックを保持）

UART 無音の真因は **`monitor reset init` が SYSREG/MSSIO をリセットし，HSS が起動時に
設定した MSS IO（IOMUX＋MSSIO バンク電気設定＋クロック）を破壊**していたこと．
FMP3 は `reset init` 後に mpfs_hal `system_startup`（`mss_nwc_init`＝IOMUX＋MSSIO バンク
IO＋IO 較正，`mss_pll`，`mss_io`）で**全再適用**するが，asp3 は system_startup を持たない．

→ asp3 は **`reset init` せず**にロードすることで，**HSS の初期化済み状態をそのまま使う**：

```
monitor halt          # reset init はしない（HSSのIO/クロック設定を保持）
load                  # L2-LIM(0x08000000)へasp.elfをロード
thread apply all set $pc=start   # 全hartにpc=start（mhartid=1が起動，他はpark）
continue
```

- 設計判断として妥当：asp3 は HSS 前提（HSS が eNVM から MSS IO/DDR/クロックを初期化済み）．
  mpfs_hal の取り込み（DDR/IO/PMP 含む大規模統合）を避け，HSS の成果を再利用する．
- 注：`reset init` 後に IOMUX を手動で Discovery 値に書いても**無音のまま**だった
  （IOMUX_CR だけでは不足．MSSIO バンク電気設定＋IO 較正＝`mss_nwc_init` 相当が要る）．
  「reset init を避ける」方が**確実かつ低コスト**．

## 4a. 解決策②：起動時に CLINT MSIP をクリア（ソース修正）

`reset init` をしないと，**HSS が残した CLINT のソフトウェア割込み保留（MSIP）**が，
`chip_initialize` の `riscv_set_mie(MIE_MSIE|...)` で発火し，
`Unregistered interrupt occurs. Interrupt Number is 0x10003`（MSI＝mcause 3）になる．
FMP3 はマルチコアで MSI を IPI に使い `clear_msip` するが（`arch/riscv_gcc/common/msi_ipi.c`），
シングルプロセッサの asp3 は MSI を使わないため未対応だった．

→ `arch/riscv_gcc/polarfire_soc/chip_kernel_impl.c` の `chip_initialize` で，MIE 許可の直前に
ブートハートの MSIP をクリア（既存 `CLINT_MSIP` マクロ＝FMP3 と同一定義 `polarfire_soc.h`）：

```c
sil_wrw_mem(CLINT_MSIP(TOPPERS_BOOT_HARTID), 0U);  /* FMP3のclear_msip相当．QEMUでは無害 */
```

これで openocd 側の手動 MSIP クリアは不要．

## 4b. 検証結果（実機 Discovery，2026-06-16）

上記①②で **sample1 が実機完動**：バナー（"...for PolarFire SoC Discovery Kit"）→
`System logging task is started` → `Sample program starts` → `task1 is running (NNN)` が
カウンタ増加（**タイマ割込み動作**）・未登録割込みなし．コンソール=`/dev/ttyUSB1`（MMUART1）．

---

## 5. 残作業

- **TTSP3 実機ドライバ統合**：`test/ttsp/run_ttsp.py` に polarfire 実機ロード（SoftConsole
  openocd 常駐＋上記 gdb 手順，**reset init なし**）の run_hw を追加し，`polarfire_soc_kit_hw`
  エントリ（`target/.../ttsp3/ttsp_target.py`）を作る．staticAPI＋代表 functional を実行．
- **将来（任意）**：mpfs_hal `system_startup` 統合で `reset init` 可能化＋DDR/IO/PMP 正規化．
  現状の「HSS 前提・reset init なし」で実機検証は足りるため優先度は低い．

**JTAG の注意**：FT4232 は ~15-25 reset/load サイクルで wedge する（全 hart park・mhartid 読みが
0x4 固定等）．**USB 抜き差し**で復旧（HSS 再ブート＝IOMUX/IO クリーン再初期化）．`pkill -x openocd`
で掃除（`-f` は自分のシェルを巻き込むので不可）．`mpfs.hart1_u54_1` の mhartid は **1 ではない**
（openocd の hart 番号 ≠ RISC-V mhartid）ので，gdb の `thread apply all`＋`continue` で全 hart に
pc=start を設定して mhartid=1 を確実に走らせる方式が堅実．
