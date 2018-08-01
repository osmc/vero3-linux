/*
 * drivers/amlogic/mailbox/scpi_protocol.c
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
#include <linux/err.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/mailbox_client.h>
#include <linux/slab.h>
#include <linux/amlogic/scpi_protocol.h>

#include "meson_mhu.h"
#include "meson_mhu_fifo.h"
#include "meson_mhu_pl.h"

#define SYNC_CMD_TAG			BIT(26)
#define ASYNC_CMD_TAG			BIT(25)

#define SCPI_DEF_SYNC			SYNC_CMD_TAG

#define MBSIZE_SHIFT			16
#define MBSIZE_MASK			0x1ff
#define MBCMD_MASK			0xffff
#define ACK_OK				0x1
#define ACK_FAIL			0x2

struct device *mhu_device;
struct device *mhu_fifo_device;
struct device *mhu_pl_device;
struct device *mhu_sec_device;
u32 mhu_f;

#define CMD_ID_SHIFT		0
#define CMD_ID_MASK		0xff
#define CMD_SENDER_ID_SHIFT	8
#define CMD_SENDER_ID_MASK	0xff
#define CMD_DATA_SIZE_SHIFT	20
#define CMD_DATA_SIZE_MASK	0x1ff
#define PACK_SCPI_CMD(cmd, sender, txsz)				\
	((((cmd) & CMD_ID_MASK) << CMD_ID_SHIFT) |			\
	(((sender) & CMD_SENDER_ID_MASK) << CMD_SENDER_ID_SHIFT) |	\
	(((txsz) & CMD_DATA_SIZE_MASK) << CMD_DATA_SIZE_SHIFT))

#define MAX_DVFS_DOMAINS	3
#define MAX_DVFS_OPPS		16
#define DVFS_LATENCY(hdr)	((hdr) >> 16)
#define DVFS_OPP_COUNT(hdr)	(((hdr) >> 8) & 0xff)

enum scpi_error_codes {
	SCPI_SUCCESS = 0, /* Success */
	SCPI_ERR_PARAM = 1, /* Invalid parameter(s) */
	SCPI_ERR_ALIGN = 2, /* Invalid alignment */
	SCPI_ERR_SIZE = 3, /* Invalid size */
	SCPI_ERR_HANDLER = 4, /* Invalid handler/callback */
	SCPI_ERR_ACCESS = 5, /* Invalid access/permission denied */
	SCPI_ERR_RANGE = 6, /* Value out of range */
	SCPI_ERR_TIMEOUT = 7, /* Timeout has occurred */
	SCPI_ERR_NOMEM = 8, /* Invalid memory area or pointer */
	SCPI_ERR_PWRSTATE = 9, /* Invalid power state */
	SCPI_ERR_SUPPORT = 10, /* Not supported or disabled */
	SCPI_ERR_DEVICE = 11, /* Device error */
	SCPI_ERR_MAX
};

static int scpi_freq_map_table[] = {
	0,
	0,
	1200,
	1300,
	1400,
	1500,
	1600,
	1700,
	1800,
	1900,
	2000,
	2100,
	2200,
	2300,
	2400,
	0
};
static int scpi_volt_map_table[] = {
	0,
	0,
	900,
	910,
	920,
	930,
	940,
	950,
	960,
	970,
	980,
	990,
	1000,
	1010,
	1020,
	0
};

struct scpi_data_buf {
	int client_id;
	struct mhu_data_buf *data;
	struct completion complete;
	int async;
	int channel;
};

static int high_priority_cmds[] = {
	SCPI_CMD_GET_CSS_PWR_STATE,
	SCPI_CMD_CFG_PWR_STATE_STAT,
	SCPI_CMD_GET_PWR_STATE_STAT,
	SCPI_CMD_SET_DVFS,
	SCPI_CMD_GET_DVFS,
	SCPI_CMD_SET_RTC,
	SCPI_CMD_GET_RTC,
	SCPI_CMD_SET_CLOCK_INDEX,
	SCPI_CMD_SET_CLOCK_VALUE,
	SCPI_CMD_GET_CLOCK_VALUE,
	SCPI_CMD_SET_PSU,
	SCPI_CMD_GET_PSU,
	SCPI_CMD_SENSOR_CFG_PERIODIC,
	SCPI_CMD_SENSOR_CFG_BOUNDS,
	SCPI_CMD_WAKEUP_REASON_GET,
	SCPI_CMD_WAKEUP_REASON_CLR,
	SCPI_CMD_INIT_DSP,
};

static int bl4_cmds[] = {
	SCPI_CMD_BL4_SEND,
	SCPI_CMD_BL4_LISTEN,
};

