/* extended_standby.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *
 *  By      : liming
 *  Version : v1.0
 *  Date    : 2013-4-17 09:08
 */
#include <linux/module.h>
#include <linux/power/aw_pm.h>
#include <mach/system.h>
#include <mach/gpio.h>
#include <linux/scenelock.h>

#define AW_EXSTANDBY_DBG   1
#undef EXSTANDBY_DBG
#if(AW_EXSTANDBY_DBG)
#define EXSTANDBY_DBG(format,args...)   printk("[exstandby]"format,##args)
#else
#define EXSTANDBY_DBG(format,args...)   do{}while(0)
#endif

static DEFINE_SPINLOCK(data_lock);

static extended_standby_t talk_standby = {
	.id           	= TALKING_STANDBY_FLAGS,
	.pwr_dm_en      = 0xf3,      //mean gpu, cpu is powered off.
	.osc_en         = 0xf,
	.init_pll_dis   = (~(0x12)), //mean pll2 is on. pll5 is shundowned by dram driver.
	.exit_pll_en    = 0x21,      //mean enable pll1 and pll6
	.pll_change     = 0x0,
	//.pll_factor[5]  = {0x18,0x1,0x1,0},
	.bus_change     = 0x0,
	//.bus_factor[2]  = {0x8,0,0x3,0,0},
};

static extended_standby_t usb_standby = {
	.id  		= USB_STANDBY_FLAGS,
	.pwr_dm_en      = 0xf3,      //mean gpu, cpu is powered off.
	.osc_en         = 0xf ,      //mean all osc is powered on.
	.init_pll_dis   = (~(0x30)), //mean pll6 is on.pll5 is shundowned by dram driver.
	.exit_pll_en    = 0x1,       //mean enable pll1 and pll6
	.pll_change     = 0x20,
	.pll_factor[5]  = {0x18,0x1,0x1,0},
	.bus_change     = 0x4,
	.bus_factor[2]  = {0x8,0,0x3,0,0},
};

static extended_standby_t temp_standby_data = {
	.id = 0,
};

extended_standby_manager_t extended_standby_manager = {
	.pextended_standby = NULL,
	.event = 0,
	.wakeup_gpio_map = 0,
	.wakeup_gpio_group = 0,
};

int copy_extended_standby_data(extended_standby_t *standby_data)
{
	int i = 0;

	if ((0 != temp_standby_data.id) && (!((standby_data->id) & (temp_standby_data.id)))) {
		temp_standby_data.id |= standby_data->id;
		temp_standby_data.pwr_dm_en |= standby_data->pwr_dm_en;
		temp_standby_data.osc_en |= standby_data->osc_en;
		temp_standby_data.init_pll_dis &= standby_data->init_pll_dis;
		temp_standby_data.exit_pll_en |= standby_data->exit_pll_en;
		if (0 != standby_data->pll_change) {
			temp_standby_data.pll_change |= standby_data->pll_change;
			for (i=0; i<4; i++) {
				temp_standby_data.pll_factor[i] = standby_data->pll_factor[i];
			}
		}
		if (0 != standby_data->bus_change) {
			temp_standby_data.bus_change |= standby_data->bus_change;
			for (i=0; i<4; i++) {
				temp_standby_data.bus_factor[i] = standby_data->bus_factor[i];
			}
		}
	} else if ((0 == temp_standby_data.id)) {
		temp_standby_data.id = standby_data->id;
		temp_standby_data.pwr_dm_en = standby_data->pwr_dm_en;
		temp_standby_data.osc_en = standby_data->osc_en;
		temp_standby_data.init_pll_dis = standby_data->init_pll_dis;
		temp_standby_data.exit_pll_en = standby_data->exit_pll_en;
		temp_standby_data.pll_change = standby_data->pll_change;
		if (0 != standby_data->pll_change) {
			for (i=0; i<4; i++) {
				temp_standby_data.pll_factor[i] = standby_data->pll_factor[i];
			}
		} else
			memset(&temp_standby_data.pll_factor, 0, sizeof(temp_standby_data.pll_factor));

		temp_standby_data.bus_change = standby_data->bus_change;
		if (0 != standby_data->bus_change) {
			for (i=0; i<4; i++) {
				temp_standby_data.bus_factor[i] = standby_data->bus_factor[i];
			}
		} else
			memset(&temp_standby_data.bus_factor, 0, sizeof(temp_standby_data.bus_factor));

	}
	return 0;
}
/**
 *	get_extended_standby_manager - get the extended_standby_manager pointer
 *
 *	Return	: if the extended_standby_manager is effective, return the extended_standby_manager pointer;
 *		  else return NULL;
 *	Notes	: you can check the configuration from the pointer.
 */
