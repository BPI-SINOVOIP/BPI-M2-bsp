/*
 * sound\soc\sun6i\pcm\sun6i-pcmdma.h
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef SUN6I_PCMDMA_H_
#define SUN6I_PCMDMA_H_

#undef PCM_DBG
#if (0)
    #define PCM_DBG(format,args...)  printk("[SWITCH] "format,##args)    
#else
    #define PCM_DBG(...)    
#endif

struct sun6i_dma_params {
	char *name;		
	dma_addr_t dma_addr;	
};

#endif
