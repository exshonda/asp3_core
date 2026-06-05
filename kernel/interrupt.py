# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: interrupt.py (converted from interrupt.trb) $
#

#
#  割込み管理機能の生成スクリプト
#

kernelCfgH.add(f"#define TNUM_ISRID\t{len(cfgData['CRE_ISR'])}")
for _, params in sorted(cfgData["CRE_ISR"].items()):
    kernelCfgH.add(f"#define {params['isrid']}\t{params['isrid'].val}")
kernelCfgH.add()

kernelCfgC.comment_header("Interrupt Management Functions")

kernelCfgC.add("#ifndef LOG_ISR_ENTER")
kernelCfgC.add("#define LOG_ISR_ENTER(isrid)")
kernelCfgC.add("#endif /* LOG_ISR_ENTER */")
kernelCfgC.add()
kernelCfgC.add("#ifndef LOG_ISR_LEAVE")
kernelCfgC.add("#define LOG_ISR_LEAVE(isrid)")
kernelCfgC.add("#endif /* LOG_ISR_LEAVE */")
kernelCfgC.add()

if 'INTNO_CREISR_VALID' not in dir():
    INTNO_CREISR_VALID = INTNO_VALID
if 'INHNO_CREISR_VALID' not in dir():
    INHNO_CREISR_VALID = INHNO_VALID

if 'INTPRI_CFGINT_VALID' not in dir():
    INTPRI_CFGINT_VALID = list(range(TMIN_INTPRI, TMAX_INTPRI + 1))

if len(INTNO_CREISR_VALID) != len(INHNO_CREISR_VALID):
    error_exit("length of `INTNO_CREISR_VALID' is different from"
               " length of `INHNO_CREISR_VALID'")

to_inhno_val = {}
to_intno_val = {}
inhno_creisr_list = list(INHNO_CREISR_VALID)
for _intno_val in INTNO_CREISR_VALID:
    _inhno_val = inhno_creisr_list.pop(0)
    to_inhno_val[int(_intno_val)] = _inhno_val
    to_intno_val[int(_inhno_val)] = _intno_val

for _, params in cfgData["CFG_INT"].items():
    if params["intno"] not in INTNO_VALID:
        error_illegal("E_PAR", params, "intno")

    if (params["intatr"] & ~(TA_ENAINT | TA_EDGE | TARGET_INTATR)) != 0:
        error_illegal_sym("E_RSATR", params, "intatr", "intno")

    if params["intpri"] not in INTPRI_CFGINT_VALID:
        error_illegal_sym("E_PAR", params, "intpri", "intno")

    if 'INTNO_FIX_NONKERNEL' in dir() and params["intno"] in INTNO_FIX_NONKERNEL:
        if params["intpri"] >= TMIN_INTPRI:
            error_ercd("E_OBJ", params, "%%intno must have higher priority"
                       " than TMIN_INTPRI in %apiname")

    if 'INTNO_FIX_KERNEL' in dir() and params["intno"] in INTNO_FIX_KERNEL:
        if params["intpri"] < TMIN_INTPRI:
            error_ercd("E_OBJ", params, "%%intno must have lower or equal"
                       " priority to TMIN_INTPRI in %apiname")

    if 'TargetCheckCfgInt' in dir():
        TargetCheckCfgInt(params)

for _, params in cfgData["DEF_INH"].items():
    if params["inhno"] not in INHNO_VALID:
        error_illegal("E_PAR", params, "inhno")

    if (params["inhatr"] & ~TARGET_INHATR) != 0:
        error_illegal_sym("E_RSATR", params, "inhatr", "inhno")

    if 'INHNO_FIX_NONKERNEL' in dir() and params["inhno"] in INHNO_FIX_NONKERNEL:
        if (params["inhatr"] & TA_NONKERNEL) == 0:
            error_ercd("E_RSATR", params, "%%inhno must be"
                       " non-kernel interrupt in %apiname")
            continue

    if 'INHNO_FIX_KERNEL' in dir() and params["inhno"] in INHNO_FIX_KERNEL:
        if (params["inhatr"] & TA_NONKERNEL) != 0:
            error_ercd("E_RSATR", params, "%%inhno must not be"
                       " non-kernel interrupt in %apiname")
            continue

    if params["inhno"] in INHNO_CREISR_VALID:
        _intno_val = to_intno_val[int(params["inhno"])]
        if _intno_val not in cfgData["CFG_INT"]:
            error_ercd("E_OBJ", params,
                       f"intno `{_intno_val}' corresponding to"
                       f" %%inhno in %apiname is not configured with CFG_INT")
        else:
            _intno_params = cfgData["CFG_INT"][_intno_val]
            if _intno_params["intpri"] < TMIN_INTPRI:
                if (params["inhatr"] & TA_NONKERNEL) == 0:
                    error_ercd("E_OBJ", params,
                               "TA_NONKERNEL must be set for"
                               " non-kernel interrupt handler in %apiname of %%inhno")
            else:
                if (params["inhatr"] & TA_NONKERNEL) != 0:
                    error_ercd("E_OBJ", params,
                               "TA_NONKERNEL must not be set for"
                               " kernel interrupt handler in %apiname of %%inhno")

    if 'TargetCheckDefInh' in dir():
        TargetCheckDefInh(params)

