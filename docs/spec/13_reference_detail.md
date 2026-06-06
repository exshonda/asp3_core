# １３．リファレンス

**原本**: `doc/user.txt` §13  
**最終更新**: 2026-05-18（上流）→ Markdown化: 2026-06-07

> **asp3_core注（本章の位置付け）**: 各サービスコール・静的APIの**詳細仕様**（パラメータ・エラーコード・呼出し可能コンテキスト・実装ソース参照）は **`docs/api/`（全111本＝サービスコール95本＋静的API16本）** を正本としてください。本章はプロトタイプの一覧（早見表）です。なお，本章の一覧は本リポジトリの `include/kernel.h`・`kernel/kernel_api.def` と整合しています（13.1にないサービスコールとして，`dis_ter`/`ena_ter` 等のタスク終了機能や `acre_*` 等は元々ASP3に存在しないため，95本との数の差は表記粒度によるものではなく，13.1は主要機能の一覧であることに注意。全数一覧は `docs/api/README.md`）。

## 13.1 サービスコール一覧

### (1) タスク管理機能

```c
ER ercd = act_tsk(ID tskid)
ER_UINT	actcnt = can_act(ID tskid)
ER ercd = get_tst(ID tskid, STAT *p_tskstat)
ER ercd = chg_pri(ID tskid, PRI tskpri)
ER ercd = get_pri(ID tskid, PRI *p_tskpri)
ER ercd = get_inf(EXINF *p_exinf)
ER ercd = ref_tsk(ID tskid, T_RTSK *pk_rtsk)
```

### (2) タスク付属同期機能

```c
ER ercd = slp_tsk(void)
ER ercd = tslp_tsk(TMO tmout)
ER ercd = wup_tsk(ID tskid)
ER_UINT wupcnt = can_wup(ID tskid)
ER ercd = rel_wai(ID tskid)
ER ercd = sus_tsk(ID tskid)
ER ercd = rsm_tsk(ID tskid)
ER ercd = dly_tsk(RELTIM dlytim)
```

### (3) タスク終了機能

```c
ER ercd = ext_tsk(void)
ER ercd = ras_ter(ID tskid)
ER ercd = dis_ter(void)
ER ercd = ena_ter(void)
bool_t state = sns_ter(void)
ER ercd = ter_tsk(ID tskid)
```

### (4) 同期・通信機能

セマフォ：

```c
ER ercd = sig_sem(ID semid)
ER ercd = wai_sem(ID semid)
ER ercd = pol_sem(ID semid)
ER ercd = twai_sem(ID semid, TMO tmout)
ER ercd = ini_sem(ID semid)
ER ercd = ref_sem(ID semid, T_RSEM *pk_rsem)
```

イベントフラグ：

```c
ER ercd = set_flg(ID flgid, FLGPTN setptn)
ER ercd = clr_flg(ID flgid, FLGPTN clrptn)
ER ercd = wai_flg(ID flgid, FLGPTN waiptn,
					MODE wfmode, FLGPTN *p_flgptn)
ER ercd = pol_flg(ID flgid, FLGPTN waiptn,
					MODE wfmode, FLGPTN *p_flgptn)
ER ercd = twai_flg(ID flgid, FLGPTN waiptn,
					MODE wfmode, FLGPTN *p_flgptn, TMO tmout)
ER ercd = ini_flg(ID flgid)
ER ercd = ref_flg(ID flgid, T_RFLG *pk_rflg)
```

データキュー：

```c
ER ercd = snd_dtq(ID dtqid, intptr_t data)
ER ercd = psnd_dtq(ID dtqid, intptr_t data)
ER ercd = tsnd_dtq(ID dtqid, intptr_t data, TMO tmout)
ER ercd = fsnd_dtq(ID dtqid, intptr_t data)
ER ercd = rcv_dtq(ID dtqid, intptr_t *p_data)
ER ercd = prcv_dtq(ID dtqid, intptr_t *p_data)
ER ercd = trcv_dtq(ID dtqid, intptr_t *p_data, TMO tmout)
ER ercd = ini_dtq(ID dtqid)
ER ercd = ref_dtq(ID dtqid, T_RDTQ *pk_rdtq)
```

