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
#include <malloc.h>
#include <asm/arch/intc.h>
#include <pmu.h>
#include "power_probe.h"


DECLARE_GLOBAL_DATA_PTR;

extern int boot_standby_action;
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
static void power_int_irq(void *p_arg)
{
#ifdef DEBUG
	int i;
#endif
	unsigned char power_int_status[8];
	int  dc_exist, bat_exist;

	axp_int_query(power_int_status);
#ifdef DEBUG
	for(i=0;i<5;i++)
	{
		tick_printf("int status %d %x\n", i, power_int_status[i]);
	}
#endif
	if(power_int_status[0] & 0x48)   //�ⲿ��Դ����
	{
		axp_power_get_dcin_battery_exist(&dc_exist, &bat_exist);
		if(dc_exist)
		{
			tick_printf("power insert\n");
			boot_standby_action &= ~0x10;
			boot_standby_action |= 0x04;
		}
	}
	if(power_int_status[0] & 0x8)   //usb �����жϣ�����usb���
	{
		tick_printf("usb in\n");
		boot_standby_action |= 8;
		usb_detect_enter();
	}
	if(power_int_status[0] & 0x4)
	{
		tick_printf("usb out\n");
		boot_standby_action &= ~0x04;
		boot_standby_action |= 0x10;
		usb_detect_exit();
	}
	if(power_int_status[2] & 0x2)	//�̰���
	{
		tick_printf("short key\n");
		boot_standby_action |= 2;

	}
	if(power_int_status[2] & 0x1)	//������
	{
		tick_printf("long key\n");
		boot_standby_action |= 1;
	}

	return;
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
void power_limit_detect_enter(void)
{
	unsigned char power_int_enable[8];

	power_int_enable[0] = 0x4C;  //dc in/out, usb in/out
	power_int_enable[1] = 0;
	power_int_enable[2] = 3;
	power_int_enable[4] = 0;
	power_int_enable[5] = 0;

	tick_printf("power limit detect enter\n");

	axp_int_enable(power_int_enable);
	irq_install_handler(AW_IRQ_NMI, power_int_irq, 0);
	irq_enable(AW_IRQ_NMI);
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
void power_limit_detect_exit(void)
{
	usb_detect_exit();
	irq_disable(AW_IRQ_NMI);
	axp_int_disable();

	tick_printf("power limit detect exit\n");
}