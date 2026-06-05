/* This file is generated from target_rename.def by genrename. */

#ifndef TOPPERS_TARGET_RENAME_H
#define TOPPERS_TARGET_RENAME_H

/*
 *  target_kernel_impl.c
 */
#define arm_tnum_memory_area		_kernel_arm_tnum_memory_area
#define arm_memory_area				_kernel_arm_memory_area
#define target_initialize			_kernel_target_initialize
#define target_exit					_kernel_target_exit

/*
 *  target_timer.c
 */
#define simtim_target_raise_hrt_int	_kernel_simtim_target_raise_hrt_int
#define simtim_target_raise_ovr_int	_kernel_simtim_target_raise_ovr_int
#define simtim_target_clear_ovr_int	_kernel_simtim_target_clear_ovr_int

/*
 *  sim_timer.c
 */
#define target_timer_initialize		_kernel_target_timer_initialize
#define target_timer_terminate		_kernel_target_timer_terminate
#define target_hrt_get_current		_kernel_target_hrt_get_current
#define target_hrt_set_event		_kernel_target_hrt_set_event
#define target_hrt_clear_event		_kernel_target_hrt_clear_event
#define target_hrt_raise_event		_kernel_target_hrt_raise_event
#define target_hrt_handler			_kernel_target_hrt_handler
#define target_ovrtimer_start		_kernel_target_ovrtimer_start
#define target_ovrtimer_stop		_kernel_target_ovrtimer_stop
#define target_ovrtimer_get_current	_kernel_target_ovrtimer_get_current
#define target_ovrtimer_handler		_kernel_target_ovrtimer_handler
#define target_custom_idle			_kernel_target_custom_idle
#define simtim_advance				_kernel_simtim_advance
#define simtim_add					_kernel_simtim_add
#define hook_hrt_set_event			_kernel_hook_hrt_set_event
#define hook_hrt_clear_event		_kernel_hook_hrt_clear_event
#define hook_hrt_raise_event		_kernel_hook_hrt_raise_event


#include "core_rename.h"

#endif /* TOPPERS_TARGET_RENAME_H */