for _, params in sorted(cfgData["CRE_ISR"].items()):
    if (params["isratr"] & ~TARGET_ISRATR) != 0:
        error_illegal("E_RSATR", params, "isratr")

    if params["intno"] not in INTNO_CREISR_VALID:
        error_illegal("E_PAR", params, "intno")

    if not (TMIN_ISRPRI <= params["isrpri"] and params["isrpri"] <= TMAX_ISRPRI):
        error_illegal("E_PAR", params, "isrpri")

    # intnoに対応するinhnoに対してDEF_INHがある場合（E_OBJ）［NGKI3013］
    # この処理で［NGKI3063］にも対応している
    #（無効なintnoの場合はNone．Ruby版のnil参照と同じく非該当として扱う）
    _inhno_val = to_inhno_val.get(int(params["intno"]))
    if _inhno_val is not None and _inhno_val in cfgData["DEF_INH"]:
        error_ercd("E_OBJ", params, f"%%intno in %apiname is duplicated"
                   f" with inhno {cfgData['DEF_INH'][_inhno_val]['inhno']}")

    if params["intno"] not in cfgData["CFG_INT"]:
        error_ercd("E_OBJ", params,
                   "%%intno in %apiname is not configured with CFG_INT")
    else:
        _intno_params = cfgData["CFG_INT"][params["intno"]]
        if _intno_params["intpri"] < TMIN_INTPRI:
            error_ercd("E_OBJ", params,
                       "interrupt service routine cannot handle"
                       " non-kernel interrupt in %apiname of %isrid")

    if 'TargetCheckCreIsr' in dir():
        TargetCheckCreIsr(params)

for _intno_val in INTNO_CREISR_VALID:
    _isr_params_list = []
    for _, params in sorted(cfgData["CRE_ISR"].items()):
        if params["intno"] == _intno_val:
            _isr_params_list.append(params)

    if len(_isr_params_list) > 0:
        _inhno_val = to_inhno_val[int(_intno_val)]

        cfgData["DEF_INH"][_inhno_val] = {
            "inhno": NumStr(_inhno_val),
            "inhatr": NumStr(TA_NULL, "TA_NULL"),
            "inthdr": f"_kernel_inthdr_{_intno_val}",
            "apiname": "DEF_INH",
            "_file_": "",
            "_line_": 0,
        }

        kernelCfgC.add("void")
        kernelCfgC.add(f"_kernel_inthdr_{_intno_val}(void)")
        kernelCfgC.add("{")
        # Pythonのsortedは安定ソートのため，isrpriのみをキーとすれば
        # 同一優先度のISRは定義順が保たれる（Ruby版のi += 1相当は不要）
        for _idx, _params in enumerate(
                sorted(_isr_params_list, key=lambda p: int(p["isrpri"]))):
            if _idx > 0:
                kernelCfgC.add()
                kernelCfgC.add("\tif (_kernel_sense_lock()) {")
                kernelCfgC.add("\t\t_kernel_unlock_cpu();")
                kernelCfgC.add2("\t}")
            kernelCfgC.add(f"\tLOG_ISR_ENTER({_params['isrid']});")
            kernelCfgC.add(f"\t((ISR)({_params['isr']}))"
                           f"((EXINF)({_params['exinf']}));")
            kernelCfgC.add(f"\tLOG_ISR_LEAVE({_params['isrid']});")
        kernelCfgC.add2("}")

if not OMIT_INITIALIZE_INTERRUPT or USE_INHINIB_TABLE:
    kernelCfgC.add(f"#define TNUM_DEF_INHNO\t{len(cfgData['DEF_INH'])}")
    kernelCfgC.add(f"const uint_t _kernel_tnum_def_inhno = TNUM_DEF_INHNO;")
    kernelCfgC.add()

    if len(cfgData["DEF_INH"]) != 0:
        for _, params in cfgData["DEF_INH"].items():
            if (params["inhatr"] & TA_NONKERNEL) == 0:
                kernelCfgC.add(f"INTHDR_ENTRY({params['inhno']},"
                               f" {params['inhno'].val}, {params['inthdr']})")
        kernelCfgC.add()

        kernelCfgC.add("const INHINIB _kernel_inhinib_table[TNUM_DEF_INHNO] = {")
        for index, (_, params) in enumerate(cfgData["DEF_INH"].items()):
            if index > 0:
                kernelCfgC.add(",")
            if (params["inhatr"] & TA_NONKERNEL) == 0:
                inthdr = (f"(FP)(INT_ENTRY({params['inhno']},"
                          f" {params['inthdr']}))")
            else:
                inthdr = f"(FP)({params['inthdr']})"
            kernelCfgC.append(f"\t{{ ({params['inhno']}), ({params['inhatr']}),"
                              f" {inthdr} }}")
        kernelCfgC.add()
        kernelCfgC.add2("};")
    else:
        kernelCfgC.add2("TOPPERS_EMPTY_LABEL(const INHINIB, _kernel_inhinib_table);")

if not OMIT_INITIALIZE_INTERRUPT or USE_INTINIB_TABLE:
    kernelCfgC.add(f"#define TNUM_CFG_INTNO\t{len(cfgData['CFG_INT'])}")
    kernelCfgC.add(f"const uint_t _kernel_tnum_cfg_intno = TNUM_CFG_INTNO;")
    kernelCfgC.add()

    if len(cfgData["CFG_INT"]) != 0:
        kernelCfgC.add("const INTINIB _kernel_intinib_table[TNUM_CFG_INTNO] = {")
        for index, (_, params) in enumerate(cfgData["CFG_INT"].items()):
            if index > 0:
                kernelCfgC.add(",")
            kernelCfgC.append(f"\t{{ ({params['intno']}), ({params['intatr']}),"
                              f" ({params['intpri']}) }}")
        kernelCfgC.add()
        kernelCfgC.add2("};")
    else:
        kernelCfgC.add2("TOPPERS_EMPTY_LABEL(const INTINIB, _kernel_intinib_table);")

initializeFunctions.append("_kernel_initialize_interrupt();")
