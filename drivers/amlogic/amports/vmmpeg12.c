/*
 * drivers/amlogic/amports/vmpeg12.c
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
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/platform_device.h>
#include <linux/amlogic/amports/ptsserv.h>
#include <linux/amlogic/amports/amstream.h>
#include <linux/amlogic/canvas/canvas.h>

#include <linux/amlogic/amports/vframe.h>
#include <linux/amlogic/amports/vframe_provider.h>
#include <linux/amlogic/amports/vframe_receiver.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/codec_mm/codec_mm.h>
#include <linux/amlogic/codec_mm/configs.h>

#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include "vdec_reg.h"
#include "vmpeg12.h"
#include "arch/register.h"
#include "amports_priv.h"
#include "decoder/decoder_bmmu_box.h"


#include "amvdec.h"
#define MEM_NAME "codec_mpeg12"
#define CHECK_INTERVAL        (HZ/100)

#define DRIVER_NAME "ammvdec_mpeg12"
#define MODULE_NAME "ammvdec_mpeg12"
#define MREG_REF0        AV_SCRATCH_2
#define MREG_REF1        AV_SCRATCH_3
/* protocol registers */
#define MREG_SEQ_INFO       AV_SCRATCH_4
#define MREG_PIC_INFO       AV_SCRATCH_5
#define MREG_PIC_WIDTH      AV_SCRATCH_6
#define MREG_PIC_HEIGHT     AV_SCRATCH_7
#define MREG_INPUT          AV_SCRATCH_8  /*input_type*/
#define MREG_BUFFEROUT      AV_SCRATCH_9  /*FROM_AMRISC_REG*/

#define MREG_CMD            AV_SCRATCH_A
#define MREG_CO_MV_START    AV_SCRATCH_B
#define MREG_ERROR_COUNT    AV_SCRATCH_C
#define MREG_FRAME_OFFSET   AV_SCRATCH_D
#define MREG_WAIT_BUFFER    AV_SCRATCH_E
#define MREG_FATAL_ERROR    AV_SCRATCH_F
#define DECODE_STOP_POS         AV_SCRATCH_K
#define PICINFO_ERROR       0x80000000
#define PICINFO_TYPE_MASK   0x00030000
#define PICINFO_TYPE_I      0x00000000
#define PICINFO_TYPE_P      0x00010000
#define PICINFO_TYPE_B      0x00020000

#define PICINFO_PROG        0x8000
#define PICINFO_RPT_FIRST   0x4000
#define PICINFO_TOP_FIRST   0x2000
#define PICINFO_FRAME       0x1000

#define SEQINFO_EXT_AVAILABLE   0x80000000
#define SEQINFO_PROG            0x00010000
#define CCBUF_SIZE      (5*1024)

#define VF_POOL_SIZE        32
#define DECODE_BUFFER_NUM_MAX 8
#define PUT_INTERVAL        (HZ/100)
#define WORKSPACE_SIZE		(4*SZ_64K) /*swap&ccbuf&matirx&MV*/
#define CTX_LMEM_SWAP_OFFSET    0
#define CTX_CCBUF_OFFSET        0x800
#define CTX_QUANT_MATRIX_OFFSET (CTX_CCBUF_OFFSET + 5*1024)
#define CTX_CO_MV_OFFSET        (CTX_QUANT_MATRIX_OFFSET + 1*1024)
#define CTX_DECBUF_OFFSET       (CTX_CO_MV_OFFSET + 0x11000)

#define MAX_BMMU_BUFFER_NUM (DECODE_BUFFER_NUM_MAX + 1)
#define DEFAULT_MEM_SIZE	(32*SZ_1M)
static u32 buf_size = 32 * 1024 * 1024;
static int pre_decode_buf_level = 0x800;
static u32 dec_control;
static u32 error_frame_skip_level;
static u32 stat;
static u32 udebug_flag;

#define INCPTR(p) ptr_atomic_wrap_inc(&p)

#define DEC_CONTROL_FLAG_FORCE_2500_720_576_INTERLACE  0x0002
#define DEC_CONTROL_FLAG_FORCE_3000_704_480_INTERLACE  0x0004
#define DEC_CONTROL_FLAG_FORCE_2500_704_576_INTERLACE  0x0008
#define DEC_CONTROL_FLAG_FORCE_2500_544_576_INTERLACE  0x0010
#define DEC_CONTROL_FLAG_FORCE_2500_480_576_INTERLACE  0x0020
#define DEC_CONTROL_INTERNAL_MASK                      0x0fff
#define DEC_CONTROL_FLAG_FORCE_SEQ_INTERLACE           0x1000

#define INTERLACE_SEQ_ALWAYS

#if 1/* MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
#define NV21
#endif


enum {
	FRAME_REPEAT_TOP,
	FRAME_REPEAT_BOT,
	FRAME_REPEAT_NONE
};
#define DEC_RESULT_NONE     0
#define DEC_RESULT_DONE     1
#define DEC_RESULT_AGAIN    2
#define DEC_RESULT_ERROR    3
#define DEC_RESULT_FORCE_EXIT 4
#define DEC_RESULT_EOS 5
#define DEC_DECODE_TIMEOUT         0x21
#define DECODE_ID(hw) (hw_to_vdec(hw)->id)

static struct vframe_s *vmpeg_vf_peek(void *);
static struct vframe_s *vmpeg_vf_get(void *);
static void vmpeg_vf_put(struct vframe_s *, void *);
static int vmpeg_vf_states(struct vframe_states *states, void *);
static int vmpeg_event_cb(int type, void *data, void *private_data);

struct vdec_mpeg12_hw_s {
	spinlock_t lock;
	struct platform_device *platform_dev;
	DECLARE_KFIFO(newframe_q, struct vframe_s *, VF_POOL_SIZE);
	DECLARE_KFIFO(display_q, struct vframe_s *, VF_POOL_SIZE);
	struct vframe_s vfpool[VF_POOL_SIZE];
	s32 vfbuf_use[DECODE_BUFFER_NUM_MAX];
	u32 frame_width;
	u32 frame_height;
	u32 frame_dur;
	u32 frame_prog;
	u32 seqinfo;
	u32 ctx_valid;
	u32 dec_control;
	void *mm_blk_handle;
	struct vframe_chunk_s *chunk;
	u32 stat;
	u8 init_flag;
	unsigned long buf_start;
	u32 buf_size;
	u32 reg_pic_width;
	u32 reg_pic_height;
	u32 reg_mpeg1_2_reg;
	u32 reg_pic_head_info;
	u32 reg_f_code_reg;
	u32 reg_slice_ver_pos_pic_type;
	u32 reg_vcop_ctrl_reg;
	u32 reg_mb_info;
	u32 frame_num;
	struct timer_list check_timer;
	unsigned decode_timeout_count;
	u32 eos;
	u32 buffer_info[DECODE_BUFFER_NUM_MAX];
	u32 pts[DECODE_BUFFER_NUM_MAX];
	u64 pts64[DECODE_BUFFER_NUM_MAX];
	bool pts_valid[DECODE_BUFFER_NUM_MAX];
	u32 canvas_spec[DECODE_BUFFER_NUM_MAX];
	struct canvas_config_s canvas_config[DECODE_BUFFER_NUM_MAX][2];
	struct dec_sysinfo vmpeg12_amstream_dec_info;

	s32 refs[2];
	int dec_result;
	struct work_struct work;

