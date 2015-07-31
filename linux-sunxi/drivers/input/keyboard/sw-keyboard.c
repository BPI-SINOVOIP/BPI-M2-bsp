/*
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
*
* Copyright (c) 2011
*
* ChangeLog
*
*
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h> 
#include <linux/wakelock.h> 
#include <linux/scenelock.h> 
#include <linux/extended_standby.h> 

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif

/* just for test */
#ifdef CONFIG_AW_FPGA_V4_PLATFORM
#define SW_INT_IRQNO_LRADC      (11 + 32)
#else
#define SW_INT_IRQNO_LRADC      (62)
#endif

#define INPUT_DEV_NAME          ("sw-keyboard")

#define KEY_MAX_CNT             (13)
 
#define KEY_BASSADDRESS         (0xf1c22800)
#define LRADC_CTRL              (0x00)
#define LRADC_INTC              (0x04)
#define LRADC_INT_STA           (0x08)
#define LRADC_DATA0             (0x0c)
#define LRADC_DATA1             (0x10)

#define FIRST_CONCERT_DLY       (0<<24)
#define CHAN                    (0x3)
#define ADC_CHAN_SELECT         (CHAN<<22)
#define LRADC_KEY_MODE          (0)
#define KEY_MODE_SELECT         (LRADC_KEY_MODE<<12)
#define LEVELB_VOL              (0<<4)

#define LRADC_HOLD_EN           (1<<6)

#define LRADC_SAMPLE_32HZ       (3<<2)
#define LRADC_SAMPLE_62HZ       (2<<2)
#define LRADC_SAMPLE_125HZ      (1<<2)
#define LRADC_SAMPLE_250HZ      (0<<2)


#define LRADC_EN                (1<<0)

#define LRADC_ADC1_UP_EN        (1<<12)
#define LRADC_ADC1_DOWN_EN      (1<<9)
#define LRADC_ADC1_DATA_EN      (1<<8)

#define LRADC_ADC0_UP_EN        (1<<4)
#define LRADC_ADC0_DOWN_EN      (1<<1)
#define LRADC_ADC0_DATA_EN      (1<<0)

#define LRADC_ADC1_UPPEND       (1<<12)
#define LRADC_ADC1_DOWNPEND     (1<<9)
#define LRADC_ADC1_DATAPEND     (1<<8)


#define LRADC_ADC0_UPPEND       (1<<4)
#define LRADC_ADC0_DOWNPEND     (1<<1)
#define LRADC_ADC0_DATAPEND     (1<<0)

#define EVB
//#define CUSTUM
#define ONE_CHANNEL
#define MODE_0V2
//#define MODE_0V15
//#define TWO_CHANNEL
#ifdef MODE_0V2
/* standard of key maping 
 * 0.2V mode
 */					
#define REPORT_START_NUM            (2)
#define REPORT_KEY_LOW_LIMIT_COUNT  (1)
#define MAX_CYCLE_COUNTER           (100)
//#define REPORT_REPEAT_KEY_BY_INPUT_CORE
//#define REPORT_REPEAT_KEY_FROM_HW
#define INITIAL_VALUE               (0Xff)

static unsigned char keypad_mapindex[64] = {
	0,0,0,0,0,0,0,0,               	/* key 1, 8个， 0-7 */
	1,1,1,1,1,1,1,                 	/* key 2, 7个， 8-14 */
	2,2,2,2,2,2,2,                 	/* key 3, 7个， 15-21 */
	3,3,3,3,3,3,                   	/* key 4, 6个， 22-27 */
	4,4,4,4,4,4,                   	/* key 5, 6个， 28-33 */
	5,5,5,5,5,5,                   	/* key 6, 6个， 34-39 */
	6,6,6,6,6,6,6,6,6,6,           	/* key 7, 10个，40-49 */
	7,7,7,7,7,7,7,7,7,7,7,7,7,7    	/* key 8, 17个，50-63 */
};
#endif
                        
