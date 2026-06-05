# ビルド手順（Makefile版／CMake版）

TOPPERS/ASP3 Core のビルド方法。**Makefile版（configure.py）** と **CMake版** の
2系統があり、どちらも cfg 3パス（静的API抽出 → カーネル構成生成 → 構成チェック）を
自動実行する。コンフィギュレータは Python版（`cfg/cfg.py`）がデフォルト。

> 共通の既定（asp3_core の変更点）
> - **非TECS構成**がデフォルト（TECSに戻す場合は configure 引数に `OMIT_TECS=` と空値を指定）
> - syssvc共通オブジェクト（syslog/banner/serial/serial_cfg/logtask）と
>   ターゲットSIOドライバは自動でリンク対象に入る
> - `OMIT_DEFAULT_SYSSVC` 指定で上記の自動付与を抑止（素のカーネルビルド用）

---

## 1. Makefile版（configure.py）

### 基本の流れ（例：QEMU mps2-an521）

```bash
# 1. ビルドフォルダを作成（場所・名前は任意。obj/ 配下は .gitignore 済み）
mkdir -p obj/obj_mps2
cd obj/obj_mps2

# 2. configure.py で Makefile を生成（-T でターゲット指定）
python3 ../../configure.py -T mps2_an521_gcc

# 3. ビルド（cfg 3パス → libkernel.a → asp → 構成チェックまで自動実行）
make

# 4. 実行（mps2 は make run で QEMU 起動。終了は Ctrl-A X）
make run
```

### 他ターゲットの例

```bash
# POSIX（ホスト実行）
mkdir -p obj/obj_linux && cd obj/obj_linux
python3 ../../configure.py -T linux_gcc
make
./asp
```

`-T` を省略すると、利用可能なターゲット一覧が表示される。

### よく使うオプション

| オプション | 意味 |
|---|---|
| `-A <名前>` | アプリ名（既定：sample1） |
| `-a <dir>` | アプリのディレクトリ（既定：`$(SRCDIR)/sample`。テストなら `-a '$(SRCDIR)/test'`） |
| `-S "x.o y.o"` | システムサービスの追加オブジェクト（共通分とターゲットSIOは自動付与されるので通常不要） |
| `-O "-DXXX"` | マクロ定義の追加 |
| `-L <dir>` | ビルド済み libkernel.a のディレクトリ（カーネル再ビルドを省略） |
| `OMIT_DEFAULT_SYSSVC` | syssvc自動付与の抑止 |
| `-g "ruby $(SRCDIR)/cfg/cfg.rb"` | Ruby版cfgに切替（make に `TARGET_KERNEL_TRB='$(TARGETDIR)/target_kernel.trb'` 等の `.trb` 指定も必要） |

### 部分ビルド（デバッグ時）

```bash
make cfg1_out.c      # パス1まで（静的API抽出）
make kernel_cfg.c    # パス2まで（カーネル構成生成）
make libkernel.a     # カーネルライブラリまで
make clean           # クリーン
```

---

## 2. CMake版

### 基本の流れ（例：QEMU mps2-an521）

```bash
# 1. configure（プリセットがビルドフォルダ・toolchain・ターゲットを決める）
cmake --preset m33-qemu -B build/m33-qemu

# 2. ビルド（cfg 3パス → libasp3.a → asp.elf → 構成チェックまで自動実行）
cmake --build build/m33-qemu

# 3. 実行（QEMU 起動。終了は Ctrl-A X）
cmake --build build/m33-qemu --target run
```

プリセット一覧は `cmake --list-presets` で表示される。

### make のように簡単に使う（ninja）

ジェネレータが Ninja のため、ビルドフォルダ内では ninja が make の代わりになる。

```bash
cd build/m33-qemu
ninja        # = make
ninja run    # = make run（ビルドしてQEMU実行）

# cd したくない場合
ninja -C build/m33-qemu run
```

### ビルド毎のフォルダを作る（Makefile版の obj/obj_xxx に相当）

`-B` で指定するフォルダがビルド毎のフォルダで、いくつでも並存できる（cd 不要）。

```bash
# ターゲット別
cmake --preset m33-qemu -B build/m33-qemu
cmake --preset posix    -B build/posix          # （posix対応後）

# 同じターゲットで構成違い（例：テストプログラム）
cmake --preset m33-qemu -B build/mps2-sem1 \
  -DASP3_APPLNAME=test_sem1 -DASP3_APPLDIR=$PWD/test
cmake --build build/mps2-sem1
```

