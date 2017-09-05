/*
 * drivers/amlogic/tvin/viu/viuin.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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


#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/module.h>

#include <linux/amlogic/amports/amstream.h>
#include <linux/amlogic/amports/ptsserv.h>
#include <linux/amlogic/amports/vframe.h>
#include <linux/amlogic/amports/vframe_provider.h>
#include <linux/amlogic/vout/vout_notify.h>
#include <linux/amlogic/tvin/tvin_v4l2.h>
#include "../tvin_global.h"
#include "../tvin_frontend.h"
#include "../tvin_format_table.h"

#define DEVICE_NAME "viuin"
#define MODULE_NAME "viuin"

#define AMVIUIN_DEC_START       1
#define AMVIUIN_DEC_STOP        0


/* register */
#define VIU_MISC_CTRL1 0x1a07
#define VPU_VIU_VENC_MUX_CTRL 0x271a
#define ENCI_INFO_READ 0x271c
#define ENCP_INFO_READ 0x271d
#define ENCT_INFO_READ 0x271e
#define ENCL_INFO_READ 0x271f
#define VPU_VIU2VDIN_HDN_CTRL 0x2780
/*follow two reg new add from txlx*/
/*VPU_422TO444_CTRL0/VPU_422TO444_CTRL1:
[31] : bypass_mode
[30] : cbcr_sel
[29] : clip 10 mode
[28] : clip 8 mode
[27] : value data in the little endian
[26] : tunnel mode
[25] : go field enable
[24] : go line enable
[23] : soft reset enable
[22] : de mode
[17:0] : tunnel channel select
*/
#define VPU_422TO444_CTRL0 0x274b
#define VPU_422TO444_CTRL1 0x274c
#define VPU_422TO444_RST 0x274a

static unsigned int vsync_enter_line_curr;
module_param(vsync_enter_line_curr, uint, 0664);
MODULE_PARM_DESC(vsync_enter_line_curr,
	"\n encoder process line num when enter isr.\n");

static unsigned int vsync_enter_line_max;
module_param(vsync_enter_line_max, uint, 0664);
MODULE_PARM_DESC(vsync_enter_line_max,
	"\n max encoder process line num when enter isr.\n");

static unsigned int vsync_enter_line_max_threshold = 10000;
module_param(vsync_enter_line_max_threshold, uint, 0664);
MODULE_PARM_DESC(vsync_enter_line_max_threshold,
	"\n max encoder process line num over threshold drop the frame.\n");

static unsigned int vsync_enter_line_min_threshold = 10000;
module_param(vsync_enter_line_min_threshold, uint, 0664);
MODULE_PARM_DESC(vsync_enter_line_min_threshold,
	"\n max encoder process line num less threshold drop the frame.\n");
static unsigned int vsync_enter_line_threshold_overflow_count;
module_param(vsync_enter_line_threshold_overflow_count, uint, 0664);
MODULE_PARM_DESC(vsync_enter_line_threshold_overflow_count,
	"\ncnt overflow encoder process line no over threshold drop the frame\n");

static unsigned short v_cut_offset;
module_param(v_cut_offset, ushort, 0664);
MODULE_PARM_DESC(v_cut_offset, "the cut window vertical offset for viuin");

static unsigned short open_cnt;
module_param(open_cnt, ushort, 0664);
MODULE_PARM_DESC(open_cnt, "open_cnt for vdin0/1");

static unsigned short scramble_mode;
module_param(scramble_mode, ushort, 0664);
MODULE_PARM_DESC(scramble_mode, "scramble_mode for viu_422to444 vencp");

static unsigned int vpu_422to444_en;
module_param(vpu_422to444_en, uint, 0664);
MODULE_PARM_DESC(vpu_422to444_en, "viu_422to444 en/dis");

/*0x2442b310 is for video only data to viu default setting*/
static unsigned int vpu_422to444_val = 0x2442b310;
module_param(vpu_422to444_val, uint, 0664);
MODULE_PARM_DESC(vpu_422to444_val, "viu_422to444 val");

