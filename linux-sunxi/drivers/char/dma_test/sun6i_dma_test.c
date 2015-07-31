/*
 * drivers/char/dma_test/sun6i_dma_test.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "sun6i_dma_test.h"

#include "test_chain_mode.h"
#include "test_other_case.h"
#include "test_single_mode.h"
#include "test_single_conti_mode.h"
#include "test_two_thread.h"

/* wait queue for waiting dma done */
wait_queue_head_t	g_dtc_queue[DTC_MAX];
atomic_t 		g_adma_done = ATOMIC_INIT(0); /* dma done flag */

static void __dma_test_init_waitqueue(void)
{
	u32 i = 0;

	for(i = 0; i < DTC_MAX; i++)
		init_waitqueue_head(&g_dtc_queue[i]);
}

static int dma_test_main(int id)
{
	u32 uret = 0;

	switch(id) {
	case DTC_CHAIN_MODE:
		uret = __dtc_chain_mode();
		break;
	case DTC_CHAIN_CONTI_MOD:
		uret = __dtc_chain_conti_mode();
		break;
	case DTC_SINGLE_MODE:
		uret = __dtc_single_mode();
		break;
	case DTC_SINGLE_CONT_MODE:
		uret = __dtc_single_conti_mode();
		break;
	case DTC_TWO_THREAD:
		uret = __dtc_two_thread();
		break;
	case DTC_MANY_ENQ:
		uret = __dtc_many_enq();
		break;
	case DTC_ENQ_AFT_DONE:
		uret = __dtc_case_enq_aftdone();
		break;
	case DTC_STOP:
		uret = __dtc_stopcmd();
		break;
	default:
		uret = __LINE__;
		break;
	}

	if(0 == uret)
		printk("%s: test success!\n", __func__);
	else
		printk("%s: test failed!\n", __func__);
	return uret;
}

const char *case_name[] = {
	"DTC_CHAIN_MODE",
	"DTC_CHAIN_CONTI_MOD",
	"DTC_SINGLE_MODE",
	"DTC_SINGLE_CONT_MODE",
	"DTC_TWO_THREAD",
	"DTC_MANY_ENQ",
	"DTC_ENQ_AFT_DONE",
	"DTC_STOP",
};

ssize_t test_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	int id = 0;

	/* get test id */
	if(strict_strtoul(buf, 10, (long unsigned int *)&id)) {
		pr_err("%s: invalid string %s\n", __func__, buf);
		return -EINVAL;
	}
	pr_info("%s: string %s, test case %s\n", __func__, buf, case_name[id]);

	if(0 != dma_test_main(id))
		pr_err("%s: dma_test_main failed! id %d\n", __func__, id);
	else
		pr_info("%s: dma_test_main success! id %d\n", __func__, id);
	return size;
}

ssize_t help_show(struct class *class, struct class_attribute *attr, char *buf)
{
	ssize_t cnt = 0;

	cnt += sprintf(buf + cnt, "usage: echo id > test\n");
	cnt += sprintf(buf + cnt, "     id for case DTC_CHAIN_MODE         is %d\n", (int)DTC_CHAIN_MODE);
	cnt += sprintf(buf + cnt, "     id for case DTC_CHAIN_CONTI_MOD    is %d\n", (int)DTC_CHAIN_CONTI_MOD);
	cnt += sprintf(buf + cnt, "     id for case DTC_SINGLE_MODE        is %d\n", (int)DTC_SINGLE_MODE);
	cnt += sprintf(buf + cnt, "     id for case DTC_SINGLE_CONT_MODE   is %d\n", (int)DTC_SINGLE_CONT_MODE);
	cnt += sprintf(buf + cnt, "     id for case DTC_TWO_THREAD         is %d\n", (int)DTC_TWO_THREAD);
	cnt += sprintf(buf + cnt, "     id for case DTC_MANY_ENQ           is %d\n", (int)DTC_MANY_ENQ);
	cnt += sprintf(buf + cnt, "     id for case DTC_ENQ_AFT_DONE       is %d\n", (int)DTC_ENQ_AFT_DONE);
	cnt += sprintf(buf + cnt, "     id for case DTC_STOP               is %d\n", (int)DTC_STOP);
	cnt += sprintf(buf + cnt, "case description:\n");
	cnt += sprintf(buf + cnt, "     DTC_CHAIN_MODE:         case for chain mode\n");
	cnt += sprintf(buf + cnt, "     DTC_CHAIN_CONTI_MOD:    case for chain continue mode\n");
	cnt += sprintf(buf + cnt, "     DTC_SINGLE_MODE:        case for single mode\n");
	cnt += sprintf(buf + cnt, "     DTC_SINGLE_CONT_MODE:   case for single continue mode\n");
	cnt += sprintf(buf + cnt, "     DTC_TWO_THREAD:         test two-thread using dma simultaneously\n");
	cnt += sprintf(buf + cnt, "     DTC_MANY_ENQ:           test many enqueueing\n");
	cnt += sprintf(buf + cnt, "     DTC_ENQ_AFT_DONE:       test enqueue after all buffer done, to see if auto start new buf\n");
	cnt += sprintf(buf + cnt, "     DTC_STOP:               test stop dma when running\n");
	return cnt;
}

static struct class_attribute dma_test_class_attrs[] = {
	__ATTR(test, 0220, NULL, test_store), /* not 222, for CTS, other group cannot have write permission, 2013-1-11 */
	__ATTR(help, 0444, help_show, NULL),
	__ATTR_NULL,
};

static struct class dma_test_class = {
	.name		= "sunxi_dma_test",
	.owner		= THIS_MODULE,
	.class_attrs	= dma_test_class_attrs,
};

static int __init sw_dma_test_init(void)
{
	int status;
	pr_info("%s enter\n", __func__);

	/* init dma wait queue */
	__dma_test_init_waitqueue();

	/* register sys class */
	status = class_register(&dma_test_class);
	if(status < 0)
		pr_info("%s err, status %d\n", __func__, status);
	else
		pr_info("%s success\n", __func__);
	return 0;
}

static void __exit sw_dma_test_exit(void)
{
	pr_info("sw_dma_test_exit: enter\n");
}

#ifdef MODULE
module_init(sw_dma_test_init);
module_exit(sw_dma_test_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("liugang");
MODULE_DESCRIPTION ("sun6i Dma Test driver code");
#else
__initcall(sw_dma_test_init);
#endif /* MODULE */
