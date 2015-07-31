/*
 *  TI Bluesleep driver
 *	Kernel module responsible for Wake up of Host
 *  Copyright (C) 2009-2010 Texas Instruments


 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * Copyright (C) 2006-2007 - Motorola
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 *  Date         Author           Comment
 * -----------  --------------   --------------------------------
 * 2006-Apr-28  Motorola         The kernel module for running the Bluetooth(R)
 *                               Sleep-Mode Protocol from the Host side
 * 2006-Sep-08  Motorola         Added workqueue for handling sleep work.
 * 2007-Jan-24  Motorola         Added mbm_handle_ioi() call to ISR.
 * 2009-Aug-10  Motorola         Changed "add_timer" to "mod_timer" to solve
 *                               race when flurry of queued work comes in.
*/

#include <linux/module.h>       /* kernel module definitions */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>

#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/sys_config.h>

#include <net/bluetooth/bluetooth.h>

/*
 * Defines
 */
#define VERSION	 "1.1"
#define PROC_DIR	"bluetooth/sleep"

#define POLARITY_LOW 0
#define POLARITY_HIGH 1

static void bluesleep_stop(void);
static int bluesleep_start(void);
static void enter_sleep_mode(void);

struct bluesleep_info {
	unsigned host_wake_irq;
	struct uart_port *uport;
	int irq_polarity;
	int host_wake;
	u32 irq_handle;
};


/* state variable names and bit positions */
#define FLAG_RESET      	0x00
#define BT_ACTIVE		0x02
#define BT_SUSPEND		0x04

/* variable use indicate lpm modle */
static bool has_lpm_enabled = false;

/* struct use save platform_device from uart */
static struct platform_device *bluesleep_uart_dev;

static struct bluesleep_info *bsi;

/* module usage */
static atomic_t open_count = ATOMIC_INIT(1);

struct proc_dir_entry *bluetooth_dir, *sleep_dir;

static struct uart_port *bluesleep_get_uart_port(void)
{
	struct uart_port *uport = NULL;
	BT_INFO("%s enter.", __FUNCTION__);
	if (bluesleep_uart_dev) {
		uport = platform_get_drvdata(bluesleep_uart_dev);
		BT_INFO("%s get uart_port from blusleep_uart_dev: %s, port irq: %d\n", 
		           __FUNCTION__, bluesleep_uart_dev->name, uport->irq);
	}

	return uport;
}

static int bluesleep_read_proc_lpm(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "unsupported to read\n");
}

static int bluesleep_write_proc_lpm(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	if (b == '0') {
		/* HCI_DEV_UNREG */
		bluesleep_stop();
		has_lpm_enabled = false;
		bsi->uport = NULL;
	} else {
		/* HCI_DEV_REG */
		if (!has_lpm_enabled) {
			has_lpm_enabled = true;
			if (bluesleep_uart_dev)
				bsi->uport = bluesleep_get_uart_port();

			/* if bluetooth started, start bluesleep*/
			bluesleep_start();
		}
	}

	return count;
}

/*
 * Global variables
 */
/** Global state flags */
static unsigned long flags;

/**
 * Schedules a tasklet to run when receiving an interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id)
{
	pr_debug("%s", __func__);
	
	return 0;
}

/**
 * Starts the Sleep-Mode Protocol on the Host.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int bluesleep_start(void)
{
	int retval;
	
	printk("bluesleep_acquire irq\n");
  bsi->irq_handle = sw_gpio_irq_request(bsi->host_wake, TRIG_EDGE_NEGATIVE,(peint_handle)bluesleep_hostwake_isr, NULL);
  if (bsi->irq_handle == 0) {
  	BT_ERR("Couldn't acquire bt_host_wake IRQ or enable it");
    retval = -1;	
    goto fail;
  }

  sw_gpio_eint_set_enable(bsi->host_wake, 0);
  sw_gpio_setcfg(bsi->host_wake, GPIO_CFG_INPUT);

	return 0;
fail:
	atomic_inc(&open_count);
	return retval;
}

/**
 * Stops the Sleep-Mode Protocol on the Host.
 */
static void bluesleep_stop(void)
{
	pr_debug("%s", __func__);

  sw_gpio_eint_set_enable(bsi->host_wake, 0);
  sw_gpio_irq_free(bsi->irq_handle);
}

