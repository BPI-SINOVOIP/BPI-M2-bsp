/*
 *  arch/arm/mach-sun6i/core.c
 *
 *  Copyright (C) 2012 - 2016 Allwinner Limited
 *  Benn Huang (benn@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl061.h>
#include <linux/amba/mmci.h>
#include <linux/memblock.h>
#include <linux/amba/pl022.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/mach-types.h>
#include <asm/pmu.h>
#include <asm/smp_twd.h>
#include <asm/pgtable.h>
#include <asm/hardware/gic.h>
#include <linux/clockchips.h>
#include <asm/hardware/cache-l2x0.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/arch_timer.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <mach/io.h>
#include <mach/timer.h>
#include <mach/ar100.h>

#include "core.h"

static struct map_desc sun6i_io_desc[] __initdata = {
	{IO_ADDRESS(AW_SRAM_A1_BASE), __phys_to_pfn(AW_SRAM_A1_BASE),  AW_SRAM_A1_SIZE, MT_MEMORY_ITCM},
	{IO_ADDRESS(AW_SRAM_A2_BASE), __phys_to_pfn(AW_SRAM_A2_BASE),  AW_SRAM_A2_SIZE, MT_DEVICE_NONSHARED},
	{IO_ADDRESS(AW_IO_PHYS_BASE), __phys_to_pfn(AW_IO_PHYS_BASE),  AW_IO_SIZE, MT_DEVICE_NONSHARED},
	{IO_ADDRESS(AW_BROM_BASE),    __phys_to_pfn(AW_BROM_BASE),     AW_BROM_SIZE, MT_DEVICE_NONSHARED},
};

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
struct tag_mem32 ion_mem = {
	.start	= ION_CARVEOUT_MEM_BASE,
	.size	= ION_CARVEOUT_MEM_SIZE,
};
EXPORT_SYMBOL(ion_mem);
#endif

struct tag_mem32 hwres_mem = {
	.start = HW_RESERVED_MEM_BASE,
	.size  = HW_RESERVED_MEM_SIZE,
};
EXPORT_SYMBOL(hwres_mem);

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
struct tag_mem32 ram_console_mem = {
	.start = SW_RAM_CONSOLE_BASE,
	.size  = SW_RAM_CONSOLE_SIZE,
};
#endif

static void __init sun6i_map_io(void)
{
	iotable_init(sun6i_io_desc, ARRAY_SIZE(sun6i_io_desc));
}


static void __init gic_init_irq(void)
{
	gic_init(0, 29, (void *)IO_ADDRESS(AW_GIC_DIST_BASE), (void *)IO_ADDRESS(AW_GIC_CPU_BASE));
}


void __init sun6i_clkevt_init(void);
int __init sun6i_clksrc_init(void);

#ifdef CONFIG_ARM_ARCH_TIMER
struct arch_timer sun6i_arch_timer __initdata = {
	.res[0] = {
		.start = 29,
		.end = 29,
		.flags = IORESOURCE_IRQ,
	},
	.res[1] = {
		.start = 30,
		.end = 30,
		.flags = IORESOURCE_IRQ,
	},
};
#endif

static void __init sun6i_timer_init(void)
{
#ifndef CONFIG_ARM_ARCH_TIMER
#error "please select CONFIG_ARM_ARCH_TIMER"
#else
	arch_timer_register(&sun6i_arch_timer);
	arch_timer_sched_clock_init();
#endif
}


static struct sys_timer sun6i_timer __initdata = {
	.init	= sun6i_timer_init,
};

static void __init sun6i_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
	struct tag *t;

	for (t = tags; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_MEM && t->u.mem.size) {
			pr_debug("[%s]: From boot, get meminfo:\n"
					"\tStart:\t0x%08x\n"
					"\tSize:\t%dMB\n",
					__func__,
					t->u.mem.start,
					t->u.mem.size >> 20);
			#ifdef CONFIG_ANDROID_PERSISTENT_RAM
			ram_console_mem.start = t->u.mem.size
					+ PLAT_PHYS_OFFSET - SW_RAM_CONSOLE_SIZE;
			#endif
			if ( (t->u.mem.size >> 20) <= 512 ) {
				ion_mem.size   = CONFIG_ION_SUNXI_CARVEOUT_SIZE_512M * SZ_1M;
				hwres_mem.size = CONFIG_SUNXI_MEMORY_RESERVED_SIZE_512M * SZ_1M;
			} else {
				ion_mem.size   = CONFIG_ION_SUNXI_CARVEOUT_SIZE_1G * SZ_1M;
				hwres_mem.size = CONFIG_SUNXI_MEMORY_RESERVED_SIZE_1G * SZ_1M;
			}
			ion_mem.start = HW_RESERVED_MEM_BASE + hwres_mem.size;
			return;
		}
	}
	pr_debug("[%s] enter\n", __func__);

	meminfo->bank[0].start = PLAT_PHYS_OFFSET;
	meminfo->bank[0].size = PLAT_MEM_SIZE;
	meminfo->nr_banks = 1;

	pr_debug("nr_banks: %d, bank.start: 0x%08x, bank.size: 0x%08lx\n",
			meminfo->nr_banks, meminfo->bank[0].start,
			meminfo->bank[0].size);
}

void __init sun6i_reserve(void)
{
	/* reserve for sys_config */
	memblock_reserve(SYS_CONFIG_MEMBASE, SYS_CONFIG_MEMSIZE);

	/*
	 * reserve for DE and VE
	 * Here, we must use memblock_remove, because it be allocated using genalloc.
	 */
	memblock_remove(hwres_mem.start, hwres_mem.size);

	/* reserve for standby */
	memblock_reserve(SUPER_STANDBY_MEM_BASE, SUPER_STANDBY_MEM_SIZE);

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
	sunxi_add_persist_ram_devices();