#ifdef MODE_0V15
/* 0.15V mode */
static unsigned char keypad_mapindex[64] = {
	0,0,0,    			/* key1 */
	1,1,1,1,1,                     	/* key2 */
	2,2,2,2,2,
	3,3,3,3,
	4,4,4,4,4,
	5,5,5,5,5,
	6,6,6,6,6,
	7,7,7,7,
	8,8,8,8,8,
	9,9,9,9,9,
	10,10,10,10,
	11,11,11,11,
	12,12,12,12,12,12,12,12,12,12  	/*key13 */
};
#endif

#ifdef EVB
static unsigned int sun6i_scankeycodes[KEY_MAX_CNT] = {
	[0 ] = KEY_VOLUMEUP,       
	[1 ] = KEY_VOLUMEDOWN,      
	[2 ] = KEY_MENU,         
	[3 ] = KEY_ENTER,       
	[4 ] = KEY_HOME,   
	[5 ] = KEY_RESERVED, 
	[6 ] = KEY_RESERVED,        
	[7 ] = KEY_RESERVED,
	[8 ] = KEY_RESERVED,
	[9 ] = KEY_RESERVED,
	[10] = KEY_RESERVED,
	[11] = KEY_RESERVED,
	[12] = KEY_RESERVED,
};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND	
struct sun6i_keyboard_data {
	struct early_suspend early_suspend;
};
#else
#ifdef CONFIG_PM
struct dev_pm_domain keyboard_pm_domain;
#endif
#endif

static volatile unsigned int key_val;
static struct input_dev *sun6ikbd_dev;
static unsigned char scancode;
static struct wake_lock lradc_lock;

static unsigned char key_cnt = 0;
static unsigned char compare_buffer[REPORT_START_NUM] = {0};
static unsigned char transfer_code = INITIAL_VALUE;

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);


#ifdef CONFIG_HAS_EARLYSUSPEND
static struct sun6i_keyboard_data *keyboard_data;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
/* 停用设备 */
static void sun6i_keyboard_early_suspend(struct early_suspend *h)
{
	//int ret;
	//struct sun6i_keyboard_data *ts = container_of(h, struct sun6i_keyboard_data, early_suspend);

	dprintk(DEBUG_SUSPEND, "[%s] enter standby state: %d. \n", __FUNCTION__, (int)standby_type);
    
	if (NORMAL_STANDBY == standby_type) {
		writel(0,KEY_BASSADDRESS + LRADC_CTRL);
	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type) {
		if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
			printk("lradc-key: talking standby, enable wakeup source lradc!!\n");
			enable_wakeup_src(CPUS_LRADC_SRC, 0);
		}
	}
	
	return ;
}

/* 重新唤醒 */
static void sun6i_keyboard_late_resume(struct early_suspend *h)
{
	//int ret;
	//struct sun6i_keyboard_data *ts = container_of(h, struct sun6i_keyboard_data, early_suspend);
	
	dprintk(DEBUG_SUSPEND, "[%s] return from standby state: %d. \n", __FUNCTION__, (int)standby_type); 

	/* process for normal standby */
	if (NORMAL_STANDBY == standby_type) {
		writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
			|LRADC_SAMPLE_250HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type) {
		if (check_scene_locked(SCENE_TALKING_STANDBY) != 0) {
#ifdef ONE_CHANNEL
		writel(LRADC_ADC0_DOWN_EN|LRADC_ADC0_UP_EN|LRADC_ADC0_DATA_EN,KEY_BASSADDRESS + LRADC_INTC);	
		writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
			|LRADC_SAMPLE_250HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
#else
#endif
		} else {
			disable_wakeup_src(CPUS_LRADC_SRC, 0);
			printk("lradc-key: resume from talking standby!!\n");
		}
	}
	
	return ; 
}
#else
#ifdef CONFIG_PM
static int sun6i_keyboard_suspend(struct device *dev)
{

	//int ret;
	//struct sun6i_keyboard_data *ts = container_of(h, struct sun6i_keyboard_data, early_suspend);

	dprintk(DEBUG_SUSPEND, "[%s] enter standby state: %d. \n", __FUNCTION__, (int)standby_type);
    
	if (NORMAL_STANDBY == standby_type) {
		writel(0,KEY_BASSADDRESS + LRADC_CTRL);
	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type)
		;
	
	return 0;
}

