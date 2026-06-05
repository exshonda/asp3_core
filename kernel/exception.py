# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#   $Id: exception.py (converted from exception.trb) $
#

kernelCfgC.comment_header("CPU Exception Management Functions")

if 'EXCNO_DEFEXC_VALID' not in dir():
    EXCNO_DEFEXC_VALID = EXCNO_VALID

for _, params in cfgData["DEF_EXC"].items():
    if params["excno"] not in EXCNO_DEFEXC_VALID:
        error_illegal("E_PAR", params, "excno")

    if (params["excatr"] & ~TARGET_EXCATR) != 0:
        error_illegal_sym("E_RSATR", params, "excatr", "excno")

    if 'TargetCheckDefExc' in dir():
        TargetCheckDefExc(params)

if not OMIT_INITIALIZE_EXCEPTION:
    kernelCfgC.add(f"#define TNUM_DEF_EXCNO\t{len(cfgData['DEF_EXC'])}")
    kernelCfgC.add(f"const uint_t _kernel_tnum_def_excno = TNUM_DEF_EXCNO;")
    kernelCfgC.add()

    if len(cfgData["DEF_EXC"]) != 0:
        for _, params in cfgData["DEF_EXC"].items():
            kernelCfgC.add(f"EXCHDR_ENTRY({params['excno']},"
                           f" {params['excno'].val}, {params['exchdr']})")
        kernelCfgC.add()

        kernelCfgC.add("const EXCINIB _kernel_excinib_table[TNUM_DEF_EXCNO] = {")
        for index, (_, params) in enumerate(cfgData["DEF_EXC"].items()):
            if index > 0:
                kernelCfgC.add(",")
            kernelCfgC.append(f"\t{{ ({params['excno']}), ({params['excatr']}),"
                              f" (FP)(EXC_ENTRY({params['excno']},"
                              f" {params['exchdr']})) }}")
        kernelCfgC.add()
        kernelCfgC.add2("};")
    else:
        kernelCfgC.add2("TOPPERS_EMPTY_LABEL(const EXCINIB, _kernel_excinib_table);")

initializeFunctions.append("_kernel_initialize_exception();")
