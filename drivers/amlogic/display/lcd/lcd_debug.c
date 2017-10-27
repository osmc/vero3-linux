/*
 * drivers/amlogic/display/vout/lcd/lcd_debug.c
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

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/amlogic/vout/lcd_vout.h>
#include <linux/amlogic/vout/lcd_notify.h>
#include <linux/amlogic/vout/lcd_unifykey.h>
#include "lcd_reg.h"
#include "lcd_common.h"

static void lcd_debug_parse_param(char *buf_orig, char **parm)
{
	char *ps, *token;
	char str[3] = {' ', '\n', '\0'};
	unsigned int n = 0;

	ps = buf_orig;
	while (1) {
		token = strsep(&ps, str);
		if (token == NULL)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}
}

static const char *lcd_common_usage_str = {
"Usage:\n"
"    echo <0|1> > enable ; 0=disable lcd; 1=enable lcd\n"
"\n"
"    echo type <adj_type> > frame_rate ; set lcd frame rate adjust type\n"
"    echo set <frame_rate> > frame_rate ; set lcd frame rate(unit in 1/100Hz)\n"
"    cat frame_rate ; read current lcd frame rate\n"
"\n"
"    echo <num> > test ; show lcd bist pattern(1~7), 0=disable bist\n"
"\n"
"    echo w<v|h|c|p|t> <reg> <data> > reg ; write data to vcbus|hiu|cbus|periphs|tcon reg\n"
"    echo r<v|h|c|p|t> <reg> > reg ; read vcbus|hiu|cbus|periphs|tcon reg\n"
"    echo d<v|h|c|p|t> <reg> <num> > reg ; dump vcbus|hiu|cbus|periphs|tcon regs\n"
"\n"
"    echo <0|1> > print ; 0=disable debug print; 1=enable debug print\n"
"    cat print ; read current debug print flag\n"
"\n"
"    echo <cmd> ... > debug ; lcd common debug, use 'cat debug' for help\n"
"    cat debug ; print help information for debug command\n"
"\n"
"    echo <on|off> <step_num> <delay> > power ; set power on/off step delay(unit: ms)\n"
"    cat power ; print lcd power on/off step\n"
"\n"
"    cat key_valid ; print lcd_key_valid setting\n"
"    cat config_load ; print lcd_config load_id(0=dts, 1=unifykey)\n"
};

static const char *lcd_debug_usage_str = {
"Usage:\n"
"    echo clk <freq> > debug ; set lcd pixel clock, unit in Hz\n"
"    echo bit <lcd_bits> > debug ; set lcd bits\n"
"    echo basic <h_active> <v_active> <h_period> <v_period> <lcd_bits> > debug ; set lcd basic config\n"
"    echo sync <hs_width> <hs_bp> <hs_pol> <vs_width> <vs_bp> <vs_pol> > debug ; set lcd sync timing\n"
"data format:\n"
"    <xx_pol>       : 0=negative, 1=positive\n"
"\n"
"    echo info > debug ; show lcd information\n"
"    echo reg > debug ; show lcd registers\n"
"    echo dump > debug ; show lcd information & registers\n"
"	 echo dith <dither_en> <rounding_en> <dither_md>  > debug ; set vpu_vencl_dith_ctrl\n"
"    echo key > debug ; show lcd_key_valid config, and lcd unifykey raw data\n"
"\n"
"    echo reset > debug; reset lcd driver\n"
"    echo power <0|1> > debug ; lcd power control: 0=power off, 1=power on\n"
};

static const char *lcd_debug_change_usage_str = {
"Usage:\n"
"    echo clk <freq> > change ; change lcd pixel clock, unit in Hz\n"
"    echo bit <lcd_bits> > change ; change lcd bits\n"
"    echo basic <h_active> <v_active> <h_period> <v_period> <lcd_bits> > change ; change lcd basic config\n"
"    echo sync <hs_width> <hs_bp> <hs_pol> <vs_width> <vs_bp> <vs_pol> > change ; change lcd sync timing\n"
"data format:\n"
"    <xx_pol>       : 0=negative, 1=positive\n"
"\n"
"    echo set > change; apply lcd config changing\n"
};

static ssize_t lcd_debug_common_help(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_common_usage_str);
}

static ssize_t lcd_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_debug_usage_str);
}

static void lcd_cpu_gpio_register_print(struct lcd_config_s *pconf)
{
	int i;
	struct lcd_cpu_gpio_s *cpu_gpio;

	pr_info("cpu_gpio register:\n");

	i = 0;
	while (i < LCD_CPU_GPIO_NUM_MAX) {
		cpu_gpio = &pconf->lcd_power->cpu_gpio[i];
		if (cpu_gpio->flag) {
			pr_info("%d: name=%s, gpio=%p\n",
				i, cpu_gpio->name, cpu_gpio->gpio);
		}
		i++;
	}
}

static void lcd_power_info_print(struct lcd_config_s *pconf, int status)
{
	int i;
	struct lcd_power_step_s *power_step;

	if (status)
		pr_info("power on step:\n");
	else
		pr_info("power off step:\n");

	i = 0;
	while (i < LCD_PWR_STEP_MAX) {
		if (status)
			power_step = &pconf->lcd_power->power_on_step[i];
		else
			power_step = &pconf->lcd_power->power_off_step[i];

		if (power_step->type >= LCD_POWER_TYPE_MAX)
			break;
		switch (power_step->type) {
		case LCD_POWER_TYPE_CPU:
		case LCD_POWER_TYPE_PMU:
			pr_info("%d: type=%d, index=%d, value=%d, delay=%d\n",
				i, power_step->type, power_step->index,
				power_step->value, power_step->delay);
			break;
		case LCD_POWER_TYPE_EXTERN:
			pr_info("%d: type=%d, index=%d, delay=%d\n",
				i, power_step->type, power_step->index,
				power_step->delay);
			break;
		case LCD_POWER_TYPE_SIGNAL:
			pr_info("%d: type=%d, delay=%d\n",
				i, power_step->type, power_step->delay);
			break;
		default:
			break;
		}
		i++;
	}
}

static void lcd_info_print(void)
{
	unsigned int lcd_clk, sync_duration;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;
	struct vbyone_config_s *vx1_conf;

	pconf = lcd_drv->lcd_config;
	lcd_clk = (pconf->lcd_timing.lcd_clk / 1000);
	sync_duration = pconf->lcd_timing.sync_duration_num * 100;
	sync_duration = sync_duration / pconf->lcd_timing.sync_duration_den;

	LCDPR("panel_type: %s\n", pconf->lcd_propname);
	LCDPR("key_valid: %d, config_load: %d\n",
		lcd_drv->lcd_key_valid, lcd_drv->lcd_config_load);
	LCDPR("chip: %d, mode : %s, status: %d\n",
		lcd_drv->chip_type, lcd_mode_mode_to_str(lcd_drv->lcd_mode),
		lcd_drv->lcd_status);
	LCDPR("fr_auto_policy: %d, vmode_vsync_en: %d\n",
		lcd_drv->fr_auto_policy, lcd_drv->lcd_vmode_vsync_en);

	LCDPR("%s, %s %ubit, %ux%u@%u.%02uHz\n",
		pconf->lcd_basic.model_name,
		lcd_type_type_to_str(pconf->lcd_basic.lcd_type),
		pconf->lcd_basic.lcd_bits,
		pconf->lcd_basic.h_active, pconf->lcd_basic.v_active,
		(sync_duration / 100), (sync_duration % 100));

	pr_info("lcd_clk           %u.%03uMHz\n"
		"ss_level          %d\n"
		"clk_auto          %d\n"
		"fr_adj_type       %d\n\n",
		(lcd_clk / 1000), (lcd_clk % 1000),
		pconf->lcd_timing.ss_level, pconf->lcd_timing.clk_auto,
		pconf->lcd_timing.fr_adjust_type);

	pr_info("h_period          %d\n"
		"v_period          %d\n"
		"hs_width          %d\n"
		"hs_backporch      %d\n"
		"hs_pol            %d\n"
		"vs_width          %d\n"
		"vs_backporch      %d\n"
		"vs_pol            %d\n\n",
		pconf->lcd_basic.h_period, pconf->lcd_basic.v_period,
		pconf->lcd_timing.hsync_width, pconf->lcd_timing.hsync_bp,
		pconf->lcd_timing.hsync_pol,
		pconf->lcd_timing.vsync_width, pconf->lcd_timing.vsync_bp,
		pconf->lcd_timing.vsync_pol);
	pr_info("h_period_min      %d\n"
		"h_period_max      %d\n"
		"v_period_min      %d\n"
		"v_period_max      %d\n"
		"pclk_min          %d\n"
		"pclk_max          %d\n\n",
		pconf->lcd_basic.h_period_min, pconf->lcd_basic.h_period_max,
		pconf->lcd_basic.v_period_min, pconf->lcd_basic.v_period_max,
		pconf->lcd_basic.lcd_clk_min, pconf->lcd_basic.lcd_clk_max);

	switch (pconf->lcd_basic.lcd_type) {
	case LCD_TTL:
		pr_info("clk_pol           %u\n"
			"hvsync_valid      %u\n"
			"de_valid          %u\n"
			"rb_swap           %u\n"
			"bit_swap          %u\n\n",
			pconf->lcd_control.ttl_config->clk_pol,
			((pconf->lcd_control.ttl_config->sync_valid >> 0) & 1),
			((pconf->lcd_control.ttl_config->sync_valid >> 1) & 1),
			((pconf->lcd_control.ttl_config->swap_ctrl >> 1) & 1),
			((pconf->lcd_control.ttl_config->swap_ctrl >> 0) & 1));
		break;
	case LCD_LVDS:
		pr_info("lvds_repack       %u\n"
			"dual_port         %u\n"
			"pn_swap           %u\n"
			"port_swap         %u\n"
			"lane_reverse      %u\n"
			"phy_vswing        0x%x\n"
			"phy_preem         0x%x\n"
			"phy_clk_vswing    0x%x\n"
			"phy_clk_preem     0x%x\n\n",
			pconf->lcd_control.lvds_config->lvds_repack,
			pconf->lcd_control.lvds_config->dual_port,
			pconf->lcd_control.lvds_config->pn_swap,
			pconf->lcd_control.lvds_config->port_swap,
			pconf->lcd_control.lvds_config->lane_reverse,
			pconf->lcd_control.lvds_config->phy_vswing,
			pconf->lcd_control.lvds_config->phy_preem,
			pconf->lcd_control.lvds_config->phy_clk_vswing,
			pconf->lcd_control.lvds_config->phy_clk_preem);
		break;
	case LCD_MLVDS:
		pr_info("channel_num       %d\n"
			"channel_sel0      0x%08x\n"
			"channel_sel1      0x%08x\n"
			"clk_phase         0x%04x\n"
			"pn_swap           %u\n"
			"bit_swap          %u\n"
			"phy_vswing        0x%x\n"
			"phy_preem         0x%x\n"
			"bit_rate          %dHz\n"
			"pi_clk_sel        0x%03x\n"
			"tcon_fb_addr      0x%08x\n\n",
			pconf->lcd_control.mlvds_config->channel_num,
			pconf->lcd_control.mlvds_config->channel_sel0,
			pconf->lcd_control.mlvds_config->channel_sel1,
			pconf->lcd_control.mlvds_config->clk_phase,
			pconf->lcd_control.mlvds_config->pn_swap,
			pconf->lcd_control.mlvds_config->bit_swap,
			pconf->lcd_control.mlvds_config->phy_vswing,
			pconf->lcd_control.mlvds_config->phy_preem,
			pconf->lcd_control.mlvds_config->bit_rate,
			pconf->lcd_control.mlvds_config->pi_clk_sel,
			pconf->lcd_control.mlvds_config->fb_addr);
		break;
	case LCD_VBYONE:
		vx1_conf = pconf->lcd_control.vbyone_config;
		pr_info("lane_count        %u\n"
			"region_num        %u\n"
			"byte_mode         %u\n"
			"color_fmt         %u\n"
			"bit_rate          %u\n"
			"phy_vswing        0x%x\n"
			"phy_preemphasis   0x%x\n"
			"intr_en           %u\n"
			"vsync_intr_en     %u\n"
			"ctrl_flag         0x%x\n\n",
			pconf->lcd_control.vbyone_config->lane_count,
			pconf->lcd_control.vbyone_config->region_num,
			pconf->lcd_control.vbyone_config->byte_mode,
			pconf->lcd_control.vbyone_config->color_fmt,
			pconf->lcd_control.vbyone_config->bit_rate,
			pconf->lcd_control.vbyone_config->phy_vswing,
			pconf->lcd_control.vbyone_config->phy_preem,
			pconf->lcd_control.vbyone_config->intr_en,
			pconf->lcd_control.vbyone_config->vsync_intr_en,
			pconf->lcd_control.vbyone_config->ctrl_flag);
		if (vx1_conf->ctrl_flag & 0x1) {
			pr_info("power_on_reset_en      %u\n"
				"power_on_reset_delay   %ums\n\n",
				(vx1_conf->ctrl_flag & 0x1),
				vx1_conf->power_on_reset_delay);
		}
		if (vx1_conf->ctrl_flag & 0x2) {
			pr_info("hpd_data_delay_en      %u\n"
				"hpd_data_delay         %ums\n\n",
				((vx1_conf->ctrl_flag >> 1) & 0x1),
				vx1_conf->hpd_data_delay);
		}
		if (vx1_conf->ctrl_flag & 0x4) {
			pr_info("cdr_training_hold_en   %u\n"
				"cdr_training_hold      %ums\n\n",
				((vx1_conf->ctrl_flag >> 2) & 0x1),
				vx1_conf->cdr_training_hold);
		}
		break;
	case LCD_MIPI:
		break;
	case LCD_EDP:
		break;
	default:
		break;
	}

	pr_info("pll_ctrl          0x%08x\n"
		"div_ctrl          0x%08x\n"
		"clk_ctrl          0x%08x\n"
		"video_on_pixel    %d\n"
		"video_on_line     %d\n\n",
		pconf->lcd_timing.pll_ctrl, pconf->lcd_timing.div_ctrl,
		pconf->lcd_timing.clk_ctrl,
		pconf->lcd_timing.video_on_pixel,
		pconf->lcd_timing.video_on_line);

	lcd_power_info_print(pconf, 1);
	lcd_power_info_print(pconf, 0);
	lcd_cpu_gpio_register_print(pconf);
}

static unsigned int lcd_reg_dump_clk[] = {
	HHI_HDMI_PLL_CNTL,
	HHI_HDMI_PLL_CNTL2,
	HHI_HDMI_PLL_CNTL3,
	HHI_HDMI_PLL_CNTL4,
	HHI_HDMI_PLL_CNTL5,
	HHI_HDMI_PLL_CNTL6,
	HHI_VID_PLL_CLK_DIV,
	HHI_VIID_CLK_DIV,
	HHI_VIID_CLK_CNTL,
	HHI_VID_CLK_CNTL2,
};

static unsigned int lcd_reg_dump_encl[] = {
	VPU_VIU_VENC_MUX_CTRL,
	ENCL_VIDEO_EN,
	ENCL_VIDEO_MODE,
	ENCL_VIDEO_MODE_ADV,
	ENCL_VIDEO_MAX_PXCNT,
	ENCL_VIDEO_MAX_LNCNT,
	ENCL_VIDEO_HAVON_BEGIN,
	ENCL_VIDEO_HAVON_END,
	ENCL_VIDEO_VAVON_BLINE,
	ENCL_VIDEO_VAVON_ELINE,
	ENCL_VIDEO_HSO_BEGIN,
	ENCL_VIDEO_HSO_END,
	ENCL_VIDEO_VSO_BEGIN,
	ENCL_VIDEO_VSO_END,
	ENCL_VIDEO_VSO_BLINE,
	ENCL_VIDEO_VSO_ELINE,
	ENCL_VIDEO_RGBIN_CTRL,
	L_GAMMA_CNTL_PORT,
	L_RGB_BASE_ADDR,
	L_RGB_COEFF_ADDR,
	L_POL_CNTL_ADDR,
	L_DITH_CNTL_ADDR,
};

static void lcd_ttl_reg_print(void)
{
	unsigned int reg;

	pr_info("\nttl registers:\n");
	reg = L_DUAL_PORT_CNTL_ADDR;
	pr_info("PORT_CNTL           [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STH1_HS_ADDR;
	pr_info("STH1_HS_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STH1_HE_ADDR;
	pr_info("STH1_HE_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STH1_VS_ADDR;
	pr_info("STH1_VS_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STH1_VE_ADDR;
	pr_info("STH1_VE_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STV1_HS_ADDR;
	pr_info("STV1_HS_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STV1_HE_ADDR;
	pr_info("STV1_HE_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STV1_VS_ADDR;
	pr_info("STV1_VS_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_STV1_VE_ADDR;
	pr_info("STV1_VE_ADDR        [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_OEH_HS_ADDR;
	pr_info("OEH_HS_ADDR         [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_OEH_HE_ADDR;
	pr_info("OEH_HE_ADDR         [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_OEH_VS_ADDR;
	pr_info("OEH_VS_ADDR         [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = L_OEH_VE_ADDR;
	pr_info("OEH_VE_ADDR         [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
}

static void lcd_lvds_reg_print(void)
{
	unsigned int reg;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	pr_info("\nlvds registers:\n");
	reg = LVDS_PACK_CNTL_ADDR;
	pr_info("LVDS_PACK_CNTL      [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = LVDS_GEN_CNTL;
	pr_info("LVDS_GEN_CNTL       [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = LCD_PORT_SWAP;
	pr_info("LCD_PORT_SWAP       [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = HHI_LVDS_TX_PHY_CNTL0;
	pr_info("LVDS_PHY_CNTL0      [0x%04x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
	reg = HHI_LVDS_TX_PHY_CNTL1;
	pr_info("LVDS_PHY_CNTL1      [0x%04x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));

	switch (lcd_drv->chip_type) {
	case LCD_CHIP_TXHD:
		reg = LVDS_CH_SWAP0;
		pr_info("LVDS_CH_SWAP0       [0x%04x] = 0x%08x\n",
			reg, lcd_vcbus_read(reg));
		reg = LVDS_CH_SWAP1;
		pr_info("LVDS_CH_SWAP1       [0x%04x] = 0x%08x\n",
			reg, lcd_vcbus_read(reg));
		reg = LVDS_CH_SWAP2;
		pr_info("LVDS_CH_SWAP2       [0x%04x] = 0x%08x\n",
			reg, lcd_vcbus_read(reg));
		break;
	default:
		break;
	}
}

static void lcd_vbyone_reg_print(void)
{
	unsigned int reg;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	pr_info("\nvbyone registers:\n");
	switch (lcd_drv->chip_type) {
	case LCD_CHIP_GXTVBB:
		reg = PERIPHS_PIN_MUX_7;
		pr_info("VX1_PINMUX          [0x%04x] = 0x%08x\n",
			reg, lcd_periphs_read(reg));
		break;
	case LCD_CHIP_TXL:
	case LCD_CHIP_TXLX:
		reg = PERIPHS_PIN_MUX_0;
		pr_info("VX1_PINMUX          [0x%04x] = 0x%08x\n",
			reg, lcd_periphs_read(reg));
		break;
	default:
		break;
	}
	reg = VBO_STATUS_L;
	pr_info("VX1_STATUS          [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	switch (lcd_drv->chip_type) {
	case LCD_CHIP_TXL:
	case LCD_CHIP_TXLX:
		reg = VBO_INSGN_CTRL;
		pr_info("VBO_INSGN_CTRL      [0x%04x] = 0x%08x\n",
			reg, lcd_vcbus_read(reg));
		break;
	default:
		break;
	}
	reg = VBO_FSM_HOLDER_L;
	pr_info("VX1_FSM_HOLDER_L    [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VBO_FSM_HOLDER_H;
	pr_info("VX1_FSM_HOLDER_H    [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VBO_INTR_STATE_CTRL;
	pr_info("VX1_INTR_STATE_CTRL [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VBO_INTR_UNMASK;
	pr_info("VX1_INTR_UNMASK     [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VBO_INTR_STATE;
	pr_info("VX1_INTR_STATE      [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = HHI_LVDS_TX_PHY_CNTL0;
	pr_info("VX1_PHY_CNTL0       [0x%04x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
}

static void lcd_mlvds_reg_print(void)
{
	unsigned int reg;

	reg = PERIPHS_PIN_MUX_4_TXHD;
	pr_info("\nPERIPHS_PIN_MUX_4   [0x%04x] = 0x%08x\n",
		reg, lcd_periphs_read(reg));
	reg = PERIPHS_PIN_MUX_5_TXHD;
	pr_info("PERIPHS_PIN_MUX_5   [0x%04x] = 0x%08x\n",
		reg, lcd_periphs_read(reg));

	lcd_lvds_reg_print();

	pr_info("\ntcon clk registers:\n");
	reg = HHI_TCON_CLK_CNTL;
	pr_info("HHI_TCON_CLK_CNTL   [0x%08x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
	reg = HHI_HDMI_PLL_CNTL6;
	pr_info("HHI_HDMI_PLL_CNTL6  [0x%08x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
	reg = HHI_DIF_TCON_CNTL0;
	pr_info("HHI_DIF_TCON_CNTL0  [0x%08x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
	reg = HHI_DIF_TCON_CNTL1;
	pr_info("HHI_DIF_TCON_CNTL1  [0x%08x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
	reg = HHI_DIF_TCON_CNTL2;
	pr_info("HHI_DIF_TCON_CNTL2  [0x%08x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));

	pr_info("\ntcon top registers:\n");
	reg = TCON_TOP_CTRL;
	pr_info("TCON_TOP_CTRL       [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
	reg = TCON_OUT_CH_SEL0;
	pr_info("TCON_OUT_CH_SEL0    [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
	reg = TCON_OUT_CH_SEL1;
	pr_info("TCON_OUT_CH_SEL1    [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
	reg = TCON_STATUS0;
	pr_info("TCON_STATUS0        [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
	reg = TCON_PLLLOCK_CNTL;
	pr_info("TCON_PLLLOCK_CNTL   [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
	reg = TCON_RST_CTRL;
	pr_info("TCON_RST_CTRL       [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
	reg = TCON_CLK_CTRL;
	pr_info("TCON_CLK_CTRL       [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
	reg = TCON_STATUS1;
	pr_info("TCON_STATUS1        [0x%04x] = 0x%08x\n",
		reg, lcd_tcon_read(reg));
}

static void lcd_phy_analog_reg_print(void)
{
	unsigned int reg;

	pr_info("\nphy analog registers:\n");
	reg = HHI_DIF_CSI_PHY_CNTL1;
	pr_info("PHY_CNTL1           [0x%04x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL2;
	pr_info("PHY_CNTL2           [0x%04x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL3;
	pr_info("PHY_CNTL3           [0x%04x] = 0x%08x\n",
		reg, lcd_hiu_read(reg));
}

static void lcd_reg_print(void)
{
	int i;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;

	pconf = lcd_drv->lcd_config;
	LCDPR("clk registers:\n");
	for (i = 0; i < ARRAY_SIZE(lcd_reg_dump_clk); i++) {
		pr_info("hiu     [0x%04x] = 0x%08x\n",
			lcd_reg_dump_clk[i],
			lcd_hiu_read(lcd_reg_dump_clk[i]));
	}

	pr_info("\nencl registers:\n");
	for (i = 0; i < ARRAY_SIZE(lcd_reg_dump_encl); i++) {
		pr_info("vcbus   [0x%04x] = 0x%08x\n",
			lcd_reg_dump_encl[i],
			lcd_vcbus_read(lcd_reg_dump_encl[i]));
	}

	switch (pconf->lcd_basic.lcd_type) {
	case LCD_TTL:
		lcd_ttl_reg_print();
		break;
	case LCD_LVDS:
		lcd_lvds_reg_print();
		lcd_phy_analog_reg_print();
		break;
	case LCD_VBYONE:
		lcd_vbyone_reg_print();
		lcd_phy_analog_reg_print();
		break;
	case LCD_MLVDS:
		lcd_mlvds_reg_print();
		lcd_phy_analog_reg_print();
		break;
	case LCD_MIPI:
		break;
	case LCD_EDP:
		break;
	default:
		break;
	}
}

static void lcd_hdr_info_print(void)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;

	pconf = lcd_drv->lcd_config;
	pr_info("lcd hdr info:\n"
		"hdr_support          %d\n"
		"features             %d\n"
		"primaries_r_x        %d\n"
		"primaries_r_y        %d\n"
		"primaries_g_x        %d\n"
		"primaries_g_y        %d\n"
		"primaries_b_x        %d\n"
		"primaries_b_y        %d\n"
		"white_point_x        %d\n"
		"white_point_y        %d\n"
		"luma_max             %d\n"
		"luma_min             %d\n\n",
		pconf->hdr_info.hdr_support, pconf->hdr_info.features,
		pconf->hdr_info.primaries_r_x, pconf->hdr_info.primaries_r_y,
		pconf->hdr_info.primaries_g_x, pconf->hdr_info.primaries_g_y,
		pconf->hdr_info.primaries_b_x, pconf->hdr_info.primaries_b_y,
		pconf->hdr_info.white_point_x, pconf->hdr_info.white_point_y,
		pconf->hdr_info.luma_max, pconf->hdr_info.luma_min);
}

#define LCD_ENC_TST_NUM_MAX    9
static char *lcd_enc_tst_str[] = {
	"0-None",        /* 0 */
	"1-Color Bar",   /* 1 */
	"2-Thin Line",   /* 2 */
	"3-Dot Grid",    /* 3 */
	"4-Gray",        /* 4 */
	"5-Red",         /* 5 */
	"6-Green",       /* 6 */
	"7-Blue",        /* 7 */
	"8-Black",       /* 8 */
};

