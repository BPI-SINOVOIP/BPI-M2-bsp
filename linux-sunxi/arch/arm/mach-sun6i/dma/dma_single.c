/*
 * arch/arm/mach-sun6i/dma/dma_single.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

/* rmv the first item from list, start it */
void __dma_start(dm_hdl_t dma_hdl)
{
	des_item *pdes_item = NULL;
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(list_empty(&pchan->list_single));

	/* remove from list */
	pdes_item = list_entry(pchan->list_single.next, des_item, list);
	list_del(&pdes_item->list); /* just remove, not free, freed in qd handle */

	DMA_WRITE_REG(pdes_item->paddr, pchan->reg_base + DMA_OFF_REG_START);
	csp_dma_chan_start(pchan);
	STATE_SGL(pchan) = SNGL_STA_RUNING;
	pchan->pcur_des = pdes_item;
}

void __dma_pause(dm_hdl_t dma_hdl)
{
	csp_dma_chan_pause((dma_channel_t *)dma_hdl);
}

void __dma_resume(dm_hdl_t dma_hdl)
{
	csp_dma_chan_resume((dma_channel_t *)dma_hdl);
}

/* not include cur buf */
void __dma_free_buflist(dma_channel_t *pchan)
{
	des_item *pdes_item = NULL;
	u32 utemp;

	while(!list_empty(&pchan->list_single)) {
		pdes_item = list_entry(pchan->list_single.next, des_item, list);
		utemp = pdes_item->paddr;
		list_del(&pdes_item->list);
#ifndef NOT_ALLOC_DES_TEMP
		dma_pool_free(g_des_pool, pdes_item, utemp);
#else
		index_put++;
		if(index_put >= TEMP_DES_CNT)
			index_put = 0;
#endif
	}
}

/* free cur buf and buf in list */
void __dma_free_allbuf(dma_channel_t *pchan)
{
	u32 utemp = 0;

	if(NULL != pchan->pcur_des) {
		utemp = pchan->pcur_des->paddr;
#ifndef NOT_ALLOC_DES_TEMP
		dma_pool_free(g_des_pool, pchan->pcur_des, utemp);
#else
		index_put++;
		if(index_put >= TEMP_DES_CNT)
			index_put = 0;
#endif
		pchan->pcur_des = NULL;
	}
	__dma_free_buflist(pchan);
}

void __dma_stop(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

#ifdef DBG_DMA
	DMA_INF("%s: state %d, buf chain: \n", __func__, (u32)STATE_SGL(pchan));
	switch(STATE_SGL(pchan)) {
	case SNGL_STA_IDLE:
		DMA_INF("%s: state idle, maybe before start or after stop, so stop the channel, free all buf list\n", __func__);
		WARN_ON(NULL != pchan->pcur_des);
		break;
	case SNGL_STA_RUNING:
		DMA_INF("%s: state running, so stop the channel, abort the cur buf, and free extra buf\n", __func__);
		WARN_ON(NULL == pchan->pcur_des);
		break;
	case SNGL_STA_LAST_DONE:
		DMA_INF("%s: state last done, so stop the channel, buffer already freed all, to check\n", __func__);
		WARN_ON(NULL != pchan->pcur_des || !list_empty(&pchan->list_single));
		break;
	default:
		break;
	}
#endif
	/* stop dma channle and clear irq pending */
	csp_dma_chan_stop(pchan);
	csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD);

	/* free buffer list */
	__dma_free_allbuf(pchan);

	/* change channel state to idle */
	STATE_SGL(pchan) = SNGL_STA_IDLE;
}

void __dma_get_status(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_status((dma_channel_t *)dma_hdl);
}

void __dma_get_cur_src_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_cur_srcaddr((dma_channel_t *)dma_hdl);
}

void __dma_get_cur_dst_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_cur_dstaddr((dma_channel_t *)dma_hdl);
}

void __dma_get_left_bytecnt(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_left_bytecnt((dma_channel_t *)dma_hdl);
}

