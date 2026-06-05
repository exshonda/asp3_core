# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: kernel_check.py (converted from kernel_check.trb) $
#

#
#  コンフィギュレータのパス3の生成スクリプト
#

timeStampFileName = "check.timestamp"

if 'lmaList' in dir():
    for lma in lmaList:
        start_data = SYMBOL(lma["START_DATA"])
        end_data = SYMBOL(lma["END_DATA"])
        start_idata = SYMBOL(lma["START_IDATA"])
        BCOPY(start_idata, start_data, end_data - start_data)

tmax_tskid = TMIN_TSKID + len(cfgData["CRE_TSK"]) - 1
tmax_semid = TMIN_SEMID + len(cfgData["CRE_SEM"]) - 1
tmax_flgid = TMIN_FLGID + len(cfgData["CRE_FLG"]) - 1
tmax_dtqid = TMIN_DTQID + len(cfgData["CRE_DTQ"]) - 1


def checkNotifyHandler(key, params, objid, exinf, nfyhdr):
    nfymode = params["nfymode"]
    nfymode1 = nfymode & 0x0f
    nfymode2 = nfymode & ~0x0f
    par1 = params.get("par1")
    par2 = params.get("par2")

    if (nfymode1 == TNFY_SETVAR or nfymode1 == TNFY_SETFLG
            or nfymode1 == TNFY_SNDDTQ):
        epar1 = params.get("par3")
    else:
        epar1 = params.get("par2")

    if nfymode == TNFY_HANDLER:
        tmehdr = nfyhdr
        params1 = dict(params)
        params1["tmehdr"] = par2
        if (tmehdr & (CHECK_FUNC_ALIGN - 1)) != 0:
            error_sapi("E_PAR", params1, "%%tmehdr is not aligned", objid)
        if CHECK_FUNC_NONNULL and tmehdr == 0:
            error_sapi("E_PAR", params1, "%%tmehdr is null", objid)

    if nfymode1 == TNFY_SETVAR or nfymode1 == TNFY_INCVAR:
        p_var = exinf
        params1 = dict(params)
        params1["p_var"] = par1
        if (p_var & (CHECK_INTPTR_ALIGN - 1)) != 0:
            error_sapi("E_PAR", params1, "%%p_var is not aligned", objid)
        if CHECK_INTPTR_NONNULL and p_var == 0:
            error_sapi("E_PAR", params1, "%%p_var is null", objid)
    elif nfymode1 == TNFY_ACTTSK or nfymode1 == TNFY_WUPTSK:
        tskid = exinf
        params1 = dict(params)
        params1["tskid"] = par1
        if not (TMIN_TSKID <= tskid and tskid <= tmax_tskid):
            error_sapi("E_ID", params1, "illegal %%tskid", objid)
    elif nfymode1 == TNFY_SIGSEM:
        semid = exinf
        params1 = dict(params)
        params1["semid"] = par1
        if not (TMIN_SEMID <= semid and semid <= tmax_semid):
            error_sapi("E_ID", params1, "illegal %%semid", objid)
    elif nfymode1 == TNFY_SETFLG:
        flgid = exinf
        params1 = dict(params)
        params1["flgid"] = par1
        if not (TMIN_FLGID <= flgid and flgid <= tmax_flgid):
            error_sapi("E_ID", params1, "illegal %%flgid", objid)
    elif nfymode1 == TNFY_SNDDTQ:
        dtqid = exinf
        params1 = dict(params)
        params1["dtqid"] = par1
        if not (TMIN_DTQID <= dtqid and dtqid <= tmax_dtqid):
            error_sapi("E_ID", params1, "illegal %%dtqid", objid)

    if nfymode2 == TENFY_SETVAR or nfymode2 == TENFY_INCVAR:
        p_var = PEEK(SYMBOL(params["nfyhdr"] + "_p_evar"), sizeof_intptr_t)
        params1 = dict(params)
        params1["p_var"] = epar1
        if (p_var & (CHECK_INTPTR_ALIGN - 1)) != 0:
            error_sapi("E_PAR", params1, "%%p_var is not aligned", objid)
        if CHECK_INTPTR_NONNULL and p_var == 0:
            error_sapi("E_PAR", params1, "%%p_var is null", objid)
    elif nfymode2 == TENFY_ACTTSK or nfymode2 == TENFY_WUPTSK:
        tskid = PEEK(SYMBOL(params["nfyhdr"] + "_etskid"), sizeof_ID)
        params1 = dict(params)
        params1["tskid"] = epar1
        if not (TMIN_TSKID <= tskid and tskid <= tmax_tskid):
            error_sapi("E_ID", params1, "illegal %%tskid", objid)
    elif nfymode2 == TENFY_SIGSEM:
        semid = PEEK(SYMBOL(params["nfyhdr"] + "_esemid"), sizeof_ID)
        params1 = dict(params)
        params1["semid"] = epar1
        if not (TMIN_SEMID <= semid and semid <= tmax_semid):
            error_sapi("E_ID", params1, "illegal %%semid", objid)
    elif nfymode2 == TENFY_SETFLG:
        flgid = PEEK(SYMBOL(params["nfyhdr"] + "_eflgid"), sizeof_ID)
        params1 = dict(params)
        params1["flgid"] = epar1
        if not (TMIN_FLGID <= flgid and flgid <= tmax_flgid):
            error_sapi("E_ID", params1, "illegal %%flgid", objid)
    elif nfymode2 == TENFY_SNDDTQ:
        dtqid = PEEK(SYMBOL(params["nfyhdr"] + "_edtqid"), sizeof_ID)
        params1 = dict(params)
        params1["dtqid"] = epar1
        if not (TMIN_DTQID <= dtqid and dtqid <= tmax_dtqid):
            error_sapi("E_ID", params1, "illegal %%dtqid", objid)