static unsigned int lcd_enc_tst[][8] = {
/*tst_mode,    Y,       Cb,     Cr,     tst_en,  vfifo_en  rgbin*/
	{0,    0x200,   0x200,  0x200,   0,      1,        3},  /* 0 */
	{1,    0x200,   0x200,  0x200,   1,      0,        1},  /* 1 */
	{2,    0x200,   0x200,  0x200,   1,      0,        1},  /* 2 */
	{3,    0x200,   0x200,  0x200,   1,      0,        1},  /* 3 */
	{0,    0x1ff,   0x1ff,  0x1ff,   1,      0,        3},  /* 4 */
	{0,    0x3ff,     0x0,    0x0,   1,      0,        3},  /* 5 */
	{0,      0x0,   0x3ff,    0x0,   1,      0,        3},  /* 6 */
	{0,      0x0,     0x0,  0x3ff,   1,      0,        3},  /* 7 */
	{0,      0x0,     0x0,    0x0,   1,      0,        3},  /* 8 */
};

static void lcd_debug_test(unsigned int num)
{
	unsigned int h_active, video_on_pixel;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	int flag;

	num = (num >= LCD_ENC_TST_NUM_MAX) ? 0 : num;
	flag = (num > 0) ? 1 : 0;
	aml_lcd_notifier_call_chain(LCD_EVENT_TEST_PATTERN, &flag);

	h_active = lcd_drv->lcd_config->lcd_basic.h_active;
	video_on_pixel = lcd_drv->lcd_config->lcd_timing.video_on_pixel;
	lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL, lcd_enc_tst[num][6]);
	lcd_vcbus_write(ENCL_TST_MDSEL, lcd_enc_tst[num][0]);
	lcd_vcbus_write(ENCL_TST_Y, lcd_enc_tst[num][1]);
	lcd_vcbus_write(ENCL_TST_CB, lcd_enc_tst[num][2]);
	lcd_vcbus_write(ENCL_TST_CR, lcd_enc_tst[num][3]);
	lcd_vcbus_write(ENCL_TST_CLRBAR_STRT, video_on_pixel);
	lcd_vcbus_write(ENCL_TST_CLRBAR_WIDTH, (h_active / 9));
	lcd_vcbus_write(ENCL_TST_EN, lcd_enc_tst[num][4]);
	lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, lcd_enc_tst[num][5], 3, 1);
	if (num > 0)
		LCDPR("show test pattern: %s\n", lcd_enc_tst_str[num]);
	else
		LCDPR("disable test pattern\n");
}