enum c_chan_t {
	C_DSPA_FIFO = 0, /* to dspa */
	C_DSPB_FIFO = 1, /* to dspb */
	C_AOCPU_FIFO = 2, /* to aocpu */
	C_SECPU_FIFO = 3, /* to secpu core */
	C_DSPA_PL = 4, /* to dspa core */
	C_DSPB_PL = 5, /* to dspb core */
	C_AOCPU_PL = 6, /* to aocpu core */
	C_SECPU_PL = 7, /* to secpu core */
	C_AOCPU_OLD = 8, /* to aocpu core */
	C_MAXNUM,
};

static struct scpi_dvfs_info *scpi_opps[MAX_DVFS_DOMAINS];

static int scpi_linux_errmap[SCPI_ERR_MAX] = {
	0, -EINVAL, -ENOEXEC, -EMSGSIZE,
	-EINVAL, -EACCES, -ERANGE, -ETIMEDOUT,
	-ENOMEM, -EINVAL, -EOPNOTSUPP, -EIO,
};

static inline int scpi_to_linux_errno(int errno)
{
	if (errno >= SCPI_SUCCESS && errno < SCPI_ERR_MAX)
		return scpi_linux_errmap[errno];
	return -EIO;
}

static int scpi_get_c_by_name(struct device *dev, int num, char *inname)
{
	int index = 0;
	int ret = 0;
	const char *name = NULL;

	for (index = 0; index < num; index++) {
		ret = of_property_read_string_index(dev->of_node,
						    "mbox-names",
						     index, &name);
		if (ret) {
			dev_err(dev, "%s get mbox[%d] name fail\n",
				__func__, index);
		} else {
			if (!strncmp(name, inname, strlen(inname)))
				return index;
		}
	}
	return -ENODEV;
}

static int scpi_get_chan(int channel, struct device *np)
{
	struct device *dev = np;
	int nums = 0;

	if (!dev) {
		pr_err("scpi mhu node null\n");
		return -ENXIO;
	}

	of_property_read_u32(dev->of_node,
			     "mbox-nums", &nums);
	if (nums == 0 || nums > CHANNEL_FIFO_MAX) {
		pr_err("scpi get mbox node fail: nums:%d\n", nums);
		return -ENXIO;
	}

	switch (channel) {
	case SCPI_DSPA:
		return scpi_get_c_by_name(dev, nums, "ap_to_dspa");
	case SCPI_DSPB:
		return scpi_get_c_by_name(dev, nums, "ap_to_dspb");
	case SCPI_AOCPU:
		return scpi_get_c_by_name(dev, nums, "ap_to_ao");
	case SCPI_SECPU:
		return scpi_get_c_by_name(dev, nums, "ap_to_se");
	default:
		return -ENXIO;
	}

	return -ENXIO;
}

static bool high_priority_chan_supported(int cmd)
{
	unsigned int idx;

	if (num_scp_chans == CHANNEL_MAX) {
		for (idx = 0; idx < ARRAY_SIZE(high_priority_cmds); idx++)
			if (cmd == high_priority_cmds[idx])
				return true;
	} else {
		for (idx = 0; idx < ARRAY_SIZE(bl4_cmds); idx++)
			if (cmd == bl4_cmds[idx])
				return true;
	}
	return false;
}

static void scpi_rx_callback(struct mbox_client *cl, void *msg)
{
	struct mhu_data_buf *data = (struct mhu_data_buf *)msg;
	struct scpi_data_buf *scpi_buf = data->cl_data;

	complete(&scpi_buf->complete);
}

