# TOPPERS/ASP3カーネル ミューテックス機能の設計

**原本**: `doc/mutex_design.txt`（対応バージョン: Release 3.7.2・最終更新: 2022年9月25日）  
**Markdown化**: 2026-06-07

このドキュメントは，TOPPERS/ASP3カーネルのミューテックス機能の設計メモである。「TOPPERS/ASP3カーネル 設計メモ」（[design_overview.md](design_overview.md)）の一部分となるべきものである。

> **asp3_core注**: 本書は `kernel/mutex.c`・`kernel/mutex.h`（PRISTINE・編集禁止）の設計解説であり，asp3_coreでも全文がそのまま有効です。**優先度上限ミューテックス（TA_CEILING）のみをサポートする標準構成**の設計です（優先度継承ミューテックスの設計は [design_inherit.md](design_inherit.md)）。本書のコード断片は実装の抜粋・簡略版であり，正確なコードは `kernel/mutex.c`・`kernel/task.c` を参照してください。

## 目次

- 実装に向けた仕様分析
  - 優先度制御規則
  - 優先度制御の必要箇所
- データ構造と下請け関数の設計と実装
  - タスク管理ブロックとミューテックス管理ブロック
  - 現在優先度の計算処理
  - 現在優先度の変更処理
  - ミューテックスの操作に伴う現在優先度の制御処理
- 優先度制御が必要な処理の設計と実装
  - chg_priの実装
  - loc_mtx，ploc_mtx，tloc_mtx
  - unl_mtx
  - ini_mtx
  - タスクの終了処理（task_terminate）
- ミューテックス機能をリンクしない工夫

---

## 実装に向けた仕様分析

統合仕様書では，優先度継承ミューテックスを含めた仕様を記述しているが，ここでは，優先度継承ミューテックスをサポートしない場合の仕様について分析する。

### 優先度制御規則

タスクの現在優先度は，常に，以下の優先度の最高値に一致するように設定する（［NGKI2014］から優先度継承ミューテックス関連を削除）。

1. そのタスクのベース優先度
2. そのタスクがロックしている優先度上限ミューテックスの優先度上限

タスクの現在優先度を決定する要素となるこれらの優先度を，「**要素優先度**」と呼ぶことにする。

現在優先度を制御する最も簡単な方法は，要素優先度が変化（または増減）するたびに，要素優先度をすべてスキャンして，その中の最高値を求める方法である。しかしこの方法は，効率が悪い。これを最適化する余地は数多くあるが，あまり細かく最適化すると，コードが複雑になり，テスト工数も増えてしまう。

どこまでの最適化を行うかは悩ましいところであるが，少なくとも要素優先度が上がる（または増える）場合には，上がった後の要素優先度とタスクの現在優先度を比較するだけで，現在優先度の変更の必要性が判断でき，すべての要素優先度をスキャンする必要はない。

また，要素優先度が下がる（または減る）場合にも，下がる前の要素優先度がタスクの現在優先度と一致していなかった場合（言い換えると，その要素優先度がタスクの現在優先度を決定していなかった場合）には，タスクの現在優先度が変化しないことは明らかである。一方，下がる前の要素優先度がタスクの現在優先度と一致していた場合には，すべての要素優先度をスキャンしてタスクの現在優先度を計算することが必要である。

言うまでもなく，要素優先度が変化しない場合には，タスクの現在優先度は変化しない。

以上より，現在優先度を決定するための要素優先度が上がる（または増える）場合と，下がる（または減る）場合は，上記のような最適化が有効と考えられるため，この粒度の最適化は行うこととする。

### 優先度制御の必要箇所

前述の(1)および(2)の要素優先度が変化（または増減）する場合に，タスクの現在優先度の変更処理が必要になる。そこで，(1)および(2)の要素優先度のそれぞれについて，どのような場合に変化（または増減）するかをリストアップする。これを元に，プログラム中のどの箇所で現在優先度の変更処理を行うべきか洗い出す。

