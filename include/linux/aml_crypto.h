/*
 * include/linux/amlogic/aml_crypto.h
 *
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
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

#ifndef _AML_CRYPTO_H_
#define _AML_CRYPTO_H_

#include <linux/types.h>

#ifndef __KERNEL__
#define __user
#endif

#define CRYPTO_OP_ENCRYPT 0
#define CRYPTO_OP_DECRYPT 1

#define MAX_CRYPTO_BUFFERS (32)

struct session_op {
	__u32   cipher;     /* aml_crypto_op_t */
	__u16   keylen;
	__u16   kte;        /* key table entry */
	__u32   ses;        /* session identifier */
};

struct crypt_mem {
	__u32    length;
	__u8    __user *addr;
};

struct crypt_op {
	__u32  ses;          /* session identifier */
	__u8   op;           /* OP_ENCRYPT or OP_DECRYPT */
	__u8   src_phys;     /* set if src is in physical addr */
	__u8   dst_phys;     /* set if dst is in physical addr */
	__u8   ivlen;        /* length of IV */
	__u8   __user *iv;   /* Notice: iv returned from physical is invalid */
	__u8   num_src_bufs;
	__u8   num_dst_bufs;
	__u16  reserved;     /* reserved */
	struct crypt_mem src[MAX_CRYPTO_BUFFERS];   /* source data */
	struct crypt_mem dst[MAX_CRYPTO_BUFFERS];   /* output data */
};

enum aml_crypto_op_t {
	CRYPTO_OP_INVALID = 0,
	CRYPTO_OP_DES_ECB = 1,
	CRYPTO_OP_DES_CBC = 2,
	CRYPTO_OP_TDES_ECB = 3,
	CRYPTO_OP_TDES_CBC = 4,
	CRYPTO_OP_AES_ECB =  5,
	CRYPTO_OP_AES_CBC =  6,
	CRYPTO_OP_AES_CTR =  7,

	CRYPTO_OP_MAX
};

#define CREATE_SESSION     _IOWR('a', 0, struct session_op)
#define CLOSE_SESSION      _IOW('a', 1, __u32)
#define DO_CRYPTO          _IOWR('a', 2, struct crypt_op)

#endif
