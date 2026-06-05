# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: mutex.py (converted from mutex.trb) $
#

class MutexObject(KernelObject):
    def __init__(self):
        super().__init__("mtx", "mutex")

    def prepare(self, key, params):
        g = globals()
        _TA_NULL = g["TA_NULL"]
        _TA_TPRI = g["TA_TPRI"]
        _TA_CEILING = g["TA_CEILING"]
        _TMIN_TPRI = g["TMIN_TPRI"]
        _TMAX_TPRI = g["TMAX_TPRI"]

        if not (params["mtxatr"] == _TA_NULL or params["mtxatr"] == _TA_TPRI
                or params["mtxatr"] == _TA_CEILING):
            error_illegal_id("E_RSATR", params, "mtxatr", "mtxid")

        if params["mtxatr"] == _TA_CEILING:
            if "ceilpri" not in params:
                error_sapi(None, params, "ceilpri must be specified", "mtxid")
            elif not (_TMIN_TPRI <= params["ceilpri"] and params["ceilpri"] <= _TMAX_TPRI):
                error_illegal_id("E_PAR", params, "ceilpri", "mtxid")
        else:
            if "ceilpri" in params:
                warning_api(params, "%%ceilpri is ignored in %apiname of %mtxid")
            params["ceilpri"] = 0

    def generateInib(self, key, params):
        return f"({params['mtxatr']}), INT_PRIORITY({params['ceilpri']})"


kernelCfgC.comment_header("Mutex Functions")
MutexObject().generate()