**(1)「そのタスクのベース優先度」** が変化するのは，chg_priによってタスクのベース優先度を変更した場合のみである。この時は，要素優先度が上がる場合も下がる場合もある。

**(2)「そのタスクがロックしている優先度上限ミューテックスの優先度上限」** が変化するのは，以下のいずれかの状況である。

- **(2-1)** そのタスクが優先度上限ミューテックスをロックした時。この時は，要素優先度が増える。
- **(2-2)** そのタスクが優先度上限ミューテックスをロック解除した時。この時は，要素優先度が減る。

## データ構造と下請け関数の設計と実装

### タスク管理ブロックとミューテックス管理ブロック

ミューテックスを管理するデータ構造として，ミューテックスの待ちキューに加えて，タスクに対してそれがロックしているミューテックスのリストと，ミューテックスに対してそれを保持しているタスクを管理する必要がある。

タスクがロックしているミューテックスのリストは，ミューテックスのロック解除はロックの逆順で行うことから，ロックの逆順のシングルリンクキューで実現する。具体的には，TCBには，最後にロックしたミューテックスの管理ブロックへのポインタを追加し，ミューテックス管理ブロックには，そのミューテックスをロックしたタスクがこの前にロックしたミューテックスの管理ブロックへのポインタを格納する。

また，TCBには，現在優先度に加えて，ベース優先度を保持するbpriorityフィールドを追加する。bpriorityフィールドは，タスクが休止状態以外で有効なフィールドとする。

```c
typedef struct task_control_block {
	...
	BIT_FIELD_UINT	bpriority : TBIT_TCB_PRIORITY;
									/* ベース優先度（内部表現）*/
	BIT_FIELD_UINT	priority : TBIT_TCB_PRIORITY;
									/* 現在優先度（内部表現）*/
	...
	BIT_FIELD_BOOL	boosted : 1;	/* 優先度上昇状態 */
	...
	MTXCB			*p_lastmtx;		/* 最後にロックしたミューテックス */
	...
} TCB;
```

boostedフィールドは，当初，サブ優先度をサポートする場合に，タスクが優先度上昇状態かどうかを効率的に判定するために追加したが，サブ優先度をサポートしない場合にも有益であることがわかったため，ベースパッケージにも導入することとした。boostedフィールドは，タスクが休止状態以外で有効なフィールドとし，make_dormantにおいて初期化する。

p_lastmtxは，休止状態以外で有効な変数であるが，休止状態に遷移する際に必ず空にするので，（make_dormantではなく）initialize_taskにおいて初期化すればよい。

ミューテックス管理ブロックには，ミューテックス待ちキューに加えて，ミューテックスをロックしているタスクのTCBへのポインタと，このミューテックスをロックしたタスクがこの前にロックしたミューテックスの管理ブロックへのポインタを格納する。

```c
typedef struct mutex_initialization_block {
	ATR			mtxatr;			/* ミューテックス属性 */
	uint_t		ceilpri;		/* ミューテックスの上限優先度（内部表現）*/
} MTXINIB;

typedef struct mutex_control_block {
	QUEUE		wait_queue;		/* ミューテックス待ちキュー */
	const MTXINIB *p_mtxinib;	/* 初期化ブロックへのポインタ */
	TCB			*p_loctsk;		/* ミューテックスをロックしているタスク */
	MTXCB		*p_prevmtx;		/* この前にロックしたミューテックス */
} MTXCB;
```

ミューテックスがロックされていないことは，p_loctskをNULLに設定することで表す。ミューテックスがロックされていない時には，p_prevmtxは無効である。

### 現在優先度の計算処理

すべての要素優先度をスキャンし，タスクの現在優先度を計算する関数を実装する。この関数を呼ぶことにより，タスクの現在優先度に設定すべき値を計算することができる。この関数は，以下のデータ構造を参照するため，これらが更新された後に呼び出さなければならない。

