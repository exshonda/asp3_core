# TOPPERS/ASP3カーネル 機能拡張・チューニングガイド

**原本**: `doc/extension.txt`（対応バージョン: Release 3.7.2）  
**Markdown化**: 2026-06-07

> **asp3_core注（本書の位置付け・重要）**: 本書が説明する改造・拡張は，いずれも **`kernel/` 配下（PRISTINE・編集禁止領域）の変更を伴います**。asp3_coreで実施する場合は，**禁則①**（`AGENTS.md` §2）に従い，**作業を止めてユーザーに確認**し，`DIVERGENCE_MAP.md` に記録してから行ってください。
>
> - 拡張パッケージ（`extension/` に9種残置）の「展開」（cp -r）も同様にPRISTINEファイルを上書きします
> - 「Makefile.kernel」の変更箇所は，asp3_coreではCMake（`asp3_core.cmake` のカーネルソースリスト）が対応します
> - 末尾の `.trb`（Ruby）コード例は，asp3_coreでは `.py`（`kernel/exception.py`・`target/<name>/target_kernel.py`）への変更になります

## 目次

- エラーチェックの省略
- ミューテックス機能を完全に外す
- サービスコールを非タスクコンテキストから呼び出せるようにする
- サービスコールをCPUロック状態から呼び出せるようにする
- ミューテックスのロック解除の順序の制限を外す
- 拡張パッケージの使い方
- 特殊目的のレジスタの扱い
- CPU例外ハンドラの直接呼出し

---

## エラーチェックの省略

サービスコールのオーバヘッドを削減するために，静的なエラーのチェックを省略する方法がある。ASP3カーネルにおいては，静的なエラーのチェックはすべてCHECKマクロを用いて行っているため，kernel/check.h中のCHECKマクロを変更することで，静的なエラーのチェックを省略することができる。

例えば，オブジェクトIDのチェックを省略したい場合には，CHECK_IDマクロの定義を，次のように変更すればよい。

```c
#define CHECK_ID(exp)			((void)(exp))
```

このマクロの定義を空にする方法もあるが，パラメータに副作用のある式が書かれている可能性を考えると（副作用のある式は書くべきではないが，書かれているコードが入ってくる可能性が全くないとは言えない），上の定義の方が安全である。副作用のない式であれば，最適化によって削除することができるため，実行時効率には影響がないと期待できる。ただし，最適化によって削除されない場合には，副作用のある式が書かれていないことを確認した上で，マクロの定義を空にしてもよい。

もう少し小さい粒度でエラーチェックを省略したい場合には，kernel/check.h中のVALIDマクロを変更する方法がある。例えば，タスクIDの中でチェックを省略したい場合には，VALID_TSKIDマクロの定義を，次のように変更すればよい。

```c
#define VALID_TSKID(tskid)		(true)
```

## ミューテックス機能を完全に外す

ミューテックス以外の同期・通信オブジェクトは，使用しない場合にはカーネルのメモリ使用量を増やすことはないが，ミューテックスはタスク管理機能と絡んでいるため，使用しない場合でも一部のコードがカーネルに含まれてしまう。ミューテックス機能を使用しない場合には，以下の方法で，ミューテックス機能を完全に外すことができる。

### (1) ミューテックス機能呼出しコードの削除

task.c/task_terminate中の以下のコードを削除する。

```c
	if (p_tcb->p_lastmtx != NULL) {
		(*mtxhook_release_all)(p_tcb);
	}
```

task_manage.c/chg_pri中の以下のコードを削除する。

```c
	else if ((p_tcb->boosted || TSTAT_WAIT_MTX(p_tcb->tstat))
						&& !((*mtxhook_check_ceilpri)(p_tcb, newbpri))) {
		ercd = E_ILUSE;							/*［NGKI1201］*/
	}
```

Makefile.kernelを以下のように変更する（mutex.cをコンパイル対象から外す）。

変更前

```makefile
KERNEL_FCSRCS = startup.c task.c taskhook.c wait.c time_event.c \
				task_manage.c task_refer.c task_sync.c task_term.c \
				semaphore.c eventflag.c dataqueue.c pridataq.c mutex.c \
				mempfix.c time_manage.c cyclic.c alarm.c \
				sys_manage.c interrupt.c exception.c
```

変更後

