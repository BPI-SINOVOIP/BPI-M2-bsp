/*
 * drivers/char/dma_test/test_case_single_md.c
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
#include <linux/random.h>

#define DTC_1T_TOTAL_LEN	SZ_512K
//#define DTC_1T_TOTAL_LEN	SZ_16K
#define DTC_1T_ONE_LEN		SZ_4K

/* src/dst addr for loop dma transfer */
static u32 g_src_addr = 0, g_dst_addr = 0;
static u32 g_qd_cnt = 0; /* queue done count */
static atomic_t g_acur_cnt = ATOMIC_INIT(0);

u32 __cb_qd_single_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		g_qd_cnt++;
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt) {
			ucur_saddr = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_1T_ONE_LEN, ENQUE_PHASE_QD))
				printk("%s err, line %d\n", __func__, __LINE__);
		} else if(ucur_cnt >= uloop_cnt){
			/* we have complete enqueueing, but not means it's the last qd irq */
			//if(true == sw_dma_sgmd_buflist_empty(dma_hdl)) {
			if(true) {
				/* 这里也不能认为是传完, 测试发现两次到这里,原因, __dtc_single_mode中enqueue
				之前加了cnt, 但enqueue被irq打断, 一直挂着, 只等irq的enqueue和transfer结束, 此时
				当然buflist_empty, 这时__dtc_single_mode才有机会继续未完成的唯一enqueue, 导致两次
				到这里.	因此本demo, 这里不能认为数据完全传完, 但对于其他场景, 一般可认为qd中list空了就结束了.
				*/
				/* maybe it's the last irq */
				atomic_set(&g_adma_done, 1);
				wake_up_interruptible(&g_dtc_queue[DTC_SINGLE_MODE]);
			}
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __func__);
		break;
	default:
		uret = __LINE__;
		goto end;
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	return uret;
}

u32 __cb_fd_single_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			ucur_saddr = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_1T_ONE_LEN, ENQUE_PHASE_FD))
				printk("%s err, line %d\n", __func__, __LINE__);
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __func__);
		break;
	default:
		uret = __LINE__;
		goto end;
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	return uret;
}

u32 __cb_hd_single_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __func__);
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			ucur_saddr = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_1T_ONE_LEN, ENQUE_PHASE_HD))
				printk("%s err, line %d\n", __func__, __LINE__);
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __func__);
		break;
	default:
		uret = __LINE__;
		goto end;
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	return uret;
}

u32 __cb_op_single_mode(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	pr_info("%s: called!\n", __func__);

	switch(op) {
	case DMA_OP_START:
		pr_info("%s: op DMA_OP_START!\n", __func__);
		atomic_set(&g_adma_done, 0);
		break;
	case DMA_OP_STOP:
		pr_info("%s: op DMA_OP_STOP!\n", __func__);
		break;
	case DMA_OP_SET_HD_CB:
		pr_info("%s: op DMA_OP_SET_HD_CB!\n", __func__);
		break;
	case DMA_OP_SET_FD_CB:
		pr_info("%s: op DMA_OP_SET_FD_CB!\n", __func__);
		break;
	case DMA_OP_SET_OP_CB:
		pr_info("%s: op DMA_OP_SET_OP_CB!\n", __func__);
		break;
	default:
		//DBG_FUN_LINE;
		break;
	}

	return 0;
}

u32 __waitdone_single_mode(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50s */

	ret = wait_event_interruptible_timeout(g_dtc_queue[DTC_SINGLE_MODE], \
		atomic_read(&g_adma_done)== 1, timeout);
	atomic_set(&g_adma_done, 0);

	if(-ERESTARTSYS == ret) {
		pr_info("%s success!\n", __func__);
		return 0;
	} else if(0 == ret) {
		pr_info("%s err, time out!\n", __func__);
		return __LINE__;
	} else {
		pr_info("%s success with condition match, ret %d!\n", __func__, (int)ret);
		return 0;
	}
}

