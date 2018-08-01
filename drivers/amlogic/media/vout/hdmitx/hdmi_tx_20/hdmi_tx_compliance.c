/*
 * drivers/amlogic/media/vout/hdmitx/hdmi_tx_20/hdmi_tx_compliance.c
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

#include <linux/types.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
/* Base Block, Vendor/Product Information, byte[8]~[17] */
struct edid_venddat_t {
	unsigned char data[10];
};

static struct edid_venddat_t vendor_6g[] = {
	/* SAMSUNG UA55KS7300JXXZ */
	{ {0x4c, 0x2d, 0x3b, 0x0d, 0x00, 0x06, 0x00, 0x01, 0x01, 0x1a} }
	/* Add new vendor data here */
};

static struct edid_venddat_t vendor_ratio[] = {
	/* Mi L55M2-AA */
	{ {0x61, 0xA4, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x19} }
	/* Add new vendor data here */
};

bool hdmitx_find_vendor_6g(struct hdmitx_dev *hdev)
{
	int i;

	if (!hdev || !hdev->edid_ptr)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_6g); i++) {
		if (memcmp(&hdev->edid_ptr[8], vendor_6g[i].data,
			   sizeof(vendor_6g[i].data)) == 0)
			return true;
	}
	return false;
}

/* need to forcely change clk raito for such TV when suspend/resume box */
bool hdmitx_find_vendor_ratio(struct hdmitx_dev *hdev)
{
	int i;

	if (!hdev || !hdev->edid_ptr)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_ratio); i++) {
		if (memcmp(&hdev->edid_ptr[8], vendor_ratio[i].data,
			   sizeof(vendor_ratio[i].data)) == 0)
			return true;
	}
	return false;
}