/* 重新唤醒 */
static int sun6i_keyboard_resume(struct device *dev)
{

	//int ret;
	//struct sun6i_keyboard_data *ts = container_of(h, struct sun6i_keyboard_data, early_suspend);
	
	dprintk(DEBUG_SUSPEND, "[%s] return from standby state: %d. \n", __FUNCTION__, (int)standby_type); 

	/* process for normal standby */
	if (NORMAL_STANDBY == standby_type) {
		writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
			|LRADC_SAMPLE_250HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type) {
#ifdef ONE_CHANNEL
		writel(LRADC_ADC0_DOWN_EN|LRADC_ADC0_UP_EN|LRADC_ADC0_DATA_EN,KEY_BASSADDRESS + LRADC_INTC);	
		writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
			|LRADC_SAMPLE_250HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
#else
#endif
	}
	
	return 0; 
}
#endif
#endif


static irqreturn_t sun6i_isr_key(int irq, void *dummy)
{
	unsigned int  reg_val;
	int judge_flag = 0;
	 
	dprintk(DEBUG_INT, "Key Interrupt\n");
	
	reg_val  = readl(KEY_BASSADDRESS + LRADC_INT_STA);
	//writel(reg_val,KEY_BASSADDRESS + LRADC_INT_STA);

	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		wake_lock_timeout(&lradc_lock, msecs_to_jiffies(1000));
	}

	if (reg_val & LRADC_ADC0_DOWNPEND) {	
		dprintk(DEBUG_INT, "key down\n");
	}
	
	if (reg_val & LRADC_ADC0_DATAPEND) {
		key_val = readl(KEY_BASSADDRESS+LRADC_DATA0);

		
		if (key_val < 0x3f) {

			compare_buffer[key_cnt] = key_val&0x3f;
		}

		if ((key_cnt + 1) < REPORT_START_NUM) {
			key_cnt++;
			/* do not report key message */

		} else {
			if(compare_buffer[0] == compare_buffer[1])
			{
			key_val = compare_buffer[1];
			scancode = keypad_mapindex[key_val&0x3f];
			judge_flag = 1;
			key_cnt = 0;
			} else {
				key_cnt = 0;
				judge_flag = 0;
			}

			if (1 == judge_flag) {
				dprintk(DEBUG_INT, "report data: key_val :%8d transfer_code: %8d , scancode: %8d\n", \
					key_val, transfer_code, scancode);

				if (transfer_code == scancode) {
					/* report repeat key value */
#ifdef REPORT_REPEAT_KEY_FROM_HW
					input_report_key(sun6ikbd_dev, sun6i_scankeycodes[scancode], 0);
					input_sync(sun6ikbd_dev);
					input_report_key(sun6ikbd_dev, sun6i_scankeycodes[scancode], 1);
					input_sync(sun6ikbd_dev);
#else
					/* do not report key value */
#endif
				} else if (INITIAL_VALUE != transfer_code) {                               
					/* report previous key value up signal + report current key value down */
					input_report_key(sun6ikbd_dev, sun6i_scankeycodes[transfer_code], 0);
					input_sync(sun6ikbd_dev);
					input_report_key(sun6ikbd_dev, sun6i_scankeycodes[scancode], 1);
					input_sync(sun6ikbd_dev);
					transfer_code = scancode;

				} else {
					/* INITIAL_VALUE == transfer_code, first time to report key event */
					input_report_key(sun6ikbd_dev, sun6i_scankeycodes[scancode], 1);
					input_sync(sun6ikbd_dev);
					transfer_code = scancode;
				}
			}
		}
	}
       
	if (reg_val & LRADC_ADC0_UPPEND) {
		
		if(INITIAL_VALUE != transfer_code) {
				
			dprintk(DEBUG_INT, "report data: key_val :%8d transfer_code: %8d \n",key_val, transfer_code);
					
			input_report_key(sun6ikbd_dev, sun6i_scankeycodes[transfer_code], 0);
			input_sync(sun6ikbd_dev);
		}

		dprintk(DEBUG_INT, "key up \n");

		key_cnt = 0;
		judge_flag = 0;
		transfer_code = INITIAL_VALUE;
	}
	
	writel(reg_val,KEY_BASSADDRESS + LRADC_INT_STA);
	return IRQ_HANDLED;
}

