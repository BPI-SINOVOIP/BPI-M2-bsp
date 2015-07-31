/*
 * arch/arm/mach-sun6i/dma/dma_interface.c
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

dma_channel_t dma_chnl[DMA_CHAN_TOTAL];

/* lock for request */
static DEFINE_MUTEX(dma_mutex);

/* dma descriptor buf pool */
struct dma_pool	*g_des_pool = NULL;

#ifdef NOT_ALLOC_DES_TEMP
u32 index_get = 0, index_put = 0;
u32 v_addr = 0, p_addr = 0;
#endif /* NOT_ALLOC_DES_TEMP */

/* data length and burst length value in config reg */
unsigned long xfer_arr[DMAXFER_MAX] =
{
	/* des:X_SIGLE  src:X_SIGLE */
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_WORD << 9),

	/* des:X_SIGLE   src:X_BURST */
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_WORD << 9),

	/* des:X_SIGLE   src:X_TIPPL */
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_WORD << 9),

	/* des:X_BURST  src:X_BURST */
	(X_BURST << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_WORD << 9),

	/* des:X_BURST   src:X_SIGLE */
	(X_BURST << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_WORD << 9),

	/* des:X_BURST   src:X_TIPPL */
	(X_BURST << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_WORD << 9),

	/* des:X_TIPPL   src:X_TIPPL */
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_WORD << 9),

	/* des:X_TIPPL   src:X_SIGLE */
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_WORD << 9),

	/* des:X_TIPPL   src:X_BURST */
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_WORD << 9),
};

/* src/dst address type value in config reg */
unsigned long addrtype_arr[DMAADDRT_MAX] =
{
	(A_LN  << 21) | (A_LN  << 5),
	(A_LN  << 21) | (A_IO  << 5),
	(A_IO  << 21) | (A_LN  << 5),
	(A_IO  << 21) | (A_IO  << 5),
};

#if 0 /* remove warning: defined but not used */
/**
 * __dma_dump_config_para - dump dma_config_t struct
 * @para:	dma_config_t struct to dump
 */
static void __dma_dump_config_para(struct dma_config_t *para)
{
	if(NULL == para) {
		DMA_ERR("%s err, line %d\n", __func__, __LINE__);
		return;
	}

	DMA_DBG("+++++++++++%s+++++++++++\n", __func__);
	DMA_DBG("  xfer_type:         %d\n", para->xfer_type);
	DMA_DBG("  address_type:      %d\n", para->address_type);
	DMA_DBG("  para:              0x%08x\n", para->para);
	DMA_DBG("  irq_spt:           %d\n", para->irq_spt);
	DMA_DBG("  src_addr:          0x%08x\n", para->src_addr);
	DMA_DBG("  dst_addr:          0x%08x\n", para->dst_addr);
	DMA_DBG("  byte_cnt:          0x%08x\n", para->byte_cnt);
	DMA_DBG("  bconti_mode:       %d\n", para->bconti_mode);
	DMA_DBG("  src_drq_type:      %d\n", para->src_drq_type);
	DMA_DBG("  dst_drq_type:      %d\n", para->dst_drq_type);
	DMA_DBG("-----------%s-----------\n", __func__);
}
#endif

/**
 * dma_check_handle - check if dma handle is valid
 * @dma_hdl:	dma handle
 *
 * return 0 if vaild, the err line number if not vaild
 */
u32 dma_check_handle(dm_hdl_t dma_hdl)
{
	u32	uret = 0;
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	if(NULL == pchan) {
		uret = __LINE__;
		goto end;
	}
	if(0 == pchan->used) { /* already released? */
		uret = __LINE__;
		goto end;
	}
	if(DMA_WORK_MODE_CHAIN != pchan->work_mode && DMA_WORK_MODE_SINGLE != pchan->work_mode) {
		uret = __LINE__;
		goto end;
	}

end:
	if(0 != uret)
		DMA_ERR("%s err, line %d\n", __func__, uret);
	return uret;
}