```makefile
KERNEL_FCSRCS = startup.c task.c taskhook.c wait.c time_event.c \
				task_manage.c task_refer.c task_sync.c task_term.c \
				semaphore.c eventflag.c dataqueue.c pridataq.c \
				mempfix.c time_manage.c cyclic.c alarm.c \
				sys_manage.c interrupt.c exception.c
```

> **asp3_core注**: Makefile.kernelは削除済み。カーネルソースリストはCMake側（`asp3_core.cmake`）で管理されています。

### (2) change_priorityの第3パラメータを削除する

task.hを以下のように変更する。

変更前

```c
extern void	change_priority(TCB *p_tcb, uint_t newpri, bool_t mtxmode);
```

変更後

```c
extern void	change_priority(TCB *p_tcb, uint_t newpri);
```

task.cを以下のように変更する。

変更前

```c
void
change_priority(TCB *p_tcb, uint_t newpri, bool_t mtxmode)
{
		...
		if (mtxmode) {
			queue_insert_next(&(ready_queue[newpri]), &(p_tcb->task_queue));
		}
		else {
			queue_insert_prev(&(ready_queue[newpri]), &(p_tcb->task_queue));
		}
		...
}
```

変更後

```c
void
change_priority(TCB *p_tcb, uint_t newpri)
{
		...
		queue_insert_prev(&(ready_queue[newpri]), &(p_tcb->task_queue));
		...
}
```

task_manage.c/chg_priを以下のように変更する。

変更前

```c
			change_priority(p_tcb, newbpri, false);		/*［NGKI1193］*/
```

変更後

```c
			change_priority(p_tcb, newbpri);		/*［NGKI1193］*/
```

### (3) TCB中のboostedフィールドとp_lastmtxフィールドの削除

この変更は，RAMサイズは小さくなるが，TCBのサイズが変わるため，コードサイズは大きくなる可能性がある。

task.h中の以下のコードを削除する。

```c
	BIT_FIELD_BOOL	boosted : 1;	/* 優先度上昇状態 */
```

```c
	MTXCB			*p_lastmtx;		/* 最後にロックしたミューテックス */
```

task.c/initialize_task中の以下のコードを削除する。

```c
		p_tcb->p_lastmtx = NULL;
```

task.c/make_dormant中の以下のコードを削除する。

```c
	p_tcb->boosted = false;
```

task_manage.c/chg_priを以下のように変更する（if文を外す）。

変更前

```c
		if (!(p_tcb->boosted)) {
			change_priority(p_tcb, newbpri, false);		/*［NGKI1193］*/
			if (p_runtsk != p_schedtsk) {
				dispatch();
			}									/*［NGKI1197］*/
		}
```

変更後

```c
		change_priority(p_tcb, newbpri, false);		/*［NGKI1193］*/
		if (p_runtsk != p_schedtsk) {
			dispatch();
		}										/*［NGKI1197］*/
```

## サービスコールを非タスクコンテキストから呼び出せるようにする

タスクコンテキスト専用のサービスコールの中で，以下に挙げるものは，単純な変更を行うことで，非タスクコンテキストから呼び出せるようにすることができる。

対象サービスコール：

- can_act，get_tst，chg_pri，get_pri，ref_tsk
- can_wup，rsm_tsk
- pol_sem，ini_sem，ref_sem
- clr_flg，pol_flg，ini_flg，ref_flg
- prcv_dtq，ini_dtq，ref_dtq
- prcv_pdq，ini_pdq，ref_pdq
- ini_mtx，ref_mtx
- pget_mpf，rel_mpf，ini_mpf，ref_mpf
- set_tim，get_tim
- sta_cyc，stp_cyc，ref_cyc
- ref_alm
- get_lod，get_nth

非タスクコンテキストから呼び出せるようにするためのサービスコールのコードの変更方法は以下の通り。

### (1) エラーチェックを外す

サービスコールの入口のエラーチェック処理を，以下のように変更する。

変更前

```c
	CHECK_TSKCTX_UNL();
```

変更後

```c
	CHECK_UNL();
```

### (2) ディスパッチャ呼出し処理の変更

サービスコール内でディスパッチャ（dispatch）を呼び出す場合には，ディスパッチャの呼出し処理を以下のように変更する。

変更前

```c
	dispatch();
```

変更後

```c
	if (!sense_context()) {
		dispatch();
	}
	else {
		request_dispatch_retint();
	}
```

### (3) 自タスク指定の処理の変更

