/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Martin zheng <zhengjiewen@allwinnertech.com>
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
#include "axp_i.h"
#include "standby.h"
#include <common.h>
#include <linux/types.h>
#include <i2c.h>
#include <asm/arch/power.h>
#include <sys_config.h>

#define DCDC2_VALUE_FOR_BOOT    (1320)      //define dcdc2 to 1.32v for boot


extern void __msdelay(unsigned long msec);

extern  __u32 PMU_type;

extern  __u32 trans_dcdc2_user_set;

static  __u32 power_step_level;
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

static __s32 _axp_clr_status(void)
{
    __u8  reg_addr1,value1;

    reg_addr1 = BOOT_POWER20_DATA_BUFFER0;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER1;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER2;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER3;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER4;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER5;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER6;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER7;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER8;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER9;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER10;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    reg_addr1 = BOOT_POWER20_DATA_BUFFER11;
    value1 = 0x00;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_write(AXP20_ADDR, &reg_addr1, 1, &value1, 1))
    {
        return -1;
    }

    return ;
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
static __s32 _axp_power_off(void)
{
    __u8   reg_addr, value;

    if(PMU_type == PMU_TYPE_AXP209)            //AXP20
    {
        reg_addr = BOOT_POWER20_OFF_CTL;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        value |= 1 << 7;
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
    }

    return 0;
}
/*
 ****************************************************************************************************
 *
 *             DRV_POWER_INIT
 *
 *  Description: ��ʼ��������ͨ����ֻ��ִ��һ�εĲ�����������
 *               ��������һ�ΰ�װ��ʱ�������������
 *               �˺���û�в�������Ҫ������ʱ����Ҫ�����ýű�ȥ��ȡ
 *  Parameters:
 *
 *  Return value:
 *       0
 *       -1
 ****************************************************************************************************
 */