static int send_scpi_cmd(struct scpi_data_buf *scpi_buf,
			 bool high_priority, int c_chan)
{
	struct mbox_chan *chan;
	struct mbox_client cl = {0};
	struct mhu_data_buf *data = scpi_buf->data;
	u32 status = 0;
	int chan_idx =  0;
	char *txbuf = NULL;
	char *rxbuf = NULL;
	char *rx_buf = NULL;
	unsigned long wait;
	int ret;
	int plhead_len = 0;

	switch (c_chan) {
	case C_DSPA_FIFO:
	case C_DSPB_FIFO:
	case C_AOCPU_FIFO:
		if (!mhu_fifo_device)
			return -ENOENT;

		cl.dev = mhu_fifo_device;
		chan_idx = scpi_get_chan(scpi_buf->channel, mhu_fifo_device);
		if (chan_idx < 0)
			return -ENOENT;

		if (data->tx_size != 0) {
			int txbuf_size = data->tx_size + MBOX_HEAD_SIZE;
			int rxbuf_size = data->rx_size + MBOX_HEAD_SIZE;

			txbuf = kmalloc(txbuf_size, GFP_KERNEL);
			if (!txbuf) {
				status = -SCPI_ERR_NOMEM;
				goto free_end;
			}
			memset(txbuf, 0, txbuf_size);

			rxbuf = kmalloc(rxbuf_size, GFP_KERNEL);
			if (!rxbuf) {
				status = -SCPI_ERR_NOMEM;
				goto free_buf;
			}
			memset(rxbuf, 0, rxbuf_size);
			memcpy(txbuf + MBOX_HEAD_SIZE,
			       data->tx_buf, data->tx_size);

			data->tx_buf = txbuf;
			data->tx_size = txbuf_size;
			rx_buf = data->rx_buf;
			data->rx_buf = rxbuf;
			data->rx_size = rxbuf_size;
		}
	break;
	case C_DSPA_PL:
	case C_DSPB_PL:
	case C_SECPU_PL:
		if (!mhu_pl_device)
			return -ENOENT;

		cl.dev = mhu_pl_device;
		chan_idx = scpi_get_chan(scpi_buf->channel, mhu_pl_device);
		if (chan_idx < 0) {
			pr_err("scpi get chan 0x%x fail\n", scpi_buf->channel);
			return -ENOENT;
		}

		if (c_chan == C_SECPU_PL)
			plhead_len = MBOX_PL_HEAD_SIZE;
		else
			plhead_len = MBOX_RESERVE_LEN;

		if (data->tx_size != 0) {
			int txbuf_size = data->tx_size + plhead_len;
			int rxbuf_size = data->rx_size + plhead_len;

			txbuf = kmalloc(txbuf_size, GFP_KERNEL);
			if (!txbuf) {
				status = -SCPI_ERR_NOMEM;
				goto free_end;
			}
			memset(txbuf, 0, txbuf_size);

			rxbuf = kmalloc(rxbuf_size, GFP_KERNEL);
			if (!rxbuf) {
				status = -SCPI_ERR_NOMEM;
				goto free_buf;
			}
			memset(rxbuf, 0, rxbuf_size);
			memcpy(txbuf + plhead_len,
			       data->tx_buf, data->tx_size);

			data->tx_buf = txbuf;
			data->tx_size = txbuf_size;
			rx_buf = data->rx_buf;
			data->rx_buf = rxbuf;
			data->rx_size = rxbuf_size;
		}
	break;
	case C_AOCPU_OLD:
		if (!mhu_device)
			return -ENOENT;

		cl.dev = mhu_device;
		if (send_listen_chans) {
			chan_idx = 31 - __builtin_clz(send_listen_chans);
			if (!high_priority)
				chan_idx = 31 -
			__builtin_clz(send_listen_chans ^ BIT(chan_idx));
		} else {
			chan_idx = high_priority;
		}
	break;
	default:
		pr_err("[scpi]: match mhu driver fail\n");
		return -ENOENT;
	break;

	}

	cl.rx_callback = scpi_rx_callback;
	chan = mbox_request_channel(&cl, chan_idx);
	if (IS_ERR(chan)) {
		status = -SCPI_ERR_NOMEM;
		goto free_buf;
	}

	init_completion(&scpi_buf->complete);
	if (mbox_send_message(chan, (void *)data) < 0) {
		status = SCPI_ERR_TIMEOUT;
		goto free_channel;
	}

	wait = msecs_to_jiffies(MBOX_TIME_OUT);
	ret = wait_for_completion_timeout(&scpi_buf->complete, wait);
	if (ret == 0) {
		pr_err("Warning: scpi wait ack time out %d\n",
		       ret);
		status = SCPI_ERR_TIMEOUT;
		goto free_channel;
	}

	switch (c_chan) {
	case C_DSPA_FIFO:
	case C_DSPB_FIFO:
		status = *(u32 *)(data->rx_buf);
		if (status == ACK_OK) {
			memcpy(rx_buf, (data->rx_buf) + MBOX_HEAD_SIZE,
			       (data->rx_size - MBOX_HEAD_SIZE));

			data->rx_buf = rx_buf;
		}

		data->rx_buf = rx_buf;

		status = SCPI_SUCCESS;
	break;
	case C_AOCPU_FIFO:
		status = *(u32 *)(data->rx_buf);
		if (status == ACK_OK) {
			*(u32 *)(rx_buf) = SCPI_SUCCESS;
		memcpy(rx_buf + sizeof(status),
		       (data->rx_buf) + MBOX_HEAD_SIZE,
		       (data->rx_size - MBOX_HEAD_SIZE - sizeof(status)));
		} else {
			*(u32 *)(rx_buf) = SCPI_ERR_SUPPORT;
		}

		data->rx_buf = rx_buf;
		status = *(u32 *)(data->rx_buf);
	break;
	case C_DSPA_PL:
	case C_DSPB_PL:
	case C_SECPU_PL:
		status = *(u32 *)(data->rx_buf);
		if (c_chan == C_SECPU_PL) {
			plhead_len = MBOX_PL_HEAD_SIZE;
			status = *(u32 *)(data->rx_buf);
		} else {
			plhead_len = MBOX_RESERVE_LEN;
			status = ACK_OK;
		}

		if (status == ACK_OK) {
			/*need to check*/
			memcpy(rx_buf, (data->rx_buf) + plhead_len,
			       (data->rx_size - plhead_len));

			data->rx_buf = rx_buf;
		}

		data->rx_buf = rx_buf;

		status = SCPI_SUCCESS;
	break;
	case C_AOCPU_OLD:
		status = *(u32 *)(data->rx_buf); /* read first word */
	break;
	}

free_channel:
	mbox_free_channel(chan);

free_buf:
	kfree(rxbuf);
	kfree(txbuf);

free_end:
	return scpi_to_linux_errno(status);
}

