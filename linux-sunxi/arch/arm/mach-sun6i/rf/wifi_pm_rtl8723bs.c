/*
 * rtl8723bs sdio wifi power management API
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

#define rtl8723bs_msg(...)    do {printk("[rtl8723bs]: "__VA_ARGS__);} while(0)

static int rtl8723bs_chip_en  = 0;
static int rtl8723bs_wl_regon = 0;
static int rtl8723bs_bt_regon = 0;
static char * axp_name[4] = {NULL};
#define CLK_OUTD_REG 0xf0

// power for gpio group pg used for sdio and uart
static void rtl8723bs_pg_power(void) {
    struct regulator *ldo = NULL;
    script_item_u val;
    script_item_value_type_e type;

    type = script_get_item(wifi_para, "rtl8723bs_gpio_power", &val);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        int ret = 0;
        rtl8723bs_msg("rtl8723bs_gpio_power:%s !\n", val.str);
        ldo = regulator_get(NULL, val.str);
        if (ldo) {
            regulator_set_voltage(ldo, 2800000, 2800000);
            ret = regulator_enable(ldo);
            if(ret < 0){
                rtl8723bs_msg("regulator_enable failed.\n");
            }
            regulator_put(ldo);
        } else {
            rtl8723bs_msg("get power regulator failed.\n");
        }
    } else {
        rtl8723bs_msg("ERR: get rtl8723bs_gpio_power failed");
    }
}

// set host 32k clk output
static void rtl8723bs_clk_config(void) {
    unsigned int reg_addr, reg_val;
    unsigned gpio;
    struct regulator *ldo = NULL;
    script_item_u val;
    script_item_value_type_e type;
    int ret = 0;

    type = script_get_item(wifi_para, "rtl8723bs_clk_power", &val);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        rtl8723bs_msg("rtl8723bs_clk_power:%s !\n", val.str);
        ldo = regulator_get(NULL, val.str);
        if (ldo) {
            regulator_set_voltage(ldo, 2800000, 2800000);
            ret = regulator_enable(ldo);
            regulator_put(ldo);
            if(ret < 0){
                rtl8723bs_msg("regulator_enable failed.\n");
                return;
            }
        } else {
            rtl8723bs_msg("get power regulator failed.\n");
            return;
        }
    } else {
        rtl8723bs_msg("ERR: get rtl8723bs_clk_power failed\n");
        return;
    }

    // get gpio PM07
    gpio = GPIOM(7);
    ret = gpio_request(gpio, NULL);
    if (ret) {
        rtl8723bs_msg("failed to request gpio PM07!\n");
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
    rtl8723bs_msg("rtl8723bs rtl8723bs_clk_config.\n");
}

// power control by axp
static int rtl8723bs_module_power(int onoff)
{
	struct regulator* wifi_ldo[4] = {NULL};
	static int first = 1;
	int i = 0, ret = 0;

	rtl8723bs_msg("rtl8723bs module power set by axp.\n");
	for (i = 0; axp_name[i] != NULL; i++){
	    rtl8723bs_msg("get power regulator----%d.\n",i);
    	wifi_ldo[i] = regulator_get(NULL, axp_name[i]);
    	if (!wifi_ldo[i]) {
    		rtl8723bs_msg("get power regulator failed----%d.\n",i);
    		goto exit;
    	}
 }
exit:
  wifi_ldo[i] = NULL;
  
	if (first) {
		rtl8723bs_msg("first time\n");
		for (i = 0; wifi_ldo[i] != NULL; i++){		
    		ret = regulator_force_disable(wifi_ldo[i]);
    		if (ret < 0) {
    			rtl8723bs_msg("regulator_force_disable fail, return %d.\n", ret);
    			regulator_put(wifi_ldo[i]);
    			return ret;
    		}
  }
  first = 0;
	}

	if (onoff) {
		rtl8723bs_msg("regulator on.\n");
		for(i = 0; wifi_ldo[i] != NULL; i++){		
    		ret = regulator_set_voltage(wifi_ldo[i], 3300000, 3300000);
    		if (ret < 0) {
    			rtl8723bs_msg("regulator_set_voltage fail, return %d.\n", ret);
    			regulator_put(wifi_ldo[i]);
    			return ret;
    		}
    
    		ret = regulator_enable(wifi_ldo[i]);
    		if (ret < 0) {
    			rtl8723bs_msg("regulator_enable fail, return %d.\n", ret);
    			regulator_put(wifi_ldo[i]);
    			return ret;
    		}
	 }
	} else {
		rtl8723bs_msg("regulator off.\n");
		for(i = 0; wifi_ldo[i] != NULL; i++){
    		ret = regulator_disable(wifi_ldo[i]);
    		if (ret < 0) {
    			rtl8723bs_msg("regulator_disable fail, return %d.\n", ret);
    			regulator_put(wifi_ldo[i]);
    			return ret;
    		}
	 }
 }
 
	for(i = 0; wifi_ldo[i] != NULL; i++){
	    regulator_put(wifi_ldo[i]);
	}
	return ret;
}

static int rtl8723bs_gpio_ctrl(char* name, int level)
{
	int i = 0;
	int ret = 0;
	int gpio = 0;
	unsigned long flags = 0;
	char * gpio_name[3] = {"rtl8723bs_wl_regon", "rtl8723bs_bt_regon","rtl8723bs_chip_en"};

	for (i = 0; i < 2; i++) {
		if (strcmp(name, gpio_name[i]) == 0) {
			switch (i)
			{
				case 0: /*rtl8723bs_wl_regon*/
					gpio = rtl8723bs_wl_regon;
					break;
				case 1: /*rtl8723bs_bt_regon*/
					gpio = rtl8723bs_bt_regon;
					break;
				case 2: /*rtl8723bs_chip_en*/
				 gpio = rtl8723bs_chip_en;
				 break;
				default:
					rtl8723bs_msg("no matched gpio.\n");
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
		rtl8723bs_msg("failed to set gpio %s to %d !\n", name, level);
		return -1;
	} else {
		gpio_free(gpio);
		rtl8723bs_msg("succeed to set gpio %s to %d !\n", name, level);
	}

	return 0;
}

