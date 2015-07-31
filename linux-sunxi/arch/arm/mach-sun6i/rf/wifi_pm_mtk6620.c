/*
 * mtk6620 usb wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include "wifi_pm.h"

#define mtk6620_msg(...)    do {printk("[mtk6620]: "__VA_ARGS__);} while(0)

static int mtk6620_powerup = 0;
static int mtk6620_suspend = 0;
static char * axp_name = NULL;

// power control by axp
static int mtk6620_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	mtk6620_msg("mtk6620 module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, axp_name);
	if (!wifi_ldo) {
		mtk6620_msg("get power regulator failed.\n");
		return -ret;
	}

	if (onoff) {
		mtk6620_msg("regulator on.\n");
		ret = regulator_set_voltage(wifi_ldo, 3000000, 3000000);
		if (ret < 0) {
			mtk6620_msg("regulator_set_voltage fail, return %d.\n", ret);
			goto out;
		}

		ret = regulator_enable(wifi_ldo);
		if (ret < 0) {
			mtk6620_msg("regulator_enable fail, return %d.\n", ret);
			goto out;
		}
	} else {
		mtk6620_msg("regulator off.\n");
		ret = regulator_disable(wifi_ldo);
		if (ret < 0) {
			mtk6620_msg("regulator_disable fail, return %d.\n", ret);
			goto out;
		}
	}
out:
	regulator_put(wifi_ldo);
	wifi_ldo = NULL;
	return ret;
}

void mtk6620_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			mtk6620_module_power(1);
			udelay(50);
			mtk6620_powerup = 1;
        } else {
			mtk6620_module_power(0);
			mtk6620_powerup = 0;
        }
        mtk6620_msg("mtk6620 wifi power state: %s\n", *updown ? "on" : "off");
    } else {
        if (mtk6620_powerup)
            *updown = 1;
        else
            *updown = 0;
		mtk6620_msg("usb wifi power state: %s\n", mtk6620_powerup ? "on" : "off");
    }
    return;	
}

static void mtk6620_standby(int instadby)
{
	if (instadby) {
		if (mtk6620_powerup) {
			/*can't poweroff axp_dldo2 because mtk6620 VRTC pin links it. fix by huzhen2013-1-23*/
			//mtk6620_module_power(0); 
			mtk6620_suspend = 1;
		}
	} else {
		if (mtk6620_suspend) {
			//mtk6620_module_power(1);
			mtk6620_suspend = 0;
		}
	}
	mtk6620_msg("usb wifi : %s\n", instadby ? "suspend" : "resume");
}

void mtk6620_gpio_init(void)
{
	script_item_u val;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	mtk6620_msg("exec mtk6620_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		mtk6620_msg("failed to fetch wifi_power\n");
		return ;
	}

	axp_name = val.str;
	mtk6620_msg("module power name %s\n", axp_name);
	
	mtk6620_powerup = 0;
	mtk6620_suspend = 0;
	ops->power     = mtk6620_power;
	ops->standby   = mtk6620_standby;
}
