/*
 *  arch/arm/mach-sun6i/include/mach/system.h
 *
 *  Copyright (C) 2012-2016 Allwinner Limited
 *  Benn Huang (benn@allwinnertech.com)
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
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H
#include <linux/io.h>
#include <asm/proc-fns.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <linux/kernel.h>
static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	printk("[%s] enter\n", __func__);
	writel(0, (AW_VIR_R_WDOG_BASE + AW_WDOG0_IRQ_EN_REG));
	writel(1, (AW_VIR_R_WDOG_BASE + AW_WDOG0_CFG_REG));
	writel(1, (AW_VIR_R_WDOG_BASE + AW_WDOG0_MODE_REG)); /* interval is 0.5 sec */
	while(1); /* never return */
}

/*
 * define chip version
 */
enum sw_ic_ver {
	MAGIC_VER_NULL      = 0,        /* invalid value    */
	MAGIC_VER_A         = 0xA,      /* chip version A   */
	MAGIC_VER_B         = 0xB,      /* chip version B   */
	MAGIC_VER_C         = 0xC,      /* chip version C   */
	MAGIC_VER_D         = 0xD,      /* chip version D   */
};
enum sw_ic_ver sw_get_ic_ver(void);


/*
 * define package ID
 */
enum sw_pac_id {
	PAC_ID_NULL     = 0,        /* invalid value    */
	PAC_ID_A31      = 0xA31,    /* package id a31   */
	PAC_ID_A31S     = 0xA32,    /* package id a31s  */
	PAC_ID_A36      = 0xA36,    /* package id a36   */
};
enum sw_pac_id sw_get_pac_id(void);

#endif
