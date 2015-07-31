/*
 * arch/arm/mach-sun6i/dma/dma_interface.h
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

#ifndef __DMA_INTERFACE_H
#define __DMA_INTERFACE_H

extern struct dma_pool	 *g_des_pool;

extern unsigned long addrtype_arr[];
extern unsigned long xfer_arr[];

extern u32 dma_check_handle(dm_hdl_t dma_hdl);

#endif  /* __DMA_INTERFACE_H */