struct viuin_s {
	unsigned int flag;
	struct vframe_prop_s *prop;
	/*add for tvin frontend*/
	struct tvin_frontend_s frontend;
	struct vdin_parm_s parm;
	unsigned int enc_info_addr;
};


static inline uint32_t rd_viu(uint32_t reg)
{
	return (uint32_t)aml_read_vcbus(reg);
}

static inline void wr_viu(uint32_t reg,
				 const uint32_t val)
{
	aml_write_vcbus(reg, val);
}

static inline void wr_bits_viu(uint32_t reg,
				    const uint32_t value,
				    const uint32_t start,
				    const uint32_t len)
{
	aml_write_vcbus(reg, ((aml_read_vcbus(reg) &
			     ~(((1L << (len)) - 1) << (start))) |
			    (((value) & ((1L << (len)) - 1)) << (start))));
}

static inline uint32_t rd_bits_viu(uint32_t reg,
				    const uint32_t start,
				    const uint32_t len)
{
	uint32_t val;

	val = ((aml_read_vcbus(reg) >> (start)) & ((1L << (len)) - 1));

	return val;
}

static int viuin_support(struct tvin_frontend_s *fe, enum tvin_port_e port)
{
	if (port == TVIN_PORT_VIU || port == TVIN_PORT_VIDEO)
		return 0;
	else
		return -1;
}
void viuin_check_venc_line(struct viuin_s *devp_local)
{
	unsigned int vencv_line_cur, cnt;
	cnt = 0;
	do {
		vencv_line_cur = (rd_viu(devp_local->enc_info_addr)>>16)&0x1fff;
		udelay(10);
		cnt++;
		if (cnt > 100000)
			break;
	} while (vencv_line_cur != 1);
	if (vencv_line_cur != 1)
		pr_info("**************%s,vencv_line_cur:%d,cnt:%d***********\n",
				__func__, vencv_line_cur, cnt);
}
static int viuin_open(struct tvin_frontend_s *fe, enum tvin_port_e port)
{
	struct viuin_s *devp = container_of(fe, struct viuin_s, frontend);
	unsigned int temp_val, viu_mux = 0;

	if (!memcpy(&devp->parm, fe->private_data,
			sizeof(struct vdin_parm_s))) {
		pr_info("[viuin..]%s memcpy error.\n", __func__);
		return -1;
	}
	/*open the venc to vdin path*/
	switch (rd_bits_viu(VPU_VIU_VENC_MUX_CTRL, 0, 2)) {
	case 0:
		viu_mux = 0x8;/* wr_bits(VPU_VIU_VENC_MUX_CTRL,0x88,4,8); */
		devp->enc_info_addr = ENCL_INFO_READ;
		break;
	case 1:
		viu_mux = 0x1;/* wr_bits(VPU_VIU_VENC_MUX_CTRL,0x11,4,8); */
		devp->enc_info_addr = ENCI_INFO_READ;
		break;
	case 2:
		viu_mux = 0x2;/* wr_bits(VPU_VIU_VENC_MUX_CTRL,0x22,4,8); */
		devp->enc_info_addr = ENCP_INFO_READ;
		break;
	case 3:
		viu_mux = 0x4;/* wr_bits(VPU_VIU_VENC_MUX_CTRL,0x44,4,8); */
		devp->enc_info_addr = ENCT_INFO_READ;
		break;
	default:
		break;
	}
	viuin_check_venc_line(devp);
	if (port == TVIN_PORT_VIDEO) {
		/* enable hsync for vdin loop */
		wr_bits_viu(VIU_MISC_CTRL1, 1, 28, 1);
		viu_mux = 0x4;
	} else {
		if (is_meson_gxbb_cpu() || is_meson_gxm_cpu() ||
				is_meson_gxl_cpu()) {
			if (devp->parm.v_active == 2160 &&
				devp->parm.frame_rate > 30)
				/* 1/2 down scaling */
				wr_viu(VPU_VIU2VDIN_HDN_CTRL, 0x40f00);
			}
	}
	/*txlx new add 422to444 module config*/
	if (is_meson_txlx_cpu()) {
		/*viu_422to444 vencp*/
		if ((viu_mux == 2) && (port == TVIN_PORT_VIU))
			temp_val = VPU_422TO444_CTRL0;
		else if (port == TVIN_PORT_VIDEO)/*viu_422to444 vd1*/
			temp_val = VPU_422TO444_CTRL1;
		else
			temp_val = 0;
		if ((vpu_422to444_en == 0) && (temp_val != 0)) {
			wr_bits_viu(temp_val, 1, 22, 1);
			wr_bits_viu(temp_val, 1, 31, 1);
			wr_bits_viu(temp_val, 1, 25, 1);
		} else if ((vpu_422to444_en == 1) && (temp_val != 0))
			wr_viu(temp_val, vpu_422to444_val);
		/*1:select vd1 high 10 bits data 0:select vd1 low 10bits data*/
		/*wr_bits_viu(VPU_422TO444_RST, 0, 3, 1);*/
	}

	wr_bits_viu(VPU_VIU_VENC_MUX_CTRL, viu_mux, 4, 4);
	wr_bits_viu(VPU_VIU_VENC_MUX_CTRL, viu_mux, 8, 4);
	devp->flag = 0;
	open_cnt++;
	return 0;
}
static void viuin_close(struct tvin_frontend_s *fe)
{
	struct viuin_s *devp = container_of(fe, struct viuin_s, frontend);

	viuin_check_venc_line(devp);
	memset(&devp->parm, 0, sizeof(struct vdin_parm_s));
	/*close the venc to vdin path*/
	if (open_cnt)
		open_cnt--;
	if (open_cnt == 0) {
		wr_bits_viu(VPU_VIU_VENC_MUX_CTRL, 0, 8, 4);
		wr_bits_viu(VPU_VIU_VENC_MUX_CTRL, 0, 4, 4);
	}
	if (rd_viu(VPU_VIU2VDIN_HDN_CTRL) != 0)
		wr_viu(VPU_VIU2VDIN_HDN_CTRL, 0x0);
	/*txlx new add 422to444 module config*/
	if (is_meson_txlx_cpu()) {
		wr_viu(VPU_422TO444_CTRL0, 0);
		wr_viu(VPU_422TO444_CTRL1, 0);
		wr_viu(VPU_422TO444_RST, 0);
	}
}

