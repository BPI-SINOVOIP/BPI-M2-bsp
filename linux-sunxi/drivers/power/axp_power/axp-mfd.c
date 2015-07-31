/*
 * Base driver for AXP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <mach/system.h>

#include "axp-cfg.h"
#include "axp22-mfd.h"

#include <mach/sys_config.h>

static int power_start;

static void axp_mfd_irq_work(struct work_struct *work)
{
	struct axp_mfd_chip *chip = container_of(work, struct axp_mfd_chip, irq_work);
	uint64_t irqs = 0;
	
	while (1) {
		if (chip->ops->read_irqs(chip, &irqs)){
			printk("read irq fail\n");
			break;
		}
		irqs &= chip->irqs_enabled;
		if (irqs == 0){
			break;
		}
		
		if(irqs > 0xffffffff){
			blocking_notifier_call_chain(&chip->notifier_list, (uint32_t)(irqs>>32), (void *)1);
		}
		else{
			blocking_notifier_call_chain(&chip->notifier_list, (uint32_t)irqs, (void *)0);
		}
	}
#ifdef	CONFIG_AXP_TWI_USED
	enable_irq(chip->client->irq);
#else
	ar100_enable_axp_irq();
#endif
}

#ifdef	CONFIG_AXP_TWI_USED
static irqreturn_t axp_mfd_irq_handler(int irq, void *data)
{
	struct axp_mfd_chip *chip = data;
	disable_irq_nosync(irq);
	(void)schedule_work(&chip->irq_work);

	return IRQ_HANDLED;
}
#else
static int axp_mfd_irq_cb(void *arg)
{
	struct axp_mfd_chip *chip = (struct axp_mfd_chip *)arg;
	/* when process axp irq, the axp irq ar100 cpu must disable now,
	 * we just need re-enable axp irq when process finished.
	 * by sunny at 2012-11-29 10:25:30.
	 */
	(void)schedule_work(&chip->irq_work);
	return 0;
}
#endif

static struct axp_mfd_chip_ops axp_mfd_ops[] = {
	[0] = {
		.init_chip    = axp22_init_chip,
		.enable_irqs  = axp22_enable_irqs,
		.disable_irqs = axp22_disable_irqs,
		.read_irqs    = axp22_read_irqs,
	},
};

