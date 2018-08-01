/*
 * drivers/amlogic/media/stream_input/parser/stream_buffer_base.h
 *
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
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

#ifndef STREAM_BUFFER_INTERFACE_H
#define STREAM_BUFFER_INTERFACE_H
#include "streambuf.h"
#include "streambuf_reg.h"

#define STBUF_READ(s, func, args...)			\
({							\
	u32 ret = 0;					\
	if ((s) && (s)->ops)				\
		ret = (s)->ops->func((s), ##args);	\
	ret;						\
})

#define STBUF_WRITE(s, func, args...)			\
({							\
	if ((s) && (s)->ops)				\
		(s)->ops->func((s), ##args);		\
})

extern struct stream_buf_ops *get_stbuf_ops(void);
extern struct stream_buf_ops *get_esparser_stbuf_ops(void);
extern struct stream_buf_ops *get_tsparser_stbuf_ops(void);
extern struct stream_buf_ops *get_psparser_stbuf_ops(void);

int stream_buffer_base_init(struct stream_buf_s *stbuf,
			    struct stream_buf_ops *ops,
			    struct parser_args *pars);

void stream_buffer_set_ext_buf(struct stream_buf_s *stbuf,
			       ulong addr,
			       u32 size,
			       u32 flag);

int stream_buffer_write(struct file *file,
			struct stream_buf_s *stbuf,
			const char *buf,
			size_t count);

ssize_t stream_buffer_write_ex(struct file *file,
			   struct stream_buf_s *stbuf,
			   const char __user *buf,
			   size_t count,
			   int flags);

void stream_buffer_meta_write(struct stream_buf_s *stbuf,
	struct stream_buffer_metainfo *meta);

#endif /* STREAM_BUFFER_INTERFACE_H */

