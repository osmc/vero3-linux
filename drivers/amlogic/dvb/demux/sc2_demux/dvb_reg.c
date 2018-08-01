/*
 * drivers/amlogic/dvb/demux/sc2_demux/dvb_reg.c
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

#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "ts_output_reg.h"
#include "dsc_reg.h"
#include "demod_reg.h"
#include "mem_desc_reg.h"
#include "../key_reg.h"
#include "../dmx_log.h"

#define pr_verify(fmt, args...)   \
	dprintk(LOG_VER, debug_rw, "reg:" fmt, ## args)
#define pr_dbg(fmt, args...)      \
	dprintk(LOG_DBG, debug_rw, "reg:" fmt, ## args)
#define dprint(fmt, args...)     \
	dprintk(LOG_ERROR, debug_rw, "reg:" fmt, ## args)

MODULE_PARM_DESC(debug_rw, "\n\t\t Enable rw information");
static int debug_rw;
module_param(debug_rw, int, 0644);

static void *p_hw_base;

void aml_write_self(unsigned int reg, unsigned int val)
{
	void *ptr = (void *)(p_hw_base + reg);

	pr_dbg("write addr:%lx, value:0x%0x\n", (unsigned long)ptr, val);
	writel(val, ptr);
	if (debug_rw & 0x2) {
		int value;

		if (reg == PID_RDY ||
		    reg == TSN_PID_READY ||
		    reg == TSD_PID_READY ||
		    reg == TSE_PID_READY ||
		    reg == KT_REE_RDY ||
		    ((reg >= TS_DMA_RCH_READY(0)) &&
		     (reg <= TS_DMA_RCH_READY(31))) ||
		    ((reg >= TS_DMA_WCH_READY(0) &&
		      (reg <= TS_DMA_WCH_READY(127)))))
			return;
		value = readl(ptr);
		pr_verify("write addr:%lx, org v:0x%0x, ret v:0x%0x\n",
			  (unsigned long)ptr, val, value);
	}
}

int aml_read_self(unsigned int reg)
{
	void *addr = p_hw_base + reg;
	int ret = readl(addr);

	pr_dbg("read addr:%lx, value:0x%0x\n", (unsigned long)addr, ret);
	return ret;
}

int init_demux_addr(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dprint("%s fail\n", __func__);
		return -1;
	}

	p_hw_base = devm_ioremap_nocache(&pdev->dev, res->start,
					 resource_size(res));
	if (p_hw_base) {
		dprint("%s base addr = %lx\n", __func__,
		       (unsigned long)p_hw_base);
	} else {
		dprint("%s base addr error\n", __func__);
	}
	return 0;
}
