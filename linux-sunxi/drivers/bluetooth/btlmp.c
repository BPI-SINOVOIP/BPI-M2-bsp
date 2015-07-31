/*

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.


   Copyright (C) 2006-2007 - Motorola
   Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.

   Date         Author           Comment
   -----------  --------------   --------------------------------
   2006-Apr-28	Motorola	 The kernel module for running the Bluetooth(R)
				 Sleep-Mode Protocol from the Host side
   2006-Sep-08  Motorola         Added workqueue for handling sleep work.
   2007-Jan-24  Motorola         Added mbm_handle_ioi() call to ISR.
   2013-Jun-13	Magic            Modified for EVB
   2013-Mar-27	Magic            Modified for support BLE
*/

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/wakelock.h>
#include <linux/serial_8250.h>
#include <linux/gpio.h>
#include <mach/sys_config.h>
#include <mach/irqs.h>
#include <mach/gpio.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include "hci_uart.h"

//#define BT_DEBUG 1
#ifdef  BT_DEBUG
#define BT_LPM_DBG(fmt, arg...)  printk(KERN_ERR "[BT_LPM] %s: " fmt "\n" , __func__ , ## arg)
#else
#define BT_LPM_DBG(fmt, arg...) 
#endif


/*
 * related defines
 */

#define VERSION		"1.2"
#define PROC_DIR	"bluetooth/sleep"

static void bluesleep_stop(void);
static int bluesleep_start(void);

struct bluesleep_info {
	unsigned host_wake;
	unsigned ext_wake;
	unsigned host_wake_irq;
	struct wake_lock wake_lock;
	struct uart_port *uport;
};

/* aw platform irq handle */
unsigned int irq_handle = 0;
unsigned int btlpm_irq_handle(void *para);

/* work function */
static void bluesleep_sleep_work(struct work_struct *work);

/* work queue */
DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);

/* Macros for handling sleep work */
#define bluesleep_rx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_rx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_idle()     schedule_delayed_work(&sleep_workqueue, 0)

/* 10 second timeout */
#define TX_TIMER_INTERVAL	10

/* state variable names and bit positions */
#define BT_PROTO	0x01
#define BT_TXDATA	0x02
#define BT_ASLEEP	0x04

/* variable use indicate lpm modle */
static bool has_lpm_enabled = false;

/* struct use save platform_device from uart */
static struct platform_device *bluesleep_uart_dev;

static struct bluesleep_info *bsi;

/* module usage */
static atomic_t open_count = ATOMIC_INIT(1);

/** Global state flags */
static unsigned long flags;

/** Tasklet to respond to change in hostwake line */
static struct tasklet_struct hostwake_task;

/** Transmission timer */
static struct timer_list tx_timer;

/** Lock for state transitions */
static spinlock_t rw_lock;

struct proc_dir_entry *bluetooth_dir, *sleep_dir;

/*
 * Local functions
 */

/*
 * bt go to sleep will call this function tell uart stop data interactive
 */
static void hsuart_power(int on)
{
	if (on) {
		BT_LPM_DBG("hsuart_power HS UART ON");
		bsi->uport->ops->set_mctrl(bsi->uport, TIOCM_RTS);
	} else {
		BT_LPM_DBG("hsuart_power HS UART OFF");
		bsi->uport->ops->set_mctrl(bsi->uport, 0);
	}
}

/**
 * @return 1 if the Host can go to sleep, 0 otherwise.
 */
static inline int bluesleep_can_sleep(void)
{
	/* check if HOST_WAKE_BT_GPIO and BT_WAKE_HOST_GPIO are both deasserted */
	return !__gpio_get_value(bsi->ext_wake) &&
#ifndef CONFIG_BT_LOW_LEVEL_TRIGGER
		!__gpio_get_value(bsi->host_wake) &&
#else
		__gpio_get_value(bsi->host_wake) &&
#endif // CONFIG_BT_LOW_LEVEL_TRIGGER
		(bsi->uport != NULL);
}

/*
 * after bt wakeup should clean BT_ASLEEP flag and start time.
 */
void bluesleep_sleep_wakeup(void)
{
	BT_LPM_DBG("%s enter.\n", __FUNCTION__);
	if (test_bit(BT_ASLEEP, &flags)) {
		/* Start the timer */
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
		BT_LPM_DBG("%s start to time.", __FUNCTION__);
		__gpio_set_value(bsi->ext_wake, 1);
		clear_bit(BT_ASLEEP, &flags);
		/*Activating UART */
		hsuart_power(1);
	}
}

/**
 * @brief@  main sleep work handling function which update the flags
 * and activate and deactivate UART ,check FIFO.
 */
