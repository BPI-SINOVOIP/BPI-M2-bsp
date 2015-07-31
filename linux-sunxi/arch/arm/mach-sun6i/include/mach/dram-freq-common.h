/*
 * arch/arm/mach-sun6i/include/mach/dram-freq-common.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __DRAM_FREQ_COMMON_H__
#define __DRAM_FREQ_COMMON_H__

#include <mach/platform.h>

#define MDFS_TABLE_LEN  (16)

#define CCM_PLL5_DDR_CTRL  		(AW_VIR_CCM_BASE      + 0x020)
#define CCM_DRAMCLK_CFG_CTRL	(AW_VIR_CCM_BASE      + 0x0f4)

#define SDR_COM_CR				(AW_VIR_DRAMCOM_BASE  +  0x00)
#define SDR_COM_MCGCR			(AW_VIR_DRAMCOM_BASE  +  0x8c)
#define SDR_COM_BWCR			(AW_VIR_DRAMCOM_BASE  +  0x90)
#define SDR_COM_MDFSCR			(AW_VIR_DRAMCOM_BASE  + 0x100)
#define SDR_COM_MDFSMER			(AW_VIR_DRAMCOM_BASE  + 0x104)
#define SDR_COM_MDFSMRMR		(AW_VIR_DRAMCOM_BASE  + 0x108)
#define SDR_COM_MDFSTR0			(AW_VIR_DRAMCOM_BASE  + 0x10c)
#define SDR_COM_MDFSTR1			(AW_VIR_DRAMCOM_BASE  + 0x110)
#define SDR_COM_MDFSTR2			(AW_VIR_DRAMCOM_BASE  + 0x114)
#define SDR_COM_MDFSGCR			(AW_VIR_DRAMCOM_BASE  + 0x11c)
#define SDR_PIR					(AW_VIR_DRAMPHY0_BASE +  0x04)
#define SDR_PGSR				(AW_VIR_DRAMPHY0_BASE +  0x0c)
#define SDR_DX0DQSTR 			(AW_VIR_DRAMPHY0_BASE + 0x1d4)
#define SDR_DX1DQSTR 			(AW_VIR_DRAMPHY0_BASE + 0x214)
#define SDR_DX2DQSTR 			(AW_VIR_DRAMPHY0_BASE + 0x254)
#define SDR_DX3DQSTR 			(AW_VIR_DRAMPHY0_BASE + 0x294)
#define SDR_SCTL				(AW_VIR_DRAMCTL0_BASE +  0x04)
#define SDR_SSTAT				(AW_VIR_DRAMCTL0_BASE +  0x08)
#define SDR_MCFG				(AW_VIR_DRAMCTL0_BASE +  0x80)
#define SDR_TOGCNT1U			(AW_VIR_DRAMCTL0_BASE +  0xc0)
#define SDR_TOGCNT100N			(AW_VIR_DRAMCTL0_BASE +  0xcc)

struct aw_mdfs_info {
    unsigned int is_dual_channel;
    unsigned int div;
    unsigned int table[MDFS_TABLE_LEN][8];
};

#endif /* __DRAM_FREQ_COMMON_H__ */