- TCB中のベース優先度
- タスクがロックしているミューテックスのリスト（TCBとミューテックス管理ブロック中で管理）

また，この関数は，優先度上昇状態であるかを判定し，p_tcb->p_boostedに返す。

```c
/* 
 *  タスクの現在優先度の計算
 *
 *  p_tcbで指定されるタスクの現在優先度（に設定すべき値）を計算する．
 *  また，優先度上昇状態であるかを判定し，p_tcb->boostedに返す．
 */
uint_t
mutex_calc_priority(TCB *p_tcb)
{
	uint_t	priority;
	MTXCB	*p_mtxcb;
	bool_t	boosted;

	priority = p_tcb->bpriority;
	p_mtxcb = p_tcb->p_lastmtx;
	boosted = false;
	while (p_mtxcb != NULL) {
		if (MTX_CEILING(p_mtxcb)) {
			if (p_mtxcb->p_mtxinib->ceilpri < priority) {
				priority = p_mtxcb->p_mtxinib->ceilpri;
			}
			boosted = true;
		}
		p_mtxcb = p_mtxcb->p_prevmtx;
	}
	p_tcb->boosted = boosted;
	return(priority);
}
```

### 現在優先度の変更処理

ミューテックスの操作に伴ってタスクの現在優先度を変更する場合には，TCB中の現在優先度を更新することに加えて，次の処理を行う必要がある。

> ミューテックス機能によりタスクの現在優先度が変化する場合と，サブ優先度が使われる状況（サブ優先度機能をサポートするカーネルで，タスクの現在優先度がサブ優先度を使用すると設定されている場合）で優先度上昇状態であるか否かが変化する場合には，以下の処理が行われる．
>
> 現在優先度を変化させるサービスコールの前後とも，当該タスクが実行できる状態である場合には，優先度が同じタスク（サブ優先度が使われる状況では，サブ優先度も同じタスク）の中で優先順位が最も高くなる【NGKI2015】．そのサービスコールにより，当該タスクが実行できる状態に遷移する場合には，優先度が同じタスク（サブ優先度が使われる状況では，サブ優先度も同じタスク）の中で優先順位が最も低くなる【NGKI2016】．
>
> そのサービスコールの後で，当該タスクが待ち状態で，タスクの優先度順の待ち行列につながれている場合には，当該タスクの変更後の現在優先度に従って，その待ち行列中での順序が変更される【NGKI2017】．待ち行列中に同じ現在優先度のタスクがある場合には，当該タスクの順序はそれらの中で最後になる【NGKI2018】．

一般に，タスクが新たに実行できる状態になった場合は，そのタスクをレディキュー中の同優先度タスクの最後に入れるため，NGKI2016に合致している。また，タスクが新たに待ち状態になった場合には，そのタスクを，待ち行列中の同優先度のタスク最後に入れるため，NGKI2018に合致している。そのため，これらの場合には，ミューテックス機能によりタスクの現在優先度が変化しても，特別な処理は必要ない。

タスクの状態が変化せず，現在優先度が変化する場合の処理は，chg_priの下請け関数であるchange_priorityを共用する。この場合，対象タスクが実行できる状態である場合には，同優先度のタスクの中で最高優先順位にする必要がある。

一方，chg_priによる現在優先度の変更時には，次の処理を行う必要がある。

> chg_priを発行した結果，対象タスクの現在優先度が変化する場合と，対象タスクが優先度上昇状態でない場合（正確な定義は「4.4.5ミューテックス」の節を参照すること．ミューテックス機能を使わない場合には，この条件は常に成り立つ）には，次の処理が行われる．
>
> 対象タスクが実行できる状態の場合には，現在優先度が同じ（サブ優先度が使われる状況では，サブ優先度も同じ）タスクの中で，対象タスクの優先順位が最も低くなる【NGKI1194】．対象タスクが待ち状態で，タスクの優先度順の待ち行列につながれている場合には，対象タスクの変更後の現在優先度に従って，その待ち行列中での順序が変更される【NGKI1195】．待ち行列中に同じ現在優先度のタスクがある場合には，対象タスクの順序はそれらの中で最後になる【NGKI1196】．
>
> 対象タスクが優先度上昇状態であり，対象タスクの現在優先度が変化しない場合には，対象タスクの優先順位や待ち行列中での順序は変更されない【NGKI1197】．

