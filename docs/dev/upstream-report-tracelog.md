# 上流報告メモ：トレースログ有効時のコンパイルエラー2件

> TOPPERSプロジェクトへのバグ報告用メモ（CLIターゲット作業で発見．経緯は
> `cli-target.md`）。下記「報告文面」をユーザーズML等にそのまま使用できる。
>
> - 状態：**報告準備中**（報告後に投稿先・日付・上流の対応状況をここに追記する）
> - 本リポジトリでの対処：両件とも修正済み（`kernel/sys_manage.c` は
>   禁則①の例外としてユーザー承認のうえ修正．`DIVERGENCE_MAP.md` 参照）。
>   上流で修正されたバージョンを取り込んだ時点で本リポジトリ側の差分は消滅する。

---

## 報告文面

### 件名

トレースログ有効時（TOPPERS_ENABLE_TRACE）にコンパイルエラーとなる箇所について（ASP3）

### 本文

ASP3カーネルおよびPOSIXシミュレーション環境ターゲット依存部において、
トレースログ機能を有効（`TOPPERS_ENABLE_TRACE` を定義）にし、該当の
`LOG_*` フックを定義するトレースログ実装を用いた場合に、コンパイル
エラーとなる箇所が2件ありましたので報告します。

いずれも、フックに**その関数のスコープに存在しない変数**が渡されている
ものです。`LOG_*` フックは未定義時に空マクロとなり引数が評価されない
ため、通常ビルドでは顕在化しません。また、ASP3同梱のTECS版トレースログ
（`arch/tracelog/trace_log.h`）は該当フック（`LOG_GET_LOD_ENTER`・
`LOG_INH_ENTER`）を定義していないため、同梱構成でも顕在化しません。
FMP3の `arch/tracelog/`（全サービスコールのフックを定義）をASP3へ移植
して使用した際に発見しました。

#### 1件目：kernel/sys_manage.c の get_lod()

- 対象：ASP3 3.7.2（`$Id: sys_manage.c 1138 2019-01-04 10:45:23Z ertl-hiro $`）
- 箇所：`get_lod()` 冒頭（3.7.2では234行目付近）

`get_lod(PRI tskpri, uint_t *p_load)` の引数は `tskpri` と `p_load` ですが、
入口フックに存在しない変数 `p_tskid` が渡されています
（`get_nth()` からの流用に伴うものと推測します）。

```
エラー例（gcc）:
kernel/sys_manage.c: In function 'get_lod':
kernel/sys_manage.c:234: error: 'p_tskid' undeclared (first use in this function)
```

修正案：

```diff
--- a/kernel/sys_manage.c
+++ b/kernel/sys_manage.c
@@ get_lod
-	LOG_GET_LOD_ENTER(p_tskid, p_load);
+	LOG_GET_LOD_ENTER(tskpri, p_load);
```

なお出口側の `LOG_GET_LOD_LEAVE(ercd, p_load)` は正しい引数です。

#### 2件目：POSIXシミュレーション環境の int_entry()

- 対象：asp3_arch_posix_gcc パッケージ
  （`$Id: posix_kernel_impl.c 1793 2023-03-05 10:30:13Z ertl-hiro $`）
- 箇所：`arch/posix_gcc/posix_kernel_impl.c` の `int_entry()`（145行目付近）

割込みスレッドのメイン関数 `int_entry()` で、割込みハンドラ呼出しの
前後のフックに存在しない変数 `inhno` が渡されています。

```
エラー例（gcc）:
arch/posix_gcc/posix_kernel_impl.c: In function 'int_entry':
arch/posix_gcc/posix_kernel_impl.c:145: error: 'inhno' undeclared (first use in this function)
```

修正案（割込みハンドラ初期化ブロックから割込みハンドラ番号を取得）：

```diff
--- a/arch/posix_gcc/posix_kernel_impl.c
+++ b/arch/posix_gcc/posix_kernel_impl.c
@@ int_entry
 	/* 割込みハンドラを呼び出す */
-	LOG_INH_ENTER(inhno);
+	LOG_INH_ENTER(p_my_intrcb->p_inhinib->inhno);
 	(*(p_my_intrcb->p_inhinib->inthdr))();
-	LOG_INH_LEAVE(inhno);
+	LOG_INH_LEAVE(p_my_intrcb->p_inhinib->inhno);
```

#### 再現条件（参考）

1. `TOPPERS_ENABLE_TRACE` を定義してビルドする
2. `arch/tracelog/trace_log.h` として `LOG_GET_LOD_ENTER`・`LOG_INH_ENTER`
   を定義する実装（例：FMP3 3.3の `arch/tracelog/` をASP3向けに変換した
   もの）を使用する
3. 1件目は `kernel/sys_manage.c`（get_lod．サブ優先度機能のため
   `ALLFUNC` 等で全関数をコンパイルする構成）、2件目はPOSIX環境の
   `arch/posix_gcc/posix_kernel_impl.c` のコンパイルで発生

---

## 補足（本リポジトリ内部向け）

- 発見の経緯・本リポジトリでの検証内容は `docs/dev/cli-target.md`
  「トレース有効時に顕在化した上流の潜在バグ」を参照。
- `kernel/sys_manage.c` の修正は PRISTINE 領域への変更のため、上流マージ時は
  `DIVERGENCE_MAP.md` の該当行に従い**上流の修正状況を確認**すること
  （上流で修正済みなら上流版をそのまま採用して本差分を解消する）。