	void (*vdec_cb)(struct vdec_s *, void *);
	void *vdec_cb_arg;
	unsigned long ccbuf_phyAddress;
	void *ccbuf_phyAddress_virt;
	unsigned long ccbuf_phyAddress_is_remaped_nocache;
	u32 frame_rpt_state;
/* for error handling */
	s32 frame_force_skip_flag;
	s32 error_frame_skip_level;
	s32 wait_buffer_counter;
	u32 first_i_frame_ready;
	u32 timer_counter;
	u32 run_count;
	u32	not_run_ready;
	u32	input_empty;
	u32 put_num;
	u32 peek_num;
	u32 get_num;
};
static void vmpeg12_local_init(struct vdec_mpeg12_hw_s *hw);
static int vmpeg12_hw_ctx_restore(struct vdec_mpeg12_hw_s *hw);
/*static struct vdec_info *gvs;*/
static int debug_enable;
/*static struct work_struct userdata_push_work;*/
#undef pr_info
#define pr_info printk
unsigned int mpeg12_debug_mask = 0xff;
static int counter_max = 3;

#define PRINT_FLAG_ERROR              0x0
#define PRINT_FLAG_RUN_FLOW           0X0001
#define PRINT_FLAG_TIMEINFO           0x0002
#define PRINT_FLAG_UCODE_DETAIL		  0x0004
#define PRINT_FLAG_VLD_DETAIL         0x0008
#define PRINT_FLAG_DEC_DETAIL         0x0010
#define PRINT_FLAG_BUFFER_DETAIL      0x0020
#define PRINT_FLAG_RESTORE            0x0040
#define PRINT_FRAME_NUM               0x0080
#define PRINT_FLAG_FORCE_DONE         0x0100
#define PRINT_FLAG_COUNTER            0X0200
#define PRINT_FRAMEBASE_DATA          0x0400


int debug_print(int index, int debug_flag, const char *fmt, ...)
{
	if (((debug_enable & debug_flag) &&
		((1 << index) & mpeg12_debug_mask))
		|| (debug_flag == PRINT_FLAG_ERROR)) {
		unsigned char buf[512];
		int len = 0;
		va_list args;
		va_start(args, fmt);
		len = sprintf(buf, "%d: ", index);
		vsnprintf(buf + len, 512-len, fmt, args);
		pr_info("%s", buf);
		va_end(args);
	}
	return 0;
}


/*static bool is_reset;*/
#define PROVIDER_NAME   "vdec.mpeg12"
static const struct vframe_operations_s vf_provider_ops = {
	.peek = vmpeg_vf_peek,
	.get = vmpeg_vf_get,
	.put = vmpeg_vf_put,
	.event_cb = vmpeg_event_cb,
	.vf_states = vmpeg_vf_states,
};


static const u32 frame_rate_tab[16] = {
	96000 / 30, 96000000 / 23976, 96000 / 24, 96000 / 25,
	9600000 / 2997, 96000 / 30, 96000 / 50, 9600000 / 5994,
	96000 / 60,
	/* > 8 reserved, use 24 */
	96000 / 24, 96000 / 24, 96000 / 24, 96000 / 24,
	96000 / 24, 96000 / 24, 96000 / 24
};


static u32 find_buffer(struct vdec_mpeg12_hw_s *hw)
{
	u32 i;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		if (hw->vfbuf_use[i] == 0)
			return i;
	}

	return DECODE_BUFFER_NUM_MAX;
}

static u32 spec_to_index(struct vdec_mpeg12_hw_s *hw, u32 spec)
{
	u32 i;

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++) {
		if (hw->canvas_spec[i] == spec)
			return i;
	}

	return DECODE_BUFFER_NUM_MAX;
}


static void set_frame_info(struct vdec_mpeg12_hw_s *hw, struct vframe_s *vf)
{
	unsigned ar_bits;
	u32 temp;
	u32 buffer_index = vf->index;
#ifdef CONFIG_AM_VDEC_MPEG12_LOG
	bool first = (hw->frame_width == 0) && (hw->frame_height == 0);
#endif

	temp = READ_VREG(MREG_PIC_WIDTH);
	if (temp > 1920)
		vf->width = hw->frame_width = 1920;
	else
		vf->width = hw->frame_width = temp;

	temp = READ_VREG(MREG_PIC_HEIGHT);
	if (temp > 1088)
		vf->height = hw->frame_height = 1088;
	else
		vf->height = hw->frame_height = temp;

	vf->flag = 0;

	if (hw->frame_dur > 0)
		vf->duration = hw->frame_dur;
	else {
		vf->duration = hw->frame_dur =
		frame_rate_tab[(READ_VREG(MREG_SEQ_INFO) >> 4) & 0xf];
	}

	ar_bits = READ_VREG(MREG_SEQ_INFO) & 0xf;

	if (ar_bits == 0x2)
		vf->ratio_control = 0xc0 << DISP_RATIO_ASPECT_RATIO_BIT;

	else if (ar_bits == 0x3)
		vf->ratio_control = 0x90 << DISP_RATIO_ASPECT_RATIO_BIT;

	else if (ar_bits == 0x4)
		vf->ratio_control = 0x74 << DISP_RATIO_ASPECT_RATIO_BIT;
	else
		vf->ratio_control = 0;

	vf->canvas0Addr = vf->canvas1Addr = -1;
	vf->plane_num = 2;

	vf->canvas0_config[0] = hw->canvas_config[buffer_index][0];
	vf->canvas0_config[1] = hw->canvas_config[buffer_index][1];

	vf->canvas1_config[0] = hw->canvas_config[buffer_index][0];
	vf->canvas1_config[1] = hw->canvas_config[buffer_index][1];


	debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
	"mpeg2dec: w(%d), h(%d), dur(%d), dur-ES(%d)\n",
		hw->frame_width, hw->frame_height, hw->frame_dur,
		frame_rate_tab[(READ_VREG(MREG_SEQ_INFO) >> 4) & 0xf]);
}

static bool error_skip(struct vdec_mpeg12_hw_s *hw,
	u32 info, struct vframe_s *vf)
{
	if (hw->error_frame_skip_level) {
		/* skip error frame */
		if ((info & PICINFO_ERROR) || (hw->frame_force_skip_flag)) {
			if ((info & PICINFO_ERROR) == 0) {
				if ((info & PICINFO_TYPE_MASK) ==
					PICINFO_TYPE_I)
					hw->frame_force_skip_flag = 0;
			} else {
				if (hw->error_frame_skip_level >= 2)
					hw->frame_force_skip_flag = 1;
			}
			if ((info & PICINFO_ERROR)
			|| (hw->frame_force_skip_flag))
				return true;
		}
	}
	return false;
}

