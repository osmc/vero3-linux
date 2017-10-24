/*
 * drivers/amlogic/display/vout/tv_vout.h
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


#ifndef _TV_VOUT_H_
#define _TV_VOUT_H_

/* Amlogic Headers */
#include <linux/amlogic/vout/vout_notify.h>

#define TV_CLASS_NAME	"tv"
#define	MAX_NUMBER_PARA  10

#define _TM_V 'V'
#ifdef CONFIG_AML_VOUT_CC_BYPASS
#define MAX_RING_BUFF_LEN 128
#define VOUT_IOC_CC_OPEN           _IO(_TM_V, 0x01)
#define VOUT_IOC_CC_CLOSE          _IO(_TM_V, 0x02)
#define VOUT_IOC_CC_DATA           _IOW(_TM_V, 0x03, struct vout_CCparm_s)
#endif

#define print_info(fmt, args...) pr_info(fmt, ##args)

#define SHOW_INFO(name) \
	{return snprintf(buf, 40, "%s\n", name); }

#define  STORE_INFO(name) \
	{mutex_lock(&TV_mutex);\
	snprintf(name, 40, "%s", buf) ;\
	mutex_unlock(&TV_mutex); }

#define SET_TV_CLASS_ATTR(name, op) \
static char name[40]; \
static ssize_t aml_TV_attr_##name##_show(struct class *cla, \
		struct class_attribute *attr, char *buf) \
{ \
	SHOW_INFO(name)	\
} \
static ssize_t  aml_TV_attr_##name##_store(struct class *cla, \
		struct class_attribute *attr, \
			    const char *buf, size_t count) \
{ \
	STORE_INFO(name); \
	op(name); \
	return strnlen(buf, count); \
} \
struct  class_attribute  class_TV_attr_##name =  \
__ATTR(name, S_IRUGO|S_IWUSR, \
		aml_TV_attr_##name##_show, aml_TV_attr_##name##_store);

struct disp_module_info_s {
	/* unsigned int major;  dev major number */
	struct vinfo_s *vinfo;
	char name[20];
	struct cdev   cdev;
	dev_t         devno;
	struct class  *base_class;
	struct device *dev;
};

static  DEFINE_MUTEX(TV_mutex);

#ifdef CONFIG_AML_VOUT_CC_BYPASS
struct CCparm_s {
	unsigned int type;
	unsigned int data;
};

struct CCring_MGR_s {
	unsigned int max_len;
	unsigned int rp;
	unsigned int wp;
	int over_flag;
	struct CCparm_s CCdata[MAX_RING_BUFF_LEN];
};

struct vout_CCparm_s {
	unsigned int type;
	unsigned char data1;
	unsigned char data2;
};
#endif

struct vmode_tvmode_tab_s {
	enum tvmode_e tvmode;
	enum vmode_e  mode;
};

static struct vmode_tvmode_tab_s mode_tab[] = {
	{TVMODE_480I, VMODE_480I},
	{TVMODE_480I_RPT, VMODE_480I_RPT},
	{TVMODE_480CVBS, VMODE_480CVBS},
	{TVMODE_NTSC_M, VMODE_NTSC_M},
	{TVMODE_480P, VMODE_480P},
	{TVMODE_480P_RPT, VMODE_480P_RPT},
	{TVMODE_480P_120HZ, VMODE_480P_120HZ},
	{TVMODE_576I, VMODE_576I},
	{TVMODE_576I_RPT, VMODE_576I_RPT},
	{TVMODE_576CVBS, VMODE_576CVBS},
	{TVMODE_PAL_M, VMODE_PAL_M},
	{TVMODE_PAL_N, VMODE_PAL_N},
	{TVMODE_576P, VMODE_576P},
	{TVMODE_576P_RPT, VMODE_576P_RPT},
	{TVMODE_576P_100HZ, VMODE_576P_100HZ},
	{TVMODE_720P, VMODE_720P},
	{TVMODE_1080I, VMODE_1080I},
	{TVMODE_1080P, VMODE_1080P},
	{TVMODE_1080P_30HZ, VMODE_1080P_30HZ},
	{TVMODE_720P_50HZ, VMODE_720P_50HZ},
	{TVMODE_720P_100HZ, VMODE_720P_100HZ},
	{TVMODE_720P_120HZ, VMODE_720P_120HZ},
	{TVMODE_1080I_50HZ, VMODE_1080I_50HZ},
	{TVMODE_1080P_50HZ, VMODE_1080P_50HZ},
	{TVMODE_1080I_100HZ, VMODE_1080I_100HZ},
	{TVMODE_1080P_100HZ, VMODE_1080P_100HZ},
	{TVMODE_1080I_120HZ, VMODE_1080I_120HZ},
	{TVMODE_1080P_120HZ, VMODE_1080P_120HZ},
	{TVMODE_1080P_24HZ, VMODE_1080P_24HZ},
	{TVMODE_4K2K_30HZ, VMODE_4K2K_30HZ},
	{TVMODE_4K2K_25HZ, VMODE_4K2K_25HZ},
	{TVMODE_4K2K_24HZ, VMODE_4K2K_24HZ},
	{TVMODE_4K2K_SMPTE, VMODE_4K2K_SMPTE},
	{TVMODE_4K2K_SMPTE_25HZ, VMODE_4K2K_SMPTE_25HZ},
	{TVMODE_4K2K_SMPTE_30HZ, VMODE_4K2K_SMPTE_30HZ},
	{TVMODE_4K2K_SMPTE_50HZ, VMODE_4K2K_SMPTE_50HZ},
	{TVMODE_4K2K_SMPTE_60HZ, VMODE_4K2K_SMPTE_60HZ},
	{TVMODE_4K2K_SMPTE_50HZ_Y420, VMODE_4K2K_SMPTE_50HZ_Y420},
	{TVMODE_4K2K_SMPTE_60HZ_Y420, VMODE_4K2K_SMPTE_60HZ_Y420},
	{TVMODE_4K2K_60HZ_Y420, VMODE_4K2K_60HZ_Y420},
	{TVMODE_4K2K_50HZ_Y420, VMODE_4K2K_50HZ_Y420},
	{TVMODE_4K2K_60HZ, VMODE_4K2K_60HZ},
	{TVMODE_4K2K_50HZ, VMODE_4K2K_50HZ},
	{TVMODE_VGA, VMODE_VGA},
	{TVMODE_SVGA, VMODE_SVGA},
	{TVMODE_XGA, VMODE_XGA},
	{TVMODE_SXGA, VMODE_SXGA},
	{TVMODE_WSXGA, VMODE_WSXGA},
	{TVMODE_FHDVGA, VMODE_FHDVGA},
	{TVMODE_4K1K_100HZ, VMODE_4K1K_100HZ},
	{TVMODE_4K1K_100HZ_Y420, VMODE_4K1K_100HZ_Y420},
	{TVMODE_4K1K_120HZ, VMODE_4K1K_120HZ},
	{TVMODE_4K1K_120HZ_Y420, VMODE_4K1K_120HZ_Y420},
	{TVMODE_4K05K_200HZ, VMODE_4K05K_200HZ},
	{TVMODE_4K05K_200HZ_Y420, VMODE_4K05K_200HZ_Y420},
	{TVMODE_4K05K_240HZ, VMODE_4K05K_240HZ},
	{TVMODE_4K05K_240HZ_Y420, VMODE_4K05K_240HZ_Y420},
	{TVMODE_640x480p60hz, VMODE_640x480p60hz},
	{TVMODE_800x480p60hz, VMODE_800x480p60hz},
	{TVMODE_800x600p60hz, VMODE_800x600p60hz},
	{TVMODE_852x480p60hz, VMODE_852x480p60hz},
	{TVMODE_854x480p60hz, VMODE_854x480p60hz},
	{TVMODE_1024x600p60hz, VMODE_1024x600p60hz},
	{TVMODE_1024x768p60hz, VMODE_1024x768p60hz},
	{TVMODE_1152x864p75hz, VMODE_1152x864p75hz},
	{TVMODE_1280x600p60hz, VMODE_1280x600p60hz},
	{TVMODE_1280x768p60hz, VMODE_1280x768p60hz},
	{TVMODE_1280x800p60hz, VMODE_1280x800p60hz},
	{TVMODE_1280x960p60hz, VMODE_1280x960p60hz},
	{TVMODE_1280x1024p60hz, VMODE_1280x1024p60hz},
	{TVMODE_1360x768p60hz, VMODE_1360x768p60hz},
	{TVMODE_1366x768p60hz, VMODE_1366x768p60hz},
	{TVMODE_1400x1050p60hz, VMODE_1400x1050p60hz},
	{TVMODE_1440x900p60hz, VMODE_1440x900p60hz},
	{TVMODE_1440x2560p60hz, VMODE_1440x2560p60hz},
	{TVMODE_1440x2560p70hz, VMODE_1440x2560p70hz},
	{TVMODE_1600x900p60hz, VMODE_1600x900p60hz},
	{TVMODE_1600x1200p60hz, VMODE_1600x1200p60hz},
	{TVMODE_1680x1050p60hz, VMODE_1680x1050p60hz},
	{TVMODE_1920x1200p60hz, VMODE_1920x1200p60hz},
	{TVMODE_2160x1200p90hz, VMODE_2160x1200p90hz},
	{TVMODE_2560x1440p60hz, VMODE_2560x1440p60hz,},
	{TVMODE_2560x1080p60hz, VMODE_2560x1080p60hz,},
	{TVMODE_2560x1600p60hz, VMODE_2560x1600p60hz,},
	{TVMODE_3440x1440p60hz, VMODE_3440x1440p60hz,},
};

#define UEVENT_FRAMERATE_AUTOMATION_MODE
#ifdef CONFIG_AML_VOUT_FRAMERATE_AUTOMATION
#ifndef UEVENT_FRAMERATE_AUTOMATION_MODE

struct fps_mode_conv {
	enum vmode_e cur_mode;
	enum vmode_e target_mode;
} fps_mode_conv;

enum hint_mode_e {
	START_HINT,
	END_HINT,
};


static struct fps_mode_conv fps_mode_map_23[] = {
	{
		.cur_mode           = VMODE_4K2K_24HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_25HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_30HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ_Y420,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ_Y420,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_1080P,
		.target_mode        = VMODE_1080P_24HZ,
	},
	{
		.cur_mode           = VMODE_1080P_50HZ,
		.target_mode        = VMODE_1080P_24HZ,
	},
	{
		.cur_mode           = VMODE_1080P_24HZ,
		.target_mode        = VMODE_1080P_24HZ,
	},
	{
		.cur_mode           = VMODE_1080I,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_1080I_50HZ,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_720P,
		.target_mode        = VMODE_720P,
	},
	{
		.cur_mode           = VMODE_720P_50HZ,
		.target_mode        = VMODE_720P,
	},
};

static struct fps_mode_conv fps_mode_map_24[] = {
	{
		.cur_mode           = VMODE_4K2K_25HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_30HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ_Y420,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ_Y420,
		.target_mode        = VMODE_4K2K_24HZ,
	},
	{
		.cur_mode           = VMODE_1080P,
		.target_mode        = VMODE_1080P_24HZ,
	},
	{
		.cur_mode           = VMODE_1080P_50HZ,
		.target_mode        = VMODE_1080P_24HZ,
	},
};

static struct fps_mode_conv fps_mode_map_25[] = {
	{
		.cur_mode           = VMODE_4K2K_24HZ,
		.target_mode        = VMODE_4K2K_25HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_30HZ,
		.target_mode        = VMODE_4K2K_25HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ,
		.target_mode        = VMODE_4K2K_25HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ_Y420,
		.target_mode        = VMODE_4K2K_25HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ,
		.target_mode        = VMODE_4K2K_25HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ_Y420,
		.target_mode        = VMODE_4K2K_25HZ,
	},
	{
		.cur_mode			= VMODE_1080P,
		.target_mode		= VMODE_1080P_50HZ,
	},
	{
		.cur_mode			= VMODE_1080P_24HZ,
		.target_mode		= VMODE_1080P_50HZ,
	},
	{
		.cur_mode			= VMODE_1080I,
		.target_mode		= VMODE_1080I_50HZ,
	},
	{
		.cur_mode           = VMODE_720P,
		.target_mode        = VMODE_720P_50HZ,
	},
};

static struct fps_mode_conv fps_mode_map_29[] = {
	{
		.cur_mode           = VMODE_4K2K_24HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_25HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_30HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ_Y420,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ_Y420,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_1080P,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080P_50HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080P_24HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080I,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_1080I_50HZ,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_720P,
		.target_mode        = VMODE_720P,
	},
	{
		.cur_mode           = VMODE_720P_50HZ,
		.target_mode        = VMODE_720P,
	},
};

static struct fps_mode_conv fps_mode_map_30[] = {
	{
		.cur_mode           = VMODE_4K2K_24HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_25HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ_Y420,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ_Y420,
		.target_mode        = VMODE_4K2K_30HZ,
	},
	{
		.cur_mode           = VMODE_1080P_50HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080P_24HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080I_50HZ,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_720P_50HZ,
		.target_mode        = VMODE_720P,
	},
};

static struct fps_mode_conv fps_mode_map_50[] = {
	{
		.cur_mode           = VMODE_4K2K_24HZ,
		.target_mode        = VMODE_4K2K_50HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_25HZ,
		.target_mode        = VMODE_4K2K_50HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_30HZ,
		.target_mode        = VMODE_4K2K_50HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ,
		.target_mode        = VMODE_4K2K_50HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ_Y420,
		.target_mode        = VMODE_4K2K_50HZ_Y420,
	},
	{
		.cur_mode           = VMODE_1080P,
		.target_mode        = VMODE_1080P_50HZ,
	},
	{
		.cur_mode           = VMODE_1080P_24HZ,
		.target_mode        = VMODE_1080P_50HZ,
	},
	{
		.cur_mode           = VMODE_1080I,
		.target_mode        = VMODE_1080I_50HZ,
	},
	{
		.cur_mode           = VMODE_720P,
		.target_mode        = VMODE_720P_50HZ,
	},
};

static struct fps_mode_conv fps_mode_map_59[] = {
	{
		.cur_mode           = VMODE_4K2K_24HZ,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_25HZ,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_30HZ,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ,
		.target_mode        = VMODE_4K2K_60HZ,
	},
	{
		.cur_mode			= VMODE_4K2K_50HZ_Y420,
		.target_mode		= VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ,
		.target_mode        = VMODE_4K2K_60HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_60HZ_Y420,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_1080P,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080P_50HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080P_24HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080I,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_1080I_50HZ,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_720P,
		.target_mode        = VMODE_720P,
	},
	{
		.cur_mode           = VMODE_720P_50HZ,
		.target_mode        = VMODE_720P,
	},
};

static struct fps_mode_conv fps_mode_map_60[] = {
	{
		.cur_mode           = VMODE_4K2K_24HZ,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_25HZ,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_30HZ,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ,
		.target_mode        = VMODE_4K2K_60HZ,
	},
	{
		.cur_mode           = VMODE_4K2K_50HZ_Y420,
		.target_mode        = VMODE_4K2K_60HZ_Y420,
	},
	{
		.cur_mode           = VMODE_1080P_50HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080P_24HZ,
		.target_mode        = VMODE_1080P,
	},
	{
		.cur_mode           = VMODE_1080I_50HZ,
		.target_mode        = VMODE_1080I,
	},
	{
		.cur_mode           = VMODE_720P_50HZ,
		.target_mode        = VMODE_720P,
	},
};

#endif
#endif

struct vmode_match_s {
	char *name;
	enum vmode_e mode;
};

static struct vmode_match_s tv_match_table[] = {
	{"480i60hz",      VMODE_480I},
	{"480irpt",       VMODE_480I_RPT},
	{"480cvbs",       VMODE_480CVBS},
	{"ntsc_m",        VMODE_NTSC_M},
	{"480p60hz",      VMODE_480P},
	{"480prtp",       VMODE_480P_RPT},
	{"480p120hz",     VMODE_480P_120HZ},
	{"576i50hz",      VMODE_576I},
	{"576irpt",       VMODE_576I_RPT},
	{"576cvbs",       VMODE_576CVBS},
	{"pal_m",         VMODE_PAL_M},
	{"pal_n",         VMODE_PAL_N},
	{"576p50hz",      VMODE_576P},
	{"576prpt",       VMODE_576P_RPT},
	{"576p100hz",     VMODE_576P_100HZ},
	{"720p60hz",      VMODE_720P},
	{"720p50hz",      VMODE_720P_50HZ},
	{"720p100hz",     VMODE_720P_100HZ},
	{"720p120hz",     VMODE_720P_120HZ},
	{"768p60hz",      VMODE_768P},
	{"768p50hz",      VMODE_768P_50HZ},
	{"1080i60hz",     VMODE_1080I},
	{"1080i50hz",     VMODE_1080I_50HZ},
	{"1080i100hz",    VMODE_1080I_100HZ},
	{"1080i120hz",    VMODE_1080I_120HZ},
	{"1080p60hz",     VMODE_1080P},
	{"1080p25hz",     VMODE_1080P_25HZ},
	{"1080p30hz",     VMODE_1080P_30HZ},
	{"1080p50hz",     VMODE_1080P_50HZ},
	{"1080p24hz",     VMODE_1080P_24HZ},
	{"1080p100hz",    VMODE_1080P_100HZ},
	{"1080p120hz",    VMODE_1080P_120HZ},
	{"2160p30hz",     VMODE_4K2K_30HZ},
	{"2160p25hz",     VMODE_4K2K_25HZ},
	{"2160p24hz",     VMODE_4K2K_24HZ},
	{"smpte24hz",     VMODE_4K2K_SMPTE},
	{"smpte25hz",     VMODE_4K2K_SMPTE_25HZ},
	{"smpte30hz",     VMODE_4K2K_SMPTE_30HZ},
	{"smpte50hz420",  VMODE_4K2K_SMPTE_50HZ_Y420},
	{"smpte50hz",     VMODE_4K2K_SMPTE_50HZ},
	{"smpte60hz420",  VMODE_4K2K_SMPTE_60HZ_Y420},
	{"smpte60hz",     VMODE_4K2K_SMPTE_60HZ},
	{"4k2k5g",        VMODE_4K2K_FAKE_5G},
	{"2160p60hz420",  VMODE_4K2K_60HZ_Y420},
	{"2160p60hz",     VMODE_4K2K_60HZ},
	{"2160p50hz420",  VMODE_4K2K_50HZ_Y420},
	{"2160p50hz",     VMODE_4K2K_50HZ},
	{"2160p5g",       VMODE_4K2K_5G},
	{"4k1k120hz420",  VMODE_4K1K_120HZ_Y420},
	{"4k1k120hz",     VMODE_4K1K_120HZ},
	{"4k1k100hz420",  VMODE_4K1K_100HZ_Y420},
	{"4k1k100hz",     VMODE_4K1K_100HZ},
	{"4k05k240hz420", VMODE_4K05K_240HZ_Y420},
	{"4k05k240hz",    VMODE_4K05K_240HZ},
	{"4k05k200hz420", VMODE_4K05K_200HZ_Y420},
	{"4k05k200hz",    VMODE_4K05K_200HZ},
	{"640x480p60hz",  VMODE_640x480p60hz},
	{"800x480p60hz",  VMODE_800x480p60hz},
	{"800x600p60hz",  VMODE_800x600p60hz},
	{"852x480p60hz",  VMODE_852x480p60hz},
	{"854x480p60hz",  VMODE_854x480p60hz},
	{"1024x600p60hz", VMODE_1024x600p60hz},
	{"1024x768p60hz", VMODE_1024x768p60hz},
	{"1152x864p75hz", VMODE_1152x864p75hz},
	{"1280x600p60hz", VMODE_1280x600p60hz},
	{"1280x768p60hz", VMODE_1280x768p60hz},
	{"1280x800p60hz", VMODE_1280x800p60hz},
	{"1280x960p60hz", VMODE_1280x960p60hz},
	{"1280x1024p60hz", VMODE_1280x1024p60hz},
	{"1360x768p60hz", VMODE_1360x768p60hz},
	{"1366x768p60hz", VMODE_1366x768p60hz},
	{"1400x1050p60hz", VMODE_1400x1050p60hz},
	{"1440x900p60hz", VMODE_1440x900p60hz},
	{"1440x2560p60hz", VMODE_1440x2560p60hz},
	{"1440x2560p70hz", VMODE_1440x2560p70hz},
	{"1600x900p60hz", VMODE_1600x900p60hz},
	{"1600x1200p60hz", VMODE_1600x1200p60hz},
	{"1680x1050p60hz", VMODE_1680x1050p60hz},
	{"1920x1200p60hz", VMODE_1920x1200p60hz},
	{"2160x1200p90hz", VMODE_2160x1200p90hz},
	{"2560x1080p60hz", VMODE_2560x1080p60hz},
	{"2560x1440p60hz", VMODE_2560x1440p60hz},
	{"2560x1600p60hz", VMODE_2560x1600p60hz},
	{"3440x1440p60hz", VMODE_3440x1440p60hz},
	/* Extra VMODE for 3D Frame Packing */
	{"720fp50hz",      VMODE_720FP50HZ},
	{"720fp60hz",      VMODE_720FP60HZ},
	{"1080fp24hz",     VMODE_1080FP24HZ},
	{"1080fp25hz",     VMODE_1080FP25HZ},
	{"1080fp30hz",     VMODE_1080FP30HZ},
	{"1080fp50hz",     VMODE_1080FP50HZ},
	{"1080fp60hz",     VMODE_1080FP60HZ},
	/* Extra VMODE for 3D Frame Packing End */
	{"null",           VMODE_NULL},
};


static struct vinfo_s cvbs_info[] = {
	{ /* VMODE_480CVBS*/
		.name              = "480cvbs",
		.mode              = VMODE_480CVBS,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.screen_real_width = 4,
		.screen_real_height = 3,
		.video_clk         = 27000000,
		.htotal            = 1716,
		.vtotal            = 525,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_NTSC_M*/
		.name              = "ntsc_m",
		.mode              = VMODE_NTSC_M,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.htotal            = 1716,
		.vtotal            = 525,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_PAL_M */
		.name              = "pal_m",
		.mode              = VMODE_PAL_M,
		.width             = 720,
		.height            = 480,
		.field_height      = 240,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.htotal            = 1716,
		.vtotal            = 525,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_576CVBS */
		.name              = "576cvbs",
		.mode              = VMODE_576CVBS,
		.width             = 720,
		.height            = 576,
		.field_height      = 288,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.screen_real_width = 4,
		.screen_real_height = 3,
		.video_clk         = 27000000,
		.htotal            = 1728,
		.vtotal            = 625,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* VMODE_PAL_N */
		.name              = "pal_n",
		.mode              = VMODE_PAL_N,
		.width             = 720,
		.height            = 576,
		.field_height      = 288,
		.aspect_ratio_num  = 4,
		.aspect_ratio_den  = 3,
		.sync_duration_num = 50,
		.sync_duration_den = 1,
		.video_clk         = 27000000,
		.htotal            = 1728,
		.vtotal            = 625,
		.viu_color_fmt     = TVIN_YUV444,
	},
	{ /* NULL mode, used as temporary witch mode state */
		.name              = "null",
		.mode              = VMODE_NULL,
		.width             = 1920,
		.height            = 1080,
		.field_height      = 1080,
		.aspect_ratio_num  = 16,
		.aspect_ratio_den  = 9,
		.sync_duration_num = 60,
		.sync_duration_den = 1,
		.screen_real_width = 16,
		.screen_real_height = 9,
		.video_clk         = 1485000000,
		.htotal            = 2200,
		.vtotal            = 1125,
		.viu_color_fmt     = TVIN_YUV444,
	},
};

#endif
