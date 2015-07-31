/*
 *  arch/arm/mach-$chip/devices.c
 *
 *  Copyright (C) 2012 AllWinner Limited
 *  Benn Huang <benn@allwinnertech.com>
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


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/pda_power.h>
#include <linux/io.h>
#include <linux/i2c.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/setup.h>
#include <mach/hardware.h>
#include <mach/system.h>
#include <mach/platform.h>
#include <mach/irqs.h>
#if(CONFIG_CPU_HAS_PMU)
#include <asm/pmu.h>
#endif

#include <linux/persistent_ram.h>
#include <linux/ram_console.h>


/* uart */
static struct plat_serial8250_port debug_uart_platform_data[] = {
	{
		.membase        = (void __iomem *)(IO_ADDRESS(AW_UART0_BASE)),
		.mapbase        = (resource_size_t)AW_UART0_BASE,
		.irq            = AW_IRQ_UART0,
		.flags          = UPF_BOOT_AUTOCONF|UPF_IOREMAP,
		.iotype         = UPIO_MEM32,
		.regshift       = 2,
		.uartclk        = 24000000,
	}, {
		.flags          = 0,
	}
 };

static struct platform_device debug_uart = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &debug_uart_platform_data[0],
	}
};

/* dma */
static u64 sw_dmac_dmamask = DMA_BIT_MASK(32);

static struct resource sw_dmac_resources[] = {
	[0] = {
		.start 	= AW_DMA_BASE,
		.end 	= AW_DMA_BASE + 0x1000, /* lead to boot_secondary->enable_aw_cpu halt? no */
		.flags 	= IORESOURCE_MEM,
	},
	[1] = {
		.start 	= AW_IRQ_DMA,
		.end 	= AW_IRQ_DMA,
		.flags 	= IORESOURCE_IRQ,
	}
};

static struct platform_device sw_dmac_device = {
	.name 		= "sw_dmac",	/* must be same as sw_dmac_driver's name */
	.id 		= 0, 		/* there is only one device for sw_dmac dirver, so id is 0 */
	.num_resources 	= ARRAY_SIZE(sw_dmac_resources),
	.resource 	= sw_dmac_resources,
	.dev 		= {
				.dma_mask = &sw_dmac_dmamask,
				.coherent_dma_mask = DMA_BIT_MASK(32),	/* validate dma_pool_alloc */
				// .platform_data = (void *) &sw_dmac_pdata,
	}
};

struct platform_device sw_pdev_nand =
{
	.name = "sw_nand",
	.id = -1,
};

#if(CONFIG_CPU_HAS_PMU)
/* cpu performance support */
static struct resource sun6i_pmu_resource = {
    .start  = AW_IRQ_PMU0,
    .end    = AW_IRQ_PMU3,
    .flags  = IORESOURCE_IRQ,
};

static struct platform_device sun6i_pmu_device = {
    .name   = "arm-pmu",
    .id     = ARM_PMU_DEVICE_CPU,
    .num_resources = 1,
    .resource = &sun6i_pmu_resource,
};
#endif


#ifdef CONFIG_ANDROID_RAM_CONSOLE
static char bootreason[128] = {0,};
int __init sunxi_boot_reason(char *s)
{
	int n;

	if (*s == '=')
		s++;
	n = snprintf(bootreason, sizeof(bootreason),
			"Boot info:\n"
			"Last boot reason: %s\n", s);
	bootreason[n] = '\0';
	return 1;
}
__setup("bootreason", sunxi_boot_reason);

struct ram_console_platform_data ram_console_pdata = {
	.bootinfo = bootreason,
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.dev = {
		.platform_data = &ram_console_pdata,
	}
};
#endif /* CONFIG_ANDROID_RAM_CONSOLE */

#ifdef CONFIG_PERSISTENT_TRACER
static struct platform_device persistent_trace_device = {
	.name = "persistent_trace",
	.id = -1,
};
#endif

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
static struct persistent_ram_descriptor pram_descs[] = {
#if defined(CONFIG_ANDROID_RAM_CONSOLE) && defined(CONFIG_PERSISTENT_TRACER)
	{
		.name = "ram_console",
		.size = SW_RAM_CONSOLE_SIZE/2,
	},
	{
		.name = "persistent_trace",
		.size = SW_RAM_CONSOLE_SIZE/2,
	},
#elif defined(CONFIG_ANDROID_RAM_CONSOLE)
	{
		.name = "ram_console",
		.size = SW_RAM_CONSOLE_SIZE,
	},
#endif
};

static struct persistent_ram persist_ram = {
	.start = SW_RAM_CONSOLE_BASE,
	.size = SW_RAM_CONSOLE_SIZE,
	.num_descs = ARRAY_SIZE(pram_descs),
	.descs = pram_descs,
};

extern struct tag_mem32 ram_console_mem;
void __init sunxi_add_persist_ram_devices(void)
{
	int ret;

	persist_ram.start = ram_console_mem.start;
	pr_info("persist ram addr: 0x%x\n",
			persist_ram.start);

	ret = persistent_ram_early_init(&persist_ram);
	if (ret)
		pr_err("%s: failed to initialize persistent ram\n", __func__);
}
#endif /* CONFIG_ANDROID_PERSISTENT_RAM */

static struct platform_device *sw_pdevs[] __initdata = {
	&debug_uart,
	&sw_dmac_device,
	&sw_pdev_nand,
	#if(CONFIG_CPU_HAS_PMU)
	&sun6i_pmu_device,
	#endif
	#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
	#endif
	#ifdef CONFIG_PERSISTENT_TRACER
	&persistent_trace_device,
	#endif
};

void sw_pdev_init(void)
{
	platform_add_devices(sw_pdevs, ARRAY_SIZE(sw_pdevs));
}

