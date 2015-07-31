/*
 * drivers/char/dma_test/test_other_case.c
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

/* total buffer and each buffer length */
//#define DTC_TOTAL_LEN	SZ_1M /* may lead to dma_pool_alloc failed in sw_dma_request */
//#define DTC_ONE_LEN		SZ_128K
#define DTC_TOTAL_LEN	SZ_512K
#define DTC_ONE_LEN	SZ_64K
/* src/dst buffer address */
static u32 g_src_addr = 0, g_dst_addr = 0;
/* cur buffer index */
static atomic_t g_acur_cnt = ATOMIC_INIT(0);

/* wait queue for dma done */
extern wait_queue_head_t g_dtc_queue[];

u32 __cb_qd_stopcmd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = DTC_TOTAL_LEN / DTC_ONE_LEN;
	u32 	ucur_cnt = 0;

	switch(cause) {
	case DMA_CB_OK:
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt) {
			ucur_saddr = g_src_addr + ucur_cnt * DTC_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_QD))
				printk("%s err, line %d\n", __func__, __LINE__);
		} else if(ucur_cnt == uloop_cnt){
			/*
			 * we have complete enqueueing, but not means it's the last qd irq,
			 * in test, we found sometimes never meet if(ucur_cnt == uloop_cnt...
			 * that is, enqueue complete during hd/fd callback.
			 */
			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
		} else {
			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
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

u32 __cb_fd_stopcmd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	return 0;
}

u32 __cb_hd_stopcmd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	return 0;
}

u32 __cb_op_stopcmd(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	return 0;
}

u32 __waitdone_stopcmd(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50 */

	ret = wait_event_interruptible_timeout(g_dtc_queue[0], \
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

u32 __dtc_stopcmd(void)
{
	u32 	uret = 0;
	u32 	i = 0;
	void 	*src_vaddr = NULL, *dst_vaddr = NULL;
	u32 	usrc_paddr = 0, udst_paddr = 0;
	dm_hdl_t dma_hdl = (dm_hdl_t)NULL;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;
	struct dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	src_vaddr = dma_alloc_coherent(NULL, DTC_TOTAL_LEN, (dma_addr_t *)&usrc_paddr, GFP_KERNEL);
	if(NULL == src_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: src_vaddr 0x%08x, usrc_paddr 0x%08x\n", __func__, (u32)src_vaddr, usrc_paddr);
	dst_vaddr = dma_alloc_coherent(NULL, DTC_TOTAL_LEN, (dma_addr_t *)&udst_paddr, GFP_KERNEL);
	if(NULL == dst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: dst_vaddr 0x%08x, udst_paddr 0x%08x\n", __func__, (u32)dst_vaddr, udst_paddr);

	get_random_bytes(src_vaddr, DTC_TOTAL_LEN);
	memset(dst_vaddr, 0x54, DTC_TOTAL_LEN);

	atomic_set(&g_acur_cnt, 0);
	g_src_addr = usrc_paddr;
	g_dst_addr = udst_paddr;

	dma_hdl = sw_dma_request("case_stp_dma", DMA_WORK_MODE_CHAIN);
	//dma_hdl = sw_dma_request("case_stp_dma", DMA_WORK_MODE_SINGLE);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set callback */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
	done_cb.func = __cb_qd_stopcmd;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set queuedone_cb success\n", __func__);
	done_cb.func = __cb_fd_stopcmd;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	done_cb.func = __cb_hd_stopcmd;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);
	op_cb.func = __cb_op_stopcmd;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set op_cb success\n", __func__);

	memset(&dma_config, 0, sizeof(dma_config));
	dma_config.src_drq_type = DRQSRC_SDRAM;
	dma_config.dst_drq_type = DRQDST_SDRAM;
	dma_config.bconti_mode = false; /* must be 0, otherwise irq will come again and again */
	dma_config.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	dma_config.address_type = DMAADDRT_D_LN_S_LN; /* change with dma type */
	dma_config.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_config.src_addr = usrc_paddr;
	dma_config.dst_addr = udst_paddr;
	dma_config.byte_cnt = DTC_ONE_LEN;
	dma_config.para = 0; /* to check here */
	if(0 != sw_dma_config(dma_hdl, &dma_config, ENQUE_PHASE_NORMAL)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_config success\n", __func__);

	atomic_set(&g_adma_done, 0);
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_start success\n", __func__);

	/* callback enqueue and normal enqueue simutanously */
	i = 0;
	while(i++ < 100) {
		u32 	ucur_saddr = 0, ucur_daddr = 0;
		u32	uindex = 0;

		get_random_bytes(&uindex, sizeof(uindex));
		uindex  %= (DTC_TOTAL_LEN / DTC_ONE_LEN);
		ucur_saddr = g_src_addr + uindex * DTC_ONE_LEN;
		ucur_daddr = g_dst_addr + uindex * DTC_ONE_LEN;
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uret = __LINE__;
			goto end;
		}
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uret = __LINE__;
			goto end;
		}
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uret = __LINE__;
			goto end;
		}
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uret = __LINE__;
			goto end;
		}
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uret = __LINE__;
			goto end;
		}
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uret = __LINE__;
			goto end;
		}
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uret = __LINE__;
			goto end;
		}
	}

