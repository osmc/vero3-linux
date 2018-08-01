/*
 * drivers/amlogic/media/vout/hdmitx/hdmi_tx_20/wrap/hdmitx_wrap.c
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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/ctype.h>
#include <linux/extcon.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/reboot.h>
#include <linux/extcon.h>
#include <linux/i2c.h>

#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmitx_wrap.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_common.h>

#include "hdmitx_inc.h"
#include "../hw/common.h"

static struct hdmitx_dev *hdmi_tx_dev;
#if 0
static void hdmitx_adjust_output_fmt_by_dv_policy(void);
{
	/* todo: hdmi resolution or colorattribute adjust */
}

static void set_dv_hdr_policy_node(void);
{
	/* todo: set HDR/DV enable, and policy sysfs
	 * as setDolbyVisionEnable() in systemcontrol
	 */
}
#endif
void setup_hdmi_set_inf(struct hdmitx_inf_param *psrc_param)
{
	struct hdmitx_inf_param *pdst_param;

	pdst_param = &hdmi_tx_dev->hdmi_set_inf;
	memcpy(pdst_param, psrc_param, sizeof(*psrc_param));
	memcpy(pdst_param->color_attr,
		   hdmi_tx_dev->fmt_attr,
		   sizeof(pdst_param->color_attr));
	pr_info("[%s]: vic:%d attr:%s frpolicy:%d hdcp_mode:%d hdr_mode:%d\n",
		__func__, pdst_param->vic,
		pdst_param->color_attr, pdst_param->frac_rate_policy,
		pdst_param->hdcp_mode, pdst_param->hdr_mode);
}
EXPORT_SYMBOL(setup_hdmi_set_inf);

ssize_t _show_hdmi_new_control(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_inf_param *phdmi_set_inf;
	struct hdmi_format_para *phdmi_fmt_para;
	char disp_mode[32];

	phdmi_set_inf = &hdmi_tx_dev->hdmi_set_inf;
	phdmi_fmt_para =
		hdmi_get_fmt_paras((enum hdmi_vic)phdmi_set_inf->vic);
	memcpy(disp_mode,
		phdmi_fmt_para->hdmitx_vinfo.name, sizeof(disp_mode));
	strncpy(buf + pos, disp_mode, strlen(disp_mode));
	pos += strlen(disp_mode);
	buf[pos] = ';';
	strncpy(buf + pos, phdmi_set_inf->color_attr,
		strlen(phdmi_set_inf->color_attr));
	pos += strlen(phdmi_set_inf->color_attr);
	buf[pos] = ';';
	pos += snprintf(buf + pos, PAGE_SIZE,
		"frac_rate_policy:%u\thdcp_mode:%u\thdr_mode:%u\n",
		phdmi_set_inf->frac_rate_policy,
		phdmi_set_inf->hdcp_mode,
		phdmi_set_inf->hdr_mode);
	return pos;
}

static bool hdmitx_check_hdr_support(struct hdmitx_dev *hdev,
				     unsigned int hdr_mode)
{
	struct rx_cap *prxcap;
	struct dv_info *pdv_info;
	struct hdr10_plus_info *phdr10p_info;
	bool rtn_val;

	rtn_val = false;
	if (!hdev)
		return rtn_val;

	prxcap = &hdev->rxcap;
	pdv_info = &prxcap->dv_info;
	phdr10p_info = &prxcap->hdr10plus_info;
	switch (hdr_mode) {
	case EOTF_T_NULL:
		rtn_val = true;
		break;
	case EOTF_T_DOLBYVISION:
		if ((pdv_info->ieeeoui == DV_IEEE_OUI) &&
			(pdv_info->block_flag == CORRECT))
			rtn_val = true;
		break;
	case EOTF_T_HDR10:
		if (prxcap->hdr_sup_eotf_smpte_st_2084)
			rtn_val = true;
		break;
	case EOTF_T_SDR:
		rtn_val = true;
		break;
	case EOTF_T_LL_MODE:
		if ((pdv_info->ieeeoui == DV_IEEE_OUI) &&
		    (pdv_info->block_flag == CORRECT) &&
		    (pdv_info->low_latency == 1))
			rtn_val = true;
		break;
	case EOTF_T_HDR10PLUS:
		if ((phdr10p_info->ieeeoui == HDR10_PLUS_IEEE_OUI) &&
		    (phdr10p_info->application_version != 0xFF))
			rtn_val = true;
		break;
	default:
		rtn_val = true;
	}
	if (rtn_val == true)
		pr_info("[%s]: HDR support correctly\n", __func__);
	else
		pr_info("[%s]: HDR support error. hdr_mode:%d\n",
			__func__, hdr_mode);
	return rtn_val;
}