static void lcd_mute_setting(unsigned char flag)
{
	LCDPR("set lcd mute: %d\n", flag);
	if (flag) {
		lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL, 3);
		lcd_vcbus_write(ENCL_TST_MDSEL, 0);
		lcd_vcbus_write(ENCL_TST_Y, 0);
		lcd_vcbus_write(ENCL_TST_CB, 0);
		lcd_vcbus_write(ENCL_TST_CR, 0);
		lcd_vcbus_write(ENCL_TST_EN, 1);
		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, 0, 3, 1);
	} else {
		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, 1, 3, 1);
		lcd_vcbus_write(ENCL_TST_EN, 0);
	}
}

static void lcd_test_check(void)
{
	unsigned int h_active, video_on_pixel;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	unsigned int num;
	int flag;

	lcd_drv->lcd_mute = 0;

	num = lcd_drv->lcd_test_flag;
	num = (num >= LCD_ENC_TST_NUM_MAX) ? 0 : num;
	flag = (num > 0) ? 1 : 0;
	aml_lcd_notifier_call_chain(LCD_EVENT_TEST_PATTERN, &flag);

	h_active = lcd_drv->lcd_config->lcd_basic.h_active;
	video_on_pixel = lcd_drv->lcd_config->lcd_timing.video_on_pixel;
	if (num >= 0) {
		lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL, lcd_enc_tst[num][6]);
		lcd_vcbus_write(ENCL_TST_MDSEL, lcd_enc_tst[num][0]);
		lcd_vcbus_write(ENCL_TST_Y, lcd_enc_tst[num][1]);
		lcd_vcbus_write(ENCL_TST_CB, lcd_enc_tst[num][2]);
		lcd_vcbus_write(ENCL_TST_CR, lcd_enc_tst[num][3]);
		lcd_vcbus_write(ENCL_TST_CLRBAR_STRT, video_on_pixel);
		lcd_vcbus_write(ENCL_TST_CLRBAR_WIDTH, (h_active / 9));
		lcd_vcbus_write(ENCL_TST_EN, lcd_enc_tst[num][4]);
		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, lcd_enc_tst[num][5], 3, 1);
		if (num > 0)
			LCDPR("show test pattern: %s\n", lcd_enc_tst_str[num]);
	}
}

static void lcd_vinfo_update(void)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct vinfo_s *vinfo;
	struct lcd_config_s *pconf;

	vinfo = lcd_drv->lcd_info;
	pconf = lcd_drv->lcd_config;
	if (vinfo) {
		vinfo->width = pconf->lcd_basic.h_active;
		vinfo->height = pconf->lcd_basic.v_active;
		vinfo->field_height = pconf->lcd_basic.v_active;
		vinfo->aspect_ratio_num = pconf->lcd_basic.screen_width;
		vinfo->aspect_ratio_den = pconf->lcd_basic.screen_height;
		vinfo->screen_real_width = pconf->lcd_basic.screen_width;
		vinfo->screen_real_height = pconf->lcd_basic.screen_height;
		vinfo->sync_duration_num = pconf->lcd_timing.sync_duration_num;
		vinfo->sync_duration_den = pconf->lcd_timing.sync_duration_den;
		vinfo->video_clk = pconf->lcd_timing.lcd_clk;
		vinfo->htotal = pconf->lcd_basic.h_period;
		vinfo->vtotal = pconf->lcd_basic.v_period;
	}
	vout_notifier_call_chain(VOUT_EVENT_MODE_CHANGE,
		&lcd_drv->lcd_info->mode);
}

static void lcd_debug_config_update(void)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	lcd_drv->module_reset();

	lcd_vinfo_update();
}

static void lcd_debug_clk_change(unsigned int pclk)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;
	unsigned int sync_duration;

	pconf = lcd_drv->lcd_config;
	sync_duration = pclk / pconf->lcd_basic.h_period;
	sync_duration = sync_duration * 100 / pconf->lcd_basic.v_period;
	pconf->lcd_timing.lcd_clk = pclk;
	pconf->lcd_timing.lcd_clk_dft = pconf->lcd_timing.lcd_clk;
	pconf->lcd_timing.sync_duration_num = sync_duration;
	pconf->lcd_timing.sync_duration_den = 100;

	/* update vinfo */
	lcd_drv->lcd_info->sync_duration_num = sync_duration;
	lcd_drv->lcd_info->sync_duration_den = 100;
	lcd_drv->lcd_info->video_clk = pclk;

	switch (lcd_drv->lcd_mode) {
#ifdef CONFIG_AML_LCD_TV
	case LCD_MODE_TV:
		lcd_tv_clk_update(pconf);
		break;
#endif
#ifdef CONFIG_AML_LCD_TABLET
	case LCD_MODE_TABLET:
		lcd_tablet_clk_update(pconf);
		break;
#endif
	default:
		LCDPR("invalid lcd mode\n");
		break;
	}

	vout_notifier_call_chain(VOUT_EVENT_MODE_CHANGE,
		&lcd_drv->lcd_info->mode);
}