優先度データキュー：

```c
ER ercd = snd_pdq(ID pdqid, intptr_t data, PRI datapri)
ER ercd = psnd_pdq(ID pdqid, intptr_t data, PRI datapri)
ER ercd = tsnd_pdq(ID pdqid, intptr_t data, PRI datapri, TMO tmout)
ER ercd = rcv_pdq(ID pdqid, intptr_t *p_data, PRI *p_datapri)
ER ercd = prcv_pdq(ID pdqid, intptr_t *p_data, PRI *p_datapri)
ER ercd = trcv_pdq(ID pdqid, intptr_t *p_data, PRI *p_datapri, TMO tmout)
ER ercd = ini_pdq(ID pdqid)
ER ercd = ref_pdq(ID pdqid, T_RPDQ *pk_rpdq)
```

ミューテックス：

```c
ER ercd = loc_mtx(ID mtxid)
ER ercd = ploc_mtx(ID mtxid)
ER ercd = tloc_mtx(ID mtxid, TMO tmout)
ER ercd = unl_mtx(ID mtxid)
ER ercd = ini_mtx(ID mtxid)
ER ercd = ref_mtx(ID mtxid, T_RMTX *pk_rmtx)
```

### (5) メモリプール管理機能

```c
ER ercd = get_mpf(ID mpfid, void **p_blk)
ER ercd = pget_mpf(ID mpfid, void **p_blk)
ER ercd = tget_mpf(ID mpfid, void **p_blk, TMO tmout)
ER ercd = rel_mpf(ID mpfid, void *blk)
ER ercd = ini_mpf(ID mpfid)
ER ercd = ref_mpf(ID mpfid, T_RMPF *pk_rmpf)
```

### (6) 時間管理機能

システム時刻管理：

```c
ER ercd = set_tim(SYSTIM systim)
ER ercd = get_tim(SYSTIM *p_systim)
ER ercd = adj_tim(int32_t adjtim)
HRTCNT hrtcnt = fch_hrt(void)
```

周期通知：

```c
ER ercd = sta_cyc(ID cycid)
ER ercd = stp_cyc(ID cycid)
ER ercd = ref_cyc(ID cycid, T_RCYC *pk_rcyc)
```

アラーム通知：

```c
ER ercd = sta_alm(ID almid, RELTIM almtim)
ER ercd = stp_alm(ID almid)
ER ercd = ref_alm(ID almid, T_RALM *pk_ralm)
```

### (7) システム状態管理機能

```c
ER ercd = rot_rdq(PRI tskpri)
ER ercd = get_tid(ID *p_tskid)
ER ercd = get_lod(PRI tskpri, uint_t *p_load)
ER ercd = get_nth(PRI tskpri, uint_t nth, ID *p_tskid)
ER ercd = loc_cpu(void)
ER ercd = unl_cpu(void)
ER ercd = dis_dsp(void)
ER ercd = ena_dsp(void)
bool_t state = sns_ctx(void)
bool_t state = sns_loc(void)
bool_t state = sns_dsp(void)
bool_t state = sns_dpn(void)
bool_t state = sns_ker(void)
ER ercd = ext_ker(void)
```

### (8) 割込み管理機能

```c
ER ercd = dis_int(INTNO intno)
ER ercd = ena_int(INTNO intno)
ER ercd = clr_int(INTNO intno)
ER ercd = ras_int(INTNO intno)
ER_BOOL state = prb_int(INTNO intno)
ER ercd = chg_ipm(PRI intpri)
ER ercd = get_ipm(PRI *p_intpri)
```

### (9) CPU例外管理機能

```c
bool_t stat = xsns_dpn(void *p_excinf)
```

> **asp3_core注**: 上記は原本どおりの一覧です。このほかASP3には，非タスクコンテキスト用サービスコール（`iact_tsk` 等の i系・実体は同一関数）や，タイムイベント通知用の関数等があります。**全95本の正確な一覧と各詳細は `docs/api/README.md`** を参照してください。