#define SCPI_SETUP_DBUF(scpi_buf, mhu_buf, _client_id,\
			_cmd, _tx_buf, _rx_buf) \
do {						\
	struct mhu_data_buf *pdata = &mhu_buf;	\
	pdata->cmd = _cmd;			\
	pdata->tx_buf = &_tx_buf;		\
	pdata->tx_size = sizeof(_tx_buf);	\
	pdata->rx_buf = &_rx_buf;		\
	pdata->rx_size = sizeof(_rx_buf);	\
	scpi_buf.client_id = _client_id;	\
	scpi_buf.data = pdata;			\
	scpi_buf.async = SYNC_CMD_TAG;		\
	scpi_buf.channel = SCPI_MAXNUM;		\
} while (0)

#define SCPI_SETUP_DBUF_SIZE(scpi_buf, mhu_buf, _client_id,\
			_cmd, _tx_buf, _tx_size, _rx_buf, _rx_size) \
do {						\
	struct mhu_data_buf *pdata = &mhu_buf;	\
	pdata->cmd = _cmd;			\
	pdata->tx_buf = _tx_buf;		\
	pdata->tx_size = _tx_size;	\
	pdata->rx_buf = _rx_buf;		\
	pdata->rx_size = _rx_size;	\
	scpi_buf.client_id = _client_id;	\
	scpi_buf.data = pdata;			\
	scpi_buf.async = SYNC_CMD_TAG;		\
	scpi_buf.channel = SCPI_MAXNUM;		\
} while (0)

#define SCPI_SETUP_DBUF_AUTO(scpi_buf, mhu_buf, _client_id,\
			_cmd, _tx_buf, _tx_size, _rx_buf, _rx_size,\
			_async, _channel) \
do {						\
	struct mhu_data_buf *pdata = &(mhu_buf);\
	struct scpi_data_buf *psdata = &(scpi_buf); \
	pdata->cmd = _cmd;			\
	pdata->tx_buf = _tx_buf;		\
	pdata->tx_size = _tx_size;		\
	pdata->rx_buf = _rx_buf;		\
	pdata->rx_size = _rx_size;		\
	psdata->client_id = _client_id;		\
	psdata->data = pdata;			\
	psdata->async = _async;			\
	psdata->channel = _channel;		\
} while (0)

