/*
 * drivers/amlogic/dvb/demux/aml_dsc.h
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

#ifndef _AML_DSC_H_
#define _AML_DSC_H_

#include "sw_demux/swdemux.h"
#include "dvbdev.h"
#include <dmxdev.h>
#include <linux/device.h>

#define MAX_DSC_CHAN_NUM  20

struct dsc_channel;

struct aml_dsc {
	struct dvb_device *dev;
	int id;

	enum dmx_input_source source;
	int demod_sid;
	int local_sid;

	struct dsc_channel *dsc_channels;
	/*protect many user operate*/
	struct mutex mutex;
//	spinlock_t slock;
};

int dsc_init(struct aml_dsc *dsc, struct dvb_adapter *dvb_adapter);
void dsc_release(struct aml_dsc *dsc);
int dsc_set_source(int id, int source);
int dsc_set_sid(int id, int source, int sid);
int dsc_dump_info(char *buf);
#endif