void __dma_set_op_cb(dm_hdl_t dma_hdl, struct dma_op_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(SNGL_STA_IDLE != STATE_SGL(pchan));
	pchan->op_cb.func = pcb->func;
	pchan->op_cb.parg = pcb->parg;
}

void __dma_set_hd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(SNGL_STA_IDLE != STATE_SGL(pchan));
	pchan->hd_cb.func = pcb->func;
	pchan->hd_cb.parg = pcb->parg;
}

void __dma_set_fd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(SNGL_STA_IDLE != STATE_SGL(pchan));
	pchan->fd_cb.func = pcb->func;
	pchan->fd_cb.parg = pcb->parg;
}

void __dma_set_qd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(SNGL_STA_IDLE != STATE_SGL(pchan));
	pchan->qd_cb.func = pcb->func;
	pchan->qd_cb.parg = pcb->parg;
}

int __dma_enqueue(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	des_item *pdes_itm = NULL;
	u32 utemp = 0;

#ifdef NOT_ALLOC_DES_TEMP
	u32 offset = 0;
	offset = sizeof(des_item) * index_get;
	pdes_itm = (des_item *)(v_addr + offset);
	utemp = p_addr + offset;
	index_get++;
	if(index_get >= TEMP_DES_CNT)
		index_get = 0;
	if(index_get == index_put)
		printk("%s err: des buffer full, to check!", __func__);
#else
	pdes_itm = (des_item *)dma_pool_alloc(g_des_pool, GFP_ATOMIC, &utemp);
	if(NULL == pdes_itm) {
		WARN(1, "not enough memory\n");
		return -ENOMEM;
	}
#endif
	pdes_itm->des = *pdes;
	pdes_itm->paddr = utemp;

	list_add_tail(&pdes_itm->list, &pchan->list_single);

	if(SNGL_STA_LAST_DONE == STATE_SGL(pchan))
		__dma_start(dma_hdl);
	return 0;
}

