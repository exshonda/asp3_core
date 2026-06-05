# ビルド手順（CMake）

TOPPERS/ASP3 Core のビルド方法。ビルドシステムは **CMake**（Ninjaジェネレータ）で、
cfg 3パス（静的API抽出 → カーネル構成生成 → 構成チェック）を自動実行する。
コンフィギュレータは Python版（`cfg/cfg.py`）。

> 共通の既定（asp3_core の変更点）
> - **非TECS構成**（TECSは廃止済み）
> - syssvc共通ソース（syslog/banner/serial/serial_cfg/logtask）と
>   ターゲットSIOドライバは自動でリンク対象に入る
> - `-DASP3_OMIT_DEFAULT_SYSSVC=ON` で上記の組込みを抑止（素のカーネルビルド用）

---

## 1. 基本の流れ（例：QEMU mps2-an521）

```bash
# 1. configure（プリセットがビルドフォルダ・toolchain・ターゲットを決める）
cmake --preset m33-qemu -B build/m33-qemu

# 2. ビルド（cfg 3パス → libasp3.a → asp.elf → 構成チェックまで自動実行）
cmake --build build/m33-qemu

# 3. 実行（QEMU 起動。終了は Ctrl-A X）
cmake --build build/m33-qemu --target run
```

プリセット一覧は `cmake --list-presets` で表示される。

> プリセット定義は**ターゲット依存部のフォルダ**（`target/<name>/presets.json`）に
> あり、ルートの `CMakePresets.json` はそれらを include する一覧のみを持つ
> （共通設定は `cmake/presets-base.json`）。ターゲット追加時の手順は
> `docs/porting/PORTING_GUIDE.md` Step 9-2 を参照。

### make のように簡単に使う（ninja）

ジェネレータが Ninja のため、ビルドフォルダ内では ninja が make の代わりになる。

```bash
cd build/m33-qemu
ninja        # ビルド
ninja run    # ビルドして実行

# cd したくない場合
ninja -C build/m33-qemu run
```

buildPresets（`run-posix`／`run-m33-qemu`／`run-zybo-qemu`／`run-a64-qemu`）でルートから
`cmake --build --preset run-m33-qemu` の1コマンド実行もできる。

---

## 2. ビルド毎のフォルダ

`-B` で指定するフォルダがビルド毎のフォルダで、いくつでも並存できる（cd 不要）。

```bash
# ターゲット別
cmake --preset m33-qemu -B build/m33-qemu
cmake --preset posix    -B build/posix

# 同じターゲットで構成違い（例：テストプログラム）
cmake --preset m33-qemu -B build/mps2-sem1 \
  -DASP3_APPLNAME=test_sem1 -DASP3_APPLDIR=$PWD/test \
  "-DASP3_EXTRA_APP_C_FILES=$PWD/syssvc/test_svc.c"
cmake --build build/mps2-sem1
```

### プリセットを使わない場合（完全手動）

```bash
cmake -B build/mytest -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DASP3_TARGET=mps2_an521_gcc
cmake --build build/mytest
```

### 主なCMake変数

| 変数 | 意味 |
|---|---|
| `ASP3_TARGET` | ターゲット名（`target/` 配下のディレクトリ名） |
| `ASP3_APPLNAME` | アプリ名（既定：sample1） |
| `ASP3_APPLDIR` | アプリのディレクトリ（既定：`sample/`） |
| `ASP3_APPCFGNAME` | `.cfg` ファイル名がアプリ名と異なる場合に指定 |
| `ASP3_EXTRA_APP_C_FILES` | 追加でビルドするソース（`;`区切り） |
| `ASP3_EXTRA_COMPILE_DEFS` | 追加のマクロ定義（`;`区切り、`-D`なし） |
| `ASP3_OMIT_DEFAULT_SYSSVC` | ONでsyssvcを組み込まない |

### 補足

- 構成は `CMakeCache.txt` に記憶されるため、2回目以降は `cmake --build build/xxx` だけでよい
- 生成物（`kernel_cfg.c/h`・`offset.h` 等）は `build/xxx/generated/` 配下
- 完全クリーンは `rm -rf build/xxx` でフォルダごと削除して configure からやり直すのが確実
- `compile_commands.json` がビルドフォルダに自動生成される
  （`ln -sf build/m33-qemu/compile_commands.json compile_commands.json` でルートにリンク：AGENTS.md §5）

