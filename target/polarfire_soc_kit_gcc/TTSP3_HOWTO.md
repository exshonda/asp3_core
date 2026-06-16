# PolarFire SoC Discovery Kit（U54 / RV64GC）実機での TTSP3 適合性テスト

TOPPERS テストスイート **TTSP3** の API 適合性テスト（`api_test`）を
**PolarFire SoC Discovery Kit 実機**（RV64GC，U54，SiFive 互換）で実行・自動判定する
手順と**実機結果**．

- ドライバ（QEMU/実機共通）：`test/ttsp/run_ttsp.py`（`--target polarfire_soc_kit_hw`）
- 設計・QEMU 結果（正本）：`docs/dev/ttsp3-conformance.md`（**本書は実機側に特化**）
- 実機ブリングアップの経緯・落とし穴：`REALBOARD_BRINGUP.md`（同フォルダ）
- **TTSP3 は読み取り専用**（一切改変しない）．

---

## 1. 方式（SoftConsole openocd + gdb，reset init なし）

QEMU の代わりに **SoftConsole 同梱 openocd（FlashPro5/FT4232）を GDB サーバとして常駐**させ，
各テストごとに **gdb で L2-LIM(0x08000000) へロード→全 hart に pc=start→continue** する．
UART(`/dev/ttyUSB1`＝MMUART1) を別途キャプチャして `All check points passed.` で判定する
（系統分類・ttg 生成・判定ロジックは QEMU 版と共通）．

### asp3 実機対応の要点（`REALBOARD_BRINGUP.md` に詳細）

- **`reset init` をしない**：asp3 は mpfs_hal の system_startup を持たず，**HSS が起動時に
  設定した MSS IO（IOMUX/MSSIO バンク）・クロックに依存**する．`reset init` はこれを破壊して
  MMUART1 が無音になるため，`monitor halt`＋`load`＋`continue` でロードし HSS 設定を保持する．
- **起動時 CLINT MSIP クリア**：`chip_initialize` で実施済み（`reset init` しないと HSS 残存の
  MSIP が MSIE 許可時に発火し未登録割込みになるため．FMP3 の `clear_msip` 相当）．
- **L2-LIM リンク**：実機ビルド（`POLARFIRE_QEMU=OFF`）は `polarfire_soc_kit_lim.ld` で
  全セクションを L2-LIM に配置（JTAG デバッグ＝lim-debug）．
- **Discovery ボード選択**：`-DPOLARFIRE_DISCOVERY=ON` で MMUART1（コンソール）を選択．

### 構成（asp3_core 側）

| 物 | 役割 |
|---|---|
| `target/polarfire_soc_kit_gcc/ttsp3/ttsp_target_test.{c,h}` | TTSP ターゲットテスト資産（RV64GC） |
| `target/polarfire_soc_kit_gcc/ttsp3/ttsp_target.cfg` | 空（自動 tecsgen.cfg スタブが吸収） |
| `target/polarfire_soc_kit_gcc/ttsp3/ttsp_target.py` | ドライバが自動探索する TTSP 定義（`polarfire_soc_kit_hw`） |
| `polarfire_soc_kit_lim.ld`（実機リンカ）・`chip_kernel_impl.c`（MSIP クリア） | 実機ブリングアップ（`REALBOARD_BRINGUP.md`） |

ドライバの `TTSP_TARGETS["polarfire_soc_kit_hw"]["hw"]` に serial / baud / capture / gdb /
gdb_port を集約．`$TTSP_HW_SERIAL` / `$TTSP_HW_CAPTURE` / `$TTSP_RISCV_GDB` で上書き可．

### 前提

| 必要物 | 本開発機での所在 |
|---|---|
| SoftConsole（openocd＋riscv64-unknown-elf-gdb） | `/home/honda/Microchip/SoftConsole-v2022.2-RISC-V-747` |
| riscv64-unknown-elf gcc（ビルド用） | apt（13.2）．**PATH へ** |
| Ruby（functional yaml の ttg 生成） | apt（3.2） |
| PolarFire Discovery Kit 実機 + FlashPro5 | `1514:2008`．**UART=`/dev/ttyUSB1`**（MMUART1=FT4232 if1）115200 |

> 注：ttyUSB の番号は接続順で動く．`udevadm info -q property -n /dev/ttyUSB1` で
> `ID_MODEL=Embedded_FlashPro5`・`ID_USB_INTERFACE_NUM=01` を確認．ずれる環境では
> `$TTSP_HW_SERIAL` で上書き．

---

## 2. 実行

```bash
export TTSP3_ROOT=/home/honda/TOPPERS/ASP3CORE/ttsp3   # この開発機のTTSP3パス（../ttsp3）

# (1) SoftConsole openocd を GDB サーバとして常駐（一度だけ）
SC=/home/honda/Microchip/SoftConsole-v2022.2-RISC-V-747
LD_LIBRARY_PATH=$SC/openocd/bin:$SC/fpServer/lib \
  $SC/openocd/bin/openocd -s $SC/openocd/share/openocd/scripts \
  -c "set DEVICE MPFS" -f board/microsemi-riscv.cfg &
#   → "found 5 harts" / "Listening on port 3333"

# (2) 1件だけ（疎通確認）
python3 test/ttsp/run_ttsp.py --target polarfire_soc_kit_hw \
  api_test/ASP/staticAPI/CRE_TSK/CRE_TSK_g-3
# staticAPI 全件（error=cfgのみ／runtime・yaml=実機）
python3 test/ttsp/run_ttsp.py --target polarfire_soc_kit_hw --tap api_test/ASP/staticAPI
# 機能テスト1モジュール（代表スポット確認）
python3 test/ttsp/run_ttsp.py --target polarfire_soc_kit_hw --tap api_test/ASP/semaphore
```