static ssize_t lcd_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp, val[6];
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;

	pconf = lcd_drv->lcd_config;
	switch (buf[0]) {
	case 'c': /* clk */
		ret = sscanf(buf, "clk %d", &temp);
		if (ret == 1) {
			if (temp > 200) {
				pr_info("set clk: %dHz\n", temp);
			} else {
				pr_info("set frame_rate: %dHz\n", temp);
				temp = pconf->lcd_basic.h_period *
					pconf->lcd_basic.v_period * temp;
				pr_info("set clk: %dHz\n", temp);
			}
			lcd_debug_clk_change(temp);
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'b':
		if (buf[1] == 'a') { /* basic */
			ret = sscanf(buf, "basic %d %d %d %d %d",
				&val[0], &val[1], &val[2], &val[3], &val[4]);
			if (ret == 4) {
				pconf->lcd_basic.h_active = val[0];
				pconf->lcd_basic.v_active = val[1];
				pconf->lcd_basic.h_period = val[2];
				pconf->lcd_basic.v_period = val[3];
				pconf->lcd_timing.h_period_dft = val[2];
				pconf->lcd_timing.v_period_dft = val[3];
				pr_info("set h_active=%d, v_active=%d\n",
					val[0], val[1]);
				pr_info("set h_period=%d, v_period=%d\n",
					val[2], val[3]);
				lcd_timing_init_config(pconf);
				lcd_debug_config_update();
			} else if (ret == 5) {
				pconf->lcd_basic.h_active = val[0];
				pconf->lcd_basic.v_active = val[1];
				pconf->lcd_basic.h_period = val[2];
				pconf->lcd_basic.v_period = val[3];
				pconf->lcd_timing.h_period_dft = val[2];
				pconf->lcd_timing.v_period_dft = val[3];
				pconf->lcd_basic.lcd_bits = val[4];
				pr_info("set h_active=%d, v_active=%d\n",
					val[0], val[1]);
				pr_info("set h_period=%d, v_period=%d\n",
					val[2], val[3]);
				pr_info("set lcd_bits=%d\n", val[4]);
				lcd_timing_init_config(pconf);
				lcd_debug_config_update();
			} else {
				LCDERR("invalid data\n");
				return -EINVAL;
			}
		} else if (buf[1] == 'i') { /* bit */
			ret = sscanf(buf, "bit %d", &val[0]);
			if (ret == 1) {
				pconf->lcd_basic.lcd_bits = val[4];
				pr_info("set lcd_bits=%d\n", val[4]);
				lcd_debug_config_update();
			} else {
				LCDERR("invalid data\n");
				return -EINVAL;
			}
		}
		break;
	case 's': /* sync */
		ret = sscanf(buf, "sync %d %d %d %d %d %d",
			&val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
		if (ret == 6) {
			pconf->lcd_timing.hsync_width = val[0];
			pconf->lcd_timing.hsync_bp =    val[1];
			pconf->lcd_timing.hsync_pol =   val[2];
			pconf->lcd_timing.vsync_width = val[3];
			pconf->lcd_timing.vsync_bp =    val[4];
			pconf->lcd_timing.vsync_pol =   val[5];
			pr_info("set hsync width=%d, bp=%d, pol=%d\n",
				val[0], val[1], val[2]);
			pr_info("set vsync width=%d, bp=%d, pol=%d\n",
				val[3], val[4], val[5]);
			lcd_timing_init_config(pconf);
			lcd_debug_config_update();
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	case 't': /* test */
		ret = sscanf(buf, "test %d", &temp);
		if (ret == 1) {
			lcd_drv->lcd_test_flag = (unsigned char)temp;
			lcd_debug_test(temp);
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'i': /* info */
		LCDPR("driver version: %s\n", lcd_drv->version);
		lcd_info_print();
		break;
	case 'r':
		if (buf[2] == 'g') { /* reg */
			LCDPR("driver version: %s\n", lcd_drv->version);
			lcd_reg_print();
		} else if (buf[2] == 's') { /* reset */
			lcd_drv->module_reset();
		} else if (buf[2] == 'n') { /* range */
			ret = sscanf(buf, "range %d %d %d %d %d %d",
				&val[0], &val[1], &val[2], &val[3],
				&val[4], &val[5]);
			if (ret == 6) {
				pconf->lcd_basic.h_period_min = val[0];
				pconf->lcd_basic.h_period_max = val[1];
				pconf->lcd_basic.h_period_min = val[2];
				pconf->lcd_basic.v_period_max = val[3];
				pconf->lcd_basic.lcd_clk_min  = val[4];
				pconf->lcd_basic.lcd_clk_max  = val[5];
				pr_info("set h_period min=%d, max=%d\n",
					pconf->lcd_basic.h_period_min,
					pconf->lcd_basic.h_period_max);
				pr_info("set v_period min=%d, max=%d\n",
					pconf->lcd_basic.v_period_min,
					pconf->lcd_basic.v_period_max);
				pr_info("set pclk min=%d, max=%d\n",
					pconf->lcd_basic.lcd_clk_min,
					pconf->lcd_basic.lcd_clk_max);
			} else {
				LCDERR("invalid data\n");
				return -EINVAL;
			}
		}
		break;
	case 'd': /* dump */
		LCDPR("driver version: %s\n", lcd_drv->version);
		lcd_info_print();
		pr_info("\n");
		lcd_reg_print();
		pr_info("\n");
		lcd_hdr_info_print();
		break;
	case 'k': /* key */
		LCDPR("key_valid: %d, config_load: %d\n",
			lcd_drv->lcd_key_valid, lcd_drv->lcd_config_load);
		if (lcd_drv->lcd_key_valid)
			lcd_unifykey_print();
		break;
	case 'h': /* hdr */
		lcd_hdr_info_print();
		break;
	case 'p': /* power */
		ret = sscanf(buf, "power %d", &temp);
		if (ret == 1) {
			mutex_lock(&lcd_power_mutex);
			LCDPR("power: %d\n", temp);
			if (temp) {
				aml_lcd_notifier_call_chain(
					LCD_EVENT_IF_POWER_ON, NULL);
			} else {
				aml_lcd_notifier_call_chain(
					LCD_EVENT_IF_POWER_OFF, NULL);
			}
			mutex_unlock(&lcd_power_mutex);
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'v':
		ret = sscanf(buf, "vout %d", &temp);
		if (ret == 1) {
			LCDPR("vout_serve bypass: %d\n", temp);
			lcd_vout_serve_bypass = temp;
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	default:
		LCDERR("wrong command\n");
		return -EINVAL;
		break;
	}

	return count;
}

static ssize_t lcd_debug_change_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_debug_change_usage_str);
}

static void lcd_debug_change_clk_change(unsigned int pclk)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;
	unsigned int sync_duration;

	pconf = lcd_drv->lcd_config;
	sync_duration = pclk / pconf->lcd_basic.h_period;
	sync_duration = sync_duration * 100 / pconf->lcd_basic.v_period;
	pconf->lcd_timing.lcd_clk = pclk;
	pconf->lcd_timing.lcd_clk_dft = pconf->lcd_timing.lcd_clk;
	pconf->lcd_timing.sync_duration_num = sync_duration;
	pconf->lcd_timing.sync_duration_den = 100;

	switch (lcd_drv->lcd_mode) {
#ifdef CONFIG_AML_LCD_TV
	case LCD_MODE_TV:
		lcd_tv_clk_config_change(pconf);
		break;
#endif
#ifdef CONFIG_AML_LCD_TABLET
	case LCD_MODE_TABLET:
		lcd_tablet_clk_config_change(pconf);
		break;
#endif
	default:
		LCDPR("invalid lcd mode\n");
		break;
	}
}

static ssize_t lcd_debug_change_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp, val[6];
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;

	pconf = lcd_drv->lcd_config;
	switch (buf[0]) {
	case 'c': /* clk */
		ret = sscanf(buf, "clk %d", &temp);
		if (ret == 1) {
			if (temp > 200) {
				pr_info("change clk=%dHz\n", temp);
			} else {
				pr_info("change frame_rate=%dHz\n", temp);
				temp = pconf->lcd_basic.h_period *
					pconf->lcd_basic.v_period * temp;
				pr_info("change clk=%dHz\n", temp);
			}
			lcd_debug_change_clk_change(temp);
			pconf->change_flag = 1;
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'b':
		if (buf[1] == 'a') { /* basic */
			ret = sscanf(buf, "basic %d %d %d %d %d",
				&val[0], &val[1], &val[2], &val[3], &val[4]);
			if (ret == 4) {
				pconf->lcd_basic.h_active = val[0];
				pconf->lcd_basic.v_active = val[1];
				pconf->lcd_basic.h_period = val[2];
				pconf->lcd_basic.v_period = val[3];
				pconf->lcd_timing.h_period_dft = val[2];
				pconf->lcd_timing.v_period_dft = val[3];
				pr_info("change h_active=%d, v_active=%d\n",
					val[0], val[1]);
				pr_info("change h_period=%d, v_period=%d\n",
					val[2], val[3]);
				lcd_timing_init_config(pconf);
				pconf->change_flag = 1;
			} else if (ret == 5) {
				pconf->lcd_basic.h_active = val[0];
				pconf->lcd_basic.v_active = val[1];
				pconf->lcd_basic.h_period = val[2];
				pconf->lcd_basic.v_period = val[3];
				pconf->lcd_timing.h_period_dft = val[2];
				pconf->lcd_timing.v_period_dft = val[3];
				pconf->lcd_basic.lcd_bits = val[4];
				pr_info("change h_active=%d, v_active=%d\n",
					val[0], val[1]);
				pr_info("change h_period=%d, v_period=%d\n",
					val[2], val[3]);
				pr_info("change lcd_bits=%d\n", val[4]);
				lcd_timing_init_config(pconf);
				pconf->change_flag = 1;
			} else {
				LCDERR("invalid data\n");
				return -EINVAL;
			}
		} else if (buf[1] == 'i') { /* bit */
			ret = sscanf(buf, "bit %d", &val[0]);
			if (ret == 1) {
				pconf->lcd_basic.lcd_bits = val[4];
				pr_info("change lcd_bits=%d\n", val[4]);
				pconf->change_flag = 1;
			} else {
				LCDERR("invalid data\n");
				return -EINVAL;
			}
		}
		break;
	case 's':
		if (buf[1] == 'e') { /* set */
			if (pconf->change_flag) {
				LCDPR("apply config changing\n");
				lcd_debug_config_update();
			} else {
				LCDPR("config is no changing\n");
			}
		} else if (buf[1] == 'y') { /* sync */
			ret = sscanf(buf, "sync %d %d %d %d %d %d",
				&val[0], &val[1], &val[2], &val[3],
				&val[4], &val[5]);
			if (ret == 6) {
				pconf->lcd_timing.hsync_width = val[0];
				pconf->lcd_timing.hsync_bp =    val[1];
				pconf->lcd_timing.hsync_pol =   val[2];
				pconf->lcd_timing.vsync_width = val[3];
				pconf->lcd_timing.vsync_bp =    val[4];
				pconf->lcd_timing.vsync_pol =   val[5];
				pr_info("change hs width=%d, bp=%d, pol=%d\n",
					val[0], val[1], val[2]);
				pr_info("change vs width=%d, bp=%d, pol=%d\n",
					val[3], val[4], val[5]);
				lcd_timing_init_config(pconf);
				pconf->change_flag = 1;
			} else {
				LCDERR("invalid data\n");
				return -EINVAL;
			}
		}
		break;
	default:
		LCDERR("wrong command\n");
		return -EINVAL;
		break;
	}

	return count;
}

static ssize_t lcd_debug_enable_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	return sprintf(buf, "lcd_status: %d\n",
		lcd_drv->lcd_status);
}

static ssize_t lcd_debug_enable_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp = 1;

	ret = sscanf(buf, "%d", &temp);
	if (ret == 1) {
		if (temp) {
			mutex_lock(&lcd_power_mutex);
			aml_lcd_notifier_call_chain(LCD_EVENT_POWER_ON, NULL);
			mutex_unlock(&lcd_power_mutex);
		} else {
			mutex_lock(&lcd_power_mutex);
			aml_lcd_notifier_call_chain(LCD_EVENT_POWER_OFF, NULL);
			mutex_unlock(&lcd_power_mutex);
		}
	} else {
		LCDERR("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t lcd_debug_resume_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	return sprintf(buf, "lcd resume flag: %d(%s)\n",
		lcd_drv->lcd_resume_flag,
		lcd_drv->lcd_resume_flag ? "workqueue" : "directly");
}

static ssize_t lcd_debug_resume_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	unsigned int temp = 1;

	ret = sscanf(buf, "%d", &temp);
	if (ret == 1) {
		lcd_drv->lcd_resume_flag = (unsigned char)temp;
		LCDPR("set lcd resume flag: %d\n", lcd_drv->lcd_resume_flag);
	} else {
		LCDERR("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t lcd_debug_power_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	lcd_power_info_print(lcd_drv->lcd_config, 1);
	lcd_power_info_print(lcd_drv->lcd_config, 0);

	return sprintf(buf, "\n");
}

static ssize_t lcd_debug_power_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int i, delay;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_power_ctrl_s *lcd_power;

	lcd_power = lcd_drv->lcd_config->lcd_power;
	switch (buf[1]) {
	case 'n': /* on */
		ret = sscanf(buf, "on %d %d", &i, &delay);
		if (ret == 2) {
			if (i >= lcd_power->power_on_step_max) {
				pr_info("invalid power_on step: %d\n", i);
				return -EINVAL;
			}
			lcd_power->power_on_step[i].delay = delay;
			pr_info("set power_on step %d delay: %dms\n",
				i, delay);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'f': /* off */
		ret = sscanf(buf, "off %d %d", &i, &delay);
		if (ret == 1) {
			if (i >= lcd_power->power_off_step_max) {
				pr_info("invalid power_off step: %d\n", i);
				return -EINVAL;
			}
			lcd_power->power_off_step[i].delay = delay;
			pr_info("set power_off step %d delay: %dms\n",
				i, delay);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	default:
		pr_info("wrong command\n");
		return -EINVAL;
		break;
	}

	return count;
}

static ssize_t lcd_debug_frame_rate_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	unsigned int sync_duration;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;
	ssize_t len;

	pconf = lcd_drv->lcd_config;
	sync_duration = pconf->lcd_timing.sync_duration_num * 100;
	sync_duration = sync_duration / pconf->lcd_timing.sync_duration_den;

	len = sprintf(buf, "get frame_rate: %u.%02uHz, fr_adjust_type: %d, ",
		(sync_duration / 100), (sync_duration % 100),
		pconf->lcd_timing.fr_adjust_type);
	len += sprintf(buf+len, "vmode_vsync_en: %d\n",
		lcd_drv->lcd_vmode_vsync_en);

	return len;
}

static ssize_t lcd_debug_frame_rate_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	switch (buf[0]) {
	case 't':
		ret = sscanf(buf, "type %d", &temp);
		if (ret == 1) {
			lcd_drv->lcd_config->lcd_timing.fr_adjust_type = temp;
			pr_info("set fr_adjust_type: %d\n", temp);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	case 's':
		ret = sscanf(buf, "set %d", &temp);
		if (ret == 1) {
			pr_info("set frame rate(*100): %d\n", temp);
			aml_lcd_notifier_call_chain(
				LCD_EVENT_FRAME_RATE_ADJUST, &temp);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'v':
		ret = sscanf(buf, "vsync %d", &temp);
		if (ret == 1) {
			pr_info("set vsync_en: %d\n", temp);
			lcd_drv->lcd_vmode_vsync_en = (unsigned char)temp;
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	default:
		pr_info("wrong command\n");
		return -EINVAL;
		break;
	}

	return count;
}

static ssize_t lcd_debug_fr_policy_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	return sprintf(buf, "fr_auto_policy: %d\n", lcd_drv->fr_auto_policy);
}

static ssize_t lcd_debug_fr_policy_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	ret = sscanf(buf, "%d", &temp);
	if (ret == 1) {
		lcd_drv->fr_auto_policy = temp;
		pr_info("set fr_auto_policy: %d\n", temp);
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t lcd_debug_ss_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "get lcd pll spread spectrum: %s\n",
			lcd_get_spread_spectrum());
}

static ssize_t lcd_debug_ss_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	ret = sscanf(buf, "%d", &temp);
	if (ret == 1) {
		lcd_drv->lcd_config->lcd_timing.ss_level = temp;
		lcd_set_spread_spectrum();
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t lcd_debug_clk_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	lcd_clk_config_print();
	return sprintf(buf, "\n");
}

static ssize_t lcd_debug_test_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	ret = sscanf(buf, "%d", &temp);
	if (ret == 1) {
		lcd_drv->lcd_test_flag = (unsigned char)temp;
		lcd_debug_test(temp);
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t lcd_debug_mute_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	return sprintf(buf, "get lcd mute state: %d\n", lcd_drv->lcd_mute);
}

static ssize_t lcd_debug_mute_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();

	ret = kstrtouint(buf, 10, &temp);
	lcd_drv->lcd_mute = (unsigned char)temp;
	lcd_mute_setting(lcd_drv->lcd_mute);

	return count;
}

static void lcd_debug_reg_write(unsigned int reg, unsigned int data,
		unsigned int type)
{
	switch (type) {
	case 0: /* vcbus */
		lcd_vcbus_write(reg, data);
		pr_info("write vcbus [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_vcbus_read(reg));
		break;
	case 1: /* hiu */
		lcd_hiu_write(reg, data);
		pr_info("write hiu [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_hiu_read(reg));
		break;
	case 2: /* cbus */
		lcd_cbus_write(reg, data);
		pr_info("write cbus [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_cbus_read(reg));
		break;
	case 3: /* periphs */
		lcd_periphs_write(reg, data);
		pr_info("write periphs [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_periphs_read(reg));
		break;
	case 4: /* tcon */
		lcd_tcon_write(reg, data);
		pr_info("write tcon [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_tcon_read(reg));
		break;
	default:
		break;
	}
}

static void lcd_debug_reg_read(unsigned int reg, unsigned int bus)
{
	switch (bus) {
	case 0: /* vcbus */
		pr_info("read vcbus [0x%04x] = 0x%08x\n",
			reg, lcd_vcbus_read(reg));
		break;
	case 1: /* hiu */
		pr_info("read hiu [0x%04x] = 0x%08x\n",
			reg, lcd_hiu_read(reg));
		break;
	case 2: /* cbus */
		pr_info("read cbus [0x%04x] = 0x%08x\n",
			reg, lcd_cbus_read(reg));
		break;
	case 3: /* periphs */
		pr_info("read periphs [0x%04x] = 0x%08x\n",
			reg, lcd_periphs_read(reg));
		break;
	case 4: /* tcon */
		pr_info("read tcon [0x%04x] = 0x%08x\n",
			reg, lcd_tcon_read(reg));
		break;
	default:
		break;
	}
}

static void lcd_debug_reg_dump(unsigned int reg, unsigned int num,
		unsigned int bus)
{
	int i;

	switch (bus) {
	case 0: /* vcbus */
		pr_info("dump vcbus regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_vcbus_read(reg + i));
		}
		break;
	case 1: /* hiu */
		pr_info("dump hiu-bus regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_hiu_read(reg + i));
		}
		break;
	case 2: /* cbus */
		pr_info("dump cbus regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_cbus_read(reg + i));
		}
		break;
	case 3: /* periphs */
		pr_info("dump periphs-bus regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_periphs_read(reg + i));
		}
		break;
	case 4: /* tcon */
		pr_info("dump tcon regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_tcon_read(reg + i));
		}
		break;
	default:
		break;
	}
}

static ssize_t lcd_debug_reg_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int bus = 0;
	unsigned int reg32 = 0, data32 = 0;

	switch (buf[0]) {
	case 'w':
		if (buf[1] == 'v') {
			ret = sscanf(buf, "wv %x %x", &reg32, &data32);
			bus = 0;
		} else if (buf[1] == 'h') {
			ret = sscanf(buf, "wh %x %x", &reg32, &data32);
			bus = 1;
		} else if (buf[1] == 'c') {
			ret = sscanf(buf, "wc %x %x", &reg32, &data32);
			bus = 2;
		} else if (buf[1] == 'p') {
			ret = sscanf(buf, "wp %x %x", &reg32, &data32);
			bus = 3;
		} else if (buf[1] == 't') {
			ret = sscanf(buf, "wt %x %x", &reg32, &data32);
			bus = 4;
		}
		if (ret == 2) {
			lcd_debug_reg_write(reg32, data32, bus);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'r':
		if (buf[1] == 'v') {
			ret = sscanf(buf, "rv %x", &reg32);
			bus = 0;
		} else if (buf[1] == 'h') {
			ret = sscanf(buf, "rh %x", &reg32);
			bus = 1;
		} else if (buf[1] == 'c') {
			ret = sscanf(buf, "rc %x", &reg32);
			bus = 2;
		} else if (buf[1] == 'p') {
			ret = sscanf(buf, "rp %x", &reg32);
			bus = 3;
		} else if (buf[1] == 't') {
			ret = sscanf(buf, "rt %x", &reg32);
			bus = 4;
		}
		if (ret == 1) {
			lcd_debug_reg_read(reg32, bus);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'd':
		if (buf[1] == 'v') {
			ret = sscanf(buf, "dv %x %d", &reg32, &data32);
			bus = 0;
		} else if (buf[1] == 'h') {
			ret = sscanf(buf, "dh %x %d", &reg32, &data32);
			bus = 1;
		} else if (buf[1] == 'c') {
			ret = sscanf(buf, "dc %x %d", &reg32, &data32);
			bus = 2;
		} else if (buf[1] == 'p') {
			ret = sscanf(buf, "dp %x %d", &reg32, &data32);
			bus = 3;
		} else if (buf[1] == 't') {
			ret = sscanf(buf, "dt %x %d", &reg32, &data32);
			bus = 4;
		}
		if (ret == 2) {
			lcd_debug_reg_dump(reg32, data32, bus);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	default:
		pr_info("wrong command\n");
		return -EINVAL;
		break;
	}

	return count;
}

static unsigned int lcd_dither_en = 1;
static unsigned int lcd_dither_round_en;
static unsigned int lcd_dither_md;
static void lcd_vpu_dither_setting(unsigned int lcd_dither_en,
		unsigned int lcd_dither_round_en, unsigned int lcd_dither_md)
{
	unsigned int data32;

	data32 = lcd_vcbus_read(VPU_VENCL_DITH_CTRL);
	data32 &= ~((1 << 0) | (1 << 1) | (1 << 2));
	data32 |= ((lcd_dither_en << 0) |
		(lcd_dither_round_en << 1) |
		(lcd_dither_md << 2));
	lcd_vcbus_write(VPU_VENCL_DITH_CTRL, data32);
}

static ssize_t lcd_debug_dither_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	ssize_t len = 0;

	switch (lcd_drv->chip_type) {
	case LCD_CHIP_TXLX:
		len = sprintf(buf, "get dither status:\n"
			"dither_en:        %d\n"
			"dither_round_en:  %d\n"
			"dither_md:        %d\n",
			lcd_dither_en, lcd_dither_round_en, lcd_dither_md);
		break;
	default:
		len = sprintf(buf,
			"don't support dither function for current chip\n");
		break;
	}

	return len;
}

static ssize_t lcd_debug_dither_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	int ret = -1;
	unsigned int temp = 0;

	switch (lcd_drv->chip_type) {
	case LCD_CHIP_TXLX:
		ret = 0;
		break;
	default:
		ret = -1;
		LCDPR("don't support dither function for current chip\n");
		break;
	}
	if (ret)
		return count;

	switch (buf[0]) {
	case 'e': /* en */
		ret = sscanf(buf, "en %d", &temp);
		if (ret == 1) {
			if (temp)
				lcd_dither_en = 1;
			else
				lcd_dither_en = 0;
			lcd_vpu_dither_setting(lcd_dither_en,
				lcd_dither_round_en, lcd_dither_md);
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'r': /* round */
		ret = sscanf(buf, "round %d", &temp);
		if (ret == 1) {
			if (temp)
				lcd_dither_round_en = 1;
			else
				lcd_dither_round_en = 0;
			lcd_vpu_dither_setting(lcd_dither_en,
				lcd_dither_round_en, lcd_dither_md);
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'm': /* md */
		ret = sscanf(buf, "method %d", &temp);
		if (ret == 1) {
			if (temp)
				lcd_dither_md = 1;
			else
				lcd_dither_md = 0;
			lcd_vpu_dither_setting(lcd_dither_en,
				lcd_dither_round_en, lcd_dither_md);
		} else {
			LCDERR("invalid data\n");
			return -EINVAL;
		}
		break;
	default:
		LCDERR("wrong command\n");
		return -EINVAL;
	}

	return count;
}

static const char *lcd_debug_tcon_usage_str = {
	"Usage:\n"
	"    echo reg > tcon ; print tcon system regs\n"
	"    echo reg save <path> > tcon ; save tcon system regs to bin file\n"
	"\n"
	"    echo table > tcon ; print tcon reg table\n"
	"    echo table r <index> > tcon ; read tcon reg table by specified index\n"
	"    echo table w <index> <value> > tcon ; write tcon reg table by specified index\n"
	"    echo table d <index> <len> > tcon ; dump tcon reg table\n"
	"data format:\n"
	"    <index>    : hex number\n"
	"    <value>    : hex number\n"
	"    <len>      : dec number\n"
	"\n"
	"    echo table update > tcon ; update tcon reg table into tcon system regs\n"
	"    echo table save <path> > tcon ; save tcon reg table to bin file\n"
	"\n"
	"    echo od <en> > tcon ; tcon over driver control\n"
	"data format:\n"
	"    <en>       : 0=disable, 1=enable\n"
};

static ssize_t lcd_debug_tcon_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_debug_tcon_usage_str);
}

static void lcd_tcon_reg_table_save(char *path,
		struct mlvds_config_s *mlvds_conf)
{
	struct file *filp = NULL;
	loff_t pos = 0;
	void *buf = NULL;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	filp = filp_open(path, O_RDWR|O_CREAT, 0666);

	if (IS_ERR(filp)) {
		LCDERR("%s: create %s error\n", __func__, path);
		return;
	}

	pos = 0;
	buf = mlvds_conf->reg_table;
	vfs_write(filp, buf, LCD_TCON_TABLE_MAX, &pos);

	vfs_fsync(filp, 0);
	filp_close(filp, NULL);
	set_fs(old_fs);

	LCDPR("save tcon reg table to %s finished\n", path);
}

static void lcd_tcon_reg_save(char *path)
{
	struct file *filp = NULL;
	loff_t pos = 0;
	unsigned char *temp;
	void *buf = NULL;
	mm_segment_t old_fs = get_fs();
	int i;

	set_fs(KERNEL_DS);
	filp = filp_open(path, O_RDWR|O_CREAT, 0666);

	if (IS_ERR(filp)) {
		LCDERR("%s: create %s error\n", __func__, path);
		return;
	}

	temp = kzalloc((sizeof(unsigned char) * LCD_TCON_TABLE_MAX),
		GFP_KERNEL);
	if (!temp) {
		LCDERR("%s: Not enough memory\n", __func__);
		return;
	}
	for (i = 0; i < LCD_TCON_TABLE_MAX; i++)
		temp[i] = lcd_tcon_read(i + TCON_SYS_REG_START);

	pos = 0;
	buf = (void *)temp;
	vfs_write(filp, buf, LCD_TCON_TABLE_MAX, &pos);

	vfs_fsync(filp, 0);
	filp_close(filp, NULL);
	set_fs(old_fs);

	LCDPR("save tcon reg to %s finished\n", path);
}

static ssize_t lcd_debug_tcon_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct mlvds_config_s *mlvds_conf;
	char *buf_orig;
	char *parm[47] = {NULL};
	long val = 0;
	unsigned int temp = 0, i, n;
	unsigned char data;
	int ret = -1;

	switch (lcd_drv->chip_type) {
	case LCD_CHIP_TXHD:
		ret = 0;
		break;
	default:
		ret = -1;
		LCDPR("don't support tcon for current chip\n");
		break;
	}
	if (ret)
		return count;

	mlvds_conf = lcd_drv->lcd_config->lcd_control.mlvds_config;
	if (mlvds_conf == NULL) {
		LCDERR("%s: invalid minilvds config\n", __func__);
		return count;
	}
	if (mlvds_conf->tcon_enable == 0) {
		LCDERR("%s: invalid tcon\n", __func__);
		return count;
	}
	if (mlvds_conf->reg_table == NULL) {
		LCDERR("%s: invalid table\n", __func__);
		return count;
	}

	if (!buf)
		return count;
	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (buf_orig == NULL) {
		LCDERR("%s: buf malloc error\n", __func__);
		return count;
	}
	lcd_debug_parse_param(buf_orig, (char **)&parm);

	if (strcmp(parm[0], "reg") == 0) {
		if (parm[1] == NULL) {
			lcd_tcon_regs_readback_print(mlvds_conf);
			return count;
		}
		if (strcmp(parm[1], "save") == 0) {
			if (parm[2] != NULL)
				lcd_tcon_reg_save(parm[2]);
			else
				pr_info("invalid save path\n");
		}
	} else if (strcmp(parm[0], "table") == 0) {
		if (parm[1] == NULL) {
			lcd_tcon_regs_table_print(mlvds_conf);
			return count;
		}
		if (strcmp(parm[1], "r") == 0) {
			if (parm[2] != NULL) {
				ret = 0;
				if (!kstrtol(parm[2], 16, &val))
					temp = (unsigned int)val;
				else
					ret = 1;
				if (ret) {
					pr_info("invalid parameters\n");
					return count;
				}
				if (temp <= 0xfff) {
					data = mlvds_conf->reg_table[temp];
					pr_info("read tcon table[%d]=0x%02x\n",
						temp, data);
				} else {
					pr_info("invalid table index: %d\n",
						temp);
				}
			}
		} else if (strcmp(parm[1], "w") == 0) {
			if (parm[3] != NULL) {
				ret = 0;
				if (!kstrtol(parm[2], 16, &val))
					temp = (unsigned int)val;
				else
					ret = 1;
				if (!kstrtol(parm[3], 16, &val))
					data = (unsigned char)val;
				else
					ret = 1;
				if (ret) {
					pr_info("invalid parameters\n");
					return count;
				}
				if (temp <= 0xfff) {
					mlvds_conf->reg_table[temp] = data;
					pr_info("write tcon table[%d]=0x%02x\n",
						temp, data);
				} else {
					pr_info("invalid table index: %d\n",
						temp);
				}
			}
		} else if (strcmp(parm[1], "d") == 0) {
			if (parm[3] != NULL) {
				ret = 0;
				if (!kstrtol(parm[2], 16, &val))
					temp = (unsigned int)val;
				else
					ret = 1;
				if (!kstrtol(parm[3], 16, &val))
					n = (unsigned char)val;
				else
					ret = 1;
				if (ret) {
					pr_info("invalid parameters\n");
					return count;
				}
				pr_info("dump tcon table:\n");
				for (i = temp; i < (temp + n); i++) {
					if (i > 0xfff)
						break;
					data = mlvds_conf->reg_table[i];
					pr_info("  [%d]=0x%02x\n", temp, data);
				}
			}
		} else if (strcmp(parm[1], "update") == 0) {
			lcd_tcon_sys_regs_update(mlvds_conf->reg_table,
				LCD_TCON_TABLE_MAX);
		} else if (strcmp(parm[1], "save") == 0) {
			if (parm[2] != NULL)
				lcd_tcon_reg_table_save(parm[2], mlvds_conf);
			else
				pr_info("invalid save path\n");
		}
	} else if (strcmp(parm[0], "od") == 0) { /* over drive */
		if (parm[1] != NULL) {
			if (strcmp(parm[1], "status") == 0) {
				temp = lcd_tcon_od_get();
				if (temp) {
					LCDPR("tcon od is enabled: %d\n", temp);
				} else {
					LCDPR("tcon od is disabled: %d\n",
						temp);
				}
			} else {
				if (!kstrtol(parm[1], 10, &val)) {
					temp = (unsigned int)val;
					if (temp)
						lcd_tcon_od_set(1);
					else
						lcd_tcon_od_set(0);
				}
			}
		}
	} else {
		LCDERR("wrong command\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t lcd_debug_print_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "get debug print flag: %d\n", lcd_debug_print_flag);
}

static ssize_t lcd_debug_print_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int temp = 0;

	ret = sscanf(buf, "%d", &temp);
	if (ret == 1) {
		lcd_debug_print_flag = (unsigned char)temp;
		LCDPR("set debug print flag: %d\n", lcd_debug_print_flag);
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static struct class_attribute lcd_debug_class_attrs[] = {
	__ATTR(help,        S_IRUGO | S_IWUSR, lcd_debug_common_help, NULL),
	__ATTR(debug,       S_IRUGO | S_IWUSR, lcd_debug_show, lcd_debug_store),
	__ATTR(change,      S_IRUGO | S_IWUSR, lcd_debug_change_show,
		lcd_debug_change_store),
	__ATTR(enable,      S_IRUGO | S_IWUSR, lcd_debug_enable_show,
		lcd_debug_enable_store),
	__ATTR(resume,      S_IRUGO | S_IWUSR, lcd_debug_resume_show,
		lcd_debug_resume_store),
	__ATTR(power,       S_IRUGO | S_IWUSR,
		lcd_debug_power_show, lcd_debug_power_store),
	__ATTR(frame_rate,  S_IRUGO | S_IWUSR,
		lcd_debug_frame_rate_show, lcd_debug_frame_rate_store),
	__ATTR(fr_policy,   S_IRUGO | S_IWUSR,
		lcd_debug_fr_policy_show, lcd_debug_fr_policy_store),
	__ATTR(ss,          S_IRUGO | S_IWUSR,
		lcd_debug_ss_show, lcd_debug_ss_store),
	__ATTR(clk,         S_IRUGO | S_IWUSR, lcd_debug_clk_show, NULL),
	__ATTR(test,        S_IRUGO | S_IWUSR, NULL, lcd_debug_test_store),
	__ATTR(mute,        S_IRUGO | S_IWUSR, lcd_debug_mute_show,
		lcd_debug_mute_store),
	__ATTR(reg,         S_IRUGO | S_IWUSR, NULL, lcd_debug_reg_store),
	__ATTR(dither,      S_IRUGO | S_IWUSR,
		lcd_debug_dither_show, lcd_debug_dither_store),
	__ATTR(tcon,        S_IRUGO | S_IWUSR,
		lcd_debug_tcon_show, lcd_debug_tcon_store),
	__ATTR(print,       S_IRUGO | S_IWUSR,
		lcd_debug_print_show, lcd_debug_print_store),
};

static const char *lcd_ttl_debug_usage_str = {
"Usage:\n"
"    echo <clk_pol> <de_valid> <hvsync_valid> <rb_swpa> <bit_swap> > ttl ; set ttl config\n"
"data format:\n"
"    <clk_pol>      : 0=negative, 1=positive\n"
"    <de_valid>     : for DE, 0=invalid, 1=valid\n"
"    <hvsync_valid> : for hvsync, 0=invalid, 1=valid\n"
"    <rb_swpa>      : for R/B port, 0=normal, 1=swap\n"
"    <bit_swap>     : for RGB MSB/LSB, 0=normal, 1=swap\n"
"\n"
};

static const char *lcd_lvds_debug_usage_str = {
"Usage:\n"
"    echo <repack> <dual_port> <pn_swap> <port_swap> <lane_reverse> > lvds ; set lvds config\n"
"data format:\n"
"    <repack>    : 0=JEIDA mode, 1=VESA mode(8bit), 2=VESA mode(10bit)\n"
"    <dual_port> : 0=single port, 1=dual port\n"
"    <pn_swap>   : 0=normal, 1=swap p/n channels\n"
"    <port_swap> : 0=normal, 1=swap A/B port\n"
"	 <lane_reverse> : 0=normal, 1=swap A0-A4/B0-B4\n"
"\n"
"    echo <vswing> <preem> > phy ; set vbyone phy config\n"
"data format:\n"
"    <vswing> : vswing level, support 0~7\n"
"    <preem>  : preemphasis level, support 0~7\n"
"\n"
};

static const char *lcd_vbyone_debug_usage_str = {
"Usage:\n"
"    echo <lane_count> <region_num> <byte_mode> > vbyone ; set vbyone config\n"
"data format:\n"
"    <lane_count> : 4/8/16\n"
"    <region_num> : 1/2\n"
"    <byte_mode>  : 3/4/5\n"
"\n"
"    echo <vswing> <preem> > phy ; set vbyone phy config\n"
"data format:\n"
"    <vswing> : vswing level, support 0~7\n"
"    <preem>  : preemphasis level, support 0~7\n"
"    <byte_mode>  : 3/4/5\n"
"\n"
"    echo intr <state> <en> > vbyone; enable or disable vbyone interrupt\n"
"data format:\n"
"    <state> : 0=temp no use intr, 1=temp use intr. keep effect until reset lcd driver\n"
"    <en>    : 0=disable intr, 1=enable intr\n"
"\n"
"    echo vintr <en> > vbyone; enable or disable vbyone interrupt\n"
"data format:\n"
"    <en>    : 0=disable vsync monitor intr, 1=enable vsync monitor intr\n"
"\n"
"    echo ctrl <ctrl_flag> <power_on_reset_delay> <hpd_data_delay> <cdr_training_hold> > vbyone; set ctrl adjust\n"
"data format:\n"
"    <ctrl_flag>    : bit[0]:power_on_reset_en, bit[1]:hpd_data_delay_en, bit[2]:cdr_training_hold_en\n"
"    others         : unit in ms\n"
"\n"
};

static const char *lcd_mipi_debug_usage_str = {
"Usage:\n"
"    echo <lane_num> <bit_rate_max> <factor> <op_mode_init> <op_mode_disp> <vid_mode_type> <clk_lp_continuous> <phy_stop_wait> > mipi ; set mpi config\n"
"data format:\n"
"    <lane_num>          : 1/2/3/4\n"
"    <bit_rate_max>      : unit in MHz\n"
"    <factor>:           : special adjust, 0 for default\n"
"    <op_mode_init>      : operation mode for init (0=video mode, 1=command mode)\n"
"    <op_mode_disp>      : operation mode for display (0=video mode, 1=command mode)\n"
"    <vid_mode_type>     : video mode type (0=sync_pulse, 1=sync_event, 2=burst)\n"
"    <clk_lp_continuous> : 0=stop, 1=continue\n"
"    <phy_stop_wait>     : 0=auto, 1=standard, 2=slow\n"
"\n"
};

static const char *lcd_edp_debug_usage_str = {
"Usage:\n"
"    echo <link_rate> <lane_count> <edid_timing_used> <sync_clock_mode> > edp ; set edp config\n"
"data format:\n"
"    <link_rate>        : 0=1.62G, 1=2.7G\n"
"    <lane_count>       : 1/2/4\n"
"    <edid_timing_used> : 0=no use, 1=use, default=0\n"
"    <sync_clock_mode>  : 0=asyncronous, 1=synchronous, default=1\n"
"\n"
};

static const char *lcd_mlvds_debug_usage_str = {
"Usage:\n"
"    echo <channel_num> <channel_sel0> <channel_sel1> <clk_phase> <pn_swap> <bit_swap> > minilvds ; set minilvds config\n"
"data format:\n"
"    <channel_sel> : minilvds 8 channels mapping in tx 10 channels\n"
"    <clk_phase>   : bit[13:12]=clk01_pi_sel, bit[11:8]=pi2, bit[7:4]=pi1, bit[3:0]=pi0\n"
"    <pn_swap>     : 0=normal, 1=swap p/n channels\n"
"    <bit_swap>    : 0=normal, 1=swap bit LSB/MSB\n"
"\n"
};

static ssize_t lcd_ttl_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_ttl_debug_usage_str);
}

static ssize_t lcd_lvds_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_lvds_debug_usage_str);
}

static ssize_t lcd_vx1_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_vbyone_debug_usage_str);
}

static ssize_t lcd_mipi_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_mipi_debug_usage_str);
}