static void viuin_start(struct tvin_frontend_s *fe, enum tvin_sig_fmt_e fmt)
{
	/* do something the same as start_amvdec_viu_in */
	struct viuin_s *devp = container_of(fe, struct viuin_s, frontend);

	if (devp->flag && AMVIUIN_DEC_START) {
		pr_info("[viuin..]%s viu_in is started already.\n", __func__);
		return;
	}
	vsync_enter_line_max = 0;
	vsync_enter_line_threshold_overflow_count = 0;
	devp->flag = AMVIUIN_DEC_START;

	return;
}

static void viuin_stop(struct tvin_frontend_s *fe, enum tvin_port_e port)
{
	struct viuin_s *devp = container_of(fe, struct viuin_s, frontend);

	if (devp->flag && AMVIUIN_DEC_START)
		devp->flag |= AMVIUIN_DEC_STOP;
	else
		pr_info("[viuin..]%s viu in dec isn't start.\n", __func__);
}

static int viuin_isr(struct tvin_frontend_s *fe, unsigned int hcnt64)
{
	int curr_port;

	struct viuin_s *devp = container_of(fe, struct viuin_s, frontend);

	curr_port = rd_bits_viu(VPU_VIU_VENC_MUX_CTRL, 0, 2);

	vsync_enter_line_curr = (rd_viu(devp->enc_info_addr)>>16)&0x1fff;
	if (vsync_enter_line_curr > vsync_enter_line_max)
		vsync_enter_line_max = vsync_enter_line_curr;
	if ((vsync_enter_line_max_threshold > vsync_enter_line_min_threshold) &&
			(curr_port == 0)) {
		if ((vsync_enter_line_curr > vsync_enter_line_max_threshold) ||
		(vsync_enter_line_curr < vsync_enter_line_min_threshold)) {
			vsync_enter_line_threshold_overflow_count++;
			return TVIN_BUF_SKIP;
		}
	}
	return 0;
}