static int scpi_execute_cmd(struct scpi_data_buf *scpi_buf)
{
	struct mhu_data_buf *data;
	bool high_priority = 0;
	int client_id = scpi_buf->client_id;
	int channel = scpi_buf->channel;
	int c_chan = C_MAXNUM;
	int txsize = 0;

	data = scpi_buf->data;

	if (client_id != SCPI_MAX)
		channel = SCPI_AOCPU;

	switch (channel) {
	case SCPI_DSPA:
	case SCPI_DSPB:
		if (mhu_f & MASK_MHU_FIFO) {
			if (channel == SCPI_DSPA)
				c_chan = C_DSPA_FIFO;
			else
				c_chan = C_DSPB_FIFO;
			txsize = data->tx_size + MBOX_HEAD_SIZE;

			if (data->tx_size > MBOX_DATA_SIZE)
				return -EINVAL;
		} else {
			if (mhu_f & MASK_MHU_PL) {
				if (channel == SCPI_DSPA)
					c_chan = C_DSPA_PL;
				else
					c_chan = C_DSPB_PL;

				if (data->tx_size > MBOX_PL_SCPITODSP_SIZE)
					return -EINVAL;

				txsize = data->tx_size + MBOX_RESERVE_LEN;
			} else {
				return -EINVAL;
			}
		}

		data->cmd = (data->cmd & 0xffff)
			    | (txsize & MBSIZE_MASK) << MBSIZE_SHIFT
			    | (scpi_buf->async); //Sync Flag
	break;
	case SCPI_AOCPU:
		if (mhu_f & MASK_MHU) {
			c_chan = C_AOCPU_OLD;
			scpi_buf->channel = SCPI_AOCPU;

			if (scpi_buf->async != ASYNC_CMD_TAG &&
			    scpi_buf->async != SYNC_CMD_TAG)
				scpi_buf->async = SCPI_DEF_SYNC;

			high_priority = high_priority_chan_supported(data->cmd);
			if (!high_priority || num_scp_chans == CHANNEL_MAX)
				data->cmd = PACK_SCPI_CMD(data->cmd,
							  scpi_buf->client_id,
							  data->tx_size);
			else if (high_priority &&
				 (num_scp_chans != CHANNEL_MAX))
				data->cmd = data->tx_size;
			else
				return -EINVAL;
		} else {
			if (mhu_f & MASK_MHU_FIFO) {
				if (data->tx_size > MBOX_DATA_SIZE)
					return -EINVAL;

				c_chan = C_AOCPU_FIFO;
				scpi_buf->channel = SCPI_AOCPU;
				txsize = data->tx_size + MBOX_HEAD_SIZE;
				if (scpi_buf->async != ASYNC_CMD_TAG &&
				    scpi_buf->async != SYNC_CMD_TAG)
					scpi_buf->async = SCPI_DEF_SYNC;

				data->cmd =
				(data->cmd & MBCMD_MASK)
				| (txsize & MBSIZE_MASK) << MBSIZE_SHIFT
				| (scpi_buf->async); //Sync Flag
			} else {
				return -EINVAL;
			}
		}
	break;
	case SCPI_SECPU:
		if (mhu_f & MASK_MHU_PL) {
			if (data->tx_size > MBOX_PL_SCPITOSE_SIZE)
				return -EINVAL;
			c_chan = C_SECPU_PL;
			txsize = data->tx_size + MBOX_PL_HEAD_SIZE;
			data->cmd = (data->cmd & MBCMD_MASK)
				    | (txsize & MBSIZE_MASK) << MBSIZE_SHIFT
				    | (scpi_buf->async); //Sync Flag
		} else {
			return -EINVAL;
		}
	break;
	default:
		break;
	}

	data->cl_data = scpi_buf;

	return send_scpi_cmd(scpi_buf, high_priority, c_chan);
}

/*old scpi api, cannot add more*/
unsigned long scpi_clk_get_val(u16 clk_id)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u32 status;
		u32 clk_rate;
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_CLOCKS,
			SCPI_CMD_GET_CLOCK_VALUE, clk_id, buf);
	if (scpi_execute_cmd(&sdata))
		return 0;

	return buf.clk_rate;
}
EXPORT_SYMBOL_GPL(scpi_clk_get_val);

int scpi_clk_set_val(u16 clk_id, unsigned long rate)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	int stat;
	struct __packed {
		u32 clk_rate;
		u16 clk_id;
	} buf;

	buf.clk_rate = (u32)rate;
	buf.clk_id = clk_id;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_CLOCKS,
			SCPI_CMD_SET_CLOCK_VALUE, buf, stat);
	return scpi_execute_cmd(&sdata);
}
EXPORT_SYMBOL_GPL(scpi_clk_set_val);

struct scpi_dvfs_info *scpi_dvfs_get_opps(u8 domain)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u32 status;
		u32 header;
		struct scpi_opp_entry opp[MAX_DVFS_OPPS];
	} buf;
	struct scpi_dvfs_info *opps;
	size_t opps_sz;
	int count, ret;

	if (domain >= MAX_DVFS_DOMAINS)
		return ERR_PTR(-EINVAL);

	if (scpi_opps[domain])	/* data already populated */
		return scpi_opps[domain];

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_DVFS,
			SCPI_CMD_GET_DVFS_INFO, domain, buf);
	ret = scpi_execute_cmd(&sdata);
	if (ret)
		return ERR_PTR(ret);

	opps = kmalloc(sizeof(*opps), GFP_KERNEL);
	if (!opps)
		return ERR_PTR(-ENOMEM);

	count = DVFS_OPP_COUNT(buf.header);
	opps_sz = count * sizeof(*opps->opp);

	opps->count = count;
	opps->latency = DVFS_LATENCY(buf.header);
	opps->opp = kmalloc(opps_sz, GFP_KERNEL);
	if (!opps->opp) {
		kfree(opps);
		return ERR_PTR(-ENOMEM);
	}

	memcpy(opps->opp, &buf.opp[0], opps_sz);
	scpi_opps[domain] = opps;

	return opps;
}
EXPORT_SYMBOL_GPL(scpi_dvfs_get_opps);

