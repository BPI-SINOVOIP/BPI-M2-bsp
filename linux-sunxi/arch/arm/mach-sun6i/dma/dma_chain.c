/*
 * arch/arm/mach-sun6i/dma/dma_chain_new.c
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

void __free_dest_list(struct list_head *plist)
{
 	des_item *pcur = NULL, *n = NULL;

	/* free all buf on the list */
	list_for_each_entry_safe(pcur, n, plist, list)
		dma_pool_free(g_des_pool, pcur, pcur->paddr);

	INIT_LIST_HEAD(plist);
}

void __dma_start_chain(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	des_item *pitem = NULL;

	BUG_ON(unlikely(list_empty(&pchan->list_chain)));

	/* get the first item from list_chain */
	pitem = list_first_entry(&pchan->list_chain, des_item, list);

	/* write the item's paddr to start reg; start dma */
	csp_dma_chan_set_startaddr(pchan, pitem->paddr);
	csp_dma_chan_start(pchan);
	STATE_CHAIN(pchan) = CHAN_STA_RUNING;
}

void __dma_stop_chain(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	/* stop dma channle and clear irq pending */
	csp_dma_chan_stop(pchan);
	csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD);

	/* free all buffer */
	__free_dest_list(&pchan->list_chain);

	/* change channel state to idle */
	STATE_CHAIN(pchan) = CHAN_STA_IDLE;
}

void __dma_pause_chain(dm_hdl_t dma_hdl)
{
	csp_dma_chan_pause((dma_channel_t *)dma_hdl);
}

void __dma_resume_chain(dm_hdl_t dma_hdl)
{
	csp_dma_chan_resume((dma_channel_t *)dma_hdl);
}

void __dma_set_op_cb_chain(dm_hdl_t dma_hdl, struct dma_op_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->op_cb.func = pcb->func;
	pchan->op_cb.parg = pcb->parg;
}

void __dma_set_hd_cb_chain(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->hd_cb.func = pcb->func;
	pchan->hd_cb.parg = pcb->parg;
}

void __dma_set_fd_cb_chain(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->fd_cb.func = pcb->func;
	pchan->fd_cb.parg = pcb->parg;
}

