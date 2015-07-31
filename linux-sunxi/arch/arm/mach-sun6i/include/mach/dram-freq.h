/*
 * arch/arm/mach-sun6i/include/mach/dram-freq.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __DRAM_FREQ_H__
#define __DRAM_FREQ_H__

#include <linux/devfreq.h>
#include <linux/platform_device.h>
#include <linux/notifier.h>
#include <mach/platform.h>
#include <mach/dram-freq-common.h>

#define SRAM_MDFS_START         (0xf0000000)

#define DRAMFREQ_PRECHANGE	(0)
#define DRAMFREQ_POSTCHANGE	(1)

#define SUN6I_DRAMFREQ_MAX			(312000000)	/* config the maximum frequency of sun6i dram */
#define SUN6I_DRAMFREQ_MIN			 (39000000)	/* config the minimum frequency of sun6i dram */
#define SUN6I_DRAMFREQ_POLLING_MS        (1000)	/* config the polling interval, based on ms   */
#define SUN6I_DRAMFREQ_TABLE_SIZE          (16) /* mdfs table size */
#define MASTER_INFO_SIZE                   (28) /* sum of all master */

struct dramfreq_frequency_table {
	unsigned int frequency; /* kHz */
	unsigned int dram_div;  /* dram div factor for dram clk */
};

enum master_type {
    MASTER_CPUX  = 0,
    MASTER_GPU0  = 1,
    MASTER_GPU1  = 2,
    MASTER_CPUS  = 4,
    MASTER_ATH   = 5,
    MASTER_GMAC  = 6,
    MASTER_SDC0  = 7,
    MASTER_SDC1  = 8,
    MASTER_SDC2  = 9,
    MASTER_SDC3 = 10,
    MASTER_USB  = 11,
    MASTER_NFC1 = 15,
    MASTER_DMAC = 16,
    MASTER_VE   = 17,
    MASTER_MP   = 18,
    MASTER_NFC0 = 19,
    MASTER_DRC0 = 20,
    MASTER_DRC1 = 21,
    MASTER_DEU0 = 22,
    MASTER_DEU1 = 23,
    MASTER_BE0  = 24,
    MASTER_FE0  = 25,
    MASTER_BE1  = 26,
    MASTER_FE1  = 27,
    MASTER_CSI0 = 28,
    MASTER_CSI1 = 29,
    MASTER_TS   = 30,
    MASTER_ALL  = 32,
    MASTER_MAX  = 33,
};

struct master_info {
    enum master_type type;
    char *name;
};

struct master_bw_table {
    enum master_type type;
    unsigned int bw_need;
};

struct dramfreq_udata {
	enum master_type user_type;
	unsigned long freq_to_user;	/* current dram frequency backup, used to notify user dram DVS is ok or not */
	unsigned long freq_manual;	/* manual set dram freq, only when dramfreq_auto_scaling=0 valid */
	unsigned long freq_to_max;	/* set dram frequency to max */
};

extern struct dramfreq_frequency_table sun6i_dramfreq_tbl[];
extern struct platform_device sun6i_dramfreq_device;
extern int dramfreq_user_notify_enable;
extern int dramfreq_auto_scaling;
extern struct master_info master_info_list[MASTER_INFO_SIZE];

extern int dramfreq_register_notifier(struct notifier_block *nb);
extern int dramfreq_unregister_notifier(struct notifier_block *nb);
extern unsigned long dramfreq_get(void);

#endif /* __DRAM_FREQ_H__ */