void rtl8723bs_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			rtl8723bs_gpio_ctrl("rtl8723bs_wl_regon", 1);
			mdelay(200);
        } else {
			rtl8723bs_gpio_ctrl("rtl8723bs_wl_regon", 0);
			mdelay(100);
        }
        rtl8723bs_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    }
    return;
}

void rtl8723bs_gpio_init(void)
{
	script_item_u val;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		rtl8723bs_msg("failed to fetch wifi_power\n");
		return;
	}

	axp_name[0] = val.str;
	rtl8723bs_msg("module power name %s\n", axp_name[0]);

  type = script_get_item(wifi_para, "rtl8723bs_power_ext1", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		rtl8723bs_msg("failed to fetch rtl8723bs_power_ext1\n");
		axp_name[1] = NULL;
	} 
	else {
  axp_name[1] = val.str;
  rtl8723bs_msg("module power ext1 name %s\n", axp_name[1]);
 }
 
  type = script_get_item(wifi_para, "rtl8723bs_power_ext2", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		rtl8723bs_msg("failed to fetch rtl8723bs_power_ext2\n");
		axp_name[2] = NULL;
	}
	else {
  axp_name[2] = val.str;
  rtl8723bs_msg("module power ext2 name %s\n", axp_name[2]);
 }
  axp_name[3] = NULL;
  
  type = script_get_item(wifi_para, "rtl8723bs_chip_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		rtl8723bs_msg("It has no rtl8723bs_chip_en gpio\n");
	else
		rtl8723bs_chip_en = val.gpio.gpio;
		
	type = script_get_item(wifi_para, "rtl8723bs_wl_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		rtl8723bs_msg("get rtl8723bs rtl8723bs_wl_regon gpio failed\n");
	else
		rtl8723bs_wl_regon = val.gpio.gpio;

	type = script_get_item(wifi_para, "rtl8723bs_bt_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		rtl8723bs_msg("get rtl8723bs rtl8723bs_bt_regon gpio failed\n");
	else
		rtl8723bs_bt_regon = val.gpio.gpio;

	rtl8723bs_pg_power();
	rtl8723bs_clk_config();
	ops->gpio_ctrl	= rtl8723bs_gpio_ctrl;
	ops->power = rtl8723bs_power;

	rtl8723bs_module_power(1);
	
	if (rtl8723bs_chip_en != 0){
  rtl8723bs_gpio_ctrl("rtl8723bs_chip_en", 1);
 }
}
