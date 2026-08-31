/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Filename : slp_mgr.c
 * Abstract : This file is a implementation for  sleep manager
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
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include "marlin_platform.h"
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
//#include <linux/wakelock.h>
#include <linux/delay.h>

#include "wcn_bus.h"
#include "mem_pd_mgr_new.h"
#include "wcn_glb_reg.h"


#define MEM_PD_ERR -3
#define CP_NO_MEM_PD_TIMEROUT 2000
#define CP_TIMEROUT 30000
/* time out in waiting wifi to come up */


struct mem_pd_t mem_pd;
struct mem_pd_meminfo_t mem_info_cp;


/* return 0, no download ini; return 1, need download ini */
unsigned int mem_pd_wifi_state(void)
{
	unsigned int ret = 0;

	if (mem_pd.cp_version)
		return 0;
	ret = mem_pd.wifi_state;

	return ret;
}

/*CP control memory shutdown,mem_pd_power_switch was deleted*/
/*********************************************************/
static int mem_pd_ap_int_cp_req(enum AP_INT_CP_REQ_ID ap_int_cp_req_id)
{
	unsigned int ap_int_cp_value;
	unsigned int aon_soft_irq = 2;

	PD_INFO("%s start,ap_int_cp_req_id	is 0x%x.",
		  __func__, ap_int_cp_req_id);
	/*wifi download ok,and then send wifi open req irq to cp*/
	sprdwcn_bus_reg_read(SYNC_AP_INT_CP_REQ_ADDR, &ap_int_cp_value, 4);
	ap_int_cp_value |= ap_int_cp_req_id;
	sprdwcn_bus_reg_write(SYNC_AP_INT_CP_REQ_ADDR, &ap_int_cp_value, 4);
	/*AP use aon intc softirq to int cp*/
	sprdwcn_bus_reg_write(REG_AON_INTC_SOFTIRQ, &aon_soft_irq, 4);
	PD_INFO("%s end.", __func__);

	return 0;
}
/*BIT0*/
static int mem_pd_inform_cp_save_cp_bin_ok(void)/*not used sdio int*/
{
	mem_pd_ap_int_cp_req(BIT_AP_INT_CP_SAVE_BIN_REQ);
	PD_INFO("%s\n", __func__);
	return 0;
}
/*BIT1*/
static int mem_pd_inform_cp_wifi_poweron(void)
{
	mem_pd_ap_int_cp_req(BIT_AP_INT_CP_WIFI_POWER_ON_REQ);
	PD_INFO("%s\n", __func__);
	return 0;
}
/*BIT2*/
static int mem_pd_inform_cp_wifi_open(void)
{
	mem_pd_ap_int_cp_req(BIT_AP_INT_CP_WIFI_OPEN_REQ);
	PD_INFO("%s\n", __func__);
	return 0;
}
/*BIT3*/
static int mem_pd_inform_cp_wifi_close(void)
{
	mem_pd_ap_int_cp_req(BIT_AP_INT_CP_WIFI_CLOSE_REQ);
	PD_INFO("%s\n", __func__);
	return 0;
}
/*BIT4*/
static int mem_pd_inform_cp_bt_poweron(void)
{
	mem_pd_ap_int_cp_req(BIT_AP_INT_CP_BT_POWER_ON_REQ);
	PD_INFO("%s\n", __func__);
	return 0;
}
/*BIT5*/
static int mem_pd_inform_cp_bt_open(void)
{
	mem_pd_ap_int_cp_req(BIT_AP_INT_CP_BT_OPEN_REQ);
	PD_INFO("%s\n", __func__);
	return 0;
}
/*BIT6*/
static int mem_pd_inform_cp_bt_close(void)
{
	mem_pd_ap_int_cp_req(BIT_AP_INT_CP_BT_CLOSE_REQ);
	PD_INFO("%s\n", __func__);
	return 0;
}
/*********************************************************/

static int mem_pd_wait_for_cp_status(enum CP_TO_AP_ACK_ID cp_to_ack_id)
{
	int count = 0;
	unsigned int cp_ack_ap_value;

	PD_INFO("%s start,cp_to_ack_id is 0x%x.", __func__, cp_to_ack_id);
	do {
		count++;
		sprdwcn_bus_reg_read(SYNC_CP_ACK_AP_VALUE_ADDR,
			&cp_ack_ap_value, 4);
		if (cp_ack_ap_value & cp_to_ack_id) {
			/*clear the bit*/
			cp_ack_ap_value &= ~(cp_to_ack_id);
			sprdwcn_bus_reg_write(SYNC_CP_ACK_AP_VALUE_ADDR,
					       &cp_ack_ap_value, 4);
			PD_INFO("cp has finish status , cp_to_ack_id is 0x%x ",
				  cp_to_ack_id);
			PD_INFO("%s end", __func__);
			return 0;
		}
		if (count > 10)
			count = 0;
		msleep(20);/*20ms*/
	} while (count);
	PD_INFO("%s   timeout, cp_ack_ap_value was %d", __func__,
			cp_ack_ap_value);
	return -1;
}

