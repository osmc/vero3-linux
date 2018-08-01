/*
 * include/linux/amlogic/media/frame_sync/tsync.h
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

#ifndef TSYNC_H
#define TSYNC_H

#define TIME_UNIT90K    (90000)
#define VIDEO_HOLD_THRESHOLD        (TIME_UNIT90K * 3)
#define VIDEO_HOLD_SLOWSYNC_THRESHOLD        (TIME_UNIT90K / 10)
#define AV_DISCONTINUE_THREDHOLD_MIN    (TIME_UNIT90K * 3)
#define AV_DISCONTINUE_THREDHOLD_MAX    (TIME_UNIT90K * 60)

#define TSYNC_IOC_MAGIC 'T'
#define TSYNC_IOC_SET_TUNNEL_MODE  _IOW(TSYNC_IOC_MAGIC, 0x00, int)
#define TSYNC_IOC_SET_VIDEO_PEEK _IOW(TSYNC_IOC_MAGIC, 0x01, int)

#define TSYNC_IOC_GET_FIRST_FRAME_TOGGLED _IOR(TSYNC_IOC_MAGIC, 0x20, int)
#define TSYNC_IOC_SET_FIRST_CHECKIN_APTS _IOW(TSYNC_IOC_MAGIC, 0x02, u32)
#define TSYNC_IOC_SET_LAST_CHECKIN_APTS _IOW(TSYNC_IOC_MAGIC, 0x03, u32)
#define TSYNC_IOC_SET_DEMUX_INFO _IOW(TSYNC_IOC_MAGIC, 0x06, struct dmx_info)
#define TSYNC_IOC_STOP_TSYNC_PCR _IO((TSYNC_IOC_MAGIC), 0x08)
#define AM_ABSSUB(a, b) (((a) >= (b)) ? ((a) - (b)) : ((b) - (a)))
struct dmx_info {
int demux_device_id;
int index;
int vpid;
int apid;
int pcrpid;
};

enum avevent_e {
	VIDEO_START,
	VIDEO_PAUSE,
	VIDEO_STOP,
	VIDEO_TSTAMP_DISCONTINUITY,
	AUDIO_START,
	AUDIO_PAUSE,
	AUDIO_RESUME,
	AUDIO_STOP,
	AUDIO_TSTAMP_DISCONTINUITY,
	AUDIO_PRE_START,
	AUDIO_WAIT
};

enum tsync_mode_e {
	TSYNC_MODE_VMASTER,
	TSYNC_MODE_AMASTER,
	TSYNC_MODE_PCRMASTER,
};

enum tysnc_func_type_e {
	TSYNC_PCRSCR_VALID,
	TSYNC_PCRSCR_GET,
	TSYNC_FIRST_PCRSCR_GET,
	TSYNC_PCRAUDIO_VALID,
	TSYNC_PCRVIDEO_VALID,
	TSYNC_BUF_BY_BYTE,
	TSYNC_STBUF_LEVEL,
	TSYNC_STBUF_SPACE,
	TSYNC_STBUF_SIZE,
	TSYNC_FUNC_TYPE_MAX,
};

extern bool disable_slow_sync;

int demux_get_pcr(int demux_device_index, int index, u64 *stc);

typedef u8 (*pfun_tsdemux_pcrscr_valid)(void);
extern pfun_tsdemux_pcrscr_valid tsdemux_pcrscr_valid_cb;

typedef u32 (*pfun_tsdemux_pcrscr_get)(void);
extern pfun_tsdemux_pcrscr_get tsdemux_pcrscr_get_cb;

typedef int (*pfun_amldemux_pcrscr_get)(int demux_device_index, int index,
					u64 *stc);
extern pfun_amldemux_pcrscr_get amldemux_pcrscr_get_cb;

typedef u32 (*pfun_tsdemux_first_pcrscr_get)(void);
extern pfun_tsdemux_first_pcrscr_get tsdemux_first_pcrscr_get_cb;

typedef u8 (*pfun_tsdemux_pcraudio_valid)(void);
extern pfun_tsdemux_pcraudio_valid tsdemux_pcraudio_valid_cb;

typedef u8 (*pfun_tsdemux_pcrvideo_valid)(void);
extern pfun_tsdemux_pcrvideo_valid tsdemux_pcrvideo_valid_cb;

typedef struct stream_buf_s *(*pfun_get_buf_by_type)(u32 type);
extern pfun_get_buf_by_type get_buf_by_type_cb;

typedef u32 (*pfun_stbuf_level)(struct stream_buf_s *buf);
extern pfun_stbuf_level stbuf_level_cb;

typedef u32 (*pfun_stbuf_space)(struct stream_buf_s *buf);
extern pfun_stbuf_space stbuf_space_cb;

typedef u32 (*pfun_stbuf_size)(struct stream_buf_s *buf);
extern pfun_stbuf_size stbuf_size_cb;

extern int register_tsync_callbackfunc(
	enum tysnc_func_type_e ntype, void *pfunc);

#ifdef MODIFY_TIMESTAMP_INC_WITH_PLL
extern void set_timestamp_inc_factor(u32 factor);
#endif

#ifdef CALC_CACHED_TIME
extern int pts_cached_time(u8 type);
#endif

u32 get_first_pic_coming(void);

extern int get_vsync_pts_inc_mode(void);

extern void tsync_avevent_locked(enum avevent_e event, u32 param);

extern void tsync_mode_reinit(void);

extern void tsync_avevent(enum avevent_e event, u32 param);

extern void tsync_audio_break(int audio_break);

extern void tsync_trick_mode(int trick_mode);

extern void tsync_set_avthresh(unsigned int av_thresh);

extern void tsync_set_syncthresh(unsigned int sync_thresh);

extern void tsync_set_dec_reset(void);

extern void tsync_set_enable(int enable);

extern int tsync_get_mode(void);

extern int tsync_get_sync_adiscont(void);

extern int tsync_get_sync_vdiscont(void);

extern void tsync_set_sync_adiscont(int syncdiscont);

extern void tsync_set_sync_vdiscont(int syncdiscont);

extern u32 tsync_get_sync_adiscont_diff(void);

extern u32 tsync_get_sync_vdiscont_diff(void);

extern void tsync_set_sync_adiscont_diff(u32 discontinue_diff);

extern void tsync_set_sync_vdiscont_diff(u32 discontinue_diff);
extern int tsync_set_apts(unsigned int pts);

extern void tsync_init(void);

extern void tsync_set_automute_on(int automute_on);

extern int tsync_get_debug_pts_checkin(void);

extern int tsync_get_debug_pts_checkout(void);

extern int tsync_get_debug_vpts(void);

extern int tsync_get_debug_apts(void);
extern int tsync_get_av_threshold_min(void);

extern int tsync_get_av_threshold_max(void);

extern int tsync_set_av_threshold_min(int min);

extern int tsync_set_av_threshold_max(int max);

extern void set_pts_realign(void);

extern int tsync_set_tunnel_mode(int mode);

extern int tsync_get_tunnel_mode(void);

extern void timestamp_set_pcrlatency(u32 latency);
extern u32 timestamp_get_pcrlatency(void);
extern bool tsync_check_vpts_discontinuity(unsigned int vpts);
int tsync_get_vpts_error_num(void);
int tsync_get_apts_error_num(void);
extern void timestamp_clac_pts_latency(u8 type, u32 pts);
extern u32 timestamp_get_pts_latency(u8 type);
extern void timestamp_clean_pts_latency(u8 type);
extern int tsync_get_vpts_adjust(void);
extern void set_video_peek(void);
extern u32 get_first_frame_toggled(void);

void tsync_set_av_state(u8 type, int state);

u8 tsync_get_demux_pcrscr_valid_for_newarch(void);

u8 tsync_get_demux_pcrscr_valid(void);

void tsync_get_demux_pcr_for_newarch(void);

u8 tsync_get_demux_pcr(u32 *pcr);

void tsync_get_first_demux_pcr_for_newarch(void);

u8 tsync_get_first_demux_pcr(u32 *first_pcr);

u8 tsync_get_audio_pid_valid_for_newarch(void);

u8 tsync_get_audio_pid_valid(void);

u8 tsync_get_video_pid_valid_for_newarch(void);

u8 tsync_get_video_pid_valid(void);

void tsync_get_buf_by_type_for_newarch(u8 type, struct stream_buf_s *pbuf);

u8 tsync_get_buf_by_type(u8 type, struct stream_buf_s *pbuf);

void tsync_get_stbuf_level_for_newarch(struct stream_buf_s *pbuf,
				       u32 *buf_level);

u8 tsync_get_stbuf_level(struct stream_buf_s *pbuf, u32 *buf_level);

void tsync_get_stbuf_space_for_newarch(struct stream_buf_s *pbuf,
				       u32 *buf_space);

u8 tsync_get_stbuf_space(struct stream_buf_s *pbuf, u32 *buf_space);

void tsync_get_stbuf_size_for_newarch(struct stream_buf_s *pbuf, u32 *buf_size);

u8 tsync_get_stbuf_size(struct stream_buf_s *pbuf, u32 *buf_size);

bool tsync_get_new_arch(void);

u32 tsync_get_checkin_apts(void);

void tsync_reset(void);

static inline u32 tsync_vpts_discontinuity_margin(void)
{
	return tsync_get_av_threshold_min();
}

#endif /* TSYNC_H */
