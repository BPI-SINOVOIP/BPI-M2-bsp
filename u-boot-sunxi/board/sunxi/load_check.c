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
#include <asm/arch/drv_display.h>
#include <bat.h>
#include <sys_config.h>
#include <asm/arch/timer.h>
#include <pmu.h>
#include "bat_cartoon.h"
#include "power_probe.h"
#include "de.h"
#include <standby.h>

DECLARE_GLOBAL_DATA_PTR;

int boot_standby_action = 0;

typedef int (* standby_func)(void);

static int board_try_boot_standby(void)
{
	uint func_addr = (uint)boot_standby_mode;
	standby_func   boot_standby_func;

	//cal the real function address of boot_standby_mode
	flush_dcache_all();
	boot_standby_func = (standby_func)(func_addr - gd->reloc_off);

	return boot_standby_func();
}

static int board_probe_power_level(void)
{
	int power_status;
	int power_start;

	//���power����
	axp_probe_key();
	//��ȡ��Դ״̬
	power_status = axp_get_power_vol_level();
	debug("power status = %d\n", power_status);
	if(power_status == BATTERY_RATIO_TOO_LOW_WITHOUT_DCIN)
	{
		tick_printf("battery power is low without no dc or ac, should be set off\n");
		sunxi_bmp_display("bat\\low_pwr.bmp");
		__msdelay(3000);

		return -1;
	}
	power_start = 0;
	//power_start�ĺ���
	//0: ��������ţֱ�ӿ���������ͨ���жϣ�����������������ֱ�ӿ���������power������ǰ����ϵͳ״̬�������ص������ͣ���������
	//1: ����״̬�£�������ţֱ�ӿ�����ͬʱҪ���ص����㹻��
	//2: ��������ţֱ�ӿ���������ͨ���жϣ�����������������ֱ�ӿ���������power������ǰ����ϵͳ״̬����Ҫ���ص���
	//3: ����״̬�£�������ţֱ�ӿ�������Ҫ���ص���
	script_parser_fetch(PMU_SCRIPT_NAME, "power_start", &power_start, 1);
	debug("power start cause = %d\n", power_start);
	if(power_status == BATTERY_RATIO_TOO_LOW_WITH_DCIN)//�͵磬ͬʱ���ⲿ��Դ״̬��
	{
		if(!(power_start & 0x02))	//��Ҫ�жϵ�ǰ��ص�����Ҫ��power_start�ĵ�1bit��ֵΪ0
		{							//��������£�ֱ�ӹػ�
			tick_printf("battery low power with dc or ac, should charge longer\n");
			sunxi_bmp_display("bat\\bempty.bmp");
			__msdelay(3000);

			return -2;
		}
		else
		{
			if(power_start == 3)	//����Ҫ�жϵ�ǰ��ص��������Ϊ3�������ϵͳ�����Ϊ0������к����ж�
			{
				return 0;
			}
		}
	}
	else							//��ص����㹻����£�����û�е��
	{
		if(power_start & 0x01)		//�����0bit��ֵΪ1�������ϵͳ
		{
			return 0;
		}
	}

	return 1;
}

static int board_probe_poweron_cause(void)
{
	int status = -1;

	status = axp_probe_startup_cause();
	debug("startup status = %d\n", status);
#ifdef FORCE_BOOT_STANDBY
	status = 1;
#endif
	return status;
}

