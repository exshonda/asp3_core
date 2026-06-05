# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: dataqueue.py (converted from dataqueue.trb) $
#

class DataqueueObject(KernelObject):
    def __init__(self):
        super().__init__("dtq", "dataqueue")

    def prepare(self, key, params):
        g = globals()
        _kernelCfgC = g["kernelCfgC"]
        _TA_TPRI = g["TA_TPRI"]

        params.setdefault("dtqmb", "NULL")

        if (params["dtqatr"] & ~_TA_TPRI) != 0:
            error_illegal_id("E_RSATR", params, "dtqatr", "dtqid")

        if params["dtqmb"] != "NULL":
            error_illegal_id("E_NOSPT", params, "dtqmb", "dtqid")

        if params["dtqcnt"] > 0:
            dtqmb_name = f"_kernel_dtqmb_{params['dtqid']}"
            _kernelCfgC.add(f"static DTQMB {dtqmb_name}[{params['dtqcnt']}];")
            params["dtqinib_dtqmb"] = dtqmb_name
        else:
            params["dtqinib_dtqmb"] = "NULL"

    def generateInib(self, key, params):
        return (f"({params['dtqatr']}),"
                f" ({params['dtqcnt']}),"
                f" {params['dtqinib_dtqmb']}")


kernelCfgC.comment_header("Dataqueue Functions")
DataqueueObject().generate()
