/*
 * drivers/amlogic/pinctrl/pinctrl-mesontxhd.c
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

#include <dt-bindings/gpio/txhd.h>
#include <linux/arm-smccc.h>
#include "pinctrl-meson.h"

#define EE_OFF	15

static const struct meson_desc_pin mesontxhd_periphs_pins[] = {
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_0, EE_OFF), 0x2, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "i2c_b"),		/*I2C_SDA_B*/
		MESON_FUNCTION(0x2, "i2s_din"),		/*I2S_DIN01*/
		MESON_FUNCTION(0x3, "pwm_vs"),		/*PWM_VS*/
		MESON_FUNCTION(0x4, "pwm_c"),		/*PWM_C*/
		MESON_FUNCTION(0x5, "pwm_cabc")),	/*PWM_CABC*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_1, EE_OFF), 0x2, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "i2c_b"),		/*I2C_SCK_B*/
		MESON_FUNCTION(0x2, "i2s_dout"),	/*I2S_DOUT23*/
		MESON_FUNCTION(0x3, "pwm_vs"),		/*PWM_VS*/
		MESON_FUNCTION(0x4, "pwm_b"),		/*PWM_B*/
		MESON_FUNCTION(0x5, "pwm_cabc")),	/*PWM_CABC*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_2, EE_OFF), 0x2, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x2, "i2s_dout"),	/*I2S_DOUT01*/
		MESON_FUNCTION(0x3, "dtv"),			/*DTV_RF_AGC*/
		MESON_FUNCTION(0x4, "pwm_d")),		/*PWM_D*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_3, EE_OFF), 0x2, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_LRCLK*/
		MESON_FUNCTION(0x5, "pwm_f")),		/*PWM_F*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_4, EE_OFF), 0x2, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "spdif_in"),	/*SPDIF_IN*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_SCLK*/
		MESON_FUNCTION(0x3, "uart_b"),		/*UART_RTS_B*/
		MESON_FUNCTION(0x4, "spdif_out"),	/*SPDIF_OUT*/
		MESON_FUNCTION(0x5, "pwm_e")),		/*PWM_E*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_5, EE_OFF), 0x2, 20,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "pwm_a"),		/*PWM_A*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_MCLK*/
		MESON_FUNCTION(0x3, "uart_b"),		/*UART_CTS_B*/
		MESON_FUNCTION(0x4, "atv"),			/*ATV_IF_AGC*/
		MESON_FUNCTION(0x5, "dtv")),		/*DTV_IF_AGC*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_6, EE_OFF), 0x2, 24,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "eth"),			/*ETH_LINK_LED*/
		MESON_FUNCTION(0x2, "i2c_a"),		/*I2C_SDA_A*/
		MESON_FUNCTION(0x3, "uart_b"),		/*UART_RX_B*/
		MESON_FUNCTION(0x5, "uart_a")),		/*UART_TX_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOZ_7, EE_OFF), 0x2, 28,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "eth"),			/*ETH_ACT_LED*/
		MESON_FUNCTION(0x2, "i2c_a"),		/*I2C_SCK_A*/
		MESON_FUNCTION(0x3, "uart_b"),		/*UART_TX_B*/
		MESON_FUNCTION(0x4, "pwm_a"),		/*PWM_A*/
		MESON_FUNCTION(0x5, "uart_a")),		/*UART_RX_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_0, EE_OFF), 0x4, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_0*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_MCLK*/
		MESON_FUNCTION(0x3, "tsin_a"),		/*TSin_CLK_A*/
		MESON_FUNCTION(0x4, "acodec"),		/*ACODEC_I2S_MCLK*/
		MESON_FUNCTION(0x5, "i2c_d"),		/*I2C_SCK_D*/
		MESON_FUNCTION(0x6, "audin")),		/*AUDIN_MCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_1, EE_OFF), 0x4, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_1*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_SCLK*/
		MESON_FUNCTION(0x3, "tsin_a"),		/*TSin_D0_A*/
		MESON_FUNCTION(0x5, "i2c_d"),		/*I2C_SDA_D*/
		MESON_FUNCTION(0x6, "audin")),		/*AUDIN_SCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_2, EE_OFF), 0x4, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_2*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_LRCLK*/
		MESON_FUNCTION(0x3, "tsin_a"),		/*TSin_VALID_A*/
		MESON_FUNCTION(0x5, "acodec"),		/*ACODEC_I2S_LRCLKIN*/
		MESON_FUNCTION(0x6, "audin")),		/*AUDIN_LRCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_3, EE_OFF), 0x4, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_3*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_DOUT01*/
		MESON_FUNCTION(0x3, "tsin_a"),		/*TSin_SOP_A*/
		MESON_FUNCTION(0x5, "acodec")),		/*ACODEC_I2S_SCLKIN*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_4, EE_OFF), 0x4, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_4*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_DOUT23*/
		MESON_FUNCTION(0x4, "i2c_a"),		/*I2C_SCK_A*/
		MESON_FUNCTION(0x5, "pcm_a")),		/*PCM_FS_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_5, EE_OFF), 0x4, 20,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_5*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_DOUT45*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D7_B*/
		MESON_FUNCTION(0x4, "i2c_a"),		/*I2C_SDA_A*/
		MESON_FUNCTION(0x5, "pcm_a")),		/*PCM_OUT_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_6, EE_OFF), 0x4, 24,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_6*/
		MESON_FUNCTION(0x2, "i2s"),			/*I2S_DOUT67*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D6_B*/
		MESON_FUNCTION(0x4, "acodec"),		/*ACODEC_I2S_DIN23*/
		MESON_FUNCTION(0x5, "pcm_a")),		/*PCM_IN_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_7, EE_OFF), 0x4, 28,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_7*/
		MESON_FUNCTION(0x2, "pwm_a"),		/*PWM_A*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D5_B*/
		MESON_FUNCTION(0x4, "acodec"),		/*ACODEC_I2S_DIN01*/
		MESON_FUNCTION(0x5, "pcm_a"),		/*PCM_CLK_A*/
		MESON_FUNCTION(0x6, "audin")),		/*AUDIN_MCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_8, EE_OFF), 0x5, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_8*/
		MESON_FUNCTION(0x2, "i2c_c"),		/*I2C_SCK_C*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D4_B*/
		MESON_FUNCTION(0x5, "pwm_e"),		/*PWM_E*/
		MESON_FUNCTION(0x6, "audin")),		/*AUDIN_SCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_9, EE_OFF), 0x5, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_9*/
		MESON_FUNCTION(0x2, "i2c_c"),		/*I2C_SDA_C*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D3_B*/
		MESON_FUNCTION(0x5, "pwm_d"),		/*PWM_D*/
		MESON_FUNCTION(0x6, "audin")),		/*AUDIN_LRCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_10, EE_OFF), 0x5, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_10*/
		MESON_FUNCTION(0x2, "i2c_d"),		/*I2C_SCK_D*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D2_B*/
		MESON_FUNCTION(0x4, "pwm_vs"),		/*PWM_VS*/
		MESON_FUNCTION(0x5, "pwm_cabc")),	/*PWM_CABC*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_11, EE_OFF), 0x5, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_11*/
		MESON_FUNCTION(0x2, "i2c_d"),		/*I2C_SDA_D*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D1_B*/
		MESON_FUNCTION(0x4, "pwm_c"),		/*PWM_C*/
		MESON_FUNCTION(0x5, "pwm_cabc")),	/*PWM_CABC*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_12, EE_OFF), 0x5, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_12*/
		MESON_FUNCTION(0x2, "uart_c"),		/*UART_RTS_C*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_D0_B*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_DIN01*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_13, EE_OFF), 0x5, 20,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_13*/
		MESON_FUNCTION(0x2, "uart_c"),		/*UART_CTS_C*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_CLK_B*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_DIN23*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_14, EE_OFF), 0x5, 24,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_14*/
		MESON_FUNCTION(0x2, "uart_c"),		/*UART_RX_C*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_SOP_B*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_DIN45*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOH_15, EE_OFF), 0x5, 28,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tcon"),		/*TCON_15*/
		MESON_FUNCTION(0x2, "uart_c"),		/*UART_TX_C*/
		MESON_FUNCTION(0x3, "tsin_b"),		/*TSin_VALID_B*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_DIN67*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_0, EE_OFF), 0x0, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_D0*/
		MESON_FUNCTION(0x1, "nandflash")),  /*NAND_D0*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_1, EE_OFF), 0x0, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_D1*/
		MESON_FUNCTION(0x1, "nandflash")),	/*NAND_D1*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_2, EE_OFF), 0x0, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_D2*/
		MESON_FUNCTION(0x1, "nandflash")),	/*NAND_D2*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_3, EE_OFF), 0x0, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_D3*/
		MESON_FUNCTION(0x1, "nandflash"),   /*NAND_D3*/
		MESON_FUNCTION(0x3, "norflash")),	/*NOR_HOLD_D3*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_4, EE_OFF), 0x0, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_D4*/
		MESON_FUNCTION(0x1, "nandflash"),   /*NAND_D4*/
		MESON_FUNCTION(0x3, "norflash")),	/*NOR_DT_D0*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_5, EE_OFF), 0x0, 20,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_D5*/
		MESON_FUNCTION(0x1, "nandflash"),   /*NAND_D5*/
		MESON_FUNCTION(0x3, "norflash")),	/*NOR_DR_D1*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_6, EE_OFF), 0x0, 24,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_D6*/
		MESON_FUNCTION(0x1, "nandflash"),   /*NAND_D6*/
		MESON_FUNCTION(0x3, "norflash")),	/*NOR_CK*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_7, EE_OFF), 0x0, 28,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),       /*EMMC_D7*/
		MESON_FUNCTION(0x1, "nandflash")), /*NAND_D7*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_8, EE_OFF), 0x1, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_CLK*/
		MESON_FUNCTION(0x2, "nandflash")),	/*NAND_CE0 */
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_9, EE_OFF), 0x1, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x2, "nandflash"),	/*NAND_ALE*/
		MESON_FUNCTION(0x3, "norflash")),	/*NOR_WP_D2*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_10, EE_OFF), 0x1, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_CMD*/
		MESON_FUNCTION(0x2, "nandflash")),	/*NAND_REN_WR */
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_11, EE_OFF), 0x1, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "emmc"),		/*EMMC_DS*/
		MESON_FUNCTION(0x2, "nandflash")),	/*NAND_CLE*/
	MESON_PINCTRL_PIN(MESON_PIN(BOOT_12, EE_OFF), 0x1, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x2, "nandflash"),	/*NAND_CE0*/
		MESON_FUNCTION(0x3, "norflash")),	/*NOR_CS*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_0, EE_OFF), 0x9, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "sdio"),		/*SDCARD_D0*/
		MESON_FUNCTION(0x2, "jtag"),		/*JTAG_TDO*/
		MESON_FUNCTION(0x3, "hdmi"),		/*HDMIrx_phy_dtb0*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_SCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_1, EE_OFF), 0x9, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "sdio"),		/*SDCARD_D1*/
		MESON_FUNCTION(0x2, "jtag"),		/*JTAG_TDI*/
		MESON_FUNCTION(0x3, "hdmi"),		/*HDMIrx_phy_dtb1*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_MCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_2, EE_OFF), 0x9, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "sdio"),		/*SDCARD_D2*/
		MESON_FUNCTION(0x2, "uart_rx_ao_a"),/*UART_RX_AO_A*/
		MESON_FUNCTION(0x3, "uart_tx_ao_a"),/*UART_TX_AO_A*/
		MESON_FUNCTION(0x4, "i2s"),			/*I2S_DOUT23*/
		MESON_FUNCTION(0x5, "i2c_c")),		/*I2C_SDA_C*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_3, EE_OFF), 0x9, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "sdio"),		/*SDCARD_D3*/
		MESON_FUNCTION(0x2, "uart_tx_ao_a"),/*UART_TX_AO_A*/
		MESON_FUNCTION(0x3, "uart_rx_ao_a"),/*UART_RX_AO_A*/
		MESON_FUNCTION(0x4, "i2s"),			/*I2S_DOUT67*/
		MESON_FUNCTION(0x5, "i2c_c")),		/*I2C_SCK_C*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_4, EE_OFF), 0x9, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "sdio"),		/*SDCARD_CLK */
		MESON_FUNCTION(0x2, "jtag"),		/*JTAG_CLK*/
		MESON_FUNCTION(0x3, "hdmi"),		/*HDMIrx_phy_dtb2*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_DOUT01*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_5, EE_OFF), 0x9, 20,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "sdio"),		/*SDCARD_CMD */
		MESON_FUNCTION(0x2, "jtag"),		/*JTAG_TMS*/
		MESON_FUNCTION(0x3, "hdmi"),		/*HDMIrx_phy_dtb3*/
		MESON_FUNCTION(0x4, "i2s")),		/*I2S_LRCLK*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_6, EE_OFF), 0x9, 24,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x4, "spi_a"),		/*SPI_SS1_A*/
		MESON_FUNCTION(0x5, "pwm_d")),		/*PWM_D*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_7, EE_OFF), 0x9, 28,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_SOP_A */
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_RX_A*/
		MESON_FUNCTION(0x3, "pcm_a"),		/*PCM_FS_A*/
		MESON_FUNCTION(0x4, "spi_a")),		/*SPI_SS0_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_8, EE_OFF), 0xa, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_VALID_A */
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_TX_A*/
		MESON_FUNCTION(0x3, "pcm_a"),		/*PCM_OUT_A*/
		MESON_FUNCTION(0x4, "spi_a")),		/*SPI_MOSI_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_9, EE_OFF), 0xa, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_D0_A */
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_CTS_A*/
		MESON_FUNCTION(0x3, "pcm_a"),		/*PCM_IN_A*/
		MESON_FUNCTION(0x4, "spi_a"),		/*SPI_MISO_A*/
		MESON_FUNCTION(0x5, "pwm_b")),		/*PWM_B*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOC_10, EE_OFF), 0xa, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_CLK_A */
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_RTS_A*/
		MESON_FUNCTION(0x3, "pcm_a"),		/*PCM_CLK_A*/
		MESON_FUNCTION(0x4, "spi_a"),		/*SPI_CLK_A*/
		MESON_FUNCTION(0x5, "pwm_c")),		/*PWM_C*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_0, EE_OFF), 0x7, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "i2c_b"),		/*I2C_SDA_B*/
		MESON_FUNCTION(0x2, "i2c_slave_tcon")),/*I2C_SLAVE_SDA_TCON*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_1, EE_OFF), 0x7, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "i2c_b"),		/*I2C_SCK_B*/
		MESON_FUNCTION(0x2, "i2c_slave_tcon")),/*I2C_SLAVE_SCK_TCON*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_2, EE_OFF), 0x7, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "dtv"),			/*DTV_IF_AGC*/
		MESON_FUNCTION(0x2, "atv"),			/*ATV_IF_AGC*/
		MESON_FUNCTION(0x3, "tcon"),		/*TCON_AGING*/
		MESON_FUNCTION(0x4, "pwm_d")),		/*PWM_D*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_3, EE_OFF), 0x7, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x2, "dtv"),			/*DTV_RF_AGC*/
		MESON_FUNCTION(0x3, "spi_a"),		/*SPI_SS3_A*/
		MESON_FUNCTION(0x4, "spdif_out")),	/*SPDIF_OUT*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_4, EE_OFF), 0x7, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "i2c_a"),		/*I2C_SDA_A*/
		MESON_FUNCTION(0x2, "spdif_in"),	/*SPDIF_IN*/
		MESON_FUNCTION(0x3, "spi_a"),		/*SPI_SS2_A*/
		MESON_FUNCTION(0x4, "pwm_a")),		/*PWM_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_5, EE_OFF), 0x7, 20,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "i2c_a"),		/*I2C_SCK_A*/
		MESON_FUNCTION(0x3, "spi_a"),		/*SPI_SS1_A*/
		MESON_FUNCTION(0x4, "pwm_vs")),		/*PWM_VS*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_6, EE_OFF), 0x7, 24,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_D0_A*/
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_RTS_A*/
		MESON_FUNCTION(0x3, "spi_a")),		/*SPI_SS0_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_7, EE_OFF), 0x7, 28,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_CLK_A*/
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_CTS_A*/
		MESON_FUNCTION(0x3, "spi_a")),		/*SPI_MISO_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_8, EE_OFF), 0x8, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_SOP_A*/
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_TX_A*/
		MESON_FUNCTION(0x3, "spi_a")),		/*SPI_MOSI_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIODV_9, EE_OFF), 0x8, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "tsin_a"),		/*TSin_VALID_A*/
		MESON_FUNCTION(0x2, "uart_a"),		/*UART_RX_A*/
		MESON_FUNCTION(0x3, "spi_a")),		/*SPI_CLK_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_0, EE_OFF), 0xb, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_a")),		/*HDMIRX_HPD_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_1, EE_OFF), 0xb, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_a")),		/*HDMIRX_5VDET_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_2, EE_OFF), 0xb, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_a"),		/*HDMIRX_SDA_A*/
		MESON_FUNCTION(0x2, "uart_ao_a")),	/*UART_TX_AO_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_3, EE_OFF), 0xb, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_a"),		/*HDMIRX_SCL_A*/
		MESON_FUNCTION(0x2, "uart_ao_a")),	/*UART_RX_AO_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_4, EE_OFF), 0xb, 16,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_c")),		/*HDMIRX_HPD_C*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_5, EE_OFF), 0xb, 20,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_c")),		/*HDMIRX_5VDET_C*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_6, EE_OFF), 0xb, 24,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_c"),		/*HDMIRX_SDA_C*/
		MESON_FUNCTION(0x2, "uart_ao_a")),	/*UART_TX_AO_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_7, EE_OFF), 0xb, 28,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_c"),		/*HDMIRX_SCL_C*/
		MESON_FUNCTION(0x2, "uart_ao_a")),	/*UART_RX_AO_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_8, EE_OFF), 0xc, 0,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_b")),		/*HDMIRX_HPD_B*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_9, EE_OFF), 0xc, 4,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_b")),		/*HDMIRX_5VDET_B*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_10, EE_OFF), 0xc, 8,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_b"),		/*HDMIRX_SDA_B*/
		MESON_FUNCTION(0x2, "uart_ao_a")),	/*UART_TX_AO_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOW_11, EE_OFF), 0xc, 12,
		MESON_FUNCTION(0x0, "gpio"),
		MESON_FUNCTION(0x1, "hdmi_b"),		/*HDMIRX_SCL_B*/
		MESON_FUNCTION(0x2, "uart_ao_a")),	/*UART_RX_AO_A*/

};

