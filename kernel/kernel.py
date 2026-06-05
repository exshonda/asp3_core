# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#       Toyohashi Open Platform for Embedded Real-Time Systems/
#       Advanced Standard Profile Kernel
#
#   $Id: kernel.py (converted from kernel.trb) $
#

#
#  コンフィギュレータのパス2の生成スクリプト
#

timeStampFileName = "kernel_cfg.timestamp"

kernelCfgH = GenFile("kernel_cfg.h")
kernelCfgH.add("/* kernel_cfg.h */")
kernelCfgH.add("#ifndef TOPPERS_KERNEL_CFG_H")
kernelCfgH.add("#define TOPPERS_KERNEL_CFG_H")
kernelCfgH.add()

kernelCfgC = GenFile("kernel_cfg.c")
kernelCfgC.add("/* kernel_cfg.c */")
kernelCfgC.add('#include "kernel/kernel_int.h"')
kernelCfgC.add('#include "kernel_cfg.h"')
kernelCfgC.add()
kernelCfgC.add("#if !(TKERNEL_PRID == 0x0007U && (TKERNEL_PRVER & 0xf000U) == 0x3000U)")
kernelCfgC.add("#error The kernel does not match this configuration file.")
kernelCfgC.add("#endif")
kernelCfgC.add()

kernelCfgC.comment_header("Include Directives")
GenerateIncludes(kernelCfgC)
kernelCfgC.add()

#
#  スタック領域の確保
#
if 'AllocStack' not in dir():
    def AllocStack(stack, size):
        kernelCfgC.add(f"static STK_T {stack}[COUNT_STK_T({size})];")
        return f"ROUND_STK_T({size})"


#
#  カーネルオブジェクトに関する情報の生成（基底クラス）
#
class KernelObject:
    def __init__(self, obj, object_name, obj_s=None):
        self._obj = obj
        self._OBJ = obj.upper()
        self._object = object_name
        self._obj_s = obj if obj_s is None else obj_s
        self._OBJ_S = self._obj_s.upper()
        self._objid = obj + "id"
        self._api = "CRE_" + self._OBJ

    def generate(self):
        g = globals()
        _kernelCfgH = g["kernelCfgH"]
        _kernelCfgC = g["kernelCfgC"]
        _cfgData = g["cfgData"]
        _initFuncs = g["initializeFunctions"]
        _USE_EXTERNAL_ID = g.get("USE_EXTERNAL_ID", False)

        _kernelCfgH.add(f"#define TNUM_{self._OBJ}ID\t{len(_cfgData[self._api])}")

        for _, params in sorted(_cfgData[self._api].items()):
            _kernelCfgH.add(f"#define {params[self._objid]}\t{params[self._objid].val}")
        _kernelCfgH.add()

        if _USE_EXTERNAL_ID:
            for _, params in sorted(_cfgData[self._api].items()):
                _kernelCfgC.add(f"const ID {params[self._objid]}_id"
                                f" = {params[self._objid].val};")
            _kernelCfgC.add()

        _kernelCfgC.add2(f"const ID _kernel_tmax_{self._obj}id"
                         f" = (TMIN_{self._OBJ}ID + TNUM_{self._OBJ}ID - 1);")

        if len(_cfgData[self._api]) > 0:
            for key, params in sorted(_cfgData[self._api].items()):
                self.prepare(key, params)

            if hasattr(self, 'generateData'):
                self.generateData()

            _kernelCfgC.add(f"const {self._OBJ_S}INIB _kernel_{self._obj_s}inib_table"
                            f"[TNUM_{self._OBJ}ID] = {{")
            for index, (key, params) in enumerate(sorted(_cfgData[self._api].items())):
                if index > 0:
                    _kernelCfgC.add(",")
                _kernelCfgC.append(f"\t{{ {self.generateInib(key, params)} }}")
            _kernelCfgC.add()
            _kernelCfgC.add2("};")

            _kernelCfgC.add2(f"{self._OBJ_S}CB _kernel_{self._obj_s}cb_table"
                             f"[TNUM_{self._OBJ}ID];")

            _initFuncs.append(f"_kernel_initialize_{self._object}();")
        else:
            _kernelCfgC.add(f"TOPPERS_EMPTY_LABEL(const {self._OBJ_S}INIB,"
                            f" _kernel_{self._obj_s}inib_table);")
            _kernelCfgC.add2(f"TOPPERS_EMPTY_LABEL({self._OBJ_S}CB,"
                             f" _kernel_{self._obj_s}cb_table);")


