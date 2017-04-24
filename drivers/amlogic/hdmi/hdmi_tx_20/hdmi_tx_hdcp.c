/*
 * drivers/amlogic/hdmi/hdmi_tx_20/hdmi_tx_hdcp.c
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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/switch.h>
#include <linux/amlogic/hdmi_tx/hdmi_info_global.h>
#include <linux/amlogic/hdmi_tx/hdmi_tx_module.h>
#include <linux/uaccess.h>
#include "hdmi_tx_hdcp.h"
/*
	hdmi_tx_hdcp.c
	version 1.1
*/

static struct switch_dev hdcp_dev = {
	.name = "hdcp",
};

/* when record log is full and starts to wrap, trigger an uevent pulse(1,0) */
static struct switch_dev log_hdcp_dev = {
	.name = "hdcp_log",
};
static int log_hdcp_cnt;

static int hdmi_authenticated;
static struct dentry *hdcp_debugfs;
static struct debugfs_blob_wrapper hdcpblob;
/* hdcp_log_mode: bit1 as 1, then pr_info() at kernel
 *                bit0 as 1, then save log to debugfs
*/
static unsigned char hdcp_log_mode = 1;
static unsigned char hdcp_log_freq = 20;

/* obs_cur, record current obs */
static struct hdcp_obs_val obs_cur;
/* if obs_cur is differ than last time, then save to obs_last */
static struct hdcp_obs_val obs_last;

static struct hdcplog_buf hdcp_log_buf; /* store the log buf */

/* when read/write hdcp_log_buf, should use mutex for lock */
static DEFINE_MUTEX(mutex);

/* verify ksv, 20 ones and 20 zeroes */
int hdcp_ksv_valid(unsigned char *dat)
{
	int i, j, one_num = 0;

	for (i = 0; i < 5; i++) {
		for (j = 0; j < 8; j++) {
			if ((dat[i] >> j) & 0x1)
				one_num++;
		}
	}
	return one_num == 20;
}

static int save_obs_val(struct hdmitx_dev *hdev, struct hdcp_obs_val *obs)
{
	return hdev->HWOp.CntlDDC((struct hdmitx_dev *)obs,
		DDC_HDCP14_SAVE_OBS, 0);
}

