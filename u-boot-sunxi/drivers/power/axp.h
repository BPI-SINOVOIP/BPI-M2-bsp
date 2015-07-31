/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef   __AXP_H__
#define   __AXP_H__

#include <common.h>

#if defined(CONFIG_SUNXI_I2C)
	#include <i2c.h>
#elif defined(CONFIG_SUNXI_P2WI)
	#include <p2wi.h>
#elif defined(CONFIG_SUNXI_RSB)
	#include <rsb.h>
#endif

#include <sys_config.h>

#define  AXP_POWER_ON_BY_POWER_KEY       0
#define  AXP_POWER_ON_BY_POWER_TRIGGER   1

#define  SUNXI_AXP_DEV_MAX               (4)

#define  SUNXI_AXP_20X                   20
#define  SUNXI_AXP_22X                   22
#define  SUNXI_AXP_15X                   15


#define  PMU_TYPE_22X                    1
#define  PMU_TYPE_15X                    2
#define  PMU_TYPE_20X                    3



typedef struct
{
	const char *pmu_name;

	int (* set_supply_status)  (int vol_name, int vol_value, int onoff);	//�������״̬����ѹ�Ϳ���
	int (* set_supply_status_byname)(char *vol_type, int vol_value, int onoff);	//�������״̬����ѹ�Ϳ���
	int (* probe_supply_status)(int vol_name, int vol_value, int onoff);    //��ȡ���״̬����ѹ�Ϳ���

	int (* set_next_sys_mode)(int status);
	int (* probe_pre_sys_mode)(void);
	int (* probe_this_poweron_cause)(void);

	int (* probe_power_status)(void);

	int (* probe_battery_vol)(void);
	int (* probe_battery_ratio)(void);
	int (* probe_battery_exist)(void);

	int (* probe_key)(void);

	int (* set_power_off)(void);
	int (* set_power_onoff_vol)(int vol_value, int stage);

	int (* set_charge_control)(void);
	int (* set_vbus_cur_limit)(int current);
	int (* set_vbus_vol_limit)(int vol_value);
	int (* set_charge_current)(int current);
	int (* probe_charge_current)(void);

	int (* probe_int_pending)(uchar *buffer);
	int (* probe_int_enable  )(uchar *buffer);
	int (* set_int_enable)(uchar *buffer);
}
sunxi_axp_dev_t;


#define  __sunxi_axp_module_init(type, name)						\
			sunxi_axp_dev_t sunxi_axp_##name =				\
			{												\
				type,										\
				axp##name##_set_supply_status,				\
				axp##name##_set_supply_status_byname,		\
				axp##name##_probe_supply_status,			\
															\
				axp##name##_set_next_sys_mode,				\
				axp##name##_probe_pre_sys_mode,				\
				axp##name##_probe_this_poweron_cause,		\
															\
				axp##name##_probe_power_status,				\
															\
				axp##name##_probe_battery_vol,				\
				axp##name##_probe_battery_ratio,			\
				axp##name##_probe_battery_exist,			\
															\
				axp##name##_probe_key,						\
															\
				axp##name##_set_power_off,					\
				axp##name##_set_power_onoff_vol,			\
															\
				axp##name##_set_charge_control,				\
				axp##name##_set_vbus_cur_limit,				\
				axp##name##_set_vbus_vol_limit,				\
				axp##name##_set_charge_current,				\
				axp##name##_probe_charge_current,			\
															\
				axp##name##_probe_int_pending,				\
				axp##name##_probe_int_enable,				\
				axp##name##_set_int_enable					\
			}

#define  sunxi_axp_module_init(type, name)  __sunxi_axp_module_init(type, name)


#define  __sunxi_axp_module_ext(name)						\
			extern sunxi_axp_dev_t sunxi_axp_##name

#define  sunxi_axp_module_ext(name)							\
			__sunxi_axp_module_ext(name)


#if defined(CONFIG_SUNXI_AXP22)
	sunxi_axp_module_ext(SUNXI_AXP_22X);
#endif

#if defined(CONFIG_SUNXI_AXP20)
	sunxi_axp_module_ext(SUNXI_AXP_20X);
#endif

static inline int axp_i2c_read(unsigned char chip, unsigned char addr, unsigned char *buffer)
{
#if defined(CONFIG_SUNXI_I2C)
	return i2c_read(chip, addr, 1, buffer, 1);
#elif defined(CONFIG_SUNXI_P2WI)
	return p2wi_read(&addr, buffer, 1);
#elif defined(CONFIG_SUNXI_RSB)
	return sunxi_rsb_read(0, addr, buffer, 1);
#else
	return -1;
#endif
}

static inline int axp_i2c_write(unsigned char chip, unsigned char addr, unsigned char data)
{
#if defined(CONFIG_SUNXI_I2C)
	return i2c_write(chip, addr, 1, &data, 1);
#elif defined(CONFIG_SUNXI_P2WI)
	return p2wi_write(&addr, &data, 1);
#elif defined(CONFIG_SUNXI_RSB)
	return sunxi_rsb_write(0, addr, &data, 1);
#else
	return -1;
#endif
}

static inline int abs(int x)
{
	return x>0?x:(-x);
}



#endif /* __AXP_H__ */