int scpi_dvfs_get_idx(u8 domain)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u32 status;
		u8 dvfs_idx;
	} buf;
	int ret;

	if (domain >= MAX_DVFS_DOMAINS)
		return -EINVAL;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_DVFS,
			SCPI_CMD_GET_DVFS, domain, buf);
	ret = scpi_execute_cmd(&sdata);

	if (!ret)
		ret = buf.dvfs_idx;
	return ret;
}
EXPORT_SYMBOL_GPL(scpi_dvfs_get_idx);

int scpi_dvfs_set_idx(u8 domain, u8 idx)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u8 dvfs_domain;
		u8 dvfs_idx;
	} buf;
	int stat;

	buf.dvfs_idx = idx;
	buf.dvfs_domain = domain;

	if (domain >= MAX_DVFS_DOMAINS)
		return -EINVAL;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_DVFS,
			SCPI_CMD_SET_DVFS, buf, stat);
	return scpi_execute_cmd(&sdata);
}
EXPORT_SYMBOL_GPL(scpi_dvfs_set_idx);

int scpi_get_sensor(char *name)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u32 status;
		u16 sensors;
	} cap_buf;
	struct __packed {
		u32 status;
		u16 sensor;
		u8 class;
		u8 trigger;
		char name[20];
	} info_buf;
	int ret;
	u16 sensor_id;

	/* This should be handled by a generic macro */
	do {
		struct mhu_data_buf *pdata = &mdata;

		pdata->cmd = SCPI_CMD_SENSOR_CAPABILITIES;
		pdata->tx_size = 0;
		pdata->rx_buf = &cap_buf;
		pdata->rx_size = sizeof(cap_buf);
		sdata.client_id = SCPI_CL_THERMAL;
		sdata.data = pdata;
		sdata.channel = SCPI_MAXNUM;
	} while (0);
	ret = scpi_execute_cmd(&sdata);
	if (ret)
		goto out;

	ret = -ENODEV;
	for (sensor_id = 0; sensor_id < cap_buf.sensors; sensor_id++) {
		SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_THERMAL,
				SCPI_CMD_SENSOR_INFO, sensor_id, info_buf);
		ret = scpi_execute_cmd(&sdata);
		if (ret)
			break;

		if (!strcmp(name, info_buf.name)) {
			ret = sensor_id;
			break;
		}
	}
out:
	return ret;
}
EXPORT_SYMBOL_GPL(scpi_get_sensor);

int scpi_get_sensor_value(u16 sensor, u32 *val)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u32 status;
		u32 val;
	} buf;
	int ret;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_THERMAL, SCPI_CMD_SENSOR_VALUE,
			sensor, buf);
	ret = scpi_execute_cmd(&sdata);
	if (ret == 0)
		*val = buf.val;

	return ret;
}
EXPORT_SYMBOL_GPL(scpi_get_sensor_value);

/****Send fail when data size > 0x1fd.      ***
 * Because of USER_LOW_TASK_SHARE_MEM_BASE ***
 * size limitation.
 * You can call scpi_send_usr_data()
 * multi-times when your data is bigger
 * than 0x1fe
 */
int scpi_send_usr_data(u32 client_id, void *val, u32 size)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u32 status;
		u32 val;
	} buf;
	int ret;

	/*client_id bit map should locates @ 0xff.
	 * bl30 will send client_id via half-Word
	 */
	if (client_id & ~0xff)
		return -E2BIG;

	/*Check size here because of USER_LOW_TASK_SHARE_MEM_BASE
	 * size limitation, and first Word is used as command,
	 * second word is used as tx_size.
	 */
	if (size > 0x1fd)
		return -EPERM;

	SCPI_SETUP_DBUF_SIZE(sdata, mdata, client_id, SCPI_CMD_SET_USR_DATA,
			val, size, &buf, sizeof(buf));
	ret = scpi_execute_cmd(&sdata);

	return ret;
}
EXPORT_SYMBOL_GPL(scpi_send_usr_data);

int scpi_get_vrtc(u32 *p_vrtc)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u32 temp = 0;
	struct __packed {
		u32 status;
		u32 vrtc;
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			SCPI_CMD_GET_RTC, temp, buf);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	*p_vrtc = buf.vrtc;

	return 0;
}
EXPORT_SYMBOL_GPL(scpi_get_vrtc);

int scpi_set_vrtc(u32 vrtc_val)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	int state;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			SCPI_CMD_SET_RTC, vrtc_val, state);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	return 0;
}
EXPORT_SYMBOL_GPL(scpi_set_vrtc);