#
#  通知ハンドラの生成関数
#
def generateNotifyHandler(key, params, objid):
    g = globals()
    _kernelCfgC = g["kernelCfgC"]

    nfymode = params["nfymode"]
    nfymode1 = nfymode & 0x0f
    nfymode2 = nfymode & ~0x0f
    par2 = params.get("par2")

    _TNFY_HANDLER = g["TNFY_HANDLER"]
    _TNFY_SETVAR = g["TNFY_SETVAR"]
    _TNFY_SETFLG = g["TNFY_SETFLG"]
    _TNFY_SNDDTQ = g["TNFY_SNDDTQ"]
    _TNFY_INCVAR = g["TNFY_INCVAR"]
    _TNFY_ACTTSK = g["TNFY_ACTTSK"]
    _TNFY_WUPTSK = g["TNFY_WUPTSK"]
    _TNFY_SIGSEM = g["TNFY_SIGSEM"]
    _TENFY_SETVAR = g["TENFY_SETVAR"]
    _TENFY_INCVAR = g["TENFY_INCVAR"]
    _TENFY_ACTTSK = g["TENFY_ACTTSK"]
    _TENFY_WUPTSK = g["TENFY_WUPTSK"]
    _TENFY_SIGSEM = g["TENFY_SIGSEM"]
    _TENFY_SETFLG = g["TENFY_SETFLG"]
    _TENFY_SNDDTQ = g["TENFY_SNDDTQ"]

    if (nfymode == _TNFY_HANDLER or nfymode1 == _TNFY_SETVAR
            or nfymode1 == _TNFY_SETFLG or nfymode1 == _TNFY_SNDDTQ):
        numpar = 2
        epar1 = params.get("par3")
        epar2 = params.get("par4")
    else:
        numpar = 1
        epar1 = params.get("par2")
        epar2 = params.get("par3")

    if ((numpar == 2 and par2 is None) or
            (nfymode2 != 0 and epar1 is None) or
            (nfymode2 == _TENFY_SETFLG and epar2 is None)):
        error_sapi(None, params,
                   f"too few parameters for nfymode `{nfymode}'", objid)
    elif ((nfymode2 == 0 and epar1 is not None) or
          (nfymode2 != _TENFY_SETFLG and epar2 is not None)):
        error_sapi(None, params,
                   f"too many parameters for nfymode `{nfymode}'", objid)
    elif nfymode1 == _TNFY_HANDLER and nfymode2 == 0:
        params["nfyhdr"] = f"(NFYHDR)({par2})"
    else:
        if nfymode2 == _TENFY_SETVAR or nfymode2 == _TENFY_INCVAR:
            _kernelCfgC.add2(f"intptr_t *const {params['nfyhdr']}_p_evar ="
                             f" (intptr_t *)({epar1});")
        elif nfymode2 == _TENFY_ACTTSK or nfymode2 == _TENFY_WUPTSK:
            _kernelCfgC.add2(f"const ID {params['nfyhdr']}_etskid = {epar1};")
        elif nfymode2 == _TENFY_SIGSEM:
            _kernelCfgC.add2(f"const ID {params['nfyhdr']}_esemid = {epar1};")
        elif nfymode2 == _TENFY_SETFLG:
            _kernelCfgC.add2(f"const ID {params['nfyhdr']}_eflgid = {epar1};")
        elif nfymode2 == _TENFY_SNDDTQ:
            _kernelCfgC.add2(f"const ID {params['nfyhdr']}_edtqid = {epar1};")

        _kernelCfgC.add("static void")
        _kernelCfgC.add(f"{params['nfyhdr']}(EXINF exinf)")
        _kernelCfgC.add("{")

        if nfymode2 == 0:
            error_code = "(void) "
        else:
            _kernelCfgC.add2("\tER\tercd;")
            error_code = "ercd = "

        if nfymode1 == _TNFY_SETVAR and nfymode2 == 0:
            _kernelCfgC.add(f"\t*((intptr_t *) exinf) = ({par2});")
        elif nfymode1 == _TNFY_INCVAR and nfymode2 == 0:
            _kernelCfgC.add("\t(void) loc_cpu();")
            _kernelCfgC.add("\t*((intptr_t *) exinf) += 1;")
            _kernelCfgC.add("\t(void) unl_cpu();")
        elif nfymode1 == _TNFY_ACTTSK:
            _kernelCfgC.add(f"\t{error_code}act_tsk((ID) exinf);")
        elif nfymode1 == _TNFY_WUPTSK:
            _kernelCfgC.add(f"\t{error_code}wup_tsk((ID) exinf);")
        elif nfymode1 == _TNFY_SIGSEM:
            _kernelCfgC.add(f"\t{error_code}sig_sem((ID) exinf);")
        elif nfymode1 == _TNFY_SETFLG:
            _kernelCfgC.add(f"\t{error_code}set_flg(((ID) exinf), {par2});")
        elif nfymode1 == _TNFY_SNDDTQ:
            _kernelCfgC.add(f"\t{error_code}psnd_dtq(((ID) exinf), {par2});")
        else:
            error_sapi("E_PAR", params, "illegal %%nfymode", objid)

        if nfymode2 != 0:
            _kernelCfgC.add("\tif (ercd != E_OK) {")

            if nfymode2 == _TENFY_SETVAR:
                _kernelCfgC.add(f"\t\t*{params['nfyhdr']}_p_evar = (intptr_t) ercd;")
            elif nfymode2 == _TENFY_INCVAR:
                _kernelCfgC.add("\t\t(void) loc_cpu();")
                _kernelCfgC.add(f"\t\t*{params['nfyhdr']}_p_evar += 1;")
                _kernelCfgC.add("\t\t(void) unl_cpu();")
            elif nfymode2 == _TENFY_ACTTSK:
                _kernelCfgC.add(f"\t\t(void) act_tsk({params['nfyhdr']}_etskid);")
            elif nfymode2 == _TENFY_WUPTSK:
                _kernelCfgC.add(f"\t\t(void) wup_tsk({params['nfyhdr']}_etskid);")
            elif nfymode2 == _TENFY_SIGSEM:
                _kernelCfgC.add(f"\t\t(void) sig_sem({params['nfyhdr']}_esemid);")
            elif nfymode2 == _TENFY_SETFLG:
                _kernelCfgC.add(f"\t\t(void) set_flg({params['nfyhdr']}_eflgid,"
                                f" {epar2});")
            elif nfymode2 == _TENFY_SNDDTQ:
                _kernelCfgC.add(f"\t\t(void) psnd_dtq({params['nfyhdr']}_edtqid,"
                                f" (intptr_t) ercd);")
            else:
                error_sapi("E_PAR", params, "illegal %%nfymode", objid)

            _kernelCfgC.add("\t}")

        _kernelCfgC.add2("}")