タスクIDをパラメータとするサービスコールで，自タスク指定（TSK_SELF）ができる場合には，自タスク指定の処理を以下のように変更する。

変更前

```c
	if (tskid == TSK_SELF) {
```

変更後

```c
	if (tskid == TSK_SELF && !sense_context()) {
```

### (4) 自タスクのベース優先度の指定の処理の変更

タスクの優先度をパラメータとするサービスコールで，自タスクのベース優先度の指定（TPRI_SELF）ができる場合には，自タスクのベース優先度の指定の処理を以下のように変更する。

変更前

```c
	if (tskpri == TPRI_SELF) {
```

変更後

```c
	if (tskpri == TPRI_SELF && !sense_context()) {
```

## サービスコールをCPUロック状態から呼び出せるようにする

CPUロック状態からは呼び出すことができないサービスコールの中で，以下に挙げるものは，単純な変更を行うことで，CPUロック状態から呼び出せるようにすることができる。ただし，この変更を行うことで，割込み禁止時間が長くなる可能性があることに注意が必要である。

対象サービスコール：

- act_tsk，can_act，get_tst，chg_pri，get_pri，get_inf，ref_tsk
- wup_tsk，can_wup，rel_wai，rsm_tsk
- ras_ter，dis_ter，ter_tsk
- sig_sem，pol_sem，ini_sem，ref_sem
- set_flg，clr_flg，pol_flg，ini_flg，ref_flg
- psnd_dtq，fsnd_dtq，prcv_dtq，ini_dtq，ref_dtq
- psnd_pdq，prcv_pdq，ini_pdq，ref_pdq
- ploc_mtx，unl_mtx，ini_mtx，ref_mtx
- pget_mpf，rel_mpf，ini_mpf，ref_mpf
- set_tim，get_tim，adj_tim
- sta_cyc，stp_cyc，ref_cyc
- sta_alm，stp_alm，ref_alm
- rot_rdq，get_tid，get_lod，get_nth，dis_dsp
- get_ipm

### (1) エラーチェックを外す

タスクコンテキスト専用のサービスコールの場合，サービスコールの入口のエラーチェック処理を，以下のように変更する。

変更前

```c
	CHECK_TSKCTX_UNL();
```

変更後

```c
	CHECK_TSKCTX();
```

非タスクコンテキストからも呼び出せるサービスコールの場合，サービスコールの入口のエラーチェック処理（`CHECK_UNL();`）を削除する。

### (2) 排他制御処理の変更

サービスコール内で排他制御を行なっている場合には，以下の変更を行う。

サービスコール処理関数の最初に，以下の変数定義を追加する。

```c
	bool_t	locked;
```

排他制御区間に入るための処理を，以下のように変更する。

変更前

```c
	lock_cpu();
```

変更後

```c
	locked = sense_lock();
	if (!locked) {
		lock_cpu();
	}
```

排他制御区間から出るための処理を，以下のように変更する。

変更前

```c
	unlock_cpu();
```

変更後

```c
	if (!locked) {
		unlock_cpu();
	}
```

### (3) ディスパッチャ呼出し処理の変更

サービスコール内でディスパッチャ（dispatch）を呼び出す場合には，ディスパッチャの呼出し処理を以下のように変更する。

変更前

```c
	dispatch();
```

変更後

```c
	if (!locked) {
		dispatch();
	}
```

### (4) unl_cpuサービスコールの変更

実行すべきタスクを変化させる可能性があるサービスコールを，CPUロック状態から呼び出せるようにした場合には，unl_cpuでCPUロックを解除した後に，必要であればタスク切換えを行う必要がある。具体的には，unl_cpuのサービスコール処理関数を以下のように変更する。

```c
ER
unl_cpu(void)
{
	ER		ercd;

	LOG_UNL_CPU_ENTER();

	if (sense_lock()) {							/*［NGKI2738］*/
		if (!sense_context() && dspflg) {
			if (p_runtsk != p_schedtsk) {
				dispatch();
			}
		}
		unlock_cpu();							/*［NGKI2737］*/
	}
	ercd = E_OK;

	LOG_UNL_CPU_LEAVE(ercd);
	return(ercd);
}
```

なお，サービスコールを，非タスクコンテキストからもCPUロック状態からも呼び出せるようにする場合には，「サービスコールを非タスクコンテキストから呼び出せるようにする」の節の変更をおこなった後に，この節の変更を行えばよい。

