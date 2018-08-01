/*
 * sound/soc/amlogic/auge/vad.h
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

#ifndef __VAD_H__
#define __VAD_H__

#include "ddr_mngr.h"

enum vad_src {
	VAD_SRC_TDMIN_A,
	VAD_SRC_TDMIN_B,
	VAD_SRC_TDMIN_C,
	VAD_SRC_SPDIFIN,
	VAD_SRC_PDMIN,
	VAD_SRC_LOOPBACK_B,
	VAD_SRC_TDMIN_LB,
	VAD_SRC_LOOPBACK_A,
};

#define DEFAULT_WAKEUP_SAMPLERATE 16000

void vad_update_buffer(bool isvadbuf);
int vad_transfer_chunk_data(unsigned long data, int frames);

bool vad_tdm_is_running(int tdm_idx);
bool vad_pdm_is_running(void);
bool vad_lb_is_running(int lb_id);
void vad_lb_force_two_channel(bool en);

void vad_enable(bool enable);
void vad_set_toddr_info(struct toddr *to);

void vad_set_trunk_data_readable(bool en);

int card_add_vad_kcontrols(struct snd_soc_card *card);

void vad_set_lowerpower_mode(bool islowpower);
#endif
