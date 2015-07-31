/*
 * arch/arm/mach-sun6i/dma/dma_common.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma common header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_COMMON_H
#define __DMA_COMMON_H

#include <linux/spinlock.h>

#define DMA_DBG_LEVEL		3

#if (DMA_DBG_LEVEL == 1)
	#define DMA_DBG(format,args...)   printk("[dma-dbg] "format,##args)
	#define DMA_INF(format,args...)   printk("[dma-inf] "format,##args)
	#define DMA_ERR(format,args...)   printk("[dma-err] "format,##args)
#elif (DMA_DBG_LEVEL == 2)
	#define DMA_DBG(format,args...)   do{}while(0)
	#define DMA_INF(format,args...)   printk("[dma-inf] "format,##args)
	#define DMA_ERR(format,args...)   printk("[dma-err] "format,##args)
#elif (DMA_DBG_LEVEL == 3)
	#define DMA_DBG(format,args...)   do{}while(0)
	#define DMA_INF(format,args...)   do{}while(0)
	#define DMA_ERR(format,args...)   printk("[dma-err] "format,##args)
#endif

/* dma channel total */
#define DMA_CHAN_TOTAL		(16)

/* dma channel owner name max len */
#define MAX_OWNER_NAME_LEN	32

/* dma end des link */
#define DMA_END_DES_LINK	0xFFFFF800

/* dam channel state, software state */
enum st_md_chain_e {
	CHAN_STA_IDLE,  	/* maybe before start or after stop */
	CHAN_STA_RUNING,	/* transferring */
};

/* dam channel state for single mode */
enum st_md_single_e {
	SNGL_STA_IDLE,  	/* maybe before start or after stop */
	SNGL_STA_RUNING,	/* transferring */
	SNGL_STA_LAST_DONE	/* the last buffer has done,
				 * in this state, any enqueueing will start dma
				 */
};

/* define dma config descriptor struct for hardware */
struct cofig_des_t {
	u32		cofig;		/* dma configuration reg */
	u32		saddr;		/* dma src addr reg */
	u32		daddr;		/* dma dst addr reg */
	u32		bcnt;		/* dma byte cnt reg */
	u32		param;		/* dma param reg */
	struct cofig_des_t *pnext;	/* next descriptor address */
};

/* descriptor item define */
typedef struct __des_item {
	struct cofig_des_t	des;		/* descriptor that will be set to hw */
	u32			paddr;		/* physical addr of this __des_item struct */
	struct list_head 	list;		/* list node */
}des_item;

/* define dma config/param info, for dma_channel_t.des_info_save */
struct des_save_info_t {
	u32		cofig;     	/* dma configuration reg */
	u32		param;     	/* dma param reg */
	u32		bconti_mode;    /* if dma transfer in continue mode */
};

/* dma channle state */
union dma_chan_sta_u {
	enum st_md_chain_e 	st_md_ch;	/* channel state for chain mode */
	enum st_md_single_e 	st_md_sg;	/* channel state for single mode */
};

/* define dma channel struct */
typedef struct {
	u32		used;     	/* 1 used, 0 unuse */
	u32		id;     	/* channel id, 0~15 */
	char 		owner[MAX_OWNER_NAME_LEN];	/* dma chnnnel owner name */
	u32		reg_base;	/* regs base addr */
	u32		bconti_mode;	/* cotinue mode */
	u32 		irq_spt;	/* channel irq supprot type, used for irq handler only enabled then can call irq callback */
	struct dma_cb_t		hd_cb;		/* half done call back func */
	struct dma_cb_t		fd_cb;		/* full done call back func */
	struct dma_cb_t		qd_cb;		/* queue done call back func */
	struct dma_op_cb_t	op_cb;		/* dma operation call back func */
	struct des_save_info_t	des_info_save;	/* save the prev buf para, used by sw_dma_enqueue */
	enum dma_work_mode_e	work_mode;
	union dma_chan_sta_u	state;		/* channel state for chain/single mode */
	spinlock_t 		lock;		/* dma channel lock */

	/* for chain mode only */
	struct list_head 	list_chain;	/* buf list which is tranferring */

	/* for single mode only */
	des_item		*pcur_des;	/* cur buffer which is transferring */
	struct list_head 	list_single;
}dma_channel_t;

#define STATE_CHAIN(dma_hdl)	(((dma_channel_t *)(dma_hdl))->state.st_md_ch)
#define STATE_SGL(dma_hdl)	(((dma_channel_t *)(dma_hdl))->state.st_md_sg)

extern dma_channel_t dma_chnl[DMA_CHAN_TOTAL];

/* dma channel lock */
#define DMA_CHAN_LOCK_INIT(lock)	spin_lock_init((lock))
#define DMA_CHAN_LOCK_DEINIT(lock)	do{}while(0)
#define DMA_CHAN_LOCK(lock, flag)	spin_lock_irqsave((lock), (flag))
#define DMA_CHAN_UNLOCK(lock, flag)	spin_unlock_irqrestore((lock), (flag))

#endif  /* __DMA_COMMON_H */