#ifdef DBG_DMA
/**
 * __dma_check_channel_free - check if channel is free
 * @pchan:	dma handle
 *
 * return true if channel is free, false if not
 *
 * NOTE: can only be called in sw_dma_request recently, becase
 * should be locked
 */
static u32 __dma_check_channel_free(dma_channel_t *pchan)
{
	if(0 == pchan->used
		&& 0 == pchan->owner[0]
		//&& CHAN_IRQ_NO == pchan->irq_spt /* maybe not use dma irq? */
		&& NULL == pchan->hd_cb.func
		&& NULL == pchan->fd_cb.func
		&& NULL == pchan->qd_cb.func
		&& NULL == pchan->op_cb.func
		&& CHAN_STA_IDLE == STATE_CHAIN(pchan)
		&& DMA_WORK_MODE_INVALID == pchan->work_mode
		)
		return true;
	else {
		dma_dump_chain(pchan);
		return false;
	}
}
#endif /* DBG_DMA */

/**
 * __dma_channel_already_exist - check if channel already requested by others
 * @name:	channel name
 *
 * return true if channel already requested, false if not
 */
bool __dma_channel_already_exist(char *name)
{
	u32 i = 0;

	if(NULL == name)
		return false;
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		if(1 == dma_chnl[i].used && !strcmp(dma_chnl[i].owner, name))
			return true;
	}
	return false;
}

/**
 * sw_dma_request - request a dma channel
 * @name:	dma channel name
 *
 * Returns handle to the channel if success, NULL if failed.
 */
dm_hdl_t sw_dma_request(char *name, enum dma_work_mode_e work_mode)
{
	u32	i = 0;
	u32	usign = 0;
	dma_channel_t *pchan = NULL;

	DMA_DBG("%s: name %s, work_mode %d\n", __func__, name, (u32)work_mode);
	if(strlen(name) >= MAX_OWNER_NAME_LEN || (work_mode != DMA_WORK_MODE_CHAIN && work_mode != DMA_WORK_MODE_SINGLE)) {
		DMA_ERR("%s: para err, name %s, work mode %d\n", __func__, name, (u32)work_mode);
		return NULL;
	}

	mutex_lock(&dma_mutex);

	/* check if already exist */
	if(NULL != name && __dma_channel_already_exist(name)) {
		usign = __LINE__;
		goto end;
	}
	/* get a free channel */
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &dma_chnl[i];
		if(0 == pchan->used) {
#ifdef DBG_DMA
			if(true != __dma_check_channel_free(pchan))
				DMA_ERR("%s(%d) err, channel is not free\n", __func__, __LINE__);
#endif /* DBG_DMA */
			break;
		}
	}
	/* cannot get a free channel */
	if(DMA_CHAN_TOTAL == i) {
		usign = __LINE__;
		goto end;
	}

	/* init channel */
	if(DMA_WORK_MODE_CHAIN == work_mode) {
		dma_request_init_chain(pchan);
	} else if(DMA_WORK_MODE_SINGLE == work_mode) {
		dma_request_init_single(pchan);
#ifdef NOT_ALLOC_DES_TEMP
		v_addr = (u32)dma_alloc_coherent(NULL, TEMP_DES_CNT * sizeof(des_item), (dma_addr_t *)&p_addr, GFP_KERNEL);
		if(0 == v_addr)
			printk("%s err, dma_alloc_coherent failed, line %d\n", __func__, __LINE__);
		else
			printk("%s: dma_alloc_coherent return v_addr 0x%08x, p_addr 0x%08x\n", __func__, v_addr, p_addr);
		index_get = 0;
		index_put = 0;
#endif /* NOT_ALLOC_DES_TEMP */
	}
	pchan->used = 1;
	if(NULL != name)
		strcpy(pchan->owner, name);
	pchan->work_mode = work_mode;

