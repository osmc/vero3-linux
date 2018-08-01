/*
 * drivers/amlogic/media/enhancement/amvecm/reg_helper.h
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

#ifndef __REG_HELPER_H
#define __REG_HELPER_H

#include "arch/vpp_regs.h"
#include "arch/ve_regs.h"
#include "arch/cm_regs.h"

/* useful inline fucntions to handle different offset */
static inline bool cpu_after_eq_tm2b(void)
{
	return (cpu_after_eq(MESON_CPU_MAJOR_ID_TM2)) &&
		(!(is_meson_rev_a() && is_meson_tm2_cpu()));
}

static inline bool cpu_after_eq_tl1(void)
{
	return cpu_after_eq(MESON_CPU_MAJOR_ID_TL1);
}

static inline bool is_sr0_reg(u32 addr)
{
	return (addr >= SRSHARP0_SHARP_HVSIZE &&
		addr <= SRSHARP0_PKOSHT_VSLUMA_LUT_H);
}

static inline bool is_sr1_reg(u32 addr)
{
	return (addr >= SRSHARP1_SHARP_HVSIZE &&
		addr <= SRSHARP1_PKOSHT_VSLUMA_LUT_H);
}

static inline bool is_lc_reg(u32 addr)
{
	return (addr >= SRSHARP1_LC_INPUT_MUX &&
		addr <= SRSHARP1_LC_MAP_RAM_DATA);
}

static inline bool is_sr0_dnlpv2_reg(u32 addr)
{
	return (addr >= SRSHARP0_DNLP2_00 &&
		addr <= SRSHARP0_DNLP2_31);
}

static inline bool is_sr1_dnlpv2_reg(u32 addr)
{
	return (addr >= SRSHARP1_DNLP2_00 &&
		addr <= SRSHARP1_DNLP2_31);
}

static inline u32 get_sr0_offset(void)
{
	/*sr0  register shfit*/
	if (cpu_after_eq_tm2b())
		return 0x1200;
	else if (cpu_after_eq_tl1())
		return 0x0;
	else if (is_meson_g12a_cpu() ||
		 is_meson_g12b_cpu() ||
		is_meson_sm1_cpu())
		return 0x0;

	return 0xfffff400 /*-0xc00*/;
}

static inline u32 get_sr1_offset(void)
{
	/*sr1 register shfit*/
	if (cpu_after_eq_tm2b())
		return 0x1300;
	else if (cpu_after_eq_tl1())
		return 0x0;
	else if (is_meson_g12a_cpu() ||
		 is_meson_g12b_cpu() ||
		is_meson_sm1_cpu())
		return 0x0;

	return 0xfffff380; /*-0xc80*/;
}

static inline u32 get_lc_offset(void)
{
	/* lc register shfit*/
	if (cpu_after_eq_tm2b())
		return 0x1300;

	return 0;
}

static inline u32 get_sr0_dnlp2_offset(void)
{
	/* SHARP0_DNLP_00 shfit*/
	if (cpu_after_eq_tm2b())
		return 0x1200;

	return 0;
}

static inline u32 get_sr1_dnlp2_offset(void)
{
	/* SHARP0_DNLP_00 shfit*/
	if (cpu_after_eq_tm2b())
		return 0x1300;

	return 0;
}

static u32 offset_addr(u32 addr)
{
	if (is_sr0_reg(addr))
		return addr + get_sr0_offset();
	else if (is_sr1_reg(addr))
		return addr + get_sr1_offset();
	else if (is_sr0_dnlpv2_reg(addr))
		return addr + get_sr0_dnlp2_offset();
	else if (is_sr1_dnlpv2_reg(addr))
		return addr + get_sr1_dnlp2_offset();
	else if (is_lc_reg(addr))
		return addr + get_lc_offset();

	return addr;
}

static inline void WRITE_VPP_REG(u32 reg,
				 const u32 value)
{
	aml_write_vcbus_s(offset_addr(reg), value);
}

static inline u32 READ_VPP_REG(u32 reg)
{
	return aml_read_vcbus_s(offset_addr(reg));
}

static inline void WRITE_VPP_REG_EX(u32 reg,
				    const u32 value,
				    bool add_offset)
{
	if (add_offset)
		reg = offset_addr(reg);

	aml_write_vcbus_s(reg, value);
}

static inline u32 READ_VPP_REG_EX(u32 reg,
				  bool add_offset)
{
	if (add_offset)
		reg = offset_addr(reg);

	return aml_read_vcbus_s(reg);
}

static inline void WRITE_VPP_REG_BITS(u32 reg,
				      const u32 value,
		const u32 start,
		const u32 len)
{
	aml_vcbus_update_bits_s(offset_addr(reg), value, start, len);
}

static inline u32 READ_VPP_REG_BITS(u32 reg,
				    const u32 start,
				    const u32 len)
{
	u32 val;
	u32 reg1 = offset_addr(reg);

	val = ((aml_read_vcbus_s(reg1) >> (start)) & ((1L << (len)) - 1));

	return val;
}

#ifndef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
#define VSYNC_WR_MPEG_REG(adr, val) WRITE_VPP_REG(adr, val)
#define VSYNC_RD_MPEG_REG(adr) READ_VPP_REG(adr)
#define VSYNC_WR_MPEG_REG_BITS(adr, val, start, len) \
	WRITE_VPP_REG_BITS(adr, val, start, len)
#else
int VSYNC_WR_MPEG_REG_BITS(u32 adr, u32 val, u32 start, u32 len);
u32 VSYNC_RD_MPEG_REG(u32 adr);
int VSYNC_WR_MPEG_REG(u32 adr, u32 val);
#endif

static inline void VSYNC_WRITE_VPP_REG(u32 reg,
				       const u32 value)
{
	VSYNC_WR_MPEG_REG(offset_addr(reg), value);
}

static inline u32 VSYNC_READ_VPP_REG(u32 reg)
{
	return VSYNC_RD_MPEG_REG(offset_addr(reg));
}

static inline void VSYNC_WRITE_VPP_REG_EX(u32 reg,
					  const u32 value,
					  bool add_offset)
{
	if (add_offset)
		reg = offset_addr(reg);
	VSYNC_WR_MPEG_REG(reg, value);
}

static inline u32 VSYNC_READ_VPP_REG_EX(u32 reg,
					bool add_offset)
{
	if (add_offset)
		reg = offset_addr(reg);
	return VSYNC_RD_MPEG_REG(reg);
}

static inline void VSYNC_WRITE_VPP_REG_BITS(u32 reg,
					    const u32 value,
		const u32 start,
		const u32 len)
{
	VSYNC_WR_MPEG_REG_BITS(offset_addr(reg), value, start, len);
}

#endif
