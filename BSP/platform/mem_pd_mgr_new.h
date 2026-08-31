/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Filename : sdio_dev.h
 * Abstract : This file is a implementation for itm sipc command/event function
 *
 * Authors	: QI.SUN
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
#ifndef __MEM_PD_MGR__
#define __MEM_PD_MGR__

#include "marlin_platform.h"
#include "wcn_ap_cp_sync.h"

#define MEM_PD_MGR_HEADER "[mem_pd_new]"

#define PD_ERR(fmt, args...)	\
	pr_err(MEM_PD_MGR_HEADER fmt "\n", ## args)
#define PD_INFO(fmt, args...)	\
	pr_info(MEM_PD_MGR_HEADER fmt "\n", ## args)
#define PD_DBG(fmt, args...)	\
	pr_debug(MEM_PD_MGR_HEADER fmt "\n", ## args)

/* cp2 create thread status */
#define THREAD_CREATE 1
#define THREAD_DELETE 0

struct mem_pd_debug_t {
	unsigned int mem_pd_open_bt;
	unsigned int mem_pd_open_wifi;
	unsigned int mem_pd_close_bt;
	unsigned int mem_pd_close_wifi;
};


#if 0
enum MEM_CMD_2CP {
	NO_CMD = 0,
	CMD_WIFI_DL,
	CMD_BT_DL,
	CMD_WIFI_SHUTDOWN,
	CMD_BT_SHUTDOWN,
	CMD_SAVE_BIN,
	CMD_WIFI_POWERON,
	CMD_BT_POWERON,
};
enum MEM_WAIT_CP {
	WAIT_SAVEBIN = 0,
	WAIT_WIFI_OPEN,
	WAIT_WIFI_CLOSE,
	WAIT_BT_OPEN,
	WAIT_BT_CLOSE,
	WAIT_WIFI_POWERON,
	WAIT_BT_POWERON
};
#endif

#ifdef CONFIG_UMW2653
struct mem_pd_t {
	struct mutex mem_pd_lock;
	/* struct completion wifi_open_completion;
	 * struct completion wifi_cls_cpl;
	 * struct completion bt_open_completion;
	 * struct completion bt_close_completion;
	 * struct completion save_bin_completion;
	 */
	unsigned int wifi_state;
	unsigned int bt_state;
	unsigned int cp_version;
	unsigned int bin_save_done;
	char *wifi_mem[WIFI_SAVE_BIN_NUM];
	char *bt_mem[BT_SAVE_BIN_NUM];
	char *wifi_clear[WIFI_SAVE_BIN_NUM];
	char *bt_clear[BT_SAVE_BIN_NUM];
	struct mem_pd_debug_t mem_pd_debug;
	unsigned int cp_mem_all_off;
};

struct mem_pd_meminfo_t {
	unsigned int wifi_begin_addr[WIFI_SAVE_BIN_NUM];
	unsigned int wifi_end_addr[WIFI_SAVE_BIN_NUM];
	unsigned int wifi_size[WIFI_SAVE_BIN_NUM];
	unsigned int wifi_used_segtions;
	unsigned int bt_begin_addr[BT_SAVE_BIN_NUM];
	unsigned int bt_end_addr[BT_SAVE_BIN_NUM];
	unsigned int bt_size[BT_SAVE_BIN_NUM];
	unsigned int bt_used_segtions;
	/* marlin3e memory shutdown was controlled by cp;*/
	unsigned int chip_id;
};
#endif

unsigned int mem_pd_wifi_state(void);
int mem_pd_poweroff_deinit(void);
int mem_pd_mgr(int subsys, int val);
int mem_pd_save_bin(void);
int mem_pd_init(void);
int mem_pd_exit(void);
unsigned int marlin_get_wcn_chipid(void);

#endif