#endif

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
	/* reserve for ION */
	memblock_remove(ion_mem.start, ion_mem.size);
#endif
}

#if 0

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
/*
 * Pick out the ion memory size.  We look for ion_reserve=size,
 * where start and size are "size[KkMm]"
 */
static int __init early_ion_reserve(char *p)
{
	char *endp;

	ion_mem.start = ION_CARVEOUT_MEM_BASE;
	ion_mem.size  = memparse(p, &endp);

	if(ion_mem.size != ION_CARVEOUT_MEM_SIZE) {
		printk("ion_reserve(%u) != ION_CARVEOUT_MEM_SIZE(%u)!\n",
							ion_mem.size, ION_CARVEOUT_MEM_SIZE);
	}

	pr_debug("[%s]: ION memory reserve: [start: 0x%08x, size: %u Mbytes]\n",
			__func__, ion_mem.start, ion_mem.size >> 20);

	return 0;
}
early_param("ion_reserve", early_ion_reserve);
#endif

#else

static int __init early_hw_reserve(char *p)
{
	char *endp = NULL;
	u32 hw_res_size = memparse(p, &endp);

	if(hw_res_size != hwres_mem.size) {
		printk("hwres_mem.size(%u) != hw_res_size(%u)!\n",
			hwres_mem.size, hw_res_size);
	}
	hwres_mem.size = hw_res_size;
	ion_mem.start = HW_RESERVED_MEM_BASE + hwres_mem.size;

	pr_debug("[%s]: de/ve memory reserve: [start: 0x%08x, size: %u Mbytes]\n",
			__func__, hwres_mem.start, hwres_mem.size >> 20);
	pr_debug("[%s]:  ION  memory reserve: [start: 0x%08x, size: %u Mbytes]\n",
			__func__, ion_mem.start, ion_mem.size >> 20);

	return 0;
}
early_param("hw_reserve", early_hw_reserve);
#endif

static void sun6i_restart(char mode, const char *cmd)
{
	pr_debug("[%s] enter\n", __func__);
	writel(0, (AW_VIR_R_WDOG_BASE + AW_WDOG0_IRQ_EN_REG));
	writel(1, (AW_VIR_R_WDOG_BASE + AW_WDOG0_CFG_REG));
	writel(1, (AW_VIR_R_WDOG_BASE + AW_WDOG0_MODE_REG)); /* interval is 0.5 sec */
	while(1); /* never return */
}

extern void sw_pdev_init(void);
static void __init sun6i_init(void)
{
	pr_debug("[%s] enter\n", __func__);
	sw_pdev_init();
	/* Register platform devices here!! */
}

void __init sun6i_init_early(void)
{
	pr_debug("[%s] enter\n", __func__);
}

static uint8_t sun6i_chip_id[16];
int get_chip_id(uint8_t *chip_id)
{
	static int read_flag = 0;

	if(!read_flag) {
		ar100_axp_get_chip_id(sun6i_chip_id);
		read_flag = 1;
	}

	memcpy(chip_id, sun6i_chip_id, 16);

	return 0;
}

MACHINE_START(SUN6I, "sun6i")
	.atag_offset	= 0x100,
	.reserve	= sun6i_reserve,
	.fixup		= sun6i_fixup,
	.map_io		= sun6i_map_io,
	.init_early	= sun6i_init_early,
	.init_irq	= gic_init_irq,
	.timer		= &sun6i_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= sun6i_init,
#ifdef CONFIG_ZONE_DMA
	.dma_zone_size	= SZ_64M,
#endif
	.restart	= sun6i_restart,
MACHINE_END