static int mem_pd_get_addr_from_cp(void)
{
	int ret;
	int i;
	int save_bin_index;
	unsigned int bt_begin[BT_SAVE_BIN_NUM] = {0};
	unsigned int bt_end[BT_SAVE_BIN_NUM] = {0};
	unsigned int wifi_begin[WIFI_SAVE_BIN_NUM] = {0};
	unsigned int wifi_end[WIFI_SAVE_BIN_NUM] = {0};
	unsigned int sync_bt_begin_addr = 0x0;
	unsigned int sync_wifi_begin_addr = 0x0;
	unsigned int sync_bt_segtions = 0x0;
	unsigned int sync_wifi_segtions = 0x0;

	PD_INFO("%s entry.", __func__);
	/*get the bt used segtions*/
	ret = sprdwcn_bus_reg_read(SYNC_BT_USED_SECTIONS_ADDR,
							&sync_bt_segtions, 4);
	PD_INFO("%s sync_bt_segtions is 0x%x.",
			__func__, sync_bt_segtions);
	/*update bt segtions*/
	mem_info_cp.bt_used_segtions = sync_bt_segtions;
	if ((ret < 0) || (mem_info_cp.bt_used_segtions  > BT_SAVE_BIN_NUM)) {
		PD_ERR("%s,ret is %d, sync_bt_segtions is 0x%x\n", __func__,
			 ret, sync_bt_segtions);
		return -1;
	}
	/*get bt begin addr*/
	sync_bt_begin_addr = SYNC_BT_BEGIN_ADDR;
	/*init bt_begin[] and be_end[];*/
	for (i = 0; i < sync_bt_segtions; i++) {
		ret = sprdwcn_bus_reg_read(sync_bt_begin_addr+(i<<0x2),
				   &bt_begin[i], 4);
		if (ret < 0) {
			PD_ERR("%s mem_pd read	bt begin addr error:%d\n",
				__func__, ret);
			return ret;
		}

		ret = sprdwcn_bus_reg_read(sync_bt_begin_addr+
			(sync_bt_segtions<<0x2) + (i<<0x2), &bt_end[i], 4);
		if (ret < 0) {
			PD_ERR("%s mem_pd read	bt begin addr error:%d\n",
				__func__, ret);
			return ret;
		}

		mem_info_cp.bt_begin_addr[i] = bt_begin[i];
		mem_info_cp.bt_end_addr[i] = bt_end[i];
		mem_info_cp.bt_size[i] = bt_end[i] - bt_begin[i];
		PD_INFO(" bt_begin_addr[%d] is 0x%x,bt_end_addr[%d] is 0x%x.",
			 i, bt_begin[i], i, bt_end[i]);
		mem_pd.bt_mem[i] = kmalloc(mem_info_cp.bt_size[i], GFP_KERNEL);
		if (!mem_pd.bt_mem[i]) {
			for (save_bin_index = i-1; save_bin_index >= 0;
				save_bin_index--) {
				kfree(mem_pd.bt_mem[save_bin_index]);
			}

			PD_INFO("mem pd bt save buff malloc Failed.");
			return MEM_PD_ERR;
		}

#if 1
		mem_pd.bt_clear[i] = kmalloc(mem_info_cp.bt_size[i],
			GFP_KERNEL);
		if (!mem_pd.bt_clear[i]) {
			kfree(mem_pd.bt_mem[i]);
			for (save_bin_index = i-1; save_bin_index >= 0;
				save_bin_index--) {
				kfree(mem_pd.bt_mem[save_bin_index]);
				kfree(mem_pd.bt_clear[save_bin_index]);
			}

			PD_INFO("mem pd clear buff malloc Failed.");
			return MEM_PD_ERR;
		}

		memset(mem_pd.bt_clear[i], 0x0, mem_info_cp.bt_size[i]);
#endif
	}

	/*get the wifi used segtions*/
	ret = sprdwcn_bus_reg_read(SYNC_WIFI_USED_SECTIONS_ADDR,
							&sync_wifi_segtions, 4);
	/*update wifi segtions*/
	mem_info_cp.wifi_used_segtions = sync_wifi_segtions;
	if ((ret < 0) || (mem_info_cp.wifi_used_segtions > WIFI_SAVE_BIN_NUM)) {
		PD_ERR("%s ret is %d,sync_wifi_segtions is 0x%x\n",
			__func__, ret, sync_wifi_segtions);
		return ret;
	}
	/*get wifi begin addr*/
	sync_wifi_begin_addr = SYNC_WIFI_BEGIN_ADDR;
	for (i = 0; i < sync_wifi_segtions; i++) {
		ret = sprdwcn_bus_reg_read(sync_wifi_begin_addr+(i<<2),
				   &wifi_begin[i], 4);
		if (ret < 0) {
			PD_ERR("%s mem_pd read	wifi begin addr error:%d\n",
				__func__, ret);
			return ret;
		}

		ret = sprdwcn_bus_reg_read(sync_wifi_begin_addr+
			(sync_wifi_segtions<<0x2)+(i<<2), &wifi_end[i], 4);
		if (ret < 0) {
			PD_ERR("%s mem_pd read	wifi begin addr error:%d\n",
				__func__, ret);
			return ret;
		}

		mem_info_cp.wifi_begin_addr[i] = wifi_begin[i];
		mem_info_cp.wifi_end_addr[i] = wifi_end[i];
		mem_info_cp.wifi_size[i] = wifi_end[i] - wifi_begin[i];
		PD_INFO(" wifi_begin[%d] is 0x%x, wifi_end_addr[%d] is 0x%x.",
			  i, wifi_begin[i], i, wifi_end[i]);
		mem_pd.wifi_mem[i] = kmalloc(mem_info_cp.wifi_size[i],
					      GFP_KERNEL);
		if (!mem_pd.wifi_mem[i]) {
			for (save_bin_index = sync_bt_segtions-1;
				save_bin_index >= 0; save_bin_index--) {
				kfree(mem_pd.bt_mem[save_bin_index]);
				kfree(mem_pd.bt_clear[save_bin_index]);
			}
			for (save_bin_index = i-1; save_bin_index >= 0;
				save_bin_index--) {
				kfree(mem_pd.wifi_mem[save_bin_index]);
			}
			PD_INFO("mem pd wifi save buff malloc Failed.");
			return MEM_PD_ERR;
		}
#if 1
		mem_pd.wifi_clear[i] = kmalloc(mem_info_cp.wifi_size[i],
			GFP_KERNEL);
		if (!mem_pd.wifi_clear[i]) {
			for (save_bin_index = sync_bt_segtions-1;
				save_bin_index >= 0; save_bin_index--) {
				kfree(mem_pd.bt_mem[save_bin_index]);
				kfree(mem_pd.bt_clear[save_bin_index]);
			}
			kfree(mem_pd.wifi_mem[i]);
			for (save_bin_index = i-1; save_bin_index >= 0;
				save_bin_index--) {
				kfree(mem_pd.wifi_mem[save_bin_index]);
				kfree(mem_pd.wifi_clear[save_bin_index]);
			}
			PD_INFO("mem pd clear buff malloc Failed.");
			return MEM_PD_ERR;
		}

		memset(mem_pd.wifi_clear[i], 0x0, mem_info_cp.wifi_size[i]);
#endif
	}

	mem_info_cp.chip_id = marlin_get_wcn_chipid();
	PD_INFO("%s ok\n", __func__);
	return 0;
}

