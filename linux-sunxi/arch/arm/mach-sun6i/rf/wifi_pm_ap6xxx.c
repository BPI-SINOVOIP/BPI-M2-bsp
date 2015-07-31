/*
 * ap6xxx sdio wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include <asm/io.h>
#include "wifi_pm.h"
#include <mach/platform.h>
#include <linux/regulator/consumer.h>

#define ap6xxx_msg(...)    do {printk("[ap6xxx]: "__VA_ARGS__);} while(0)

static int ap6xxx_wl_regon = 0;
static int ap6xxx_bt_regon = 0;
static char * axp_name = NULL;
#define CLK_OUTD_REG 0xf0

// power for gpio group pg used for sdio and uart
static void ap6xxx_pg_power(void) {
    struct regulator *ldo = NULL;
    script_item_u val;
    script_item_value_type_e type;

    type = script_get_item(wifi_para, "ap6xxx_gpio_power", &val);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        int ret = 0;
        ap6xxx_msg("ap6xxx_gpio_power:%s !\n", val.str);
        ldo = regulator_get(NULL, val.str);
        if (ldo) {
            regulator_set_voltage(ldo, 2800000, 2800000);
            ret = regulator_enable(ldo);
            if(ret < 0){
                ap6xxx_msg("regulator_enable failed.\n");
            }
            regulator_put(ldo);
        } else {
            ap6xxx_msg("get power regulator failed.\n");
        }
    } else {
        ap6xxx_msg("ERR: get ap6xxx_gpio_power failed");
    }
}

// set host 32k clk output
static void ap6xxx_clk_config(void) {
    unsigned int reg_addr, reg_val;
    unsigned gpio;
    struct regulator *ldo = NULL;
    script_item_u val;
    script_item_value_type_e type;
    int ret = 0;

    type = script_get_item(wifi_para, "ap6xxx_clk_power", &val);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        ap6xxx_msg("ap6xxx_clk_power:%s !\n", val.str);
        ldo = regulator_get(NULL, val.str);
        if (ldo) {
            regulator_set_voltage(ldo, 2800000, 2800000);
            ret = regulator_enable(ldo);
            regulator_put(ldo);
            if(ret < 0){
                ap6xxx_msg("regulator_enable failed.\n");
                return;
            }
        } else {
            ap6xxx_msg("get power regulator failed.\n");
            return;
        }
    } else {
        ap6xxx_msg("ERR: get ap6xxx_clk_power failed\n");
        return;
    }

    // get gpio PM07
    gpio = GPIOM(7);
    ret = gpio_request(gpio, NULL);
    if (ret) {
        ap6xxx_msg("failed to request gpio PM07!\n");
        return;
    }
    sw_gpio_setpull(gpio, 1);  
    sw_gpio_setdrvlevel(gpio, 3);
    // PM07_SELECT   011:RTC_CLKO
    sw_gpio_setcfg(gpio, 0x03);
    gpio_free(gpio);

    //enable clk
    reg_addr = AW_VIR_R_PRCM_BASE + CLK_OUTD_REG;
    reg_val = readl(reg_addr);
    // Clock outputD enable
    writel( reg_val | (1<<31), reg_addr);
    ap6xxx_msg("ap6xxx ap6xxx_clk_config.\n");
}

// power control by axp
static int ap6xxx_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	ap6xxx_msg("ap6xxx module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, axp_name);
	if (!wifi_ldo) {
		ap6xxx_msg("get power regulator failed.\n");
		return -ret;
	}

	if (first) {
		ap6xxx_msg("first time\n");
		ret = regulator_force_disable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_force_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
		first = 0;
	}

	if (onoff) {
		ap6xxx_msg("regulator on.\n");
		ret = regulator_set_voltage(wifi_ldo, 3300000, 3300000);
		if (ret < 0) {
			ap6xxx_msg("regulator_set_voltage fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}

		ret = regulator_enable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_enable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	} else {
		ap6xxx_msg("regulator off.\n");
		ret = regulator_disable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	}
	regulator_put(wifi_ldo);
	return ret;
}

static int ap6xxx_gpio_ctrl(char* name, int level)
{
	int i = 0;
	int ret = 0;
	int gpio = 0;
	unsigned long flags = 0;
	char * gpio_name[2] = {"ap6xxx_wl_regon", "ap6xxx_bt_regon"};

	for (i = 0; i < 2; i++) {
		if (strcmp(name, gpio_name[i]) == 0) {
			switch (i)
			{
				case 0: /*ap6xxx_wl_regon*/
					gpio = ap6xxx_wl_regon;
					break;
				case 1: /*ap6xxx_bt_regon*/
					gpio = ap6xxx_bt_regon;
					break;
				default:
					ap6xxx_msg("no matched gpio.\n");
			}
			break;
		}
	}

	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		ap6xxx_msg("failed to set gpio %s to %d !\n", name, level);
		return -1;
	} else {
		gpio_free(gpio);
		ap6xxx_msg("succeed to set gpio %s to %d !\n", name, level);
	}

	return 0;
}

void ap6xxx_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			ap6xxx_gpio_ctrl("ap6xxx_wl_regon", 1);
			mdelay(200);
        } else {
			ap6xxx_gpio_ctrl("ap6xxx_wl_regon", 0);
			mdelay(100);
        }
        ap6xxx_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    }
    return;
}

void ap6xxx_gpio_init(void)
{
	script_item_u val;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		ap6xxx_msg("failed to fetch wifi_power\n");
		return;
	}

	axp_name = val.str;
	ap6xxx_msg("module power name %s\n", axp_name);

	type = script_get_item(wifi_para, "ap6xxx_wl_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		ap6xxx_msg("get ap6xxx ap6xxx_wl_regon gpio failed\n");
	else
		ap6xxx_wl_regon = val.gpio.gpio;

	type = script_get_item(wifi_para, "ap6xxx_bt_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		ap6xxx_msg("get ap6xxx ap6xxx_bt_regon gpio failed\n");
	else
		ap6xxx_bt_regon = val.gpio.gpio;

	ap6xxx_pg_power();
	ap6xxx_clk_config();
	ops->gpio_ctrl	= ap6xxx_gpio_ctrl;
	ops->power = ap6xxx_power;

	ap6xxx_module_power(1);
}