static void pr_obs(struct hdcp_obs_val *obst0, struct hdcp_obs_val *obst1,
	unsigned int mask)
{
	struct hdcplog_buf *log = &hdcp_log_buf;

	/* if idx > HDCP_LOG_SIZE, then set idx as 0 for wrapping log */
#define GETBITS(val, start, len) (((val) >> start) & ((1 << len) - 1))
#define DIFF_ST(v1, v0, s, l, str) \
	do { \
		if (GETBITS(v1, s, l) != (GETBITS(v0, s, l))) { \
			if (hdcp_log_mode & (1 << 1)) \
				pr_info("%s: %x\n", str, GETBITS(v1, s, l)); \
			if (hdcp_log_mode & (1 << 0)) \
				log->idx += snprintf(log->idx + log->buf, \
					HDCP_LOG_SIZE, \
					"%s: %x\n", str, GETBITS(v1, s, l)); \
			if (log->idx >= HDCP_LOG_SIZE) \
				log->idx = 0; \
			log->buf[log->idx] = 0; \
		} \
	} while (0)

	if (mask & (1 << 0)) {
		if (GETBITS(obst1->obs0, 1, 7) != GETBITS(obst0->obs0, 1, 7)) {
			if (hdcp_log_mode & (1 << 1))
				pr_info("StateA: %x  SubStateA: %x\n",
					GETBITS(obst1->obs0, 4, 4),
					GETBITS(obst1->obs0, 1, 3));
			if (hdcp_log_mode & (1 << 0))
				log->idx += snprintf(log->idx + log->buf,
					HDCP_LOG_SIZE,
					"StateA: %x  SubStateA: %x\n",
					GETBITS(obst1->obs0, 4, 4),
					GETBITS(obst1->obs0, 1, 3));
			if (log->idx >= HDCP_LOG_SIZE)
				log->idx = 0;
			log->buf[log->idx] = 0; /* keep the last char as 0 */
		}
		DIFF_ST(obst1->obs0, obst0->obs0, 0, 1, "hdcpengaged");
	}
	if (mask & (1 << 1)) {
		DIFF_ST(obst1->obs1, obst0->obs1, 4, 3, "StateOEG");
		DIFF_ST(obst1->obs1, obst0->obs1, 0, 4, "StateR");
	}
	if (mask & (1 << 2)) {
		DIFF_ST(obst1->obs2, obst0->obs2, 3, 3, "StateE");
		DIFF_ST(obst1->obs2, obst0->obs2, 0, 3, "StateEEG");
	}
	if (mask & (1 << 3)) {
		DIFF_ST(obst1->obs3, obst0->obs3, 7, 1, "RSVD");
		DIFF_ST(obst1->obs3, obst0->obs3, 6, 1, "Repeater");
		DIFF_ST(obst1->obs3, obst0->obs3, 5, 1, "KSV_FIFO_Ready");
		DIFF_ST(obst1->obs3, obst0->obs3, 4, 1, "Fast_i2c");
		DIFF_ST(obst1->obs3, obst0->obs3, 3, 1, "RSVD2");
		DIFF_ST(obst1->obs3, obst0->obs3, 2, 1, "Hdmi_mode");
		DIFF_ST(obst1->obs3, obst0->obs3, 1, 1, "Features1.1");
		DIFF_ST(obst1->obs3, obst0->obs3, 0, 1, "Fast_Reauth");
	}
	if (mask & (1 << 4)) {
		DIFF_ST(obst1->intstat, obst0->intstat, 7, 1, "hdcp_engaged");
		DIFF_ST(obst1->intstat, obst0->intstat, 6, 1, "hdcp_failed");
		DIFF_ST(obst1->intstat, obst0->intstat, 4, 1, "i2cnack");
		DIFF_ST(obst1->intstat, obst0->intstat, 3, 1,
			"lostarbitration");
		DIFF_ST(obst1->intstat, obst0->intstat, 2, 1,
			"keepouterrorint");
		DIFF_ST(obst1->intstat, obst0->intstat, 1, 1, "KSVsha1calcint");
	}
}

static void _hdcp_do_work(struct work_struct *work)
{
	int ret = 0;
	struct hdmitx_dev *hdev =
		container_of(work, struct hdmitx_dev, work_do_hdcp.work);

	switch (hdev->hdcp_mode) {
	case 2:
		/* hdev->HWOp.CntlMisc(hdev, MISC_HDCP_CLKDIS, 1); */
		/* schedule_delayed_work(&hdev->work_do_hdcp, HZ / 50); */
		break;
	case 1:
		if (hdcp_log_mode == 0)
			return;
		mutex_lock(&mutex);
		ret = save_obs_val(hdev, &obs_cur);
		/* ret is NZ, then update obs_last */
		if (ret) {
			pr_obs(&obs_last, &obs_cur, ret);
			obs_last = obs_cur;
		}
		mutex_unlock(&mutex);
		/* log time frequency */
		if ((hdcp_log_freq == 0) || (hdcp_log_freq > 100))
			hdcp_log_freq = 20; /* Default value */
		schedule_delayed_work(&hdev->work_do_hdcp, HZ / hdcp_log_freq);
		break;
	case 0:
	default:
		mutex_lock(&mutex);
		log_hdcp_cnt = 0;
		memset(&obs_cur, 0, sizeof(obs_cur));
		memset(&obs_last, 0, sizeof(obs_last));
		memset(&hdcp_log_buf, 0, sizeof(hdcp_log_buf));
		mutex_unlock(&mutex);
		hdev->HWOp.CntlMisc(hdev, MISC_HDCP_CLKDIS, 0);
		break;
	}
}

void hdmitx_hdcp_do_work(struct hdmitx_dev *hdev)
{
	_hdcp_do_work(&hdev->work_do_hdcp.work);
}