end:
	mutex_unlock(&dma_mutex);
	if(0 != usign) {
		DMA_ERR("%s err, line %d\n", __func__, usign);
		return (dm_hdl_t)NULL;
	} else {
		DMA_DBG("%s: success, channel id %d\n", __func__, i);
		return (dm_hdl_t)pchan;
	}
}
EXPORT_SYMBOL(sw_dma_request);

/**
 * sw_dma_release - free a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, other value if failed.
 */
u32 sw_dma_release(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	BUG_ON(unlikely(NULL == pchan));
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
		dma_release_single(dma_hdl);
	else
		dma_release_chain(dma_hdl);
	return 0;
}
EXPORT_SYMBOL(sw_dma_release);

/**
 * sw_dma_ctl - dma ctrl operation
 * @dma_hdl:	dma handle
 * @op:		dma operation type
 * @parg:	arg for the op
 *
 */
u32 sw_dma_ctl(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	BUG_ON(unlikely(NULL == pchan));
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
		dma_ctrl_single(dma_hdl, op, parg);
	else
		dma_ctrl_chain(dma_hdl, op, parg);
	return 0;
}
EXPORT_SYMBOL(sw_dma_ctl);

/**
 * sw_dma_config - config dma channel, enqueue the buffer
 * @dma_hdl:	dma handle
 * @pcfg:	dma cofig para
 * @phase:	dma enqueue phase
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_dma_config(dm_hdl_t dma_hdl, struct dma_config_t *pcfg, enum dma_enque_phase_e phase)
{
	dma_channel_t	*pchan = (dma_channel_t *)dma_hdl;

	BUG_ON(unlikely(NULL == pchan));
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
		dma_config_single(dma_hdl, pcfg);
	else
		dma_config_chain(dma_hdl, pcfg);
	return 0;
}
EXPORT_SYMBOL(sw_dma_config);

/**
 * sw_dma_enqueue - enqueue the buffer to des chain
 * @dma_hdl:	dma handle
 * @src_addr:	buffer src phys addr
 * @dst_addr:	buffer dst phys addr
 * @byte_cnt:	buffer byte cnt
 * @phase:	enqueue phase
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_dma_enqueue(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt,
				enum dma_enque_phase_e phase)
{
	dma_channel_t 	*pchan = (dma_channel_t *)dma_hdl;

	BUG_ON(unlikely(NULL == pchan));
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
		dma_enqueue_single(dma_hdl, src_addr, dst_addr, byte_cnt);
	else
		dma_enqueue_chain(dma_hdl, src_addr, dst_addr, byte_cnt);
	return 0;
}
EXPORT_SYMBOL(sw_dma_enqueue);

/**
 * sw_dma_getposition - get the src and dst address from the reg
 * @dma_hdl:	dma handle
 * @psrc:	pointed to src addr that will be got
 * @pdst:	pointed to dst addr that will be got
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int sw_dma_getposition(dm_hdl_t dma_hdl, u32 *psrc, u32 *pdst)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;

	if(NULL == dma_hdl || NULL == psrc || NULL == pdst) {
		DMA_ERR("%s err, line %d\n", __func__, __LINE__);
		return __LINE__;
	}
	if(0 == pchan->used) {
		DMA_ERR("%s err, line %d\n", __func__, __LINE__);
		return __LINE__;
	}

	*psrc = csp_dma_chan_get_cur_srcaddr(pchan);
	*pdst = csp_dma_chan_get_cur_dstaddr(pchan);
	DMA_DBG("%s: get *psrc 0x%08x, *pdst 0x%08x\n", __func__, *psrc, *pdst);
	return 0;
}
EXPORT_SYMBOL(sw_dma_getposition);

/**
 * sw_dma_dump_chan - dump dma chain
 * @dma_hdl:	dma handle
 */
void sw_dma_dump_chan(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = (dma_channel_t *)dma_hdl;
	unsigned long	flags = 0;

	BUG_ON(unlikely(NULL == pchan));

	DMA_CHAN_LOCK(&pchan->lock, flags);
	dma_dump_chain(pchan);
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}
EXPORT_SYMBOL(sw_dma_dump_chan);

