/*
 * USI WCDMA Modem PM Control driver
 *
 * Copyright (C) 2011 Mickey Lin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */


/*!
 * @file usi_modem_ctrl.c
 *
 * @brief Main file for GPIO kernel module. Contains driver entry/exit
 *
 */

#include <linux/fs.h>       /* Async notification */
#include <linux/uaccess.h>  /* for get_user, put_user, access_ok */
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>    /* for msleep */
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/module.h>


#define DEBUG
#ifdef DEBUG
#define dbg(format, arg...)						\
	printk(KERN_WARNING "%s: " format "\n", __FILE__, ##arg)
#else
#define dbg(format, arg...)						\
do {									\
	if (0)								\
		printk(KERN_DEBUG "%s: " format "\n", __FILE__, ##arg); \
} while (0)
#endif


//#define USI_MODEM_POWERKEY  	(0*32 + 0)      /* Customize by different HW layout and GPIO pin mapping */
#define USI_MODEM_USBEN      	(0*32 + 3)      /* Customize by different HW layout and GPIO pin mapping */
#define USI_MODEM_UARTEN  	(0*32 + 1)      /* Customize by different HW layout and GPIO pin mapping */
#define USI_MODEM_APReady  	(0*32 + 2)      /* Customize by different HW layout and GPIO pin mapping */
#define USI_MODEM_APWAKE	(0*32 + 0)      /* Customize by different HW layout and GPIO pin mapping */

#define USI_MODEM_CTRL_DEVICE_FILE_NAME "modemctrl"
#define USI_MODEM_DEVICE_FILE_NAME "/dev/ttyACM0"
#define USI_MODEM_DEVICE_FILE_NAME1 "/dev/ttyACM1"
#define IRQ_BB_WAKEUP_AP_TRIGGER    IRQF_TRIGGER_RISING

#define BUF_LEN 2
#define POWERKEY_PULSE 50

typedef enum
{
    ACTIVE_LOW,
    ACTIVE_HIGH
} GPIO_ACTIVE;

static int Device_Open;     /* Only allow a single user of this device */
static struct cdev usi_modem_cdev;
static dev_t usi_modem_ctrl_dev;
static struct class *modem_class;
static struct device *modem_class_dev;
static int flight_mode_off = 1;
static int do_wakeup_irq = 0;
static struct wake_lock modem_wakelock;

/*************************************************************************
* FUNCTION
*    usi_modem_powerkey_high_pulse
*
* DESCRIPTION
*    GPIO make a high pulse
*
* PARAMETERS
*    time: the high pulse width
*
* RETURNS
*    None
*************************************************************************/
void usi_modem_powerkey_high_pulse(int time)
{
    #ifdef USI_MODEM_POWERKEY
    gpio_set_value(USI_MODEM_POWERKEY, 1);
    msleep(time);
    gpio_set_value(USI_MODEM_POWERKEY, 0);
    #endif
}

/*************************************************************************
* FUNCTION
*    modem_exist
*
* DESCRIPTION
*    test /dev/ttyUSB0 exist
*
* PARAMETERS
*    None
*
* RETURNS
*    0: Device is not exist, 1: Device exist
*************************************************************************/
int modem_exist(void)
{
    struct file *filp;
    struct file1 *filp1;
    
    filp = filp_open(USI_MODEM_DEVICE_FILE_NAME, O_RDONLY, 0);
    if (IS_ERR(filp)) {

        filp1 = filp_open(USI_MODEM_DEVICE_FILE_NAME1, O_RDONLY, 0);
        if (IS_ERR(filp1)) {
            dbg("No such Modem device: %s %s\n", USI_MODEM_DEVICE_FILE_NAME, USI_MODEM_DEVICE_FILE_NAME1);
            return 0;
        } else {
           filp_close(filp1, NULL);
           dbg("exist Modem device: %s\n", USI_MODEM_DEVICE_FILE_NAME1);
           return 1;
        }
      
    } else {
        filp_close(filp, NULL);
        dbg("exist Modem device: %s\n", USI_MODEM_DEVICE_FILE_NAME);
        return 1;
    }

}

static int device_open(struct inode *inode, struct file *fp)
{
    /* We don't want to talk to two processes at the same time. */
    if (Device_Open) {
        dbg("device_open() - Returning EBUSY. \
            Device already open... \n");
        return -EBUSY;
    }
    Device_Open++;
    try_module_get(THIS_MODULE);

    return 0;
}

static int device_release(struct inode *inode, struct file *fp)
{
    /* We're now ready for our next caller */
    Device_Open--;
    module_put(THIS_MODULE);

    return 0;
}

static ssize_t device_write(struct file *file, const char __user *buf,
             size_t len, loff_t *off)
{
    static char cmd[BUF_LEN];
    int ret = 0;
    int i;
    int modem_on = 0;
    
    if (len > BUF_LEN) {
        return -EINVAL;
    }

    ret = copy_from_user(&cmd, buf, len);
    if (ret != 0) {
        return -EFAULT;
    }

//dbg("[John]: cmd[0]= %d\n", cmd[0]);

    //Flight mode on
    if (cmd[0] == '0') {
    modem_on = modem_exist();
        if (modem_on) {
            #ifdef USI_MODEM_USBEN
	    gpio_set_value(USI_MODEM_USBEN, 0);
            #endif
            //msleep(1000);
            //#ifdef USI_MODEM_POWERKEY
            //usi_modem_powerkey_high_pulse(POWERKEY_PULSE);
            //#endif
            flight_mode_off = 0;
	}
    }
    
    //Flight mode off
    if (cmd[0] == '1') {
    modem_on = modem_exist();
	if (!modem_on) {
            #ifdef USI_MODEM_USBEN
	    gpio_set_value(USI_MODEM_USBEN, 1);
            #endif
            flight_mode_off = 1;
	}
    }

    //sleep mode diable
    if (cmd[0] == '2') {
            #ifdef USI_MODEM_UARTEN
            gpio_set_value(USI_MODEM_UARTEN, 0);
            #endif
    }

    //sleep mode enable
    if (cmd[0] == '3') {
            #ifdef USI_MODEM_UARTEN
            gpio_set_value(USI_MODEM_UARTEN, 1);
            #endif
    }

    //USBEN High , USB connect
    if (cmd[0] == '4') {
            #ifdef USI_MODEM_USBEN
	    gpio_set_value(USI_MODEM_USBEN, 1);
            #endif
    }

    //USBEN Low , USB disconnect
    if (cmd[0] == '5') {
            #ifdef USI_MODEM_USBEN
	    gpio_set_value(USI_MODEM_USBEN, 0);
            #endif
    }

    return len;
}

struct file_operations Fops = {
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

/* Initialize the module - Register the character device */
int init_chrdev(struct device *dev)
{
    int ret, usi_modem_major;

    dbg("%s, %s\n", __func__, "entry");
    ret = alloc_chrdev_region(&usi_modem_ctrl_dev, 1, 1, "usi_modem_ctrl");
    usi_modem_major = MAJOR(usi_modem_ctrl_dev);
    if (ret < 0) {
        dev_err(dev, "can't get major %d\n", usi_modem_major);
        goto err3;
    }

    cdev_init(&usi_modem_cdev, &Fops);
    usi_modem_cdev.owner = THIS_MODULE;

    ret = cdev_add(&usi_modem_cdev, usi_modem_ctrl_dev, 1);
    if (ret) {
        dev_err(dev, "can't add cdev\n");
        goto err2;
    }

    /* create class and device for udev information */
    modem_class = class_create(THIS_MODULE, "modem");
    if (IS_ERR(modem_class)) {
        dev_err(dev, "failed to create modem class\n");
        ret = -ENOMEM;
        goto err1;
    }

    modem_class_dev = device_create(modem_class, NULL, MKDEV(usi_modem_major, 1), NULL,
                     USI_MODEM_CTRL_DEVICE_FILE_NAME);
    if (IS_ERR(modem_class_dev)) {
        dev_err(dev, "failed to create modem ctrl class device\n");
        ret = -ENOMEM;
        goto err0;
    }

    return 0;
err0:
    class_destroy(modem_class);
err1:
    cdev_del(&usi_modem_cdev);
err2:
    unregister_chrdev_region(usi_modem_ctrl_dev, 1);
err3:
    return ret;
}

/* Cleanup - unregister the appropriate file from /proc. */
void cleanup_chrdev(void)
{
    dbg("%s, %s\n", __func__, "entry");
    /* destroy gps device class */
    device_destroy(modem_class, MKDEV(MAJOR(usi_modem_ctrl_dev), 1));
    class_destroy(modem_class);

    /* Unregister the device */
    cdev_del(&usi_modem_cdev);
    unregister_chrdev_region(usi_modem_ctrl_dev, 1);
}

static irqreturn_t detect_irq_handler(int irq, void *dev_id)
{
    if(do_wakeup_irq)
    {
        do_wakeup_irq = 0;
        wake_lock_timeout(&modem_wakelock, 10 * HZ);
        //schedule_delayed_work(&wakeup_work, 2*HZ);
    }else
    	return IRQ_NONE;
    	
    return IRQ_HANDLED;
}

/*!
 * This function initializes the driver in terms of memory of the soundcard
 * and some basic HW clock settings.
 *
 * @return              0 on success, -1 otherwise.
 */
static int __init usi_modem_ioctrl_probe(struct platform_device *pdev)
{
    int grc, irq = 0;
    
    dbg("%s, %s\n", __func__, "entry");
    
    #ifdef USI_MODEM_USBEN
    /* MODEM_USBEN */
    grc = gpio_request(USI_MODEM_USBEN, "modem-usben");
    if (grc)
        goto err_gpio_usben_req;

    grc = gpio_direction_output(USI_MODEM_USBEN, 1);
    if (grc)
        goto err_gpio_usben_dir;
    #endif

    #ifdef USI_MODEM_APReady
    /* MODEM_APReady */
    grc = gpio_request(USI_MODEM_APReady, "modem-apready");
    if (grc)
        goto err_gpio_apready_req;

    grc = gpio_direction_output(USI_MODEM_APReady, 1);
    if (grc)
        goto err_gpio_apready_dir;
    #endif

    #ifdef USI_MODEM_POWERKEY
    /* MODEM_POWERKEY */
    grc = gpio_request(USI_MODEM_POWERKEY, "modem-powerkey");
    if (grc)
        goto err_gpio_powerkey_req;

    grc = gpio_direction_output(USI_MODEM_POWERKEY, 0);
    if (grc)
        goto err_gpio_powerkey_dir;
    #endif

    #ifdef USI_MODEM_UARTEN
    /* MODEM_UARTEN */
    grc = gpio_request(USI_MODEM_UARTEN, "modem-uarten");
    if (grc)
        goto err_gpio_uarten_req;

    grc = gpio_direction_output(USI_MODEM_UARTEN, 1);
    if (grc)
        goto err_gpio_uarten_dir;
    #endif    
    
    #ifdef USI_MODEM_APWAKE
    /* MODEM_APWAKE */
    grc = gpio_request(USI_MODEM_APWAKE, "modem-apawake");
    if (grc) {
	goto err_gpio_apwake_req;
    }
    gpio_direction_input(USI_MODEM_APWAKE);
    irq	= gpio_to_irq(USI_MODEM_APWAKE);
    if(irq < 0)
    {
	gpio_free(USI_MODEM_APWAKE);
	printk("failed to request modem-apawake\n");
    }
    grc = request_irq(irq, detect_irq_handler, IRQ_BB_WAKEUP_AP_TRIGGER | IRQF_DISABLED | IRQF_SHARED, "modem-apawake", NULL);
    if (grc < 0) {
	printk("%s: request_irq(%d) failed\n", __func__, irq);
	gpio_free(USI_MODEM_APWAKE);
	goto err_gpio_apwake_dir;
    }
    enable_irq_wake(irq);
    wake_lock_init(&modem_wakelock, WAKE_LOCK_SUSPEND, "modem-apawake");
    #endif

    /* Check Modem exist */
    modem_exist();

    /* Register character device */
    init_chrdev(&(pdev->dev));
    return 0;

#ifdef USI_MODEM_POWERKEY
err_gpio_powerkey_dir:
    printk(KERN_ERR "%s: USI_MODEM_POWERKEY gpio(%d) setting failed\n", __func__, USI_MODEM_POWERKEY);
    gpio_free(USI_MODEM_POWERKEY);

err_gpio_powerkey_req:
    printk(KERN_ERR "%s: USI_MODEM_POWERKEY gpio(%d) request failed\n", __func__, USI_MODEM_POWERKEY);
#endif

#ifdef USI_MODEM_UARTEN
err_gpio_uarten_dir:
    printk(KERN_ERR "%s: USI_MODEM_UARTEN gpio(%d) setting failed\n", __func__, USI_MODEM_UARTEN);
    gpio_free(USI_MODEM_UARTEN);

err_gpio_uarten_req:
    printk(KERN_ERR "%s: USI_MODEM_UARTEN gpio(%d) request failed\n", __func__, USI_MODEM_UARTEN);
#endif

#ifdef USI_MODEM_USBEN
err_gpio_usben_dir:
    printk(KERN_ERR "%s: USI_MODEM_USBEN gpio(%d) setting failed\n", __func__, USI_MODEM_USBEN);
    gpio_free(USI_MODEM_USBEN);

err_gpio_usben_req:
    printk(KERN_ERR "%s: USI_MODEM_USBEN gpio(%d) request failed\n", __func__, USI_MODEM_USBEN);
#endif

#ifdef USI_MODEM_APReady
err_gpio_apready_dir:
    printk(KERN_ERR "%s: USI_MODEM_APReady gpio(%d) setting failed\n", __func__, USI_MODEM_APReady);
    gpio_free(USI_MODEM_APReady);

err_gpio_apready_req:
    printk(KERN_ERR "%s: USI_MODEM_APReady gpio(%d) request failed\n", __func__, USI_MODEM_APReady);
#endif

#ifdef USI_MODEM_APWAKE
err_gpio_apwake_dir:
    printk(KERN_ERR "%s: USI_MODEM_APWAKE gpio(%d) setting failed\n", __func__, USI_MODEM_APWAKE);
    gpio_free(USI_MODEM_APWAKE);

err_gpio_apwake_req:
    printk(KERN_ERR "%s: USI_MODEM_APWAKE gpio(%d) request failed\n", __func__, USI_MODEM_APWAKE);
#endif
    return grc;
}

static int usi_modem_ioctrl_remove(struct platform_device *pdev)
{
    dbg("%s, %s\n", __func__, "entry");

    /* Character device cleanup.. */
    cleanup_chrdev();
    
    #ifdef USI_MODEM_USBEN   
    gpio_set_value(USI_MODEM_USBEN, 0);
    gpio_free(USI_MODEM_USBEN);
    #endif

    #ifdef USI_MODEM_UARTEN
    gpio_set_value(USI_MODEM_UARTEN, 1);
    gpio_free(USI_MODEM_UARTEN);
    #endif

    #ifdef USI_MODEM_APReady
    gpio_set_value(USI_MODEM_APReady, 0);
    gpio_free(USI_MODEM_APReady);
    #endif

    #ifdef USI_MODEM_POWERKEY
    usi_modem_powerkey_high_pulse(POWERKEY_PULSE);
    gpio_free(USI_MODEM_POWERKEY);
    #endif

    #ifdef USI_MODEM_APWAKE
    gpio_free(USI_MODEM_APWAKE);
    #endif

    return 0;
}

static int usi_modem_ioctrl_suspend(struct platform_device *pdev, pm_message_t state)
{
    dbg("%s, %s\n", __func__, "entry");

    do_wakeup_irq = 1;

    if (flight_mode_off == 1){
    #ifdef USI_MODEM_USBEN
    gpio_set_value(USI_MODEM_USBEN, 0);
    #endif
    }

    #ifdef USI_MODEM_UARTEN
    gpio_set_value(USI_MODEM_UARTEN, 1);
    #endif

    #ifdef USI_MODEM_APReady
    gpio_set_value(USI_MODEM_APReady, 0);
    #endif

    return 0;
}

static int usi_modem_ioctrl_resume(struct platform_device *pdev)
{
    dbg("%s, %s\n", __func__, "entry");

    msleep(1000);
    if (flight_mode_off == 1){
    #ifdef USI_MODEM_USBEN
    gpio_set_value(USI_MODEM_USBEN, 1);
    #endif
    }
    msleep(2000);
    #ifdef USI_MODEM_APReady
    gpio_set_value(USI_MODEM_APReady, 1);
    #endif

    return 0;
}

static struct platform_driver usi_modem_ioctrl_driver = {
    .probe = usi_modem_ioctrl_probe,
    .remove = usi_modem_ioctrl_remove,
    .suspend = usi_modem_ioctrl_suspend,
    .resume = usi_modem_ioctrl_resume,
    .driver = {
           .name = "usi_modem_ioctrl",
           },
};

static struct platform_device usi_modem_ctrl_device = {
    .name = "usi_modem_ioctrl",
};

static struct platform_device *usi_modem_devices[] __initdata = {
    &usi_modem_ctrl_device,
};

/*!
 * Entry point for usi modem ioctrl module.
 *
 */
static int __init usi_modem_ioctrl_init(void)
{
    dbg("%s, %s\n", __func__, "entry");

    platform_add_devices(usi_modem_devices,
			ARRAY_SIZE(usi_modem_devices));

    return platform_driver_register(&usi_modem_ioctrl_driver);
}

/*!
 * unloading module.
 *
 */
static void __exit usi_modem_ioctrl_exit(void)
{
    dbg("%s, %s\n", __func__, "entry");
    platform_driver_unregister(&usi_modem_ioctrl_driver);
}

module_init(usi_modem_ioctrl_init);
module_exit(usi_modem_ioctrl_exit);
MODULE_DESCRIPTION("USI MODEM DEVICE CONTROL DRIVER");
MODULE_AUTHOR("USI Co. Ltd,. Mickey Lin");
MODULE_LICENSE("GPL");