static inline void vmpeg12_save_hw_context(struct vdec_mpeg12_hw_s *hw)
{
	hw->seqinfo = READ_VREG(MREG_SEQ_INFO);
	hw->reg_pic_width = READ_VREG(MREG_PIC_WIDTH);
	hw->reg_pic_height = READ_VREG(MREG_PIC_HEIGHT);
	hw->reg_mpeg1_2_reg = READ_VREG(MPEG1_2_REG);
	hw->reg_pic_head_info = READ_VREG(PIC_HEAD_INFO);
	hw->reg_f_code_reg = READ_VREG(F_CODE_REG);
	hw->reg_slice_ver_pos_pic_type = READ_VREG(SLICE_VER_POS_PIC_TYPE);
	hw->reg_vcop_ctrl_reg = READ_VREG(VCOP_CTRL_REG);
	hw->reg_mb_info = READ_VREG(MB_INFO);
}
#if 0
static void userdata_push_do_work(struct work_struct *work)
{
	u32 reg;

	struct userdata_poc_info_t user_data_poc;
/*	struct vdec_mpeg12_hw_s *hw =
		container_of(work, struct vdec_mpeg12_hw_s, work);*/
	user_data_poc.poc_info = 0;
	user_data_poc.poc_number = 0;
	reg = READ_VREG(MREG_BUFFEROUT);
	/*pr_info("%s,%d\n",__func__,__LINE__);*/

	if (!hw->ccbuf_phyAddress_is_remaped_nocache &&
		hw->ccbuf_phyAddress &&
		hw->ccbuf_phyAddress_virt) {
		codec_mm_dma_flush(
			hw->ccbuf_phyAddress_virt,
			CCBUF_SIZE,
			DMA_FROM_DEVICE);
	}
	wakeup_userdata_poll(user_data_poc,
		reg & 0xffff,
		(unsigned long)hw->ccbuf_phyAddress_virt,
		CCBUF_SIZE, 0);

	WRITE_VREG(MREG_BUFFEROUT, 0);
}
#endif
static irqreturn_t vmpeg12_isr(struct vdec_s *vdec)
{
	u32 reg, info, seqinfo, offset, pts, pts_valid = 0;
	struct vframe_s *vf = NULL;
	u32 index;
	u64 pts_us64 = 0;
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)(vdec->private);

	if (hw->eos)
		return IRQ_HANDLED;

	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	if (READ_VREG(AV_SCRATCH_M) != 0 &&
		(debug_enable & PRINT_FLAG_UCODE_DETAIL)) {

		debug_print(DECODE_ID(hw), PRINT_FLAG_UCODE_DETAIL,
		"dbg %x: %x, level %x, wp %x, rp %x, cnt %x\n",
			READ_VREG(AV_SCRATCH_M), READ_VREG(AV_SCRATCH_N),
			READ_VREG(VLD_MEM_VIFIFO_LEVEL),
			READ_VREG(VLD_MEM_VIFIFO_WP),
			READ_VREG(VLD_MEM_VIFIFO_RP),
			READ_VREG(VIFF_BIT_CNT));
		WRITE_VREG(AV_SCRATCH_M, 0);
		return IRQ_HANDLED;
	}

	reg = READ_VREG(MREG_BUFFEROUT);

	if ((reg >> 16) == 0xfe) {
		/*pr_info("%s,%d\n",__func__,__LINE__);*/
		/*schedule_work(&userdata_push_work);*/
		WRITE_VREG(MREG_BUFFEROUT, 0);
	} else if (reg == 2) {
		/*timeout when decoding next frame*/

		debug_print(DECODE_ID(hw), PRINT_FLAG_VLD_DETAIL,
		"amvdec_mpeg12: Insufficient data\n");
		debug_print(DECODE_ID(hw), PRINT_FLAG_VLD_DETAIL,
		"level=%x, vtl=%x,bcnt=%d\n",
		READ_VREG(VLD_MEM_VIFIFO_LEVEL),
		READ_VREG(VLD_MEM_VIFIFO_CONTROL),
		READ_VREG(VIFF_BIT_CNT));

		hw->dec_result = DEC_RESULT_AGAIN;
		schedule_work(&hw->work);
		hw->timer_counter = 0;
		return IRQ_HANDLED;
	} else {
		hw->timer_counter = 0;
		info = READ_VREG(MREG_PIC_INFO);
		offset = READ_VREG(MREG_FRAME_OFFSET);
		index = spec_to_index(hw, READ_VREG(REC_CANVAS_ADDR));
		seqinfo = READ_VREG(MREG_SEQ_INFO);
		if (index >= DECODE_BUFFER_NUM_MAX) {

			debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
			"invalid buffer index,index=%d\n",
				index);
			hw->dec_result = DEC_RESULT_ERROR;
			schedule_work(&hw->work);
			return IRQ_HANDLED;
		}

		hw->dec_result = DEC_RESULT_DONE;

		debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
		"amvdec_mpeg12: error = 0x%x, offset = 0x%x\n",
			info & PICINFO_ERROR, offset);

		if ((hw->first_i_frame_ready == 0) &&
			((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_I) &&
			((info & PICINFO_ERROR) == 0))
			hw->first_i_frame_ready = 1;
		if (((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_I) ||
			((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_P)) {
			if (hw->chunk) {
				hw->pts_valid[index] = hw->chunk->pts_valid;
				hw->pts[index] = hw->chunk->pts;
				hw->pts64[index] = hw->chunk->pts64;

		debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
		"!!!cpts=%d,pts64=%lld,size=%d,offset=%d\n",
			hw->pts[index], hw->pts64[index],
			hw->chunk->size, hw->chunk->offset);
		} else {
				if (pts_lookup_offset_us64(PTS_TYPE_VIDEO,
				offset, &pts, 0, &pts_us64) == 0) {
					hw->pts_valid[index] = true;
					hw->pts[index] = pts;
					hw->pts64[index] = pts_us64;
				} else {
					hw->pts_valid[index] = false;
				}
			}
		} else {
			hw->pts_valid[index] = false;
		}
		/*if (frame_prog == 0) */
		{
			hw->frame_prog = info & PICINFO_PROG;
			if ((seqinfo & SEQINFO_EXT_AVAILABLE)
				&& (!(seqinfo & SEQINFO_PROG)))
				hw->frame_prog = 0;
		}
		if ((hw->dec_control &
		DEC_CONTROL_FLAG_FORCE_2500_720_576_INTERLACE) &&
			(hw->frame_width == 720) &&
			(hw->frame_height == 576) &&
			(hw->frame_dur == 3840)) {
			hw->frame_prog = 0;
		} else if ((hw->dec_control
		& DEC_CONTROL_FLAG_FORCE_3000_704_480_INTERLACE) &&
			(hw->frame_width == 704) &&
			(hw->frame_height == 480) &&
			(hw->frame_dur == 3200)) {
			hw->frame_prog = 0;
		} else if ((hw->dec_control
		& DEC_CONTROL_FLAG_FORCE_2500_704_576_INTERLACE) &&
			(hw->frame_width == 704) &&
			(hw->frame_height == 576) &&
			(hw->frame_dur == 3840)) {
			hw->frame_prog = 0;
		} else if ((hw->dec_control
		& DEC_CONTROL_FLAG_FORCE_2500_544_576_INTERLACE) &&
			(hw->frame_width == 544) &&
			(hw->frame_height == 576) &&
			(hw->frame_dur == 3840)) {
			hw->frame_prog = 0;
		} else if ((hw->dec_control
		& DEC_CONTROL_FLAG_FORCE_2500_480_576_INTERLACE) &&
			(hw->frame_width == 480) &&
			(hw->frame_height == 576) &&
			(hw->frame_dur == 3840)) {
			hw->frame_prog = 0;
		} else if (hw->dec_control
		& DEC_CONTROL_FLAG_FORCE_SEQ_INTERLACE) {
			hw->frame_prog = 0;
		}

		hw->buffer_info[index] = info;

		debug_print(DECODE_ID(hw), PRINT_FLAG_DEC_DETAIL,
		"amvdec_mpeg12: decoded buffer %d, frame_type %s\n",
		index,
		((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_I) ? "I" :
		((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_P)
		? "P" : "B");

		/* Buffer management
		todo: add sequence-end flush */
		if (((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_I) ||
			((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_P)) {
			hw->vfbuf_use[index]++;
			if (hw->refs[1] == -1) {
				hw->refs[1] = index;
				index = DECODE_BUFFER_NUM_MAX;
			} else if (hw->refs[0] == -1) {
				hw->refs[0] = hw->refs[1];
				hw->refs[1] = index;
				index = hw->refs[0];
			} else {
				hw->vfbuf_use[hw->refs[0]]--;
				hw->refs[0] = hw->refs[1];
				hw->refs[1] = index;
				index = hw->refs[0];
			}
		} else {
		/* if this is a B frame, then drop
		(depending on if there are two reference frames)
		or display immediately*/
			if (hw->refs[0] == -1)
				index = DECODE_BUFFER_NUM_MAX;

		}

		vmpeg12_save_hw_context(hw);

		if (index >= DECODE_BUFFER_NUM_MAX) {
			pr_err("invalid buffer index,index=%d\n", index);
			hw->dec_result = DEC_RESULT_ERROR;
			schedule_work(&hw->work);
			return IRQ_HANDLED;
		}

		info = hw->buffer_info[index];
		pts_valid = hw->pts_valid[index];
		pts = hw->pts[index];
		pts_us64 = hw->pts64[index];

		debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
		"amvdec_mpeg12: display buffer %d, frame_type %s\n",
			index,
			((info & PICINFO_TYPE_MASK) == PICINFO_TYPE_I) ? "I" :
			((info & PICINFO_TYPE_MASK)
			== PICINFO_TYPE_P)	? "P" : "B");
		if (hw->frame_prog & PICINFO_PROG) {

			seqinfo = READ_VREG(MREG_SEQ_INFO);

			if (kfifo_get(&hw->newframe_q, &vf) == 0) {
				debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
				"fatal error, no available buffer slot.");
				hw->dec_result = DEC_RESULT_ERROR;
				schedule_work(&hw->work);

				return IRQ_HANDLED;
			}

			vf->index = index;
			set_frame_info(hw, vf);
			vf->signal_type = 0;

#ifdef NV21
			vf->type =
				VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD |
				VIDTYPE_VIU_NV21;
#else
			vf->type = VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD;
#endif
			if ((hw->seqinfo & SEQINFO_EXT_AVAILABLE)
				&& (hw->seqinfo & SEQINFO_PROG)) {
				if (info & PICINFO_RPT_FIRST) {
					if (info & PICINFO_TOP_FIRST) {
						vf->duration =
							vf->duration * 3;
						/* repeat three times */
					} else {
						vf->duration =
							vf->duration * 2;
						/* repeat two times */
					}
				}
				vf->duration_pulldown = 0;
					/* no pull down */

			} else {
				vf->duration_pulldown =
					(info & PICINFO_RPT_FIRST) ?
						vf->duration >> 1 : 0;
			}

			vf->duration += vf->duration_pulldown;

			vf->orientation = 0;
			vf->pts = (pts_valid) ? pts : 0;
			vf->pts_us64 = (pts_valid) ? pts_us64 : 0;
			vf->type_original = vf->type;
			hw->vfbuf_use[index]++;

			if ((error_skip(hw, info, vf)) ||
				((hw->first_i_frame_ready == 0)
				 && ((PICINFO_TYPE_MASK & info) !=
					 PICINFO_TYPE_I))) {
				/*gvs->drop_frame_count++;*/
				hw->vfbuf_use[index]--;

				kfifo_put(&hw->newframe_q,
				(const struct vframe_s *)vf);

			} else {

			debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
			"cpts=%d,pts64=%lld\n",
				vf->pts, vf->pts_us64);
				kfifo_put(&hw->display_q,
				(const struct vframe_s *)vf);
				vf_notify_receiver(vdec->vf_provider_name,
					VFRAME_EVENT_PROVIDER_VFRAME_READY,
					NULL);
			}

		}
/*interlace temp*/
		else {

			int first_field_type = (info & PICINFO_TOP_FIRST) ?
					VIDTYPE_INTERLACE_TOP :
					VIDTYPE_INTERLACE_BOTTOM;

#ifdef INTERLACE_SEQ_ALWAYS
			/* once an interlaced sequence exist,
			always force interlaced type */
			/* to make DI easy. */
			hw->dec_control |= DEC_CONTROL_FLAG_FORCE_SEQ_INTERLACE;
#endif

			hw->frame_rpt_state = FRAME_REPEAT_NONE;

			if (kfifo_get(&hw->newframe_q, &vf) == 0) {
				debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
				"fatal error, no available buffer slot.");
				schedule_work(&hw->work);
				return IRQ_HANDLED;
			}

			hw->vfbuf_use[index] += 2;
			vf->signal_type = 0;
			vf->index = index;
			set_frame_info(hw, vf);
			vf->type =
				(first_field_type == VIDTYPE_INTERLACE_TOP) ?
				VIDTYPE_INTERLACE_TOP :
				VIDTYPE_INTERLACE_BOTTOM;
#ifdef NV21
			vf->type |= VIDTYPE_VIU_NV21;
#endif
			if (info & PICINFO_RPT_FIRST)
				vf->duration /= 3;
			else
				vf->duration >>= 1;
			vf->duration_pulldown = (info & PICINFO_RPT_FIRST) ?
						vf->duration >> 1 : 0;
			vf->duration += vf->duration_pulldown;
			vf->orientation = 0;
			vf->pts = (pts_valid) ? pts : 0;
			vf->pts_us64 = (pts_valid) ? pts_us64 : 0;

			if ((error_skip(hw, info, vf)) ||
				((hw->first_i_frame_ready == 0)
				 && ((PICINFO_TYPE_MASK & info) !=
					 PICINFO_TYPE_I))) {
				hw->vfbuf_use[index]--;
				/*gvs->drop_frame_count++;*/
				kfifo_put(&hw->newframe_q,
				(const struct vframe_s *)vf);
			} else {

			debug_print(DECODE_ID(hw), PRINT_FLAG_TIMEINFO,
			"cpts=%d,pts64=%lld\n",
				vf->pts, vf->pts_us64);
				kfifo_put(&hw->display_q,
				(const struct vframe_s *)vf);

				vf_notify_receiver(vdec->vf_provider_name,
				VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			}

			if (kfifo_get(&hw->newframe_q, &vf) == 0) {
				debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
				"mmpeg12: fatal error, no available buffer slot.");

				hw->vfbuf_use[index]--;

				schedule_work(&hw->work);

				return IRQ_HANDLED;
			}
			vf->signal_type = 0;
			vf->index = index;
			set_frame_info(hw, vf);
			vf->type = (first_field_type ==
				VIDTYPE_INTERLACE_TOP) ?
				VIDTYPE_INTERLACE_BOTTOM :
				VIDTYPE_INTERLACE_TOP;
#ifdef NV21
			vf->type |= VIDTYPE_VIU_NV21;
#endif
			if (info & PICINFO_RPT_FIRST)
				vf->duration /= 3;
			else
				vf->duration >>= 1;
			vf->duration_pulldown = (info & PICINFO_RPT_FIRST) ?
				vf->duration >> 1 : 0;
			vf->duration += vf->duration_pulldown;
			vf->orientation = 0;
			vf->pts = 0;
			vf->pts_us64 = 0;
			vf->type_original = vf->type;

			if ((error_skip(hw, info, vf)) ||
				((hw->first_i_frame_ready == 0)
				 && ((PICINFO_TYPE_MASK & info) !=
					PICINFO_TYPE_I))) {
				/*gvs->drop_frame_count++;*/
				hw->vfbuf_use[index]--;

				kfifo_put(&hw->newframe_q,
				(const struct vframe_s *)vf);
			} else {
				kfifo_put(&hw->display_q,
				(const struct vframe_s *)vf);

				vf_notify_receiver(vdec->vf_provider_name,
				VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			}

			if (info & PICINFO_RPT_FIRST) {
				if (kfifo_get(&hw->newframe_q, &vf) == 0) {
					debug_print(DECODE_ID(hw),
					PRINT_FLAG_ERROR,
					"error, no available buffer slot.");
					return IRQ_HANDLED;
				}

				vf->index = index;
				set_frame_info(hw, vf);
				vf->type = (first_field_type ==
						VIDTYPE_INTERLACE_TOP) ?
						VIDTYPE_INTERLACE_TOP :
						VIDTYPE_INTERLACE_BOTTOM;
#ifdef NV21
				vf->type |= VIDTYPE_VIU_NV21;
#endif
				vf->duration /= 3;
				vf->duration_pulldown =
					(info & PICINFO_RPT_FIRST) ?
						vf->duration >> 1 : 0;
				vf->duration += vf->duration_pulldown;
				vf->orientation = 0;

				vf->pts = 0;
				vf->pts_us64 = 0;
				if ((error_skip(hw, info, vf)) ||
					((hw->first_i_frame_ready == 0)
						&& ((PICINFO_TYPE_MASK & info)
							!= PICINFO_TYPE_I))) {
					kfifo_put(&hw->newframe_q,
					(const struct vframe_s *)vf);
				} else {
					kfifo_put(&hw->display_q,
					(const struct vframe_s *)vf);
					vf_notify_receiver(
					vdec->vf_provider_name,
					VFRAME_EVENT_PROVIDER_VFRAME_READY,
					NULL);
				}
			}
		}
		schedule_work(&hw->work);
		hw->frame_num++;

		debug_print(DECODE_ID(hw), PRINT_FRAME_NUM,
		"frame_num=%d\n", hw->frame_num);
		if (hw->frame_num == 1)
			debug_print(DECODE_ID(hw), PRINT_FRAME_NUM,
		"frame_num==1\n");
		if (hw->frame_num == 1000)
			debug_print(DECODE_ID(hw), PRINT_FRAME_NUM,
		"frame_num==1000\n");
	}

	return IRQ_HANDLED;
}

static void vmpeg12_work(struct work_struct *work)
{
	struct vdec_mpeg12_hw_s *hw =
	container_of(work, struct vdec_mpeg12_hw_s, work);
	if (hw->dec_result != DEC_RESULT_DONE)
		debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
	"mmpeg12: vmpeg_work,result=%d,status=%d\n",
	hw->dec_result, hw_to_vdec(hw)->next_status);

	if (hw->dec_result == DEC_RESULT_DONE) {
		if (!hw->ctx_valid)
			hw->ctx_valid = 1;

		vdec_vframe_dirty(hw_to_vdec(hw), hw->chunk);
	} else if (hw->dec_result == DEC_RESULT_AGAIN
	&& (hw_to_vdec(hw)->next_status !=
		VDEC_STATUS_DISCONNECTED)) {
		/*
			stream base: stream buf empty or timeout
			frame base: vdec_prepare_input fail
		*/
		if (!vdec_has_more_input(hw_to_vdec(hw))) {
			hw->dec_result = DEC_RESULT_EOS;
			vdec_schedule_work(&hw->work);
			/*pr_info("%s: return\n",
			__func__);*/
			return;
		}
	} else if (hw->dec_result == DEC_RESULT_FORCE_EXIT) {
		debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
		"%s: force exit\n",
			__func__);
	} else if (hw->dec_result == DEC_RESULT_EOS) {
		/*pr_info("%s: end of stream\n",
			__func__);*/
		if (READ_VREG(VLD_MEM_VIFIFO_LEVEL) < 0x100)
			hw->eos = 1;
		vdec_vframe_dirty(hw_to_vdec(hw), hw->chunk);
	}
	amvdec_stop();
	vdec_set_status(hw_to_vdec(hw), VDEC_STATUS_CONNECTED);
	del_timer_sync(&hw->check_timer);
	hw->stat &= ~STAT_TIMER_ARM;

	if (hw->vdec_cb)
		hw->vdec_cb(hw_to_vdec(hw), hw->vdec_cb_arg);

}