tinib = SYMBOL("_kernel_tinib_table")
for key, params in sorted(cfgData["CRE_TSK"].items()):
    task = PEEK(tinib + offsetof_TINIB_task, sizeof_TASK)
    if (task & (CHECK_FUNC_ALIGN - 1)) != 0:
        error_wrong_id("E_PAR", params, "task", "tskid", "not aligned")
    if CHECK_FUNC_NONNULL and task == 0:
        error_wrong_id("E_PAR", params, "task", "tskid", "null")

    if USE_TSKINICTXB:
        stk = GetStackTskinictxb(key, params, tinib)
    else:
        stk = PEEK(tinib + offsetof_TINIB_stk, sizeof_void_ptr)
    if (stk & (CHECK_STACK_ALIGN - 1)) != 0:
        error_wrong_id("E_PAR", params, "stk", "tskid", "not aligned")
    if CHECK_STACK_NONNULL and stk == 0:
        error_wrong_id("E_PAR", params, "stk", "tskid", "null")

    tinib += sizeof_TINIB

mpfinib = SYMBOL("_kernel_mpfinib_table", True)
for _, params in sorted(cfgData["CRE_MPF"].items()):
    mpf = PEEK(mpfinib + offsetof_MPFINIB_mpf, sizeof_void_ptr)
    if (mpf & (CHECK_MPF_ALIGN - 1)) != 0:
        error_wrong_id("E_PAR", params, "mpf", "mpfid", "not aligned")
    if CHECK_MPF_NONNULL and mpf == 0:
        error_wrong_id("E_PAR", params, "mpf", "mpfid", "null")
    mpfinib += sizeof_MPFINIB

cycinib = SYMBOL("_kernel_cycinib_table", True)
for key, params in sorted(cfgData["CRE_CYC"].items()):
    exinf = PEEK(cycinib + offsetof_CYCINIB_exinf, sizeof_EXINF)
    nfyhdr = PEEK(cycinib + offsetof_CYCINIB_nfyhdr, sizeof_NFYHDR)
    checkNotifyHandler(key, params, "cycid", exinf, nfyhdr)
    cycinib += sizeof_CYCINIB

alminib = SYMBOL("_kernel_alminib_table", True)
for key, params in sorted(cfgData["CRE_ALM"].items()):
    exinf = PEEK(alminib + offsetof_ALMINIB_exinf, sizeof_EXINF)
    nfyhdr = PEEK(alminib + offsetof_ALMINIB_nfyhdr, sizeof_NFYHDR)
    checkNotifyHandler(key, params, "almid", exinf, nfyhdr)
    alminib += sizeof_ALMINIB

if not OMIT_ISTACK and not OMIT_ISTK:
    if len(cfgData["DEF_ICS"]) > 0:
        params0 = cfgData["DEF_ICS"][1]
        istk = PEEK(SYMBOL("_kernel_istk"), sizeof_void_ptr)
        if (istk & (CHECK_STACK_ALIGN - 1)) != 0:
            error_wrong("E_PAR", params0, "istk", "not aligned")
        if CHECK_STACK_NONNULL and istk == 0:
            error_wrong("E_PAR", params0, "istk", "null")

inirtnb = SYMBOL("_kernel_inirtnb_table", True)
for _, params in cfgData["ATT_INI"].items():
    inirtn = PEEK(inirtnb + offsetof_INIRTNB_inirtn, sizeof_INIRTN)
    if (inirtn & (CHECK_FUNC_ALIGN - 1)) != 0:
        error_wrong("E_PAR", params, "inirtn", "not aligned")
    if CHECK_FUNC_NONNULL and inirtn == 0:
        error_wrong("E_PAR", params, "inirtn", "null")
    inirtnb += sizeof_INIRTNB

terrtnb = SYMBOL("_kernel_terrtnb_table", True)
for _, params in reversed(list(cfgData["ATT_TER"].items())):
    terrtn = PEEK(terrtnb + offsetof_TERRTNB_terrtn, sizeof_TERRTN)
    if (terrtn & (CHECK_FUNC_ALIGN - 1)) != 0:
        error_wrong("E_PAR", params, "terrtn", "not aligned")
    if CHECK_FUNC_NONNULL and terrtn == 0:
        error_wrong("E_PAR", params, "terrtn", "null")
    terrtnb += sizeof_TERRTNB
