/*
 * drivers/amlogic/media/vout/hdmitx/hdmi_tx_20/wrap/hdmitx_inc.h
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

#ifndef __HDMITX_INC__
#define __HDMITX_INC__

/**
 *
 */
void hdmitx_wrap_init(struct hdmitx_dev *hdev);

ssize_t _show_hdmi_new_control(struct device *dev,
			       struct device_attribute *attr,
			       char *buf);
int hdmitx_module_disable(enum vmode_e cur_vmod);

#endif