__s32 boot_power_init(void *power_para)
{
    __u8  reg_addr, value;
    __u8  reg_addr1,value1;
    boot_core_para_t   *core_para = (boot_core_para_t *)power_para;
    __u32 dcin_exist, bat_exist;
    __u32 bat_vol;

    //try to check if the pmu is AXP199
    reg_addr = BOOT_POWER20_VERSION;
    if(!core_para->user_set_core_vol)
    {
        core_para->user_set_core_vol = DCDC2_VALUE_FOR_BOOT;
    }
    trans_dcdc2_user_set = core_para->user_set_core_vol;
    /*if(!BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
    if(!i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
    {
        value &= 0xf;
        if(value == 0x01)
        {
            PMU_type     = PMU_TYPE_AXP209;
        }
        else if((!value) || (value == 0x03))
        {
            PMU_type     = PMU_TYPE_AXP199;
        }
        else
        {
            PMU_type = PMU_TYPE_NONE;

            return -1;
        }
    }
    else
    {
        //value is invalid
        PMU_type = PMU_TYPE_NONE;

        return -1;
    }
    printf("pmu type = %d\n", PMU_type);
    boot_power_data_trans();
    //Ϊ�����¾ɰ汾IC��ȡ��PMUӲ�����RDC����
    if(PMU_type == PMU_TYPE_AXP209)
    {
        reg_addr1 = BOOT_POWER20_COULOMB_CAL;
        /*BOOT_TWI_Read(AXP20_ADDR, &reg_addr1, &value1);*/
        i2c_read(AXP20_ADDR, reg_addr1, 1, &value1, 1);
        value1 |= 0x80;//pouse ocv.
        /*BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1);*/
        i2c_write(AXP20_ADDR, reg_addr1, 1, &value1, 1);
        reg_addr1 = BOOT_POWER20_RDC1;
        /*BOOT_TWI_Read(AXP20_ADDR, &reg_addr1, &value1);*/
        i2c_read(AXP20_ADDR, &reg_addr1, 1, &value1, 1);
        value1 &= 0x7F;//disable rdc calculation
        /*BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1);*/
        i2c_write(AXP20_ADDR, reg_addr1, 1, &value1, 1);
        reg_addr1 = BOOT_POWER20_COULOMB_CAL;
        /*BOOT_TWI_Read(AXP20_ADDR, &reg_addr1, &value1);*/
        i2c_read(AXP20_ADDR, reg_addr1, 1, &value1, 1);
        value1 &= 0x7F;//start ocv
        /*BOOT_TWI_Write(AXP20_ADDR, &reg_addr1, &value1);*/
        i2c_write(AXP20_ADDR, reg_addr1, 1, &value1, 1);
    }

    reg_addr1 = BOOT_POWER20_COULOMB_CTL;
    /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr1, &value1))*/
    if(i2c_read(AXP20_ADDR, reg_addr1, 1, &value1, 1))
    {
        return -1;
    }
    if(!(value1 & 0x80)) // coulomb conuter on off control
    {
        _axp_clr_status(); //clear the data buffer
    }

    //ʹ�ܿ��ؼ�
    boot_power_enable_coulomb();
    //����ѹ�������Ƿ񿪻�
    boot_power_get_dcin_battery_exist(&dcin_exist, &bat_exist);
    //���ж�����������ϴιػ���¼�ĵ����ٷֱ�<=5%,ͬʱ���ؼ�ֵС��5mAh����ػ�����������ж�
    __msdelay(100);
    boot_power_get_battery_vol(&bat_vol);
    printf("bat vol = %d mv\n", bat_vol);
    if((bat_vol < 3400) && (!dcin_exist))
    {
        boot_set_power_off_vol(); //���˲���Ӧ�ùػ�������������С�
    }
    debug("core_para->vol_threshold =%d\n",core_para->vol_threshold);
    debug("core_para->user_set_core_vol =%d\n",core_para->user_set_core_vol);
    debug("core_para->user_set_clock =%d\n",core_para->user_set_clock);
    reg_addr1 = BOOT_POWER20_DATA_BUFFER1;										//��֮ǰ�ıȷֱȼ�¼
    /*BOOT_TWI_Read(AXP20_ADDR, &reg_addr1, &value1);*/
    i2c_read(AXP20_ADDR, reg_addr1, 1, &value1, 1);
    if(!core_para->vol_threshold)
    {
        core_para->vol_threshold = 3600;
    }
    if(value1 & 0x80)				            //����־λ��1��Ч
    {
        int bat_cou;
        int bat_value;

        bat_value = value1 & 0x7f;  //��ذٷֱ�
        bat_cou = ABS(Get_Bat_Coulomb_Count());
        if((bat_value <= 0) && bat_cou < 30)     //����С��5%ͬʱ���ؼ�ֵС��6���򲻿���
        {
            printf("bat_cou=%x\n", bat_cou);
            if(dcin_exist)
            {
                if(bat_vol > (core_para->vol_threshold+100))
                {
                    _axp_clr_status();
                    power_step_level = BATTERY_RATIO_ENOUGH;
                    printf("dcin_exist\n");
                }
                else
                {
                    power_step_level = BATTERY_RATIO_TOO_LOW_WITH_DCIN;
                }
            }
            else
            {
                if(bat_vol > core_para->vol_threshold)
                {
                    power_step_level = BATTERY_RATIO_ENOUGH;
                    _axp_clr_status();
                    printf("bat_vol > %d\n",core_para->vol_threshold);
                }
                else
                {
                    power_step_level = BATTERY_RATIO_TOO_LOW_WITHOUT_DCIN;
                }
            }
            printf("power_step_level =%x, bat_vol = %x\n", power_step_level,bat_vol);
        }
        else															//����һ�����������㣬�Ϳ��Կ���
        {
            power_step_level = BATTERY_RATIO_ENOUGH;
        }
    }
    else
    {
        if(dcin_exist)
        {
            //			if(bat_vol > 3900)
            //			{
            //				_axp_clr_status();
            power_step_level = BATTERY_RATIO_ENOUGH;
            debug("no baterry ratio flag,dcin_exist,set BATTERY_RATIO_ENOUGH\n");
            //			}
            //			else
            //			{
            //				power_step_level = BATTERY_RATIO_TOO_LOW_WITH_DCIN;
            //			}
        }
        else
        {
            if(bat_vol >= core_para->vol_threshold)
            {
                power_step_level = BATTERY_RATIO_ENOUGH;
            }
            else
            {
                power_step_level = BATTERY_RATIO_TOO_LOW_WITHOUT_DCIN;
            }
        }

    }
    debug("after set,power_step_level=%d\n",power_step_level);
    //set dcdc2 value
    boot_power_set_dcdc2(core_para->user_set_core_vol);
    //boot_power_set_dcdc3(1250);
    if(PMU_type == PMU_TYPE_AXP209)
    {
        // open bat acin vbus voltage and current adc
        reg_addr = BOOT_POWER20_ADC_EN1;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        value |= 0xfe << 0;
        reg_addr = BOOT_POWER20_ADC_EN1;
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_write(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        //Enable Charge
        reg_addr = BOOT_POWER20_CHARGE1;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        value |= 0x80 << 0;
        reg_addr = BOOT_POWER20_CHARGE1;
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_write(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        //set bus current limit off
        reg_addr = BOOT_POWER20_IPS_SET;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        value &= 0xfC;
        value |= 0x03;
        reg_addr = BOOT_POWER20_IPS_SET;
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_write(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        {
            __u8 coulomb_val, tmp_val;
            //hold
            reg_addr = BOOT_POWER20_COULOMB_CAL;
            /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &coulomb_val))*/
            if(i2c_read(AXP20_ADDR, reg_addr, 1, &coulomb_val, 1))
            {
                return -1;
            }
            coulomb_val |= 0x80;
            /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &coulomb_val))*/
            if(i2c_write(AXP20_ADDR, reg_addr, 1,&coulomb_val, 1))
            {
                return -1;
            }
            //
            reg_addr = 0xBA;
            /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &tmp_val))*/
            if(i2c_read(AXP20_ADDR, reg_addr, 1, &tmp_val, 1))
            {
                return -1;
            }
            tmp_val &= ~0x80;
            /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &tmp_val))*/
            if(i2c_write(AXP20_ADDR, reg_addr, 1, &tmp_val, 1))
            {
                return -1;
            }
            reg_addr = BOOT_POWER20_COULOMB_CAL;
            coulomb_val &= ~0x80;
            /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &coulomb_val))*/
            if(i2c_write(AXP20_ADDR, reg_addr, 1, &coulomb_val, 1))
            {
                return -1;
            }
        }

    }
    else
    {
        return -1;
    }

    return 0;
}
/*
 ****************************************************************************************************
 *
 *             DRV_POWER_EXIT
 *
 *  Description: ж��������ͨ����ֻ��ִ��һ�εĲ�����������
 *               �������˳���ʱ����������������
 *               ж�غ���û�в���
 *  Parameters:
 *
 *  Return value:
 *       0
 *       -1
 ****************************************************************************************************
 */