#
#  各機能モジュールのコンフィギュレーション
#
initializeFunctions = []
IncludeTrb("kernel/task.py")
IncludeTrb("kernel/semaphore.py")
IncludeTrb("kernel/eventflag.py")
IncludeTrb("kernel/dataqueue.py")
IncludeTrb("kernel/pridataq.py")
IncludeTrb("kernel/mutex.py")
IncludeTrb("kernel/mempfix.py")
IncludeTrb("kernel/cyclic.py")
IncludeTrb("kernel/alarm.py")
IncludeTrb("kernel/interrupt.py")
IncludeTrb("kernel/exception.py")

#
#  非タスクコンテキスト用のスタック領域
#
if not OMIT_ISTACK:
    kernelCfgC.comment_header("Stack Area for Non-task Context")

    if len(cfgData["DEF_ICS"]) == 0:
        if 'DEFAULT_ISTK' not in dir():
            istksz = AllocStack("_kernel_istack", "DEFAULT_ISTKSZ")
            istk = "_kernel_istack"
        else:
            istksz = "DEFAULT_ISTKSZ"
            istk = "DEFAULT_ISTK"
    else:
        for index, (_, params) in enumerate(cfgData["DEF_ICS"].items()):
            params.setdefault("istk", "NULL")

            if params["istksz"] < TARGET_MIN_ISTKSZ:
                error_wrong("E_PAR", params, "istksz", "too small")

            if (params["istk"] != "NULL"
                    and (params["istksz"] & (CHECK_STKSZ_ALIGN - 1)) != 0):
                error_wrong("E_PAR", params, "istksz", "not aligned")

            if index > 0:
                error_ercd("E_OBJ", params, "%apiname is duplicated")

            if params["istk"] == "NULL":
                istksz = AllocStack("_kernel_istack", params["istksz"])
                istk = "_kernel_istack"
            else:
                istksz = f"({params['istksz']})"
                istk = f"(void *)({params['istk']})"

    kernelCfgC.add(f"const size_t _kernel_istksz = {istksz};")
    if not OMIT_ISTK:
        kernelCfgC.add(f"STK_T *const _kernel_istk = {istk};")
    if TOPPERS_ISTKPT:
        kernelCfgC.add(f"STK_T *const _kernel_istkpt"
                       f" = TOPPERS_ISTKPT({istk}, {istksz});")
    kernelCfgC.add()