int scpi_get_ring_value(unsigned char *val)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		unsigned int status;
		unsigned char ringinfo[8];
	} buf;
	int ret;
	u32 temp = 0;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE, SCPI_CMD_OSCRING_VALUE,
			temp, buf);
	ret = scpi_execute_cmd(&sdata);
	if (ret == 0)
		memcpy(val, &buf.ringinfo, sizeof(buf.ringinfo));
	return ret;
}
EXPORT_SYMBOL_GPL(scpi_get_ring_value);

int scpi_get_wakeup_reason(u32 *wakeup_reason)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u32 temp = 0;
	struct __packed {
		u32 status;
		u32 reason;
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			SCPI_CMD_WAKEUP_REASON_GET, temp, buf);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	*wakeup_reason = buf.reason;

	return 0;
}
EXPORT_SYMBOL_GPL(scpi_get_wakeup_reason);

int scpi_clr_wakeup_reason(void)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u32 temp = 0, state;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			SCPI_CMD_WAKEUP_REASON_CLR, temp, state);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	return 0;
}
EXPORT_SYMBOL_GPL(scpi_clr_wakeup_reason);

int scpi_init_dsp_cfg0(u32 id, u32 addr, u32 cfg0)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u32 temp = 0;
	struct __packed {
		u32 id;
		u32 addr;
		u32 cfg0;
	} buf;
	buf.id = id;
	buf.addr = addr;
	buf.cfg0 = cfg0;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			SCPI_CMD_INIT_DSP, buf, temp);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	return 0;
}
EXPORT_SYMBOL_GPL(scpi_init_dsp_cfg0);

int scpi_set_cec_val(enum scpi_std_cmd index, u32 cec_data)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	int status;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			index, cec_data, status);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	return 0;
}
EXPORT_SYMBOL_GPL(scpi_set_cec_val);

int scpi_get_cec_val(enum scpi_std_cmd index, u32 *p_cec)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u32 temp = 0;
	struct __packed {
		u32 status;
		u32 cec_val;
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			index, temp, buf);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	*p_cec = buf.cec_val;
	return 0;

}
EXPORT_SYMBOL_GPL(scpi_get_cec_val);

/****Send fail when data size > 0x20.      ***
 * Because of USER_LOW_TASK_SHARE_MEM_BASE ***
 * size limitation.
 * You can call scpi_send_usr_data()
 * multi-times when your data is bigger
 * than 0x20
 */
int scpi_send_cec_data(u32 cmd_id, void *val, u32 size)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	struct __packed {
		u32 status;
		u32 val;
	} buf;
	int ret;

	/*Check size here because of USER_LOW_TASK_SHARE_MEM_BASE
	 * size limitation, and first Word is used as command,
	 * second word is used as tx_size.
	 */
	if (size > 0x1fd)
		return -EPERM;

	SCPI_SETUP_DBUF_SIZE(sdata, mdata, SCPI_CL_NONE, cmd_id,
			     val, size, &buf, sizeof(buf));
	ret = scpi_execute_cmd(&sdata);

	return ret;
}
EXPORT_SYMBOL_GPL(scpi_send_cec_data);

int scpi_get_cec_wk_msg(enum scpi_std_cmd index, unsigned char *cec_msg)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u32 temp = 0;
	struct __packed {
		u32 status;
		unsigned char msg_len;
		unsigned char cec_msg[16];
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			index, temp, buf);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	memcpy(cec_msg, &buf.msg_len,
		sizeof(buf.msg_len) + sizeof(buf.cec_msg));
	return 0;
}
EXPORT_SYMBOL_GPL(scpi_get_cec_wk_msg);

u8 scpi_get_ethernet_calc(void)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u8 temp = 0;

	struct __packed {
		u32 status;
		u8 eth_calc;
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
		SCPI_CMD_GET_ETHERNET_CALC, temp, buf);
	if (scpi_execute_cmd(&sdata))
		return 0;
	return buf.eth_calc;
}
EXPORT_SYMBOL_GPL(scpi_get_ethernet_calc);

int scpi_get_cpuinfo(enum scpi_get_pfm_type type, u32 *freq, u32 *vol)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u8 index = 0;
	int ret;

	struct __packed {
		u32 status;
		u8 pfm_info[4];
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
		SCPI_CMD_GET_CPUINFO, index, buf);
	if (scpi_execute_cmd(&sdata))
		return -EPERM;

	switch (type) {
	case SCPI_CPUINFO_VERSION:
		ret = buf.pfm_info[0];
		break;
	case SCPI_CPUINFO_CLUSTER0:
		index = (buf.pfm_info[1] >> 4) & 0xf;
		*freq = scpi_freq_map_table[index];
		index = buf.pfm_info[1] & 0xf;
		*vol = scpi_volt_map_table[index];
		ret = 0;
		break;
	case SCPI_CPUINFO_CLUSTER1:
		index = (buf.pfm_info[2] >> 4) & 0xf;
		*freq = scpi_freq_map_table[index];
		index = buf.pfm_info[2] & 0xf;
		*vol = scpi_volt_map_table[index];
		ret = 0;
		break;
	case SCPI_CPUINFO_SLT:
		index = (buf.pfm_info[3] >> 4) & 0xf;
		*freq = scpi_freq_map_table[index];
		index = buf.pfm_info[3] & 0xf;
		*vol = scpi_volt_map_table[index];
		ret = 0;
		break;
	default:
		*freq = 0;
		*vol = 0;
		ret = -1;
		break;
	};
	return ret;
}
EXPORT_SYMBOL_GPL(scpi_get_cpuinfo);