<!-- 変換注: 原文の「oCPUロック状態」は「CPUロック状態」の誤記（衍字）と判断し修正した -->


## ミューテックスのロック解除の順序の制限を外す

TOPPERS第3世代カーネル（ITRON系）統合仕様では，ミューテックスのロック解除は，ロックしたのと逆順で行わなければならないものとした。この制限に従わない場合には，unl_mtxがE_OBJエラーとなる。

この制限を外すためには，unl_mtxを次のように変更すればよい。

変更前

```c
	lock_cpu();
	if (p_mtxcb != p_runtsk->p_lastmtx) {
		ercd = E_OBJ;
	}
	else {
		p_runtsk->p_lastmtx = p_mtxcb->p_prevmtx;
		if (MTX_CEILING(p_mtxcb)) {
			mutex_drop_priority(p_runtsk, p_mtxcb->p_mtxinib->ceilpri);
		}
		mutex_release(p_mtxcb);
	後略
```

変更後（1行変更，1行削除）

```c
	lock_cpu();
	if (!remove_mutex(p_runtsk, p_mtxcb)) {
		ercd = E_OBJ;
	}
	else {
		if (MTX_CEILING(p_mtxcb)) {
			mutex_drop_priority(p_runtsk, p_mtxcb->p_mtxinib->ceilpri);
		}
		mutex_release(p_mtxcb);
	後略
```

## 拡張パッケージの使い方

ASP3カーネルでは，いくつかの拡張機能を実装するために，拡張パッケージをサポートしている。拡張パッケージは，extensionディレクトリに置いてある。

拡張パッケージを使用する場合には，UNIXであれば，ASP3カーネルのソースファイルのトップディレクトリで，

```console
% cp -r extension/<拡張パッケージのディレクトリ名>/* .
```

を実行する。この時，元の（拡張前の）ソースファイルは上書きされてしまうため，拡張しないカーネルも使用したい場合には，別のディレクトリにソースファイルを展開して，上のコマンドを実行すること。

複数の拡張パッケージを使うことは考慮していないが，拡張パッケージの組み合わせによっては，手作業により複数の拡張パッケージをマージすることは可能である。

> **asp3_core注**: この展開は **`kernel/`（PRISTINE）の上書き＝禁則①に抵触**します。asp3_coreで拡張パッケージを使用する場合は，必ずユーザー確認のうえ `DIVERGENCE_MAP.md` に記録してください。拡張後は上流マージの手順（`AGENTS.md` §10）も変わります。

### ドリフト調整機能拡張パッケージ

ドリフト調整機能拡張パッケージは，ドリフトの調整機能を追加するための拡張パッケージである。ドリフト調整機能拡張パッケージは，`extension/drift` ディレクトリに置いてある。

ドリフト調整機能拡張パッケージでは，**TOPPERS_SUPPORT_DRIFT**がkernel.h中で定義されているので，これを用いてドリフト調整機能を使用できるかどうかを判別することができる。

追加されるサービスコール：

```c
ER ercd = set_dft(int32_t drift)
```

ドリフト調整機能拡張パッケージは，標準では32ビット演算のみを用いてドリフト調整を行うが，64ビット演算命令を持つプロセッサでは，64ビット演算を用いた方が効率が良い可能性がある。そこで，USE_64BIT_OPSをマクロ定義してコンパイルすることで，64ビット演算を用いてドリフト調整を行う。なお，USE_64BIT_HRTCNTをマクロ定義した場合には，USE_64BIT_OPSもマクロ定義したものと扱われる。

### メッセージバッファ機能拡張パッケージ

メッセージバッファ機能拡張パッケージは，メッセージバッファ機能を追加するための拡張パッケージである。メッセージバッファ機能拡張パッケージは，`extension/messagebuf` ディレクトリに置いてある。

メッセージバッファ機能拡張パッケージでは，**TOPPERS_SUPPORT_MESSAGEBUF**がkernel.h中で定義されているので，これを用いてメッセージバッファ機能を使用できるかどうかを判別することができる。