static void analysize_attr(struct hdmitx_inf_param *phdmi_set_inf,
			   struct hdmi_format_para *para)
{
	if (strstr(phdmi_set_inf->color_attr, "420"))
		para->cs = COLORSPACE_YUV420;
	else if (strstr(phdmi_set_inf->color_attr, "444"))
		para->cs = COLORSPACE_YUV444;
	else if (strstr(phdmi_set_inf->color_attr, "422"))
		para->cs = COLORSPACE_YUV422;
	else
		para->cs = COLORSPACE_RGB444;

	if (strstr(phdmi_set_inf->color_attr, "16bit"))
		para->cd = COLORDEPTH_48B;
	else if (strstr(phdmi_set_inf->color_attr, "12bit"))
		para->cd = COLORDEPTH_36B;
	else if (strstr(phdmi_set_inf->color_attr, "10bit"))
		para->cd = COLORDEPTH_30B;
	else
		para->cd = COLORDEPTH_24B;
}
/*
 * TV can support the param or not by EDID check,
 * include disp_mode + colorattribute + hdr/dv mode.
 * setting param is recorded in hdmi_tx_dev->hdmi_set_inf
 */
static bool hdmitx_check_rx_support(void)
{
	struct hdmi_format_para *para = NULL;
	struct hdmitx_inf_param *phdmi_set_inf = &hdmi_tx_dev->hdmi_set_inf;
	bool valid;

	/* 1. display_mode + color_attr support */
	para = hdmi_get_fmt_paras(phdmi_set_inf->vic);
	if (para) {
		pr_info(SYS "vic = %d\n", phdmi_set_inf->vic);
		pr_info(SYS "sname = %s\n", para->sname);
		pr_info(SYS "char_clk = %d\n", para->tmds_clk);
		analysize_attr(phdmi_set_inf, para);
		pr_info(SYS "cd = %d\n", para->cd);
		pr_info(SYS "cs = %d\n", para->cs);
	}
	valid = hdmitx_edid_check_valid_mode(hdmi_tx_dev, para);
	if (!valid) {
		pr_info("disp_mode + colorattribute not supported\n");
		return false;
	}
	/* 2. hdr/dv support */
	valid = hdmitx_check_hdr_support(hdmi_tx_dev, phdmi_set_inf->hdr_mode);
	if (!valid) {
		pr_info("hdr mode not supported\n");
		return false;
	}

	return true;
}
#if 0
static int hdmitx_send_pkt_new(void)
{
	/*todo send packets, such as avi,vsif,drm and so on*/
	/*setting param is recorded in hdmi_tx_dev->hdmi_set_inf*/
	return 1;
}

/* todo: */
void callback_send_drm_dv_hdr10p_pkt(struct hdmitx_inf_param *phdmi_set_inf)
{

}
#endif
static void hdmitx_sleep_frames(char *disp_mode, unsigned int frame_num)
{
	unsigned int vsync_period_ms;
	struct vinfo_s *info;
	unsigned int tmp;
	unsigned int sleep_time_us;

	/*Compute the time of one frame*/
	info = hdmi_get_valid_vinfo(disp_mode);
	tmp = info->sync_duration_num * 1000;
	tmp = tmp / info->sync_duration_den;
	vsync_period_ms = (1000000 + tmp / 2) / tmp;/*tmp/2->round*/
	sleep_time_us = vsync_period_ms * frame_num * 1000;
	usleep_range(sleep_time_us, sleep_time_us + 1000);
}

/* use to hdmi resolution/colorspace/colordepth or DV mode change */
static int new_set_hdmitx_mode(struct hdmitx_inf_param *phdmi_set_inf)
{
	char disp_mode[32];
	struct hdmi_format_para *pfmt_param;
	unsigned int copy_len;
	int cmd;

	pr_info("[%s]\n", __func__);
	/* 1st: set avmute */
	cmd = SET_AVMUTE;
	hdmi_tx_dev->hwop.cntlmisc(hdmi_tx_dev, MISC_AVMUTE_OP, cmd);

	pfmt_param = hdmi_get_fmt_paras(phdmi_set_inf->vic);
	copy_len = strlen(pfmt_param->hdmitx_vinfo.name);
	if (copy_len >= sizeof(disp_mode))
		copy_len = sizeof(disp_mode) - 1;
	memcpy(disp_mode, pfmt_param->hdmitx_vinfo.name, copy_len);
	disp_mode[copy_len] = '\0';
	/*Keep set_avmute signal for time of 3 frames */
	hdmitx_sleep_frames(disp_mode, 3);

	/*Disable HDMI output*/
	hdmitx_module_disable(VMODE_HDMI);
	/* temporarily set name to 'null' */
	set_vout_local_name("null");
	/* set HDMI output */
	set_vout_mode(disp_mode);
	/* notify hdr/dv module about EDID hdr/dv info */

	/* set_window_axis(); */
	/* set_osd_free_scale(); */

	if (phdmi_set_inf->hdr_mode) {
		pr_info("[%s]: hdr_mode:%d\n",
			__func__, phdmi_set_inf->hdr_mode);
		/* send packets for HDR/HDR10+/DV manually or by callback */
		/*callback_send_drm_dv_hdr10p_pkt(phdmi_set_inf);*/
		/* wait for 3 frames to make sure that
		 * pkt send functions already called
		 */
		hdmitx_sleep_frames(disp_mode, 3);
	}
	/*Clear avmute*/
	cmd = CLR_AVMUTE;
	hdmi_tx_dev->hwop.cntlmisc(hdmi_tx_dev, MISC_AVMUTE_OP, cmd);

	/* Keep clear_avmute signal for time of 3 frames */
	hdmitx_sleep_frames(disp_mode, 3);

	return 1;
}

