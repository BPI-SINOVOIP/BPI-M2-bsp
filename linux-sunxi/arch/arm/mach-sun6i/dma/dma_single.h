/*
 * arch/arm/mach-sun6i/dma/dma_single.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_SINGLE_H
#define __DMA_SINGLE_H

void dma_irq_hdl_single(dma_channel_t *pchan, u32 upend_bits);
void dma_enqueue_single(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt);
void dma_config_single(dm_hdl_t dma_hdl, struct dma_config_t *pcfg);
void dma_ctrl_single(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg);
void dma_release_single(dm_hdl_t dma_hdl);
void dma_request_init_single(dma_channel_t *pchan);

#endif  /* __DMA_SINGLE_H */

