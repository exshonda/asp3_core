# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: cyclic.py (converted from cyclic.trb) $
#

class CyclicObject(KernelObject):
    def __init__(self):
        super().__init__("cyc", "cyclic")

    def prepare(self, key, params):
        g = globals()
        _TA_STA = g["TA_STA"]
        _TMAX_RELTIM = g["TMAX_RELTIM"]

        if (params["cycatr"] & ~_TA_STA) != 0:
            error_illegal_id("E_RSATR", params, "cycatr", "cycid")

        if not (0 < params["cyctim"] and params["cyctim"] <= _TMAX_RELTIM):
            error_illegal_id("E_PAR", params, "cyctim", "cycid")

        if not (0 <= params["cycphs"] and params["cycphs"] <= _TMAX_RELTIM):
            error_illegal_id("E_PAR", params, "cycphs", "cycid")

        params["nfyhdr"] = f"_kernel_cychdr_{params['cycid']}"
        generateNotifyHandler(key, params, "cycid")

    def generateInib(self, key, params):
        return (f"({params['cycatr']}),"
                f" (intptr_t)({params['par1']}),"
                f" {params['nfyhdr']},"
                f" ({params['cyctim']}),"
                f" ({params['cycphs']})")


kernelCfgC.comment_header("Cyclic Notification Functions")
CyclicObject().generate()
