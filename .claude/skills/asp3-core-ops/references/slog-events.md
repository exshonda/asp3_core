# 構造化ログ（slog）イベント一覧（asp3_core 固有）

asp3_core のトレースログ機能（`arch/tracelog/trace_slog.c`、asp3_core新規）は、カーネル
イベントを行指向の構造化ログで低レベル文字出力へ逐次出す。トレースログ有効ビルド
（`ASP3_ENABLE_TRACE=ON`。linuxプリセットは既定ON）で、ホストsimは実行時 `--slog` で有効化。

## フォーマット

```
T=<tick_us>,EV=<event>[,<key>=<val>]*
```

## イベント一覧（trace_slog.c 冒頭コメントが一次情報）

| イベント | 意味 | 主なキー |
|---|---|---|
| `TSK_STAT` | タスク状態変化 | `ID`, `STAT`（RUNNING/RUNNABLE/WAITING/SUSPENDED/DORMANT 等） |
| `DSP_ENTER` | ディスパッチ元タスク（切り替え元） | `ID` |
| `TSK_RUN` | ディスパッチ先タスク＝実行開始 | `ID` |
| `INH_ENTER` / `INH_LEAVE` | 割込みハンドラの出入り | `INHNO` |
| `ISR_ENTER` / `ISR_LEAVE` | 割込みサービスルーチンの出入り | `INTNO` |
| `CYC_ENTER` / `CYC_LEAVE` | 周期ハンドラの出入り | `ID` |
| `ALM_ENTER` / `ALM_LEAVE` | アラームハンドラの出入り | `ID` |
| `EXC_ENTER` / `EXC_LEAVE` | CPU例外ハンドラの出入り | `EXCNO` |
| `SVC_ENTER` | サービスコール入口 | `API`, `A1`〜`A5`（引数） |
| `SVC_LEAVE` | サービスコール出口 | `API`, `RET`（戻り値）, `V1`/`V2`（出力値） |
| `ERR` | サービスコールのエラー復帰 | `API`, `CODE`。`SVC_LEAVE` の `RET` がエラー範囲（-99〜-1）のとき追加で出る |

> システムログ（`syslog` の COMMENT/ASSERT）はコンソールへ通常出力され、slogには出ない。

## 読み方の例

```
T=4881,EV=TSK_STAT,ID=2,STAT=RUNNABLE   # タスク2が実行可能に
T=4890,EV=SVC_ENTER,API=act_tsk,A1=3    # act_tsk(3)
T=4899,EV=SVC_LEAVE,API=act_tsk,RET=0   # E_OK
T=5200,EV=ERR,API=wai_sem,CODE=-18      # wai_sem が E_ID(-18) で復帰
```

切り分けの着眼（最初の `TSK_RUN` が無い＝未起動、`SVC_ENTER` だけで `SVC_LEAVE` 無し＝
待ち入り or ハング/フォルト 等）は skill `toppers-kernel-debug` の `observing-execution.md`。

## 解析スクリプト

```bash
# 整形（人間可読）
timeout 5 ./build/linux/asp --slog 2>&1 | python3 scripts/parse_slog.py
# JSON Lines 化
timeout 5 ./build/linux/asp --slog 2>&1 | python3 scripts/parse_slog.py --json > actual.jsonl
# 期待イベント列との照合（PASS/FAIL）
python3 scripts/check_events.py test/porting/expected/<name>.json actual.jsonl
```

QEMUターゲットは `./build/linux/asp` の代わりに qemu 起動コマンドの標準出力を slog として
パイプする。期待ファイルは `test/porting/expected/` に置く。

## トレースが出ないとき

- トレースログ機能が無効ビルド（`ASP3_ENABLE_TRACE=OFF`）→ 有効化して再ビルド。
- 実機で文字出力先が未設定 → `TRACE_SLOG_PUTC`／`target_fput_log` のターゲット実装を確認。