つまり，記述された条件を満たした場合には，同優先度のタスクの中で最低優先順位にする必要がある。

そこで，change_priorityにmtxmodeパラメータを設けて，同優先度のタスクの中での優先順位を指定できるようにする。具体的には，mtxmodeがfalseの時は最低優先順位，mtxmodeがtrueの時は最高優先順位とする。ミューテックスの操作に伴って現在優先度を変更する場合にはmtxmodeをtrueに，chg_priにより現在優先度を変更する場合にはmtxmodeをfalseにして，change_priorityを呼び出す。

なお，ミューテックスの操作に伴って現在優先度を変更する場合には，現在優先度が実際に変化する場合にのみ，change_priorityを呼び出すようにしなければならない（現在優先度が変化しない場合に呼び出すと，仕様に反して優先順位や待ち行列中での順序が変更されてしまう）。

```c
void
change_priority(TCB *p_tcb, uint_t newpri, bool_t mtxmode)
{
	uint_t	oldpri;
	MTXCB	*p_mtxcb;
	ATR		mtxproto;

	oldpri = p_tcb->priority;
	p_tcb->priority = newpri;

	if (TSTAT_RUNNABLE(p_tcb->tstat)) {
		/*
		 *  タスクが実行できる状態の場合
		 */
		queue_delete(&(p_tcb->task_queue));
		if (queue_empty(&(ready_queue[oldpri]))) {
			primap_clear(oldpri);
		}
		if (mtxmode) {
			queue_insert_next(&(ready_queue[newpri]), &(p_tcb->task_queue));
		}
		else {
			queue_insert_prev(&(ready_queue[newpri]), &(p_tcb->task_queue));
		}
		primap_set(newpri);

		if (dspflg) {
			if (p_schedtsk == p_tcb) {
				if (newpri >= oldpri) {
					p_schedtsk = search_schedtsk();
				}
			}
			else {
				if (newpri <= p_schedtsk->priority) {
					p_schedtsk = (TCB *)(ready_queue[newpri].p_next);
				}
			}
		}
	}
	else {
		if (TSTAT_WAIT_WOBJCB(p_tcb->tstat)) {
			/*
			 *  タスクがその他のオブジェクト待ち状態で，同期・通信オブ
			 *  ジェクトの管理ブロックの共通部分（WOBJCB）の待ちキュー
			 *  につながれているの場合
			 */
			wobj_change_priority(((WINFO_WOBJ *)(p_tcb->p_winfo))->p_wobjcb,
																	p_tcb);
		}
	}
}
```

### ミューテックスの操作に伴う現在優先度の制御処理

前節のchange_priorityを用いて，ミューテックスの操作に伴う現在優先度の制御処理を実装する。なお，これらの処理は，ミューテックスの操作によってタスク状態が変化しない場合に用いるものである。

ミューテックスの操作に伴って要素優先度が上がる（または増える）場合の処理は次の通り。newpriには，上がった後の（または増えた）要素優先度を渡す。

```c
void
mutex_raise_priority(TCB *p_tcb, uint_t newpri)
{
	p_tcb->boosted = true;
	if (newpri < p_tcb->priority) {
		change_priority(p_tcb, newpri, true);
	}
}
```

＊サブ優先度をサポートする場合

