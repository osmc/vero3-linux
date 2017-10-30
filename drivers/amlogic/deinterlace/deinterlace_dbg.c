/* * amlogic m2
 * frame buffer driver-----------deinterlace
 * author: rain zhang <rain.zhang@amlogic.com>
 * copyright (c) 2010 amlogic, inc.
 *
 * this program is free software; you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation; either version 2 of the named license,
 * or any later version.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the
 * gnu general public license for more details.
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/amports/vframe.h>
#include <linux/amlogic/canvas/canvas.h>
#include <linux/amlogic/canvas/canvas_mgr.h>
#include <linux/amlogic/vout/vout_notify.h>
#include "deinterlace_dbg.h"

void dump_di_reg(void)
{
	unsigned int i = 0, base_addr = 0;
	unsigned int size_reg_addr[57] = {
		0x1702, 0x1703, 0x2d01,
		0x2d01, 0x2d8f, 0x2d08,
		0x2d09, 0x2f00, 0x2f01,
		0x17d0, 0x17d1, 0x17d2,
		0x17d3, 0x17dd, 0x17de,
		0x17df, 0x17e0, 0x17f7,
		0x17f8, 0x17f9, 0x17fa,
		0x17c0, 0x17c1,	0x17a0,
		0x17a1, 0x17c3, 0x17c4,
		0x17cb, 0x17cc, 0x17a3,
		0x17a4, 0x17a5, 0x17a6,
		0x2f92, 0x2f93, 0x2f95,
		0x2f96, 0x2f98,	0x2f99,
		0x2f9b, 0x2f9c, 0x2f65,
		0x2f66, 0x2f67, 0x2f68,
		0x1a53, 0x1a54, 0x1a55,
		0x1a56, 0x17ea, 0x17eb,
		0x17ec, 0x17ed, 0x2012,
		0x2013, 0x2014, 0x2015
	};
	if (is_meson_txlx_cpu())
		base_addr = 0xff900000;
	else
		base_addr = 0xd0100000;

	pr_info("----dump di reg----\n");
	pr_info("----dump size reg---");
	for (i = 0; i < 57; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((size_reg_addr[i]) << 2),
			0x2010 + i, RDMA_RD(size_reg_addr[i]));
	for (i = 0; i < 255; i++) {
		if (i == 0x45)
			pr_info("----nr reg----");
		if (i == 0x80)
			pr_info("----3d reg----");
		if (i == 0x9e)
			pr_info("---nr reg done---");
		if (i == 0x9c)
			pr_info("---3d reg done---");
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x1700 + i) << 2),
			0x1700 + i, RDMA_RD(0x1700 + i));
	}
	pr_info("----dump mcdi reg----\n");
	for (i = 0; i < 201; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x2f00 + i) << 2),
			0x2f00 + i, RDMA_RD(0x2f00 + i));
	pr_info("----dump pulldown reg----\n");
	for (i = 0; i < 26; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x2fd0 + i) << 2),
			0x2fd0 + i, RDMA_RD(0x2fd0 + i));
	pr_info("----dump bit mode reg----\n");
	for (i = 0; i < 4; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x20a7 + i) << 2),
			0x20a7 + i, RDMA_RD(0x20a7 + i));
	pr_info("[0x%x][0x%x]=0x%x\n",
		base_addr + (0x2022 << 2),
		0x2022, RDMA_RD(0x2022));
	pr_info("[0x%x][0x%x]=0x%x\n",
		base_addr + (0x17c1 << 2),
		0x17c1, RDMA_RD(0x17c1));
	pr_info("[0x%x][0x%x]=0x%x\n",
		base_addr + (0x17c2 << 2),
		0x17c2, RDMA_RD(0x17c2));
	pr_info("[0x%x][0x%x]=0x%x\n",
		base_addr + (0x1aa7 << 2),
		0x1aa7, RDMA_RD(0x1aa7));
	pr_info("----dump dnr reg----\n");
	for (i = 0; i < 29; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x2d00 + i) << 2),
			0x2d00 + i, RDMA_RD(0x2d00 + i));
	pr_info("----dump if0 reg----\n");
	for (i = 0; i < 26; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x1a60 + i) << 2),
			0x1a50 + i, RDMA_RD(0x1a50 + i));
	pr_info("----dump gate reg----\n");
	pr_info("[0x%x][0x1718]=0x%x\n",
			base_addr + ((0x1718) << 2),
			RDMA_RD(0x1718));
	for (i = 0; i < 5; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x2006 + i) << 2),
			0x2006 + i, RDMA_RD(0x2006 + i));
	pr_info("[0x%x][0x%x]=0x%x\n",
		base_addr + ((0x2dff) << 2),
		0x2dff, RDMA_RD(0x2dff));
	pr_info("----dump if2 reg----\n");
	for (i = 0; i < 29; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((0x2010 + i) << 2),
			0x2010 + i, RDMA_RD(0x2010 + i));
	pr_info("----dump nr4 reg----\n");
	for (i = 0x2da4; i < 0x2df6; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + (i << 2),
			i, RDMA_RD(i));
	for (i = 0x3700; i < 0x373f; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + (i << 2),
			i, RDMA_RD(i));
	for (i = 0; i < 57; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + ((size_reg_addr[i]) << 2),
			size_reg_addr[i], RDMA_RD(size_reg_addr[i]));
	pr_info("----dump reg done----\n");
}

void dump_pre_mif_state(void)
{
	unsigned int i = 0;
	Wr_reg_bits(DI_INP_GEN_REG3, 3, 10, 2);
	Wr_reg_bits(DI_MEM_GEN_REG3, 3, 10, 2);
	Wr_reg_bits(DI_CHAN2_GEN_REG3, 3, 10, 2);
	pr_info("DI_INP_GEN_REG2=0x%x.\n", Rd(DI_INP_GEN_REG2));
	pr_info("DI_INP_GEN_REG3=0x%x.\n", Rd(DI_INP_GEN_REG3));
	pr_info("DI_INP_LUMA_FIFO_SIZE=0x%x.\n", Rd(DI_INP_LUMA_FIFO_SIZE));
	pr_info("DI_INP_RANGE_MAP_Y=0x%x.\n", Rd(DI_INP_RANGE_MAP_Y));
	pr_info("DI_INP_RANGE_MAP_CB=0x%x.\n", Rd(DI_INP_RANGE_MAP_CB));
	pr_info("DI_INP_RANGE_MAP_CR=0x%x.\n", Rd(DI_INP_RANGE_MAP_CR));
	pr_info("DI_INP_URGENT_CTRL=0x%x.\n", Rd(DI_INP_URGENT_CTRL));
	for (i = 0; i < 10; i++)
		pr_info("0x%x=0x%x.\n", 0x17ce + i, Rd(0x17ce + i));
	pr_info("DI_MEM_GEN_REG2=0x%x.\n", Rd(DI_MEM_GEN_REG2));
	pr_info("DI_MEM_GEN_REG3=0x%x.\n", Rd(DI_MEM_GEN_REG3));
	pr_info("DI_MEM_LUMA_FIFO_SIZE=0x%x.\n", Rd(DI_MEM_LUMA_FIFO_SIZE));
	pr_info("DI_MEM_RANGE_MAP_CB=0x%x.\n", Rd(DI_MEM_RANGE_MAP_CB));
	pr_info("DI_MEM_RANGE_MAP_CR=0x%x.\n", Rd(DI_MEM_RANGE_MAP_CR));
	pr_info("DI_MEM_URGENT_CTRL=0x%x.\n", Rd(DI_MEM_URGENT_CTRL));
	for (i = 0; i < 10; i++)
		pr_info("0x%x=0x%x.\n", 0x17db + i, Rd(0x17db + i));
	pr_info("DI_CHAN2_GEN_REG2=0x%x.\n", Rd(DI_CHAN2_GEN_REG2));
	pr_info("DI_CHAN2_GEN_REG3=0x%x.\n", Rd(DI_CHAN2_GEN_REG3));
	pr_info("DI_CHAN2_LUMA_FIFO_SIZE=0x%x.\n", Rd(DI_CHAN2_LUMA_FIFO_SIZE));
	pr_info("DI_CHAN2_RANGE_MAP_CB=0x%x.\n", Rd(DI_CHAN2_RANGE_MAP_CB));
	pr_info("DI_CHAN2_RANGE_MAP_CR=0x%x.\n", Rd(DI_CHAN2_RANGE_MAP_CR));
	pr_info("DI_CHAN2_URGENT_CTRL=0x%x.\n", Rd(DI_CHAN2_URGENT_CTRL));
	for (i = 0; i < 10; i++)
		pr_info("0x%x=0x%x.\n", 0x17f5 + i, Rd(0x17f5 + i));
}

void dump_post_mif_reg(void)
{
	pr_info("VIU_MISC_CTRL0=0x%x\n", Rd(VIU_MISC_CTRL0));

	pr_info("VD1_IF0_GEN_REG=0x%x\n", Rd(VD1_IF0_GEN_REG));
	pr_info("VD1_IF0_GEN_REG2=0x%x\n", Rd(VD1_IF0_GEN_REG2));
	pr_info("VD1_IF0_GEN_REG3=0x%x\n", Rd(VD1_IF0_GEN_REG3));
	pr_info("VD1_IF0_LUMA_X0=0x%x\n", Rd(VD1_IF0_LUMA_X0));
	pr_info("VD1_IF0_LUMA_Y0=0x%x\n", Rd(VD1_IF0_LUMA_Y0));
	pr_info("VD1_IF0_CHROMA_X0=0x%x\n", Rd(VD1_IF0_CHROMA_X0));
	pr_info("VD1_IF0_CHROMA_Y0=0x%x\n", Rd(VD1_IF0_CHROMA_Y0));
	pr_info("VD1_IF0_LUMA_X1=0x%x\n", Rd(VD1_IF0_LUMA_X1));
	pr_info("VD1_IF0_LUMA_Y1=0x%x\n", Rd(VD1_IF0_LUMA_Y1));
	pr_info("VD1_IF0_CHROMA_X1=0x%x\n", Rd(VD1_IF0_CHROMA_X1));
	pr_info("VD1_IF0_CHROMA_Y1=0x%x\n", Rd(VD1_IF0_CHROMA_Y1));
	pr_info("VD1_IF0_REPEAT_LOOP=0x%x\n", Rd(VD1_IF0_RPT_LOOP));
	pr_info("VD1_IF0_LUMA0_RPT_PAT=0x%x\n", Rd(VD1_IF0_LUMA0_RPT_PAT));
	pr_info("VD1_IF0_CHROMA0_RPT_PAT=0x%x\n", Rd(VD1_IF0_CHROMA0_RPT_PAT));
	pr_info("VD1_IF0_LUMA_PSEL=0x%x\n", Rd(VD1_IF0_LUMA_PSEL));
	pr_info("VD1_IF0_CHROMA_PSEL=0x%x\n", Rd(VD1_IF0_CHROMA_PSEL));
	pr_info("VIU_VD1_FMT_CTRL=0x%x\n", Rd(VIU_VD1_FMT_CTRL));
	pr_info("VIU_VD1_FMT_W=0x%x\n", Rd(VIU_VD1_FMT_W));

	pr_info("DI_IF1_GEN_REG=0x%x\n", Rd(DI_IF1_GEN_REG));
	pr_info("DI_IF1_GEN_REG2=0x%x\n", Rd(DI_IF1_GEN_REG2));
	pr_info("DI_IF1_GEN_REG3=0x%x\n", Rd(DI_IF1_GEN_REG3));
	pr_info("DI_IF1_CANVAS0=0x%x\n", Rd(DI_IF1_CANVAS0));
	pr_info("DI_IF1_LUMA_X0=0x%x\n", Rd(DI_IF1_LUMA_X0));
	pr_info("DI_IF1_LUMA_Y0=0x%x\n", Rd(DI_IF1_LUMA_Y0));
	pr_info("DI_IF1_CHROMA_X0=0x%x\n", Rd(DI_IF1_CHROMA_X0));
	pr_info("DI_IF1_CHROMA_Y0=0x%x\n", Rd(DI_IF1_CHROMA_Y0));
	pr_info("DI_IF1_LUMA0_RPT_PAT=0x%x\n", Rd(DI_IF1_LUMA0_RPT_PAT));
	pr_info("DI_IF1_CHROMA0_RPT_PAT=0x%x\n", Rd(DI_IF1_LUMA0_RPT_PAT));
	pr_info("DI_IF1_FMT_CTRL=0x%x\n", Rd(DI_IF1_FMT_CTRL));
	pr_info("DI_IF1_FMT_W=0x%x\n", Rd(DI_IF1_FMT_W));

	pr_info("DI_IF2_GEN_REG=0x%x\n", Rd(DI_IF2_GEN_REG));
	pr_info("DI_IF2_GEN_REG2=0x%x\n", Rd(DI_IF2_GEN_REG2));
	pr_info("DI_IF2_GEN_REG3=0x%x\n", Rd(DI_IF2_GEN_REG3));
	pr_info("DI_IF2_CANVAS0=0x%x\n", Rd(DI_IF2_CANVAS0));
	pr_info("DI_IF2_LUMA_X0=0x%x\n", Rd(DI_IF2_LUMA_X0));
	pr_info("DI_IF2_LUMA_Y0=0x%x\n", Rd(DI_IF2_LUMA_Y0));
	pr_info("DI_IF2_CHROMA_X0=0x%x\n", Rd(DI_IF2_CHROMA_X0));
	pr_info("DI_IF2_CHROMA_Y0=0x%x\n", Rd(DI_IF2_CHROMA_Y0));
	pr_info("DI_IF2_LUMA0_RPT_PAT=0x%x\n", Rd(DI_IF2_LUMA0_RPT_PAT));
	pr_info("DI_IF2_CHROMA0_RPT_PAT=0x%x\n", Rd(DI_IF2_LUMA0_RPT_PAT));
	pr_info("DI_IF2_FMT_CTRL=0x%x\n", Rd(DI_IF2_FMT_CTRL));
	pr_info("DI_IF2_FMT_W=0x%x\n", Rd(DI_IF2_FMT_W));

	pr_info("DI_DIWR_Y=0x%x\n", Rd(DI_DIWR_Y));
	pr_info("DI_DIWR_CTRL=0x%x", Rd(DI_DIWR_CTRL));
	pr_info("DI_DIWR_X=0x%x.\n", Rd(DI_DIWR_X));
}