static struct vframe_s *vmpeg_vf_peek(void *op_arg)
{
	struct vframe_s *vf;
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)vdec->private;
	hw->peek_num++;
	if (kfifo_peek(&hw->display_q, &vf))
		return vf;

	return NULL;
}

static struct vframe_s *vmpeg_vf_get(void *op_arg)
{
	struct vframe_s *vf;
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)vdec->private;

	hw->get_num++;
	if (kfifo_get(&hw->display_q, &vf))
		return vf;

	return NULL;
}

static void vmpeg_vf_put(struct vframe_s *vf, void *op_arg)
{
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)vdec->private;

	hw->vfbuf_use[vf->index]--;
	hw->put_num++;
	kfifo_put(&hw->newframe_q, (const struct vframe_s *)vf);
}

static int vmpeg_event_cb(int type, void *data, void *private_data)
{
	return 0;
}

static int vmpeg_vf_states(struct vframe_states *states, void *op_arg)
{
	unsigned long flags;
	struct vdec_s *vdec = op_arg;
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)vdec->private;

	spin_lock_irqsave(&hw->lock, flags);

	states->vf_pool_size = VF_POOL_SIZE;
	states->buf_free_num = kfifo_len(&hw->newframe_q);
	states->buf_avail_num = kfifo_len(&hw->display_q);
	states->buf_recycle_num = 0;

	spin_unlock_irqrestore(&hw->lock, flags);
	return 0;
}
static int dec_status(struct vdec_s *vdec, struct vdec_info *vstatus)
{
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)vdec->private;
	if (!hw)
		return 0;
	vstatus->frame_width = hw->frame_width;
	vstatus->frame_height = hw->frame_height;
	if (hw->frame_dur != 0)
		vstatus->frame_rate = 96000 / hw->frame_dur;
	else
		vstatus->frame_rate = -1;
	vstatus->error_count = READ_VREG(AV_SCRATCH_C);
	vstatus->status = hw->stat;



	return 0;
}



