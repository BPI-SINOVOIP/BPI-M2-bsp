/*
 * axp22x-gpio.c  --  gpiolib support for Krosspower &axp PMICs
 *
 * Copyright 2012 X-POWERS Microelectronics PLC.
 *
 * Author: Weijin Zhong <>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mfd/core.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/i2c.h>
#include <linux/mfd/axp-mfd.h>
#include <asm-generic/gpio.h>
#include <mach/gpio.h>
#include "axp-gpio.h"

struct virtual_gpio_data {
	struct mutex lock;
	int gpio;				//gpio number : 0/1/2/...
	int io;         //0: input      1: output
	int value;		  //0: low        1: high
};

int axp_gpio_set_io(int gpio, int io_mode)
{
	if(io_mode == 1){//output
		switch(gpio)
		{
			case 0: return axp_clr_bits(&axp->dev,AXP22_GPIO0_CFG, 0x06);
			case 1: return axp_clr_bits(&axp->dev,AXP22_GPIO1_CFG, 0x06);
			case 2: return 0;
			case 3: return axp_clr_bits(&axp->dev,AXP22_GPIO3_CFG, 0x08);
			case 4: return axp_clr_bits(&axp->dev,AXP22_GPIO4_CFG, 0x10);	
			default:return -ENXIO;
		}
	}
	else if(io_mode == 0){//input
		switch(gpio)
		{
			case 0: axp_clr_bits(&axp->dev,AXP22_GPIO0_CFG,0x05);
					return axp_set_bits(&axp->dev,AXP22_GPIO0_CFG,0x02);
			case 1: axp_clr_bits(&axp->dev,AXP22_GPIO1_CFG,0x05);
					return axp_set_bits(&axp->dev,AXP22_GPIO1_CFG,0x02);
			case 2:
					printk("This IO can not config as an input!"); 
					return -EINVAL;
			case 3: 
					printk("This IO can not config as an input!"); 
					return -EINVAL;
			case 4: 
					printk("This IO can not config as an input!"); 
					return -EINVAL;	
			default:return -ENXIO;
		}
	}
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(axp_gpio_set_io);


int axp_gpio_get_io(int gpio, int *io_mode)
{
	uint8_t val;
	switch(gpio)
	{
		case 0: axp_read(&axp->dev,AXP22_GPIO0_CFG,&val);val &= 0x07;
				if(val < 0x02)
					*io_mode = 1;
				else if (val == 0x02)
					*io_mode = 0;
				else
					return -EIO;
				break;
		case 1: axp_read(&axp->dev,AXP22_GPIO1_CFG,&val);val &= 0x07;
				if(val < 0x02)
					*io_mode = 1;
				else if (val == 0x02)
					*io_mode = 0;
				else
					return -EIO;
				break;
		case 2: *io_mode = 1;
				break;
		case 3: axp_read(&axp->dev,AXP22_GPIO3_CFG,&val);val &= 0x08;
				if(!val)
					*io_mode = 1;
				else
					return -EIO;	
				break;
		case 4: axp_read(&axp->dev,AXP22_GPIO4_CFG,&val);val &= 0x10;
				if(!val)
					*io_mode = 1;
				else
					return -EIO;			
				break;

		default:return -ENXIO;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(axp_gpio_get_io);


int axp_gpio_set_value(int gpio, int value)
{
	int io_mode,ret;
	ret = axp_gpio_get_io(gpio,&io_mode);
	if(ret)
		return ret;
	if(1 == io_mode){//output
		if(value){//high
			switch(gpio) {
				case 0: {
					axp_clr_bits(&axp->dev,AXP22_GPIO0_CFG,0x06);
					return axp_set_bits(&axp->dev,AXP22_GPIO0_CFG,0x01);
				}
				case 1: {
						axp_clr_bits(&axp->dev,AXP22_GPIO1_CFG,0x06);
						ret = axp_set_bits(&axp->dev,AXP22_GPIO1_CFG,0x01);
						return ret;
				}
				case 2: {
					return axp_set_bits(&axp->dev,AXP22_GPIO2_CFG,0x80);
				}
				case 3: {
					axp_clr_bits(&axp->dev,AXP22_GPIO3_CFG, 0x08);
					ret = axp_clr_bits(&axp->dev,AXP22_GPIO3_CFG,0x30);
					return ret;
				}
				case 4: {
					axp_clr_bits(&axp->dev,AXP22_GPIO4_CFG, 0x10);
					return axp_set_bits(&axp->dev,AXP22_GPIO4_STA,0x04);
				}
				default:break;
			}
		}
		else{//low
			switch(gpio)
			{
				case 0: axp_clr_bits(&axp->dev,AXP22_GPIO0_CFG,0x06);
						return axp_clr_bits(&axp->dev,AXP22_GPIO0_CFG,0x03);
				case 1: axp_clr_bits(&axp->dev,AXP22_GPIO1_CFG,0x06);
						return axp_clr_bits(&axp->dev,AXP22_GPIO1_CFG,0x03);
				case 2: return axp_clr_bits(&axp->dev,AXP22_GPIO2_CFG,0x80);
				case 3: axp_clr_bits(&axp->dev,AXP22_GPIO3_CFG, 0x08);
						return axp_set_bits(&axp->dev,AXP22_GPIO3_CFG,0x30);
				case 4: axp_clr_bits(&axp->dev,AXP22_GPIO4_CFG, 0x10);
						return axp_clr_bits(&axp->dev,AXP22_GPIO4_STA,0x04);
				default:break;
			}
		}
		return -ENXIO;
	}
	return -ENXIO;
}
EXPORT_SYMBOL_GPL(axp_gpio_set_value);


int axp_gpio_get_value(int gpio, int *value)
{
	int io_mode;
	int ret;
	uint8_t val;
	ret = axp_gpio_get_io(gpio,&io_mode);
//	printk("%s: line %d,%d,0x%x\n", __func__, __LINE__,gpio,(int)value);
	if(ret)
		return ret;
	if(1 == io_mode){//output
		switch(gpio)
		{
			case 0:
				ret = axp_read(&axp->dev,AXP22_GPIO0_CFG,&val);
				if(ret)
					{
						printk("Read AXP22_GPIO0 value fail! ");
					*value = 0;
						return -ENXIO;
				  }
				else
				   *value = val & 0x01;
				break;
			case 1:
				ret =axp_read(&axp->dev,AXP22_GPIO1_CFG,&val);
				if(ret)
					{
						printk("Read AXP22_GPIO1 value fail! ");
					*value = 0;
						return -ENXIO;
				  }
				*value = val & 0x01;
				break;
			case 2:
					ret =axp_read(&axp->dev,AXP22_GPIO2_CFG,&val);
					if(ret)
						{
							printk("Read AXP22_GPIO2 value fail! ");
						*value = 0;
							return -ENXIO;
					  }
				  *value = ((val & 0x80) >> 7);
					break;
			case 3:
					ret =axp_read(&axp->dev,AXP22_GPIO3_CFG,&val);
					if(ret)
						{
							printk("Read AXP22_GPIO3 value fail! ");
						*value = 0;
							return -ENXIO;
					  }
					  if((val & 0x30) == 0x30)
					  	{
					  		*value = 0;
					  	}
					  	else
					  		{
					  			*value = 1;
					  		}
					break;
			case 4:
					ret =axp_read(&axp->dev,AXP22_GPIO4_STA,&val);
						if(ret)
							{
								printk("Read AXP22_GPIO4 value fail! ");
							*value = 0;
								return -ENXIO;
						  }
						  *value = ((val & 0x04) >> 2);
						break;
			default:return -ENXIO;
		}
	}
	else{//input
		switch(gpio)
		{
			case 0:ret = axp_read(&axp->dev,AXP22_GPIO01_STATE,&val);val &= 0x10;*value = val>>4;break;
			case 1:ret = axp_read(&axp->dev,AXP22_GPIO01_STATE,&val);val &= 0x20;*value = val>>5;break;
			case 2:printk("This IO is not an input,no return value!");return -ENXIO;
			case 3:printk("This IO is not an input,no return value!");return -ENXIO;
			case 4:printk("This IO is not an input,no return value!");return -ENXIO;
			default:return -ENXIO;
		}
	}
	return ret;
}
EXPORT_SYMBOL_GPL(axp_gpio_get_value);

static int __axp_gpio_input(struct gpio_chip *chip, unsigned offset)
{
	u32  index = chip->base + offset;
//	printk("%s: line %d,%d,\n", __func__, __LINE__,offset);

	if(GPIO_AXP(0) == index)
		return axp_gpio_set_io(0, 0);
	else if(GPIO_AXP(1) == index)
		return axp_gpio_set_io(1, 0);
	else if(GPIO_AXP(2) == index || GPIO_AXP(3) == index || GPIO_AXP(4) == index)
		{
			printk("This IO can not config as an input IO!");
			return -EINVAL;
		}
	else
		return -EINVAL;
}
static int __axp_gpio_output(struct gpio_chip *chip, unsigned offset, int value)
{
	u32  index = chip->base + (int)offset;
	int  ret = 0;
	int  id = index - GPIO_AXP(0);
	
//	printk("%s: line %d,%d,%d\n", __func__, __LINE__,offset,value);
	switch(id) {
		case 0: 
			ret = axp_gpio_set_io(0, 1); /* set to output */
			if(ret)
				return ret;
			return axp_gpio_set_value(0, value); /* set value */
		case 1:
			ret = axp_gpio_set_io(1, 1); /* set to output */
			if(ret)
				return ret;
			return axp_gpio_set_value(1, value); /* set value */
		case 2:
			ret = axp_gpio_set_io(2, 1); /* set to output */
			if(ret)
				return ret;
			return axp_gpio_set_value(2, value); /* set value */
		case 3:
			ret = axp_gpio_set_io(3, 1); /* set to output */
			if(ret) 
				return ret;
			return axp_gpio_set_value(3, value); /* set value */
		case 4:
			ret = axp_gpio_set_io(4, 1); /* set to output */
			if(ret)
				return ret;
			return axp_gpio_set_value(4, value); /* set value */
		default: return -EINVAL;
	}
}
static void __axp_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	u32  index = chip->base + offset;
	int id = index - GPIO_AXP(0);