static ssize_t lcd_edp_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_edp_debug_usage_str);
}

static ssize_t lcd_mlvds_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", lcd_mlvds_debug_usage_str);
}

static ssize_t lcd_ttl_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct ttl_config_s *ttl_conf;
	unsigned int temp[5];

	ttl_conf = lcd_drv->lcd_config->lcd_control.ttl_config;
	ret = sscanf(buf, "%d %d %d %d %d",
		&temp[0], &temp[1], &temp[2], &temp[3], &temp[4]);
	if (ret == 5) {
		pr_info("set ttl config:\n"
			"clk_pol=%d, de_valid=%d, de_valid=%d\n"
			"rb_swap=%d, bit_swap=%d\n",
			temp[0], temp[1], temp[2], temp[3], temp[4]);
		ttl_conf->clk_pol = temp[0];
		ttl_conf->sync_valid = ((temp[1] << 1) | temp[2]);
		ttl_conf->swap_ctrl = ((temp[3] << 1) | temp[4]);
		lcd_debug_config_update();
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t lcd_lvds_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lvds_config_s *lvds_conf;

	lvds_conf = lcd_drv->lcd_config->lcd_control.lvds_config;
	ret = sscanf(buf, "%d %d %d %d %d",
		&lvds_conf->lvds_repack, &lvds_conf->dual_port,
		&lvds_conf->pn_swap, &lvds_conf->port_swap,
		&lvds_conf->lane_reverse);
	if (ret == 5 || ret == 4) {
		pr_info("set lvds config:\n"
			"repack=%d, dual_port=%d, pn_swap=%d, port_swap=%d, lane_reverse=%d\n",
			lvds_conf->lvds_repack, lvds_conf->dual_port,
			lvds_conf->pn_swap, lvds_conf->port_swap,
			lvds_conf->lane_reverse);
		lcd_debug_config_update();
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static int vx1_intr_state = 1;
static ssize_t lcd_vx1_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct vbyone_config_s *vx1_conf;
	int val[5];

	vx1_conf = lcd_drv->lcd_config->lcd_control.vbyone_config;
	if (buf[0] == 'i') { /* intr */
		ret = sscanf(buf, "intr %d %d", &val[0], &val[1]);
		if (ret == 1) {
			pr_info("set vbyone interrupt enable: %d\n", val[0]);
			vx1_intr_state = val[0];
			lcd_vbyone_interrupt_enable(vx1_intr_state);
		} else if (ret == 2) {
			pr_info("set vbyone interrupt enable: %d %d\n",
				val[0], val[1]);
			vx1_intr_state = val[0];
			vx1_conf->intr_en = val[1];
			lcd_vbyone_interrupt_enable(vx1_intr_state);
		} else {
			pr_info("vx1_intr_enable: %d %d\n",
				vx1_intr_state, vx1_conf->intr_en);
			return -EINVAL;
		}
	} else if (buf[0] == 'v') { /* vintr */
		ret = sscanf(buf, "vintr %d", &val[0]);
		if (ret == 1) {
			pr_info("set vbyone vsync interrupt enable: %d\n",
				val[0]);
			vx1_conf->vsync_intr_en = val[0];
			lcd_vbyone_interrupt_enable(vx1_intr_state);
		} else {
			pr_info("vx1_vsync_intr_enable: %d\n",
				vx1_conf->vsync_intr_en);
			return -EINVAL;
		}
	} else if (buf[0] == 'c') { /* ctrl */
		ret = sscanf(buf, "ctrl %x %d %d %d",
			&val[0], &val[1], &val[2], &val[3]);
		if (ret == 4) {
			pr_info("set vbyone ctrl_flag: 0x%x\n", val[0]);
			pr_info("power_on_reset_delay: %dms\n", val[1]);
			pr_info("hpd_data_delay: %dms\n", val[2]);
			pr_info("cdr_training_hold: %dms\n", val[3]);
			vx1_conf->ctrl_flag = val[0];
			vx1_conf->power_on_reset_delay = val[1];
			vx1_conf->hpd_data_delay = val[2];
			vx1_conf->cdr_training_hold = val[3];
			lcd_debug_config_update();
		} else {
			pr_info("set vbyone ctrl_flag: 0x%x\n",
				vx1_conf->ctrl_flag);
			pr_info("power_on_reset_delay: %dms\n",
				vx1_conf->power_on_reset_delay);
			pr_info("hpd_data_delay: %dms\n",
				vx1_conf->hpd_data_delay);
			pr_info("cdr_training_hold: %dms\n",
				vx1_conf->cdr_training_hold);
			return -EINVAL;
		}
	} else {
		ret = sscanf(buf, "%d %d %d", &vx1_conf->lane_count,
			&vx1_conf->region_num, &vx1_conf->byte_mode);
		if (ret == 3) {
			pr_info("set vbyone config:\n"
				"lane_count=%d, region_num=%d, byte_mode=%d\n",
				vx1_conf->lane_count, vx1_conf->region_num,
				vx1_conf->byte_mode);
			lcd_debug_config_update();
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
	}

	return count;
}

static ssize_t lcd_mipi_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	pr_info("to do\n");

	return count;
}

static ssize_t lcd_edp_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	pr_info("to do\n");

	return count;
}

