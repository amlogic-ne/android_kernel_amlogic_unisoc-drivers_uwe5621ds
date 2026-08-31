/*
 * btif_woble.c
 *
 *  Created on: 2020
 *      Author: Unisoc
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/vt_kern.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/version.h>

#include <linux/file.h>
#include <linux/string.h>
#include "marlin_platform.h"
#include "tty.h"
#include "woble.h"

#define MAX_WAKE_DEVICE_MAX_NUM 36
#define CONFIG_FILE_PATH "/data/misc/bluedroid/bt_config.conf"

static struct hci_cmd_t hci_cmd;
uint8_t device_count_db;
const woble_config_t s_woble_config_cust = {WOBLE_MOD_ENABLE, WOBLE_SLEEP_MOD_COULD_NOT_KNOW, 0, WOBLE_SLEEP_MOD_NOT_NEED_NOTITY};

mtty_bt_wake_t bt_wake_dev_db[MAX_WAKE_DEVICE_MAX_NUM];
woble_switch_t woble_switch;
unsigned char peer_addr[BD_ADDR_LEN];
unsigned char own_public_addr[BD_ADDR_LEN];

int woble_init(void)
{
	memset(&woble_switch, 0, sizeof(woble_switch_t));
	memset(&hci_cmd, 0, sizeof(struct hci_cmd_t));
	sema_init(&hci_cmd.wait, 0);

	return 0;
}

int mtty_bt_str_hex(char *str, uint8_t count, char *hex)
{
	uint8_t data_buf[12] = {0x00};
	uint8_t i = 0;
	while ((*str != '\0') && (i < count * 2)) {
		if ((*str >= '0') && (*str <= '9')) {
			data_buf[i] = *str - '0' + 0x00;
			i++;
		} else if ((*str >= 'A') && (*str <= 'F')) {
			data_buf[i] = *str - 'A' + 0x0A;
			i++;
		} else if ((*str >= 'a') && (*str <= 'f')) {
			data_buf[i] = *str - 'a' + 0x0a;
			i++;
		} else {
			;
		}
		str++;
	}
	if (count == 1) {
		*hex = data_buf[0];
	} else {
		for (i = 0; i < count; i++) {
			hex[i] = (data_buf[0 + i * 2] << 4) + data_buf[1 + i * 2];
		}
	}
	for (i = 0; i < count; i++) {
		pr_info("%s data[%d] = %02x\n", __func__, i, hex[i]);
	}
	return 0;
}

int mtty_bt_conf_prase(char *conf_str)
{
	char *tok_str = conf_str;
	char *str1, *str2, *str3;
	uint8_t device_count = 0;
	uint8_t loop_count = 0;
	if (conf_str) {
		while (tok_str != NULL) {
			tok_str = strsep(&conf_str, "\r\n");
			if (!tok_str)
				continue;
			if ((strpbrk(tok_str, "[") != NULL) &&
				(strpbrk(tok_str, "]") != NULL) &&
				(strpbrk(tok_str, ":") != NULL)) {
				bt_wake_dev_db[device_count].addr_str = tok_str;
				continue;
			}
			if (strstr(tok_str, "DevType") != NULL) {
				bt_wake_dev_db[device_count].dev_tp_str = tok_str;
				continue;
			}
			if (strstr(tok_str, "AddrType") != NULL) {
				bt_wake_dev_db[device_count].addr_tp_str = tok_str;
				device_count++;
			}
		}
	} else {
		pr_info("%s conf_str is NULL\n", __func__);
	}
	if (device_count) {
		for (; loop_count < device_count; loop_count++) {
			str1 = strchr(bt_wake_dev_db[loop_count].addr_str, '[');
			str2 = strstrip(bt_wake_dev_db[loop_count].dev_tp_str);
			str2 = strchr(str2, '=');
			str3 = strstrip(bt_wake_dev_db[loop_count].addr_tp_str);
			str3 = strchr(str3, '=');
			mtty_bt_str_hex(str1, 6, bt_wake_dev_db[loop_count].addr);
			mtty_bt_str_hex(str2, 1, &(bt_wake_dev_db[loop_count].dev_tp));
			mtty_bt_str_hex(str3, 1, &(bt_wake_dev_db[loop_count].addr_tp));
		}
	}
	return device_count;
}

int mtty_bt_read_conf(void)
{
	char *buffer = NULL;
	unsigned char *p_buf = NULL;
	struct file *bt_conf_fp;
	unsigned int read_len, buffer_len;
	uint8_t device_count = 0;
	loff_t file_size = 0;
	loff_t file_offset = 0;
	memset(bt_wake_dev_db, 0, sizeof(mtty_bt_wake_t) * MAX_WAKE_DEVICE_MAX_NUM);
	bt_conf_fp = filp_open(CONFIG_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(bt_conf_fp)) {
		pr_info("%s open file %s error %ld \n",
				__func__, CONFIG_FILE_PATH, PTR_ERR(bt_conf_fp));
		return device_count;
	}
	file_size = vfs_llseek(bt_conf_fp, 0, SEEK_END);
	buffer_len = 0;
	buffer = vmalloc(file_size);
	p_buf = buffer;
	if (!buffer) {
		fput(bt_conf_fp);
		pr_info("%s no memory\n", __func__);
		return device_count;
	}

	do {
		//read_len = kernel_read(bt_conf_fp, file_offset, p_buf, file_size);
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
		read_len = kernel_read(bt_conf_fp, (void *)p_buf, file_size, &file_offset);
#else
		read_len = kernel_read(bt_conf_fp, file_offset, p_buf, file_size);
#endif
		if (read_len > 0) {
			buffer_len += read_len;
			file_size -= read_len;
			p_buf += read_len;
		}
	} while ((read_len > 0) && (file_size > 0));

	fput(bt_conf_fp);
	pr_info("%s read %s data_len:0x%x\n",
			__func__, CONFIG_FILE_PATH, buffer_len);
	device_count = mtty_bt_conf_prase(buffer);
	return device_count;
}

int woble_data_recv(const unsigned char *buf, int count)
{
	unsigned short opcode = 0;
	unsigned short conn_handle = 0;
	unsigned char status = 0;
	unsigned char sub_code = 0;
	const unsigned char *p = 0;
	const unsigned char *rxmsg = buf;
	int left_length = count;
	int pack_length = 0;
	int last = 0;
	int ret = -1;

	if (count < 0) {
		pr_err("%s count < 0!!!", __func__);
	}

	do {
		rxmsg = buf + (count - left_length);


		switch (rxmsg[PACKET_TYPE]) {
		case HCI_EVT:
			if (left_length < 3) {
				pr_err("%s left_length <3 !!!!!", __func__);
			}

			pack_length = rxmsg[EVT_HEADER_SIZE];
			pack_length += 3;

			if (left_length - pack_length < 0) {
				pr_err("%s left_length - pack_length <0 !!!!!", __func__);

			}
			switch (rxmsg[EVT_HEADER_EVENT]) {
			default:
			case BT_HCI_EVT_CMD_COMPLETE:
				p = rxmsg + 4;
				STREAM_TO_UINT16(opcode, p);
				STREAM_TO_UINT8(status, p);

				if (opcode == BT_HCI_OP_READ_BDADR) {
        		STREAM_TO_ARRAY(own_public_addr, p, BD_ADDR_LEN);
        		pr_info("%s local address %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,\
            			own_public_addr[5], own_public_addr[4], own_public_addr[3], \
						own_public_addr[2], own_public_addr[1], own_public_addr[0]);
    			}
				break;

			case BT_HCI_EVT_CMD_STATUS:
				p = rxmsg + 5;
				STREAM_TO_UINT16(opcode, p);
				status = rxmsg[3];
				break;

			case LE_HCI_EVT_CMD_DISCONNECTION_COMPLETE:
				p = rxmsg + 3;
				STREAM_TO_UINT16(conn_handle, p);
				pr_info("disconnect conn_handle=%d.\n",conn_handle);
				if (woble_switch.conn_handle == conn_handle) {
					if (!woble_switch.driver_diconn) {
						woble_switch.suspend_role = SUSPEND_IS_NOT_BT;
						pr_info("disconnect from application.\n");
					} else {
						woble_switch.driver_diconn = false;
					}
				}
				break;

			case LE_HCI_EVT_CMD_META:
				p = rxmsg + 3;
				STREAM_TO_UINT8(sub_code, p);

				if (LE_HCI_EVT_CMD_CONN_COMPLETE == sub_code || LE_HCI_EVT_CMD_ENHANCED_CONN_COMPLETE == sub_code) {
					pr_info("get connect handle and suspend_role\n");
					p = rxmsg + 4;
					if (STATUS_SUCCESS == *p) {
						STREAM_TO_UINT16(woble_switch.conn_handle, p);
						pr_info("connect woble_switch.conn_handle=%d.\n",woble_switch.conn_handle);
						woble_switch.suspend_role = SUSPNED_IS_BT;

						p = rxmsg + 9;
						STREAM_TO_ARRAY(peer_addr, p, BD_ADDR_LEN);
						pr_info("%s peer address %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
            			peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
					}
				}
				break;
			}
			last = left_length;
			left_length -= pack_length;
			break;
		default:
			left_length = 0;
			break;
		}
	} while (left_length);

	if (hci_cmd.opcode == opcode && hci_cmd.opcode) {
		pr_info("%s opcode: 0x%04X, status: %d\n", __func__, opcode, status);
		if (BT_HCI_OP_DISCONNECT == opcode)
			woble_switch.driver_diconn = true;
		up(&hci_cmd.wait);
		ret = 0;
	}
	return ret;
}

int hci_cmd_send_sync(unsigned short opcode, struct HC_BT_HDR *py,
		struct HC_BT_HDR *rsp)
{
	unsigned char msg_req[HCI_CMD_MAX_LEN], *p;
	int ret = 0;
	pr_info("%s max_len=%d+++\n", __func__, HCI_CMD_MAX_LEN);

	p = msg_req;
	UINT8_TO_STREAM(p, HCI_CMD);
	UINT16_TO_STREAM(p, opcode);

	if (py == NULL) {
		UINT8_TO_STREAM(p, 0);
	} else {
		UINT8_TO_STREAM(p, py->len);
		ARRAY_TO_STREAM(p, py->data, py->len);
	}
	hci_cmd.opcode = opcode;

	ret = sdio_data_transmit(msg_req, p - msg_req);

	if (!ret) {
		hci_cmd.opcode = 0;
		pr_err("%s sdio_data_transmit fail", __func__);
		return 0;
	}
	down(&hci_cmd.wait);
	hci_cmd.opcode = 0;
	pr_info("%s ---\n", __func__);
	return 0;
}

void hci_disconnect(void)
{
    struct HC_BT_HDR* payload = (struct HC_BT_HDR*)vmalloc(sizeof(struct HC_BT_HDR) + 3);
    unsigned char* p;
    p = payload->data;

    pr_info("%s start\n", __func__);

    payload->len = 3;
    UINT16_TO_STREAM_SAME(p, woble_switch.conn_handle);

    UINT8_TO_STREAM(p, HCI_ERR_PEER_USER); // 0x13

    hex_dump_block(payload->data, payload->len);
    hci_cmd_send_sync(BT_HCI_OP_DISCONNECT, payload, NULL);
    vfree(payload);

    pr_info("%s end\n", __func__);
    return;
}

void hci_set_ap_sleep_mode(int is_shutdown, int is_resume)
 {
    struct HC_BT_HDR* payload = (struct HC_BT_HDR*)vmalloc(sizeof(struct HC_BT_HDR) + 5); // 6
    unsigned char* p;
    p = payload->data;

    pr_info("%s start\n", __func__);

    payload->len = 5;
    if (is_resume) {
        UINT8_TO_STREAM(p, WOBLE_MOD_DISABLE);
    } else {
        UINT8_TO_STREAM(p, WOBLE_MOD_ENABLE);
    }
    UINT8_TO_STREAM(p, 0);
    UINT16_TO_STREAM(p, s_woble_config_cust.timeout);
    UINT8_TO_STREAM(p, s_woble_config_cust.notify);

    hex_dump_block(payload->data, payload->len);
    hci_cmd_send_sync(BT_HCI_OP_SET_SLEEPMODE, payload, NULL);
    vfree(payload);
    pr_info("%s end\n", __func__);
    return;
}

void hci_set_scan_parameters(void)
{
    struct HC_BT_HDR* payload = (struct HC_BT_HDR*)vmalloc(sizeof(struct HC_BT_HDR) + 8);
    unsigned char* p;
    p = payload->data;

    pr_info("%s start\n", __func__);
    payload->len = 8;
    UINT8_TO_STREAM(p, 0x00);
    UINT8_TO_STREAM(p, 0x00);
    UINT8_TO_STREAM(p, 0x01);
    UINT8_TO_STREAM(p, 0x01);

    UINT8_TO_STREAM(p, 0x40);
    UINT8_TO_STREAM(p, 0x06); //1000ms

    UINT8_TO_STREAM(p, 0x20);
    UINT8_TO_STREAM(p, 0x02); //340ms

    hex_dump_block(payload->data, payload->len);
    hci_cmd_send_sync(BT_HCI_OP_LE_SET_EX_SCAN_PARAMETERS, payload, NULL);

    vfree(payload);
    pr_info("%s end\n", __func__);
    return;
}

void hci_set_scan_enable(int enable)
{
    struct HC_BT_HDR* payload = (struct HC_BT_HDR*)vmalloc(sizeof(struct HC_BT_HDR) + 6);
    unsigned char* p;
    p = payload->data;

    pr_info("%s start\n", __func__);
    payload->len = 6;
    UINT8_TO_STREAM(p, enable);
    UINT8_TO_STREAM(p, 0x00);
    UINT8_TO_STREAM(p, 0x00);
    UINT8_TO_STREAM(p, 0x00);
    UINT8_TO_STREAM(p, 0x00);
    UINT8_TO_STREAM(p, 0x00);

    hex_dump_block(payload->data, payload->len);
    hci_cmd_send_sync(BT_HCI_OP_LE_SET_EX_SCAN_ENABLE, payload, NULL);

    vfree(payload);
    pr_info("%s end\n", __func__);
    return;
}

void hci_cleanup_wakeup_list(void)
{
    pr_info("%s start\n", __func__);

    hci_cmd_send_sync(BT_HCI_OP_CLEANUP_WAKEUPLIST, NULL, NULL);
    pr_info("%s end\n", __func__);
    return;
}

void hci_add_device_to_wakeup_list(void)
{
    struct HC_BT_HDR* payload = NULL;
    unsigned char* p;
    unsigned char adv_type = 0xff;
    unsigned char address[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    pr_info("%s start %s\n", __func__, WOBLE_TYPE);

    if (!strcmp(WOBLE_TYPE, "demo")) {
        payload = (struct HC_BT_HDR*)vmalloc(sizeof(struct HC_BT_HDR) + 49);
        p = payload->data;
        payload->len = 49;
	UINT8_TO_STREAM(p, ADV_ADDRESS_FROM_WHITTLIST);

        ARRAY_TO_STREAM(p, address, 6);
        UINT16_TO_STREAM(p, WOBLE_WAKE_MOD_SUBADV_FILTER_DATA);
        UINT8_TO_STREAM(p, 0); // modify 0
        UINT8_TO_STREAM(p, 0x03);

        UINT8_TO_STREAM(p, 0x01);
        UINT8_TO_STREAM(p, adv_type);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x04);
        UINT8_TO_STREAM(p, 0x13); // manufacturer Data
        UINT8_TO_STREAM(p, 0xff);
        UINT8_TO_STREAM(p, 0x46);
        UINT8_TO_STREAM(p, 0x00);

        UINT8_TO_STREAM(p, 0x01);
        UINT8_TO_STREAM(p, adv_type);
        UINT8_TO_STREAM(p, 0x04);//0x05
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x06);
        ARRAY_TO_STREAM(p, own_public_addr, 6);

        UINT8_TO_STREAM(p, 0x01);
        UINT8_TO_STREAM(p, adv_type);
        UINT8_TO_STREAM(p, 0x0f);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x00);
        UINT8_TO_STREAM(p, 0x05);
        UINT8_TO_STREAM(p, 0x43);
        UINT8_TO_STREAM(p, 0x52);
        UINT8_TO_STREAM(p, 0x4B);
        UINT8_TO_STREAM(p, 0x54);
        UINT8_TO_STREAM(p, 0x4D);

        UINT16_TO_STREAM(p, 0x00C8); // time
    }
    hex_dump_block(payload->data, payload->len);
    hci_cmd_send_sync(BT_HCI_OP_ADD_WAKEUPLIST, payload, NULL);
    vfree(payload);

    pr_info("%s end\n", __func__);
    return;
}

 void hci_set_ap_start_sleep(void)
 {
    pr_info("%s start\n", __func__);

    hci_cmd_send_sync(BT_HCI_OP_SET_STARTSLEEP, NULL, NULL);
    pr_info("%s end\n", __func__);
    return;
}

void hci_evt_disconnect_complete(void)
{
	unsigned char msg_req[HCI_CMD_MAX_LEN], *p;
	pr_info("%s start\n", __func__);

	p = msg_req;
	UINT8_TO_STREAM(p, HCI_EVT);
	UINT8_TO_STREAM(p, LE_HCI_EVT_CMD_DISCONNECTION_COMPLETE);
	UINT8_TO_STREAM(p, 4);
	UINT8_TO_STREAM(p, STATUS_SUCCESS);
	UINT16_TO_STREAM_SAME(p, woble_switch.conn_handle);
	UINT8_TO_STREAM(p, HCI_ERR_PEER_USER);

	hex_dump_block(msg_req, p-msg_req);
	woble_data_recv((unsigned char *)msg_req, p-msg_req);
	pr_info("%s end\n", __func__);
	return;
}

int bt_tx_powerchange(int channel, int is_resume)
{
	unsigned long power_state = marlin_get_power_state();
	pr_info("%s channel %d is_resume =%d", __func__, channel, is_resume);

	if (strcmp(WOBLE_TYPE, "disable")) {
		pr_info("%s is_resume =%d", __func__, is_resume);

		if (test_bit(MARLIN_BLUETOOTH, &power_state)) {
			if (!is_resume) {
				if (woble_switch.suspend_role) {
					hci_disconnect();
					hci_cleanup_wakeup_list();
					hci_add_device_to_wakeup_list();
					hci_set_scan_parameters();
					hci_set_scan_enable(1);
					pr_info("this is bt suspend.\n");
				} else {
					hci_cleanup_wakeup_list();
					hci_set_scan_enable(0);
				}
				hci_set_ap_sleep_mode(WOBLE_IS_NOT_SHUTDOWN, WOBLE_IS_NOT_RESUME);
				hci_set_ap_start_sleep();
			} else {
				hci_set_ap_sleep_mode(WOBLE_IS_NOT_SHUTDOWN, WOBLE_IS_RESUME);
				hci_set_scan_enable(0);

				if (woble_switch.suspend_role) {
					hci_evt_disconnect_complete();
				}
			}
		}
 	}
	return 0;
}