---

## 3. 実行（QEMU／実機）

### run ターゲット

ターゲットの `target.cmake`（`ASP3_RUN_COMMAND`）が定義する。
未ビルドなら自動でビルドしてから実行する。

```bash
ninja -C build/m33-qemu run     # QEMU（終了は Ctrl-A X）
ninja -C build/posix run        # ホスト実行
ninja -C build/pico2-m33 run    # 実機書込み（OpenOCD program）
```

テストプログラムは `ext_ker` 時にセミホスティングで QEMU が自動終了する。

### 実機デバッグ系ターゲット（pico2／stm32mp257-a35）

`target/<name>/run.cmake` が定義する（`ninja -C build/<dir> <target>`）：

| ターゲット | pico2 | stm32mp257-a35 | 内容 |
|---|---|---|---|
| `run`／`swd-run` | run | swd-run | OpenOCDで書込み・実行 |
| `openocd`・`gdb`・`swd-debug` | ○ | ○ | デバッグセッション |
| `osdebug` | − | ○ | OS-awareness付き（gdb-multiarch） |
| `console` | ○ | ○ | UARTコンソール（picocom等自動選択） |

### QEMUを直接起動

```bash
qemu-system-arm -machine mps2-an521 -nographic \
  -semihosting-config enable=on,target=native -kernel build/m33-qemu/asp.elf

# zybo（UART1が2本目の-serial）
qemu-system-arm -M xilinx-zynq-a9 -semihosting -nographic \
  -serial /dev/null -serial mon:stdio -kernel build/zybo-qemu/asp.elf
```

スクリプトからは `timeout` と入力の送り込みが使える：

```bash
(sleep 3; printf 'r'; sleep 3) | timeout 10 qemu-system-arm -machine mps2-an521 \
  -nographic -semihosting-config enable=on,target=native -kernel build/m33-qemu/asp.elf
```

---

## 4. テストランナ

```bash
# 機能テスト（QEMU mps2の例）：作業ディレクトリを作って実行
mkdir TEST-MPS2 && cd TEST-MPS2
echo '--preset m33-qemu' > TARGET_OPTIONS
echo 'timeout 20 qemu-system-arm -machine mps2-an521 -nographic \
  -semihosting-config enable=on,target=native -kernel asp.elf' > TARGET_RUN
python3 ../test/testexec.py sem1 flg1      # build＋exec

# cfgテスト（dummyターゲット）
mkdir TESTCFG && cd TESTCFG
echo '-G Ninja -DASP3_TARGET=dummy_gcc -DASP3_OMIT_DEFAULT_SYSSVC=ON \
  -DASP3_EXTRA_COMPILE_DEFS=TOPPERS_OMIT_SYSLOG' > TARGET_OPTIONS
python3 ../test_cfg/testcfg.py all
```

---

## 5. ターゲット対応状況

| ターゲット | プリセット | 実行 |
|---|---|---|
| linux_gcc | posix | ホスト実行（run） |
| mps2_an521_gcc | m33-qemu | QEMU（run） |
| zybo_z7_gcc | zybo-qemu | QEMU（run） |
| zcu102_arm64_gcc | a64-qemu | QEMU（run．aarch64-none-elf が無い環境は `-DA35_TOOLCHAIN_PREFIX=aarch64-linux-gnu-` を付与．実機ZCU102向けは `-DZCU102_QEMU=OFF`） |
| raspberrypi_pico2_gcc | pico2-m33 | 実機（run=OpenOCD書込み．gdb/console等） |
| stm32mp257f_dk_arm64_gcc | stm32mp257-a35 | 実機（swd-run/osdebug/console等．リンクは aarch64-none-elf 環境） |
| dummy_gcc | （プリセット無し．`-DASP3_TARGET=dummy_gcc`） | cfgテスト用ホストビルド |
| pico2-riscv | （予約） | arch/riscv_gcc 未実装 |

CMake対応の経緯は `docs/dev/cmake.md` を参照。
