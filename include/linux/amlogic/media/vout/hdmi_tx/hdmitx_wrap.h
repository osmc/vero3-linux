/*
 * include/linux/amlogic/media/vout/hdmi_tx/hdmitx_wrap.h
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

#ifndef __HDMITX_WRAP_H__
#define __HDMITX_WRAP_H__

#include <linux/types.h>
#include <asm/ioctl.h>
#include <linux/hdmi.h>
#include <drm/drmP.h>
#include <linux/amlogic/media/amvecm/amvecm.h>

#include "hdmi_common.h"

/**
 * hdmitx_get_hpd_st - return current HPD status
 * Returns:
 * True if HPD is high, else false
 */
bool hdmitx_get_hpd_st(void);

/**
 * hdmitx_get_rawedid - return the raw edid of attached Receiver
 * @raw: the pointer of stored data, should be 512Bytes size, 4 blocks
 * @len: the length of edid data
 *
 * Returns:
 * True if current EDID data is valid, else false
 */
bool hdmitx_get_rawedid(unsigned char *raw, unsigned int *len);

/**
 * hdmitx_get_rxcap - return the parsed edid data
 *
 * Returns:
 * the parsed edid data
 */
struct rx_cap *hdmitx_get_rxcap(void);

#if 0
/**
 * hdmitx_set_instant_packet - set various hdmi packets
 * @type: hdmi infoframe packet
 * @data: the infoframe data, if data is NULL, then disable current packet
 *
 * Return:
 * True if set successful, else false
 */
bool hdmitx_set_instant_packet(enum hdr_type_e type, void *data);


#define AMHDMITX_IOCTL_BASE 'a'
#define AMHDMITX_IO(nr)			_IO(AMHDMITX_IOCTL_BASE, nr)
#define AMHDMITX_IOR(nr, type)		_IOR(AMHDMITX_IOCTL_BASE, nr, type)
#define AMHDMITX_IOW(nr, type)		_IOW(AMHDMITX_IOCTL_BASE, nr, type)
#define AMHDMITX_IOWR(nr, type)		_IOWR(AMHDMITX_IOCTL_BASE, nr, type)

#define AMHDMITX_IOCTL_GETHPDST AMHDMITX_IOR(0x00, hdmitx_get_hpd_st)
#define AMHDMITX_TOCTL_GETRAWEDID AMHDMITX_IOWR(0x01, hdmitx_get_rawedid)
#define AMHDMITX_TOCTL_GETRXCAP AMHDMITX_IOR(0x02, hdmitx_get_rxcap)
#define AMHDMITX_TOCTL_SETCSCD AMHDMITX_IOWR(0x03, hdmitx_set_cscd)
#define AMHDMITX_TOCTL_SETRESOLUTION AMHDMITX_IOWR(0x04, hdmitx_set_resolution)
#define AMHDMITX_TOCTL_STARTHDCP AMHDMITX_IOR(0x05, hdmitx_start_hdcp)
#define AMHDMITX_TOCTL_STOPHDCP AMHDMITX_IOR(0x06, hdmitx_stop_hdcp)
#define AMHDMITX_TOCTL_GETHDCPAUTH
	AMHDMITX_IOR(0x07, hdmitx_get_hdcp_authenticated)
#define AMHDMITX_TOCTL_SETPKT AMHDMITX_IOWR(0x08, hdmitx_set_instant_packet)
#endif

#endif

