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
#include <asm/io.h>
#include <pmu.h>
#include <asm/arch/timer.h>
#include <asm/arch/key.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <boot_type.h>
#include <sys_partition.h>
#include <sys_config.h>
/* The sunxi internal brom will try to loader external bootloader
 * from mmc0, nannd flash, mmc2.
 * We check where we boot from by checking the config
 * of the gpio pin.
 */
DECLARE_GLOBAL_DATA_PTR;

u32 get_base(void)
{

	u32 val;

	__asm__ __volatile__("mov %0, pc \n":"=r"(val)::"memory");
	val &= 0xF0000000;
	val >>= 28;
	return val;
}

/* do some early init */
void s_init(void)
{
	watchdog_disable();
}

void reset_cpu(ulong addr)
{
	watchdog_enable();
#ifndef CONFIG_A50_FPGA
loop_to_die:
	goto loop_to_die;
#endif
}

void v7_outer_cache_enable(void)
{
	return ;
}

void v7_outer_cache_inval_all(void)
{
	return ;
}

void v7_outer_cache_flush_range(u32 start, u32 stop)
{
	return ;
}

void enable_caches(void)
{
    icache_enable();
    dcache_enable();
}

void disable_caches(void)
{
    icache_disable();
	dcache_disable();
}

int display_inner(void)
{
	tick_printf("version: %s\n", uboot_spare_head.boot_head.version);

	return 0;
}

int script_init(void)
{
    uint offset, length;
	char *addr;

	offset = uboot_spare_head.boot_head.uboot_length;
	length = uboot_spare_head.boot_head.length - uboot_spare_head.boot_head.uboot_length;
	addr   = (char *)CONFIG_SYS_TEXT_BASE + offset;

    debug("script offset=%x, length = %x\n", offset, length);

	if(length)
	{
		memcpy((void *)SYS_CONFIG_MEMBASE, addr, length);
		script_parser_init((char *)SYS_CONFIG_MEMBASE);
	}
	else
	{
		script_parser_init(NULL);
	}
#if defined(CONFIG_SUNXI_SCRIPT_REINIT)
	{
		void *tmp_target_buffer = (void *)(CONFIG_SYS_TEXT_BASE - 0x01000000);

		memset(tmp_target_buffer, 0, 1024 * 1024);
		memcpy(tmp_target_buffer, (void *)CONFIG_SYS_TEXT_BASE, uboot_spare_head.boot_head.length);
	}
#endif
	return 0;
}


static int get_dp_dm_status_normal(void)
{
	__u32 base_reg_val;
	__u32 reg_val = 0;
	__u32 dp, dm = 0;
	__u32 dpdm_det[6];
	__u32 dpdm_ret = 0;
	int   i =0 ;

	reg_val = readl(0X01C19400);
    base_reg_val = reg_val;

    reg_val |= (1 << 16) | (1 << 17);
    writel(reg_val,0X01C19400);
	printf("USBC_REG_ISCR = 0x%x\n",reg_val);

 	__msdelay(10);

	for(i=0;i<6;i++)
	{
		reg_val = readl(0X01C19400);
 	 	dp = (reg_val >> 26) & 0x01;
 	 	dm = (reg_val >> 27) & 0x01;

 	 	dpdm_det[i] = (dp << 1) | dm;
 	 	dpdm_ret += dpdm_det[i];

 	 	__msdelay(10);
	}
	writel(0X01C19400, base_reg_val);

  	if(dpdm_ret > 12)
  	{
  		return 1;			//DC
  	}
  	else
  	{
  		return 0;			//PC
  	}
}

void axp_judge_pc_charge(void)
{
	int battery_exist = 0;
	int dcin_exist = 0;
	int dp_dm_status = 0;
	int i = 0;;

	do
	{
		axp_power_get_dcin_battery_exist(&dcin_exist,&battery_exist);//判断电池是否存在
		printf("dcin_exist = %d , battery_exist = %d\n",dcin_exist,battery_exist);
		i ++;
		if(battery_exist >= 0)
		{
			break;
		}
	}
	while(i <4);

	if(i==4)
	{
		axp_set_vbus_limit_dc();
		printf("the battery not exist,axp_set_vbus_limit_dc!!!\n");

		return ;
	}

	if(battery_exist == BATTERY_EXIST)
	{
		printf("BATTERY_EXIST!!!!\n");
		if((dcin_exist == AXP_DCIN_EXIST) || (dcin_exist == AXP_VBUS_EXIST))
		{
			if(dcin_exist == AXP_DCIN_EXIST)
			{
				axp_set_vbus_limit_dc();
				printf("AXP_DCIN_EXIST,axp_set_vbus_limit_pc!!!!\n");
			}
			else
			{
				printf("AXP_VBUS_EXIST!!!\n");
				dp_dm_status = get_dp_dm_status_normal();

				if(dp_dm_status == 1)
				{
					axp_set_vbus_limit_dc();    //dp_dm 拉高
					printf("axp_set_vbus_limit_dc!!\n");
				}
				else
				{
					axp_set_vbus_limit_pc();
					printf("axp_set_vbus_limit_pc!!\n");
				}
				printf("dp_dm_status = %d !!!!\n",dp_dm_status);
			}
		}
		else
		{
			axp_set_vbus_limit_pc();
			printf("the dcin not exist,axp_set_vbus_limit_pc!!!\n");
		}
	}
	else							//若不存在，限流至dc
	{
		axp_set_vbus_limit_dc();
		printf("the battery not exist,axp_set_vbus_limit_dc!!!\n");
	}
}

int power_source_init(void)
{
	int pll1;
	int dcdc3_vol;

	if(script_parser_fetch("power_sply", "dcdc3_vol", &dcdc3_vol, 1))
	{
		dcdc3_vol = 1200;
	}
	if(axp_probe() > 0)
	{
		axp_probe_factory_mode();
		if(!axp_probe_power_supply_condition())
		{
			if(!axp_set_supply_status(0, PMU_SUPPLY_DCDC3, dcdc3_vol, -1))
			{
				tick_printf("PMU: dcdc3 %d\n", dcdc3_vol);
				sunxi_clock_set_corepll(uboot_spare_head.boot_data.run_clock, 0);
			}
			else
			{
				printf("axp_set_dcdc3 fail\n");
			}
		}
		else
		{
			printf("axp_probe_power_supply_condition error\n");
		}
	}
	else
	{
		printf("axp_probe error\n");
	}

	pll1 = sunxi_clock_get_corepll();

	tick_printf("PMU: pll1 %d Mhz\n", pll1);

    axp_set_charge_vol_limit();
    axp_set_all_limit();
    axp_set_hardware_poweron_vol();

	axp_set_power_supply_output();

	axp_judge_pc_charge();	//判断当前的外部电源情况

	return 0;
}



