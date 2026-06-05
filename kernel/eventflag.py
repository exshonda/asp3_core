# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: eventflag.py (converted from eventflag.trb) $
#

class EventflagObject(KernelObject):
    def __init__(self):
        super().__init__("flg", "eventflag")

    def prepare(self, key, params):
        g = globals()
        _TA_TPRI = g["TA_TPRI"]
        _TA_WMUL = g["TA_WMUL"]
        _TA_CLR = g["TA_CLR"]
        _TBIT_FLGPTN = g["TBIT_FLGPTN"]

        if (params["flgatr"] & ~(_TA_TPRI | _TA_WMUL | _TA_CLR)) != 0:
            error_illegal_id("E_RSATR", params, "flgatr", "flgid")

        if (params["iflgptn"] & ~((1 << _TBIT_FLGPTN) - 1)) != 0:
            error_wrong_id("E_PAR", params, "iflgptn", "flgid", "too large")

    def generateInib(self, key, params):
        return f"({params['flgatr']}), ({params['iflgptn']})"


kernelCfgC.comment_header("Eventflag Functions")
EventflagObject().generate()
