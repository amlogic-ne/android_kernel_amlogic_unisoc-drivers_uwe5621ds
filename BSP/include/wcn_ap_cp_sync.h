#ifndef _WCN_AP_CP_SYNC_H
#define _WCN_AP_CP_SYNC_H
/***************************************************************/
/*AP_CP SYNC function: the following define MUST be the same as CP*/
enum AP_INT_CP_REQ_ID {
	/*BIT0~BIT6 was defined for mem_pd*/
	BIT_AP_INT_CP_SAVE_BIN_REQ			= BIT(0),
	BIT_AP_INT_CP_WIFI_POWER_ON_REQ		= BIT(1),
	BIT_AP_INT_CP_WIFI_OPEN_REQ			= BIT(2),
	BIT_AP_INT_CP_WIFI_CLOSE_REQ			= BIT(3),
	BIT_AP_INT_CP_BT_POWER_ON_REQ		= BIT(4),
	BIT_AP_INT_CP_BT_OPEN_REQ			= BIT(5),
	BIT_AP_INT_CP_BT_CLOSE_REQ			= BIT(6),
	/*BIT7 was defined for AP CP sleep wakeup*/
	BIT_AP_INT_CP_WAKEUP_REQ			= BIT(7),
	/*set by ap,clear by cp*/
	/*update: Not used,replace by ap_allow_cp*/
};

enum CP_TO_AP_ACK_ID {
	BIT_CP_TO_AP_BIN_ADDR_OK			= BIT(0),
	BIT_AP_INT_CP_WIFI_POWER_ON_ACK		= BIT(1),
	BIT_AP_INT_CP_WIFI_OPEN_ACK			= BIT(2),
	BIT_AP_INT_CP_WIFI_CLOSE_ACK			= BIT(3),
	BIT_AP_INT_CP_BT_POWER_ON_ACK		= BIT(4),
	BIT_AP_INT_CP_BT_OPEN_ACK			= BIT(5),
	BIT_AP_INT_CP_BT_CLOSE_ACK			= BIT(6),
	BIT_CP_TO_AP_WAKEUP_STATUS			= BIT(7),
	/*control by cp*/
	/*update: Not used,replace by MTX_M5_STOP*/
};

enum MEM_PD_FLAG_ID {
	BIT_MEM_PD_CP_ENABLE				= BIT(0),
	BIT_MEM_PD_AP_ENABLE				= BIT(1),
};

/***************************************************************/
/*for ap_cp_syc add info*/
#define WIFI_SAVE_BIN_NUM 0x5/*the max sections, m3e used only 3*/
#define BT_SAVE_BIN_NUM 0x5/*the max sections,m3e used only 2*/

#endif