/****************************************/
static void vmpeg12_canvas_init(struct vdec_mpeg12_hw_s *hw)
{
	int i, ret;
	u32 canvas_width, canvas_height;
	u32 decbuf_size, decbuf_y_size, decbuf_uv_size;
	unsigned long decbuf_start;
	/*u32 disp_addr = 0xffffffff;*/
	struct vdec_s *vdec = hw_to_vdec(hw);

	if (buf_size <= 0x00400000) {
		/* SD only */
		canvas_width = 768;
		canvas_height = 576;
		decbuf_y_size = 0x80000;
		decbuf_uv_size = 0x20000;
		decbuf_size = 0x100000;
	} else {
		/* HD & SD */
		canvas_width = 1920;
		canvas_height = 1088;
		decbuf_y_size = 0x200000;
		decbuf_uv_size = 0x80000;
		decbuf_size = 0x300000;
	}

	for (i = 0; i < MAX_BMMU_BUFFER_NUM; i++) {

		unsigned canvas;
		if (i == (MAX_BMMU_BUFFER_NUM - 1)) /* SWAP&CCBUF&MATIRX&MV */
			decbuf_size = WORKSPACE_SIZE;
		ret = decoder_bmmu_box_alloc_buf_phy(hw->mm_blk_handle, i,
				decbuf_size, DRIVER_NAME, &decbuf_start);
		if (ret < 0) {
			pr_err("mmu alloc failed! size 0x%d  idx %d\n",
				decbuf_size, i);
			return;
		}

		if (i == (MAX_BMMU_BUFFER_NUM - 1)) {
			hw->buf_start = decbuf_start;
			hw->ccbuf_phyAddress = hw->buf_start + CTX_CCBUF_OFFSET;
			WRITE_VREG(MREG_CO_MV_START, hw->buf_start);
		} else {
			canvas = vdec->get_canvas(i, 2);
			hw->canvas_spec[i] = canvas;

			hw->canvas_config[i][0].phy_addr =
			decbuf_start;
			hw->canvas_config[i][0].width =
			canvas_width;
			hw->canvas_config[i][0].height =
			canvas_height;
			hw->canvas_config[i][0].block_mode =
			CANVAS_BLKMODE_32X32;

			canvas_config_config(canvas_y(canvas),
			&hw->canvas_config[i][0]);

			hw->canvas_config[i][1].phy_addr =
			decbuf_start + decbuf_y_size;
			hw->canvas_config[i][1].width =
			canvas_width;
			hw->canvas_config[i][1].height =
			canvas_height / 2;
			hw->canvas_config[i][1].block_mode =
			CANVAS_BLKMODE_32X32;

			canvas_config_config(canvas_u(canvas),
			&hw->canvas_config[i][1]);
		}
	}
	return;
}
static void vmpeg2_dump_state(struct vdec_s *vdec)
{
	struct vdec_mpeg12_hw_s *hw =
		(struct vdec_mpeg12_hw_s *)(vdec->private);
	debug_print(DECODE_ID(hw), 0,
		"====== %s\n", __func__);
	debug_print(DECODE_ID(hw), 0,
		"width/height (%d/%d)\n",
		hw->frame_width,
		hw->frame_height
		);
	debug_print(DECODE_ID(hw), 0,
		"is_framebase(%d), eos %d, state 0x%x, dec_result 0x%x dec_frm %d put_frm %d run %d not_run_ready %d input_empty %d\n",
		input_frame_based(vdec),
		hw->eos,
		hw->stat,
		hw->dec_result,
		hw->frame_num,
		hw->put_num,
		hw->run_count,
		hw->not_run_ready,
		hw->input_empty
		);
	if (vf_get_receiver(vdec->vf_provider_name)) {
		enum receviver_start_e state =
		vf_notify_receiver(vdec->vf_provider_name,
			VFRAME_EVENT_PROVIDER_QUREY_STATE,
			NULL);
		debug_print(DECODE_ID(hw), 0,
			"\nreceiver(%s) state %d\n",
			vdec->vf_provider_name,
			state);
	}
	debug_print(DECODE_ID(hw), 0,
	"%s, newq(%d/%d), dispq(%d/%d) vf peek/get/put (%d/%d/%d)\n",
	__func__,
	kfifo_len(&hw->newframe_q),
	VF_POOL_SIZE,
	kfifo_len(&hw->display_q),
	VF_POOL_SIZE,
	hw->peek_num,
	hw->get_num,
	hw->put_num
	);
	debug_print(DECODE_ID(hw), 0,
		"VIFF_BIT_CNT=0x%x\n",
		READ_VREG(VIFF_BIT_CNT));
	debug_print(DECODE_ID(hw), 0,
		"VLD_MEM_VIFIFO_LEVEL=0x%x\n",
		READ_VREG(VLD_MEM_VIFIFO_LEVEL));
	debug_print(DECODE_ID(hw), 0,
		"VLD_MEM_VIFIFO_WP=0x%x\n",
		READ_VREG(VLD_MEM_VIFIFO_WP));
	debug_print(DECODE_ID(hw), 0,
		"VLD_MEM_VIFIFO_RP=0x%x\n",
		READ_VREG(VLD_MEM_VIFIFO_RP));
	debug_print(DECODE_ID(hw), 0,
		"PARSER_VIDEO_RP=0x%x\n",
		READ_PARSER_REG(PARSER_VIDEO_RP));
	debug_print(DECODE_ID(hw), 0,
		"PARSER_VIDEO_WP=0x%x\n",
		READ_PARSER_REG(PARSER_VIDEO_WP));
	if (input_frame_based(vdec) &&
		debug_enable & PRINT_FRAMEBASE_DATA
		) {
		int jj;
		if (hw->chunk && hw->chunk->block &&
			hw->chunk->size > 0) {
			u8 *data =
			((u8 *)hw->chunk->block->start_virt) +
				hw->chunk->offset;
			debug_print(DECODE_ID(hw), 0,
				"frame data size 0x%x\n",
				hw->chunk->size);
			for (jj = 0; jj < hw->chunk->size; jj++) {
				if ((jj & 0xf) == 0)
					debug_print(DECODE_ID(hw),
					PRINT_FRAMEBASE_DATA,
						"%06x:", jj);
				debug_print(DECODE_ID(hw),
				PRINT_FRAMEBASE_DATA,
					"%02x ", data[jj]);
				if (((jj + 1) & 0xf) == 0)
					debug_print(DECODE_ID(hw),
					PRINT_FRAMEBASE_DATA,
						"\n");
			}
		}
	}
}
static void timeout_process(struct vdec_mpeg12_hw_s *hw)
{
	amvdec_stop();
	debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
	"%s decoder timeout\n", __func__);
	hw->dec_result = DEC_RESULT_DONE;

	vdec_schedule_work(&hw->work);
}