static int __init sun6ikbd_init(void)
{
	int i;
	int err =0;

 	dprintk(DEBUG_INIT, "sun6ikbd_init \n");
	
	sun6ikbd_dev = input_allocate_device();
	if (!sun6ikbd_dev) {
		printk(KERN_DEBUG "sun6ikbd: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sun6ikbd_dev->name = INPUT_DEV_NAME;  
	sun6ikbd_dev->phys = "sun6ikbd/input0"; 
	sun6ikbd_dev->id.bustype = BUS_HOST;      
	sun6ikbd_dev->id.vendor = 0x0001;
	sun6ikbd_dev->id.product = 0x0001;
	sun6ikbd_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	sun6ikbd_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	printk(KERN_DEBUG "REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	sun6ikbd_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif

	for (i = 0; i < KEY_MAX_CNT; i++)
		set_bit(sun6i_scankeycodes[i], sun6ikbd_dev->keybit);
	
#ifdef ONE_CHANNEL
	writel(LRADC_ADC0_DOWN_EN|LRADC_ADC0_UP_EN|LRADC_ADC0_DATA_EN,KEY_BASSADDRESS + LRADC_INTC);	
	writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
		|LRADC_SAMPLE_250HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
#else
#endif


	if (request_irq(SW_INT_IRQNO_LRADC, sun6i_isr_key, 0, "sun6ikbd", NULL)) {
		err = -EBUSY;
		printk(KERN_DEBUG "request irq failure. \n");
		goto fail2;
	}

	
#ifdef CONFIG_HAS_EARLYSUSPEND
#else
#ifdef CONFIG_PM
	keyboard_pm_domain.ops.suspend = sun6i_keyboard_suspend;
	keyboard_pm_domain.ops.resume = sun6i_keyboard_resume;
	sun6ikbd_dev->dev.pm_domain = &keyboard_pm_domain;	
#endif
#endif

	err = input_register_device(sun6ikbd_dev);
	if (err)
		goto fail3;

#ifdef CONFIG_HAS_EARLYSUSPEND 
	dprintk(DEBUG_INIT, "==register_early_suspend =\n");
	keyboard_data = kzalloc(sizeof(*keyboard_data), GFP_KERNEL);
	if (keyboard_data == NULL) {
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	keyboard_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 3;	
	keyboard_data->early_suspend.suspend = sun6i_keyboard_early_suspend;
	keyboard_data->early_suspend.resume	= sun6i_keyboard_late_resume;
	register_early_suspend(&keyboard_data->early_suspend);
#endif
	wake_lock_init(&lradc_lock, WAKE_LOCK_SUSPEND, "lradc_wakelock");

	dprintk(DEBUG_INIT, "sun6ikbd_init end\n");

	return 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
err_alloc_data_failed:
#endif
fail3:	
	free_irq(SW_INT_IRQNO_LRADC, NULL);
fail2:	
	input_free_device(sun6ikbd_dev);
fail1:
	printk(KERN_DEBUG "sun6ikbd_init failed. \n");

	return err;
}

static void __exit sun6ikbd_exit(void)
{	
	wake_lock_destroy(&lradc_lock);
#ifdef CONFIG_HAS_EARLYSUSPEND	
	unregister_early_suspend(&keyboard_data->early_suspend);	
#endif
	free_irq(SW_INT_IRQNO_LRADC, NULL);
	input_unregister_device(sun6ikbd_dev);
}

module_init(sun6ikbd_init);
module_exit(sun6ikbd_exit);


MODULE_AUTHOR(" <@>");
MODULE_DESCRIPTION("sun6i-keyboard driver");
MODULE_LICENSE("GPL");