//	printk("%s: line %d,%d,%d\n", __func__, __LINE__,offset,value);

	switch (id) {
		case 0:axp_gpio_set_value(0, value);break;
		case 1:axp_gpio_set_value(1, value);break;
		case 2:axp_gpio_set_value(2, value);break;
		case 3:axp_gpio_set_value(3, value);break;
		case 4:axp_gpio_set_value(4, value);break;
		default:WARN_ON(1);	
	}
}
static int __axp_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	u32  index = chip->base + offset;
	int  value = 0;
//	printk("%s: line %d,%d,\n", __func__, __LINE__,offset);

	if(GPIO_AXP(0) == index) {
		WARN_ON(0 != axp_gpio_get_value(0, &value));
		return value;
	} else if(GPIO_AXP(1) == index) {
		WARN_ON(0 != axp_gpio_get_value(1, &value));
		return value;
	} else if(GPIO_AXP(2) == index) {
		WARN_ON(0 != axp_gpio_get_value(2, &value));
		return value;
	} else if(GPIO_AXP(3) == index) {
		WARN_ON(0 != axp_gpio_get_value(3, &value));
		return value;
	} else if(GPIO_AXP(4) == index) {
		WARN_ON(0 != axp_gpio_get_value(4, &value));
		return value;
	}	else {
		printk("%s err: line %d\n", __func__, __LINE__);;
		return 0;
	}
}

