/*
 * drivers/char/dma_test/sun6i_dma_test.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma test head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUN6I_DMA_TEST_H
#define __SUN6I_DMA_TEST_H

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
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/dma.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/dma-mapping.h>
#include <linux/wait.h>
#include <linux/random.h>

#include <mach/dma.h>

/* dma test case id */
enum dma_test_case_e {
	DTC_CHAIN_MODE,		/* test chain mode */
	DTC_CHAIN_CONTI_MOD,	/* test chain continue mode */
	DTC_SINGLE_MODE,	/* test single mode */
	DTC_SINGLE_CONT_MODE,	/* test single continue mode */

	DTC_TWO_THREAD,		/* test two-thread using dma simultaneously */
	DTC_MANY_ENQ, 		/* test many enqueueing */
	DTC_ENQ_AFT_DONE,	/* test enqueue after all buffer done, to see if auto start new buf */
	DTC_STOP,		/* test stop dma when running */
	DTC_MAX
};

extern wait_queue_head_t g_dtc_queue[];
extern atomic_t g_adma_done;

#endif /* __SUN6I_DMA_TEST_H */

