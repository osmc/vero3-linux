/*
 * include/linux/amlogic/media/vout/hdmi_tx/hdmi_hdcp.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __HDMI_HDCP_H__
#define __HDMI_HDCP_H__
#include <linux/wait.h>

enum hdcp_ver_e {
	HDCPVER_NONE = 0,
	HDCPVER_14,
	HDCPVER_22,
};

#define MAX_KSV_LISTS 127
struct hdcprp14_topo {
	unsigned char max_cascade_exceeded:1;
	unsigned char depth:3;
	unsigned char rsvd : 4;
	unsigned char max_devs_exceeded:1;
	unsigned char device_count:7; /* 1 ~ 127 */
	unsigned char ksv_list[MAX_KSV_LISTS * 5];
} __packed;

struct hdcprp22_topo {
	unsigned int depth;
	unsigned int device_count;
	unsigned int v1_X_device_down;
	unsigned int v2_0_repeater_down;
	unsigned int max_devs_exceeded;
	unsigned int max_cascade_exceeded;
	unsigned char id_num;
	unsigned char id_lists[MAX_KSV_LISTS * 5];
};

struct hdcprp_topo {
	/* hdcp_ver currently used */
	enum hdcp_ver_e hdcp_ver;
	union {
		struct hdcprp14_topo topo14;
		struct hdcprp22_topo topo22;
	} topo;
};

struct hdcp_obs_val {
	unsigned char obs0;
	unsigned char obs1;
	unsigned char obs2;
	unsigned char obs3;
	unsigned char intstat;
};

#define HDCP_LOG_SIZE 4096 /* 4KB */
struct hdcplog_buf {
	unsigned int wr_pos;
	unsigned int rd_pos;
	wait_queue_head_t wait;
	unsigned char buf[HDCP_LOG_SIZE];
};

#endif
