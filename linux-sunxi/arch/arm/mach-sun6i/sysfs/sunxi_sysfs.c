/*
 * arch/arm/mach-sun6i/pm/scene.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@Reuuimllatech.com>
 *
 * Scene Sysfs Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>

struct class *sunxi_class;

static int __init sunxi_class_init(void)
{
    sunxi_class = class_create(THIS_MODULE, "sunxi");
    if (IS_ERR(sunxi_class)) {
        printk(KERN_ERR "%s: couldn't create sunxi class\n", __FILE__);
        return PTR_ERR(sunxi_class);
    }

    return 0;
}
subsys_initcall(sunxi_class_init);

static void __exit sunxi_class_exit(void)
{
    class_destroy(sunxi_class);
}
module_exit(sunxi_class_exit);


char power_dev_name[] = "power";
struct platform_device sunxi_power_device = {
    .name   = "sunxi_power",
    .dev    = {
        .platform_data = power_dev_name,
    },
};

char dram_dev_name[] = "dram";
struct platform_device sunxi_dram_device = {
    .name   = "sunxi_dram",
    .dev    = {
        .platform_data = dram_dev_name,
    },
};

static struct platform_device *sunxi_sysfs_pdevs[] __initdata = {
    &sunxi_power_device,
    &sunxi_dram_device,
};

static int __init sunxi_sys_dev_init(void)
{
    return platform_add_devices(sunxi_sysfs_pdevs, ARRAY_SIZE(sunxi_sysfs_pdevs));
}
module_init(sunxi_sys_dev_init);


