/*
 * drivers/char/timer_test/sunxi_timer_test_i.h
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

#ifndef __SUNXI_TIMER_TEST_I_H
#define __SUNXI_TIMER_TEST_I_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/device.h>

#define TIMER_DBG_FUN_LINE_TODO		printk("[TIMER_TEST]%s, line %d, todo############\n", __FUNCTION__, __LINE__)
#define TIMER_DBG_FUN_LINE 		printk("[TIMER_TEST]%s, line %d\n", __FUNCTION__, __LINE__)
#define TIMER_ERR_FUN_LINE 		printk("[TIMER_TEST]%s err, line %d\n", __FUNCTION__, __LINE__)
#define TIMER_ASSERT_RET(x, ret, pos)	if(!(x)) {ret = __LINE__; goto pos;}

#endif /* __SUNXI_TIMER_TEST_I_H */

