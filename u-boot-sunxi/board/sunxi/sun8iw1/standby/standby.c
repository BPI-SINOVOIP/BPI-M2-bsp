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
#include "standby_i.h"
#include "standby.h"
#include "sunxi_board.h"

DECLARE_GLOBAL_DATA_PTR;

static int boot_enter_standby(void);
static int boot_exit_standby(void);
static int boot_standby_detect(void);
static int boot_mod_enter_standby(void);
static int boot_mod_exit_standby(void);
static int boot_early_standby_mode(void);

static int standby_flag = 0;

int boot_standby_mode(void)
{
	int status;

	//axp_set_suspend_chgcur();
	boot_store_sp();
	boot_set_sp();

	status = boot_early_standby_mode();
	//standby_serial_putc('[');

	boot_restore_sp();
	//standby_serial_putc('{');

	return status;
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
static int boot_early_standby_mode(void)
{
	__s32 key_status, usb_status;
	__s32 status;

	//����Ƿ��а�������
	key_status = standby_axp_probe_key();
	if(key_status & 0x01)			//������������£����ܵ�Դ�Ƿ��Ƴ���ֱ�ӽ���ϵͳ
	{
		if(standby_flag)
		{
			boot_mod_exit_standby();
		}
		return 3;
	}
	//����ⲿ��Դ�Ƿ����
	if(standby_axp_probe_power_exist() <= 0)
	{
		if(standby_flag)
		{
			boot_mod_exit_standby();
		}
		return 4;
	}
	if(key_status & 0x02)			//�̰���ѹ����������£���ʾ��綯��
	{
		if(standby_flag)
		{
			boot_mod_exit_standby();
		}
		return 2;
	}
	if(!standby_flag)
	{
		boot_mod_enter_standby();      //����ģ�����standby
	}
	//����Ƿ���USB��Դ����
	usb_status = standby_axp_probe_usb();
	if(usb_status > 0)
	{
		return 8;
	}
	status = -1;
	boot_enter_standby();
	do
	{
		//��ʼѭ������Ƿ�ʼ����
		boot_halt();
		status = boot_standby_detect();
	}
	while(status <= 0);
	//������Ҫ���ѣ��˳�standby
	boot_exit_standby();
	//�˳�ģ���standby
	if((status != 8) && (status != 9))
	{
		//standby_serial_putc('x');
		boot_mod_exit_standby();
		standby_flag = 0;
	}
	else
	{
		standby_flag = 1;
	}
	//standby_serial_putc('[');

	return status;
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
static int boot_enter_standby(void)
{
	//�����ж�
	standby_int_disable();

	//mctl_deep_sleep_test();
	dram_power_save_process();

	//standby_serial_putc('1');

	standby_gic_store();
	//standby_serial_putc('m');
	standby_axp_store_int_status();
	//standby_serial_putc('n');
	standby_clock_to_24M();
	//standby_serial_putc('o');
	standby_clock_plldisable();
	//standby_serial_putc('p');
	standby_axp_output_control(0);

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
static int boot_exit_standby(void)
{
	standby_axp_output_control(1);
	standby_clock_pllenable();
	standby_clock_to_pll1();
	//standby_serial_putc('7');
	dram_power_up_process();
	//standby_serial_putc('8');
	standby_axp_restore_int_status();
	standby_gic_restore();
	standby_int_enable();
	//standby_serial_putc('9');

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
static int boot_standby_detect(void)
{
	__u8  power_int_status[8];

	//����жϴ���
	standby_axp_int_query(power_int_status);
	//standby_serial_putc('1');
	if(power_int_status[2] & 0x02)			//��Դ�����̰�
	{
		return 2;
	}
	if(power_int_status[2] & 0x01)			//��Դ��������
	{
		return 3;
	}
	if(power_int_status[0] & 0x24)			//�ⲿ��Դ�Ƴ�
	{
		if(standby_axp_probe_power_exist() <= 0)	//û���ⲿ��Դ����
		{
			return 4;
		}
	}
	if(power_int_status[0] & 0x08)			//�ⲿ��Դ����
	{
		return 8;
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
static int boot_mod_enter_standby(void)
{
	uint addr;
	int (* boot_mod_standby)(uint cmd, void *pArg);

	addr = (uint)drv_disp_standby + gd->reloc_off;
	boot_mod_standby = (int (* )(uint cmd, void *pArg))addr;

	boot_mod_standby(BOOT_MOD_ENTER_STANDBY, 0);

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
static int boot_mod_exit_standby(void)
{
	uint addr;
	int (* boot_mod_standby)(uint cmd, void *pArg);

	addr = (uint)drv_disp_standby + gd->reloc_off;
	boot_mod_standby = (int (* )(uint cmd, void *pArg))addr;

	boot_mod_standby(BOOT_MOD_EXIT_STANDBY, 0);

	return 0;
}




