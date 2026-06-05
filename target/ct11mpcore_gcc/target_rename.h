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
#define target_ovrtimer_initialize	_kernel_target_ovrtimer_initialize
#define target_ovrtimer_terminate	_kernel_target_ovrtimer_terminate
#define target_ovrtimer_start		_kernel_target_ovrtimer_start
#define target_ovrtimer_stop		_kernel_target_ovrtimer_stop
#define target_ovrtimer_get_current	_kernel_target_ovrtimer_get_current
#define target_ovrtimer_handler		_kernel_target_ovrtimer_handler


#include "core_rename.h"

#endif /* TOPPERS_TARGET_RENAME_H */
