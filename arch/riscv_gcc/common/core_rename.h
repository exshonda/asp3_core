/* This file is generated from core_rename.def by genrename. */

#ifndef TOPPERS_CORE_RENAME_H
#define TOPPERS_CORE_RENAME_H

/*
 *  kernel_cfg.c
 */
#define inh_table					_kernel_inh_table
#define intcfg_table				_kernel_intcfg_table
#define exc_table					_kernel_exc_table

/*
 *  core_support.S
 */
#define dispatch					_kernel_dispatch
#define start_dispatch				_kernel_start_dispatch
#define exit_and_dispatch			_kernel_exit_and_dispatch
#define call_exit_kernel			_kernel_call_exit_kernel
#define start_r						_kernel_start_r
#define core_int_entry				_kernel_core_int_entry
#define core_exc_entry				_kernel_core_exc_entry

/*
 *  core_kernel_impl.c
 */
#define excpt_nest_count			_kernel_excpt_nest_count
#define core_initialize				_kernel_core_initialize
#define core_terminate				_kernel_core_terminate
#define xlog_sys					_kernel_xlog_sys
#define default_int_handler			_kernel_default_int_handler
#define default_exc_handler			_kernel_default_exc_handler

/*
 *  core_kernel_impl.h
 */
#define lock_cpu					_kernel_lock_cpu
#define unlock_cpu					_kernel_unlock_cpu
#define sense_lock					_kernel_sense_lock

/*
 *  plic_kernel_impl.c
 */
#define plic_context_initialize		_kernel_plic_context_initialize
#define plic_global_initialize		_kernel_plic_global_initialize
#define plic_initialize_interrupt	_kernel_plic_initialize_interrupt


#endif /* TOPPERS_CORE_RENAME_H */