メッセージバッファ機能拡張パッケージでは，メッセージバッファに対する送信待ち状態のタスクが複数待ち解除される場合がある。この場合，サービスコールの実行時間およびカーネル内での割込み禁止時間が，待ち解除されるタスクの数の数のオーダで長くなるので注意が必要である。メッセージバッファに対する送信待ち状態のタスクが複数待ち解除されるのは，メッセージバッファ管理領域に格納されたメッセージが受信された結果，管理領域に空き領域が生じた場合に加えて，送信待ち行列の先頭につながれているタスクの強制終了や待ち解除の場合にも生じる。

追加されるサービスコール：

```c
ER ercd = snd_mbf(ID mbfid, const void *msg, uint_t msgsz)
ER ercd = psnd_mbf(ID mbfid, const void *msg, uint_t msgsz)
ER ercd = tsnd_mbf(ID mbfid, const void *msg, uint_t msgsz, TMO tmout)
ER_UINT msgsz = rcv_mbf(ID mbfid, void *msg)
ER_UINT msgsz = prcv_mbf(ID mbfid, void *msg)
ER_UINT msgsz = trcv_mbf(ID mbfid, void *msg, TMO tmout)
ER ercd = ini_mbf(ID mbfid)
ER ercd = ref_mbf(ID mbfid, T_RMBF *pk_rmbf)
```

追加される静的API：

```c
CRE_MBF(ID mbfid, { ATR mbfatr, uint_t maxmsz, size_t mbfsz, void *mbfmb })
```

メッセージバッファ機能拡張パッケージでは，memcpyを使用しているため，標準Cライブラリが必要である。標準Cライブラリを用意する代わりに，memcpy関数のみを自分で用意してもよい。

### オーバランハンドラ機能拡張パッケージ

オーバランハンドラ機能拡張パッケージは，オーバランハンドラ機能を追加するための拡張パッケージである。ただし，この拡張パッケージを使うためには，ターゲット依存部が対応している必要がある。オーバランハンドラ機能拡張パッケージは，`extension/ovrhdr` ディレクトリに置いてある。

オーバランハンドラ機能拡張パッケージで，ターゲット依存部が拡張パッケージに対応している場合には，**TOPPERS_SUPPORT_OVRHDR**がkernel.h中で定義されるので，これを用いてオーバランタイマ機能が使用できるかどうかを判別することができる。

追加されるサービスコール：

```c
ER ercd = sta_ovr(ID tskid, PRCTIM ovrtim)
ER ercd = stp_ovr(ID tskid)
ER ercd = ref_ovr(ID tskid, T_ROVR *pk_rovr)
```

追加される静的API：

```c
DEF_OVR({ ATR ovratr, OVRHDR ovrhdr })
```

### タスク優先度拡張パッケージ

タスク優先度拡張パッケージは，タスク優先度を最大256段階に拡張するための拡張パッケージである。この拡張パッケージは，タスク優先度に加えて，データ優先度，メッセージ優先度，割込みサービスルーチン優先度も256段階に拡張する。タスク優先度拡張パッケージは，`extension/pri_level` ディレクトリに置いてある。

タスク優先度拡張パッケージでは，**TOPPERS_SUPPORT_PRI_LEVEL**がkernel.h中で定義されているので，これを用いてタスク優先度の範囲が拡張されているかどうかを判別することができる。

### 制約タスク拡張パッケージ

制約タスク拡張パッケージは，制約タスクの機能を追加するための拡張パッケージである。制約タスク拡張パッケージは，`extension/rstr_task` ディレクトリに置いてある。

制約タスク拡張パッケージでは，**TOPPERS_SUPPORT_RSTR_TASK**がkernel.h中で定義されているので，これを用いて制約タスクの機能が使用できるかどうかを判別することができる。

追加されるタスク属性：`TA_RSTR`

### サブ優先度機能拡張パッケージ

サブ優先度機能拡張パッケージは，サブ優先度機能を追加するための拡張パッケージである。サブ優先度機能拡張パッケージは，`extension/subprio` ディレクトリに置いてある。

サブ優先度機能拡張パッケージでは，**TOPPERS_SUPPORT_SUBPRIO**がkernel.h中で定義されているので，これを用いてサブ優先度機能が使用できるかどうかを判別することができる。

追加されるサービスコール：

```c
ER ercd = chg_spr(ID tskid, uint_t subpri)
```

追加される静的API：

```c
ENA_SPR(PRI tskpri)
```

### 優先度継承拡張パッケージ

優先度継承拡張パッケージは，優先度継承ミューテックス機能を追加するための拡張パッケージである。優先度継承拡張パッケージは，`extension/inherit` ディレクトリに置いてある。

