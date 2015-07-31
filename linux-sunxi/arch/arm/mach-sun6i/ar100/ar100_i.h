/*
 *  arch/arm/mach-sun6i/ar100/ar100_i.h
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

#ifndef __AR100_I_H__
#define __AR100_I_H__

#include "./include/ar100_includes.h"
#include <mach/ar100.h>
//add by superm
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mach/sys_config.h>
#include <mach/system.h>
#include <asm/atomic.h>

#define DRV_NAME    "sun6i-ar100"
#define DEV_NAME    "sun6i-ar100"
#define DRV_VERSION "1.00"


//local functions
extern int ar100_config_dram_paras(void);
extern int ar100_config_ir_paras(void);
extern int ar100_config_pmu_paras(void);
extern int ar100_suspend_flag_query(void);

#endif  //__AR100_I_H__