static const struct meson_desc_pin mesontxhd_aobus_pins[] = {
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_0, 0), 0x0, 0,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "uart_ao_a"),	/*UART_TX_AO_A*/
		MESON_FUNCTION(0x1, "uart_ao_b")),	/*UART_TX_AO_B*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_1, 0), 0x0, 4,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "uart_ao_a"),	/*UART_RX_AO_A*/
		MESON_FUNCTION(0x1, "uart_ao_b")),	/*UART_RX_AO_B*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_2, 0), 0x0, 8,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "uart_ao_a"),	/*UART_CTS_AO_A*/
		MESON_FUNCTION(0x2, "uart_ao_b"),	/*UART_CTS_AO_B*/
		MESON_FUNCTION(0x4, "pwm_ao_d")),	/* PWMAO_D*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_3, 0), 0x0, 12,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "uart_ao_a"),	/*UART_RTS_AO_A*/
		MESON_FUNCTION(0x2, "uart_ao_b"),	/*UART_RTS_AO_B*/
		MESON_FUNCTION(0x3, "jtag"),		/* JTAG_TDI*/
		MESON_FUNCTION(0x4, "pwm_ao_a"),	/* PWMAO_A*/
		MESON_FUNCTION(0x5, "pwm_ao_az")),	/* PWMAO_AZ*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_4, 0), 0x0, 16,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "i2c_ao"),		/*I2C_SCK_AO*/
		MESON_FUNCTION(0x2, "uart_ao_b"),	/*UART_TX_AO_B*/
		MESON_FUNCTION(0x3, "i2c_slave_ao"),/*I2C_SLAVE_SCK_AO*/
		MESON_FUNCTION(0x4, "jtag")),		/*JTAG_TDO */
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_5, 0), 0x0, 20,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "i2c_ao"),		/*I2C_SDA_AO*/
		MESON_FUNCTION(0x2, "uart_ao_b"),	/*UART_RX_AO_B*/
		MESON_FUNCTION(0x3, "i2c_slave_ao"),/*I2C_SLAVE_SDA_AO*/
		MESON_FUNCTION(0x4, "jtag")),		/*JTAG_CLK */
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_6, 0), 0x0, 24,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "ir_in"),		/*IR_REMOTE_INPUT*/
		MESON_FUNCTION(0x2, "ir_out")),		/*IR_REMOTE_OUTPUT*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_7, 0), 0x0, 28,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "cec_ao"),      /*AO_CEC*/
		MESON_FUNCTION(0x2, "cec_ao_b"),    /*AO_CEC_B*/
		MESON_FUNCTION(0x3, "jtag"),		/*JTAG_TMS*/
		MESON_FUNCTION(0x4, "pwm_ao_a")),	/*PWMAO_A*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_8, 0), 0x1, 0,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "cec_ao"),      /*AO_CEC*/
		MESON_FUNCTION(0x2, "cec_ao_b"),    /*AO_CEC_B*/
		MESON_FUNCTION(0x3, "clk32"),		/*CLK_32K_IN*/
		MESON_FUNCTION(0x4, "pwm_ao_c"),	/*PWMAO_C*/
		MESON_FUNCTION(0x5, "pwm_ao_cz")),	/*PWMAO_CZ*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_9, 0), 0x1, 4,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x4, "pwm_ao_b")),	/*PWMAO_B*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_10, 0), 0x1, 8,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "i2c_ao"),		/*I2C_SCK_AO */
		MESON_FUNCTION(0x2, "uart_ao_b"),   /*UART_TX_AO_B*/
		MESON_FUNCTION(0x3, "i2c_slave_ao"),/*I2C_SLAVE_SCK_AO */
		MESON_FUNCTION(0x4, "ir_in")),		/*REMOTE_INPUT */
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_11, 0), 0x1, 12,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "i2c_ao"),		/*I2C_SDA_AO */
		MESON_FUNCTION(0x2, "uart_ao_b"),   /*UART_RX_AO_B*/
		MESON_FUNCTION(0x3, "i2c_slave_ao"),/*I2C_SLAVE_SDA_AO*/
		MESON_FUNCTION(0x4, "i2c_ao"),		/*REMOTE_OUT */
		MESON_FUNCTION(0x5, "gen_clk_ao"),  /*GEN_CLK_AO*/
		MESON_FUNCTION(0x6, "gen_clk")),	/*GEN_CLK_EE*/
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_12, 0), 0x1, 16,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "clk12"),		/*CLK12_24*/
		MESON_FUNCTION(0x4, "pwm_ao_c")),	/*PWMAO_C */
	MESON_PINCTRL_PIN(MESON_PIN(GPIOAO_13, 0), 0x1, 20,
		MESON_FUNCTION(0x0, "gpio_ao"),
		MESON_FUNCTION(0x1, "gen_clk"),		/*GEN_CLK_EE*/
		MESON_FUNCTION(0x4, "pwm_ao_d")),	/*PWMAO_D */

	MESON_PINCTRL_PIN(MESON_PIN(GPIO_TEST_N, 0), 0x1, 24,
		MESON_FUNCTION(0x0, "gpio_ao")),
};