static void bluesleep_sleep_work(struct work_struct *work)
{
	if (bluesleep_can_sleep()) {
		/* already asleep, this is an error case */
		if (test_bit(BT_ASLEEP, &flags)) {
			BT_LPM_DBG("already asleep.");
			return;
		}
		if (bsi->uport->ops->tx_empty(bsi->uport)) {
			BT_LPM_DBG("bt going to sleep.");
			set_bit(BT_ASLEEP, &flags);
			/*Deactivating UART */
			hsuart_power(0);
			wake_lock_timeout(&bsi->wake_lock, HZ / 2);
		} else {
		  	mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
			return;
		}
	} else if(!__gpio_get_value(bsi->ext_wake) && !test_bit(BT_ASLEEP, &flags) ) {
		__gpio_set_value(bsi->ext_wake, 1);
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
	}else {
		bluesleep_sleep_wakeup();
	}
}

/**
 * A tasklet function that runs in tasklet context and reads the value
 * of the HOST_WAKE GPIO pin and further defer the work.
 * @param data Not used.
 */
static void bluesleep_hostwake_task(unsigned long data)
{
	spin_lock(&rw_lock);

#ifndef CONFIG_BT_LOW_LEVEL_TRIGGER
	if (__gpio_get_value(bsi->host_wake))
#else
	if (!__gpio_get_value(bsi->host_wake))
#endif // CONFIG_BT_LOW_LEVEL_TRIGGER
		bluesleep_rx_busy();
	else
		bluesleep_rx_idle();

	spin_unlock(&rw_lock);
}

/**
 * Handles proper timer action when outgoing data is delivered to the
 * HCI line discipline. Sets BT_TXDATA.
 */
static void bluesleep_outgoing_data(void)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	/* log data passing by */
	set_bit(BT_TXDATA, &flags);
	/* if the tx side is sleeping... */
	if (!__gpio_get_value(bsi->ext_wake)) {
		BT_LPM_DBG("tx was sleeping.");
		bluesleep_sleep_wakeup();
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

static struct uart_port *bluesleep_get_uart_port(void)
{
	struct uart_port *uport = NULL;
	BT_LPM_DBG("%s enter.", __FUNCTION__);
	if (bluesleep_uart_dev) {
		uport = platform_get_drvdata(bluesleep_uart_dev);
		BT_LPM_DBG("%s get uart_port from blusleep_uart_dev: %s, port irq: %d", 
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

static int bluesleep_read_proc_btwrite(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "unsupported to read\n");
}

static int bluesleep_write_proc_btwrite(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	/* HCI_DEV_WRITE */
	if (b != '0') {
		bluesleep_outgoing_data();
	}

	return count;
}

/**
 * Handles transmission timer expiration.
 * @param data Not used.
 */
static void bluesleep_tx_timer_expire(unsigned long data)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	BT_LPM_DBG("Tx timer expired");

	/* were we silent during the last timeout */
	if (!test_bit(BT_TXDATA, &flags)) {
		BT_LPM_DBG("Tx has been idle");
		__gpio_set_value(bsi->ext_wake, 0);
		bluesleep_tx_idle();
	} else {
		BT_LPM_DBG("Tx data during last period");
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	}

	/* clear the incoming data flag */
	clear_bit(BT_TXDATA, &flags);

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

/**
 * Schedules a tasklet to run when receiving an interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id)
{
	/* schedule a tasklet to handle the change in the host wake line */
	tasklet_schedule(&hostwake_task);
	wake_lock(&bsi->wake_lock);
	BT_LPM_DBG("%s line %d\n", __FUNCTION__, __LINE__);
	return IRQ_HANDLED;
}

/**
 *AW platform irq handle function.
 *eint_handle will call the real irq function.
 *@param para Not used.
*/
unsigned int btlpm_irq_handle(void *para)
{
	bluesleep_hostwake_isr(0, NULL);
	return 0;	
}

/**
 * Starts the Sleep-Mode Protocol on the Host.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int bluesleep_start(void)
{
	int retval = 0;
	unsigned long irq_flags;

	BT_LPM_DBG("%s enter.", __FUNCTION__);
	spin_lock_irqsave(&rw_lock, irq_flags);

	if (test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return retval;
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);

	if (!atomic_dec_and_test(&open_count)) {
		atomic_inc(&open_count);
		return -EBUSY;
	}

	/* start the timer */

	mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));

	/* assert BT_WAKE */
	__gpio_set_value(bsi->ext_wake, 1);