優先度継承拡張パッケージでは，**TOPPERS_SUPPORT_INHERIT**がkernel.h中で定義されているので，これを用いて優先度継承ミューテックス機能が使用できるかどうかを判別することができる。

追加されるミューテックス属性：`TA_INHERIT`

> **asp3_core注**: 設計の詳細は [design_inherit.md](design_inherit.md) を参照。

### 動的生成機能拡張パッケージ

動的生成機能拡張パッケージは，オブジェクトの動的生成機能を追加するための拡張パッケージである。TOPPERS第3世代カーネル（ITRON系）統合仕様書に規定された以下のオブジェクト生成／削除のためのサービスコール，割付け可能なID番号の数を指定する静的API，カーネルメモリプール領域を設定する静的APIを実装している。動的生成機能拡張パッケージは，`extension/dcre` ディレクトリに置いてある。

ただし，カーネルメモリプール領域の動的メモリ管理に関しては，ターゲット非依存部では，メモリプール領域の先頭から順に割り当てを行い，すべてのメモリ領域が解放されるまで解放されたメモリ領域を再利用しないメモリプール管理機能のみを実装している。本格的な動的メモリ管理を行いたい場合には，ターゲット依存部またはユーザ側で，そのための関数を用意する必要がある。用意する関数等については，「TOPPERS/ASP3カーネル ターゲット依存部 ポーティングガイド」の「6.15 動的メモリ管理」の節を参照すること。

動的生成機能拡張パッケージでは，**TOPPERS_SUPPORT_DYNAMIC_CRE**がkernel.h中で定義されているので，これを用いて動的生成機能が使用できるかどうかを判別することができる。

追加されるサービスコール：

```c
ER_ID tskid = acre_tsk(const T_CTSK *pk_ctsk)
ER ercd = del_tsk(ID tskid)
ER_ID semid = acre_sem(const T_CSEM *pk_csem)
ER ercd = del_sem(ID semid)
ER_ID flgid = acre_flg(const T_CFLG *pk_cflg)
ER ercd = del_flg(ID flgid)
ER_ID dtqid = acre_dtq(const T_CDTQ *pk_cdtq)
ER ercd = del_dtq(ID dtqid)
ER_ID pdqid = acre_pdq(const T_CPDQ *pk_cpdq)
ER ercd = del_pdq(ID pdqid)
ER_ID mtxid = acre_mtx(const T_CMTX *pk_cmtx)
ER ercd = del_mtx(ID mtxid)
ER_ID mpfid = acre_mpf(const T_CMPF *pk_cmpf)
ER ercd = del_mpf(ID mpfid)
ER_ID cycid = acre_cyc(const T_CCYC *pk_ccyc)
ER ercd = del_cyc(ID cycid)
ER_ID almid = acre_alm(const T_CALM *pk_calm)
ER ercd = del_alm(ID almid)
ER_ID isrid = acre_isr(const T_CISR *pk_cisr)
ER ercd = del_isr(ID isrid)
```

追加される静的API：

```c
AID_TSK(uint_t notsk)
AID_SEM(uint_t nosem)
AID_FLG(uint_t noflg)
AID_DTQ(uint_t nodtq)
AID_PDQ(uint_t nopdq)
AID_MTX(uint_t nomtx)
AID_MPF(uint_t nompf)
AID_CYC(uint_t nocyc)
AID_ALM(uint_t noalm)
AID_ISR(uint_t noisr)
DEF_MPK({ size_t mpksz, MB_T *mpk })
```

動的生成機能拡張パッケージでは，del_yyyで削除するオブジェクトに対する待ち状態のタスクが複数あると，del_yyyにより，複数のタスクが待ち解除される。この場合，サービスコールの実行時間およびカーネル内での割込み禁止時間が，待ち解除されるタスクの数のオーダで長くなるので注意が必要である。

### モノトニックタイマ機能拡張パッケージ

モノトニックタイマ機能拡張パッケージは，マイクロ秒単位でカウントアップする64ビットのタイマ（これを，モノトニックタイマと呼ぶ）を参照する機能を追加するための拡張パッケージである。モノトニックタイマの更新は，高分解能タイマによって行われ，システム時刻の設定（set_tim），システム時刻の調整（adj_tim），ドリフト量の設定（set_dft）の影響を受けない。モノトニックタイマ機能拡張パッケージは，`extension/fch_mnt` ディレクトリに置いてある。

