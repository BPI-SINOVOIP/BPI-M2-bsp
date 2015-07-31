/*
 * linux/include/linux/sunxi_timer_test.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi timer test head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_TIMER_TEST_H
#define __SUNXI_TIMER_TEST_H

enum e_sunxi_timer_test_cmd
{
	TIMER_TEST_CMD_FUNC_NORMAL,
	TIMER_TEST_CMD_FUNC_HRTIMER,
};

struct timer_test_para {
	unsigned int 	timer_interv_us;	/* timer interval in us */
	unsigned int 	print_gap_s;		/* print gap in s */
	unsigned int 	total_test_s;		/* total test time in s */
};

#endif /* __SUNXI_TIMER_TEST_H */