__s32 boot_power_exit(void)
{
    return 0;
}
__s32 boot_power_enable_coulomb(void)
{
    __u8   reg_addr;
    __u8   tmp_value;

    reg_addr = BOOT_POWER20_COULOMB_CTL;
    tmp_value = 0x80;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &tmp_value))*/
    if(i2c_write(AXP20_ADDR, reg_addr, 1, &tmp_value, 1))
    {
        return -1;
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
__s32 boot_power_get_battery_vol(__u32 *battery_vol)
{
    __u8 reg_addr, value;

    if(PMU_type == PMU_TYPE_AXP209)
    {
        //pmu is AXP209
        reg_addr = BOOT_POWER20_BAT_AVERVOL_H8;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        *battery_vol  = (value << 4);
        reg_addr = BOOT_POWER20_BAT_AVERVOL_L4;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        *battery_vol += value;
        *battery_vol *= 1.1;
    }
    else
    {
        return -1;
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
__s32 boot_power_set_off(void)
{
    return _axp_power_off();
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
__s32 boot_power_set_data_buffer(int value)
{
    __u8   reg_addr;
    __u8   tmp_value;

    reg_addr = 0x0f;
    tmp_value = value & 0xff;
    /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &tmp_value))*/
    if(i2c_write(AXP20_ADDR, reg_addr, 1, &tmp_value, 1))
    {
        return -1;
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
__s32 boot_power_check_startup(void)
{
    __u8   dc_in,  startup_trigger;
    __u8   reg_addr, value;

    if(PMU_type == PMU_TYPE_AXP209)
    {
        __u8 tmp_value;
        __s32 ret;

        reg_addr = BOOT_POWER20_DATA_BUFFER11;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        if(value == 0x0e)		//��ʾǰһ������ϵͳ״̬����һ��Ӧ��Ҳ����ϵͳ
        {
            printf("pre sys mode\n");
            return -1;
        }
        else if(value == 0x0f)      //��ʾǰһ������boot standby״̬����һ��ҲӦ�ý���boot standby
        {
            printf("pre boot mode\n");
            return 0;
        }

        reg_addr = BOOT_POWER20_STATUS;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        //dc_in = ((value >> 4) & 0x01) |  ((value >> 6) & 0x01);
        startup_trigger  = (value >> 0) & 0x01;

        reg_addr = BOOT_POWER20_DATA_BUFFER11;
        if(startup_trigger==AXP_POWER_ON_BY_POWER_TRIGGER)    //���ACIN/VBUS��������ACIN/VBUS������������power_off
        {
            printf("power trigger\n");
            tmp_value = 0x0f;
            ret = 0;
        }
        else
        {
            printf("key trigger\n");
            tmp_value = 0x0e;
            ret = 1;
        }
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &tmp_value))*/
        if(i2c_write(AXP20_ADDR, reg_addr, 1, &tmp_value, 1))
        {
            return -1;
        }

        return ret;
    }

    return -1;

ext_power_checked:

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
__s32 boot_power_set_sw1(__u32 on_off)
{
    __u8 reg_addr, value;

    if(PMU_type == PMU_TYPE_AXP209)
    {
        ;
    }
    else
    {
        return -1;
    }

    return 0;
}

__s32 boot_power_set_vol(void)
{
    boot_power_set_dcdc3(-1);
    boot_power_set_ldo2(-1);
    boot_power_set_ldo3(-1);
    boot_power_set_ldo4(-1);

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
__s32 boot_power_type(void)
{
    __u8  reg_addr;
    __u8  value;
    int   type;

    reg_addr = BOOT_POWER20_STATUS;
    /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
    if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
    {
        return -1;
    }
    type = (value >>5) & 0x01;
    type |= ((value >>7) & 0x01) << 1;

    return type;
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
__s32 boot_set_power_on_vol(void) //���ÿ���֮��PMUӲ���ػ���ѹΪ2.9V
{
    __u8  reg_addr;
    __u8  reg_value;
    __s32 vol_value, ret;

    ret = script_parser_fetch("pmu_para", "pmu_pwron_vol", &vol_value, 1);
    if(ret)
    {
        printf("boot power:unable to find power off vol set\n");
        vol_value = 2900;
    }
    reg_addr = BOOT_POWER20_VOFF_SET;
    /*if(!BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &reg_value))*/
    if(!i2c_read(AXP20_ADDR, reg_addr, 1, &reg_value, 1))
    {
        reg_value &= 0xf8;
        if(vol_value >= 2600 && vol_value <= 3300)
        {
            reg_value |= (vol_value - 2600)/100;
        }
        else if(vol_value <= 2600)
        {
            reg_value |= 0x00;
        }
        else
        {
            reg_value |= 0x07;
        }
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &reg_value))*/
        if(i2c_write(AXP20_ADDR, reg_addr, 1, &reg_value, 1))
        {
            printf("boot power:unable to set voff vol\n");
        }
        else
        {
            return 0;
        }
    }

    return -1;
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
__s32 boot_set_power_off_vol(void) //���ùػ�֮��PMUӲ���´ο�����ѹΪ3.3V
{
    __u8  reg_addr;
    __u8  reg_value;
    __s32 vol_value, ret;

    boot_config_charge_current(1);//config shutdown charge current
    ret = script_parser_fetch("pmu_para", "pmu_pwroff_vol", &vol_value, 1);
    if(ret)
    {
        printf("boot power:unable to find power on vol set\n");
        vol_value = 3300;
    }
    reg_addr = BOOT_POWER20_VOFF_SET;
    /*if(!BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &reg_value))*/
    if(!i2c_read(AXP20_ADDR, reg_addr, 1, &reg_value, 1))
    {
        reg_value &= 0xf8;
        if(vol_value >= 2600 && vol_value <= 3300)
        {
            reg_value |= (vol_value - 2600)/100;
        }
        else if(vol_value <= 2600)
        {
            reg_value |= 0x00;  // 2.9v
        }
        else
        {
            reg_value |= 0x07;  // 3.3v
        }
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &reg_value))*/
        if(i2c_write(AXP20_ADDR, reg_addr, 1, &reg_value, 1))
        {
            printf("boot power:unable to set power on vol\n");
        }
        else
        {
            //����VBUS������
            //			boot_power_vbus_cur_limit();
            //������ɣ�ͬʱ�ػ�
            reg_addr = POWER_OFF_CTL;
            /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &reg_value))*/
            if(i2c_read(AXP20_ADDR, reg_addr, 1, &reg_value, 1))
            {
                return -1;
            }
            reg_value |= 1 << 7;
            /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &reg_value))*/
            if(i2c_write(AXP20_ADDR, reg_addr, 1, &reg_value, 1))
            {
                return -1;
            }

            return 0;
        }
    }

    return -1;
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
__s32 boot_get_power_vol_level(void)
{
    return power_step_level;
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
__s32 boot_set_charge_current(int current)
{
    __u8  reg_addr, value;
    int   step;

    if(PMU_type == PMU_TYPE_AXP209)
    {
        reg_addr = BOOT_POWER20_CHARGE1;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        value &= ~0x0f;
        if(current > 1800)
        {
            current = 1800;
        }
        else if(current < 300)
        {
            current = 300;
        }
        step   = (current-300)/100 ;//before is (current/100-3)
        value |= (step & 0x0f);
        /*if(BOOT_TWI_Write(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_write(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
    }
    else
    {
        return -1;
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
__s32 boot_get_charge_current(void)
{
    __u8  reg_addr, value;
    int	  current;

    if(PMU_type == PMU_TYPE_AXP209)
    {
        reg_addr = BOOT_POWER20_CHARGE1;
        /*if(BOOT_TWI_Read(AXP20_ADDR, &reg_addr, &value))*/
        if(i2c_read(AXP20_ADDR, reg_addr, 1, &value, 1))
        {
            return -1;
        }
        value &= 0x0f;
        current = (value + 3) * 100;
    }
    else
    {
        return -1;
    }

    return current;
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
__s32 boot_config_charge_current(int start_time)
{
    __s32 ret;
    int   value;
    script_gpio_set_t  gpio;
    __u32  gpio_hd;

    //1 ����ػ�״̬
    //0 ������״̬
    if(start_time == 1)
    {
        ret = script_parser_fetch("pmu_para", "pmu_used2", &value, 1);
        if((ret >= 0) && (value == 1))
        {
            ret = script_parser_fetch("pmu_para", "pmu_adpdet", (void *)&gpio, sizeof(script_gpio_set_t)/sizeof(int));
            if(ret >= 0)
            {
                /*gpio_hd = boot_GPIO_Request((void *)&gpio, 1);*/
                /*value = boot_GPIO_Read_One_PIN_Value(gpio_hd, 0);*/
                if(value > 0)
                {
                    ret = script_parser_fetch("pmu_para", "pmu_shutdown_chgcur", &value, 1);
                }
                else
                {
                    ret = script_parser_fetch("pmu_para", "pmu_shutdown_chgcur2", &value, 1);
                }
                if(ret >= 0)
                {
                    boot_set_charge_current(value);

                    return 0;
                }
            }
        }
        else
        {
            ret = script_parser_fetch("pmu_para", "pmu_shutdown_chgcur", &value, 1);
            if(ret >= 0)
            {
                boot_set_charge_current(value);

                return 0;
            }
        }
    }
    else
    {
        boot_set_charge_current(300);
    }

    return -1;
}