static const struct i2c_device_id axp_mfd_id_table[] = {
	{ "axp22_mfd", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, axp_mfd_id_table);

int axp_mfd_create_attrs(struct axp_mfd_chip *chip)
{
	int j,ret;
	if (chip->type ==  AXP22){
		for (j = 0; j < ARRAY_SIZE(axp22_mfd_attrs); j++) {
			ret = device_create_file(chip->dev,&axp22_mfd_attrs[j]);
			if (ret)
			goto sysfs_failed;
		}
	}
	else
		ret = 0;
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(chip->dev,&axp22_mfd_attrs[j]);
succeed:
	return ret;
}

static int __remove_subdev(struct device *dev, void *unused)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

static int axp_mfd_remove_subdevs(struct axp_mfd_chip *chip)
{
	return device_for_each_child(chip->dev, NULL, __remove_subdev);
}

static int __devinit axp_mfd_add_subdevs(struct axp_mfd_chip *chip,
					struct axp_platform_data *pdata)
{
	struct axp_funcdev_info *regl_dev;
	struct axp_funcdev_info *sply_dev;
	struct axp_funcdev_info *gpio_dev;
	struct platform_device *pdev;
	int i, ret = 0;

	/* register for regultors */
	for (i = 0; i < pdata->num_regl_devs; i++) {
		regl_dev = &pdata->regl_devs[i];
		pdev = platform_device_alloc(regl_dev->name, regl_dev->id);
		pdev->dev.parent = chip->dev;
		pdev->dev.platform_data = regl_dev->platform_data;
		ret = platform_device_add(pdev);
		if (ret)
			goto failed;
	}

	/* register for power supply */
	for (i = 0; i < pdata->num_sply_devs; i++) {
	sply_dev = &pdata->sply_devs[i];
	pdev = platform_device_alloc(sply_dev->name, sply_dev->id);
	pdev->dev.parent = chip->dev;
	pdev->dev.platform_data = sply_dev->platform_data;
	ret = platform_device_add(pdev);
	if (ret)
		goto failed;

	}

	/* register for gpio */
	for (i = 0; i < pdata->num_gpio_devs; i++) {
	gpio_dev = &pdata->gpio_devs[i];
	pdev = platform_device_alloc(gpio_dev->name, gpio_dev->id);
	pdev->dev.parent = chip->dev;
	pdev->dev.platform_data = gpio_dev->platform_data;
	ret = platform_device_add(pdev);
	if (ret)
		goto failed;
	}

	return 0;

failed:
	axp_mfd_remove_subdevs(chip);
	return ret;
}

static void axp_power_off(void)
{
	uint8_t val;

#if defined (CONFIG_AW_AXP22)
	if(pmu_pwroff_vol >= 2600 && pmu_pwroff_vol <= 3300){
		if (pmu_pwroff_vol > 3200){
			val = 0x7;
		}
		else if (pmu_pwroff_vol > 3100){
			val = 0x6;
		}
		else if (pmu_pwroff_vol > 3000){
			val = 0x5;
		}
		else if (pmu_pwroff_vol > 2900){
			val = 0x4;
		}
		else if (pmu_pwroff_vol > 2800){
			val = 0x3;
		}
		else if (pmu_pwroff_vol > 2700){
			val = 0x2;
		}
		else if (pmu_pwroff_vol > 2600){
			val = 0x1;
		}
		else
			val = 0x0;

		axp_update(&axp->dev, AXP22_VOFF_SET, val, 0x7);
	}
	val = 0xff;
    printk("[axp] send power-off command!\n");
    mdelay(20);
    if(power_start != 1){
		axp_read(&axp->dev, AXP22_STATUS, &val);
		if(val & 0xF0){
	    	axp_read(&axp->dev, AXP22_MODE_CHGSTATUS, &val);
	    	if(val & 0x20){
			printk("[axp] set flag!\n");
			axp_read(&axp->dev, AXP22_BUFFERC, &val);
			if (0x0d != val)
				axp_write(&axp->dev, AXP22_BUFFERC, 0x0f);
			mdelay(20);
		    	printk("[axp] reboot!\n");
		    	arch_reset(0,NULL);
		    	printk("[axp] warning!!! arch can't ,reboot, maybe some error happend!\n");
	    	}
		}
	}
    axp_read(&axp->dev, AXP22_BUFFERC, &val);
    if (0x0d != val)
        axp_write(&axp->dev, AXP22_BUFFERC, 0x00);
    mdelay(20);
	axp_set_bits(&axp->dev, AXP22_OFF_CTL, 0x80);
    mdelay(20);
    printk("[axp] warning!!! axp can't power-off, maybe some error happend!\n");

#endif
}

static int __devinit axp_mfd_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct axp_platform_data *pdata = client->dev.platform_data;
	struct axp_mfd_chip *chip;
	int ret;
	script_item_u script_val;
	script_item_value_type_e type;


	chip = kzalloc(sizeof(struct axp_mfd_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	axp = client;

	chip->client = client;
	chip->dev = &client->dev;
	chip->ops = &axp_mfd_ops[id->driver_data];

	mutex_init(&chip->lock);
	INIT_WORK(&chip->irq_work, axp_mfd_irq_work);
	BLOCKING_INIT_NOTIFIER_HEAD(&chip->notifier_list);

	i2c_set_clientdata(client, chip);

	ret = chip->ops->init_chip(chip);
	if (ret)
		goto out_free_chip;

#ifdef	CONFIG_AXP_TWI_USED
	ret = request_irq(client->irq, axp_mfd_irq_handler,
		IRQF_SHARED|IRQF_DISABLED, "axp_mfd", chip);
  	if (ret) {
  		dev_err(&client->dev, "failed to request irq %d\n",
  				client->irq);
  		goto out_free_chip;
  	}
#else
	ret = ar100_axp_cb_register(axp_mfd_irq_cb, chip);
	if (ret) {
  		dev_err(&client->dev, "failed to reg irq cb %d\n",
  				client->irq);
  		goto out_free_chip;
  	}
  	/* enable ar100 system axp irq,
  	 * by sunny at 2013-1-10 9:00:59.
  	 */
  	ar100_enable_axp_irq();
#endif

	ret = axp_mfd_add_subdevs(chip, pdata);
	if (ret)
		goto out_free_irq;

	/* PM hookup */
	if(!pm_power_off) {
               type = script_get_item("pmu_para", "pmu_fake_power_off_enable", &script_val);
               if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
                       printk("pmu fake power off config type err!");
                       script_val.val = 0;
               }
               if (script_val.val) {
                       pm_power_off = ar100_fake_power_off;
               } else {
                       pm_power_off = axp_power_off;
               }
       }

	ret = axp_mfd_create_attrs(chip);
	if(ret){
		return ret;
	}
	
	/* set ac/usb_in shutdown mean restart */
  	ret = axp_script_parser_fetch("pmu_para", "power_start", &power_start, sizeof(int));
  	if (ret)
  	{
    	printk("[AXP]axp driver uning configuration failed(%d)\n", __LINE__);
     	power_start = 0;
     	printk("[AXP]power_start = %d\n",power_start);
  	}
  	
	return 0;

out_free_irq:
	free_irq(client->irq, chip);

out_free_chip:
	i2c_set_clientdata(client, NULL);
	kfree(chip);

	return ret;
}

static int __devexit axp_mfd_remove(struct i2c_client *client)
{
	struct axp_mfd_chip *chip = i2c_get_clientdata(client);

	pm_power_off = NULL;
	axp = NULL;

	axp_mfd_remove_subdevs(chip);
	kfree(chip);
	return 0;
}

static struct i2c_driver axp_mfd_driver = {
	.driver	= {
		.name	= "axp_mfd",
		.owner	= THIS_MODULE,
	},
	.probe		= axp_mfd_probe,
	.remove		= __devexit_p(axp_mfd_remove),
	.id_table	= axp_mfd_id_table,
};

static int __init axp_mfd_init(void)
{
	return i2c_add_driver(&axp_mfd_driver);
}
subsys_initcall(axp_mfd_init);

static void __exit axp_mfd_exit(void)
{
	i2c_del_driver(&axp_mfd_driver);
}
module_exit(axp_mfd_exit);

MODULE_DESCRIPTION("PMIC MFD Driver for AXP");
MODULE_AUTHOR("Weijin Zhong X-POWERS");
MODULE_LICENSE("GPL");
