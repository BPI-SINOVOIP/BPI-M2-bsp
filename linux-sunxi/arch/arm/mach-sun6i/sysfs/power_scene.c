/*
 * arch/arm/mach-sun6i/scene.c
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
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/scene.h>

extern struct class *sunxi_class;
extern struct platform_device sunxi_power_device;

static struct srcu_notifier_head scene_change_notifier;
static bool init_scene_change_notifier_called;
static struct mutex scene_mutex;
struct device *scene_dev;
static enum scene_type cur_type;
static u8 scene_list[SCENE_TYPE_MAX] = {
        SCENE_TYPE_DEF, SCENE_TYPE1, SCENE_TYPE2, SCENE_TYPE3,
        SCENE_TYPE4, SCENE_TYPE5, SCENE_TYPE6, SCENE_TYPE7,
        SCENE_TYPE8, SCENE_TYPE9
};

static int __init init_scene_change_notifier(void)
{
	srcu_init_notifier_head(&scene_change_notifier);
	init_scene_change_notifier_called = true;
	return 0;
}
pure_initcall(init_scene_change_notifier);

int scene_register_notifier(struct notifier_block *nb)
{
	WARN_ON(!init_scene_change_notifier_called);
	return srcu_notifier_chain_register(&scene_change_notifier, nb);
}
EXPORT_SYMBOL_GPL(scene_register_notifier);

int scene_unregister_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_unregister(&scene_change_notifier, nb);
}
EXPORT_SYMBOL_GPL(scene_unregister_notifier);


static ssize_t available_scenes_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	ssize_t i = 0;
    int j;
    enum scene_type scene;

    for (j = 0; j < ARRAY_SIZE(scene_list); j++) {
        scene = scene_list[j];
        i += sprintf(&buf[i], "%d ", scene);
    }
	i += sprintf(&buf[i], "\n");

	return i;
}

static ssize_t set_scene_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%d\n", cur_type);
}

static ssize_t set_scene_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
    enum scene_type st;
    int value, ret;

	ret = sscanf(buf, "%d", &value);
	if (ret != 1)
		goto out;

    mutex_lock(&scene_mutex);
    st = (enum scene_type)value;
    srcu_notifier_call_chain(&scene_change_notifier, st, NULL);
    cur_type = st;
    ret = count;
    mutex_unlock(&scene_mutex);

out:
	return ret;
}

static DEVICE_ATTR(available_scenes, S_IRUGO, available_scenes_show, NULL);
static DEVICE_ATTR(set_scene, S_IRUGO | S_IWUSR, set_scene_show, set_scene_store);

static struct attribute *scene_dev_entries[] = {
	&dev_attr_available_scenes.attr,
    &dev_attr_set_scene.attr,
	NULL,
};

static struct attribute_group scene_dev_attr_group = {
	.name	= "scene",
	.attrs	= scene_dev_entries,
};

static int __init scene_init(void)
{
	int ret;

	scene_dev = device_create(sunxi_class, &sunxi_power_device.dev, 0, NULL,
            (char *)sunxi_power_device.dev.platform_data);

    ret = sysfs_create_group(&scene_dev->kobj, &scene_dev_attr_group);
    if (ret) {
        printk(KERN_ERR "Failed to add %s group attrs\n", scene_dev_attr_group.name);
        goto out;
    }

    cur_type = SCENE_TYPE_DEF;
    mutex_init(&scene_mutex);

	return 0;

out:
    return ret;
}
late_initcall(scene_init);
