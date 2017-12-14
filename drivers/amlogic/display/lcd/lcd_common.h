
/*
 * drivers/amlogic/display/lcd/lcd_common.h
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

#ifndef __AML_LCD_COMMON_H__
#define __AML_LCD_COMMON_H__
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/amlogic/vout/lcd_vout.h>
#include "lcd_clk_config.h"

#define VPP_OUT_SATURATE            (1 << 0)

/* -------------------------- */
/* lvsd phy parameters define */
/* -------------------------- */
#define LVDS_PHY_CNTL1_G9TV    0x606cca80
#define LVDS_PHY_CNTL2_G9TV    0x0000006c
#define LVDS_PHY_CNTL3_G9TV    0x00000800

#define LVDS_PHY_CNTL1_TXHD    0x6c60ca80
#define LVDS_PHY_CNTL2_TXHD    0x00000070
#define LVDS_PHY_CNTL3_TXHD    0x03ff0c00
/* -------------------------- */

/* -------------------------- */
/* vbyone phy parameters define */
/* -------------------------- */
#define VX1_PHY_CNTL1_G9TV            0x6e0ec900
#define VX1_PHY_CNTL1_G9TV_PULLUP     0x6e0f4d00
#define VX1_PHY_CNTL2_G9TV            0x0000007c
#define VX1_PHY_CNTL3_G9TV            0x00ff0800
/* -------------------------- */

/* -------------------------- */
/* minilvsd phy parameters define */
/* -------------------------- */
#define MLVDS_PHY_CNTL1_TXHD   0x6c60ca80
#define MLVDS_PHY_CNTL2_TXHD   0x00000070
#define MLVDS_PHY_CNTL3_TXHD   0x03ff0c00
/* -------------------------- */

extern struct mutex lcd_power_mutex;
extern struct mutex lcd_vout_mutex;
extern unsigned char lcd_resume_flag;
extern int lcd_vout_serve_bypass;

/* lcd common */
extern int lcd_type_str_to_type(const char *str);
extern char *lcd_type_type_to_str(int type);
extern unsigned char lcd_mode_str_to_mode(const char *str);
extern char *lcd_mode_mode_to_str(int mode);

extern void lcd_cpu_gpio_register(unsigned int index);
extern void lcd_cpu_gpio_set(unsigned int index, int value);
extern unsigned int lcd_cpu_gpio_get(unsigned int index);
extern void lcd_ttl_pinmux_set(int status);
extern void lcd_vbyone_pinmux_set(int status);
extern unsigned int lcd_lvds_channel_on_value(struct lcd_config_s *pconf);
extern void lcd_mlvds_pinmux_set(int status);
extern int lcd_power_load_from_dts(struct lcd_config_s *pconf,
		struct device_node *child);
extern int lcd_power_load_from_unifykey(struct lcd_config_s *pconf,
		unsigned char *buf, int key_len, int len);

extern void lcd_hdr_vinfo_update(void);
extern void lcd_timing_init_config(struct lcd_config_s *pconf);
extern int lcd_vmode_change(struct lcd_config_s *pconf);
extern void lcd_venc_change(struct lcd_config_s *pconf);
extern void lcd_clk_gate_switch(int status);

/* lcd tcon */
extern void lcd_tcon_regs_table_print(struct mlvds_config_s *mlvds_conf);
extern void lcd_tcon_regs_readback_print(struct mlvds_config_s *mlvds_conf);
extern int lcd_tcon_sys_regs_update(unsigned char *table, int len);
extern int lcd_tcon_init(struct lcd_config_s *pconf);
extern void lcd_tcon_disable(void);
extern int lcd_tcon_probe(struct device *dev);

extern int lcd_tcon_od_set(int flag);
extern int lcd_tcon_od_get(void);


/* lcd debug */
extern int lcd_class_creat(void);
extern int lcd_class_remove(void);

/* lcd driver */
#ifdef CONFIG_AML_LCD_TV
extern void lcd_vbyone_interrupt_enable(int flag);
extern void lcd_tv_clk_config_change(struct lcd_config_s *pconf);
extern void lcd_tv_clk_update(struct lcd_config_s *pconf);
extern void lcd_tv_vout_server_init(void);
extern int lcd_tv_probe(struct device *dev);
extern int lcd_tv_remove(struct device *dev);
#endif
#ifdef CONFIG_AML_LCD_TABLET
extern void lcd_tablet_clk_config_change(struct lcd_config_s *pconf);
extern void lcd_tablet_clk_update(struct lcd_config_s *pconf);
extern void lcd_tablet_vout_server_init(void);
extern int lcd_tablet_probe(struct device *dev);
extern int lcd_tablet_remove(struct device *dev);
#endif

#endif
