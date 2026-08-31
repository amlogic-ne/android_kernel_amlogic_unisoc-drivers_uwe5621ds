/*
 * This function include:
 * 1. register reset callback
 * 2. notify BT FM WIFI GNSS CP2 Assert
 */

#include <linux/debug_locks.h>
#include <linux/version.h>
#if KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE
#include <linux/sched/debug.h>
#endif
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/vt_kern.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/ratelimit.h>

#include "wcn_glb.h"

ATOMIC_NOTIFIER_HEAD(wcn_reset_notifier_list);
EXPORT_SYMBOL_GPL(wcn_reset_notifier_list);

void wcn_reset_cp2(void)
{
#ifndef CONFIG_WCN_USB
	wcn_chip_power_off();
#endif
	atomic_notifier_call_chain(&wcn_reset_notifier_list, 0, NULL);
}
EXPORT_SYMBOL_GPL(wcn_reset_cp2);