モノトニックタイマ機能拡張パッケージでは，**TOPPERS_SUPPORT_FCH_MNT**がkernel.h中で定義されているので，これを用いてモノトニックタイマの参照機能が使用できるかどうかを判別することができる。

追加されるサービスコール：

```c
uint64_t mntcnt = fch_mnt()
```

fch_mntは，TOPPERS第3世代カーネル（ITRON系）統合仕様書に規定されていないサービスコールである。その仕様は以下の通り。

#### fch_mnt — モノトニックタイマの参照〔TI〕

**C言語API**

```c
uint64_t mntcnt = fch_mnt()
```

**パラメータ**: なし

**リターンパラメータ**

| 型 | 名称 | 説明 |
|---|---|---|
| `uint64_t` | mntcnt | モノトニックタイマの現在のカウント値 |

**機能**

モノトニックタイマを現在のカウント値を読み出す。

fch_mntは，任意の状態から呼び出すことができる。タスクコンテキストからも非タスクコンテキストからも呼び出すことができるし，CPUロック状態であっても呼び出すことができる。

**使用上の注意**

fch_mntは，任意の状態から呼び出すことができるように，全割込みロック状態を用いて実装されている。そのため，fch_mntを用いると，カーネル管理外の割込みの応答性が低下する。

なお，64ビットの整数型を持たないターゲットでは，モノトニックタイマ機能拡張パッケージを使用することはできない。

## 特殊目的のレジスタの扱い

FPUレジスタやDSPレジスタなどの特殊目的のレジスタ（以下，特殊レジスタ）を持つプロセッサでは，レジスタの扱いについて大きく次の3つの方法が考えられる。

### (1) 特殊レジスタをタスクのコンテキストに含めない

1つのタスクのみが特殊レジスタを使用する場合には，特殊レジスタをタスクのコンテキストに含める必要がなく，カーネルで管理する必要がない。

### (2) 特殊レジスタをタスクのコンテキストに含める

複数のタスクが特殊レジスタを使用する場合には，特殊レジスタをタスクのコンテキストに含める方法が最も単純である。そのためには，タスクディスパッチャと割込みハンドラ/CPU例外ハンドラの出入口で，特殊レジスタを保存/復帰するコードを追加する必要がある。実際の保存/復帰場所は，スクラッチレジスタとそれ以外のレジスタで異なるため，注意が必要である。

### (3) 特殊レジスタをコンテキストに含めるかどうかをタスク毎に指定する

特殊レジスタを使用するタスクと使用しないタスクがある場合で，すべてのタスクのコンテキストに特殊レジスタを含める方法ではオーバヘッドが問題になる場合には，特殊レジスタをコンテキストに含めるかどうかをタスク毎に指定する方法が有力である。これを実現する方法は次の通りである。

まず，特殊レジスタをコンテキストに含めるかどうかを指定するタスク属性を設ける。例えば，FPUレジスタであれば，タスク属性にTA_FPUを設ける。タスクディスパッチャでは，タスク属性を見て，その属性が設定されていれば特殊レジスタを保存/復帰する。

ハードウェア的に特殊レジスタがディスエーブルできる場合には，その属性が設定されていないタスクに切り換える時に特殊レジスタをディスエーブルすると，誤って特殊レジスタを使った場合を検出できる。

さらに，割込みハンドラ（ISR，周期ハンドラ，アラームハンドラを含む）やCPU例外ハンドラで特殊レジスタを使用する場合には，これらの処理単位にも特殊レジスタを使用するかどうかの属性を設ける方法が考えられる。

ここで，タスク（または他の処理単位）が特殊レジスタを使用するかどうかは，コンパイラやライブラリに依存する場合があるため，注意が必要である。例えば，浮動小数点演算を含まないプログラムであっても，コンパイラがその方が性能が高いと判断すれば，浮動小数点命令を生成する場合がある。これによって，意図せずに特殊レジスタが使用される可能性があるため，ハードウェアがそれをサポートしていれば，特殊レジスタを使用しない処理単位の実行中は，特殊レジスタを使用できないように設定する（FPUレジスタであれば，FPUをディスエーブルする）ことが望ましい。

> **asp3_core注**: コーディング規約「浮動小数点はタスクコンテキストでのみ使用（割込みハンドラ内禁止）」（`AGENTS.md` §6）は，本節のFPU退避コストの議論に基づきます。