static void check_timer_func(unsigned long arg)
{
	struct vdec_mpeg12_hw_s *hw = (struct vdec_mpeg12_hw_s *)arg;
	struct vdec_s *vdec = hw_to_vdec(hw);


	if ((input_stream_based(vdec) ||
		 vdec->status == VDEC_STATUS_ACTIVE
		 ) && READ_VREG(VLD_MEM_VIFIFO_LEVEL) <= 0x100) {
		if (hw->decode_timeout_count > 0)
			hw->decode_timeout_count--;
		if (hw->decode_timeout_count == 0)
			timeout_process(hw);
	}


	if (vdec->next_status == VDEC_STATUS_DISCONNECTED) {
		hw->dec_result = DEC_RESULT_FORCE_EXIT;
		vdec_schedule_work(&hw->work);
		debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
		"vdec requested to be disconnected\n");
		return;
	}

	hw->timer_counter++;
	if ((hw->timer_counter > counter_max
	&& vdec->status == VDEC_STATUS_ACTIVE
	&& debug_enable == 0)
	|| (debug_enable & PRINT_FLAG_FORCE_DONE) != 0) {
		hw->dec_result = DEC_RESULT_DONE;
		vdec_schedule_work(&hw->work);
		debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
		"vdec %d is forced to be disconnected\n",
			debug_enable & 0xff);
		if (debug_enable & PRINT_FLAG_FORCE_DONE)
			debug_enable = 0;
		return;
	}
	mod_timer(&hw->check_timer, jiffies + CHECK_INTERVAL);
}