## 13.2 静的API一覧

### (1) タスク管理機能

```c
CRE_TSK(ID tskid, { ATR tskatr, EXINF exinf, TASK task,
							PRI itskpri, size_t stksz, STK_T *stk })
```

### (4) 同期・通信機能

```c
CRE_SEM(ID semid, { ATR sematr, uint_t isemcnt, uint_t maxsem })
CRE_FLG(ID flgid, { ATR flgatr, FLGPTN iflgptn })
CRE_DTQ(ID dtqid, { ATR dtqatr, uint_t dtqcnt, void *dtqmb })
CRE_PDQ(ID pdqid, { ATR pdqatr, uint_t pdqcnt, PRI maxdpri, void *pdqmb })
CRE_MTX(ID mtxid, { ATR mtxatr, PRI ceilpri })
```

### (5) メモリプール管理機能

```c
CRE_MPF(ID mpfid, { ATR mpfatr, uint_t blkcnt, uint_t blksz,
									MPF_T *mpf, void *mpfmb })
```

### (6) 時間管理機能

```c
CRE_CYC(ID cycid, { ATR cycatr, ＜通知方法の指定＞,
									RELTIM cyctim, RELTIM cycphs })
CRE_ALM(ID almid, { ATR almatr, ＜通知方法の指定＞ })
```

### (8) 割込み管理機能

```c
CFG_INT(INTNO intno, { ATR intatr, PRI intpri })
CRE_ISR(ID isrid, { ATR isratr, EXINF exinf,
							INTNO intno, ISR isr, PRI isrpri })
DEF_INH(INHNO inhno, { ATR inhatr, INTHDR inthdr })
```

### (9) CPU例外管理機能

```c
DEF_EXC(EXCNO excno, { ATR excatr, EXCHDR exchdr })
```

### (10) システム構成管理機能

```c
DEF_ICS({ size_t istksz, STK_T *istk })
ATT_INI({ ATR iniatr, EXINF exinf, INIRTN inirtn })
ATT_TER({ ATR teratr, EXINF exinf, TERRTN terrtn })
```

> **asp3_core注**: 全16本＝`kernel/kernel_api.def`（構造の正本）と1対1対応。各詳細は `docs/api/` の大文字ファイル名（`CRE_TSK.md` 等）を参照。番号(2)(3)(7)に静的APIはありません（原本どおり欠番）。

## 13.3 バージョン履歴

| 日付 | リリース | 備考 |
|---|---|---|
| 2014年11月24日 | Release 3.A.0 | 最初の早期リリース（α版） |
| 2015年8月5日 | Release 3.B.0 | |
| 2016年2月8日 | Release 3.0.0 | 最初の一般公開 |
| 2016年2月14日 | Release 3.0.1 | |
| 2016年5月15日 | Release 3.1.0 | |
| 2017年7月21日 | Release 3.2.0 | |
| 2018年4月19日 | Release 3.3.0 | |
| 2018年5月2日 | Release 3.3.1 | |
| 2019年3月20日 | Release 3.4.0 | |
| 2019年10月6日 | Release 3.5.0 | |
| 2020年12月23日 | Release 3.6.0 | |
| 2023年3月14日 | Release 3.7.0 | |
| 2023年4月11日 | Release 3.7.1 | |
| 2026年5月8日 | Release 3.7.2 | |

> **asp3_core注**: asp3_coreが追従している上流バージョンは `UPSTREAM_VERSION` ファイルで管理しています。詳細な変更履歴は `doc/version.txt`（上流マージ追従対象のため，テキストのまま残置）を参照。

---

**原本との対応確認**:
- 章立て: ✅ 13.1（(1)〜(9)）・13.2（(1)〜(10)）・13.3 完全網羅
- 本文: ✅ 原文に忠実（サービスコール・プロトタイプ全数，静的API16本，リリース履歴14件を保持）
- asp3_core注: ✅ docs/api/（111本）への誘導・kernel_api.defとの対応・UPSTREAM_VERSIONを付記
