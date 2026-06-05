# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: mempfix.py (converted from mempfix.trb) $
#

class MempfixObject(KernelObject):
    def __init__(self):
        super().__init__("mpf", "mempfix")

    def prepare(self, key, params):
        g = globals()
        _kernelCfgC = g["kernelCfgC"]
        _TA_TPRI = g["TA_TPRI"]

        params.setdefault("mpf", "NULL")
        params.setdefault("mpfmb", "NULL")

        if (params["mpfatr"] & ~_TA_TPRI) != 0:
            error_illegal_id("E_RSATR", params, "mpfatr", "mpfid")

        if params["blkcnt"] == 0:
            error_illegal_id("E_PAR", params, "blkcnt", "mpfid")

        if params["blksz"] == 0:
            error_illegal_id("E_PAR", params, "blksz", "mpfid")

        if params["mpf"] == "NULL":
            mpf_name = f"_kernel_mpf_{params['mpfid']}"
            _kernelCfgC.add(f"static MPF_T {mpf_name}"
                            f"[{params['blkcnt']} * COUNT_MPF_T({params['blksz']})];"
                            )
            params["mpfinib_mpf"] = mpf_name
        else:
            params["mpfinib_mpf"] = f"(void *)({params['mpf']})"

        if params["mpfmb"] != "NULL":
            error_illegal_id("E_NOSPT", params, "mpfmb", "mpfid")

        mpfmb_name = f"_kernel_mpfmb_{params['mpfid']}"
        _kernelCfgC.add(f"static MPFMB {mpfmb_name}[{params['blkcnt']}];")
        params["mpfinib_mpfmb"] = mpfmb_name

    def generateInib(self, key, params):
        return (f"({params['mpfatr']}),"
                f" ({params['blkcnt']}),"
                f" ROUND_MPF_T({params['blksz']}),"
                f" {params['mpfinib_mpf']},"
                f" {params['mpfinib_mpfmb']}")


kernelCfgC.comment_header("Fixed-sized Memorypool Functions")
MempfixObject().generate()