static ssize_t lcd_mlvds_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct mlvds_config_s *mlvds_conf;

	mlvds_conf = lcd_drv->lcd_config->lcd_control.mlvds_config;
	ret = sscanf(buf, "%d %x %x %x %d %d",
		&mlvds_conf->channel_num,
		&mlvds_conf->channel_sel0, &mlvds_conf->channel_sel1,
		&mlvds_conf->clk_phase,
		&mlvds_conf->pn_swap, &mlvds_conf->bit_swap);
	if (ret == 6) {
		pr_info("set minilvds config:\n"
			"channel_num=%d,\n"
			"channel_sel0=0x%08x, channel_sel1=0x%08x,\n"
			"clk_phase=0x%04x,\n"
			"pn_swap=%d, bit_swap=%d\n",
			mlvds_conf->channel_num,
			mlvds_conf->channel_sel0, mlvds_conf->channel_sel1,
			mlvds_conf->clk_phase,
			mlvds_conf->pn_swap, mlvds_conf->bit_swap);
		lcd_debug_config_update();
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static void lcd_phy_config_update(unsigned int *para, int cnt)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;
	struct lvds_config_s *lvdsconf;
	int type;
	unsigned int data32, vswing, preem, ext_pullup;

	pconf = lcd_drv->lcd_config;
	type = pconf->lcd_basic.lcd_type;
	switch (type) {
	case LCD_LVDS:
		lvdsconf = pconf->lcd_control.lvds_config;
		if (lcd_drv->chip_type == LCD_CHIP_TXHD) {
			if (cnt >= 2) {
				if ((para[0] > 7) || (para[1] > 3)) {
					LCDERR("%s: wrong value:\n", __func__);
					pr_info("vswing=0x%x, preem=0x%x\n",
						para[0], para[1]);
					return;
				}

				lvdsconf->phy_vswing = para[0];
				lvdsconf->phy_preem = para[1];

				data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL1);
				data32 &= ~((0x7 << 3) | (0x7 << 0) |
					(0x3 << 23));
				data32 |= ((para[0] << 3) | (para[0] << 0) |
					(para[1] << 23));
				lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL1, data32);
				data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL2);
				data32 &= ~((0x3 << 14) | (0x3 << 12) |
					(0x3 << 26) | (0x3 << 24));
				data32 |= ((para[1] << 14) | (para[1] << 12) |
					(para[1] << 26) | (para[1] << 24));
				lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL2, data32);
				data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL3);
				data32 &= ~((0x3 << 6) | (0x3 << 4) |
					(0x3 << 2) | (0x3 << 0) |
					(0x3 << 30));
				data32 = ((para[1] << 6) | (para[1] << 4) |
					(para[1] << 2) | (para[1] << 0) |
					(para[1] << 30));
				lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL3, data32);

				LCDPR("%s: vswing=0x%x, preemphasis=0x%x\n",
					__func__, para[0], para[1]);
			} else {
				LCDERR("%s: invalid parameters cnt: %d\n",
					__func__, cnt);
			}
		} else {
			if (cnt == 4) {
				if ((para[0] > 7) || (para[1] > 7) ||
					(para[2] > 3) || (para[3] > 7)) {
					LCDERR("%s: wrong value:\n", __func__);
					pr_info("vswing=0x%x, preem=0x%x\n",
						para[0], para[1]);
					pr_info("clk vswing=%x, preem=%x\n",
						para[2], para[3]);
					return;
				}

				lvdsconf->phy_vswing = para[0];
				lvdsconf->phy_preem = para[1];
				lvdsconf->phy_clk_vswing = para[2];
				lvdsconf->phy_clk_preem = para[3];

				data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL1);
				data32 &= ~((0x7 << 26) | (0x7 << 0));
				data32 |= ((para[0] << 26) | (para[1] << 0));
				lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL1, data32);
				data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL3);
				data32 &= ~((0x3 << 8) | (0x7 << 5));
				data32 |= ((para[2] << 8) | (para[3] << 5));
				lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL3, data32);

				LCDPR("%s:\n", __func__);
				pr_info("vswing=0x%x, preemphasis=0x%x\n",
					para[0], para[1]);
				pr_info("clk_vswing=0x%x, clk_preem=0x%x\n",
					para[2], para[3]);
			} else if (cnt == 2) {
				if ((para[0] > 7) || (para[1] > 7)) {
					LCDERR("%s: wrong value:\n", __func__);
					pr_info("vswing=0x%x, preem=0x%x\n",
						para[0], para[1]);
					return;
				}

				lvdsconf->phy_vswing = para[0];
				lvdsconf->phy_preem = para[1];

				data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL1);
				data32 &= ~((0x7 << 26) | (0x7 << 0));
				data32 |= ((para[0] << 26) | (para[1] << 0));
				lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL1, data32);

				LCDPR("%s: vswing=0x%x, preemphasis=0x%x\n",
					__func__, para[0], para[1]);
			} else {
				LCDERR("%s: invalid parameters cnt: %d\n",
					__func__, cnt);
			}
		}
		break;
	case LCD_MLVDS:
		if (cnt >= 2) {
			if ((para[0] > 7) || (para[1] > 3)) {
				LCDERR("%s: wrong value:\n", __func__);
				pr_info("vswing=0x%x, preemphasis=0x%x\n",
					para[0], para[1]);
				return;
			}

			pconf->lcd_control.mlvds_config->phy_vswing = para[0];
			pconf->lcd_control.mlvds_config->phy_preem = para[1];

			data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL1);
			data32 &= ~((0x7 << 3) | (0x7 << 0) | (0x3 << 23));
			data32 |= ((para[0] << 3) | (para[0] << 0) |
				(para[1] << 23));
			lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL1, data32);
			data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL2);
			data32 &= ~((0x3 << 14) | (0x3 << 12) |
				(0x3 << 26) | (0x3 << 24));
			data32 |= ((para[1] << 14) | (para[1] << 12) |
				(para[1] << 26) | (para[1] << 24));
			lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL2, data32);
			data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL3);
			data32 &= ~((0x3 << 6) | (0x3 << 4) |
				(0x3 << 2) | (0x3 << 0) | (0x3 << 30));
			data32 |= ((para[1] << 6) | (para[1] << 4) |
				(para[1] << 2) | (para[1] << 0) |
				(para[1] << 30));
			lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL3, data32);

			LCDPR("%s: vswing=0x%x, preemphasis=0x%x\n",
				__func__, para[0], para[1]);
		} else {
			LCDERR("%s: invalid parameters cnt: %d\n",
				__func__, cnt);
		}
		break;
	case LCD_VBYONE:
		if (cnt >= 2) {
			ext_pullup = (para[0] >> 4) & 1;
			vswing = para[0] & 0xf;
			preem = para[1];
			if ((vswing > 7) || (preem > 7)) {
				LCDERR("%s: wrong value:\n", __func__);
				pr_info("vswing=0x%x, preemphasis=0x%x\n",
					vswing, preem);
				return;
			}

			pconf->lcd_control.vbyone_config->phy_vswing = para[0];
			pconf->lcd_control.vbyone_config->phy_preem = para[1];

			data32 = lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL1);
			data32 &= ~((0x7 << 3) | (1 << 10));
			data32 |= ((vswing << 3) | (ext_pullup << 10));
			if (ext_pullup)
				data32 &= ~(1 << 15);
			lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL1, data32);
			data32 =  lcd_hiu_read(HHI_DIF_CSI_PHY_CNTL2);
			data32 &= ~(0x7 << 20);
			data32 |= (preem << 20);
			lcd_hiu_write(HHI_DIF_CSI_PHY_CNTL2, data32);

			LCDPR("%s: vswing=0x%x, preemphasis=0x%x\n",
				__func__, para[0], para[1]);
		} else {
			LCDERR("%s: invalid parameters cnt: %d\n",
				__func__, cnt);
		}
		break;
	default:
		LCDERR("%s: not support lcd_type: %s\n",
			__func__, lcd_type_type_to_str(type));
		break;
	}
}