```c
void
mutex_raise_priority(TCB *p_tcb, uint_t newpri)
{
	if (newpri <= p_tcb->priority) {
		if (newpri < p_tcb->priority
				|| ((subprio_primap & PRIMAP_BIT(p_tcb->priority)) != 0U
												&& !(p_tcb->boosted))) {
			/*
			 *  p_tcb->boostedは，change_priorityの中で参照するため，
			 *  それを呼ぶ前に更新する必要がある．
			 */
			p_tcb->boosted = true;
			change_priority(p_tcb, newpri, true);
		}
		else {
			p_tcb->boosted = true;
		}
	}
}
```

ミューテックスの操作に伴って要素優先度が下がる（または減る）場合の処理は次の通り。oldpriには，下がる前の（または減った）要素優先度を渡す。

```c
void
mutex_drop_priority(TCB *p_tcb, uint_t oldpri)
{
	uint_t	newpri;

	if (oldpri == p_tcb->priority) {
		newpri = mutex_calc_priority(p_tcb);
		if (newpri != p_tcb->priority) {
			change_priority(p_tcb, newpri, true);
		}
	}
}
```

＊サブ優先度をサポートする場合

```c
void
mutex_drop_priority(TCB *p_tcb, uint_t oldpri)
{
	uint_t	newpri;

	if (oldpri == p_tcb->priority) {
		newpri = mutex_calc_priority(p_tcb);
		if (newpri != p_tcb->priority
				|| ((subprio_primap & PRIMAP_BIT(p_tcb->priority)) != 0U
												&& !(p_tcb->boosted))) {
			change_priority(p_tcb, newpri, true);
		}
	}
}
```

## 優先度制御が必要な処理の設計と実装

優先度制御が必要な処理の設計と実装について，それぞれ検討する。

### chg_pri

chg_priは，優先度制御の必要箇所の(1)に該当する。タスクが優先度上限ミューテックスをロックしている間は，chg_priにより現在優先度が下がることはない（優先度上限ミューテックスの優先度上限になっているため）し，上がることはない（次に述べるE_ILUSEエラーとなる）。そのため，change_priorityを呼び出すのは，タスクが優先度上限ミューテックスをロックしていない場合（＝優先度上昇状態でない場合）のみである。

```c
	bool_t	boosted;

	lock_cpu();
	if (TSTAT_DORMANT(p_tcb->tstat)) {
		ercd = E_OBJ;							/*［NGKI1191］*/
	}
	else if ((p_tcb->boosted || TSTAT_WAIT_MTX(p_tcb->tstat))
						&& !((*mtxhook_check_ceilpri)(p_tcb, newbpri))) {
		ercd = E_ILUSE;							/*［NGKI1201］*/
	}
	else {
		p_tcb->bpriority = newbpri;				/*［NGKI1192］*/
		if (!(p_tcb->boosted)) {
			change_priority(p_tcb, newbpri, false);		/*［NGKI1193］*/
			if (p_runtsk != p_schedtsk) {
				dispatch();
			}									/*［NGKI1197］*/
		}
		ercd = E_OK;
	}
	unlock_cpu();
```

E_ILUSEエラーを返す処理は，以下のエラーチェックのためのコードである。

> 対象タスクが優先度上限ミューテックスをロックしているかロックを待っている場合，tskpriは，それらのミューテックスの上限優先度と同じかそれより低くなければならない．そうでない場合には，E_ILUSEエラーとなる【NGKI1201】．

mutex_check_ceilpriの実装は次の通り。この関数は，エラーを検出した場合にfalseを，そうでない場合にtrueを返す。

