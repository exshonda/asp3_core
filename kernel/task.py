# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: task.py (converted from task.trb) $
#

#
#  タスク管理モジュールの生成スクリプト
#

class TaskObject(KernelObject):
    def __init__(self):
        super().__init__("tsk", "task", "t")

    def prepare(self, key, params):
        g = globals()
        _kernelCfgC = g["kernelCfgC"]
        _TA_ACT = g["TA_ACT"]
        _TA_NOACTQUE = g["TA_NOACTQUE"]
        _TARGET_TSKATR = g["TARGET_TSKATR"]
        _TMIN_TPRI = g["TMIN_TPRI"]
        _TMAX_TPRI = g["TMAX_TPRI"]
        _TARGET_MIN_STKSZ = g["TARGET_MIN_STKSZ"]
        _CHECK_STKSZ_ALIGN = g["CHECK_STKSZ_ALIGN"]
        _USE_TSKINICTXB = g["USE_TSKINICTXB"]

        params.setdefault("stk", "NULL")

        if (params["tskatr"] & ~(_TA_ACT | _TA_NOACTQUE | _TARGET_TSKATR)) != 0:
            error_illegal_id("E_RSATR", params, "tskatr", "tskid")

        if not (_TMIN_TPRI <= params["itskpri"] and params["itskpri"] <= _TMAX_TPRI):
            error_illegal_id("E_PAR", params, "itskpri", "tskid")

        if params["stksz"] < _TARGET_MIN_STKSZ:
            error_wrong_id("E_PAR", params, "stksz", "tskid", "too small")

        if params["stk"] == "NULL":
            stk_name = f"_kernel_stack_{params['tskid']}"
            params["tinib_stksz"] = AllocStack(stk_name, params["stksz"])
            params["tinib_stk"] = stk_name
        else:
            if (params["stksz"] & (_CHECK_STKSZ_ALIGN - 1)) != 0:
                error_wrong_id("E_PAR", params, "stksz", "tskid", "not aligned")
            params["tinib_stksz"] = params["stksz"]
            params["tinib_stk"] = f"(void *)({params['stk']})"

        if 'TargetTaskPrepare' in dir():
            TargetTaskPrepare(key, params)

    def generateInib(self, key, params):
        g = globals()
        _USE_TSKINICTXB = g["USE_TSKINICTXB"]
        if _USE_TSKINICTXB:
            tskinictxb = GenerateTskinictxb(key, params)
        else:
            tskinictxb = f"{params['tinib_stksz']}, {params['tinib_stk']}"
        return (f"({params['tskatr']}), (EXINF)({params['exinf']}),"
                f" (TASK)({params['task']}),"
                f" INT_PRIORITY({params['itskpri']}),"
                f" {tskinictxb}")


if len(cfgData["CRE_TSK"]) == 0:
    error("no task is registered")

kernelCfgC.comment_header("Task Management Functions")
TaskObject().generate()

kernelCfgC.add("const ID _kernel_torder_table[TNUM_TSKID] = { ")
kernelCfgC.append("\t")
for index, (_, params) in enumerate(cfgData["CRE_TSK"].items()):
    if index > 0:
        kernelCfgC.append(", ")
    kernelCfgC.append(f"{params['tskid']}")
kernelCfgC.add()
kernelCfgC.add2("};")