static ssize_t lcd_phy_debug_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	struct lcd_config_s *pconf;
	unsigned int vswing = 0xff, preem = 0xff;
	unsigned int clk_vswing = 0xff, clk_preem = 0xff;
	ssize_t len = 0;

	pconf = lcd_drv->lcd_config;
	switch (pconf->lcd_basic.lcd_type) {
	case LCD_LVDS:
		vswing = pconf->lcd_control.lvds_config->phy_vswing;
		preem = pconf->lcd_control.lvds_config->phy_preem;
		clk_vswing = pconf->lcd_control.lvds_config->phy_clk_vswing;
		clk_preem = pconf->lcd_control.lvds_config->phy_clk_preem;
		len += sprintf(buf+len, "%s:\n", __func__);
		len += sprintf(buf+len, "vswing=0x%x, preemphasis=0x%x\n",
			vswing, preem);
		len += sprintf(buf+len,
			"clk_vswing=0x%x, clk_preemphasis=0x%x\n",
			clk_vswing, clk_preem);
		break;
	case LCD_MLVDS:
		vswing = pconf->lcd_control.mlvds_config->phy_vswing;
		preem = pconf->lcd_control.mlvds_config->phy_preem;
		len += sprintf(buf+len, "%s:\n", __func__);
		len += sprintf(buf+len, "vswing=0x%x, preemphasis=0x%x\n",
			vswing, preem);
		break;
	case LCD_VBYONE:
		vswing = pconf->lcd_control.vbyone_config->phy_vswing;
		preem = pconf->lcd_control.vbyone_config->phy_preem;
		len += sprintf(buf+len, "%s:\n", __func__);
		len += sprintf(buf+len, "vswing=0x%x, preemphasis=0x%x\n",
			vswing, preem);
		break;
	default:
		len = sprintf(buf, "%s: invalid lcd_type: %d\n",
			__func__, pconf->lcd_basic.lcd_type);
		break;
	}
	return len;
}