#
#  タイムイベント管理
#
kernelCfgC.comment_header("Time Event Management")
kernelCfgC.add("TMEVTN   _kernel_tmevt_heap[1 + TNUM_TSKID + TNUM_CYCID + TNUM_ALMID];")
kernelCfgC.add()

#
#  各モジュールの初期化関数
#
kernelCfgC.comment_header("Module Initialization Function")
kernelCfgC.add("void")
kernelCfgC.add("_kernel_initialize_object(void)")
kernelCfgC.add("{")
for func in initializeFunctions:
    kernelCfgC.add(f"\t{func}")
kernelCfgC.add2("}")

#
#  初期化ルーチン機能
#
kernelCfgH.add2(f"#define TNUM_INIRTN\t{len(cfgData['ATT_INI'])}")

kernelCfgC.comment_header("Initialization Routine")

for _, params in cfgData["ATT_INI"].items():
    if params["iniatr"] != TA_NULL:
        error_illegal_sym("E_RSATR", params, "iniatr", "inirtn")

kernelCfgC.add2(f"const uint_t _kernel_tnum_inirtn = TNUM_INIRTN;")

if len(cfgData["ATT_INI"]) > 0:
    kernelCfgC.add("const INIRTNB _kernel_inirtnb_table[TNUM_INIRTN] = {")
    for index, (_, params) in enumerate(cfgData["ATT_INI"].items()):
        if index > 0:
            kernelCfgC.add(",")
        kernelCfgC.append(f"\t{{ (INIRTN)({params['inirtn']}),"
                          f" (EXINF)({params['exinf']}) }}")
    kernelCfgC.add()
    kernelCfgC.add2("};")
else:
    kernelCfgC.add2("TOPPERS_EMPTY_LABEL(const INIRTNB, _kernel_inirtnb_table);")

#
#  終了処理ルーチン機能
#
kernelCfgH.add2(f"#define TNUM_TERRTN\t{len(cfgData['ATT_TER'])}")

kernelCfgC.comment_header("Termination Routine")

for _, params in cfgData["ATT_TER"].items():
    if params["teratr"] != TA_NULL:
        error_illegal_sym("E_RSATR", params, "teratr", "terrtn")

kernelCfgC.add2(f"const uint_t _kernel_tnum_terrtn = TNUM_TERRTN;")

if len(cfgData["ATT_TER"]) > 0:
    kernelCfgC.add("const TERRTNB _kernel_terrtnb_table[TNUM_TERRTN] = {")
    for index, (_, params) in enumerate(reversed(list(cfgData["ATT_TER"].items()))):
        if index > 0:
            kernelCfgC.add(",")
        kernelCfgC.append(f"\t{{ (TERRTN)({params['terrtn']}),"
                          f" (EXINF)({params['exinf']}) }}")
    kernelCfgC.add()
    kernelCfgC.add2("};")
else:
    kernelCfgC.add2("TOPPERS_EMPTY_LABEL(const TERRTNB, _kernel_terrtnb_table);")

#
#  kernel_cfg.hの末尾部分の生成
#
kernelCfgH.add("#endif /* TOPPERS_KERNEL_CFG_H */")