struct gpio_chip axp_gpio_chip = {
	.base	= AXP_NR_BASE,
	.ngpio	= AXP_NR,
	.label	= "axp_pin",
	.direction_input = __axp_gpio_input,
	.direction_output = __axp_gpio_output,
	.set	= __axp_gpio_set,
	.get	= __axp_gpio_get,
};

static ssize_t show_gpio(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct virtual_gpio_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->gpio);
}

static ssize_t set_gpio(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct virtual_gpio_data *data = dev_get_drvdata(dev);
	long val;

	if (strict_strtol(buf, 10, &val) != 0)
		return count;

	data->gpio = val;

	return count;
}

static ssize_t show_io(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct virtual_gpio_data *data = dev_get_drvdata(dev);
	int ret;
	mutex_lock(&data->lock);

	ret = axp_gpio_get_io(data->gpio,&data->io);

	mutex_unlock(&data->lock);

	if(ret)
		return ret;

	return sprintf(buf, "%d\n", data->io);
}

static ssize_t set_io(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct virtual_gpio_data *data = dev_get_drvdata(dev);
	long val;
	int ret;

	if (strict_strtol(buf, 10, &val) != 0)
		return count;

	mutex_lock(&data->lock);

	data->io = val;
	ret = axp_gpio_set_io(data->gpio,data->io);

	mutex_unlock(&data->lock);
	if(ret)
		return ret;
	return count;
}


