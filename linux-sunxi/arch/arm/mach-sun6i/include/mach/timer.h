/*
 * arch\arm\mach-sun6i\timer.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.reuuimllatech.com>
 * chenpailin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __AW_TIMER_H__
#define __AW_TIMER_H__
#include <mach/irqs-sun6i.h>

#ifdef CONFIG_AW_ASIC_EVB_PLATFORM

#define AW_CLOCK_SRC (24000000)

#else 

#define AW_CLOCK_SRC (32000)

#endif

#define AW_CLOCK_DIV (1)

/*
* 1 cycle = 1/(AW_CLOCK_SRC)s
* tickrate= 1cycle* (AW_CLOCK_SRC/(DIV*HZ)
* TICKRATE = 10ms
*/
#define TIMER0_VALUE (AW_CLOCK_SRC / (AW_CLOCK_DIV*100))
#define CLOCK_TICK_RATE   TIMER0_VALUE

#endif  /* #ifndef __AW_CLOCKSRC_H__ */