### プリセットを使わない場合（完全手動）

```bash
cmake -B build/mytest -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DASP3_TARGET=mps2_an521_gcc
cmake --build build/mytest
```

### Makefile版との対応表

| Makefile版 | CMake版 |
|---|---|
| `mkdir obj/obj_xxx && cd obj/obj_xxx` | `-B build/xxx`（自動作成・cd不要） |
| `python3 ../../configure.py -T <target>` | `cmake --preset <preset>`（または `-DASP3_TARGET=<target>`） |
| `make` | `cmake --build build/xxx`／`ninja -C build/xxx` |
| `make run` | `cmake --build build/xxx --target run`／`ninja -C build/xxx run` |
| `make clean` | `--target clean`／`ninja -C build/xxx clean` |
| アプリ指定 `-A perf1 -a '$(SRCDIR)/test'` | `-DASP3_APPLNAME=perf1 -DASP3_APPLDIR=$PWD/test` |
| 生成物はビルドフォルダ直下 | `build/xxx/generated/` 配下 |

### 補足

- 構成は `CMakeCache.txt` に記憶されるため、2回目以降は `cmake --build build/xxx` だけでよい
- インクリメンタルビルドは賢く動く（.cfg を変更しても生成物の内容が同じなら再リンクされない）
- 完全クリーンは `rm -rf build/xxx` でフォルダごと削除して configure からやり直すのが確実
- `compile_commands.json` がビルドフォルダに自動生成される
  （`ln -sf build/m33-qemu/compile_commands.json compile_commands.json` でルートにリンク：AGENTS.md §5）

---

## 3. QEMUでの実行

### run ターゲット（推奨）

ターゲットの `target.cmake`（`ASP3_RUN_COMMAND`）／`Makefile.target` が定義する。
未ビルドなら自動でビルドしてから起動し、UARTが端末に接続される（キー入力可）。
**終了は Ctrl-A X**。

```bash
ninja -C build/m33-qemu run        # CMake版
make run                            # Makefile版（ビルドフォルダ内）
```

テストプログラムは `ext_ker` 時にセミホスティングで QEMU が自動終了する
（`TOPPERS_USE_QEMU` 定義済みのため）。

### QEMUを直接起動

```bash
qemu-system-arm -machine mps2-an521 -nographic \
  -semihosting-config enable=on,target=native -kernel build/m33-qemu/asp.elf
```

### スクリプトから使う（タイムアウト付き・入力送り込み）

```bash
# 10秒で打ち切り
timeout 10 qemu-system-arm -machine mps2-an521 -nographic \
  -semihosting-config enable=on,target=native -kernel build/m33-qemu/asp.elf

# 3秒後に 'r' を送ってタスク切替を確認するような自動検証
(sleep 3; printf 'r'; sleep 3) | timeout 10 qemu-system-arm -machine mps2-an521 \
  -nographic -semihosting-config enable=on,target=native -kernel build/m33-qemu/asp.elf
```

### ターゲット別のQEMUマシン

| ターゲット | QEMUコマンド |
|---|---|
| mps2_an521_gcc | `qemu-system-arm -machine mps2-an521 -nographic -semihosting-config enable=on,target=native -kernel <elf>` |
| zybo_z7_gcc | `qemu-system-arm -M xilinx-zynq-a9 -semihosting -nographic -serial /dev/null -serial mon:stdio -kernel <elf>`（UART1が2本目のserial） |

---

## 4. ビルド方式の対応状況

| ターゲット | Makefile版 | CMake版 |
|---|---|---|
| mps2_an521_gcc | ○（make run対応） | ○（preset: m33-qemu，runターゲット対応） |
| linux_gcc | ○ | ○（preset: posix，runターゲット対応） |
| zybo_z7_gcc | ○ | ○（preset: zybo-qemu，runターゲット対応） |
| raspberrypi_pico2_gcc | ○ | 未対応（pico-sdk 必須．preset: pico2-m33 は予約） |
| stm32mp257f_dk_arm64_gcc | ○（リンクは aarch64-none-elf 環境） | ○（preset: stm32mp257-a35．リンク・実機は aarch64-none-elf 環境） |
| ct11mpcore_gcc／gr_peach_gcc／dummy_gcc | ○ | 未定 |

`cmake --build --preset run-m33-qemu` のように buildPresets（run-posix / run-m33-qemu /
run-zybo-qemu）でルートから1コマンド実行もできる。

CMake対応の進捗は `docs/dev/cmake.md` を参照。
