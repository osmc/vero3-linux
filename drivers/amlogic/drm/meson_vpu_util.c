/*
 * drivers/amlogic/drm/meson_vpu_util.c
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

#include <linux/ktime.h>
#include "meson_vpu_util.h"
#include "meson_vpu_reg.h"
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
#include <linux/amlogic/media/rdma/rdma_mgr.h>
#endif

/*****drm reg access by rdma*****/
/*one item need two u32 = 8byte,total 512 item is enough for vpu*/
#define MESON_VPU_RDMA_TABLE_SIZE (512 * 8)

#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
static int meson_vpu_reg_handle;
static void meson_vpu_vsync_rdma_irq(void *arg)
{
	if (meson_vpu_reg_handle == -1)
		return;
}

static struct rdma_op_s meson_vpu_vsync_rdma_op = {
	meson_vpu_vsync_rdma_irq,
	NULL
};

void meson_vpu_reg_handle_register(void)
{
	meson_vpu_reg_handle = rdma_register(&meson_vpu_vsync_rdma_op,
					     NULL, MESON_VPU_RDMA_TABLE_SIZE);
}

/*suggestion: call this after atomic done*/
int meson_vpu_reg_vsync_config(void)
{
	return rdma_config(meson_vpu_reg_handle, RDMA_TRIGGER_VSYNC_INPUT);
}

static int meson_vpu_get_active_begin_line(u32 viu_index)
{
	int active_line_begin;
	u32 enc_sel;

	if (viu_index == 1)
		enc_sel = (aml_read_vcbus(VPU_VIU_VENC_MUX_CTRL) >> 2) & 0x3;
	else
		enc_sel = aml_read_vcbus(VPU_VIU_VENC_MUX_CTRL) & 0x3;
	switch (enc_sel) {
	case 0:
		active_line_begin =
			aml_read_vcbus(ENCL_VIDEO_VAVON_BLINE);
		break;
	case 1:
		active_line_begin =
			aml_read_vcbus(ENCI_VFIFO2VD_LINE_TOP_START);
		break;
	case 2:
		active_line_begin =
			aml_read_vcbus(ENCP_VIDEO_VAVON_BLINE);
		break;
	case 3:
		active_line_begin =
			aml_read_vcbus(ENCT_VIDEO_VAVON_BLINE);
		break;
	}

	return active_line_begin;
}

static int meson_vpu_get_enter_encp_line(int viu_index)
{
	int enc_line = 0;
	unsigned int reg = 0;
	u32 enc_sel;

	if (viu_index == 1)
		enc_sel = (aml_read_vcbus(VPU_VIU_VENC_MUX_CTRL) >> 2) & 0x3;
	else
		enc_sel = aml_read_vcbus(VPU_VIU_VENC_MUX_CTRL) & 0x3;
	switch (enc_sel) {
	case 0:
		reg = aml_read_vcbus(ENCL_INFO_READ);
		break;
	case 1:
		reg = aml_read_vcbus(ENCI_INFO_READ);
		break;
	case 2:
		reg = aml_read_vcbus(ENCP_INFO_READ);
		break;
	case 3:
		reg = aml_read_vcbus(ENCT_INFO_READ);
		break;
	}
	enc_line = (reg >> 16) & 0x1fff;

	return enc_line;
}

void meson_vpu_line_check(int viu_index, int vdisplay)
{
	int active_begin_line, wait_cnt, cur_line;

	active_begin_line = meson_vpu_get_active_begin_line(viu_index);
	cur_line = meson_vpu_get_enter_encp_line(viu_index);
	/* if nearly vsync signal, wait vsync here */
	wait_cnt = 0;
	while (cur_line >= vdisplay + active_begin_line *
	       (100 - LINE_THRESHOLD) / 100 ||
	       cur_line <= active_begin_line * LINE_THRESHOLD / 100) {
		DRM_DEBUG("enc line=%d\n", cur_line);
		/* 0.5ms */
		usleep_range(500, 600);
		wait_cnt++;
		if (wait_cnt >= WAIT_CNT_MAX) {
			DRM_DEBUG("time out\n");
			break;
		}
		cur_line = meson_vpu_get_enter_encp_line(viu_index);
	}
}
#endif
u32 meson_vpu_read_reg(u32 addr)
{
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	return rdma_read_reg(meson_vpu_reg_handle, addr);
#else
	return aml_read_vcbus(addr);
#endif
}

int meson_vpu_write_reg(u32 addr, u32 val)
{
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	return rdma_write_reg(meson_vpu_reg_handle, addr, val);
#else
	aml_write_vcbus(addr, val);
	return 0;
#endif
}

int meson_vpu_write_reg_bits(u32 addr, u32 val, u32 start, u32 len)
{
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	return rdma_write_reg_bits(meson_vpu_reg_handle,
				   addr, val, start, len);
#else
	aml_vcbus_update_bits(adr, ((1 << len) - 1) << start, val << start);
	return 0;
#endif
}

/** reg direct access without rdma **/
u32 meson_drm_read_reg(u32 addr)
{
	u32 val;

	val = aml_read_vcbus(addr);

	return val;
}

void meson_drm_write_reg(u32 addr, u32 val)
{
	aml_write_vcbus(addr, val);
}

/** canvas config  **/

void meson_drm_canvas_config(u32 index, unsigned long addr, u32 width,
			     u32 height, u32 wrap, u32 blkmode)
{
	canvas_config(index, addr, width, height, wrap, blkmode);
}

int meson_drm_canvas_pool_alloc_table(const char *owner, u32 *table, int size,
				      enum canvas_map_type_e type)
{
	return canvas_pool_alloc_canvas_table(owner, table, size, type);
}

