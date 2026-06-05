/* This file is generated from posix_rename.def by genrename. */

/* This file is included only when posix_rename.h has been included. */
#ifdef TOPPERS_POSIX_RENAME_H
#undef TOPPERS_POSIX_RENAME_H

/*
 *  posix_kernel_impl.c
 */
#undef sigmask_intlock
#undef sigmask_cpulock
#undef idle_thread
#undef excpt_nest_count
#undef int_entry
#undef dispatch
#undef exit_and_dispatch
#undef start_dispatch_task
#undef exc_sense_intmask
#undef exc_entry_generic
#undef define_exc
#undef call_exit_kernel
#undef posix_initialize

/*
 *  posix_kernel_impl.h
 */
#undef lock_cpu
#undef unlock_cpu
#undef sense_lock

/*
 *  interrupt_sim.c
 */
#undef intrcb_table
#undef scheduled_thread
#undef disable_int
#undef enable_int
#undef clear_int
#undef raise_int
#undef raise_int_main
#undef probe_int
#undef t_set_ipm
#undef t_get_ipm
#undef return_intr
#undef initialize_interrupt

/*
 *  thread_ctrl.c
 */
#undef main_thread
#undef thrcb_key
#undef init_thrcb
#undef suspend_thread
#undef exit_thread
#undef terminate_thread
#undef preempt_thread
#undef start_dispatch_thread
#undef initialize_thread_ctrl

/*
 *  posix_timer.c, posix_timer_itimer.c
 */
#undef target_timer_initialize
#undef target_timer_terminate
#undef target_hrt_get_current
#undef target_hrt_set_event
#undef target_hrt_raise_event
#undef target_ovrtimer_start
#undef target_ovrtimer_stop
#undef target_ovrtimer_get_current


#endif /* TOPPERS_POSIX_RENAME_H */
