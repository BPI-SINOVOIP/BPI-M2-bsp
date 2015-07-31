/*
 *  arch/arm/mach-sun6i/cpu-freq/cpu-freq-table.c
 *
 * Copyright (c) 2012 Softwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/types.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include "cpu-freq.h"

struct cpufreq_frequency_table sunxi_freq_tbl[] = {
	{ .frequency = 120000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 144000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 168000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 192000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 216000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 240000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 264000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 288000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 312000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 336000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 360000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 384000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 408000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 432000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 456000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 480000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 504000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 528000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 552000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 576000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 600000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 624000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 648000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 672000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 696000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 720000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 744000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 768000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 792000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 828000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 864000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 900000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 936000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 972000 ,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1008000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1044000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1080000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1116000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1152000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1200000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1248000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1296000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1344000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1392000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	{ .frequency = 1440000,	.index = SUNXI_CLK_DIV(0, 0, 0, 0), },
 	
    /* table end */
    { .frequency = CPUFREQ_TABLE_END,  .index = 0,              },
};

