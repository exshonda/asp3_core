# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: semaphore.py (converted from semaphore.trb) $
#

#
#  セマフォ機能の生成スクリプト
#

class SemaphoreObject(KernelObject):
    def __init__(self):
        super().__init__("sem", "semaphore")

    def prepare(self, key, params):
        g = globals()
        _TA_TPRI = g["TA_TPRI"]
        _TMAX_MAXSEM = g["TMAX_MAXSEM"]

        if (params["sematr"] & ~_TA_TPRI) != 0:
            error_illegal_id("E_RSATR", params, "sematr", "semid")

        if not (1 <= params["maxsem"] and params["maxsem"] <= _TMAX_MAXSEM):
            error_illegal_id("E_PAR", params, "maxsem", "semid")

        if not (0 <= params["isemcnt"] and params["isemcnt"] <= params["maxsem"]):
            error_wrong_id("E_PAR", params, "isemcnt", "semid", "too large")

    def generateInib(self, key, params):
        return (f"({params['sematr']}),"
                f" ({params['isemcnt']}),"
                f" ({params['maxsem']})")


kernelCfgC.comment_header("Semaphore Functions")
SemaphoreObject().generate()