#if 0
	if(0 != __waitdone_stopcmd()) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: __waitdone_stopcmd sucess\n", __func__);
#endif
	sw_dma_dump_chan(dma_hdl);

	/* stop and release dma channel */
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

	/* check if data ok */
	if(0 == memcmp(src_vaddr, dst_vaddr, DTC_TOTAL_LEN))
		pr_info("%s: data check ok!\n", __func__);
	else {
		pr_err("%s: data check err!\n", __func__);
		//uret = __LINE__; /* we donnot need data ok, just test stop cmd */
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	else
		pr_info("%s success!\n", __func__);

	if((dm_hdl_t)NULL != dma_hdl) {
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
		if(0 != sw_dma_release(dma_hdl))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
	}

	if(NULL != src_vaddr)
		dma_free_coherent(NULL, DTC_TOTAL_LEN, src_vaddr, usrc_paddr);
	if(NULL != dst_vaddr)
		dma_free_coherent(NULL, DTC_TOTAL_LEN, dst_vaddr, udst_paddr);

	return uret;
}

u32 __cb_qd_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = DTC_TOTAL_LEN / DTC_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __func__);
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt) {
			pr_info("%s, line %d, ucur_cnt %d\n", __func__, __LINE__, ucur_cnt);
			//ucur_saddr = g_src_addr + atomic_read(&g_acur_cnt) * DTC_ONE_LEN; /* BUG: data check maybe err */
			ucur_saddr = g_src_addr + ucur_cnt * DTC_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_QD))
				printk("%s err, line %d\n", __func__, __LINE__);
		} else if(ucur_cnt == uloop_cnt){
			/*
			 * we have complete enqueueing, but not means it's the last qd irq,
			 * in test, we found sometimes never meet if(ucur_cnt == uloop_cnt...
			 * that is, enqueue complete during hd/fd callback.
			 */
			pr_info("%s, line %d\n", __func__, __LINE__);

#if 0			/* NOTE: cannot sigal g_adma_done here, because maybe it's NOT the last qd irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
#endif
		} else {
			/*
			 * NOTE: cannot sigal g_adma_done here, because maybe:
			 * (1) it's the last irq, in this case, we can sigal g_adma_done
			 * (2) it's the last but one irq. maybe ucur_cnt already > uloop_cnt before(eg: in hd/fd cb),
			 * 	so, at this time, it's the last but one irq, after this, __dma_chan_handle_qd will start
			 *	the rest buffer.
			 * in test, we find here(pr_info("%s, line %d\n", __func__, __LINE__) below) will meet 2 times.
			 */
			pr_info("%s, line %d\n", __func__, __LINE__);

#if 0
			sw_dma_dump_chan(dma_hdl); /* for debug */
			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
#endif
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

u32 __cb_fd_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = DTC_TOTAL_LEN / DTC_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			pr_info("%s, line %d, ucur_cnt %d\n", __func__, __LINE__, ucur_cnt);
			ucur_saddr = g_src_addr + ucur_cnt * DTC_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_FD))
				printk("%s err, line %d\n", __func__, __LINE__);
		} else
			pr_info("%s, line %d\n", __func__, __LINE__); /* do nothing */
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