## CPU例外ハンドラの直接呼出し

CPU例外ハンドラの出入口処理は，CPU例外が発生しないように実装しなければならないが，これが防げないターゲットにおいては，CPU例外ハンドラの出入口処理を経由せずに，アプリケーションが用意したCPU例外ハンドラを直接実行する方法を用意するのが望ましい。これを，CPU例外ハンドラの直接呼出しと呼ぶ。

ここでは，ハードウェアでベクタテーブルを持つプロセッサにおいて，ターゲット非依存部に含まれる標準のCPU例外管理機能の初期化処理を用いている場合（OMIT_INITILIZE_EXCEPTIONをマクロ定義していない場合）に，ターゲット依存部のみの修正により，CPU例外ハンドラの直接呼出しの機能を追加する方法について説明する。

### TA_DIRECT属性の導入

CPU例外ハンドラの直接呼出しを指定するために，CPU例外ハンドラ属性に，TA_DIRECT属性を導入する。

### ターゲット依存部の修正箇所

TA_DIRECTの値を，target_kernel.h（または，そこからインクルードされるファイル）で定義し，その値をコンフィギュレータが取り出せるように，target_sym.defに次の行を追加する。

```
TA_DIRECT
```

また，target_kernel_impl.hで，TARGET_EXCATRに定義される値にTA_DIRECTを追加する。例えば，他にターゲット依存のCPU例外ハンドラ属性がない場合には，次のように定義する。

```c
#define TARGET_EXCATR		(TA_DIRECT)
```

次に，target_kernel_impl.hでOMIT_INITILIZE_EXCEPTIONをマクロ定義する。これにより，EXCINIBとinitialize_exceptionの定義がカーネルのターゲット非依存部から取り除かれるため，それらのコードをターゲット依存部に追加する。

また，exception.trb中のCPU例外ハンドラのための標準的な初期化情報を生成する処理がスキップされるため，スキップされるコードをtarget_kernel.trbにコピーした上で，以下の変更を加える。

CPU例外ハンドラのエントリを生成するための記述（EXCHDR_ENTRYのリスト）を生成する部分は，次のように変更する。

```ruby
    $cfgData[:DEF_EXC].each do |_, params|
      if (params[:excatr] & $TA_DIRECT) == 0
        $kernelCfgC.add("EXCHDR_ENTRY(#{params[:excno]}, " \
						"#{params[:excno].val}, #{params[:exchdr]})")
      end
    end
```

また，CPU例外ハンドラ初期化ブロックの定義を生成する部分は，次のように変更する。

```ruby
    $kernelCfgC.add("const EXCINIB _kernel_excinib_table[TNUM_DEF_EXCNO] = {")
    $cfgData[:DEF_EXC].each_with_index do |(_, params), index|
      $kernelCfgC.add(",") if index > 0
      if (params[:excatr] & $TA_DIRECT) == 0
        $kernelCfgC.append("\t{ (#{params[:excno]}), (#{params[:excatr]}), " \
				"(FP)(EXC_ENTRY(#{params[:excno]}, #{params[:exchdr]})) }")
      else
        $kernelCfgC.append("\t{ (#{params[:excno]}), (#{params[:excatr]}), " \
											"(FP)(#{params[:exchdr]}) }")
      end
    end
    $kernelCfgC.add
    $kernelCfgC.add2("};")
```

> **asp3_core注**: 上記コード例は上流のRuby版生成スクリプト（`.trb`）のものです。asp3_coreで同等の変更を行う場合は，Python版生成スクリプト（`kernel/exception.py` のコードを `target/<name>/target_kernel.py` へコピーして変更）に読み替えてください。

以上

---

**原本との対応確認**:
- 章立て: ✅ 全8章（エラーチェック省略・mutex除去・非タスクコンテキスト化・CPUロック化・ロック解除順制限・拡張パッケージ9種・特殊レジスタ・CPU例外直接呼出し）完全網羅
- 本文: ✅ 原文に忠実（対象サービスコールリスト・変更前後コード・追加API（acre_*/del_*/AID_*等）・fch_mnt仕様を全数保持）
- asp3_core注: ✅ 禁則①との関係（PRISTINE上書き＝要ユーザー確認・DIVERGENCE_MAP記録）・CMake/Python読み替えを付記