static int hdmitx_hdcp_task(void *data)
{
	struct hdmitx_dev *hdev = (struct hdmitx_dev *)data;

	INIT_DELAYED_WORK(&hdev->work_do_hdcp, _hdcp_do_work);
	while (hdev->hpd_event != 0xff) {
		if (hdev->hdcp_mode) {
			hdmi_authenticated = hdev->HWOp.CntlDDC(hdev,
				DDC_HDCP_GET_AUTH, 0);
			switch_set_state(&hdcp_dev, hdmi_authenticated);
			log_hdcp_cnt++;
			switch_set_state(&log_hdcp_dev, log_hdcp_cnt / 5);
		} else
			hdmi_authenticated = 0;
		msleep_interruptible(200);
	}

	return 0;
}

static int hdcp_log_show(struct seq_file *s, void *what)
{
	mutex_lock(&mutex);

	seq_printf(s, "%s", &hdcp_log_buf.buf[hdcp_log_buf.idx + 1]);
	if (hdcp_log_buf.idx)
		seq_printf(s, "%s", &hdcp_log_buf.buf[0]);

	/* Once reading, clear the log */
	memset(&hdcp_log_buf, 0, sizeof(hdcp_log_buf));

	mutex_unlock(&mutex);

	return 0;
}

static int hdcp_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, hdcp_log_show, inode->i_private);
}

static ssize_t hdcp_log_write(struct file *file, const char __user *userbuf,
				   size_t count, loff_t *ppos)
{
	return count;
}

static const struct file_operations hdcp_log_fops = {
	.open		= hdcp_log_open,
	.read		= seq_read,
	.write		= hdcp_log_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static int __init hdmitx_hdcp_init(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct dentry *log;
	struct dentry *blob;
	struct dentry *mode;
	struct dentry *freq;

	pr_info("hdmitx_hdcp_init\n");
	if (hdev->hdtx_dev == NULL) {
		hdmi_print(IMP, SYS "exit for null device of hdmitx!\n");
		return -ENODEV;
	}

	switch_dev_register(&hdcp_dev);
	switch_dev_register(&log_hdcp_dev);

	hdev->task_hdcp = kthread_run(hdmitx_hdcp_task,	(void *)hdev,
		"kthread_hdcp");

	hdcp_debugfs = debugfs_create_dir("hdcp", NULL);
	if (!hdcp_debugfs)
		return -ENOENT;
	log = debugfs_create_file("log", 0644, hdcp_debugfs, NULL,
		&hdcp_log_fops);
	if (!log)
		goto exit;

	hdcpblob.data = (void *)hdcp_log_buf.buf;
	hdcpblob.size = HDCP_LOG_SIZE;
	blob = debugfs_create_blob("blob", 0644, hdcp_debugfs, &hdcpblob);
	if (!blob)
		goto exit;

	mode = debugfs_create_u8("mode", 0644, hdcp_debugfs, &hdcp_log_mode);
	if (!mode)
		goto exit;

	freq = debugfs_create_u8("frequency", 0644, hdcp_debugfs,
		&hdcp_log_freq);
	if (!freq)
		goto exit;

	return 0;
exit:
	debugfs_remove_recursive(hdcp_debugfs);
	hdcp_debugfs = NULL;
	return -ENOENT;
}

static void __exit hdmitx_hdcp_exit(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev)
		cancel_delayed_work_sync(&hdev->work_do_hdcp);
	switch_dev_unregister(&hdcp_dev);
	switch_dev_unregister(&log_hdcp_dev);
	debugfs_remove_recursive(hdcp_debugfs);
}


MODULE_PARM_DESC(hdmi_authenticated, "\n hdmi_authenticated\n");
module_param(hdmi_authenticated, int, S_IRUGO);

module_init(hdmitx_hdcp_init);
module_exit(hdmitx_hdcp_exit);
MODULE_DESCRIPTION("AMLOGIC HDMI TX HDCP driver");
MODULE_LICENSE("GPL");
