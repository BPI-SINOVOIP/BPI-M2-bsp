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
#include <common.h>
#include <power.h>
#if defined(CONFIG_SUNXI_AXP22)
#  include <power/axp22_reg.h>
#endif
#if defined(CONFIG_SUNXI_AXP20)
#  include <power/axp20_reg.h>
#endif
#include "axp.h"
#include <pmu.h>

DECLARE_GLOBAL_DATA_PTR;

sunxi_axp_dev_t  *sunxi_axp_dev[SUNXI_AXP_DEV_MAX] = {(void *)(-1)};

extern int axp22_probe(void);
extern int axp20_probe(void);
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_probe(void)
{
	int ret = 0;
#if defined(CONFIG_SUNXI_AXP22)
	if(axp22_probe())
	{
		printf("probe axp22x failed\n");
	}
	else
	{
		/* pmu type AXP22X */
		tick_printf("PMU: AXP22x found\n");
		ret ++;
	}
	sunxi_axp_dev[PMU_TYPE_22X] = &sunxi_axp_22;
#if (CONFIG_SUNXI_AXP_MAIN == PMU_TYPE_22X)
	sunxi_axp_dev[0] = &sunxi_axp_22;
#endif
#endif

#if defined(CONFIG_SUNXI_AXP20)
	if(axp20_probe())
	{
		printf("probe axp20x failed\n");
	}
	else
	{
		/* pmu type AXP22X */
		tick_printf("PMU: AXP20x found\n");
		ret ++;
	}
	sunxi_axp_dev[PMU_TYPE_20X] = &sunxi_axp_20;
#if (CONFIG_SUNXI_AXP_MAIN == PMU_TYPE_20X)
	sunxi_axp_dev[0] = &sunxi_axp_20;
#endif
#endif

#if defined(CONFIG_SUNXI_AXP15)
	if(axp15_probe())
	{
		printf("probe axp15 failed\n");
	}
	else
	{
		/* pmu type AXP15X */
		tick_printf("PMU: AXP15x found\n");
		ret ++;
	}
	sunxi_axp_dev[PMU_TYPE_15X] = &sunxi_axp_15;
#if (CONFIG_SUNXI_AXP_MAIN == PMU_TYPE_15X)
	sunxi_axp_dev[0] = &sunxi_axp_15;
#endif
#endif

	return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int axp_reinit(void)
{
	int i;

	for(i=0;i<SUNXI_AXP_DEV_MAX;i++)
	{
		if((sunxi_axp_dev[i] != NULL) && (sunxi_axp_dev[i] != (void *)(-1)))
		{
			sunxi_axp_dev[i] = (sunxi_axp_dev_t *)((uint)sunxi_axp_dev[i] + gd->reloc_off);
		}
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_get_power_vol_level(void)
{
	return gd->power_step_level;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_probe_startup_cause(void)
{
	int ret = -1;
	int buffer_value;
	int status;
	int poweron_reason, next_action = 0;

	buffer_value = sunxi_axp_dev[0]->probe_pre_sys_mode();
	debug("axp buffer %x\n", buffer_value);
	if(buffer_value < 0)
	{
		return -1;
	}
    if(buffer_value == PMU_PRE_SYS_MODE)//��ʾǰһ������ϵͳ״̬����һ��Ӧ��Ҳ����ϵͳ
    {
    	tick_printf("pre sys mode\n");
    	return -1;
    }
    else if(buffer_value == PMU_PRE_BOOT_MODE)//��ʾǰһ������boot standby״̬����һ��ҲӦ�ý���boot standby
	{
		tick_printf("pre boot mode\n");
		status = sunxi_axp_dev[0]->probe_power_status();
    	if(status == AXP_VBUS_EXIST)	//only vbus exist
    	{
    		return AXP_VBUS_EXIST;
    	}
    	else if(status == AXP_DCIN_EXIST)	//dc exist(dont care wether vbus exist)
    	{
    		return AXP_DCIN_EXIST;
    	}
		return AXP_VBUS_DCIN_NOT_EXIST;  // return if dont have external power supply
	}
	else if(buffer_value == PMU_PRE_FASTBOOT_MODE)
	{
		tick_printf("pre fastboot mode\n");
		return -1;
	}
	//��ȡ ����ԭ���ǰ������������߲����ѹ����
	poweron_reason = sunxi_axp_dev[0]->probe_this_poweron_cause();
	if(poweron_reason == AXP_POWER_ON_BY_POWER_KEY)
	{
		tick_printf("key trigger\n");
		next_action = PMU_PRE_SYS_MODE;
		ret = 0;
	}
	else if(poweron_reason == AXP_POWER_ON_BY_POWER_TRIGGER)
	{
		tick_printf("power trigger\n");
		next_action = PMU_PRE_SYS_MODE;
    	ret = 1;
	}
	//�ѿ���ԭ��д��Ĵ���
	sunxi_axp_dev[0]->set_next_sys_mode(next_action);

    return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�axp_probe_startup_check_factory_mode
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    �����������ָ��������ú󣬵�һ������ϵͳҪ��Ҫ��USB���ţ��������²��ܰ����������������������־
*
*
************************************************************************************************************
*/
int axp_probe_factory_mode(void)
{
	int buffer_value, status;
	int poweron_reason;

	buffer_value = sunxi_axp_dev[0]->probe_pre_sys_mode();

	if(buffer_value == PMU_PRE_FACTORY_MODE)	//factory mode: need the power key and dc or vbus
	{
		printf("factory mode detect\n");
		status = sunxi_axp_dev[0]->probe_power_status();
		if(status > 0)  //has the dc or vbus
		{
			//��ȡ ����ԭ���ǰ������������߲����ѹ����
			poweron_reason = sunxi_axp_dev[0]->probe_this_poweron_cause();
			if(poweron_reason == AXP_POWER_ON_BY_POWER_KEY)
			{
				//set the system next powerom status as 0x0e(the system mode)
				printf("factory mode release\n");
				sunxi_axp_dev[0]->set_next_sys_mode(PMU_PRE_SYS_MODE);
			}
			else
			{
				printf("factory mode: try to poweroff without power key\n");
				axp_set_hardware_poweron_vol();  //poweroff
				axp_set_power_off();
				for(;;);
			}
		}
		else
		{
			printf("factory mode: try to poweroff without power in\n");
			axp_set_hardware_poweroff_vol();  //poweroff
			axp_set_power_off();
			for(;;);
		}
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_hardware_poweron_vol(void) //���ÿ���֮��PMUӲ���ػ���ѹΪ2.9V
{
	int vol_value = 0;

	if(script_parser_fetch("pmu_para", "pmu_pwron_vol", &vol_value, 1))
	{
		puts("set power on vol to default\n");
	}

	return sunxi_axp_dev[0]->set_power_onoff_vol(vol_value, 1);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_hardware_poweroff_vol(void) //���ùػ�֮��PMUӲ���´ο�����ѹΪ3.3V
{
	int vol_value = 0;

	if(script_parser_fetch("pmu_para", "pmu_pwroff_vol", &vol_value, 1))
	{
		puts("set power off vol to default\n");
	}

	return sunxi_axp_dev[0]->set_power_onoff_vol(vol_value, 0);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_set_power_off(void)
{
	return sunxi_axp_dev[0]->set_power_off();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_next_poweron_status(int value)
{
	return sunxi_axp_dev[0]->set_next_sys_mode(value);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_probe_pre_sys_mode(void)
{
	return sunxi_axp_dev[0]->probe_pre_sys_mode();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_power_get_dcin_battery_exist(int *dcin_exist, int *battery_exist)
{
	*dcin_exist    = sunxi_axp_dev[0]->probe_power_status();
	*battery_exist = sunxi_axp_dev[0]->probe_battery_exist();

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_probe_battery_vol(void)
{
	return sunxi_axp_dev[0]->probe_battery_vol();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_probe_rest_battery_capacity(void)
{
	return sunxi_axp_dev[0]->probe_battery_ratio();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_probe_key(void)
{
	return sunxi_axp_dev[0]->probe_key();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_probe_charge_current(void)
{
	return sunxi_axp_dev[0]->probe_charge_current();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_set_charge_current(int current)
{
	return sunxi_axp_dev[0]->set_charge_current(current);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int  axp_set_charge_control(void)
{
	return sunxi_axp_dev[0]->set_charge_control();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_vbus_limit_dc(void)
{
	sunxi_axp_dev[0]->set_vbus_vol_limit(gd->limit_vol);
	sunxi_axp_dev[0]->set_vbus_cur_limit(gd->limit_cur);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_vbus_limit_pc(void)
{
	sunxi_axp_dev[0]->set_vbus_vol_limit(gd->limit_pcvol);
	sunxi_axp_dev[0]->set_vbus_cur_limit(gd->limit_pccur);

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_all_limit(void)
{
	int usbvol_limit = 0;
	int usbcur_limit = 0;

	script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbvol_limit", &usbvol_limit, 1);
	script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbcur_limit", &usbcur_limit, 1);
	script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbvol", (int *)&gd->limit_vol, 1);
	script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbcur", (int *)&gd->limit_cur, 1);
	script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbvol_pc", (int *)&gd->limit_pcvol, 1);
	script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbcur_pc", (int *)&gd->limit_pccur, 1);
#ifdef DEBUG
	printf("usbvol_limit = %d, limit_vol = %d\n", usbvol_limit, gd->limit_vol);
	printf("usbcur_limit = %d, limit_cur = %d\n", usbcur_limit, gd->limit_cur);
#endif
	if(!usbvol_limit)
	{
		gd->limit_vol = 0;

	}
	if(!usbcur_limit)
	{
		gd->limit_cur = 0;
	}

	axp_set_vbus_limit_pc();

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_suspend_chgcur(void)
{
	return sunxi_axp_dev[0]->set_charge_current(gd->pmu_suspend_chgcur);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_runtime_chgcur(void)
{
	return sunxi_axp_dev[0]->set_charge_current(gd->pmu_runtime_chgcur);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_charge_vol_limit(void)
{
	int ret1;
	int ret2;

	ret1 = script_parser_fetch(PMU_SCRIPT_NAME, "pmu_runtime_chgcur", (int *)&gd->pmu_runtime_chgcur, 1);
	ret2 = script_parser_fetch(PMU_SCRIPT_NAME, "pmu_suspend_chgcur", (int *)&gd->pmu_suspend_chgcur, 1);

	if(ret1)
	{
		gd->pmu_runtime_chgcur = 600;
	}
	if(ret2)
	{
		gd->pmu_suspend_chgcur = 1500;
	}
#if DEBUG
	printf("pmu_runtime_chgcur=%d\n", gd->pmu_runtime_chgcur);
	printf("pmu_suspend_chgcur=%d\n", gd->pmu_suspend_chgcur);
#endif
	axp_set_suspend_chgcur();

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_set_power_supply_output(void)
{
	int  ret, onoff;
	uint power_supply_hd;
	char power_name[16];
	int  power_vol, power_index = 0;

#if defined(CONFIG_SUNXI_AXP22)
    power_supply_hd = script_parser_fetch_subkey_start("power_sply");
    if(!power_supply_hd)
    {
        printf("unable to set power supply\n");

		return -1;
	}
	do
	{
		memset(power_name, 0, 16);
		ret = script_parser_fetch_subkey_next(power_supply_hd, power_name, &power_vol, &power_index);
		if(ret < 0)
		{
			printf("find power_sply to end\n");

            return 0;
        }
        printf("%s = %d\n", power_name, power_vol);
        onoff = -1;
        if((power_vol>>16) & 0xffff)
        {
            onoff = 1;
        }
        if(sunxi_axp_dev[0]->set_supply_status_byname(power_name, power_vol & 0xffff, onoff))
        {
            printf("axp set %s to %d failed\n", power_name, power_vol);
        }
    }
    while(1);
#endif
#if defined(CONFIG_SUNXI_AXP20)
    power_supply_hd = script_parser_fetch_subkey_start("target");
    if(!power_supply_hd)
    {
        printf("unable to set power supply\n");

        return -1;
    }
    do
    {
        memset(power_name, 0, 16);
        ret = script_parser_fetch_subkey_next(power_supply_hd, power_name, &power_vol, &power_index);
        if(ret < 0)
        {
            printf("find power_sply to end\n");

            return 0;
        }
        printf("%s = %d\n", power_name, power_vol);
        onoff = -1;
        if((power_vol>>16) & 0xffff)
        {
            onoff = 1;
        }
        if(sunxi_axp_dev[0]->set_supply_status_byname(power_name, power_vol & 0xffff, onoff))
        {
            printf("axp set %s to %d failed\n", power_name, power_vol);
        }
    }
    while(1);
#endif
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_probe_power_supply_condition(void)
{
	int   dcin_exist;
	int   ratio;

	//����ѹ�������Ƿ񿪻�
	dcin_exist = sunxi_axp_dev[0]->probe_power_status();

#ifdef DEBUG
    printf("dcin_exist = %x\n", dcin_exist);
#endif
    //���ж�����������ϴιػ���¼�ĵ����ٷֱ�<=5%,ͬʱ���ؼ�ֵС��5mAh����ػ�����������ж�
	ratio = sunxi_axp_dev[0]->probe_battery_ratio();
	tick_printf("PMU: bat ratio = %d\n", ratio);
	if(ratio < 0)
	{
		return -1;
	}
	else if(ratio < 1)		//��ʾratio=0�����͵���
	{
		if(dcin_exist)
		{
			//�ⲿ��Դ���ڣ���ص�������
			gd->power_step_level = BATTERY_RATIO_TOO_LOW_WITH_DCIN;
		}
		else
		{
			//�ⲿ��Դ�����ڣ���ص�������
			gd->power_step_level = BATTERY_RATIO_TOO_LOW_WITHOUT_DCIN;
		}
	}
	else
	{
		gd->power_step_level = BATTERY_RATIO_ENOUGH;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
static __u8 power_int_value[8];

int axp_int_enable(__u8 *value)
{
	sunxi_axp_dev[0]->probe_int_enable(power_int_value);
	sunxi_axp_dev[0]->set_int_enable(value);
	//��Сcpu���ж�ʹ��
	//*(volatile unsigned int *)(0x01f00c00 + 0x10) |= 1;
	//*(volatile unsigned int *)(0x01f00c00 + 0x40) |= 1;

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_int_disable(void)
{
	//*(volatile unsigned int *)(0x01f00c00 + 0x10) |= 1;
	//*(volatile unsigned int *)(0x01f00c00 + 0x40) &= ~1;
	return sunxi_axp_dev[0]->set_int_enable(power_int_value);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int axp_int_query(__u8 *addr)
{
	int ret;

	ret = sunxi_axp_dev[0]->probe_int_pending(addr);
	//*(volatile unsigned int *)(0x01f00c00 + 0x10) |= 1;

	return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :  ���pmu_type = 0, ��ʾ������PMU
*                     ����Ϸ��ķ���ֵ����ʾ����ָ��pmu
*                     ����Ƿ�ֵ����ִ���κβ���
*
************************************************************************************************************
*/
int axp_set_supply_status(int pmu_type, int vol_name, int vol_value, int onoff)
{
	//���ò�ͬ�ĺ���ָ��
	return sunxi_axp_dev[pmu_type]->set_supply_status(vol_name, vol_value, onoff);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int axp_set_supply_status_byname(char *vol_type, int vol_value, int onoff)
{
	//���ò�ͬ�ĺ���ָ��
	int   i;
	int   index = -1;

	for(i=1;i<SUNXI_AXP_DEV_MAX;i++)
	{
		if((sunxi_axp_dev[i] == NULL) || (sunxi_axp_dev[i] == (void *)(-1)))
		{
			return -1;
		}
		if(sunxi_axp_dev[i]->pmu_name == NULL)
		{
			return -1;
		}
		if(!strncmp(sunxi_axp_dev[i]->pmu_name, vol_type, 5))
		{
			index = i;
			break;
		}
	}

	return sunxi_axp_dev[index]->set_supply_status_byname(vol_type + 6, vol_value, onoff);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int axp_probe_supply_status(int pmu_type, int vol_name, int vol_value, int onoff)
{
	//���ò�ͬ�ĺ���ָ��
	return sunxi_axp_dev[pmu_type]->probe_supply_status(vol_name, vol_value, onoff);
}





