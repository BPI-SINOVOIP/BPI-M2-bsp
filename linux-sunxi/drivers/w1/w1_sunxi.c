#include <linux/device.h>
#include <linux/module.h>
#include <linux/w1-gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/sys_config.h>

static int gpiochip = -1;
module_param(gpiochip, int, 0444);
MODULE_PARM_DESC(gpiochip, "w1 gpio lowlevel number");

static struct platform_device *w1_device;

static int __init w1_sunxi_init(void)
{
	int ret = 0;
	struct w1_gpio_platform_data w1_gpio_pdata = {
		.pin = gpiochip,
		.is_open_drain = 0,
	};

	if (!gpio_is_valid(w1_gpio_pdata.pin)) {	

		script_item_u val;
		script_item_value_type_e type;
		script_item_u *list = NULL;
		int idx;
		int cnt;

		if (gpiochip == -1 ) {
			type = script_get_item("w1_para", "w1_gpio", &val);
			if  (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
				cnt = script_get_pio_list("w1_para", &list);

				for (idx = 0; idx < cnt; idx++)
					gpio_request(list[idx].gpio.gpio, NULL);
		
				sw_gpio_setall_range(&list[0].gpio, cnt);

				for (idx = 0; idx < cnt; idx++) {
					gpio_free(list[idx].gpio.gpio);
					gpiochip = list[idx].gpio.gpio;
				}
				ret = 0;
			} else {
				ret = 1;
			}
		}

		printk(KERN_INFO "w1_sunxi_init gpiochip:%d\n", gpiochip);
		
		if (ret || !gpio_is_valid(gpiochip)) {
			pr_err("invalid gpio pin in fex configuration : %d\n",
			       gpiochip);
			return -EINVAL;
		}
		w1_gpio_pdata.pin = gpiochip;
	}

	gpio_free(w1_gpio_pdata.pin);
	w1_device = platform_device_alloc("w1-gpio", 0);

	if (!w1_device)
		return -ENOMEM;

	ret =
	    platform_device_add_data(w1_device, &w1_gpio_pdata,
				     sizeof(struct w1_gpio_platform_data));
	if (ret)
		goto err;

	ret = platform_device_add(w1_device);
	if (ret)
		goto err;

	return 0;

err:
	platform_device_put(w1_device);
	return ret;
}

static void __exit w1_sunxi_exit(void)
{
	platform_device_unregister(w1_device);
}

module_init(w1_sunxi_init);
module_exit(w1_sunxi_exit);

MODULE_DESCRIPTION("GPIO w1 sunxi platform device based on Damien Nicolet <zardam@gmail.com> work.");
MODULE_AUTHOR("Grzegorz Rajtar <mcgregor@blackmesaeast.com.pl>");
MODULE_LICENSE("GPL");
