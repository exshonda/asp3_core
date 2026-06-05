/* This file is generated from posix_rename.def by genrename. */

#ifndef TOPPERS_POSIX_RENAME_H
#define TOPPERS_POSIX_RENAME_H

/*
 *  posix_kernel_impl.c
 */
#define sigmask_intlock				_kernel_sigmask_intlock
#define sigmask_cpulock				_kernel_sigmask_cpulock
#define idle_thread					_kernel_idle_thread
#define excpt_nest_count			_kernel_excpt_nest_count
#define int_entry					_kernel_int_entry
#define dispatch					_kernel_dispatch
#define exit_and_dispatch			_kernel_exit_and_dispatch
#define start_dispatch_task			_kernel_start_dispatch_task
#define exc_sense_intmask			_kernel_exc_sense_intmask
#define exc_entry_generic			_kernel_exc_entry_generic
#define define_exc					_kernel_define_exc
#define call_exit_kernel			_kernel_call_exit_kernel
#define posix_initialize			_kernel_posix_initialize

/*
 *  posix_kernel_impl.h
 */
#define lock_cpu					_kernel_lock_cpu
#define unlock_cpu					_kernel_unlock_cpu
#define sense_lock					_kernel_sense_lock

/*
 *  interrupt_sim.c
 */
#define intrcb_table				_kernel_intrcb_table
#define scheduled_thread			_kernel_scheduled_thread
#define disable_int					_kernel_disable_int
#define enable_int					_kernel_enable_int
#define clear_int					_kernel_clear_int
#define raise_int					_kernel_raise_int
#define raise_int_main				_kernel_raise_int_main
#define probe_int					_kernel_probe_int
#define t_set_ipm					_kernel_t_set_ipm
#define t_get_ipm					_kernel_t_get_ipm
#define return_intr					_kernel_return_intr
#define initialize_interrupt		_kernel_initialize_interrupt

/*
 *  thread_ctrl.c
 */
#define main_thread					_kernel_main_thread
#define thrcb_key					_kernel_thrcb_key
#define init_thrcb					_kernel_init_thrcb
#define suspend_thread				_kernel_suspend_thread
#define exit_thread					_kernel_exit_thread
#define terminate_thread			_kernel_terminate_thread
#define preempt_thread				_kernel_preempt_thread
#define start_dispatch_thread		_kernel_start_dispatch_thread
#define initialize_thread_ctrl		_kernel_initialize_thread_ctrl

/*
 *  posix_timer.c, posix_timer_itimer.c
 */
#define target_timer_initialize		_kernel_target_timer_initialize
#define target_timer_terminate		_kernel_target_timer_terminate
#define target_hrt_get_current		_kernel_target_hrt_get_current
#define target_hrt_set_event		_kernel_target_hrt_set_event
#define target_hrt_raise_event		_kernel_target_hrt_raise_event
#define target_ovrtimer_start		_kernel_target_ovrtimer_start
#define target_ovrtimer_stop		_kernel_target_ovrtimer_stop
#define target_ovrtimer_get_current	_kernel_target_ovrtimer_get_current


#endif /* TOPPERS_POSIX_RENAME_H */