u32 __cb_hd_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = DTC_TOTAL_LEN / DTC_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			pr_info("%s, line %d, ucur_cnt %d\n", __func__, __LINE__, ucur_cnt);
			ucur_saddr = g_src_addr + ucur_cnt * DTC_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_HD))
				printk("%s err, line %d\n", __func__, __LINE__);
		} else
			pr_info("%s, line %d\n", __func__, __LINE__); /* do nothing */
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

u32 __cb_op_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
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
		pr_info("%s, line %d\n", __func__, __LINE__);
		break;
	}

	return 0;
}

u32 __waitdone_many_enq(void)
{
	long 	ret = 0;
	long 	timeout = 3 * HZ; /* 3s */

	ret = wait_event_interruptible_timeout(g_dtc_queue[0], \
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
 * __dtc_many_enq - dma test case for DTC_1TM2M_MANY_ENQ
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_many_enq(void)
{
	u32 	uret = 0;
	void 	*src_vaddr = NULL, *dst_vaddr = NULL;
	u32 	usrc_paddr = 0, udst_paddr = 0;
	dm_hdl_t dma_hdl = (dm_hdl_t)NULL;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;
	struct dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	src_vaddr = dma_alloc_coherent(NULL, DTC_TOTAL_LEN, (dma_addr_t *)&usrc_paddr, GFP_KERNEL);
	if(NULL == src_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: src_vaddr 0x%08x, usrc_paddr 0x%08x\n", __func__, (u32)src_vaddr, usrc_paddr);
	dst_vaddr = dma_alloc_coherent(NULL, DTC_TOTAL_LEN, (dma_addr_t *)&udst_paddr, GFP_KERNEL);
	if(NULL == dst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: dst_vaddr 0x%08x, udst_paddr 0x%08x\n", __func__, (u32)dst_vaddr, udst_paddr);

	get_random_bytes(src_vaddr, DTC_TOTAL_LEN);
	memset(dst_vaddr, 0x54, DTC_TOTAL_LEN);

	atomic_set(&g_acur_cnt, 0);
	g_src_addr = usrc_paddr;
	g_dst_addr = udst_paddr;

	dma_hdl = sw_dma_request("m2m_dma", DMA_WORK_MODE_CHAIN);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set callback */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
	done_cb.func = __cb_qd_many_enq;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set queuedone_cb success\n", __func__);
	done_cb.func = __cb_fd_many_enq;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	done_cb.func = __cb_hd_many_enq;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);
	op_cb.func = __cb_op_many_enq;
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
	dma_config.src_addr = usrc_paddr;
	dma_config.dst_addr = udst_paddr;
	dma_config.byte_cnt = DTC_ONE_LEN;
	dma_config.bconti_mode = false;
	dma_config.src_drq_type = DRQSRC_SDRAM;
	dma_config.dst_drq_type = DRQDST_SDRAM;
	if(0 != sw_dma_config(dma_hdl, &dma_config, ENQUE_PHASE_NORMAL)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_config success\n", __func__);
	sw_dma_dump_chan(dma_hdl);

	/* start dma */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uret = __LINE__;
		goto end;
	}

	/* normal enqueue and callback enqueue simutanously */
	{
		u32 	ucur_cnt = 0, ucur_saddr = 0, ucur_daddr = 0;
		u32	uloop_cnt = DTC_TOTAL_LEN / DTC_ONE_LEN;

		while((ucur_cnt = atomic_add_return(1, &g_acur_cnt)) < uloop_cnt) {
			pr_info("%s, line %d, ucur_cnt %d\n", __func__, __LINE__, ucur_cnt);
			ucur_saddr = g_src_addr + ucur_cnt * DTC_ONE_LEN;
			ucur_daddr = g_dst_addr + ucur_cnt * DTC_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, DTC_ONE_LEN, ENQUE_PHASE_NORMAL))
				printk("%s err, line %d\n", __func__, __LINE__);

			/* in order to meet cb/normal enqueue simutanously */
			msleep(0);
		}
	}
	pr_info("%s, line %d\n", __func__, __LINE__);

	__waitdone_many_enq();

	if(0 == memcmp(src_vaddr, dst_vaddr, DTC_TOTAL_LEN))
		pr_info("%s: data check ok!\n", __func__);
	else {
		pr_err("%s: data check err!\n", __func__);
		uret = __LINE__; /* return err */
		goto end;
	}

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

	if(NULL != src_vaddr)
		dma_free_coherent(NULL, DTC_TOTAL_LEN, src_vaddr, usrc_paddr);
	if(NULL != dst_vaddr)
		dma_free_coherent(NULL, DTC_TOTAL_LEN, dst_vaddr, udst_paddr);

	pr_info("%s end!\n", __func__);
	return uret;
}

u32 __cb_qd_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __func__);
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