static int board_probe_bat_status(int standby_mode)
{
	int   dc_exist, bat_exist, counter;
	int   bat_init = 0;
	int   bat_cal = 1;
	//��ǰ����ȷ���ǻ�ţ�����������Ƿ񿪻�����ȷ������Ҫȷ�ϵ���Ƿ����
	//����ز����ڼ���������ش�����ػ�
	counter = 4;
	do
	{
		dc_exist = 0;
		bat_exist = 0;
		axp_power_get_dcin_battery_exist(&dc_exist, &bat_exist);
		printf("bat_exist=%d\n", bat_exist);
		if(bat_exist == -1)
		{
			printf("bat is unknown\n");
			if(!bat_init)
			{
				if(battery_charge_cartoon_init(0) < 0)
				{
					tick_printf("init charge cartoon fail\n");

					return -1;
				}
				bat_init = 1;
			}
			__msdelay(500);
		}
		else
		{
			break;
		}
	}
	while(counter --);
#ifdef FORCE_BOOT_STANDBY
	bat_exist = 1;
#else
	if(standby_mode)
	{
		bat_exist = 1;
	}
#endif
	if(bat_exist <= 0)
	{
		tick_printf("no battery exist\n");
		if(bat_init)
		{
			battery_charge_cartoon_exit();
		}
		return 0;
	}
	if(!bat_init)
	{
		bat_cal = axp_probe_rest_battery_capacity();
		printf("bat not inited\n");
		if(battery_charge_cartoon_init(bat_cal/(100/(SUNXI_BAT_BMP_MAX-1))) < 0)
		{
			tick_printf("init charge cartoon fail\n");

			return -1;
		}
	}
	if((!bat_cal) && (standby_mode))
	{
		bat_cal = 100;
	}

	return bat_cal;
}