static int vmpeg12_hw_ctx_restore(struct vdec_mpeg12_hw_s *hw)
{
	u32 index;
	index = find_buffer(hw);
	if (index >= DECODE_BUFFER_NUM_MAX)
		return -1;

	vmpeg12_canvas_init(hw);

	/* prepare REF0 & REF1
	points to the past two IP buffers
	prepare REC_CANVAS_ADDR and ANC2_CANVAS_ADDR
	points to the output buffer*/
	WRITE_VREG(MREG_REF0,
	(hw->refs[1] == -1) ? 0xffffffff : hw->canvas_spec[hw->refs[0]]);
	WRITE_VREG(MREG_REF1,
	(hw->refs[0] == -1) ? 0xffffffff : hw->canvas_spec[hw->refs[1]]);
	WRITE_VREG(REC_CANVAS_ADDR, hw->canvas_spec[index]);
	WRITE_VREG(ANC2_CANVAS_ADDR, hw->canvas_spec[index]);

	debug_print(DECODE_ID(hw), PRINT_FLAG_RESTORE,
	"%s,ref0=0x%x, ref1=0x%x,rec=0x%x, ctx_valid=%d,index=%d\n",
	__func__,
	READ_VREG(MREG_REF0),
	READ_VREG(MREG_REF1),
	READ_VREG(REC_CANVAS_ADDR),
	hw->ctx_valid, index);

	/* set to mpeg1 default */
	WRITE_VREG(MPEG1_2_REG,
	(hw->ctx_valid) ? hw->reg_mpeg1_2_reg : 0);
	/* disable PSCALE for hardware sharing */
	WRITE_VREG(PSCALE_CTRL, 0);
	/* for Mpeg1 default value */
	WRITE_VREG(PIC_HEAD_INFO,
	(hw->ctx_valid) ? hw->reg_pic_head_info : 0x380);
	/* disable mpeg4 */
	WRITE_VREG(M4_CONTROL_REG, 0);
	/* clear mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);
	/* clear buffer IN/OUT registers */
	WRITE_VREG(MREG_BUFFEROUT, 0);
	/* set reference width and height */
	if ((hw->frame_width != 0) && (hw->frame_height != 0))
		WRITE_VREG(MREG_CMD,
		(hw->frame_width << 16) | hw->frame_height);
	else
		WRITE_VREG(MREG_CMD, 0);


	debug_print(DECODE_ID(hw), PRINT_FLAG_RESTORE,
	"0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
	hw->frame_width, hw->frame_height, hw->seqinfo,
	hw->reg_f_code_reg, hw->reg_slice_ver_pos_pic_type,
	hw->reg_mb_info);

	WRITE_VREG(MREG_PIC_WIDTH, hw->reg_pic_width);
	WRITE_VREG(MREG_PIC_HEIGHT, hw->reg_pic_height);
	WRITE_VREG(MREG_SEQ_INFO, hw->seqinfo);
	WRITE_VREG(F_CODE_REG, hw->reg_f_code_reg);
	WRITE_VREG(SLICE_VER_POS_PIC_TYPE,
	hw->reg_slice_ver_pos_pic_type);
	WRITE_VREG(MB_INFO, hw->reg_mb_info);
	WRITE_VREG(VCOP_CTRL_REG, hw->reg_vcop_ctrl_reg);

	/* clear error count */
	WRITE_VREG(MREG_ERROR_COUNT, 0);
	WRITE_VREG(MREG_FATAL_ERROR, 0);
	/* clear wait buffer status */
	WRITE_VREG(MREG_WAIT_BUFFER, 0);
#ifdef NV21
	SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<17);
#endif

	if (hw->chunk) {
		/*frame based input*/
		WRITE_VREG(MREG_INPUT,
		(hw->chunk->offset & 7) | (1<<7) | (hw->ctx_valid<<6));
	} else {
		/*stream based input*/
		WRITE_VREG(MREG_INPUT, (hw->ctx_valid<<6));
	}

	return 0;
}

static void vmpeg12_local_init(struct vdec_mpeg12_hw_s *hw)
{
	int i;
	INIT_KFIFO(hw->display_q);
	INIT_KFIFO(hw->newframe_q);

	for (i = 0; i < VF_POOL_SIZE; i++) {
		const struct vframe_s *vf;
		vf = &hw->vfpool[i];
		hw->vfpool[i].index = DECODE_BUFFER_NUM_MAX;
		kfifo_put(&hw->newframe_q, (const struct vframe_s *)vf);
	}

	for (i = 0; i < DECODE_BUFFER_NUM_MAX; i++)
		hw->vfbuf_use[i] = 0;


	if (hw->mm_blk_handle) {
		decoder_bmmu_box_free(hw->mm_blk_handle);
		hw->mm_blk_handle = NULL;
	}

	hw->mm_blk_handle = decoder_bmmu_box_alloc_box(
			DRIVER_NAME,
			0,
			MAX_BMMU_BUFFER_NUM,
			4 + PAGE_SHIFT,
			CODEC_MM_FLAGS_CMA_CLEAR |
			CODEC_MM_FLAGS_FOR_VDECODER);

	hw->frame_width = hw->frame_height = 0;
	hw->frame_dur = hw->frame_prog = 0;
	hw->frame_force_skip_flag = 0;
	hw->wait_buffer_counter = 0;
	hw->first_i_frame_ready = 0;
	hw->dec_control &= DEC_CONTROL_INTERNAL_MASK;
	hw->refs[0] = -1;
	hw->refs[1] = -1;
	hw->frame_num = 0;
	hw->put_num = 0;
	hw->run_count = 0;
	hw->not_run_ready = 0;
	hw->input_empty = 0;
	hw->peek_num = 0;
	hw->get_num = 0;
	hw->input_empty = 0;
	hw->error_frame_skip_level = error_frame_skip_level;
	if (dec_control)
		hw->dec_control = dec_control;
	INIT_WORK(&hw->work, vmpeg12_work);
}

static s32 vmpeg12_init(struct vdec_mpeg12_hw_s *hw)
{
	debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
	"vmpeg12_init\n");

	amvdec_enable();

	vmpeg12_local_init(hw);
	init_timer(&hw->check_timer);

	hw->check_timer.data = (unsigned long)hw;
	hw->check_timer.function = check_timer_func;
	hw->check_timer.expires = jiffies + CHECK_INTERVAL;
	hw->stat |= STAT_TIMER_ARM;
	hw->buf_start = 0;
	hw->init_flag = 0;
	hw->timer_counter = 0;
	WRITE_VREG(DECODE_STOP_POS, udebug_flag);
	return 0;
}

static bool run_ready(struct vdec_s *vdec)
{
	int index;
	struct vdec_mpeg12_hw_s *hw =
		(struct vdec_mpeg12_hw_s *)vdec->private;
	if (hw->eos)
		return 0;
	if (vdec_stream_based(vdec) && (hw->init_flag == 0)
		&& pre_decode_buf_level != 0) {
		u32 rp, wp, level;

		rp = READ_PARSER_REG(PARSER_VIDEO_RP);
		wp = READ_PARSER_REG(PARSER_VIDEO_WP);
		if (wp < rp)
			level = vdec->input.size + wp - rp;
		else
			level = wp - rp;

		if (level < pre_decode_buf_level)
			return 0;
	}

	index = find_buffer(hw);
	if (index >= DECODE_BUFFER_NUM_MAX) {
		hw->not_run_ready++;
		return 0;
	}
	hw->not_run_ready = 0;

	return true;
}