static int mem_pd_save_bin_finish(void)/*umw2653*/
{
	int err = 0;
	int i;

	PD_INFO("%s  save wifi/bt mem bin start", __func__);

	if (mem_pd_get_addr_from_cp() < 0) {
		PD_INFO("%s  mem_pd get	sync_addr from cp failed", __func__);
		return -1;/*get the sync addr error*/
	}
	/*----*/
	for (i = 0; i < mem_info_cp.wifi_used_segtions; i++) {
		err = sprdwcn_bus_direct_read(mem_info_cp.wifi_begin_addr[i],
			mem_pd.wifi_mem[i], mem_info_cp.wifi_size[i]);
		if (err < 0) {
			pr_err("%s wifi save mem bin error:%d", __func__, err);
			return err;
		}
		PD_INFO("read wifi_mem[%d] ok, begin[%d] is 0x%x,size is 0x%x",
			  i, i, mem_info_cp.wifi_begin_addr[i],
			  mem_info_cp.wifi_size[i]);
	}

	for (i = 0; i < mem_info_cp.bt_used_segtions; i++) {
		err = sprdwcn_bus_direct_read(mem_info_cp.bt_begin_addr[i],
					       mem_pd.bt_mem[i],
					       mem_info_cp.bt_size[i]);
		if (err < 0) {
			pr_err("%s bt save mem bin error:%d", __func__, err);
			return err;
		}
		PD_INFO("read bt_mem[%d] ok,begin[%d] is 0x%x,size is 0x%x",
			i, i,
			mem_info_cp.bt_begin_addr[i],
			mem_info_cp.bt_size[i]);
	}

	PD_INFO("%s save wifi/bt mem bin finish", __func__);

	return 0;
}