```c
bool_t
mutex_check_ceilpri(TCB *p_tcb, uint_t bpriority)
{
	MTXCB	*p_mtxcb;

	/*
	 *  タスクがロックしている優先度上限ミューテックスの中で，上限優先
	 *  度がbpriorityよりも低いものがあれば，falseを返す．
	 */
	p_mtxcb = p_tcb->p_lastmtx;
	while (p_mtxcb != NULL) {
		if (MTX_CEILING(p_mtxcb) && bpriority < p_mtxcb->p_mtxinib->ceilpri) {
			return(false);
		}
		p_mtxcb = p_mtxcb->p_prevmtx;
	}

	/*
	 *  タスクが優先度上限ミューテックスのロックを待っている場合に，そ
	 *  の上限優先度がbpriorityよりも低くければ，falseを返す．
	 */
	if (TSTAT_WAIT_MTX(p_tcb->tstat)) {
		p_mtxcb = ((WINFO_MTX *)(p_tcb->p_winfo))->p_mtxcb;
		if (MTX_CEILING(p_mtxcb) && bpriority < p_mtxcb->p_mtxinib->ceilpri) {
			return(false);
		}
	}

	/*
	 *  いずれの条件にも当てはまらなければtrueを返す．
	 */
	return(true);
}
```

### loc_mtx，ploc_mtx，tloc_mtx

loc_mtx（ploc_mtx，tloc_mtxも同様。以下同じ）は，対象ミューテックスがロック解除されていれば，ミューテックスをロックし，そうでない場合には，ミューテックスのロック待ちに入る。

loc_mtxでミューテックスをロックする時は，優先度制御の必要箇所の(2-1)に該当する。タスクはloc_mtxの呼び出し前後とも実行できる状態であるため，loc_mtxから呼ばれるmutex_acquireで，タスクの要素優先度が増える場合の処理を行う。

```c
	if (MTX_CEILING(p_mtxcb)) {
		mutex_raise_priority(p_tcb, p_mtxcb->p_mtxinib->ceilpri);
	}
```

### unl_mtx

unl_mtxは，対象ミューテックスをロック解除し，ミューテックスのロック待ちのタスクがあれば，待ちキューの先頭のタスクを待ち解除し，ミューテックスをロックさせる。

unl_mtxでミューテックスをロック解除する処理は，優先度制御の必要箇所の(2-2)に該当する。タスクはunl_mtxの呼び出し前後とも実行できる状態であるため，unl_mtxでは，タスクの要素優先度が減る場合の処理を行う。減る要素優先度は優先度上限ミューテックスの優先度上限である。

```c
	if (MTX_CEILING(p_mtxcb)) {
		mutex_drop_priority(p_loctsk, p_mtxcb->p_mtxinib->ceilpri);
	}
```

unl_mtxで待ちキューの先頭のタスクにミューテックスをロックさせる処理は，優先度制御の必要箇所の(2-1)に該当するが，タスクは待ち状態から実行できる状態に遷移するため，待ち解除されたタスクは，必要な優先度の変更を行なった後に，通常通り実行可能状態に遷移させれば良い。なお，この処理は，unl_mtxから呼ばれるmutex_releaseで行う。

```c
	if (MTX_CEILING(p_mtxcb)) {
		if (p_mtxcb->p_mtxinib->ceilpri < p_tcb->priority) {
			p_tcb->priority = p_mtxcb->p_mtxinib->ceilpri;
		}
		p_tcb->boosted = true;
	}
```

### ini_mtx

ini_mtxは，対象ミューテックスをロックしているタスクにロック解除させ，ミューテックスのロック待ちタスクをすべて待ち解除する。

ini_mtxでミューテックスをロック解除させる処理は，優先度制御の必要箇所の(2-2)に該当する。ロックを解除させられるタスクの状態は，ini_mtxの前後で変化しないため，ini_mtxでは，タスクの要素優先度が減る場合の処理を行う。減る要素優先度は，優先度上限ミューテックスの優先度上限である。

```c
	lock_cpu();
	init_wait_queue(&(p_mtxcb->wait_queue));
	p_loctsk = p_mtxcb->p_loctsk;
	if (p_loctsk != NULL) {
		p_mtxcb->p_loctsk = NULL;
		(void) remove_mutex(p_loctsk, p_mtxcb);
		if (MTX_CEILING(p_mtxcb)) {
			mutex_drop_priority(p_loctsk, p_mtxcb->p_mtxinib->ceilpri);
		}
	}
	if (p_runtsk != p_schedtsk) {
		dispatch();
	}
	ercd = E_OK;
	unlock_cpu();
```