static struct meson_bank mesontxhd_periphs_banks[] = {
	/* name   first   last   irq  pullen  pull   dir  out  in */
	BANK("Z", PIN(GPIOZ_0, EE_OFF), PIN(GPIOZ_7, EE_OFF), 14,
	3, 0,  3,  0,  9,  0,  10, 0,  11, 0),
	BANK("H", PIN(GPIOH_0, EE_OFF), PIN(GPIOH_15, EE_OFF), 22,
	1, 16,  1,  16,  3,  16,  4, 16,  5, 16),
	BANK("BOOT", PIN(BOOT_0, EE_OFF), PIN(BOOT_12, EE_OFF), 38,
	2, 0,  2,  0,  6,  0,  7, 0,  8, 0),
	BANK("C", PIN(GPIOC_0, EE_OFF), PIN(GPIOC_10, EE_OFF), 51,
	2, 16,  2,  16,  6,  16, 7, 16, 8, 16),
	BANK("DV", PIN(GPIODV_0, EE_OFF), PIN(GPIODV_9, EE_OFF), 62,
	0, 0,  0,  0,  0,  0, 1, 0, 2, 0),
	BANK("W", PIN(GPIOW_0, EE_OFF), PIN(GPIOW_11, EE_OFF), 72,
	1, 0,  1,  0,  3,  0, 4, 0, 5, 0),
};