static struct tvin_decoder_ops_s viu_dec_ops = {
	.support            = viuin_support,
	.open               = viuin_open,
	.start              = viuin_start,
	.stop               = viuin_stop,
	.close              = viuin_close,
	.decode_isr         = viuin_isr,
};

static void viuin_sig_propery(struct tvin_frontend_s *fe,
		struct tvin_sig_property_s *prop)
{
	static const struct vinfo_s *vinfo;
	struct viuin_s *devp = container_of(fe, struct viuin_s, frontend);
	if (devp->parm.port == TVIN_PORT_VIDEO)
		prop->color_format = TVIN_YUV444;
	else {
		vinfo = get_current_vinfo();
		prop->color_format = vinfo->viu_color_fmt;
	}
	prop->dest_cfmt = devp->parm.dfmt;

	prop->scaling4w = devp->parm.dest_hactive;
	prop->scaling4h = devp->parm.dest_vactive;

	prop->vs = v_cut_offset;
	prop->ve = 0;
	prop->hs = 0;
	prop->he = 0;
	prop->decimation_ratio = 0;
}

static bool viu_check_frame_skip(struct tvin_frontend_s *fe)
{
	struct viuin_s *devp = container_of(fe, struct viuin_s, frontend);

	if (devp->parm.skip_count > 0) {
		devp->parm.skip_count--;
		return true;
	}

	return false;
}

static struct tvin_state_machine_ops_s viu_sm_ops = {
	.get_sig_propery = viuin_sig_propery,
	.check_frame_skip = viu_check_frame_skip,
};

static int viuin_probe(struct platform_device *pdev)
{
	struct viuin_s *viuin_devp;

	viuin_devp = kmalloc(sizeof(struct viuin_s), GFP_KERNEL);
	if (!viuin_devp) {
		pr_info("[viuin..]%s kmalloc error.\n", __func__);
		return -ENOMEM;
	}
	memset(viuin_devp, 0, sizeof(struct viuin_s));
	sprintf(viuin_devp->frontend.name, "%s", DEVICE_NAME);
	if (!tvin_frontend_init(&viuin_devp->frontend,
		&viu_dec_ops, &viu_sm_ops, 0)) {
		if (tvin_reg_frontend(&viuin_devp->frontend))
			pr_info("[viuin..]%s register viu frontend error.\n",
					__func__);
	}
	platform_set_drvdata(pdev, viuin_devp);
	pr_info("[viuin..]%s probe ok.\n", __func__);
	return 0;
}

static int viuin_remove(struct platform_device *pdev)
{
	struct viuin_s *devp = platform_get_drvdata(pdev);
	if (devp) {
		tvin_unreg_frontend(&devp->frontend);
		kfree(devp);
	}
	return 0;
}

static struct platform_driver viuin_driver = {
	.probe	= viuin_probe,
	.remove	= viuin_remove,
	.driver	= {
		.name	= DEVICE_NAME,
	}
};

static struct platform_device *viuin_device;

static int __init viuin_init_module(void)
{
	pr_info("[viuin..]%s viuin module init\n", __func__);
	viuin_device = platform_device_alloc(DEVICE_NAME, 0);
	if (!viuin_device) {
		pr_err("[viuin..]%s failed to alloc viuin_device.\n",
				__func__);
		return -ENOMEM;
	}

	if (platform_device_add(viuin_device)) {
		platform_device_put(viuin_device);
		pr_err("[viuin..]%sfailed to add viuin_device.\n", __func__);
		return -ENODEV;
	}
	if (platform_driver_register(&viuin_driver)) {
		pr_err("[viuin..]%sfailed to register viuin driver.\n",
				__func__);
		platform_device_del(viuin_device);
		platform_device_put(viuin_device);
		return -ENODEV;
	}

	return 0;
}

static void __exit viuin_exit_module(void)
{
	pr_info("[viuin..]%s viuin module remove.\n", __func__);
	platform_driver_unregister(&viuin_driver);
		platform_device_unregister(viuin_device);
	return;
}


module_init(viuin_init_module);
module_exit(viuin_exit_module);
MODULE_DESCRIPTION("AMLOGIC viu input driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("3.0.0");
