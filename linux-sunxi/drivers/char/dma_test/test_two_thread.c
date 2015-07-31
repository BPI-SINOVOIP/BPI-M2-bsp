/*
 * drivers/char/dma_test/test_two_thread.c
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

/* dma name for request */
#define THREAD1_DMA_NAME 	"m2m_dma_thread1"
#define THREAD2_DMA_NAME 	"m2m_dma_thread2"
/* total transfer length */
#define DTC_2T_TOTAL_LEN	SZ_512K
#define DTC_2T_ONE_LEN		SZ_4K
/* test time for each thread */
#define TEST_TIME_THREAD1 	0x0fffffff	/* ms */
#define TEST_TIME_THREAD2 	0x0fffffff

/* dma done flag */
static atomic_t 	g_adma_done1 = ATOMIC_INIT(0);
static atomic_t 	g_adma_done2 = ATOMIC_INIT(0);
/* src/dst buffer physical addr */
static u32 		g_sadr1 = 0, g_dadr1 = 0;
static u32 		g_sadr2 = 0, g_dadr2 = 0;
/* cur buffer index */
static atomic_t 	g_acur_cnt1 = ATOMIC_INIT(0);
static atomic_t 	g_acur_cnt2 = ATOMIC_INIT(0);

/**
 * __dump_cur_mem_info - dump current mem info
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dump_cur_mem_info(void)
{
	struct sysinfo s_info;
	int error;

	/* get cur system memory info */
	memset(&s_info, 0, sizeof(s_info));
	error = do_sysinfo(&s_info);
	pr_info("%s: cur time 0x%08xs, total mem %dM, free mem %dM, total high %dM, free high %dM\n", \
		__func__, (u32)s_info.uptime,
		(u32)(s_info.totalram / 1024 / 1024),
		(u32)(s_info.freeram / 1024 / 1024),
		(u32)(s_info.totalhigh / 1024 / 1024),
		(u32)(s_info.freehigh / 1024 / 1024));
	return 0;
}

u32 __cb_qd_2(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, udst_saddr = 0;
	u32	uloop_cnt = DTC_2T_TOTAL_LEN / DTC_2T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		ucur_cnt = atomic_add_return(1, &g_acur_cnt2);
		if(ucur_cnt < uloop_cnt) {
			ucur_saddr = g_sadr2 + ucur_cnt * DTC_2T_ONE_LEN;
			udst_saddr = g_dadr2 + ucur_cnt * DTC_2T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, udst_saddr, DTC_2T_ONE_LEN, ENQUE_PHASE_QD))
				printk("%s err, line %d\n", __func__, __LINE__);
		} else if(ucur_cnt == uloop_cnt){
			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done2, 1);
			wake_up_interruptible(&g_dtc_queue[2]);
		} else {
			/* maybe it's the last irq */
			atomic_set(&g_adma_done2, 1);
			wake_up_interruptible(&g_dtc_queue[2]);
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

u32 __cb_fd_2(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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

/**
 * __cb_hd_2 - dma half done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __cb_hd_2(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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

/**
 * __cb_op_2 - dma op callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __cb_op_2(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	pr_info("%s: called!\n", __func__);
	switch(op) {
	case DMA_OP_START:
		pr_info("%s: op DMA_OP_START!\n", __func__);
		atomic_set(&g_adma_done2, 0);
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
		printk("%s err, line %d\n", __func__, __LINE__);
		return __LINE__;
	}

	return 0;
}

/**
 * __waitdone_2 - wait dma done for DTC_2T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __waitdone_2(void)
{
	long 	ret = 0;
	long 	timeout = 2 * HZ;

	ret = wait_event_interruptible_timeout(g_dtc_queue[2],
		atomic_read(&g_adma_done2)== 1, timeout);
	atomic_set(&g_adma_done2, 0);

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
 * _thread2_proc - thread2 test func
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 _thread2_proc(void)
{
	u32 	uret = 0;
	void 	*usrc_vaddr = NULL, *udst_vaddr = NULL;
	u32 	usrc_paddr = 0, udst_paddr = 0;
	dm_hdl_t dma_hdl = (dm_hdl_t)NULL;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;
	struct dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	usrc_vaddr = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&usrc_paddr, GFP_KERNEL);
	if(NULL == usrc_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: usrc_vaddr 0x%08x, usrc_paddr 0x%08x\n", __func__, (u32)usrc_vaddr, usrc_paddr);
	udst_vaddr = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&udst_paddr, GFP_KERNEL);
	if(NULL == udst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: udst_vaddr 0x%08x, udst_paddr 0x%08x\n", __func__, (u32)udst_vaddr, udst_paddr);

	get_random_bytes(usrc_vaddr, DTC_2T_TOTAL_LEN);
	memset(udst_vaddr, 0x53, DTC_2T_TOTAL_LEN);

	atomic_set(&g_acur_cnt2, 0);
	g_sadr2 = usrc_paddr;
	g_dadr2 = udst_paddr;

	dma_hdl = sw_dma_request(THREAD2_DMA_NAME, DMA_WORK_MODE_CHAIN);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set callback */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
	done_cb.func = __cb_qd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set queuedone_cb success\n", __func__);
	done_cb.func = __cb_fd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	done_cb.func = __cb_hd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);
	op_cb.func = __cb_op_2;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set op_cb success\n", __func__);

	/* config dma para and enqueue buffer */
	memset(&dma_config, 0, sizeof(dma_config));
	dma_config.src_drq_type = DRQSRC_SDRAM;
	dma_config.dst_drq_type = DRQDST_SDRAM;
	//dma_config.conti_mode = 1;
	dma_config.bconti_mode = false; /* must be 0, otherwise irq will come again and again */
	dma_config.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	dma_config.address_type = DMAADDRT_D_LN_S_LN; /* change with dma type */
	dma_config.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_config.src_addr = usrc_paddr;
	dma_config.dst_addr = udst_paddr;
	dma_config.byte_cnt = DTC_2T_ONE_LEN;
	dma_config.para = 0;
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
	pr_info("%s: sw_dma_start success\n", __func__);

	/* wait dma done */
	if(0 != __waitdone_2()) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: __waitdone_2 sucess\n", __func__);

	/* check if data ok */
	if(0 == memcmp(usrc_vaddr, udst_vaddr, DTC_2T_TOTAL_LEN))
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
		pr_info("%s success!\n", __func__);

	if((dm_hdl_t)NULL != dma_hdl) {
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
			pr_err("%s err, line %d!\n", __func__, __LINE__);
		}
		if(0 != sw_dma_release(dma_hdl)) {
			pr_err("%s err, line %d!\n", __func__, __LINE__);
		}
	}

	if(NULL != usrc_vaddr)
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, usrc_vaddr, usrc_paddr);
	if(NULL != udst_vaddr)
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, udst_vaddr, udst_paddr);

	return uret;
}