static void run(struct vdec_s *vdec,
void (*callback)(struct vdec_s *, void *),
		void *arg)
{
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)vdec->private;
	int save_reg = READ_VREG(POWER_CTL_VLD);
	int r;
	/* reset everything except DOS_TOP[1] and APB_CBUS[0]*/
	WRITE_VREG(DOS_SW_RESET0, 0xfffffff0);
	WRITE_VREG(DOS_SW_RESET0, 0);
	WRITE_VREG(POWER_CTL_VLD, save_reg);
	hw->run_count++;

	hw->vdec_cb_arg = arg;
	hw->vdec_cb = callback;

	r = vdec_prepare_input(vdec, &hw->chunk);
	if (r < 0) {
		hw->input_empty++;
		hw->dec_result = DEC_RESULT_AGAIN;
		schedule_work(&hw->work);
		return;
	}
	hw->input_empty = 0;
	hw->timer_counter = 0;
	debug_print(DECODE_ID(hw), PRINT_FLAG_RUN_FLOW,
	"%s,%d, r=%d\n",
	__func__, __LINE__, r);
	vdec_enable_input(vdec);
	mod_timer(&hw->check_timer, jiffies + CHECK_INTERVAL);
	hw->decode_timeout_count = 2;
	hw->init_flag = 1;

	if (hw->chunk)
		pr_debug("input chunk offset %d, size %d\n",
			hw->chunk->offset, hw->chunk->size);

	hw->dec_result = DEC_RESULT_NONE;

	if (amvdec_loadmc_ex(VFORMAT_MPEG12, "vmmpeg12_mc", NULL) < 0) {
		hw->dec_result = DEC_RESULT_ERROR;
		debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
		"amvdec_mmpeg12 errorloading ucode\n");
		schedule_work(&hw->work);
		return;
	}

	if (vmpeg12_hw_ctx_restore(hw) < 0) {
		hw->dec_result = DEC_RESULT_ERROR;
		debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
		"amvdec_mpeg12: error HW context restore\n");
		schedule_work(&hw->work);
		return;
	}
	/*wmb();*/
	amvdec_start();
}

static void reset(struct vdec_s *vdec)
{
	struct vdec_mpeg12_hw_s *hw =
	(struct vdec_mpeg12_hw_s *)vdec->private;

	debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
	"amvdec_mpeg12: reset.\n");

	return;

}

static int amvdec_mpeg12_probe(struct platform_device *pdev)
{
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	struct vdec_mpeg12_hw_s *hw = NULL;

	pr_info("amvdec_mpeg12 probe start.\n");

	if (pdata == NULL) {
		pr_info("amvdec_mmpeg12 platform data undefined.\n");
		return -EFAULT;
	}

	hw = (struct vdec_mpeg12_hw_s *)devm_kzalloc(&pdev->dev,
		sizeof(struct vdec_mpeg12_hw_s), GFP_KERNEL);
	if (hw == NULL) {
		debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
		"\namvdec_mpeg12 decoder driver alloc failed\n");
		return -ENOMEM;
	}

	pdata->private = hw;
	pdata->dec_status = dec_status;
	pdata->run_ready = run_ready;
	pdata->run = run;
	pdata->reset = reset;
	pdata->irq_handler = vmpeg12_isr;
	pdata->dump_state = vmpeg2_dump_state;
	if (pdata->use_vfm_path)
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			VFM_DEC_PROVIDER_NAME);
	else
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			PROVIDER_NAME ".%02x", pdev->id & 0xff);
	vf_provider_init(&pdata->vframe_provider, pdata->vf_provider_name,
		&vf_provider_ops, pdata);

	platform_set_drvdata(pdev, pdata);

	hw->platform_dev = pdev;

	if (pdata->sys_info)
		hw->vmpeg12_amstream_dec_info = *pdata->sys_info;

	if (vmpeg12_init(hw) < 0) {
		debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
		"amvdec_mpeg12 init failed.\n");

		return -ENODEV;
	}
	hw->eos = 0;
	/*INIT_WORK(&userdata_push_work, userdata_push_do_work);*/

	debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
	"amvdec_mmpeg12 probe end.\n");
	return 0;
}

static int amvdec_mpeg12_remove(struct platform_device *pdev)
{
	struct vdec_mpeg12_hw_s *hw =
		(struct vdec_mpeg12_hw_s *)
		(((struct vdec_s *)(platform_get_drvdata(pdev)))->private);
	cancel_work_sync(&hw->work);
	amvdec_disable();
	if (hw->mm_blk_handle) {
		decoder_bmmu_box_free(hw->mm_blk_handle);
		hw->mm_blk_handle = NULL;
	}
	if (hw->stat & STAT_TIMER_ARM) {
		del_timer_sync(&hw->check_timer);
		hw->stat &= ~STAT_TIMER_ARM;
	}

	vdec_set_status(hw_to_vdec(hw), VDEC_STATUS_DISCONNECTED);

	debug_print(DECODE_ID(hw), PRINT_FLAG_ERROR,
	"amvdec_mpeg12 remove.\n");
	return 0;
}

/****************************************/

static struct platform_driver amvdec_mpeg12_driver = {
	.probe = amvdec_mpeg12_probe,
	.remove = amvdec_mpeg12_remove,
#ifdef CONFIG_PM
	.suspend = amvdec_suspend,
	.resume = amvdec_resume,
#endif
	.driver = {
		.name = DRIVER_NAME,
	}
};

static struct codec_profile_t amvdec_mpeg12_profile = {
	.name = "mmpeg12",
	.profile = ""
};

static struct mconfig mmpeg12_configs[] = {
	MC_PU32("stat", &stat),
	MC_PU32("dec_control", &dec_control),
	MC_PU32("error_frame_skip_level", &error_frame_skip_level),
};
static struct mconfig_node mmpeg12_node;

static int __init amvdec_mmpeg12_driver_init_module(void)
{
	pr_info("amvdec_mmpeg12 module init\n");

	if (platform_driver_register(&amvdec_mpeg12_driver)) {
		pr_info("failed to register amvdec_mpeg12 driver\n");
		return -ENODEV;
	}
	vcodec_profile_register(&amvdec_mpeg12_profile);
	INIT_REG_NODE_CONFIGS("media.decoder", &mmpeg12_node,
		"mmpeg12", mmpeg12_configs, CONFIG_FOR_RW);
	return 0;
}

static void __exit amvdec_mmpeg12_driver_remove_module(void)
{
	pr_info("amvdec_mpeg12 module remove.\n");
	platform_driver_unregister(&amvdec_mpeg12_driver);
}

/****************************************/
module_param(dec_control, uint, 0664);
MODULE_PARM_DESC(dec_control, "\n amvmmpeg12 decoder control\n");
module_param(error_frame_skip_level, uint, 0664);
MODULE_PARM_DESC(error_frame_skip_level,
				 "\n amvdec_mmpeg12 error_frame_skip_level\n");
module_param(debug_enable, uint, 0664);
MODULE_PARM_DESC(debug_enable,
					 "\n amvdec_mmpeg12 debug enable\n");
module_param(pre_decode_buf_level, int, 0664);
MODULE_PARM_DESC(pre_decode_buf_level,
		"\n ammvdec_mmpeg12 pre_decode_buf_level\n");
module_param(counter_max, int, 0664);
MODULE_PARM_DESC(counter_max,
		"\n ammvdec_mmpeg12 isr timeout max counter,30ms default\n");
module_param(udebug_flag, uint, 0664);
MODULE_PARM_DESC(udebug_flag, "\n amvdec_mmpeg12 udebug_flag\n");

module_init(amvdec_mmpeg12_driver_init_module);
module_exit(amvdec_mmpeg12_driver_remove_module);

MODULE_DESCRIPTION("AMLOGIC MULTI MPEG1/2 Video Decoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");