static ssize_t show_value(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct virtual_gpio_data *data = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&data->lock);

	ret = axp_gpio_get_value(data->gpio,&data->value);

	mutex_unlock(&data->lock);

	if(ret)
		return ret;

	return sprintf(buf, "%d\n", data->value);
}

static ssize_t set_value(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct virtual_gpio_data *data = dev_get_drvdata(dev);
	long val;
	int ret;

	if (strict_strtol(buf, 10, &val) != 0)
		return count;

	mutex_lock(&data->lock);

	data->value = val;
	ret = axp_gpio_set_value(data->gpio,data->value);

	mutex_unlock(&data->lock);

	if(ret){
		return ret;
	}

	return count;
}


static DEVICE_ATTR(gpio,0664, show_gpio, set_gpio);
static DEVICE_ATTR(io, 0664, show_io, set_io);
static DEVICE_ATTR(value, 0664, show_value, set_value);

struct device_attribute *attributes[] = {
	&dev_attr_gpio,
	&dev_attr_io,
	&dev_attr_value,
};


static int __devinit axp_gpio_probe(struct platform_device *pdev)
{
	//struct axp_mfd_chip *axp_chip = dev_get_drvdata(pdev->dev.parent);
	struct virtual_gpio_data *drvdata;
	int ret, i;

	drvdata = kzalloc(sizeof(struct virtual_gpio_data), GFP_KERNEL);
	if (drvdata == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	mutex_init(&drvdata->lock);

	for (i = 0; i < ARRAY_SIZE(attributes); i++) {
		ret = device_create_file(&pdev->dev, attributes[i]);
		if (ret != 0)
			goto err;
	}

	platform_set_drvdata(pdev, drvdata);

	return 0;

err:
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(&pdev->dev, attributes[i]);
	kfree(drvdata);
	return ret;

return 0;
}

static int __devexit axp_gpio_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver axp_gpio_driver = {
	.driver.name	= "axp22-gpio",
	.driver.owner	= THIS_MODULE,
	.probe		= axp_gpio_probe,
	.remove		= __devexit_p(axp_gpio_remove),
};

static int __init axp_gpio_init(void)
{
	/* register axp gpio chip */
	if(0 != gpiochip_add(&axp_gpio_chip))
		printk("%s err, line %d\n", __func__, __LINE__);
	return platform_driver_register(&axp_gpio_driver);
}
subsys_initcall(axp_gpio_init);

static void __exit axp_gpio_exit(void)
{
	platform_driver_unregister(&axp_gpio_driver);
}
module_exit(axp_gpio_exit);

MODULE_AUTHOR("Weijin Zhong ");
MODULE_DESCRIPTION("GPIO interface for AXP PMICs");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:axp-gpio");
