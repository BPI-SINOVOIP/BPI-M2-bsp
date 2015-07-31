#ifndef __ASM_MACH_CLKDEV_H
#define __ASM_MACH_CLKDEV_H

#include <linux/clk.h>

int __clk_get(struct clk *hclk);
void __clk_put(struct clk *clk);

#endif
