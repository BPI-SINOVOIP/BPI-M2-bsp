/* Vibrator driver for sun6i platform 
 * ported from msm pmic vibrator driver
 *  by tom cubie <tangliang@reuuimllatech.com>
 *
 * Copyright (C) 2011 ReuuiMlla Technology.
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>
#include "../drivers/staging/android/timed_output.h"

static struct work_struct vibrator_work;
static struct hrtimer vibe_timer;
static spinlock_t vibe_lock;
static int vibe_state;
static struct regulator *ldo = NULL;

static struct motor_config_info{
	int motor_used;
	int vibe_off;
	u32 ldo_voltage;
	char* ldo;
	struct gpio_config motor_gpio;
}motor_config;

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_DATA_INFO = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static void set_sun6i_vibrator(int on)
{
	dprintk(DEBUG_DATA_INFO, "sun6i_vibrator on %d\n", on);

	if (0 != motor_config.motor_gpio.gpio) {
		if(on) {
			__gpio_set_value(motor_config.motor_gpio.gpio, !motor_config.vibe_off);
		} else {
			__gpio_set_value(motor_config.motor_gpio.gpio, motor_config.vibe_off);
		}
	} else {
		if (motor_config.ldo) {
			if(on) {
				regulator_enable(ldo);
			} else {
				if (regulator_is_enabled(ldo))
					regulator_disable(ldo);
			}
		}
	}
}

static void update_vibrator(struct work_struct *work)
{
	set_sun6i_vibrator(vibe_state);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	unsigned long	flags;

	spin_lock_irqsave(&vibe_lock, flags);
	hrtimer_cancel(&vibe_timer);

	dprintk(DEBUG_DATA_INFO, "sun6i_vibrator enable %d\n", value);

	if (value <= 0)
		vibe_state = 0;
	else {
		value = (value > 15000 ? 15000 : value);
		vibe_state = 1;
		hrtimer_start(&vibe_timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&vibe_lock, flags);

	schedule_work(&vibrator_work);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct timespec time_tmp;
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		time_tmp = ktime_to_timespec(r);
		//return r.tv.sec * 1000 + r.tv.nsec/1000000;
		return time_tmp.tv_sec* 1000 + time_tmp.tv_nsec/1000000;
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	vibe_state = 0;
	schedule_work(&vibrator_work);
	dprintk(DEBUG_DATA_INFO, "sun6i_vibrator timer expired\n");
	return HRTIMER_NORESTART;
}

static struct timed_output_dev sun6i_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

/**
 * motor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int sun6i_vibrator_fetch_sysconfig_para(void)
{
	script_item_u	val;
	script_item_value_type_e  type;

	type = script_get_item("motor_para", "motor_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_ERR "%s script_parser_fetch \"motor_para\" motor_used = %d\n",
				__FUNCTION__, val.val);
		goto script_get_err;
	}
	motor_config.motor_used = val.val;

	if(!motor_config.motor_used) {
		printk(KERN_ERR"%s motor is not used in config\n", __FUNCTION__);
		goto script_get_err;
	}

	type = script_get_item("motor_para", "motor_shake", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk(KERN_ERR "no motor_shake, ignore it!");
	} else {
		motor_config.motor_gpio = val.gpio;
		motor_config.vibe_off = val.gpio.data;
	}

	type = script_get_item("motor_para", "motor_ldo", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type)
		printk(KERN_ERR "no ldo for moto, ignore it\n");
	else
		motor_config.ldo = val.str;

	type = script_get_item("motor_para", "motor_ldo_voltage", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_ERR "no ldo moto voltage config, ignore it\n");
	} else
		motor_config.ldo_voltage = val.val;

	return 0;
script_get_err:
	printk("=========script_get_err============\n");
	return -1;
}


static int __init sun6i_vibrator_init(void)
{
	dprintk(DEBUG_INIT, "hello, sun6i_vibrator init\n");

	if (sun6i_vibrator_fetch_sysconfig_para()) {
		printk("%s: motor_fetch_sysconfig_para err.\n", __func__);
		goto exit1;
	}

	if (0 != motor_config.motor_gpio.gpio) {
		if(0 != gpio_request(motor_config.motor_gpio.gpio, NULL)) {
			printk(KERN_ERR "ERROR: vibe Gpio_request is failed\n");
		}

		if (0 != sw_gpio_setall_range(&motor_config.motor_gpio, 1)) {
			printk(KERN_ERR "vibe gpio set err!");
			goto exit;
		}

		__gpio_set_value(motor_config.motor_gpio.gpio, motor_config.vibe_off);
	}

	if (motor_config.ldo) {
		ldo = regulator_get(NULL, motor_config.ldo);
		if (!ldo) {
			pr_err("%s: could not get moto ldo '%s' in probe, maybe config error,"
			"ignore firstly !!!!!!!\n", __func__, motor_config.ldo);
			goto exit;
		}
		regulator_set_voltage(ldo, (int)(motor_config.ldo_voltage)*1000,
                                        (int)(motor_config.ldo_voltage)*1000);
	}

	INIT_WORK(&vibrator_work, update_vibrator);

	spin_lock_init(&vibe_lock);
	vibe_state = 0;
	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&sun6i_vibrator);

	dprintk(DEBUG_INIT, "sun6i_vibrator init end\n");

	return 0;
exit:
	if (0 != motor_config.motor_gpio.gpio)
		gpio_free(motor_config.motor_gpio.gpio);
exit1:	
	return -1;
}

static void __exit sun6i_vibrator_exit(void)
{
	dprintk(DEBUG_INIT, "bye, sun6i_vibrator_exit\n");
	timed_output_dev_unregister(&sun6i_vibrator);
	if (0 != motor_config.motor_gpio.gpio)
		gpio_free(motor_config.motor_gpio.gpio);
}
module_init(sun6i_vibrator_init);
module_exit(sun6i_vibrator_exit);

/* Module information */
MODULE_DESCRIPTION("timed output vibrator device for sun6i");
MODULE_LICENSE("GPL");