const extended_standby_manager_t *get_extended_standby_manager(void)
{
	unsigned long irqflags;
	extended_standby_manager_t *manager_data = NULL;
	spin_lock_irqsave(&data_lock, irqflags);
	manager_data = &extended_standby_manager;
	spin_unlock_irqrestore(&data_lock, irqflags);
	if ((NULL != manager_data) && (NULL != manager_data->pextended_standby))
		EXSTANDBY_DBG("leave %s : id 0x%lx\n", __func__, manager_data->pextended_standby->id);

	return manager_data;
}

/**
 *	set_extended_standby_manager - set the extended_standby_manager;
 *	manager@: the manager config.
 *
 *      return value: if the setting is correct, return true.
 *		      else return false;
 *      notes: the function will check the struct member: pextended_standby and event.
 *		if the setting is not proper, return false.
 */
bool set_extended_standby_manager(unsigned long flags)
{
	unsigned long irqflags;

	EXSTANDBY_DBG("enter %s\n", __func__);

	if (USB_STANDBY_FLAGS & flags) {
		spin_lock_irqsave(&data_lock, irqflags);

		copy_extended_standby_data(&usb_standby);
		extended_standby_manager.pextended_standby = &temp_standby_data;

		spin_unlock_irqrestore(&data_lock, irqflags);
	}

	if (TALKING_STANDBY_FLAGS & flags) {
		spin_lock_irqsave(&data_lock, irqflags);

		copy_extended_standby_data(&talk_standby);
		extended_standby_manager.pextended_standby = &temp_standby_data;

		spin_unlock_irqrestore(&data_lock, irqflags);
	}

	if (MP3_STANDBY_FLAGE & flags) {
	}

	if (0 == flags) {
		spin_lock_irqsave(&data_lock, irqflags);
		temp_standby_data.id = 0;
		extended_standby_manager.pextended_standby = NULL;
		spin_unlock_irqrestore(&data_lock, irqflags);
		return true;
	}
	if (NULL != extended_standby_manager.pextended_standby)
		EXSTANDBY_DBG("leave %s : id 0x%lx\n", __func__, extended_standby_manager.pextended_standby->id);
	return true;
}

/**
 *	extended_standby_enable_wakeup_src   - 	enable the wakeup src.
 *
 *	function:		the device driver care about the wakeup src.
 *				if the device driver do want the system be wakenup while in standby state.
 *				the device driver should use this function to enable corresponding intterupt.
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src, 
 *				else ignored.
 *	notice:			1. for gpio intterupt, only access the enable bit, mean u need care about other config,
 *				such as: int mode, pull up or pull down resistance, etc.
 *				2. At a31, only gpio��pa, pb, pe, pg, pl, pm��int wakeup src is supported. 
*/
int extended_standby_enable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;
	spin_lock_irqsave(&data_lock, irqflags);
	extended_standby_manager.event |= src;
	if (CPUS_GPIO_SRC & src) {
		if ( para > GPIO_INDEX_END) {
			pr_info("gpio config err. \n");
		} else if ( para >= AXP_NR_BASE) {
			extended_standby_manager.wakeup_gpio_map |= (WAKEUP_GPIO_AXP((para - AXP_NR_BASE)));
		} else if ( para >= PM_NR_BASE) {
			extended_standby_manager.wakeup_gpio_map |= (WAKEUP_GPIO_PM((para - PM_NR_BASE)));
		} else if ( para >= PL_NR_BASE) {
			extended_standby_manager.wakeup_gpio_map |= (WAKEUP_GPIO_PL((para - PL_NR_BASE)));
		} else if ( para >= PH_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('H'));
		} else if ( para >= PG_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('G'));
		} else if ( para >= PF_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('F'));
		} else if ( para >= PE_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('E'));
		} else if ( para >= PD_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('D'));
		} else if ( para >= PC_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('C'));
		} else if ( para >= PB_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('B'));
		} else if ( para >= PA_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('A'));
		} else {
			pr_info("cpux need care gpio %d. but, notice, currently, \
				cpux not support it.\n", para);
		}
	}
	spin_unlock_irqrestore(&data_lock, irqflags);
	EXSTANDBY_DBG("leave %s : event 0x%lx\n", __func__, extended_standby_manager.event);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_map 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_map);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_group 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_group);
	return 0;
}

