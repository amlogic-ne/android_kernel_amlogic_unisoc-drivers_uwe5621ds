/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Filename : marlin.h
 * Abstract : This file is a implementation for driver of marlin2
 *
 * Authors	: yufeng.yang
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __MARLIN_H__
#define __MARLIN_H__

#include <linux/types.h>
#include "wcn_bus.h"
#ifdef CONFIG_THIRD_PARTY_BOARD
#include <linux/notifier.h>
#endif

#define FALSE								(0)
#define TRUE								(1)

typedef int (*marlin_reset_callback) (void *para);
extern marlin_reset_callback marlin_reset_func;
extern void *marlin_callback_para;
extern unsigned int marlin_get_wcn_xpe_efuse_data(void);
#define WCN_XPE_EFUSE_DATA 1

/* sync with wcn_get_chip_type() and wcn_chip_name */
enum wcn_chip_id_type {
	WCN_CHIP_ID_INVALID,
	WCN_CHIP_ID_AA,
	WCN_CHIP_ID_AB,
	WCN_CHIP_ID_AC,
	WCN_CHIP_ID_AD,
	/* WCN_CHIP_ID_MAX is the last one */
	WCN_CHIP_ID_MAX,
};

enum wcn_clock_type {
	WCN_CLOCK_TYPE_UNKNOWN,
	WCN_CLOCK_TYPE_TCXO,
	WCN_CLOCK_TYPE_TSX,
};

enum wcn_clock_mode {
	WCN_CLOCK_MODE_UNKNOWN,
	WCN_CLOCK_MODE_XO,
	WCN_CLOCK_MODE_BUFFER,
};

enum marlin_wake_host_en {
	BT_WAKE_HOST = 0,
	WL_WAKE_HOST,
	WL_NO_WAKE_HOST
};

enum marlin_cp2_status {
	MARLIN_CP2_STS_READY = 0,
	MARLIN_CP2_STS_ASSERTED = 1,
};

enum wcn_clock_type wcn_get_xtal_26m_clk_type(void);
enum wcn_clock_mode wcn_get_xtal_26m_clk_mode(void);
const char *wcn_get_chip_name(void);
enum wcn_chip_id_type wcn_get_chip_type(void);
enum wcn_chip_model wcn_get_chip_model(void);

void marlin_power_off(enum wcn_sub_sys subsys);
int marlin_get_power(void);
int marlin_get_set_power_status(void);
int marlin_set_wakeup(enum wcn_sub_sys subsys);
int marlin_set_sleep(enum wcn_sub_sys subsys, bool enable);
int marlin_reset_reg(void);
int start_marlin(enum wcn_sub_sys subsys);
int stop_marlin(enum wcn_sub_sys subsys);
void marlin_schedule_download_wq(void);
int open_power_ctl(void);
bool marlin_get_download_status(void);
void marlin_set_download_status(int f);
void marlin_chip_en(bool enable, bool reset);
int marlin_get_module_status(void);
int marlin_get_module_status_changed(void);
int wcn_get_module_status_changed(void);
void wcn_set_module_status_changed(bool status);
int marlin_reset_register_notify(void *callback_func, void *para);
int marlin_reset_unregister_notify(void);
int is_first_power_on(enum wcn_sub_sys subsys);
int cali_ini_need_download(enum wcn_sub_sys subsys);
const char *strno(enum wcn_sub_sys subsys);
void wcn_chip_power_on(void);
void wcn_chip_power_off(void);
int marlin_reset_notify_call(enum marlin_cp2_status sts);
void mdbg_assert_interface(char *str);

unsigned long marlin_get_power_state(void);
unsigned char marlin_get_bt_wl_wake_host_en(void);
#ifdef CONFIG_THIRD_PARTY_BOARD
unsigned int marlin_get_wcn_chipid(void);
int marlin_reset_callback_register(u32 subsys, struct notifier_block *nb);
void marlin_reset_callback_unregister(u32 subsys, struct notifier_block *nb);
#endif

#ifdef CONFIG_WCN_USB
void marlin_schedule_usb_hotplug(void);
int marlin_probe_status(void);
int marlin_get_usb_hotplug_status(void);
void marlin_set_usb_hotplug_status(int status);
void marlin_set_usb_reset_status(int status);
int marlin_get_usb_reset_status(void);
extern struct completion wcn_usb_rst_fdl_done;
#endif

#endif