u32 __cb_qd_1(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, udst_saddr = 0;
	u32	uloop_cnt = DTC_2T_TOTAL_LEN / DTC_2T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);
	switch(cause) {
	case DMA_CB_OK:
		ucur_cnt = atomic_add_return(1, &g_acur_cnt1);
		if(ucur_cnt < uloop_cnt) {
			ucur_saddr = g_sadr1 + ucur_cnt * DTC_2T_ONE_LEN;
			udst_saddr = g_dadr1 + ucur_cnt * DTC_2T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, udst_saddr, DTC_2T_ONE_LEN, ENQUE_PHASE_QD))
				printk("%s err, line %d\n", __func__, __LINE__);
		} else if(ucur_cnt == uloop_cnt){
			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done1, 1);
			wake_up_interruptible(&g_dtc_queue[1]);
		} else {
			/* maybe it's the last irq */
			atomic_set(&g_adma_done1, 1);
			wake_up_interruptible(&g_dtc_queue[1]);
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

/**
 * __cb_fd_1 - dma full done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __cb_fd_1(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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

/**
 * __cb_hd_1 - dma half done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __cb_hd_1(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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

/**
 * __cb_op_1 - dma op callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __cb_op_1(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	pr_info("%s: called!\n", __func__);
	switch(op) {
	case DMA_OP_START:
		pr_info("%s: op DMA_OP_START!\n", __func__);
		atomic_set(&g_adma_done1, 0);
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
		printk("%s err, line %d\n", __func__, __LINE__);
		return __LINE__;
	}

	return 0;
}

/**
 * __waitdone_1 - wait dma done for DTC_2T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __waitdone_1(void)
{
	long 	ret = 0;
	long 	timeout = 2 * HZ;

	ret = wait_event_interruptible_timeout(g_dtc_queue[1], \
		atomic_read(&g_adma_done1)== 1, timeout);
	atomic_set(&g_adma_done1, 0);

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
 * _thread1_proc - thread2 test func
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 _thread1_proc(void)
{
	u32 	uret = 0;
	void 	*usrc_vaddr = NULL, *udst_vaddr = NULL;
	u32 	usrc_paddr = 0, udst_paddr = 0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	struct dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	usrc_vaddr = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&usrc_paddr, GFP_KERNEL);
	if(NULL == usrc_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: usrc_vaddr 0x%08x, usrc_paddr 0x%08x\n", __func__, (u32)usrc_vaddr, usrc_paddr);
	udst_vaddr = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&udst_paddr, GFP_KERNEL);
	if(NULL == udst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: udst_vaddr 0x%08x, udst_paddr 0x%08x\n", __func__, (u32)udst_vaddr, udst_paddr);

	get_random_bytes(usrc_vaddr, DTC_2T_TOTAL_LEN);
	memset(udst_vaddr, 0x56, DTC_2T_TOTAL_LEN);

	atomic_set(&g_acur_cnt1, 0);
	g_sadr1 = usrc_paddr;
	g_dadr1 = udst_paddr;

	dma_hdl = sw_dma_request(THREAD1_DMA_NAME, DMA_WORK_MODE_CHAIN);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set callback */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
	done_cb.func = __cb_qd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set queuedone_cb success\n", __func__);
	done_cb.func = __cb_fd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	done_cb.func = __cb_hd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);
	op_cb.func = __cb_op_1;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set op_cb success\n", __func__);

	/* config dma para and enqueue buffer */
	memset(&dma_config, 0, sizeof(dma_config));
	dma_config.src_drq_type = DRQSRC_SDRAM;
	dma_config.dst_drq_type = DRQDST_SDRAM;
	//dma_config.conti_mode = 1;
	dma_config.bconti_mode = false; /* must be 0, otherwise irq will come again and again */
	dma_config.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	dma_config.address_type = DMAADDRT_D_LN_S_LN; /* change with dma type */
	dma_config.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_config.src_addr = usrc_paddr;
	dma_config.dst_addr = udst_paddr;
	dma_config.byte_cnt = DTC_2T_ONE_LEN;
	dma_config.para = 0;
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
	pr_info("%s: sw_dma_start success\n", __func__);

	/* wait dma done */
	if(0 != __waitdone_1()) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: __waitdone_1 sucess\n", __func__);

	/* check if data ok */
	if(0 == memcmp(usrc_vaddr, udst_vaddr, DTC_2T_TOTAL_LEN))
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
		pr_info("%s success!\n", __func__);

	if((dm_hdl_t)NULL != dma_hdl) {
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
		if(0 != sw_dma_release(dma_hdl))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
	}

	if(NULL != usrc_vaddr)
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, usrc_vaddr, usrc_paddr);
	if(NULL != udst_vaddr)
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, udst_vaddr, udst_paddr);

	return uret;
}