/**
 * __dtc_single_mode - dma test case one-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_single_mode(void)
{
	u32 	uret = 0;
	u32	uqueued_normal = 0;
	void 	*src_vaddr = NULL, *dst_vaddr = NULL;
	u32 	src_paddr = 0, dst_paddr = 0;
	dm_hdl_t dma_hdl = (dm_hdl_t)NULL;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;
	struct dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	/* prepare the buffer and data */
	src_vaddr = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&src_paddr, GFP_KERNEL);
	if(NULL == src_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: src_vaddr 0x%08x, src_paddr 0x%08x\n", __func__, (u32)src_vaddr, src_paddr);
	dst_vaddr = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&dst_paddr, GFP_KERNEL);
	if(NULL == dst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: dst_vaddr 0x%08x, dst_paddr 0x%08x\n", __func__, (u32)dst_vaddr, dst_paddr);
	get_random_bytes(src_vaddr, DTC_1T_TOTAL_LEN);
	memset(dst_vaddr, 0x54, DTC_1T_TOTAL_LEN);

	atomic_set(&g_acur_cnt, 0);
	g_src_addr = src_paddr;
	g_dst_addr = dst_paddr;

	dma_hdl = sw_dma_request("m2m_dma", DMA_WORK_MODE_SINGLE);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set callback */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
	done_cb.func = __cb_qd_single_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set queuedone_cb success\n", __func__);
	done_cb.func = __cb_fd_single_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	done_cb.func = __cb_hd_single_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);
	op_cb.func = __cb_op_single_mode;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set op_cb success\n", __func__);

	memset(&dma_config, 0, sizeof(dma_config));
	dma_config.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	dma_config.address_type = DMAADDRT_D_LN_S_LN;
	dma_config.para = 0;
	dma_config.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_config.src_addr = src_paddr;
	dma_config.dst_addr = dst_paddr;
	dma_config.byte_cnt = DTC_1T_ONE_LEN;
	//dma_config.conti_mode = 1;
	dma_config.bconti_mode = false;
	dma_config.src_drq_type = DRQSRC_SDRAM;
	dma_config.dst_drq_type = DRQDST_SDRAM;
	if(0 != sw_dma_config(dma_hdl, &dma_config, ENQUE_PHASE_NORMAL)) {
		uret = __LINE__;
		goto end;
	}
	uqueued_normal++;
	pr_info("%s: sw_dma_config success\n", __func__);
	sw_dma_dump_chan(dma_hdl);

	g_qd_cnt = 0;

	/* start dma */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uret = __LINE__;
		goto end;
	}

	/* normal enqueue and callback enqueue simutanously */
	{
		u32 	ucur_cnt = 0, ucur_saddr = 0, ucur_daddr = 0;
		u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;

		while((ucur_cnt = atomic_add_return(1, &g_acur_cnt)) < uloop_cnt) {
			ucur_saddr = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL))
				printk("%s err, line %d\n", __func__, __LINE__);

			uqueued_normal++;
		}
	}

	if(0 != __waitdone_single_mode()) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: __waitdone_single_mode sucess\n", __func__);

	/* NOTE: must sleep here, see the analysis in __cb_qd_single_mode, 2012-11-14 */
	msleep(2000);

	/* check if data ok */
	if(0 == memcmp(src_vaddr, dst_vaddr, DTC_1T_TOTAL_LEN)) {
		//pr_info("%s: data check ok! g_qd_cnt %d\n", __func__, g_qd_cnt);
		printk("%s: data check ok! g_qd_cnt %d, normal queued %d\n", __func__, g_qd_cnt, uqueued_normal);
	} else {
		pr_err("%s: data check err!\n", __func__);
		uret = __LINE__; /* return err */
		goto end;
	}

	/* stop and free dma channel */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_stop success\n", __func__);
	if(0 != sw_dma_release(dma_hdl)) {
		uret = __LINE__;
		goto end;
	}
	dma_hdl = (dm_hdl_t)NULL;
	pr_info("%s: sw_dma_release success\n", __func__);

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	else
		pr_info("%s, success!\n", __func__);

	if((dm_hdl_t)NULL != dma_hdl) {
		pr_err("%s, stop and release dma handle now!\n", __func__);
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
		if(0 != sw_dma_release(dma_hdl))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
	}
	pr_err("%s, line %d!\n", __func__, __LINE__);

	if(NULL != src_vaddr)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, src_vaddr, src_paddr);
	if(NULL != dst_vaddr)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, dst_vaddr, dst_paddr);

	pr_err("%s, end!\n", __func__);
	return uret;
}

