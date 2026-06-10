# TOPPERS/ASP3 OS Awareness for gdb

gdb の Python 機構で TOPPERS/ASP3 カーネルの状態（タスク・同期/通信オブジェクト・
時間イベント・割込み）を可視化するスクリプト集。本体は `os_awareness.py`。
FMP3（マルチプロセッサ）版を ASP3（シングルプロセッサ）用に移植し、
全ターゲット共通のエンジンとして一般化したもの（経緯は `docs/dev/os-awareness.md`）。

## 適用範囲

- エンジン（`os_awareness.py`）は ASP3 のカーネルシンボル（`_kernel_*`）と
  `kernel_cfg.h` にのみ依存し、**ターゲット非依存**。POSIX／QEMU 4機種
  （mps2-an521・xlnx-zcu102・xilinx-zynq-a9・microchip-icicle-kit）で検証済み
  （STM32MP257F-DK 実機は実機側PCで回帰確認）。
- アーキ依存の知識（割込みコントローラ・レディキューのビット方向）は
  ターゲット依存部（`target_os_awareness.py`、後述）に分離されており、
  **無くてもタスク等の主要コマンドは動く**（`intr` の ena/pend 列が省略される）。

## 必要なもの

- **Python 対応の gdb**。Arm GNU Toolchain 同梱の `aarch64-none-elf-gdb` は
  Python 非対応（`Python scripting is not supported`）なので使えない。
  Ubuntu の **`gdb-multiarch`**（arm/aarch64/riscv64 対応）を使う:
  ```bash
  sudo apt-get install -y gdb-multiarch
  ```
  POSIX（linux ターゲット）はネイティブ `gdb` でよい。
- 動的情報（タスク状態・カウント・待ちキュー）は RAM 上にあるため、**実行中ターゲット**
  （QEMU gdbserver／OpenOCD 経由の SWD など）または**コアダンプ**への接続が要る。
  静的情報（`const`）は ELF 単体でも読める。

## 使い方

### QEMU ターゲット（osdebug）

```bash
ninja -C build/mps2_an521-qemu osdebug   # QEMU(-s -S)＋gdb-multiarch を起動
# gdb内: continue →（実行中に）Ctrl-C で halt → atask / stask / sem / intr 等
```

QEMU の出力はビルドディレクトリの `qemu-osdebug.log` に出る。
zcu102／zybo／polarfire も同様（`build/<preset>` を読み替え）。

### POSIX（linux ターゲット）

```bash
gdb ./build/linux/asp -ex 'source scripts/gdb_os_aware/os_awareness.py'
# (gdb) b task → run → atask 等（または run 中に Ctrl-C で halt）
```

### 実機（STM32MP257F-DK）

```bash
ninja -C build/stm32mp257f_dk_arm64 osdebug   # OpenOCD＋gdb-multiarch（SWD）
```

### 静的情報のみ（実行環境不要）

```bash
gdb-multiarch -q -nx build/<preset>/asp.elf \
  -ex 'source scripts/gdb_os_aware/os_awareness.py' -ex stask
```

## コマンド

| コマンド | 内容 |
|---|---|
| `stask` | タスクの静的情報（`_kernel_tinib_table`）: entry/exinf/優先度/スタック/属性 |
| `atask [tid\|name]` | タスクの動的情報（TCB）: 状態(DORMANT/READY/RUNNING/WAIT-xxx/SUSPENDED)・現/基底優先度・**スタック使用量(use/size)**・待ちオブジェクト名。引数なしで全タスク＋レディキュー（`_kernel_p_runtsk`/`_kernel_p_schedtsk`/`_kernel_ready_primap`/`_kernel_dspflg`/`_kernel_enadsp`/`_kernel_excpt_nest_count`）＋待ちキュー |
| `sem` `dtq` `pdq` `flg` `mtx` `mpf` `[id\|name]` | 同期・通信オブジェクトの静的＋動的を 1 コマンドで（属性・容量/初期値・現在値・待ちキュー）。`dtq`/`pdq` は送信 snd / 受信 rcv 別。`mtx` はロック中タスク(owner)。該当オブジェクトが無ければ `no <X> objects` |
| `cyc` `alm` `[id\|name]` | 時間イベントハンドラ（周期・アラーム）の静的＋動的: ハンドラ(symbol)・周期/位相(cyc)・動作状態(STA/STP)・次回起動時刻(evttim) |
| `intr` | 設定済み割込み要求ライン（INTINIB）: INTNO・優先度・属性・**ハンドラ関数名**。さらにターゲット依存部があれば**割込みコントローラの許可/禁止(ena/dis)・ペンディング(pend)** 状態（要 halt）。ハンドラはハンドラ表（ターゲット依存部経由）から解決し、ATT_ISR のラッパ（`_kernel_inthdr_<n>`）は kernel_cfg.c を解析して**実 ISR 関数名と exinf** に解決（例 `sio_isr(exinf=1) [ISR]`）。INTINIB を持たない posix シムは INHINIB 表から登録情報のみ表示 |
| `osa-target [dir]` | ターゲット依存部の明示指定・再ロード（通常は自動発見されるので不要） |

> `atask` のスタック使用量は **保存スタックポインタ(`tskctxb.sp`)** から `(stk+stksz)-sp` で概算する。
> 非実行タスクでは正確、実行中タスクは最後の切替時の値（概算）。休止/範囲外/番地情報なし(posix)は `-`。

- 引数は **数字（ID）または名前**（`CRE_TSK`/`CRE_SEM` で付けた名前）。
- ID 表記は `名前(ID)`（`kernel_cfg.h` を解析。ELF と同じディレクトリ →
  同 `generated/` → cwd → `./gen` の順で探す。見つからなければ数字のみ）。