### タスクの終了処理（task_terminate）

ASP3カーネルでは，タスクの終了処理は，task_terminateで行われる。task_terminateは，ext_tsk，ras_ter，ena_ter，ter_tsk，ena_dsp，chg_ipmから呼び出される。

teminate_taskは，次の処理を行う。

- 対象タスクがミューテックスのロック待ち状態であれば待ち解除する。
- 対象タスクがロックしていたミューテックスをロック解除する。
- ロック解除したミューテックスにロック待ちのタスクがあれば，待ちキューの先頭のタスクを待ち解除し，ミューテックスをロックさせる。

対象タスクがロックしていたミューテックスをロック解除する処理は，優先度制御の必要箇所の(2-2)に該当するが，次の理由により省略することができる。(2-2)で行うべき処理は，ミューテックスのロック解除により対象タスクの現在優先度／優先順位を変更するものであるが，対象タスクは終了するため，現在優先度／優先順位を更新する必要はない。

ロック解除したミューテックスにロック待ちのタスクがあった場合に，待ちキューの先頭のタスクを待ち解除し，ミューテックスをロックさせる処理は，優先度制御の必要箇所の(2-1)に該当する。この処理は，unl_mtxで待ちキューの先頭のタスクにミューテックスをロックさせる処理と共通であり，teminate_taskから，mutex_release_allを経由してmutex_releaseを呼び出すことで行う。

```c
void
task_terminate(TCB *p_tcb)
{
	...
	mutex_release_all(p_tcb);
	...
}

void
mutex_release_all(TCB *p_tcb)
{
	MTXCB	*p_mtxcb;

	while ((p_mtxcb = p_tcb->p_lastmtx) != NULL) {
		p_tcb->p_lastmtx = p_mtxcb->p_prevmtx;
		mutex_release(p_mtxcb);
	}
}
```

## ミューテックス機能をリンクしない工夫

最後に，ミューテックス機能を使用しない場合に，ミューテックス関連のコードをリンクしない工夫を行う。

ミューテックスモジュールの外から呼び出すミューテックスモジュールの内部関数は次の通り。

- `mutex_check_ceilpri`（chg_priより）
- `mutex_release_all`（task_terminateより）

これらの関数はフックルーチンであるものとし，呼び出す際には，変数参照をはさむことにする。例えば，「mutex_release_all(p_tcb)」に代えて，「(\*mtxhook_release_all)(p_tcb)」と記述する。

mtxhook_release_allには，initialize_mutexにおいて，mutex_release_allへのポインタを格納する。initialize_mutexは，ミューテックス機能を用いる場合にのみ呼び出されるため，これにより，ミューテックス機能を用いない場合には，mutex_release_allがリンクされないことになる。

ここで，ミューテックス機能を用いない場合には，mtxhook_release_allを参照してはならないものとする（NULLが入ることは仮定しない）。例えば，task_terminateからmutex_release_allを呼ぶ箇所は次のようにする。

```c
	if (p_tcb->p_lastmtx != NULL) {
		(*mtxhook_release_all)(p_runtsk);
	}
```

TCB中のp_lastmtxがNULLでなくなるのは，タスクがミューテックスをロックしている場合のみで，これはミューテックス機能を用いた場合に限られる。そのため，上のコードで，ミューテックス機能を用いない場合にはmtxhook_release_allが参照されることはない。

以上

---

**原本との対応確認**:
- 章立て: ✅ 全4章（仕様分析・データ構造と下請け関数・優先度制御が必要な処理・リンクしない工夫）完全網羅
- 本文: ✅ 原文に忠実（NGKI要件番号・実装コード（mutex_calc_priority／change_priority／mutex_raise_priority／mutex_drop_priority／mutex_check_ceilpri／mutex_release_all等）を全数保持）
- asp3_core注: ✅ PRISTINE領域＝全文有効・実装ファイルへの参照を付記
