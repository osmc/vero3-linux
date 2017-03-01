/*
 * meson-rng.c - Random Number Generator driver for the Amlogic Meson
 *
 * Copyright (C) 2014 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/hw_random.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/printk.h>

#define RNG_DATA 0x00

/* minimum time between 32-bit RNG data reads in us */
#define MESON_RNG_DEFAULT_PERIOD   (2)

static u64 min_period = MESON_RNG_DEFAULT_PERIOD;
static ktime_t expires;

static int meson_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	void __iomem *rng_base = (void __iomem *)rng->priv;

	if (min_period > 0) {
		while (1) {
			s64 delta;
			ktime_t now = ktime_get();
			delta = ktime_us_delta(expires, now);
			if (delta > 0 && delta <= min_period) {
				if (!wait)
					return 0;
				udelay(delta);
			} else {
				break;
			}
		}
	}

	*(u32 *)buf = __raw_readl(rng_base + RNG_DATA);

	if (min_period > 0)
		expires = ktime_add_us(ktime_get(), min_period);
	return sizeof(u32);
}

static struct hwrng meson_rng_ops = {
	.name = "meson",
	.read = meson_read,
};

static int meson_rng_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	void __iomem *rng_base;
	int err, i;

	expires = ktime_get();

	if (!of_property_read_u32(np, "period", &i))
		min_period = i;

	/* map peripheral */
	rng_base = of_iomap(np, 0);
	if (!rng_base) {
		dev_err(dev, "failed to remap rng regs");
		return -ENODEV;
	}
	meson_rng_ops.priv = (unsigned long)rng_base;

	/* register driver */
	err = hwrng_register(&meson_rng_ops);
	if (err) {
		dev_err(dev, "hwrng registration failed\n");
		iounmap(rng_base);
	} else {
		dev_info(dev, "hwrng registered\n");
	}
	return err;
}

static int meson_rng_remove(struct platform_device *pdev)
{
	void __iomem *rng_base = (void __iomem *)meson_rng_ops.priv;

	/* unregister driver */
	hwrng_unregister(&meson_rng_ops);
	iounmap(rng_base);
	return 0;
}

static const struct of_device_id meson_rng_of_match[] = {
	{ .compatible = "amlogic,meson-rng", },
	{},
};

static struct platform_driver meson_rng_driver = {
	.driver		= {
		.name	= "meson-rng",
		.owner	= THIS_MODULE,
		.of_match_table = meson_rng_of_match,
	},
	.probe		= meson_rng_probe,
	.remove		= meson_rng_remove,
};

module_platform_driver(meson_rng_driver);

MODULE_DESCRIPTION("Meson H/W Random Number Generator driver");
MODULE_AUTHOR("Lawrence Mok <lawrence.mok@amlogic.com");
MODULE_LICENSE("GPL");