static int board_standby_status(int source_bat_cal)
{
	int   bat_cal, this_bat_cal;
	int   i, j, status;
	int   one_delay;
	int   ret;

	boot_standby_action = 0;
	this_bat_cal = source_bat_cal;
	tick_printf("base bat_cal = %d\n", this_bat_cal);
	if(this_bat_cal > 95)
	{
		this_bat_cal = 100;
	}
	//�����жϼ��
	usb_detect_for_charge(BOOT_USB_DETECT_DELAY_TIME + 200);
	//����axp���
	power_limit_detect_enter();
	status = 1;
	goto __start_case_status__;
/******************************************************************
*
*	standby ����ֵ˵��
*
*	   -1: ����standbyʧ��
*		1: ��ͨ��������
*		2: ��Դ�����̰�����
*		3: ��Դ������������
*		4: �ⲿ��Դ�Ƴ�����
*		5: ��س�����
*		6: �ڻ���״̬���ⲿ��Դ���Ƴ�
*		7: �ڻ���״̬�³�����
*
******************************************************************/
	do
	{
		tick_printf("enter standby\n");
		board_display_layer_close();
		power_limit_detect_exit();
		status = board_try_boot_standby();
		tick_printf("exit standby by %d\n", status);

		bat_cal = axp_probe_rest_battery_capacity();
		tick_printf("current bat_cal = %d\n", bat_cal);
		if(bat_cal > this_bat_cal)
		{
			this_bat_cal = bat_cal;
		}
__start_case_status__:
		tick_printf("status = %d\n", status);
		switch(status)
		{
			case 2:		//�̰�power�������»���
				//�����жϼ��
				boot_standby_action = 0;
				power_limit_detect_enter();
				board_display_layer_open();
			case 1:
				//���¼��㶯����ʱʱ��
				if(this_bat_cal == 100)
				{
					one_delay = 1000;
				}
				else
				{
					one_delay = 1000/(10 - (this_bat_cal/10));
				}
				//���ƶ���
				for(j=0;j<3;j++)
				{
					for(i=this_bat_cal/(100/(SUNXI_BAT_BMP_MAX-1));i<SUNXI_BAT_BMP_MAX;i++)
					{
						battery_charge_cartoon_rate(i);
						if(boot_standby_action & 0x08)		//�����ⲿ��Դ
						{
							boot_standby_action &= ~0x08;
							j = 0;
						}
						else if(boot_standby_action & 0x02)	//�̰�
						{
							boot_standby_action &= ~2;
							j = 0;
						}
						else if(boot_standby_action & 0x01) //����
						{
							battery_charge_cartoon_exit();
							power_limit_detect_exit();

							return 0;
						}
						else if(boot_standby_action & 0x10) //�ε��ⲿ��Դ��û���ⲿ��Դ
						{
							status = 10;
							boot_standby_action &= ~0x10;

							goto __start_case_status__;
						}
						__msdelay(one_delay);
					}
				}
				//ֹͣ�������̶���ʾ��ǰ����
				battery_charge_cartoon_rate(this_bat_cal/(100/(SUNXI_BAT_BMP_MAX-1)));
				for(j=0;j<4;j++)
				{
					if(boot_standby_action & 0x08)		//�����ⲿ��Դ
					{
						boot_standby_action &= ~0x08;
						j = 0;
					}
					else if(boot_standby_action & 0x10) //�ε��ⲿ��Դ��û���ⲿ��Դ
					{
						status = 10;
						boot_standby_action &= ~0x10;

						goto __start_case_status__;
					}
					else if(boot_standby_action & 0x01) //����
					{
						battery_charge_cartoon_exit();
						power_limit_detect_exit();

						return 0;
					}
					__msdelay(250);
				}
				break;

			case 3:		//������Դ����֮�󣬹رյ��ͼ�꣬����ϵͳ
				battery_charge_cartoon_exit();

				return 0;

			case 4:		//���Ƴ��ⲿ��Դʱ��������ʾ��ǰ���ͼ���3���ػ�
			case 5:		//����س����ɵ�ʱ����Ҫ�ػ�
				//�����жϼ��
				boot_standby_action = 0;
				power_limit_detect_enter();

				board_display_layer_open();
				battery_charge_cartoon_rate(this_bat_cal/(100/(SUNXI_BAT_BMP_MAX-1)));
			case 6:
			case 7:
				if((status != 4) && (status != 5))
				{
					board_display_layer_open();
					battery_charge_cartoon_rate(this_bat_cal/(100/(SUNXI_BAT_BMP_MAX-1)));
				}
			case 10:
				battery_charge_cartoon_rate(this_bat_cal/(100/(SUNXI_BAT_BMP_MAX-1)));
				__msdelay(500);
				do
				{
					if(!(boot_standby_action & 0x04))
					{
						ret = battery_charge_cartoon_degrade(5);
					}
					else
					{
						status = 1;
						battery_charge_cartoon_reset();

						goto __start_case_status__;
					}
				}
				while(!ret);

				battery_charge_cartoon_exit();

				power_limit_detect_exit();

				return -1;

			case 8:		//standby�����м�⵽vbus���ڱ仯
			{
				usb_detect_for_charge(BOOT_USB_DETECT_DELAY_TIME + 200);
			}
			break;

			case 9:		//standby�����м�⵽vbus�Ƴ���ͬʱ������ͨdc
			{
//					power_set_usbpc();
			}
			break;

			default:
				break;
		}
	}
	while(1);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :  standby_mode: 0, ��ͨģʽ����Ҫ����Դ״̬
*
*					                1, ����ģʽ��ǿ�ƽ���standbyģʽ�����۵�Դ״̬
*
*    return        :
*
*    note          :  probe power and other condition
*
*
*
************************************************************************************************************
*/
void board_status_probe(int standby_mode)
{
	int ret;

	//���power����
	axp_probe_key();
	//���������жϣ���һ�׶Σ�����Դ��ѹ״̬
	if(!standby_mode)
	{
		ret = board_probe_power_level();	//�������ػ���0������ϵͳ������������
		debug("stage1 resule %d\n", ret);
		if(ret < 0)
		{
			do_shutdown(NULL, 0, 1, NULL);
		}
		else if(!ret)
		{
			return ;
		}
		//���������жϣ��ڶ��׶Σ���⿪��ԭ��
		ret = board_probe_poweron_cause();		//������0������ϵͳ����������������ֱ�ӹػ�
		debug("stage2 resule %d\n", ret);
		if(ret <= 0)
		{
			return ;
		}
		else if(ret == AXP_VBUS_DCIN_NOT_EXIST) //��ǰһ��Ϊboot standby״̬����������ʱ������ⲿ��Դ��ֱ�ӹػ�
		{
			do_shutdown(NULL, 0, 1, NULL);
		}
	}
	//���������жϣ������׶Σ�����ش���
	//�������ػ���0������ϵͳ������������
	ret = board_probe_bat_status(standby_mode);
	debug("stage3 resule %d\n", ret);
	if(ret < 0)
	{
		do_shutdown(NULL, 0, 1, NULL);
	}
	else if(!ret)
	{
		return ;
	}
	//���������жϣ����Ľ׶Σ�����boot����
	//�������ػ�������������ϵͳ
	ret = board_standby_status(ret);
	debug("stage4 resule %d\n", ret);
	if(ret < 0)
	{
		do_shutdown(NULL, 0, 1, NULL);
	}

	return ;
}


