/* This file is generated from core_rename.def by genrename. */

/* This file is included only when core_rename.h has been included. */
#ifdef TOPPERS_CORE_RENAME_H
#undef TOPPERS_CORE_RENAME_H

/*
 *  kernel_cfg.c
 */
#undef inh_table
#undef intcfg_table
#undef exc_table

/*
 *  core_support.S
 */
#undef dispatch
#undef start_dispatch
#undef exit_and_dispatch
#undef call_exit_kernel
#undef start_r
#undef core_int_entry
#undef core_exc_entry

/*
 *  core_kernel_impl.c
 */
#undef excpt_nest_count
#undef core_initialize
#undef core_terminate
#undef xlog_sys
#undef default_int_handler
#undef default_exc_handler

/*
 *  core_kernel_impl.h
 */
#undef lock_cpu
#undef unlock_cpu
#undef sense_lock

/*
 *  mtimer.c
 */
#undef target_hrt_initialize
#undef target_hrt_terminate
#undef target_hrt_handler

/*
 *  plic_kernel_impl.c
 */
#undef plic_context_initialize
#undef plic_global_initialize
#undef plic_initialize_interrupt


#endif /* TOPPERS_CORE_RENAME_H */
