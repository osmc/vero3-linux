/*
 * drivers/amlogic/display/osd/osd_sync.h
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


#ifndef _OSD_SYNC_H_
#define _OSD_SYNC_H_

enum {
	GLES_COMPOSE_MODE = 0,
	DIRECT_COMPOSE_MODE = 1,
	GE2D_COMPOSE_MODE = 2,
};

#define FB_SYNC_REQUEST_MAGIC  0x54376812
#define FB_SYNC_REQUEST_RENDER_MAGIC  0x55386816



struct sync_req_old_s {
	unsigned int xoffset;
	unsigned int yoffset;
	int in_fen_fd;
	int out_fen_fd;
};

struct sync_req_s {
	int magic;
	int len;
	unsigned int xoffset;
	unsigned int yoffset;
	int in_fen_fd;
	int out_fen_fd;
	int format;
	int reserved[3];
};


struct sync_req_render_s {
	int magic;
	int len;
	unsigned int    xoffset;
	unsigned int    yoffset;
	int             in_fen_fd;
	int             out_fen_fd;
	int             width;
	int             height;
	int             format;
	int             shared_fd;
	unsigned int    op;
	unsigned int    type; /*direct render or ge2d*/
	unsigned int    dst_x;
	unsigned int    dst_y;
	unsigned int    dst_w;
	unsigned int    dst_h;
	int				byte_stride;
	int				pxiel_stride;
	unsigned int    reserve;
};


struct fb_sync_request_s {
	union {
		struct sync_req_old_s sync_req_old;
		struct sync_req_s sync_req;
		struct sync_req_render_s sync_req_render;
	};
};
#endif