- **逐次実行のみ**（シリアル1本＝実機1台）．`--jobs` は使わない．
- error 系は cfg がエラーを出してビルド失敗すれば PASS＝**実機不要・QEMU 版と同一**．
- 実機実行は1件あたり ~15-25s（cfg+ビルド＋gdb ロード（~2s）＋完走検出で早期打切り）．

### 仕組み（run_hw_openocd_gdb_riscv）

1. `stty` で UART を raw 設定し，`timeout <capture> cat /dev/ttyUSB1` を背景起動．
2. gdb を背景起動：`target extended-remote :3333; monitor halt; load;
   thread apply all set $pc=start; continue`（**reset init なし**）．
3. UART ログを **最後のカーネルバナー以降**に限定して `All check points passed.` を検出．
   検出で早期に gdb/cat を停止（`_current_boot`．harts は走り続ける）．

---

## 3. 実機結果（PolarFire SoC Discovery Kit）

> 環境：本開発機（Ubuntu 24.04）＋ Discovery Kit 実機（FlashPro5）．SoftConsole-v2022.2
> openocd/gdb，riscv64-unknown-elf 13.2，Ruby 3.2．**TTSP3 は変更なし**．asp3_core 側は
> 実機ブリングアップ（`REALBOARD_BRINGUP.md`：L2-LIM ld・Discovery 選択・MSIP クリア）と
> TTSP 資産・ドライバの run_hw 追加のみ．

| 範囲 | 件数 | 結果 | 日付 |
|---|---|---|---|
| `api_test/ASP/staticAPI` 全件（error 113＋runtime 5＋yaml 20） | 138 | **PASS 123・FAIL 6・SKIP 9** | 2026-06-16 |
| `api_test/ASP/semaphore`（functional・代表スポット） | 117 | **PASS 73・FAIL 0・SKIP 44** | 2026-06-16 |

```
== TTSP3 [polarfire_soc_kit_hw] PASS=123 FAIL=6 SKIP=9 / 138 ==   # staticAPI
== TTSP3 [polarfire_soc_kit_hw] PASS=73  FAIL=0 SKIP=44 / 117 ==  # semaphore（functional 代表）
```

- **functional 代表 `semaphore` は FAIL 0**（実行できた全 73 件 PASS）．SKIP 44 は HW タイマ
  早送り（`gain_tick`）依存の timeout 系・`_ntc`/`_ten` 変種で，RISC-V＝QEMU 版と同じ SKIP 規則．
  pico（arm_m）と同一の 73/0/44．**実機ロード（gdb・reset init なし）の連続実行で JTAG wedge なし**．

- **実機（runtime/yaml）の FAIL はゼロ**（実行できた 16 件すべて完走）．SKIP 9 は HW 依存
  （割込み発生 `int_raise`＝CFG_INT_d-1/d-2・CRE_ISR_d・DEF_INH_e，HW タイマ早送り `gain_tick`＝
  CRE_ALM_f・CRE_CYC_h/i-1/i-2，CPU 例外発生 `cpuexc_raise`＝DEF_EXC_d）で，RISC-V＝QEMU 版と
  同じ SKIP 規則（`unsupported_hw`/`skip_modules`）．

### staticAPI の FAIL 6（ターゲット依存の cfg エラーコード差・既知）

すべて error 系で，TTSP3 の `err_code.txt` が参照ターゲット（zybo＝arm）基準であり，RISC-V
（PolarFire）とは割込み/例外モデル・属性の有効ビットが異なるため cfg のエラーコードが一致しない
（**機能適合性とは別問題**．kernel/・arch/ は不変）．通知属性等の E_PAR 未検出：
`ATT_INI_b`・`ATT_TER_b`・`CRE_ALM_d-1`・`CRE_CYC_d-1`・`CRE_TSK_b`・`DEF_INH_c`．
同じ非 arm 系として **arm_m（mps2/pico）より FAIL 数は少ない**（pico=19 に対し polarfire=6）．

---

## 4. トラブルシュート

| 症状 | 対処 |
|---|---|
| 1件目だけ FAIL（合格文字列なし） | 前操作直後のボード遷移状態のことがある．再実行で解消（gdb が halt→load し直す） |
| 全 hart が park・mhartid 読みが 0x4 固定等／無出力が続く | **FT4232 JTAG の wedge**（~15-25 サイクル）．**USB 抜き差し**で HSS 再ブート→openocd 再起動 |
| `Unsupported DTM` 等で openocd 起動失敗 | `pkill -x openocd`（`-f` は自分のシェルを巻き込むので不可）．残存プロセスを掃除して再起動 |
| UART が無出力 | `/dev/ttyUSB1` が FlashPro5 if1 か `udevadm` で確認．`$TTSP_HW_SERIAL` で上書き |
| gdb が見つからない | SoftConsole の `riscv64-unknown-elf-gdb` を `$TTSP_RISCV_GDB` で指定 |
| error 系が FAIL | §3 既知のターゲット依存 cfg エラーコード差（RISC-V＝参照 arm と割込み/例外モデルが異なる） |

---

## 同梱物とライセンス

| 同梱物 | 内容 | ライセンス |
|---|---|---|
| `target/polarfire_soc_kit_gcc/ttsp3/ttsp_target_test.{c,h}` | TTSP ターゲットテスト資産（RV64GC） | TOPPERS ライセンス |
| `target/polarfire_soc_kit_gcc/ttsp3/ttsp_target.cfg` | 空（自動 tecsgen.cfg スタブが結線を吸収） | TOPPERS ライセンス |

> 実機ロードは SoftConsole openocd + gdb（**reset init なし**）．openocd 設定・接続の
> 詳細は `target_user.md`／`REALBOARD_BRINGUP.md` を参照．
