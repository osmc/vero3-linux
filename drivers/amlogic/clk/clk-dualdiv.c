/*
 * drivers/amlogic/clk/clk-dualdiv.c
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

/*
 * The AO Domain embeds a dual/divider to generate a more precise
 * 32,768KHz clock for low-power suspend mode and CEC.
 *     ______   ______
 *    |      | |      |
 *    | Div1 |-| Cnt1 |
 *   /|______| |______|\
 * -|  ______   ______  X--> Out
 *   \|      | |      |/
 *    | Div2 |-| Cnt2 |
 *    |______| |______|
 *
 * The dividing can be switched to single or dual, with a counter
 * for each divider to set when the switching is done.
 */

#include <linux/clk-provider.h>
#include <linux/module.h>

#include "clk-dualdiv.h"
#include "clkc.h"

static inline unsigned int meson_parm_read(void __iomem *base, struct parm *p)
{
	unsigned int val;

	val = readl(base + p->reg_off);
	return PARM_GET(p->width, p->shift, val);
}

static inline void meson_parm_write(void __iomem *base, struct parm *p,
				    unsigned int val)
{
	unsigned int reg;

	reg = readl(base + p->reg_off);
	writel(PARM_SET(p->width, p->shift, reg, val), base + p->reg_off);
}

static inline struct meson_clk_dualdiv_data *
meson_clk_dualdiv_data(struct meson_dualdiv_clk *clk)
{
	return (struct meson_clk_dualdiv_data *)clk->data;
}

static unsigned long
__dualdiv_param_to_rate(unsigned long parent_rate,
			const struct meson_clk_dualdiv_param *p)
{
	if (!p->dual)
		return DIV_ROUND_CLOSEST(parent_rate, p->n1);

	return DIV_ROUND_CLOSEST(parent_rate * (p->m1 + p->m2),
				 p->n1 * p->m1 + p->n2 * p->m2);
}

static unsigned long meson_clk_dualdiv_recalc_rate(struct clk_hw *hw,
						   unsigned long parent_rate)
{
	struct meson_dualdiv_clk *clk = to_meson_dualdiv_clk(hw);
	struct meson_clk_dualdiv_data *dualdiv = meson_clk_dualdiv_data(clk);
	struct meson_clk_dualdiv_param setting;

	setting.dual = meson_parm_read(clk->base, &dualdiv->dual);
	setting.n1 = meson_parm_read(clk->base, &dualdiv->n1) + 1;
	setting.m1 = meson_parm_read(clk->base, &dualdiv->m1) + 1;
	setting.n2 = meson_parm_read(clk->base, &dualdiv->n2) + 1;
	setting.m2 = meson_parm_read(clk->base, &dualdiv->m2) + 1;

	return __dualdiv_param_to_rate(parent_rate, &setting);
}

static const struct meson_clk_dualdiv_param *
__dualdiv_get_setting(unsigned long rate, unsigned long parent_rate,
		      struct meson_clk_dualdiv_data *dualdiv)
{
	const struct meson_clk_dualdiv_param *table = dualdiv->table;
	unsigned long best = 0, now = 0;
	unsigned int i, best_i = 0;

	if (!table)
		return NULL;

	for (i = 0; table[i].n1; i++) {
		now = __dualdiv_param_to_rate(parent_rate, &table[i]);

		/* If we get an exact match, don't bother any further */
		if (now == rate) {
			return &table[i];
		} else if (abs(now - rate) < abs(best - rate)) {
			best = now;
			best_i = i;
		}
	}

	return (struct meson_clk_dualdiv_param *)&table[best_i];
}

static long meson_clk_dualdiv_round_rate(struct clk_hw *hw, unsigned long rate,
					 unsigned long *parent_rate)
{
	struct meson_dualdiv_clk *clk = to_meson_dualdiv_clk(hw);
	struct meson_clk_dualdiv_data *dualdiv = meson_clk_dualdiv_data(clk);
	const struct meson_clk_dualdiv_param *setting =
		__dualdiv_get_setting(rate, *parent_rate, dualdiv);

	if (!setting)
		return meson_clk_dualdiv_recalc_rate(hw, *parent_rate);

	return __dualdiv_param_to_rate(*parent_rate, setting);
}

static int meson_clk_dualdiv_set_rate(struct clk_hw *hw, unsigned long rate,
				      unsigned long parent_rate)
{
	struct meson_dualdiv_clk *clk = to_meson_dualdiv_clk(hw);
	struct meson_clk_dualdiv_data *dualdiv = meson_clk_dualdiv_data(clk);
	const struct meson_clk_dualdiv_param *setting =
		__dualdiv_get_setting(rate, parent_rate, dualdiv);

	if (!setting)
		return -EINVAL;

	meson_parm_write(clk->base, &dualdiv->dual, setting->dual);
	meson_parm_write(clk->base, &dualdiv->n1, setting->n1 - 1);
	meson_parm_write(clk->base, &dualdiv->m1, setting->m1 - 1);
	meson_parm_write(clk->base, &dualdiv->n2, setting->n2 - 1);
	meson_parm_write(clk->base, &dualdiv->m2, setting->m2 - 1);

	return 0;
}

const struct clk_ops meson_clk_dualdiv_ops = {
	.recalc_rate	= meson_clk_dualdiv_recalc_rate,
	.round_rate	= meson_clk_dualdiv_round_rate,
	.set_rate	= meson_clk_dualdiv_set_rate,
};
EXPORT_SYMBOL_GPL(meson_clk_dualdiv_ops);

const struct clk_ops meson_clk_dualdiv_ro_ops = {
	.recalc_rate	= meson_clk_dualdiv_recalc_rate,
};
EXPORT_SYMBOL_GPL(meson_clk_dualdiv_ro_ops);

MODULE_DESCRIPTION("Amlogic dual divider driver");
MODULE_AUTHOR("Amlogic");
MODULE_LICENSE("GPL v2");
