/*
 *  linux/arch/arm/mach-sun6i/platsmp.c
 *
 *  Copyright (C) 2012-2016 Allwinner Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>

#include <mach/hardware.h>
#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>

#include <mach/platform.h>

#include "core.h"

extern void sun6i_secondary_startup(void);

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen"
 */
volatile int pen_release = -1;


static DEFINE_SPINLOCK(boot_lock);

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 *
 * Note: for arch/arm/kernel/setup.csetup_arch(..)
 */
static void __iomem *scu_base_addr(void)
{
    pr_debug("[%s] enter\n", __FUNCTION__);
    return __io_address(AW_SCU_BASE);
}


void enable_aw_cpu(int cpu)
{
    long paddr;
    u32 pwr_reg;

    spin_lock(&boot_lock);
    paddr = virt_to_phys(sun6i_secondary_startup);
    writel(paddr, IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUCFG_P_REG0);

    /* step1: Assert nCOREPORESET LOW and hold L1RSTDISABLE LOW.
              Ensure DBGPWRDUP is held LOW to prevent any external
              debug access to the processor.
    */
    /* assert cpu core reset */
    writel(0, IO_ADDRESS(AW_R_CPUCFG_BASE) + CPUX_RESET_CTL(cpu));
    /* L1RSTDISABLE hold low */
    pwr_reg = readl(IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUCFG_GENCTL);
    pwr_reg &= ~(1<<cpu);
    writel(pwr_reg, IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUCFG_GENCTL);
    /* step2: release power clamp */
    //write bit3, bit4 to 0
    writel(0xe7, IO_ADDRESS(AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(cpu));
    while((0xe7) != readl(IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUX_PWR_CLAMP_STATUS(cpu)))
	    ;
    //write 012567 bit to 0
    writel(0x00, IO_ADDRESS(AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(cpu));
    while((0x00) != readl(IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUX_PWR_CLAMP_STATUS(cpu)))
	    ;
    spin_unlock(&boot_lock);

    usleep_range(2000, 2000);

    spin_lock(&boot_lock);
    /* step3: clear power-off gating */
    pwr_reg = readl(IO_ADDRESS(AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG);
    pwr_reg &= ~(0x00000001<<cpu);
    writel(pwr_reg, IO_ADDRESS(AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG);
    spin_unlock(&boot_lock);

    usleep_range(1000, 1000);

    /* step4: de-assert core reset */
    writel(3, IO_ADDRESS(AW_R_CPUCFG_BASE) + CPUX_RESET_CTL(cpu));
}

void __init smp_init_cpus(void)
{
    unsigned int i, ncores;

    ncores =  scu_get_core_count(NULL);
    pr_debug("[%s] ncores=%d\n", __FUNCTION__, ncores);

    for (i = 0; i < ncores; i++) {
        set_cpu_possible(i, true);
    }

    set_smp_cross_call(gic_raise_softirq);
}

/*
 * for arch/arm/kernel/smp.c:smp_prepare_cpus(unsigned int max_cpus)
 */
void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
    void __iomem *scu_base;

    pr_debug("[%s] enter\n", __FUNCTION__);
    scu_base = scu_base_addr();
    scu_enable(scu_base);
}

/*
 * for linux/arch/arm/kernel/smp.c:secondary_start_kernel(void)
 */
extern int arch_timer_common_register(void);
void __cpuinit platform_secondary_init(unsigned int cpu)
{
    pr_debug("[%s] enter, cpu:%d\n", __FUNCTION__, cpu);
    gic_secondary_init(0);
}

/*
 * for linux/arch/arm/kernel/smp.c:__cpu_up(..)
 */
int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
    pr_debug("[%s] enter\n", __FUNCTION__);
    enable_aw_cpu(cpu);
    return 0;
}