- gdb 内の表示は全て英語。

## ターゲット依存部（target/chip/core_os_awareness.py）

割込みコントローラ（GIC/NVIC/PLIC）の状態・ハンドラ表・レディキューのビット方向は
アーキ依存のため、ASP3 のソース階層 target→chip→core に対応した3層で実装する:

| 層 | ファイル | 役割 |
|---|---|---|
| target | `target/<name>/target_os_awareness.py` | ボード依存。通常は chip（無ければ core）の API を再エクスポート |
| chip | `arch/<arch>/<chip>/chip_os_awareness.py` | SoC 依存。コントローラのベースアドレス等を持ち core を呼ぶ |
| core | `arch/<arch>/common/core_os_awareness.py` | アーキ共通。レジスタ配置・ハンドラ表・`primap_bit` |

公開 API（エンジンが参照する）:

| 関数 | 内容 |
|---|---|
| `int_enabled(intno)` | 割込み許可状態（True/False）。未定義なら `intr` の ena/pend 列を省略 |
| `int_pending(intno)` | ペンディング状態 |
| `inh_handler(intno)` | 割込みハンドラ番地（ハンドラ表から） |
| `primap_bit(pri)` | レディキュービットマップのビット値（arm/arm64=`0x8000>>pri`、他=`1<<pri`）。未定義ならキュー走査で代替 |

実装済みの層:

| アーキ | core 層 | chip 層 | target 層 |
|---|---|---|---|
| arm64（GICv2） | `arm64_gcc/common` | stm32mp2（GICD=0x4AC10000）・zynqmp（0xF9010000） | stm32mp257f_dk・zcu102 |
| arm（GICv2） | `arm_gcc/common` | zynq7000（0xF8F01000） | zybo_z7 |
| arm_m（NVIC） | `arm_m_gcc/common` | rp2350（パススルー） | mps2_an521（chip層なし）・pico2_arm |
| riscv（PLIC） | `riscv_gcc/common` | polarfire_soc（0x0C000000, cidx=1） | polarfire_soc_kit |
| posix | −（層なし。エンジンの汎用動作） | − | − |

### ターゲット依存部の発見

エンジンが以下の順で自動発見する:

1. 既に `import target_os_awareness` 可能（launcher が `sys.path` 設定済み）
2. 環境変数 `ASP3_OSA_TARGET_DIR` の指すディレクトリ
3. **ELF と同じディレクトリの `CMakeCache.txt`** から `ASP3_TARGET` を読み、
   `<ソースルート>/target/<ASP3_TARGET>/` を解決（CMake ビルドの標準配置）

手動指定は gdb コマンド `osa-target <dir>`。

## 既知の制約

- **zcu102（QEMU 8.2）**: gdbstub 経由の GIC 領域読出しで QEMU 自体が segfault する
  不具合があるため、GICD 読出し（ena/pend 列）を既定無効としている。新しい QEMU や
  実機では環境変数 `ASP3_OSA_GICREAD=1` で有効化できる
  （`target/zcu102_arm64_gcc/target_os_awareness.py` 参照）。
- **polarfire（icicle-kit）**: マルチクラスタ構成（E51＋U54）のため、gdb は
  `target extended-remote` で接続し U54 クラスタ（inferior 2）に attach する必要が
  ある。`osdebug` はこの手順を自動化している（`target.cmake` の
  `ASP3_OSDEBUG_GDB_CONNECT`）。
- **mps2-an521**: Secure 動作のため NVIC は Secure バンクが見える（QEMU では問題なし）。

## ASP3（シングルプロセッサ）と FMP3（マルチプロセッサ）の違い

- 実行状態は **グローバル変数**を直接読む（FMP3 のプロセッサ毎 PCB 走査は廃止）:
  `_kernel_p_runtsk` / `_kernel_p_schedtsk` / `_kernel_ready_queue[TNUM_TPRI]` /
  `_kernel_ready_primap` / `_kernel_dspflg` / `_kernel_enadsp` / `_kernel_excpt_nest_count`。
- TCB 表は構造体配列 `_kernel_tcb_table[]`（FMP3 のポインタ表 `_kernel_p_tcb_table[]` ではない）。
- 同期・通信オブジェクトの CB も**構造体配列** `_kernel_<obj>cb_table[]`（ポインタ表ではない）。
  待ちオブジェクトの逆引きは CB 配列要素の番地一致で行う。
- 待ちオブジェクトは TCB の `p_winfo` を `WINFO_WOBJ*` にキャストして `p_wobjcb` を読む
  （ASP3 の TCB に `p_wobjcb` フィールドは無い）。
- 割込みハンドラ表は単一のハンドラ表（INTNO で添字。arm64/arm/riscv は
  `_kernel_inh_table[]`、arm_m は `_kernel_exc_tbl[]`）。INTINIB は
  `intno`/`intatr`/`intpri` のみ（FMP3 の iprcid/affinity やプロセッサ毎ハンドラ表・
  スピンロック・IPI は無い）。

## 実装メモ（エンジンの頑健化）

- **CB 配列の要素数**は `_kernel_tmax_<obj>id` を正とする。配列シンボルは停止位置の
  コンパイル単位によって extern 宣言（不完全型 `SEMCB []`）に解決されることがあり、
  `sizeof` が当てにならないため。同じ理由で配列シンボルは
  `gdb.lookup_global_symbol` で定義側を引いてから評価する。
- **TINIB のスタック表現**はアーキ毎に異なる（標準 `stk`/`stksz`、arm_m
  `tskinictxb.stk_top/stk_bottom`、posix `tskinictxb.stksz` のみ）。エンジンが
  3形態を吸収する。
