/*
 * sound\soc\sun6i\hdmiaudio\sun6i-hdmipcm.h
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

#ifndef SUN6I_HDMIPCM_H_
#define SUN6I_HDMIPCM_H_

#define SUN6I_HDMIBASE 		0x01c16000
#define SUN6I_HDMIAUDIO_TX	0x400

struct sun6i_dma_params {
	char *name;
	dma_addr_t dma_addr;
};
#endif /*SUN6I_HDMIPCM_H_*/