void __dma_set_qd_cb_chain(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	WARN_ON(CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->qd_cb.func = pcb->func;
	pchan->qd_cb.parg = pcb->parg;
}

void __dam_chain_enqueue(dma_channel_t *pchan, struct cofig_des_t *pdes)
{
	des_item *pdes_temp, *pdes_last;
	unsigned long flags;
	u32 paddr;

	pdes_temp = (des_item *)dma_pool_alloc(g_des_pool, GFP_ATOMIC, &paddr);
	if(NULL == pdes_temp) {
		WARN(1, "not enough memory\n");
		return;
	}

	pdes_temp->des = *pdes;
	pdes_temp->paddr = paddr;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	if(unlikely(true == pchan->des_info_save.bconti_mode)) {
		WARN_ON(unlikely(!list_empty(&pchan->list_chain)));
		pdes_temp->des.pnext = (struct cofig_des_t *)paddr; /* link to itself */
	}

	if(CHAN_STA_IDLE == STATE_CHAIN(pchan)) {
		/* just add to list, not start dma */
		if(!list_empty(&pchan->list_chain)) {
			pdes_last = (des_item *)list_entry(pchan->list_chain.prev, des_item, list);
			pdes_last->des.pnext = (struct cofig_des_t *)paddr;
		}
		list_add_tail(&pdes_temp->list, &pchan->list_chain);
		goto end;
	}

	/* if state is CHAN_STA_RUNNING, add to list and make sure the buffer be transferred by hw */
	if(DMA_END_DES_LINK != csp_dma_chan_get_startaddr(pchan)) {
		/* change last->link to cur, add to list */
		if(!list_empty(&pchan->list_chain)) {
			pdes_last = (des_item *)list_entry(pchan->list_chain.prev, des_item, list);
			pdes_last->des.pnext = (struct cofig_des_t *)paddr;
		}
		list_add_tail(&pdes_temp->list, &pchan->list_chain);

		/* if startaddr not 0xfffff800 then ok; else set startaddr to newbuf, and enable */
		if(unlikely(DMA_END_DES_LINK == csp_dma_chan_get_startaddr(pchan))) {
			printk("%s: aha\n", __func__); /* just the point */
			csp_dma_chan_set_startaddr(pchan, paddr);
			csp_dma_chan_start(pchan);
		}
	} else {
		/* change last->link to cur, add to list */
		if(!list_empty(&pchan->list_chain)) {
			pdes_last = (des_item *)list_entry(pchan->list_chain.prev, des_item, list);
			pdes_last->des.pnext = (struct cofig_des_t *)paddr;
		}
		list_add_tail(&pdes_temp->list, &pchan->list_chain);

		csp_dma_chan_set_startaddr(pchan, paddr);
		csp_dma_chan_start(pchan);
	}
end:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

void dma_dump_chain(dma_channel_t *pchan)
{
	des_item *pitem = NULL;

	BUG_ON(NULL == pchan);

	printk("+++++++++++%s+++++++++++\n", __func__);
	printk("  channel id:        %d\n", pchan->id);
	printk("  channel used:      %d\n", pchan->used);
	printk("  channel owner:     %s\n", pchan->owner);
	printk("  bconti_mode:       %d\n", pchan->bconti_mode);
	printk("  channel irq_spt:   0x%08x\n", pchan->irq_spt);
	printk("  channel reg_base:  0x%08x\n", pchan->reg_base);
	printk("          EN REG:             0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_EN));
	printk("          PAUSE REG:          0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_PAUSE));
	printk("          START REG:          0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_START));
	printk("          CONFIG REG:         0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CFG));
	printk("          CUR SRC REG:        0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_SRC));
	printk("          CUR DST REG:        0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_DST));
	printk("          BYTE CNT LEFT REG:  0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_BCNT_LEFT));
	printk("          PARA REG:           0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_PARA));
	printk("  channel hd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->hd_cb.func, (u32)pchan->hd_cb.parg);
	printk("  channel fd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->fd_cb.func, (u32)pchan->fd_cb.parg);
	printk("  channel qd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->qd_cb.func, (u32)pchan->qd_cb.parg);
	printk("  channel op_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->op_cb.func, (u32)pchan->op_cb.parg);
	printk("  channel des_info_save:  (cofig: 0x%08x, param: 0x%08x, bconti_mode %d)\n", pchan->des_info_save.cofig,
		pchan->des_info_save.param, pchan->des_info_save.bconti_mode);
	if(DMA_WORK_MODE_CHAIN == pchan->work_mode) { /* chain mode */
		/* dump cur des chain */
		printk("  channel cur des buf chain:\n");
		list_for_each_entry(pitem, &pchan->list_chain, list) {
			printk("   pitem: 0x%08x, &pitem->list 0x%08x, &pchan->list_chain 0x%08x\n", (u32)pitem, (u32)&pitem->list, (u32)&pchan->list_chain);
			printk("   paddr:        0x%08x\n", (u32)pitem->paddr);
			printk("   cofig/saddr/daddr/bcnt/param/pnext: 0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/\n",
				pitem->des.cofig, pitem->des.saddr, pitem->des.daddr, pitem->des.bcnt, pitem->des.param, (u32)pitem->des.pnext);
		}
		printk("  channel state:     0x%08x\n", (u32)STATE_CHAIN(pchan));
	} else { /* single mode */
		list_for_each_entry(pitem, &pchan->list_single, list) {
			printk("   pitem: 0x%08x, &pitem->list 0x%08x, &pchan->list_chain 0x%08x\n", (u32)pitem, (u32)&pitem->list, (u32)&pchan->list_chain);
			printk("   paddr:        0x%08x\n", (u32)pitem->paddr);
			printk("   cofig/saddr/daddr/bcnt/param/pnext: 0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/\n",
				pitem->des.cofig, pitem->des.saddr, pitem->des.daddr, pitem->des.bcnt, pitem->des.param, (u32)pitem->des.pnext);
		}
		printk("  channel state:     0x%08x\n", (u32)STATE_SGL(pchan));
	}
	printk("-----------%s-----------\n", __func__);
}

void dma_enqueue_chain(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	struct cofig_des_t des;
	unsigned long flags;
	bool warn = false;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	if(unlikely(true == pchan->des_info_save.bconti_mode && !list_empty(&pchan->list_chain)))
		warn = true;
	DMA_CHAN_UNLOCK(&pchan->lock, flags);

	if(warn) {
		WARN(1, "can't enqueu more than one buf in chain-conti mode\n");
		return;
	}

	memset(&des, 0, sizeof(des));
	des.saddr 	= src_addr;
	des.daddr 	= dst_addr;
	des.bcnt 	= byte_cnt;
	des.cofig 	= pchan->des_info_save.cofig;
	des.param 	= pchan->des_info_save.param;
	des.pnext	= (struct cofig_des_t *)DMA_END_DES_LINK;
	__dam_chain_enqueue(dma_hdl, &des);
}

void dma_config_chain(dm_hdl_t dma_hdl, struct dma_config_t *pcfg)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	struct cofig_des_t des;
	unsigned long flags;
	u32 uconfig = 0;
	bool warn = false;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	WARN_ON(CHAN_STA_IDLE != STATE_CHAIN(pchan));

	if(unlikely(true == pchan->des_info_save.bconti_mode && !list_empty(&pchan->list_chain)))
		warn = true;
	DMA_CHAN_UNLOCK(&pchan->lock, flags);

	if(warn) {
		WARN(1, "can't enqueu more than one buf in chain-conti mode\n");
		return;
	}

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

	/* irq enable */
	csp_dma_chan_irq_enable(pchan, pcfg->irq_spt);
	__dam_chain_enqueue(dma_hdl, &des);
}

void dma_ctrl_chain(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	unsigned long flags;

	/* only in start/stop/pause/resume case can parg be NULL  */
	if((NULL == parg)
		&& (DMA_OP_START != op) && (DMA_OP_PAUSE != op)
		&& (DMA_OP_RESUME != op) && (DMA_OP_STOP != op)) {
		WARN(1, "args invalid\n");
		return;
	}

	DMA_CHAN_LOCK(&pchan->lock, flags);

	if((DMA_OP_SET_OP_CB != op) && (NULL != pchan->op_cb.func))
		pchan->op_cb.func(dma_hdl, pchan->op_cb.parg, op);

	switch(op) {
	/* dma hw control */
	case DMA_OP_START:
		__dma_start_chain(dma_hdl);
		break;
	case DMA_OP_PAUSE:
		__dma_pause_chain(dma_hdl);
		break;
	case DMA_OP_RESUME:
		__dma_resume_chain(dma_hdl);
		break;
	case DMA_OP_STOP:
		__dma_stop_chain(dma_hdl);
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
	/* set callback */
	case DMA_OP_SET_OP_CB:
		__dma_set_op_cb_chain(dma_hdl, (struct dma_op_cb_t *)parg);
		break;
	case DMA_OP_SET_HD_CB:
		__dma_set_hd_cb_chain(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_FD_CB:
		__dma_set_fd_cb_chain(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_QD_CB:
		__dma_set_qd_cb_chain(dma_hdl, (struct dma_cb_t *)parg);
		break;
	default:
		WARN_ON(1);
		break;
	}

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

void dma_release_chain(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	unsigned long flags = 0;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	/* if not idle, call stop */
	if(CHAN_STA_IDLE != STATE_CHAIN(pchan))
		__dma_stop_chain(dma_hdl);

	/* assert all buf freed */
	WARN_ON(!list_empty(&pchan->list_chain));

	//memset(pchan, 0, sizeof(*pchan)); /* donot do that, because id...shouldnot be cleared */
	pchan->used = 0;
	pchan->irq_spt = CHAN_IRQ_NO;
	pchan->bconti_mode = false;
	memset(pchan->owner, 0, sizeof(pchan->owner));
	memset(&pchan->des_info_save, 0, sizeof(pchan->des_info_save));
	memset(&pchan->op_cb, 0, sizeof(pchan->op_cb));
	memset(&pchan->hd_cb, 0, sizeof(pchan->hd_cb));
	memset(&pchan->fd_cb, 0, sizeof(pchan->fd_cb));
	memset(&pchan->qd_cb, 0, sizeof(pchan->qd_cb));

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

void dma_request_init_chain(dma_channel_t *pchan)
{
	INIT_LIST_HEAD(&pchan->list_chain);
	STATE_CHAIN(pchan) = CHAN_STA_IDLE;

	/* init for single mode, incase err access by someone */
	INIT_LIST_HEAD(&pchan->list_single);
}

void dma_irq_hdl_chain(dma_channel_t *pchan, u32 upend_bits)
{
	u32 uirq_spt;
	unsigned long flags;
	des_item *pdes_item;
	bool need_free = false;

	uirq_spt = pchan->irq_spt;
	if(upend_bits & CHAN_IRQ_HD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD);

		if((uirq_spt & CHAN_IRQ_HD) && (NULL != pchan->hd_cb.func))
			pchan->hd_cb.func((dm_hdl_t)pchan, pchan->hd_cb.parg, DMA_CB_OK);
	}
	if(upend_bits & CHAN_IRQ_FD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_FD);

		if((uirq_spt & CHAN_IRQ_FD) && (NULL != pchan->fd_cb.func))
			pchan->fd_cb.func((dm_hdl_t)pchan, pchan->fd_cb.parg, DMA_CB_OK);
		if(likely(false == pchan->des_info_save.bconti_mode))
			need_free = true;
	}
	if(upend_bits & CHAN_IRQ_QD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_QD);

		if((uirq_spt & CHAN_IRQ_QD) && (NULL != pchan->qd_cb.func))
			pchan->qd_cb.func((dm_hdl_t)pchan, pchan->qd_cb.parg, DMA_CB_OK);
	}

	if(false == need_free)
		return;

	DMA_CHAN_LOCK(&pchan->lock, flags);
	if(list_empty(&pchan->list_chain)) { /* stopped during irq handle before */
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		return;
	}
	pdes_item = list_entry(pchan->list_chain.next, des_item, list);
	__list_del_entry(pchan->list_chain.next);
	DMA_CHAN_UNLOCK(&pchan->lock, flags);

	dma_pool_free(g_des_pool, pdes_item, pdes_item->paddr);
}