#ifndef CONFIG_BT_LOW_LEVEL_TRIGGER
	irq_handle = sw_gpio_irq_request(bsi->host_wake, TRIG_EDGE_POSITIVE,
#else
	irq_handle = sw_gpio_irq_request(bsi->host_wake, TRIG_EDGE_NEGATIVE,
#endif // CONFIG_BT_LOW_LEVEL_TRIGGER
						 btlpm_irq_handle, NULL);

	if (irq_handle == 0) {
		BT_ERR("Couldn't acquire bt_host_wake IRQ or enable it");
		retval = -1;	
		goto fail;
	}

	set_bit(BT_PROTO, &flags);
	wake_lock(&bsi->wake_lock);

	return retval;
fail:
	del_timer(&tx_timer);
	atomic_inc(&open_count);

	return retval;
}

/**
 * Stops the Sleep-Mode Protocol on the Host.
 */
static void bluesleep_stop(void)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	if (!test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return;
	}

	/* assert BT_WAKE */
	__gpio_set_value(bsi->ext_wake, 1);
	del_timer(&tx_timer);
	clear_bit(BT_PROTO, &flags);

	if (test_bit(BT_ASLEEP, &flags)) {
		clear_bit(BT_ASLEEP, &flags);
		hsuart_power(1);
	}

	atomic_inc(&open_count);

	spin_unlock_irqrestore(&rw_lock, irq_flags);

	if (irq_handle != 0)
		sw_gpio_irq_free(irq_handle);

	wake_lock_timeout(&bsi->wake_lock, HZ / 2);

}
/**
 * Read the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the
 * pin is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_btwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "btwake:%u\n", __gpio_get_value(bsi->ext_wake));
}

/**
 * Write the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluepower_write_proc_btwake(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char *buf;

	if (count < 1)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}

	if (buf[0] == '0') {
		__gpio_set_value(bsi->ext_wake, 0);
	} else if (buf[0] == '1') {
		__gpio_set_value(bsi->ext_wake, 1);
	} else {
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);
	return count;
}

/**
 * Read the <code>BT_HOST_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the pin
 * is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_hostwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "hostwake: %u \n", __gpio_get_value(bsi->host_wake));
}


/**
 * Read the low-power status of the Host via the proc interface.
 * When this function returns, <code>page</code> contains a 1 if the Host
 * is asleep, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_asleep(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int asleep;

	asleep = test_bit(BT_ASLEEP, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "asleep: %u\n", asleep);
}

/**
 * Read the low-power protocol being used by the Host via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the Host
 * is using the Sleep Mode Protocol, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_proto(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int proto;

	proto = test_bit(BT_PROTO, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "proto: %u\n", proto);
}

/**
 * Modify the low-power protocol used by the Host via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluesleep_write_proc_proto(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char proto;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&proto, buffer, 1))
		return -EFAULT;

	if (proto == '0')
		bluesleep_stop();
	else
		bluesleep_start();

	/* claim that we wrote everything */
	return count;
}

void bluesleep_setup_uart_port(struct platform_device *uart_dev)
{
	BT_LPM_DBG("%s enter.", __FUNCTION__);
	bluesleep_uart_dev = uart_dev;
	BT_LPM_DBG("%s device name: %s.", __FUNCTION__, bluesleep_uart_dev->name);
}
EXPORT_SYMBOL(bluesleep_setup_uart_port);

