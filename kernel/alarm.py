# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: alarm.py (converted from alarm.trb) $
#

class AlarmObject(KernelObject):
    def __init__(self):
        super().__init__("alm", "alarm")

    def prepare(self, key, params):
        g = globals()
        _TA_NULL = g["TA_NULL"]

        if params["almatr"] != _TA_NULL:
            error_illegal_id("E_RSATR", params, "almatr", "almid")

        params["nfyhdr"] = f"_kernel_almhdr_{params['almid']}"
        generateNotifyHandler(key, params, "almid")

    def generateInib(self, key, params):
        return (f"({params['almatr']}),"
                f" (intptr_t)({params['par1']}),"
                f" {params['nfyhdr']}")


kernelCfgC.comment_header("Alarm Notification Functions")
AlarmObject().generate()