void meson_drm_mode_set(void)
{
	bool rtn_val = true;
	bool dv_type_changed = false;
	struct hdmitx_inf_param *phdmi_set_inf;

	phdmi_set_inf = &hdmi_tx_dev->hdmi_set_inf;
	/* hdmi mode and colorattribute maybe adjusted according to
	 * DV type and edid capability before setting mode in systemcontrol
	 */
	if (dv_type_changed) {
		hdmi_tx_dev->hwop.cntlmisc(hdmi_tx_dev,
					MISC_AVMUTE_OP, SET_AVMUTE);
		/*hdmitx_adjust_output_fmt_by_dv_policy();
		 *set_dv_hdr_policy_node();
		 */
	}
	/* check hdmi video format/hdr/dv mode/ support */
	rtn_val = hdmitx_check_rx_support();
	if (rtn_val)
		new_set_hdmitx_mode(phdmi_set_inf);
}

static irqreturn_t intr_vsync_handler(int irq, void *dev)
{
	static int vsync_cnt;

	/* TODO */
	/* Here may handle the transition between SDR/DRM/HLG/DV/HDR10+/DHDR */

	vsync_cnt++;

	return IRQ_HANDLED;
}

bool hdmitx_get_hpd_st(void)
{
	bool st;

	st = hdmi_tx_dev->hpd_state;
	return st;
}
EXPORT_SYMBOL(hdmitx_get_hpd_st);

bool hdmitx_get_rawedid(unsigned char *raw, unsigned int *len)
{
	if (!check_dvi_hdmi_edid_valid(hdmi_tx_dev->edid_ptr))
		return 0;

	*len = hdmi_tx_dev->edid_ptr[0x7e] * 128;
	memcpy(raw, hdmi_tx_dev->edid_ptr, *len);
	return 1;
}
EXPORT_SYMBOL(hdmitx_get_rawedid);

struct rx_cap *hdmitx_get_rxcap(void)
{
	return &hdmi_tx_dev->rxcap;
}
EXPORT_SYMBOL(hdmitx_get_rxcap);
#if 0
/* packet send FSM */
bool hdmitx_set_instant_packet(enum hdr_type_e type, void *data)
{
	unsigned char DRM_HB[3] = {0x87, 0x1, 26};
	static unsigned char DRM_DB[26] = {0x0};

	switch (type) {
	case HDRTYPE_SDR:
		// send HDR/SDR
		memset(DRM_DB, 0, sizeof(DRM_DB));
		hdmi_tx_dev->hwop.setpacket(HDMI_PACKET_DRM, DRM_DB, DRM_HB);
#if 0
		if (ticker > 1.5 seconds)
			// disable HDR/SDR
#endif
		pr_info("%s: send zero DRM\n", __func__);
		break;
	case HDRTYPE_HDR10:
		// send HDR/SDR firstly ??
		/* TODO */
		hdmi_tx_dev->hwop.setpacket(HDMI_PACKET_DRM, DRM_DB, DRM_HB);
		break;
	case HDRTYPE_HLG:
		// send HDR/SDR firstly ??
		//
		break;
	case HDRTYPE_HDR10PLUS:
		// send HDR/SDR firstly ??
		//
		break;
	case HDRTYPE_DOVI:
		// send HDR/SDR firstly ??
		break;
	default:
		break;
	}
	return 0;
}
EXPORT_SYMBOL(hdmitx_set_instant_packet);
#endif

void hdmitx_wrap_init(struct hdmitx_dev *dev)
{
	int rtn_val;

	hdmi_tx_dev = dev;
	rtn_val = request_irq(hdmi_tx_dev->irq_vsync, &intr_vsync_handler,
			IRQF_SHARED, "hdmitx_vsync", (void *)hdmi_tx_dev);
	if (rtn_val)
		pr_info("request_irq hdmitx_vsync failed\n");
}