int scpi_unlock_bl40(void)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	u8 temp = 0;

	struct __packed {
		u32 status;
	} buf;

	SCPI_SETUP_DBUF(sdata, mdata, SCPI_CL_NONE,
			SCPI_CMD_BL4_WAIT_UNLOCK, temp, buf);
	if (scpi_execute_cmd(&sdata))
		return -1;
	return 0;
}
EXPORT_SYMBOL_GPL(scpi_unlock_bl40);

int scpi_send_bl40(unsigned int cmd, struct bl40_msg_buf *bl40_buf)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;

	SCPI_SETUP_DBUF_SIZE(sdata, mdata, SCPI_CL_NONE,
			     cmd, bl40_buf->buf, bl40_buf->size,
			     bl40_buf->buf, sizeof(bl40_buf->buf));
	return scpi_execute_cmd(&sdata);
}

/* scpi send data api
 * use for send data to dsp/aocpu/secpu/cm3/cm4
 * donot add yourself scpi api, use this common api
 */
int scpi_send_data(void *data, int size, int channel,
		   int cmd, void *revdata, int revsize)
{
	struct scpi_data_buf sdata;
	struct mhu_data_buf mdata;
	char *repack_buf = NULL;
	char *s_buf = NULL;
	char *r_buf = NULL;
	u32 status = 0;
	int sync = SCPI_DEF_SYNC;
	int client_id = SCPI_MAX;
	int ret = 0;

	switch (channel) {
	case SCPI_DSPA:
	case SCPI_DSPB:
	case SCPI_SECPU:
		s_buf = data;
		r_buf = revdata;
		SCPI_SETUP_DBUF_AUTO(sdata, mdata, client_id, cmd, s_buf,
				     size, r_buf, revsize, sync, channel);
		ret = scpi_execute_cmd(&sdata);
	break;
	case SCPI_AOCPU:
	{
		if (mhu_f & MASK_MHU_FIFO) {
			if (size > MBOX_DATA_SIZE ||
			    revsize > MBOX_DATA_SIZE) {
				pr_err("[scpi]: size: %d %d over max: %d\n",
				       size, revsize, MBOX_DATA_SIZE);
				return -EINVAL;
			}
			repack_buf =
				kmalloc(revsize + sizeof(status), GFP_KERNEL);
			if (!repack_buf)
				return -ENOMEM;

			memset(repack_buf, 0, (revsize + sizeof(status)));

			s_buf = data;
			r_buf = repack_buf;

			SCPI_SETUP_DBUF_AUTO(sdata, mdata, client_id, cmd,
					     s_buf, size, r_buf,
					     (revsize + sizeof(status)),
					     sync, channel);
			ret = scpi_execute_cmd(&sdata);
			if (ret == 0)
				memcpy(revdata, r_buf + sizeof(status),
				       revsize);
			else
				pr_err("[scpi]: mhu fifo send fail\n");
		} else {
			if (mhu_f & MASK_MHU) {
				repack_buf =
				kmalloc(revsize + sizeof(status), GFP_KERNEL);
				if (!repack_buf)
					return -ENOMEM;

				memset(repack_buf, 0,
				       (revsize + sizeof(status)));

				s_buf = data;
				r_buf = repack_buf;

				client_id = SCPI_CL_NONE;

				SCPI_SETUP_DBUF_AUTO(sdata, mdata, client_id,
						     cmd, s_buf, size, r_buf,
						     revsize + sizeof(status),
						     sync, channel);
				ret = scpi_execute_cmd(&sdata);

				if (ret == 0)
					memcpy(revdata,
					       (r_buf + sizeof(status)),
					       revsize);
				else
					pr_err("[scpi]: mhu send fail\n");
			} else {
				pr_err("Match AOCPU mhu driver fail\n");
				return -ENXIO;
			}
		}
	}
	break;
	default:
		pr_err("%s: cannot match channel 0x%x cmd 0x%x\n",
		       __func__, channel, cmd);
		return -ENXIO;
	}

	kfree(repack_buf);

	return ret;
}
EXPORT_SYMBOL_GPL(scpi_send_data);
