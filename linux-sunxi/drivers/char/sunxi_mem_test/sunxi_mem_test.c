/*
 * drivers/char/sunxi_mem_test/sunxi_mem_test.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi sunxi_mem test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/sunxi_physmem.h>

#define TEST_SEC  20

/*
 * usage: 
 *   func_name: function name to defination
 *   test_sec:  test time in seconds, 0 means test forever.
 */
#define DEFINE_SUNMM_FUNC(func_name, test_sec)									\
static int func_name(void * arg)										\
{														\
	do {													\
		u32 	pa = 0, rest_size = 0, temp = 0;							\
		u32	size = (get_random_int() % 32) * SZ_1M;							\
		struct sysinfo start, end;									\
														\
		printk("%s start: size %d Mbytes, test_sec %d\n", __func__, size / SZ_1M, test_sec);		\
														\
		do_sysinfo(&start);										\
		while(1) {											\
			do_sysinfo(&end);									\
			if(0 == test_sec || end.uptime - start.uptime <= (long)test_sec) {			\
				if(0 == (pa = sunxi_mem_alloc(size)))						\
					printk("%s: out of memory! size 0x%08x\n", __func__, size);		\
				else {										\
					temp = (get_random_int() % 10) * 10; msleep_interruptible(temp);	\
					rest_size = sunxi_mem_get_rest_size();					\
					printk("%s: alloc %d Mbytes success, pa 0x%08x, sleep %d ms, rest %d Mbytes\n",\
						__func__, size / SZ_1M, pa, temp, rest_size / SZ_1M);		\
					sunxi_mem_free(pa);							\
					pa = 0;									\
				}										\
			} else {										\
				printk("%s: time passed! start_sec %d, end_sec %d\n", __func__, (int)start.uptime, (int)end.uptime);\
				break;										\
			}											\
		}												\
														\
		rest_size = sunxi_mem_get_rest_size();								\
		printk("%s end, rest size %d Mbytes\n", __func__, rest_size / SZ_1M);				\
		return 0;											\
	}while(0);												\
}

/**
 * DEFINE_SUNMM_FUNC(sunmm_test_thread1, 10)
 *
 * define a function sunmm_test_thread1, test for 10sec.
 */
DEFINE_SUNMM_FUNC(sunmm_test_thread1, TEST_SEC)
DEFINE_SUNMM_FUNC(sunmm_test_thread2, TEST_SEC)
DEFINE_SUNMM_FUNC(sunmm_test_thread3, TEST_SEC)
DEFINE_SUNMM_FUNC(sunmm_test_thread4, TEST_SEC)
DEFINE_SUNMM_FUNC(sunmm_test_thread5, TEST_SEC)

int __init sunmm_test_module_init(void)
{
	printk("%s enter\n", __func__);
	kernel_thread(sunmm_test_thread1, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunmm_test_thread2, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunmm_test_thread3, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunmm_test_thread4, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunmm_test_thread5, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

void __exit sunmm_test_module_exit(void)
{
	pr_debug("%s enter\n", __func__);
}

module_init(sunmm_test_module_init);
module_exit(sunmm_test_module_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("liugang");
MODULE_DESCRIPTION ("sunxi mem test driver");