/*  TEST_N is special pin, only used as gpio output at present.
 *  the direction control bit from AO_SEC_REG0 bit[0], it
 *  configured to output when pinctrl driver is initialized.
 *  to make the api of gpiolib work well, the reserved bit(bit[14])
 *  seen as direction control bit.
 *
 * AO_GPIO_O_EN_N       0x09<<2=0x24     bit[31]     output level
 * AO_GPIO_I            0x0a<<2=0x28     bit[31]     input level
 * AO_SEC_REG0          0x50<<2=0x140    bit[0]      input enable
 * AO_RTI_PULL_UP_REG   0x0b<<2=0x2c     bit[14]     pull-up/down
 * AO_RTI_PULL_UP_REG   0x0b<<2=0x2c     bit[30]     pull-up enable
 */
static struct meson_bank mesontxhd_aobus_banks[] = {
	/*   name  first  last  irq  pullen  pull    dir     out     in  */
	BANK("AO", PIN(GPIOAO_0, 0), PIN(GPIOAO_13, 0), 0,
	0,  16,  0,  0,  0,  0,  0, 16,  1,  0),
	BANK("TEST", PIN(GPIO_TEST_N, 0), PIN(GPIO_TEST_N, 0), -1,
	0, 30, 0, 14, 0, 14, 0, 31, 1, 31),
};