static int __init bluesleep_probe(struct platform_device *pdev)
{
	int ret;
	int host_wake_bt = 0;
	int bt_wake_host = 0;
	script_item_u val;
	script_item_value_type_e type;

	BT_LPM_DBG("%s enter.", __FUNCTION__);

	bsi = kmalloc(sizeof(struct bluesleep_info), GFP_KERNEL);
	if (!bsi)
		return -ENOMEM;

	//get bt_wake & bt_host_wake from sys_config.fex
	type = script_get_item("wifi_para", "ap6xxx_bt_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		BT_ERR("get ap6xxx ap6xxx_bt_wake gpio failed\n");
	else
		host_wake_bt = val.gpio.gpio;	
	
	type = script_get_item("wifi_para", "ap6xxx_bt_host_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		BT_ERR("get ap6xxx ap6xxx_bt_host_wake gpio failed\n");
	else
		bt_wake_host = val.gpio.gpio;		
	
	//1.get bt_host_wake gpio number
	bsi->host_wake = bt_wake_host;
	wake_lock_init(&bsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");
	BT_LPM_DBG("%s line %d\n", __FUNCTION__, __LINE__);
	
	//2.set bt_host_wake as input function
	ret = gpio_request_one(bsi->host_wake, GPIOF_IN, NULL);
	if (ret != 0) {
		BT_ERR("couldn't set bt_host_wake as input function\n");
		goto 	free_bt_host_wake;
	}
	
	//3.get bt_wake gpio number
	bsi->ext_wake = host_wake_bt;
	
	//4.set bt_wake as output and the level is 1, assert bt wake
	ret = gpio_request_one(bsi->ext_wake, GPIOF_OUT_INIT_HIGH, NULL);
	if (ret != 0) {
		BT_ERR("couldn't set bt_wake as output function\n");
		goto free_bt_ext_wake;
	} 
	
	//5.get bt_host_wake gpio irq
	bsi->host_wake_irq = __gpio_to_irq(bsi->host_wake);
	if (bsi->host_wake_irq == -ENXIO) {
		BT_ERR("couldn't find host_wake irq\n");
		ret = -ENODEV;
		goto free_bt_ext_wake;
	}
	
	return 0;

free_bt_ext_wake:
	gpio_free(bsi->ext_wake);
free_bt_host_wake:
	gpio_free(bsi->host_wake);
	kfree(bsi);
	return ret;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	/* assert bt wake */
	__gpio_set_value(bsi->ext_wake, 1);
	if (test_bit(BT_PROTO, &flags)) {
		if (irq_handle != 0)
			sw_gpio_irq_free(irq_handle);

		del_timer(&tx_timer);
		if (test_bit(BT_ASLEEP, &flags))
			hsuart_power(1);
	}

	gpio_free(bsi->host_wake);
	gpio_free(bsi->ext_wake);
	wake_lock_destroy(&bsi->wake_lock);
	BT_LPM_DBG("%s line %d\n", __FUNCTION__, __LINE__);
	kfree(bsi);
	return 0;
}

static struct platform_driver bluesleep_driver = {
	.probe = bluesleep_probe,
	.remove = bluesleep_remove,
	.driver = {
		.name = "btlpm",
		.owner = THIS_MODULE,
	},
};

static struct resource bluesleep_resource[] = {
	[0] = {
		.start	= AW_IRQ_EINTM,
		.end	= AW_IRQ_EINTM,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device bluesleep_device[] = {
	[0] = {
		.name           	= "btlpm",
		.id             	= 0,
		.num_resources		= ARRAY_SIZE(bluesleep_resource),
		.resource       	= bluesleep_resource,
	}
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

	BT_LPM_DBG("MSM Sleep Mode Driver Ver %s", VERSION);

	retval = platform_driver_register(&bluesleep_driver);
	if (retval) {
		BT_LPM_DBG("%s platform_driver_register ok.", __FUNCTION__);
		return retval;
	}

	retval = platform_device_register(&bluesleep_device[0]);
	if (retval) {
		BT_LPM_DBG("%s platform_device_register ok.", __FUNCTION__);
		return retval;
	}

	bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR("Unable to create /proc/bluetooth directory");
		return -ENOMEM;
	}

	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
	if (sleep_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
		return -ENOMEM;
	}

	/* Creating read/write "btwake" entry */
	ent = create_proc_entry("btwake", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluepower_read_proc_btwake;
	ent->write_proc = bluepower_write_proc_btwake;

	/* read only proc entries */
	if (create_proc_read_entry("hostwake", 0, sleep_dir,
				bluepower_read_proc_hostwake, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/hostwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	/* read/write proc entries */
	ent = create_proc_entry("proto", 0666, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/proto entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_proto;
	ent->write_proc = bluesleep_write_proc_proto;

	/* read only proc entries */
	if (create_proc_read_entry("asleep", 0,
			sleep_dir, bluesleep_read_proc_asleep, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/asleep entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	/* read/write proc entries */
	ent = create_proc_entry("lpm", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/lpm entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_lpm;
	ent->write_proc = bluesleep_write_proc_lpm;

	/* read/write proc entries */
	ent = create_proc_entry("btwrite", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwrite entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_btwrite;
	ent->write_proc = bluesleep_write_proc_btwrite;

	flags = 0; /* clear all status bits */

	/* Initialize spinlock. */
	spin_lock_init(&rw_lock);

	/* Initialize timer */
	init_timer(&tx_timer);
	tx_timer.function = bluesleep_tx_timer_expire;
	tx_timer.data = 0;

	/* initialize host wake tasklet */
	tasklet_init(&hostwake_task, bluesleep_hostwake_task, 0);

	BT_LPM_DBG("%s ok....", __FUNCTION__);
	return 0;

fail:
	BT_LPM_DBG("%s fail.", __FUNCTION__);
	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
	return retval;
}

/**
 * Cleans up the module.
 */
static void __exit bluesleep_exit(void)
{
	platform_driver_unregister(&bluesleep_driver);

	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver %s " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
