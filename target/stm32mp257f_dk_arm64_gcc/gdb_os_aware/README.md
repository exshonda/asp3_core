# TOPPERS/ASP3 OS Awareness for gdb

gdb の Python 機構で TOPPERS/ASP3 カーネルの状態（タスク・同期/通信オブジェクト・割込み）を
可視化するスクリプト集（Step2: OS Awareness）。本体は `os_awareness.py`。
FMP3（マルチプロセッサ）版を ASP3（シングルプロセッサ）用に移植したもの。

## 必要なもの

- **Python 対応の gdb**。Arm GNU Toolchain 同梱の `aarch64-none-elf-gdb` は **Python 非対応**
  （`Python scripting is not supported`）なので使えない。Ubuntu の **`gdb-multiarch`**
  （Python 対応・AArch64 対応）を使う:
  ```bash
  sudo apt-get install -y gdb-multiarch
  ```
- 動的情報（タスク状態・カウント・待ちキュー）は RAM 上にあるため，**実行中ターゲット**
  （OpenOCD 経由の SWD など）または**コアダンプ**への接続が要る。静的情報（`const`）は
  ELF 単体でも読める。

## コマンド

| コマンド | 内容 |
|---|---|
| `stask` | タスクの静的情報（`_kernel_tinib_table`）: entry/exinf/優先度/スタック/属性 |
| `atask [tid\|name]` | タスクの動的情報（TCB）: 状態(DORMANT/READY/RUNNING/WAIT-xxx/SUSPENDED)・現/基底優先度・**スタック使用量(use/size)**・待ちオブジェクト名。引数なしで全タスク＋レディキュー（`_kernel_p_runtsk`/`_kernel_p_schedtsk`/`_kernel_ready_primap`/`_kernel_dspflg`/`_kernel_enadsp`/`_kernel_excpt_nest_count`）＋待ちキュー |
| `sem` `dtq` `pdq` `flg` `mtx` `mpf` `[id\|name]` | 同期・通信オブジェクトの静的＋動的を 1 コマンドで（属性・容量/初期値・現在値・待ちキュー）。`dtq`/`pdq` は送信 snd / 受信 rcv 別。`mtx` はロック中タスク(owner)。該当オブジェクトが無ければ `no <X> objects` |
| `cyc` `alm` `[id\|name]` | 時間イベントハンドラ（周期・アラーム）の静的＋動的: ハンドラ(symbol)・周期/位相(cyc)・動作状態(STA/STP)・次回起動時刻(evttim) |
| `intr` | 設定済み割込み要求ライン（INTINIB）: INTNO・優先度・属性・**ハンドラ関数名**。さらに **GIC の許可/禁止(ena/dis)・ペンディング(pend)** 状態（ena/pend は実機接続時のみ）。ハンドラは `_kernel_inh_table`（INTNO で添字, ターゲット依存部経由）から解決し，ATT_ISR のラッパ（`_kernel_inthdr_<n>`）は kernel_cfg.c を解析して **実 ISR 関数名と exinf** に解決（例 `sio_isr(exinf=1) [ISR]`） |

> `atask` のスタック使用量は **保存スタックポインタ(`tskctxb.sp`)** から `(stk+stksz)-sp` で概算する。
> 非実行タスクでは正確，実行中タスクは最後の切替時の値（概算）。休止/範囲外は `-`。

- 引数は **数字（ID）または名前**（`CRE_TSK`/`CRE_SEM` で付けた名前）。
- ID 表記は `名前(ID)`（`kernel_cfg.h` を解析。見つからなければ数字のみ）。
- gdb 内の表示は全て英語。

## ASP3（シングルプロセッサ）と FMP3（マルチプロセッサ）の違い

- 実行状態は **グローバル変数**を直接読む（FMP3 のプロセッサ毎 PCB 走査は廃止）:
  `_kernel_p_runtsk` / `_kernel_p_schedtsk` / `_kernel_ready_queue[TNUM_TPRI]` /
  `_kernel_ready_primap` / `_kernel_dspflg` / `_kernel_enadsp` / `_kernel_excpt_nest_count`。
- TCB 表は構造体配列 `_kernel_tcb_table[]`（FMP3 のポインタ表 `_kernel_p_tcb_table[]` ではない）。
- 同期・通信オブジェクトの CB も**構造体配列** `_kernel_<obj>cb_table[]`（ポインタ表ではない）。
  待ちオブジェクトの逆引きは CB 配列要素の番地一致で行う。
- 待ちオブジェクトは TCB の `p_winfo` を `WINFO_WOBJ*` にキャストして `p_wobjcb` を読む
  （ASP3 の TCB に `p_wobjcb` フィールドは無い）。
- 割込みハンドラ表は単一の `_kernel_inh_table[]`（INTNO で添字）。INTINIB は `intno`/`intatr`/`intpri`
  のみ（FMP3 の iprcid/affinity やプロセッサ毎ハンドラ表・スピンロック・IPI は無い）。

## 使い方

### 静的情報のみ（実機不要）
```bash
gdb-multiarch -q -nx <OBJ>/asp \
  -ex 'source <ASP3>/target/stm32mp257f_dk_arm64_gcc/gdb_os_aware/os_awareness.py' -ex stask
```
`<OBJ>/asp` と同じディレクトリ（または cwd / `./gen`）に `kernel_cfg.h` があれば名前解決される。

### 実機（STM32MP257F-DK ターゲット）
ビルドディレクトリで OpenOCD（SWD）に接続し，`continue`→Ctrl-C で halt してから
`stask`/`atask`/`sem`/… を実行する。`intr` の ena/pend 列は実機接続（halt）時のみ表示される。

## ターゲット依存の割込み状態（GIC）・ハンドラ表

割込みの許可/禁止・ペンディング状態は GIC レジスタに，ハンドラ表（`_kernel_inh_table`）は
arm64 共通部の実装に依存するため，**ターゲット依存部**にレイヤ構造で実装している
（ASP3 のソース階層 target→chip→core に対応）:

| ファイル | 場所 | 役割 |
|---|---|---|
| `target_os_awareness.py` | `target/stm32mp257f_dk_arm64_gcc/` | ボード依存。今回は追加なしで chip を再エクスポート |
| `chip_os_awareness.py` | `arch/arm64_gcc/stm32mp2/` | SoC 依存。GICD ベース(0x4AC10000)を持ち core を呼ぶ |
| `core_os_awareness.py` | `arch/arm64_gcc/common/` | arm64 共通。GICv2 の `GICD_ISENABLER`/`ISPENDR` と，ハンドラ表 `_kernel_inh_table[intno]` を読む |

- `target_os_awareness.py` → `chip_os_awareness.py` → `core_os_awareness.py` と `import` で連鎖。
- `os_awareness.py` は **任意 import**（`import target_os_awareness`）。読めれば `intr` に
  ena/pend/handler 列を出し，読めなければその列を省く。
- GIC レジスタ読み出しのため実機/コアダンプ接続（halt）が必要。ハンドラ表は const（.rodata）
  なので **ELF 単体（静的）でも読める**。

## 適用範囲

- `os_awareness.py` は **ASP3（AArch64）** のカーネルシンボル（`_kernel_*`）と `kernel_cfg.h`
  にのみ依存し，ボード非依存。AArch64 の ASP3 ターゲット一般で使える。
- arm64 の優先度ビットマップ（`PRIMAP_BIT(pri)=0x8000>>pri`, MSB 詰め）に依存する。
- GIC 状態の `target/chip/core_os_awareness.py` は STM32MP257F-DK / GICv2 固有。