/**
 *	extended_standby_disable_wakeup_src  - 	disable the wakeup src.
 *
 *	function:		if the device driver do not want the system be wakenup while in standby state again.
 *				the device driver should use this function to disable the corresponding intterupt.
 *
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src, 
 *				else ignored.
 *	notice:			for gpio intterupt, only access the enable bit, mean u need care about other config,
 *				such as: int mode, pull up or pull down resistance, etc.
 */
int extended_standby_disable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;
	spin_lock_irqsave(&data_lock, irqflags);
	extended_standby_manager.event &= (~src);
	if (CPUS_GPIO_SRC & src) {
		if ( para > GPIO_INDEX_END){
			pr_info("gpio config err. \n");
		}else if ( para >= AXP_NR_BASE) {
			extended_standby_manager.wakeup_gpio_map &= (~(WAKEUP_GPIO_AXP((para - AXP_NR_BASE))));
		}else if ( para >= PM_NR_BASE) {
			extended_standby_manager.wakeup_gpio_map &= (~(WAKEUP_GPIO_PM((para - PM_NR_BASE))));
		}else if ( para >= PL_NR_BASE) {
			extended_standby_manager.wakeup_gpio_map &= (~(WAKEUP_GPIO_PL((para - PL_NR_BASE))));
		}else if ( para >= PH_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('H')));
		}else if ( para >= PG_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('G')));
		}else if ( para >= PF_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('F')));
		}else if ( para >= PE_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('E')));
		}else if ( para >= PD_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('D')));
		}else if ( para >= PC_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('C')));
		}else if ( para >= PA_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('B')));
		}else if ( para >= PB_NR_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('A')));
		}else {
			pr_info("cpux need care gpio %d. but, notice, currently, \
				cpux not support it.\n", para);
		}
	}
	spin_unlock_irqrestore(&data_lock, irqflags);
	EXSTANDBY_DBG("leave %s : event 0x%lx\n", __func__, extended_standby_manager.event);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_map 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_map);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_group 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_group);
	return 0;
}

/**
 *	extended_standby_check_wakeup_state   -   to get the corresponding wakeup src intterupt state, enable or disable.
 *
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src, 
 *				else ignored.
 *
 *	return value:		enable, 	return 1,
 *				disable,	return 2,
 *				error: 		return -1.
 */
int extended_standby_check_wakeup_state(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;
	int ret = -1;
	spin_lock_irqsave(&data_lock, irqflags);
	if (extended_standby_manager.event & src)
		ret = 1;
	else
		ret = 2;
	spin_unlock_irqrestore(&data_lock, irqflags);

	return ret;
}

/**
 *	extended_standby_show_state  - 	show current standby state, for debug purpose.
 *
 *	function:		standby state including locked_scene, power_supply dependancy, the wakeup src.
 *
 *	return value:		succeed, return 0, else return -1.
 */
int extended_standby_show_state(void)
{
	unsigned long irqflags;

	printk("scence_lock: ");
	if (!check_scene_locked(SCENE_USB_STANDBY))
		printk("usb_standby ");
	if (!check_scene_locked(SCENE_TALKING_STANDBY))
		printk("talking_standby ");
	if (!check_scene_locked(SCENE_MP3_STANDBY))
		printk("mp3_standby");
	printk("\n");

	spin_lock_irqsave(&data_lock, irqflags);
	printk("wakeup_src 0x%lx\n", extended_standby_manager.event);
	printk("wakeup_gpio_map 0x%lx\n", extended_standby_manager.wakeup_gpio_map);
	printk("wakeup_gpio_group 0x%lx\n", extended_standby_manager.wakeup_gpio_group);
	if (NULL != extended_standby_manager.pextended_standby)
		printk("extended_standby id = 0x%lx\n", extended_standby_manager.pextended_standby->id);
	spin_unlock_irqrestore(&data_lock, irqflags);

	return 0;
}

