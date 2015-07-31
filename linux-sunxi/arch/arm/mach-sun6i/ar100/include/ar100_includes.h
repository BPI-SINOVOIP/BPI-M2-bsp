/*
 *  arch/arm/mach-sun6i/ar100/include/ar100_includes.h
 *
 * Copyright (c) 2012 Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __AR100_INCLUDES_H
#define __AR100_INCLUDES_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <mach/irqs-sun6i.h>
#include <mach/hwmsgbox.h>
#include <mach/hwspinlock.h>
#include <mach/hardware.h>
#include <mach/platform.h>

/* configure and debugger */
#include "./ar100_cfgs.h"
#include "./ar100_dbgs.h"

/* messages define */
#include "./ar100_messages.h"
#include "./ar100_message_manager.h"

/* driver headers */
#include "./ar100_hwmsgbox.h"
#include "./ar100_hwspinlock.h"

/* global functions */
extern int ar100_axp_int_notify(struct ar100_message *pmessage);
extern int ar100_set_debug_level(unsigned int level);
extern int ar100_dvfs_cfg_vf_table(void);
extern int ar100_set_uart_baudrate(u32 baudrate);
extern int ar100_set_dram_crc_paras(unsigned int dram_crc_en, unsigned int dram_crc_srcaddr, unsigned int dram_crc_len);

/* global vars */
extern unsigned long ar100_sram_a2_vbase;

#endif /* __AR100_INCLUDES_H */