int __test_thread1(void * arg)
{
	u32 	uret = 0;
	u32 	begin_ms = 0, end_ms = 0;

	begin_ms = (jiffies * 1000) / HZ;
	pr_info("%s: begin_ms 0x%08x\n", __func__, begin_ms);
	/* loop test until time passed */
	while(1) {
		__dump_cur_mem_info();
		if(0 != _thread1_proc()) {
			uret = __LINE__;
			goto end;
		}

		/* calculate time passed */
		end_ms = (jiffies * 1000) / HZ;
		pr_info("%s: cur_ms 0x%08x\n", __func__, end_ms);
		if(end_ms - begin_ms >= TEST_TIME_THREAD1) {
			pr_info("%s: time passed! ok!\n", __func__);
			break;
		}
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	else
		pr_info("%s success!\n", __func__);

	return uret;
}

int __test_thread2(void * arg)
{
	u32 	uret = 0;
	u32 	begin_ms = 0, end_ms = 0;

	begin_ms = (jiffies * 1000) / HZ;
	while(1) {
		__dump_cur_mem_info();
		if(0 != _thread2_proc()) {
			uret = __LINE__;
			goto end;
		}

		/* calculate time passed */
		end_ms = (jiffies * 1000) / HZ;
		if(end_ms - begin_ms >= TEST_TIME_THREAD2) {
			pr_info("%s: time passed! ok!\n", __func__);
			break;
		}
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	else
		pr_info("%s success!\n", __func__);

	return uret;
}

u32 __dtc_two_thread(void)
{
	/* dump the initial memory status, for test memory leak */
	__dump_cur_mem_info();

	kernel_thread(__test_thread1, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(__test_thread2, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

