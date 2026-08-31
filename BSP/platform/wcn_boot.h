/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _WCN_BOOT
#define _WCN_BOOT

#include "marlin_platform.h"

#include "rf/rf.h"

#define SUFFIX "androidboot.slot_suffix="

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "WCN BOOT: " fmt

struct wcn_sync_info_t {
	unsigned int init_status;
	unsigned int mem_pd_bt_start_addr;
	unsigned int mem_pd_bt_end_addr;
	unsigned int mem_pd_wifi_start_addr;
	unsigned int mem_pd_wifi_end_addr;
	unsigned int prj_type;
	unsigned short tsx_dac_data;
	unsigned short rsved;
#ifdef CONFIG_WCN_SDIO
	unsigned int sdio_config;
#endif
} __packed;

struct tsx_data {
	u32 flag; /* cali flag ref */
	u16 dac; /* AFC cali data */
	u16 reserved;
};

struct tsx_cali {
	u32 init_flag;
	struct tsx_data tsxdata;
};

#ifdef CONFIG_WCN_SDIO
/*
 * sdio config to cp side
 * bit[31:24]: reserved
 * bit[23]: wake_host_data_separation:
 *	0: if BT_WAKEUP_HOST en or WL_WAKEUP_HOST en,
 *	    wifi and bt packets can wake host;
 *	1: if BT_WAKEUP_HOST en, ONLY bt packets can wake host;
 *	    if WL_WAKEUP_HOST en, ONLY wifi packets can wake host
 * bit[22:18]: wake_host_level_duration_10ms: BT_WAKEUP_HOST or WL_WAKEUP_HOST
 *	      level dyration time per 10ms, example: 0:0ms; 3:30ms; 20:200ms
 * bit[17:16]: wl_wake_host_trigger_type:
 *	     00:WL_WAKEUP_HOST  trigger type low
 *	     01:WL_WAKEUP_HOST  trigger type rising
 *	     10:WL_WAKEUP_HOST  trigger type falling
 *	     11:WL_WAKEUP_HOST  trigger type high
 * bit[15]: wl_wake_host_en: 0: disable, 1: enable
 * bit[14:13]: sdio_irq_trigger_type:
 *	      00:pubint gpio irq trigger type low
 *	      01:pubint gpio irq trigger type rising [NOT support]
 *	      10:pubint gpio irq trigger type falling [NOT support]
 *	      11:pubint gpio irq trigger type high
 * bit[12:11]: sdio_irq_type:
 *	      00:dedicated irq, gpio1
 *	      01:inband data1 irq
 *	      10:use BT_WAKEUP_HOST(pubint) pin as gpio irq
 *	      11:use WL_WAKEUP_HOST(esmd3) pin as gpio irq
 * bit[10:9]: bt_wake_host_trigger_type:
 *	     00:BT_WAKEUP_HOST  trigger type low
 *	     01:BT_WAKEUP_HOST  trigger type rising
 *	     10:BT_WAKEUP_HOST  trigger type falling
 *	     11:BT_WAKEUP_HOST  trigger type high
 * bit[8]: bt_wake_host_en: 0: disable, 1: enable
 * bit[7:5]: sdio_blk_size: 000: blocksize 840; 001: blocksize 512
 * bit[4]: sdio_rx_mode: 0: adma; 1: sdma
 * bit[3:1]: vendor_id: 000: default id, unisoc[0x0]
 *		       001: hisilicon/goke default version, pull chipen after resume
 *		       010: hisilicon/goke version, keep power (NOT pull chipen) and
 *			    reset sdio after resume
 * bit[0]: sdio_config_en: 0: disable sdio config
 *		          1: enable sdio config
 */
union wcn_sdiohal_config {
	unsigned int val;
	struct {
		unsigned char sdio_config_en:1;
		unsigned char vendor_id:3;
		unsigned char sdio_rx_mode:1;
		unsigned char sdio_blk_size:3;
		unsigned char bt_wake_host_en:1;
		unsigned char bt_wake_host_trigger_type:2;
		unsigned char sdio_irq_type:2;
		unsigned char sdio_irq_trigger_type:2;
		unsigned char wl_wake_host_en:1;
		unsigned char wl_wake_host_trigger_type:2;
		unsigned char wake_host_level_duration_10ms:5;
		unsigned char wake_host_data_separation:1;
		unsigned int  reserved:8;
	} cfg;
};
#endif