static ssize_t lcd_phy_debug_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	unsigned int para[4];

	ret = sscanf(buf, "%x %x %x %x",
		&para[0], &para[1], &para[2], &para[3]);

	if (ret == 4) {
		lcd_phy_config_update(para, 4);
	} else if (ret == 2) {
		lcd_phy_config_update(para, 2);
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static struct class_attribute lcd_interface_debug_class_attrs[] = {
	__ATTR(ttl,    S_IRUGO | S_IWUSR,
		lcd_ttl_debug_show, lcd_ttl_debug_store),
	__ATTR(lvds,   S_IRUGO | S_IWUSR,
		lcd_lvds_debug_show, lcd_lvds_debug_store),
	__ATTR(vbyone, S_IRUGO | S_IWUSR,
		lcd_vx1_debug_show, lcd_vx1_debug_store),
	__ATTR(mipi,   S_IRUGO | S_IWUSR,
		lcd_mipi_debug_show, lcd_mipi_debug_store),
	__ATTR(edp,    S_IRUGO | S_IWUSR,
		lcd_edp_debug_show, lcd_edp_debug_store),
	__ATTR(minilvds, S_IRUGO | S_IWUSR,
		lcd_mlvds_debug_show, lcd_mlvds_debug_store),
};

static struct class_attribute lcd_phy_debug_class_attrs[] = {
	__ATTR(phy,    S_IRUGO | S_IWUSR,
		lcd_phy_debug_show, lcd_phy_debug_store),
};

static int lcd_black_screen_notifier(struct notifier_block *nb,
		unsigned long event, void *data)
{
	if ((event & LCD_EVENT_BLACK_SCREEN) == 0)
		return NOTIFY_DONE;
	if (lcd_debug_print_flag)
		LCDPR("%s: 0x%lx\n", __func__, event);

	lcd_mute_setting(1);

	return NOTIFY_OK;
}

static struct notifier_block lcd_black_screen_nb = {
	.notifier_call = lcd_black_screen_notifier,
	.priority = LCD_PRIORITY_BLACK_SCREEN,
};

int lcd_class_creat(void)
{
	int i;
	int ret;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	int type;

	ret = aml_lcd_notifier_register(&lcd_black_screen_nb);
	if (ret)
		LCDERR("register lcd_black_screen_notifier failed\n");

	lcd_drv->lcd_test_pattern_restore = lcd_test_check;

	lcd_drv->lcd_debug_class = class_create(THIS_MODULE, "lcd");
	if (IS_ERR(lcd_drv->lcd_debug_class)) {
		LCDERR("create lcd debug class fail\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(lcd_debug_class_attrs); i++) {
		if (class_create_file(lcd_drv->lcd_debug_class,
			&lcd_debug_class_attrs[i])) {
			LCDERR("create lcd debug attribute %s fail\n",
				lcd_debug_class_attrs[i].attr.name);
		}
	}

	type = lcd_drv->lcd_config->lcd_basic.lcd_type;
	for (i = 0; i < ARRAY_SIZE(lcd_interface_debug_class_attrs); i++) {
		if (strcmp(lcd_interface_debug_class_attrs[i].attr.name,
			lcd_type_type_to_str(type)))
			continue;
		if (class_create_file(lcd_drv->lcd_debug_class,
			&lcd_interface_debug_class_attrs[i])) {
			LCDERR("create lcd_interface debug attribute %s fail\n",
				lcd_interface_debug_class_attrs[i].attr.name);
		}
	}

	switch (type) {
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MLVDS:
		for (i = 0; i < ARRAY_SIZE(lcd_phy_debug_class_attrs); i++) {
			if (class_create_file(lcd_drv->lcd_debug_class,
				&lcd_phy_debug_class_attrs[i])) {
				LCDERR("create phy debug attribute %s fail\n",
					lcd_phy_debug_class_attrs[i].attr.name);
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int lcd_class_remove(void)
{
	int i;
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
	int type;

	for (i = 0; i < ARRAY_SIZE(lcd_debug_class_attrs); i++) {
		class_remove_file(lcd_drv->lcd_debug_class,
			&lcd_debug_class_attrs[i]);
	}

	type = lcd_drv->lcd_config->lcd_basic.lcd_type;
	for (i = 0; i < ARRAY_SIZE(lcd_interface_debug_class_attrs); i++) {
		if (strcmp(lcd_interface_debug_class_attrs[i].attr.name,
			lcd_type_type_to_str(type)))
			continue;
		class_remove_file(lcd_drv->lcd_debug_class,
			&lcd_interface_debug_class_attrs[i]);
	}

	switch (type) {
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MLVDS:
		for (i = 0; i < ARRAY_SIZE(lcd_phy_debug_class_attrs); i++) {
			class_remove_file(lcd_drv->lcd_debug_class,
				&lcd_phy_debug_class_attrs[i]);
		}
		break;
	default:
		break;
	}

	class_destroy(lcd_drv->lcd_debug_class);
	lcd_drv->lcd_debug_class = NULL;

	return 0;
}