void bluesleep_setup_uart_port(struct platform_device *uart_dev)
{
	BT_INFO("%s enter.", __FUNCTION__);
	bluesleep_uart_dev = uart_dev;
	BT_INFO("%s device name: %s.", __FUNCTION__, bluesleep_uart_dev->name);
}
EXPORT_SYMBOL(bluesleep_setup_uart_port);

static int bluesleep_probe(struct platform_device *pdev)
{
	int ret;
  script_item_u val;
	script_item_value_type_e type;

	bsi = kzalloc(sizeof(struct bluesleep_info), GFP_KERNEL);
	if (!bsi)
		return -ENOMEM;

	type = script_get_item("wifi_para", "rtl8723bs_bt_host_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) {
		BT_ERR("get rtl8723bs rtl8723bs_bt_host_wake gpio failed\n");
		goto free_bsi;
	} else {
		bsi->host_wake = val.gpio.gpio;
  }
  
	clear_bit(BT_SUSPEND, &flags);
	set_bit(BT_ACTIVE, &flags);

	return 0;

free_bsi:
	kfree(bsi);
	return ret;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	pr_debug("%s", __func__);
	kfree(bsi);
	return 0;
}

static int bluesleep_resume(struct platform_device *pdev)
{
	printk("%s", __func__);
	if (test_bit(BT_SUSPEND, &flags)) {
		sw_gpio_eint_set_enable(bsi->host_wake, 0);
    sw_gpio_setcfg(bsi->host_wake, GPIO_CFG_INPUT);
    
		clear_bit(BT_SUSPEND, &flags);
		set_bit(BT_ACTIVE, &flags);
	}

	return 0;
}

static int bluesleep_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk("%s", __func__); 
	if(test_bit(BT_ACTIVE, &flags)) {
		
		sw_gpio_setcfg(bsi->host_wake, R_GPIO_CFG_EINT);
	  sw_gpio_setpull(bsi->host_wake, 1);
	  
		sw_gpio_eint_set_enable(bsi->host_wake, 1);
		
		clear_bit(BT_ACTIVE, &flags);
	  set_bit(BT_SUSPEND, &flags);
	}
	
	return 0;
}

static struct resource bluesleep_resource[] = {
	[0] = {
		.start	= AW_IRQ_EINTM,
		.end	= AW_IRQ_EINTM,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device bluesleep_device[] = {
	[0] = {
		.name           	= "rtl_bluesleep",
		.id             	= 0,
		.num_resources		= ARRAY_SIZE(bluesleep_resource),
		.resource       	= bluesleep_resource,
	}
};

static struct platform_driver bluesleep_driver = {
	.probe = bluesleep_probe,
	.remove = bluesleep_remove,
	.suspend = bluesleep_suspend,
	.resume = bluesleep_resume,
	.driver = {
		.name = "rtl_bluesleep",
		.owner = THIS_MODULE,
	},
};

/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init bluesleep_init(void)
{
	int retval;
  struct proc_dir_entry *ent;

	BT_INFO("BlueSleep Mode Driver Ver %s", VERSION);

	retval = platform_driver_register(&bluesleep_driver);
	if (retval)
		goto fail;

  retval = platform_device_register(&bluesleep_device[0]);
	if (retval) {
		printk("%s platform_device_register failed.", __FUNCTION__);
		goto unreg_drv;
	}

  bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR("Unable to create /proc/bluetooth directory");
		goto unreg_drv;
	}

	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
	if (sleep_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
		goto rm_bt_dir;
	}
	
	/* read/write proc entries */
	ent = create_proc_entry("lpm", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/lpm entry", PROC_DIR);
		retval = -ENOMEM;
		goto rm_sleep_dir;
	}
	ent->read_proc = bluesleep_read_proc_lpm;
	ent->write_proc = bluesleep_write_proc_lpm;

	return 0;

rm_sleep_dir:
  remove_proc_entry("sleep", bluetooth_dir);
rm_bt_dir:
  remove_proc_entry("bluetooth", 0);
unreg_drv:
  platform_driver_unregister(&bluesleep_driver);
fail:
	return retval;
}

/**
 * Cleans up the module.
 */
static void __exit bluesleep_exit(void)
{ 
	platform_driver_unregister(&bluesleep_driver);
	
	remove_proc_entry("lpm", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver %s " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