u32 __cb_fd_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __func__);
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

u32 __cb_hd_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __func__);
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

u32 __cb_op_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
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
		pr_info("%s, line %d\n", __func__, __LINE__);
		break;
	}

	return 0;
}

u32 __waitdone_conti_mode(void)
{
	long 	ret = 0;
	long 	timeout = 3 * HZ; /* 3s */

	ret = wait_event_interruptible_timeout(g_dtc_queue[0], \
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
 * __dtc_chain_conti_mode - dma test case for DTC_CHAIN_CONTI_MOD
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_chain_conti_mode(void)
{
	u32 	uret = 0;
	void 	*src_vaddr = NULL, *dst_vaddr = NULL;
	u32 	usrc_paddr = 0, udst_paddr = 0;
	dm_hdl_t dma_hdl = (dm_hdl_t)NULL;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;
	struct dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	/* prepare the buffer and data */
	src_vaddr = dma_alloc_coherent(NULL, DTC_TOTAL_LEN, (dma_addr_t *)&usrc_paddr, GFP_KERNEL);
	if(NULL == src_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: src_vaddr 0x%08x, usrc_paddr 0x%08x\n", __func__, (u32)src_vaddr, usrc_paddr);
	dst_vaddr = dma_alloc_coherent(NULL, DTC_TOTAL_LEN, (dma_addr_t *)&udst_paddr, GFP_KERNEL);
	if(NULL == dst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: dst_vaddr 0x%08x, udst_paddr 0x%08x\n", __func__, (u32)dst_vaddr, udst_paddr);

	get_random_bytes(src_vaddr, DTC_TOTAL_LEN);
	memset(dst_vaddr, 0x54, DTC_TOTAL_LEN);

	atomic_set(&g_acur_cnt, 0);
	g_src_addr = usrc_paddr;
	g_dst_addr = udst_paddr;

	dma_hdl = sw_dma_request("m2m_dma", DMA_WORK_MODE_CHAIN);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set callback */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
	done_cb.func = __cb_qd_conti_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set queuedone_cb success\n", __func__);
	done_cb.func = __cb_fd_conti_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	done_cb.func = __cb_hd_conti_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);
	op_cb.func = __cb_op_conti_mode;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set op_cb success\n", __func__);

	memset(&dma_config, 0, sizeof(dma_config));
	dma_config.xfer_type 	= DMAXFER_D_BWORD_S_BWORD;
	dma_config.address_type = DMAADDRT_D_LN_S_LN;
	dma_config.para 	= 0;
	dma_config.irq_spt 	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_config.src_addr 	= usrc_paddr;
	dma_config.dst_addr 	= udst_paddr;
	//dma_config.byte_cnt 	= DTC_ONE_LEN;
	dma_config.byte_cnt 	= DTC_TOTAL_LEN; /* only enqueue one buf */
	dma_config.bconti_mode 	= true;
	dma_config.src_drq_type 	= DRQSRC_SDRAM;
	dma_config.dst_drq_type 	= DRQDST_SDRAM;
	if(0 != sw_dma_config(dma_hdl, &dma_config, ENQUE_PHASE_NORMAL)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_config success\n", __func__);
	sw_dma_dump_chan(dma_hdl);

	/* start dma */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uret = __LINE__;
		goto end;
	}
	/* let callback go on enqueueing... */
	msleep(8000);

	/* check if data ok */
	if(0 == memcmp(src_vaddr, dst_vaddr, DTC_TOTAL_LEN))
		pr_info("%s: data check ok!\n", __func__);
	else {
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

	if(NULL != src_vaddr)
		dma_free_coherent(NULL, DTC_TOTAL_LEN, src_vaddr, usrc_paddr);
	if(NULL != dst_vaddr)
		dma_free_coherent(NULL, DTC_TOTAL_LEN, dst_vaddr, udst_paddr);

	pr_info("%s, end!\n", __func__);
	return uret;
}