#define WCN_BOUND_CONFIG_NUM	4
struct wcn_pmic_config {
	bool enable;
	char name[32];
	/* index [0]:addr [1]:mask [2]:unboudval [3]boundval */
	u32 config[WCN_BOUND_CONFIG_NUM];
};

struct wcn_clock_info {
	enum wcn_clock_type type;
	enum wcn_clock_mode mode;
	/*
	 * xtal-26m-clk-type-gpio config in the dts.
	 * if xtal-26m-clk-type config in the dts,this gpio unvalid.
	 */
	int gpio;
};

struct firmware_backup {
	size_t size;
	u8 *data;
};

struct marlin_device {
	struct wcn_clock_info clk_xtal_26m;
#ifdef CONFIG_WCN_BT_HOST_WAKE
	int bt_host_wake;
#endif
#ifdef CONFIG_WCN_WL_HOST_WAKE
	int wl_host_wake;
#endif
#ifdef CONFIG_WCN_EXTERNAL_3V3_DCDC
	int wcn_power_en;
#endif
#ifdef CONFIG_WCN_EXTERNAL_1V2_DCDC
	int wcn_1v2;
#endif
	int wakeup_ap;
	int reset;
	int chip_en;
	int int_ap;
	int coexist;
	/* pmic config */
	struct regmap *syscon_pmic;
	/* sharkl5 vddgen1 */
	struct wcn_pmic_config avdd12_parent_bound_chip;
	struct wcn_pmic_config avdd12_bound_wbreq;
	struct wcn_pmic_config avdd33_bound_wbreq;

	bool bound_avdd12;
	bool bound_dcxo18;
	/* power sequence */
	/* VDDIO->DVDD12->chip_en->rst_N->AVDD12->AVDD33 */
	struct regulator *dvdd12;
	struct regulator *avdd12;
	/* for PCIe */
	struct regulator *avdd18;
	/* for wifi PA, BT TX RX */
	struct regulator *avdd33;
	/* for internal 26M clock */
	struct regulator *dcxo18;
	struct clk *clk_32k;
	struct regulator *avdd33_usb20;

	struct clk *clk_parent;
	struct clk *clk_enable;
	struct device_node *np;
	struct mutex power_lock;
	struct completion carddetect_done;
	struct completion download_done;
	struct completion gnss_download_done;
	unsigned long power_state;
	char *write_buffer;
	struct delayed_work power_wq;
	struct work_struct download_wq;
	struct work_struct gnss_dl_wq;
	bool keep_power_on;
	bool wait_ge2;
	bool is_btwf_in_sysfs;
	bool is_gnss_in_sysfs;
	int wifi_need_download_ini_flag;
	int first_power_on_ready;
	atomic_t download_finish_flag;
	unsigned char bt_wl_wake_host_en;
#ifdef CONFIG_THIRD_PARTY_BOARD
	unsigned int bt_wake_host_int_num;
	unsigned int wl_wake_host_int_num;
#endif
	unsigned char gnss_dl_finish_flag;
	int loopcheck_status_change;
	struct wcn_sync_info_t sync_f;
	struct tsx_cali tsxcali;
	char *btwf_path;
	char *gnss_path;
	phys_addr_t	base_addr_btwf;
	uint32_t	maxsz_btwf;
	phys_addr_t	base_addr_gnss;
	uint32_t	maxsz_gnss;
	int first_power_on_flag;
	struct firmware_backup firmware;
#ifdef CONFIG_WCN_USB
	int marlin_probe_status;
	int usb_hotplug_status;
	int usb_reset_status;
	struct work_struct usb_hotplug;
#endif
};

struct wifi_calibration {
	struct wifi_config_t config_data;
	struct wifi_cali_t cali_data;
};

#endif
