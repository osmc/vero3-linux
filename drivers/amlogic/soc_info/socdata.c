/*
 * drivers/amlogic/soc_info/socdata.c
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

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_address.h>
#include <linux/cma.h>
#include <linux/sysfs.h>
#include <linux/random.h>
#include <linux/amlogic/secmon.h>
#include "soc_info.h"

unsigned int read_nocsdata_cmd;
unsigned int write_nocsdata_cmd;
static void __iomem *soc_ver1_addr, *soc_poc_addr;

struct socdata_dev_t {
	struct cdev cdev;
	dev_t  devno;
};

static struct socdata_dev_t *socdata_devp;

static int socdata_open(struct inode *inode, struct file *file)
{
	struct socdata_dev_t *devp;

	devp = container_of(inode->i_cdev, struct socdata_dev_t, cdev);
	file->private_data = devp;
	return 0;
}

static long socdata_unlocked_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	unsigned long ret = 0;
	void __user *argp = (void __user *)arg;
	unsigned long __user *soc_info = argp;
	unsigned char info[NOCS_DATA_LENGTH];
	long long int offset = 0;

	switch (cmd) {
	case CMD_POC_DATA:
		if (put_user(readl(soc_poc_addr), soc_info))
			return -EFAULT;
		break;
	case CMD_SOCVER1_DATA:
		if (put_user(readl(soc_ver1_addr), soc_info))
			return -EFAULT;
		break;
	case CMD_SOCVER2_DATA:
		if (put_user(readl(soc_ver1_addr + 4), soc_info))
			return -EFAULT;
		break;
	case CMD_NOCSDATA_READ:
		nocsdata_read(info, NOCS_DATA_LENGTH, &offset);
		ret = copy_to_user(argp, info, sizeof(info));
		if (ret != 0) {
			pr_debug("%s:%d,copy_to_user fail\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case CMD_NOCSDATA_WRITE:
		ret = copy_from_user(info, argp, (NOCS_DATA_LENGTH - 28));
		if (ret != 0) {
			pr_debug("%s:%d,copy_from_user fail\n",
				__func__, __LINE__);
			return ret;
		}
		if (nocsdata_write(info, (NOCS_DATA_LENGTH - 28), &offset))
			return -EFAULT;
		break;

	default:
		return -EINVAL;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long socdata_compat_ioctl(struct file *filp,
			      unsigned int cmd, unsigned long args)
{
	unsigned long ret;

	args = (unsigned long)compat_ptr(args);
	ret = socdata_unlocked_ioctl(filp, cmd, args);

	return ret;
}
#endif

static const struct file_operations socdata_fops = {
	.owner      = THIS_MODULE,
	.open       = socdata_open,
	.unlocked_ioctl      = socdata_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = socdata_compat_ioctl,
#endif
};

static struct class socdata_class = {
	.name = SOCDATA_CLASS_NAME,
};

static int aml_socdata_probe(struct platform_device *pdev)
{
	int ret =  -1;
	struct device *devp;

	socdata_devp = devm_kzalloc(&pdev->dev,
		sizeof(struct socdata_dev_t), GFP_KERNEL);
	if (!socdata_devp) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "socdata: failed to allocate memory\n ");
		goto out;
	}

	soc_ver1_addr = of_iomap(pdev->dev.of_node, 0);
	soc_poc_addr = of_iomap(pdev->dev.of_node, 1);

	if (of_property_read_u32(pdev->dev.of_node, "read_nocsdata_cmd",
			&read_nocsdata_cmd)) {
		dev_err(&pdev->dev, "please config read nocsdata cmd\n");
		return -1;
	}
	if (of_property_read_u32(pdev->dev.of_node, "write_nocsdata_cmd",
		&write_nocsdata_cmd)) {
		dev_err(&pdev->dev, "please config write nocsdata cmd\n");
		return -1;
	}

	ret = alloc_chrdev_region(&socdata_devp->devno, 0, 1,
		SOCDATA_DEVICE_NAME);
	if (ret < 0) {
		dev_err(&pdev->dev, "socdata: failed to allocate major number\n ");
		ret = -ENODEV;
		goto error1;
	}
	dev_info(&pdev->dev, "socdata_devno:%x\n", socdata_devp->devno);
	ret = class_register(&socdata_class);
	if (ret) {
		dev_err(&pdev->dev, "socdata: failed to register class\n ");
		goto error2;
	}

	/* connect the file operations with cdev */
	cdev_init(&socdata_devp->cdev, &socdata_fops);
	socdata_devp->cdev.owner = THIS_MODULE;
	/* connect the major/minor number to the cdev */
	ret = cdev_add(&socdata_devp->cdev, socdata_devp->devno, 1);
	if (ret) {
		dev_err(&pdev->dev, "socdata: failed to add device\n");
		goto error3;
	}
	devp = device_create(&socdata_class, NULL, socdata_devp->devno,
		NULL, SOCDATA_DEVICE_NAME);
	if (IS_ERR(devp)) {
		dev_err(&pdev->dev, "socdata: failed to create device node\n");
		ret = PTR_ERR(devp);
		goto error4;
	}
	return 0;

error4:
	cdev_del(&socdata_devp->cdev);
error3:
	class_unregister(&socdata_class);
error2:
	unregister_chrdev_region(socdata_devp->devno, 1);
error1:
//	devm_kfree(&(pdev->dev), socdata_devp);
out:
	return ret;
}

static int aml_socdata_remove(struct platform_device *pdev)
{
	unregister_chrdev_region(socdata_devp->devno, 1);
	device_destroy(&socdata_class, socdata_devp->devno);
	cdev_del(&socdata_devp->cdev);
	class_unregister(&socdata_class);
	return 0;
}

static const struct of_device_id meson_socdata_dt_match[] = {
	{	.compatible = "amlogic, socdata",
	},
	{},
};

static struct platform_driver aml_socdata_driver = {
	.probe = aml_socdata_probe,
	.remove = aml_socdata_remove,
	.driver = {
		.name = SOCDATA_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = meson_socdata_dt_match,
	},
};

module_platform_driver(aml_socdata_driver);

MODULE_DESCRIPTION("AMLOGIC socdata driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<huaihong.lei@amlogic.com>");