int mem_pd_save_bin(void)
{
	unsigned int sync_flag = BIT_MEM_PD_AP_ENABLE;
	/* mutex_lock(&(mem_pd.mem_pd_lock)); */
	PD_INFO("%s start addr=%x\n", __func__, SYNC_MEM_PD_SYNC_FLAG);
	sprdwcn_bus_reg_write(SYNC_MEM_PD_SYNC_FLAG, &sync_flag, 4);
	/*step1:wait the sync addr from cp*/
	if (mem_pd_wait_for_cp_status(BIT_CP_TO_AP_BIN_ADDR_OK) < 0) {
		PD_INFO("cp version has no mem_pd function");
		/* mutex_unlock(&(mem_pd.mem_pd_lock)); */
		mem_pd.cp_version = 1;
		return 0;
	}
	if (mem_pd.bin_save_done == 0) {
		mem_pd.bin_save_done = 1;
		PD_INFO("cp first power on");
		/*step2: save wifi or bt mem_pd bin*/
		if (mem_pd_save_bin_finish() < 0) {
			PD_INFO("mem_pd save bin from cp fail");
			return 0;
		}

		/* save done */
	} else
		PD_INFO("cp not first power on %s do nothing", __func__);
	/*step3:send int save_bin_req to cp*/
	mem_pd_inform_cp_save_cp_bin_ok();
	PD_INFO("%s end", __func__);
	/* mutex_unlock(&(mem_pd.mem_pd_lock)); */
	return 0;
}
static int mem_pd_download_mem_bin(int subsys)
{
	int err = 0;
	int i;
	unsigned int addr = 0;
	char *mem;
	unsigned int len = 0;

	PD_INFO("%s start", __func__);
	switch (subsys) {
	case MARLIN_WIFI:
		for (i = 0; i < mem_info_cp.wifi_used_segtions; i++) {
			addr = mem_info_cp.wifi_begin_addr[i];
			mem = mem_pd.wifi_mem[i];
			len = mem_info_cp.wifi_size[i];
			sprdwcn_bus_direct_write(addr, mem, len);
		}
		PD_INFO("%s, wifi mem download ok", __func__);
	break;
	case MARLIN_BLUETOOTH:
		for (i = 0; i < mem_info_cp.bt_used_segtions; i++) {
			addr = mem_info_cp.bt_begin_addr[i];
			mem = mem_pd.bt_mem[i];
			len = mem_info_cp.bt_size[i];
			sprdwcn_bus_direct_write(addr, mem, len);
		}
		PD_INFO("%s, bt mem download ok", __func__);
	break;
	default:
		return MEM_PD_ERR;
	}

	if (err < 0) {
		pr_err("%s download mem bin error:%d", __func__, err);
		return err;
	}
	PD_INFO("%s end", __func__);
	return 0;
}
int mem_pd_mgr(int subsys, int val)
{
	if (mem_pd.cp_version)
		return 0;
	PD_INFO("%s start", __func__);
	if ((subsys != MARLIN_WIFI) && (subsys != MARLIN_BLUETOOTH)) {
		PD_INFO("subsys:%d, do nothing, return ok", subsys);
		return 0;
	}
	mutex_lock(&(mem_pd.mem_pd_lock));
	switch (subsys) {
	case MARLIN_WIFI:
		PD_INFO("marlin wifi state:%d, subsys %d power %d",
			  mem_pd.wifi_state, subsys, val);
		if (val) {
			if (mem_pd.wifi_state != THREAD_DELETE) {
				PD_INFO("wifi opened, do nothing");
				goto out;
			}
			mem_pd.wifi_state = THREAD_CREATE;
			/*step1: power on wifi mem*/
			mem_pd_inform_cp_wifi_poweron();
			if (mem_pd_wait_for_cp_status(
				BIT_AP_INT_CP_WIFI_POWER_ON_ACK) < 0) {
				PD_INFO("wait wifi [mem power on] fail");
				goto mem_pd_err;
			}
			/*step2:download wifi bin*/
			mem_pd_download_mem_bin(subsys);
			/*step3:inform cp open wifi,such as create wifi thread*/
			mem_pd_inform_cp_wifi_open();
			if (mem_pd_wait_for_cp_status(
				BIT_AP_INT_CP_WIFI_OPEN_ACK) < 0) {
				PD_INFO("wait wifi [open] fail");
				goto mem_pd_err;
			}
			PD_INFO("cp wifi creat thread ok");
		} else {
			if (mem_pd.wifi_state != THREAD_CREATE) {
				PD_INFO("wifi closed ,do nothing");
				goto out;
			}
			/* step1:close wifi,cp will delete thread,power off mem,
			 * send close_ack to ap!
			 */
			mem_pd_inform_cp_wifi_close();
			if (mem_pd_wait_for_cp_status(
				BIT_AP_INT_CP_WIFI_CLOSE_ACK) < 0) {
				PD_INFO("wait wifi [close] fail");
				goto mem_pd_err;
			}
			mem_pd.wifi_state = THREAD_DELETE;
			PD_INFO("cp wifi delete thread ok");
		}
		break;
	case MARLIN_BLUETOOTH:
		PD_INFO("marlin bt state:%d, subsys %d, power %d",
			  mem_pd.bt_state, subsys, val);
		if (val) {
			if (mem_pd.bt_state != THREAD_DELETE) {
				PD_INFO("bt opened, do nothing");
				goto out;
			}
			mem_pd.bt_state = THREAD_CREATE;
			/*step1: power on bt mem*/
			mem_pd_inform_cp_bt_poweron();
			if (mem_pd_wait_for_cp_status(
				BIT_AP_INT_CP_BT_POWER_ON_ACK) < 0) {
				PD_INFO("wait bt [mem power on] fail");
				goto mem_pd_err;
			}
			/*step2:download wifi bin*/
			mem_pd_download_mem_bin(subsys);
			/*step3:inform cp open wifi,such as create wifi thread*/
			mem_pd_inform_cp_bt_open();
			if (mem_pd_wait_for_cp_status(
				BIT_AP_INT_CP_BT_OPEN_ACK) < 0) {
				PD_INFO("wait bt [open] fail");
				goto mem_pd_err;
			}
			PD_INFO("cp bt creat thread ok");
		} else {
			if (mem_pd.bt_state != THREAD_CREATE) {
				PD_INFO("bt closed ,do nothing");
				goto out;
			}
			/* step1:close bt,cp will delete thread,
			 * power off mem,send close_ack to ap!
			 */
			mem_pd_inform_cp_bt_close();
			if (mem_pd_wait_for_cp_status(
				BIT_AP_INT_CP_BT_CLOSE_ACK) < 0) {
				PD_INFO("wait bt [close] fail");
				goto mem_pd_err;
			}
			mem_pd.bt_state = THREAD_DELETE;
			PD_INFO("cp bt delete thread ok");
		}
		break;
	default:
		PD_INFO("%s switch default", __func__);
	}

out:
		mutex_unlock(&(mem_pd.mem_pd_lock));
		PD_INFO("%s end", __func__);
		return 0;

mem_pd_err:
		mutex_unlock(&(mem_pd.mem_pd_lock));
		PD_ERR("%s return error", __func__);

		return -1;
}
int mem_pd_poweroff_deinit(void)
{
	if (mem_pd.cp_version)
		return 0;
	PD_INFO("mem_pd_chip_poweroff_deinit");
	mem_pd.wifi_state = 0;
	mem_pd.bt_state = 0;
	mem_pd.cp_version = 0;
	mem_pd.cp_mem_all_off = 0;
	return 0;
}
int mem_pd_init(void)
{
	PD_INFO("%s start", __func__);
	mutex_init(&(mem_pd.mem_pd_lock));
	mem_pd.wifi_state = 0;
	mem_pd.bt_state = 0;
	mem_pd.cp_version = 0;
	mem_pd.cp_mem_all_off = 0;

	PD_INFO("%s end!", __func__);

	return 0;
}

int mem_pd_exit(void)
{
	PD_INFO("%s enter", __func__);
	/* atomic_set(&(slp_mgr.cp2_state), STAY_SLPING); */
	/* sleep_active_modules = 0; */
	/* wake_cnt = 0; */
	mutex_destroy(&(mem_pd.mem_pd_lock));
	/* mutex_destroy(&(slp_mgr.wakeup_lock)); */
	kfree(mem_pd.wifi_mem);
	mem_pd.wifi_mem[0] = NULL;
	kfree(mem_pd.bt_mem);
	mem_pd.bt_mem[0] = NULL;
	kfree(mem_pd.wifi_clear);
	mem_pd.wifi_clear[0] = NULL;
	kfree(mem_pd.bt_clear);
	mem_pd.bt_clear[0] = NULL;
	PD_INFO("%s ok!", __func__);

	return 0;
}