void __handle_qd_sgmd(dma_channel_t *pchan)
{
	enum st_md_single_e cur_state = 0;
	unsigned long flags = 0;
	u32 utemp = 0;

	/* cannot lock fd_cb function, in case sw_dma_enqueue called and locked agin */
	if(NULL != pchan->qd_cb.func)
		pchan->qd_cb.func((dm_hdl_t)pchan, pchan->qd_cb.parg, DMA_CB_OK);

	DMA_CHAN_LOCK(&pchan->lock, flags);

	cur_state = STATE_SGL(pchan);

	/* stopped when hd_cb/fd_cb/qd_cb/somewhere calling? */
	if(likely(SNGL_STA_RUNING == cur_state)) {
		/* for continue mode, just re start the cur buffer */
		if(unlikely(true == pchan->bconti_mode)) {
			WARN_ON(!list_empty(&pchan->list_single));
			list_add_tail(&pchan->pcur_des->list, &pchan->list_single);
			__dma_start((dm_hdl_t)pchan);
			goto end;
		}

		/* for no-continue mode, free cur buf and start the next buf in chain */
		WARN_ON(NULL == pchan->pcur_des);
		utemp = pchan->pcur_des->paddr;
#ifndef NOT_ALLOC_DES_TEMP
		dma_pool_free(g_des_pool, pchan->pcur_des, utemp);
#else
		index_put++;
		if(index_put >= TEMP_DES_CNT)
			index_put = 0;
#endif
		pchan->pcur_des = NULL;

		/* start next if there is, or change state to last done */
		if(!list_empty(&pchan->list_single))
			__dma_start((dm_hdl_t)pchan);
		else
			STATE_SGL(pchan) = SNGL_STA_LAST_DONE;
	} else if(SNGL_STA_IDLE == cur_state)
		WARN_ON(NULL != pchan->pcur_des); /* stopped in cb before? */
	else /* should never be last done */
		BUG();

end:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

void dma_request_init_single(dma_channel_t *pchan)
{
	INIT_LIST_HEAD(&pchan->list_single);
	STATE_SGL(pchan) = SNGL_STA_IDLE;

	/* init for chain mode, incase err access by someone */
	INIT_LIST_HEAD(&pchan->list_chain);
}

/**
 * dma_release_single - release dma channel, for single mode
 * @dma_hdl:	dma handle
 */
void dma_release_single(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	unsigned long flags;

	DMA_CHAN_LOCK(&pchan->lock, flags);

#ifdef DBG_DMA
	if(0 != dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		return;
	}
#endif

	/* if not idle, call stop first */
	if(SNGL_STA_IDLE != STATE_SGL(pchan)) {
		DMA_INF("%s(%d): state(%d) not idle, stop dma first!\n", __func__, __LINE__, STATE_SGL(pchan));
		__dma_stop(dma_hdl);
	}

	//memset(pchan, 0, sizeof(*pchan)); /* donot do that, because id...shouldnot be cleared */

	pchan->used = 0;
	memset(pchan->owner, 0, sizeof(pchan->owner));
	pchan->irq_spt = CHAN_IRQ_NO;
	pchan->bconti_mode = false;
	memset(&pchan->des_info_save, 0, sizeof(pchan->des_info_save));
	pchan->work_mode = DMA_WORK_MODE_INVALID;
	memset(&pchan->op_cb, 0, sizeof(pchan->op_cb));
	memset(&pchan->hd_cb, 0, sizeof(pchan->hd_cb));
	memset(&pchan->fd_cb, 0, sizeof(pchan->fd_cb));
	memset(&pchan->qd_cb, 0, sizeof(pchan->qd_cb));

	/* maybe enqueued but not started, so free buf */
	WARN_ON(NULL != pchan->pcur_des);
	__dma_free_buflist(pchan);

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

/**
 * dma_ctrl_single - dma ctrl, for single mode
 * @dma_hdl:	dma handle
 * @op:		dma operation type
 * @parg:	arg for the op
 */
void dma_ctrl_single(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	unsigned long flags;

	/* only in start/stop/pause/resume case can parg be NULL  */
	if((NULL == parg)
		&& (DMA_OP_START != op)
		&& (DMA_OP_PAUSE != op)
		&& (DMA_OP_RESUME != op)
		&& (DMA_OP_STOP != op)) {
		DMA_ERR("%s err, line %d\n", __func__, __LINE__);
		return;
	}

	DMA_CHAN_LOCK(&pchan->lock, flags);

#ifdef DBG_DMA
	if(0 != dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		DMA_ERR("%s err, line %d\n", __func__, __LINE__);
		return;
	}
#endif
	/* let the caller to do some operation before op */
	if((DMA_OP_SET_OP_CB != op) && (NULL != pchan->op_cb.func))
		pchan->op_cb.func(dma_hdl, pchan->op_cb.parg, op);

	switch(op) {
	case DMA_OP_START:
		__dma_start(dma_hdl);
		break;
	case DMA_OP_PAUSE:
		__dma_pause(dma_hdl);
		break;
	case DMA_OP_RESUME:
		__dma_resume(dma_hdl);
		break;
	case DMA_OP_STOP:
		__dma_stop(dma_hdl);
		break;
	case DMA_OP_GET_STATUS:
		*(u32 *)parg = csp_dma_chan_get_status(pchan);
		break;
	case DMA_OP_GET_CUR_SRC_ADDR:
		*(u32 *)parg = csp_dma_chan_get_cur_srcaddr(pchan);
		break;
	case DMA_OP_GET_CUR_DST_ADDR:
		*(u32 *)parg = csp_dma_chan_get_cur_dstaddr(pchan);
		break;
	case DMA_OP_GET_BYTECNT_LEFT:
		*(u32 *)parg = csp_dma_chan_get_left_bytecnt(pchan);
		break;
	case DMA_OP_SET_OP_CB:
		__dma_set_op_cb(dma_hdl, (struct dma_op_cb_t *)parg);
		break;
	case DMA_OP_SET_HD_CB:
		__dma_set_hd_cb(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_FD_CB:
		__dma_set_fd_cb(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_QD_CB:
		__dma_set_qd_cb(dma_hdl, (struct dma_cb_t *)parg);
		break;
	default:
		WARN_ON(1);
		break;
	}

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

/**
 * dma_config_single - config dma channel, enqueue the buffer, for single mode only
 * @dma_hdl:	dma handle
 * @pcfg:	dma cofig para
 */
void dma_config_single(dm_hdl_t dma_hdl, struct dma_config_t *pcfg)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	struct cofig_des_t des;
	unsigned long flags;
	u32 uconfig = 0;

	/* get dma config val */
	uconfig |= xfer_arr[pcfg->xfer_type]; /* src/dst burst length and data width */
	uconfig |= addrtype_arr[pcfg->address_type]; /* src/dst address mode */
	uconfig |= (pcfg->src_drq_type << DMA_OFF_BITS_SDRQ)
			| (pcfg->dst_drq_type << DMA_OFF_BITS_DDRQ); /* src/dst drq type */

	/* fill cofig_des_t struct */
	memset(&des, 0, sizeof(des));
	des.cofig = uconfig;
	des.saddr = pcfg->src_addr;
	des.daddr = pcfg->dst_addr;
	des.bcnt  = pcfg->byte_cnt;
	des.param = pcfg->para;
	des.pnext = (struct cofig_des_t *)DMA_END_DES_LINK;
	/* get continue mode flag */
	pchan->bconti_mode = pcfg->bconti_mode;
	/* get irq surport type for channel handle */
	pchan->irq_spt = pcfg->irq_spt;
	/* bkup config/param */
	pchan->des_info_save.cofig = uconfig;
	pchan->des_info_save.param = pcfg->para;
	pchan->des_info_save.bconti_mode = pcfg->bconti_mode;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	if(true == pcfg->bconti_mode && !list_empty(&pchan->list_single)) {
		WARN(1, "can't enqueue more than one buf in continue mode\n");
		goto end;
	}

	csp_dma_chan_irq_enable(pchan, pcfg->irq_spt);

	WARN_ON(0 != __dma_enqueue(dma_hdl, &des));
end:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

/**
 * sw_dma_enqueue - enqueue the buffer, for single mode only
 * @dma_hdl:	dma handle
 * @src_addr:	buffer src phys addr
 * @dst_addr:	buffer dst phys addr
 * @byte_cnt:	buffer byte cnt
 */
void dma_enqueue_single(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	struct cofig_des_t des;
	unsigned long flags = 0;

	memset(&des, 0, sizeof(des));
	des.saddr 	= src_addr;
	des.daddr 	= dst_addr;
	des.bcnt 	= byte_cnt;
	des.cofig 	= pchan->des_info_save.cofig;
	des.param 	= pchan->des_info_save.param;
	des.pnext	= (struct cofig_des_t *)DMA_END_DES_LINK;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	if(true == pchan->des_info_save.bconti_mode && !list_empty(&pchan->list_single)) {
		WARN(1, "can't enqueue more than one buf in continue mode\n");
		goto end;
	}

	WARN_ON(0 != __dma_enqueue(dma_hdl, &des));
end:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

/**
 * dma_irq_hdl_sgmd - dma irq handle, for single mode only
 * @pchan:	dma channel handle
 * @upend_bits:	irq pending for the channel
 *
 */
void dma_irq_hdl_single(dma_channel_t *pchan, u32 upend_bits)
{
	u32	uirq_spt = 0;

	WARN_ON(0 == upend_bits);
	uirq_spt = pchan->irq_spt;

	if(upend_bits & CHAN_IRQ_HD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD);
		if((uirq_spt & CHAN_IRQ_HD) && NULL != pchan->hd_cb.func)
			pchan->hd_cb.func((dm_hdl_t)pchan, pchan->hd_cb.parg, DMA_CB_OK);
	}
	if(upend_bits & CHAN_IRQ_FD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_FD);
		if((uirq_spt & CHAN_IRQ_FD) && NULL != pchan->fd_cb.func)
			pchan->fd_cb.func((dm_hdl_t)pchan, pchan->fd_cb.parg, DMA_CB_OK);
	}
	if(upend_bits & CHAN_IRQ_QD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_QD);
		if(uirq_spt & CHAN_IRQ_QD)
			__handle_qd_sgmd(pchan);
	}
}

