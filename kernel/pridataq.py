# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: pridataq.py (converted from pridataq.trb) $
#

class PridataqObject(KernelObject):
    def __init__(self):
        super().__init__("pdq", "pridataq")

    def prepare(self, key, params):
        g = globals()
        _kernelCfgC = g["kernelCfgC"]
        _TA_TPRI = g["TA_TPRI"]
        _TMIN_DPRI = g["TMIN_DPRI"]
        _TMAX_DPRI = g["TMAX_DPRI"]

        params.setdefault("pdqmb", "NULL")

        if (params["pdqatr"] & ~_TA_TPRI) != 0:
            error_illegal_id("E_RSATR", params, "pdqatr", "pdqid")

        if not (_TMIN_DPRI <= params["maxdpri"] and params["maxdpri"] <= _TMAX_DPRI):
            error_illegal_id("E_PAR", params, "maxdpri", "pdqid")

        if params["pdqmb"] != "NULL":
            error_illegal_id("E_NOSPT", params, "pdqmb", "pdqid")

        if params["pdqcnt"] > 0:
            pdqmb_name = f"_kernel_pdqmb_{params['pdqid']}"
            _kernelCfgC.add(f"static PDQMB {pdqmb_name}[{params['pdqcnt']}];")
            params["pdqinib_pdqmb"] = pdqmb_name
        else:
            params["pdqinib_pdqmb"] = "NULL"

    def generateInib(self, key, params):
        return (f"({params['pdqatr']}),"
                f" ({params['pdqcnt']}),"
                f" ({params['maxdpri']}),"
                f" {params['pdqinib_pdqmb']}")


kernelCfgC.comment_header("Priority Dataqueue Functions")
PridataqObject().generate()