static struct meson_domain_data mesontxhd_periphs_domain_data = {
	.type		= EE_DOMAIN,
	.name		= "periphs-banks",
	.banks		= mesontxhd_periphs_banks,
	.num_banks	= ARRAY_SIZE(mesontxhd_periphs_banks),
	.pin_base	= 15,
	.num_pins	= 70,
};

static struct meson_domain_data mesontxhd_aobus_domain_data = {
	.type		= AO_DOMAIN,
	.name		= "aobus-banks",
	.banks		= mesontxhd_aobus_banks,
	.num_banks	= ARRAY_SIZE(mesontxhd_aobus_banks),
	.pin_base	= 0,
	.num_pins	= 15,
};

struct meson_pinctrl_data  meson_txhd_periphs_pinctrl_data = {
	.meson_pins  = mesontxhd_periphs_pins,
	.domain_data = &mesontxhd_periphs_domain_data,
	.num_pins = ARRAY_SIZE(mesontxhd_periphs_pins),
};

struct meson_pinctrl_data  meson_txhd_aobus_pinctrl_data = {
	.meson_pins  = mesontxhd_aobus_pins,
	.domain_data = &mesontxhd_aobus_domain_data,
	.num_pins = ARRAY_SIZE(mesontxhd_aobus_pins),
};

int meson_txhd_aobus_init(struct meson_pinctrl *pc)
{
	struct arm_smccc_res res;
	/*set TEST_N to output*/
	arm_smccc_smc(CMD_TEST_N_DIR, TEST_N_OUTPUT, 0, 0, 0, 0, 0, 0, &res);

	return 0;
}
