#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/module.h>

#include "axp-cfg.h"
#include <mach/sys_config.h>

int		pmu_used;                  
int		pmu_twi_addr;              
int		pmu_twi_id;                
int		pmu_irq_id;                
int		pmu_battery_rdc;           
int		pmu_battery_cap;           
int		pmu_batdeten;							
int		pmu_runtime_chgcur;        
int		pmu_earlysuspend_chgcur;   
int		pmu_suspend_chgcur;        
int		pmu_shutdown_chgcur;       
int		pmu_init_chgvol;           
int		pmu_init_chgend_rate;      
int		pmu_init_chg_enabled;			
int		pmu_init_adc_freq;         
int		pmu_init_adcts_freq;       
int		pmu_init_chg_pretime;      
int		pmu_init_chg_csttime;
int		pmu_batt_cap_correct;
int		pmu_bat_regu_en;

                          
int		pmu_bat_para1;             
int		pmu_bat_para2;            
int		pmu_bat_para3;            
int		pmu_bat_para4;            
int		pmu_bat_para5;           
int		pmu_bat_para6;           
int		pmu_bat_para7;           
int		pmu_bat_para8;           
int		pmu_bat_para9;           
int		pmu_bat_para10;         
int		pmu_bat_para11;          
int		pmu_bat_para12;          
int		pmu_bat_para13;          
int		pmu_bat_para14;          
int		pmu_bat_para15;          
int		pmu_bat_para16;          
int		pmu_bat_para17;          
int		pmu_bat_para18;          
int		pmu_bat_para19;          
int		pmu_bat_para20;          
int		pmu_bat_para21;          
int		pmu_bat_para22;          
int		pmu_bat_para23;         
int		pmu_bat_para24;          
int		pmu_bat_para25;          
int		pmu_bat_para26;          
int		pmu_bat_para27;          
int		pmu_bat_para28;          
int		pmu_bat_para29;          
int		pmu_bat_para30;          
int		pmu_bat_para31;          
int		pmu_bat_para32;          
	                          
int		pmu_usbvol_limit;       
int		pmu_usbcur_limit;        
int		pmu_usbvol;        
int		pmu_usbcur;              
int		pmu_usbvol_pc;            
int		pmu_usbcur_pc;           
int		pmu_pwroff_vol;           
int		pmu_pwron_vol;          
int		dcdc1_vol;           
int		dcdc2_vol;               
int		dcdc3_vol;               
int		dcdc4_vol;               
int		dcdc5_vol;               
int		aldo2_vol;               
int		aldo3_vol;               
int		pmu_pekoff_time;
int		pmu_pekoff_en;          
int		pmu_pekoff_func;          
int		pmu_peklong_time;         
int		pmu_pekon_time;        
int		pmu_pwrok_time;          
int		pmu_battery_warning_level1;
int		pmu_battery_warning_level2;
int		pmu_restvol_adjust_time;
int		pmu_ocv_cou_adjust_time; 
int		pmu_chgled_func; 
int		pmu_chgled_type;         
int		pmu_vbusen_func;			  
int		pmu_reset;					
int		pmu_IRQ_wakeup;					
int		pmu_hot_shutdowm;				
int		pmu_inshort;
int		power_start;

int     pmu_temp_protect_en;
int		pmu_charge_ltf;
int		pmu_charge_htf;
int		pmu_discharge_ltf;
int		pmu_discharge_htf;
int		pmu_temp_para1;
int		pmu_temp_para2;
int		pmu_temp_para3;
int		pmu_temp_para4;
int		pmu_temp_para5;
int		pmu_temp_para6;
int		pmu_temp_para7;
int		pmu_temp_para8;
int		pmu_temp_para9;
int		pmu_temp_para10;
int		pmu_temp_para11;
int		pmu_temp_para12;
int		pmu_temp_para13;
int		pmu_temp_para14;
int		pmu_temp_para15;
int		pmu_temp_para16;

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{

	vcc_ldo1,
	vcc_ldo2,
	vcc_ldo3,
	vcc_ldo4,
	vcc_ldo5,
	vcc_ldo6,
	vcc_ldo7,
	vcc_ldo8,
	vcc_ldo9,
	vcc_ldo10,
	vcc_ldo11,
	vcc_ldo12,
	
	vcc_DCDC1,
	vcc_DCDC2,
	vcc_DCDC3,
	vcc_DCDC4,
	vcc_DCDC5,
	vcc_ldoio0,
	vcc_ldoio1,
};

/* The values of the various regulator constraints are obviously dependent
 * on exactly what is wired to each ldo.  Unfortunately this information is
 * not generally available.  More information has been requested from Xbow
 * but as of yet they haven't been forthcoming.
 *
 * Some of these are clearly Stargate 2 related (no way of plugging
 * in an lcd on the IM2 for example!).
 */

static struct regulator_consumer_supply ldo1_data[] = {
		{
			.supply = "axp22_rtc",
		},
	};


static struct regulator_consumer_supply ldo2_data[] = {
		{
			.supply = "axp22_aldo1",
		},
	};

static struct regulator_consumer_supply ldo3_data[] = {
		{
			.supply = "axp22_aldo2",
		},
	};

static struct regulator_consumer_supply ldo4_data[] = {
		{
			.supply = "axp22_aldo3",
		},
	};

static struct regulator_consumer_supply ldo5_data[] = {
		{
			.supply = "axp22_dldo1",
		},
	};


static struct regulator_consumer_supply ldo6_data[] = {
		{
			.supply = "axp22_dldo2",
		},
	};

static struct regulator_consumer_supply ldo7_data[] = {
		{
			.supply = "axp22_dldo3",
		},
	};

static struct regulator_consumer_supply ldo8_data[] = {
		{
			.supply = "axp22_dldo4",
		},
	};

static struct regulator_consumer_supply ldo9_data[] = {
		{
			.supply = "axp22_eldo1",
		},
	};


static struct regulator_consumer_supply ldo10_data[] = {
		{
			.supply = "axp22_eldo2",
		},
	};

static struct regulator_consumer_supply ldo11_data[] = {
		{
			.supply = "axp22_eldo3",
		},
	};

static struct regulator_consumer_supply ldo12_data[] = {
		{
			.supply = "axp22_dc5ldo",
		},
	};
	
static struct regulator_consumer_supply ldoio0_data[] = {
		{
			.supply = "axp22_ldoio0",
		},
	};

static struct regulator_consumer_supply ldoio1_data[] = {
		{
			.supply = "axp22_ldoio1",
		},
	};

static struct regulator_consumer_supply DCDC1_data[] = {
		{
			.supply = "axp22_dcdc1",
		},
	};

static struct regulator_consumer_supply DCDC2_data[] = {
		{
			.supply = "axp22_dcdc2",
		},
	};

static struct regulator_consumer_supply DCDC3_data[] = {
		{
			.supply = "axp22_dcdc3",
		},
	};

static struct regulator_consumer_supply DCDC4_data[] = {
		{
			.supply = "axp22_dcdc4",
		},
	};

static struct regulator_consumer_supply DCDC5_data[] = {
		{
			.supply = "axp22_dcdc5",
		},
	};


static struct regulator_init_data axp_regl_init_data[] = {
	[vcc_ldo1] = {
		.constraints = { 
			.name = "axp22_ldo1",
			.min_uV =  AXP22LDO1 * 1000,
			.max_uV =  AXP22LDO1 * 1000,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},
	[vcc_ldo2] = {
		.constraints = { 
			.name = "axp22_ldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo2_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},
	[vcc_ldo3] = {
		.constraints = {
			.name = "axp22_ldo3",
			.min_uV =  700000,
			.max_uV =  3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo3_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},
	[vcc_ldo4] = {
		.constraints = {
			.name = "axp22_ldo4",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo4_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},
	[vcc_ldo5] = {
		.constraints = { 
			.name = "axp22_ldo5",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo5_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo5_data),
		.consumer_supplies = ldo5_data,
	},
	[vcc_ldo6] = {
		.constraints = { 
			.name = "axp22_ldo6",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo6_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo6_data),
		.consumer_supplies = ldo6_data,
	},
	[vcc_ldo7] = {
		.constraints = {
			.name = "axp22_ldo7",
			.min_uV =  700000,
			.max_uV =  3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo7_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo7_data),
		.consumer_supplies = ldo7_data,
	},
	[vcc_ldo8] = {
		.constraints = {
			.name = "axp22_ldo8",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo8_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo8_data),
		.consumer_supplies = ldo8_data,
	},
	[vcc_ldo9] = {
		.constraints = { 
			.name = "axp22_ldo9",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo9_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo9_data),
		.consumer_supplies = ldo9_data,
	},
	[vcc_ldo10] = {
		.constraints = {
			.name = "axp22_ldo10",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo10_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo10_data),
		.consumer_supplies = ldo10_data,
	},
	[vcc_ldo11] = {
		.constraints = {
			.name = "axp22_ldo11",
			.min_uV =  700000,
			.max_uV =  3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo11_vol * 1000,
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo11_data),
		.consumer_supplies = ldo11_data,
	},
	[vcc_ldo12] = {
		.constraints = {
			.name = "axp22_ldo12",
			.min_uV = 700000,
			.max_uV = 1400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo12_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo12_data),
		.consumer_supplies = ldo12_data,
	},
	[vcc_DCDC1] = {
		.constraints = {
			.name = "axp22_DCDC1",
			.min_uV = 1600000,
			.max_uV = 3400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = dcdc1_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC1_data),
		.consumer_supplies = DCDC1_data,
	},
	[vcc_DCDC2] = {
		.constraints = {
			.name = "axp22_DCDC2",
			.min_uV = 600000,
			.max_uV = 1540000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = dcdc3_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC2_data),
		.consumer_supplies = DCDC2_data,
	},
	[vcc_DCDC3] = {
		.constraints = { 
			.name = "axp22_DCDC3",
			.min_uV = 600000,
			.max_uV = 1860000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = dcdc3_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC3_data),
		.consumer_supplies = DCDC3_data,
	},
	[vcc_DCDC4] = {
		.constraints = { 
			.name = "axp22_DCDC4",
			.min_uV = 600000,
			.max_uV = 1540000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = dcdc4_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC4_data),
		.consumer_supplies = DCDC4_data,
	},
	[vcc_DCDC5] = {
		.constraints = { 
			.name = "axp22_DCDC5",
			.min_uV = 1000000,
			.max_uV = 2550000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = dcdc5_vol * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC5_data),
		.consumer_supplies = DCDC5_data,
	},
	[vcc_ldoio0] = {
		.constraints = {
			.name = "axp22_ldoio0",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio0_data),
		.consumer_supplies = ldoio0_data,
	},
	[vcc_ldoio1] = {
		.constraints = {
			.name = "axp22_ldoio1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio1_data),
		.consumer_supplies = ldoio1_data,
	},
};

static struct axp_funcdev_info axp_regldevs[] = {
	{
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO1,
		.platform_data = &axp_regl_init_data[vcc_ldo1],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO2,
		.platform_data = &axp_regl_init_data[vcc_ldo2],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO3,
		.platform_data = &axp_regl_init_data[vcc_ldo3],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO4,
		.platform_data = &axp_regl_init_data[vcc_ldo4],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO5,
		.platform_data = &axp_regl_init_data[vcc_ldo5],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO6,
		.platform_data = &axp_regl_init_data[vcc_ldo6],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO7,
		.platform_data = &axp_regl_init_data[vcc_ldo7],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO8,
		.platform_data = &axp_regl_init_data[vcc_ldo8],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO9,
		.platform_data = &axp_regl_init_data[vcc_ldo9],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO10,
		.platform_data = &axp_regl_init_data[vcc_ldo10],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO11,
		.platform_data = &axp_regl_init_data[vcc_ldo11],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO12,
		.platform_data = &axp_regl_init_data[vcc_ldo12],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC1,
		.platform_data = &axp_regl_init_data[vcc_DCDC1],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC2,
		.platform_data = &axp_regl_init_data[vcc_DCDC2],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC3,
		.platform_data = &axp_regl_init_data[vcc_DCDC3],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC4,
		.platform_data = &axp_regl_init_data[vcc_DCDC4],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC5,
		.platform_data = &axp_regl_init_data[vcc_DCDC5],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDOIO0,
		.platform_data = &axp_regl_init_data[vcc_ldoio0],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDOIO1,
		.platform_data = &axp_regl_init_data[vcc_ldoio1],
	},
};

static struct power_supply_info battery_data ={
		.name ="PTI PL336078",
		.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
		//.voltage_max_design = pmu_init_chgvol,
		//.voltage_min_design = pmu_pwroff_vol,
		//.energy_full_design = pmu_battery_cap,
		.use_for_apm = 1,
};


static struct axp_supply_init_data axp_sply_init_data = {
	.battery_info = &battery_data,
	.chgcur = 1500000,
	.chgvol = 4200000,
	.chgend = 10,
	.chgen = 1,
	.sample_time = 800,
	.chgpretime = 50,
	.chgcsttime = 720,
};

static struct axp_funcdev_info axp_splydev[]={
   	{
   		.name = "axp22-supplyer",
		.id = AXP22_ID_SUPPLY,
      .platform_data = &axp_sply_init_data,
    },
};

static struct axp_funcdev_info axp_gpiodev[]={
   	{  
		.name = "axp22-gpio",
   		.id = AXP22_ID_GPIO,
    },
};

static struct axp_platform_data axp_pdata = {
	.num_regl_devs = ARRAY_SIZE(axp_regldevs),
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
	.num_gpio_devs = ARRAY_SIZE(axp_gpiodev),
	.regl_devs = axp_regldevs,
	.sply_devs = axp_splydev,
	.gpio_devs = axp_gpiodev,
	.gpio_base = 0,
};

static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp22_mfd",
		//.addr = 0x34,
		.platform_data = &axp_pdata,
		//.irq = pmu_irq_id,
	},
};

int axp_script_parser_fetch(char *main, char *sub, u32 *val, u32 size)
{
		script_item_u script_val;
		script_item_value_type_e type;
		type = script_get_item(main, sub, &script_val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("type err!");
		}
		*val = script_val.val;
//		printk("axp config [%s] [%s] : %d\n", main, sub, *val);
		return 0;
}


static int __init axp22_board_init(void)
{
	int ret;
	
    ret = axp_script_parser_fetch("pmu_para", "pmu_used", &pmu_used, sizeof(int));
    if (ret)
    {
 //       printk("axp driver uning configuration failed(%d)\n", __LINE__);
        return -1;
    }
    if (pmu_used)
    {
        ret = axp_script_parser_fetch("pmu_para", "pmu_twi_id", &pmu_twi_id, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_twi_id = 0;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_twi_addr", &pmu_twi_addr, sizeof(int));
        if (ret)
        {
  //          printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_twi_addr = 34;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_battery_rdc", &pmu_battery_rdc, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_battery_rdc = BATRDC;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_battery_cap", &pmu_battery_cap, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_battery_cap = 4000;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_batdeten", &pmu_batdeten, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_batdeten = 1;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_runtime_chgcur", &pmu_runtime_chgcur, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_runtime_chgcur = INTCHGCUR / 1000;
        }
        pmu_runtime_chgcur = pmu_runtime_chgcur * 1000;
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_earlysuspend_chgcur", &pmu_earlysuspend_chgcur, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_earlysuspend_chgcur = 500;
        }
        pmu_earlysuspend_chgcur = pmu_earlysuspend_chgcur * 1000,
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_suspend_chgcur", &pmu_suspend_chgcur, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_suspend_chgcur = 1200;
        }
        pmu_suspend_chgcur = pmu_suspend_chgcur * 1000;
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_shutdown_chgcur", &pmu_shutdown_chgcur, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_shutdown_chgcur = 1200;
        }
				pmu_shutdown_chgcur = pmu_shutdown_chgcur *1000;
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_init_chgvol", &pmu_init_chgvol, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_init_chgvol = INTCHGVOL / 1000;
        }
        pmu_init_chgvol = pmu_init_chgvol * 1000;
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_init_chgend_rate", &pmu_init_chgend_rate, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_init_chgend_rate = INTCHGENDRATE;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_init_chg_enabled", &pmu_init_chg_enabled, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_init_chg_enabled = 1;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_init_adc_freq", &pmu_init_adc_freq, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_init_adc_freq = INTADCFREQ;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_init_adcts_freq", &pmu_init_adcts_freq, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_init_adcts_freq = INTADCFREQC;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_init_chg_pretime", &pmu_init_chg_pretime, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_init_chg_pretime = INTCHGPRETIME;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_init_chg_csttime", &pmu_init_chg_csttime, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_init_chg_csttime = INTCHGCSTTIME;
        }

		ret = axp_script_parser_fetch("pmu_para", "pmu_batt_cap_correct", &pmu_batt_cap_correct, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_batt_cap_correct = 1;
        }

	ret = axp_script_parser_fetch("pmu_para", "pmu_bat_regu_en", &pmu_bat_regu_en, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_regu_en = 0;
        }	

        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para1", &pmu_bat_para1, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para1 = OCVREG0;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para2", &pmu_bat_para2, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para2 = OCVREG1;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para3", &pmu_bat_para3, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para3 = OCVREG2;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para4", &pmu_bat_para4, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para4 = OCVREG3;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para5", &pmu_bat_para5, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para5 = OCVREG4;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para6", &pmu_bat_para6, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para6 = OCVREG5;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para7", &pmu_bat_para7, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para7 = OCVREG6;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para8", &pmu_bat_para8, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para8 = OCVREG7;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para9", &pmu_bat_para9, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para9 = OCVREG8;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para10", &pmu_bat_para10, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para10 = OCVREG9;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para11", &pmu_bat_para11, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para11 = OCVREGA;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para12", &pmu_bat_para12, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para12 = OCVREGB;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para13", &pmu_bat_para13, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para13 = OCVREGC;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para14", &pmu_bat_para14, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para14 = OCVREGD;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para15", &pmu_bat_para15, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para15 = OCVREGE;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para16", &pmu_bat_para16, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para16 = OCVREGF;
        }
        //Add 32 Level OCV para 20121128 by evan		
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para17", &pmu_bat_para17, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para17 = OCVREG10;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para18", &pmu_bat_para18, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para18 = OCVREG11;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para19", &pmu_bat_para19, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para19 = OCVREG12;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para20", &pmu_bat_para20, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para20 = OCVREG13;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para21", &pmu_bat_para21, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para21 = OCVREG14;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para22", &pmu_bat_para22, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para22 = OCVREG15;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para23", &pmu_bat_para23, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para23 = OCVREG16;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para24", &pmu_bat_para24, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para24 = OCVREG17;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para25", &pmu_bat_para25, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para25 = OCVREG18;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para26", &pmu_bat_para26, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para26 = OCVREG19;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para27", &pmu_bat_para27, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para27 = OCVREG1A;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para28", &pmu_bat_para28, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para28 = OCVREG1B;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para29", &pmu_bat_para29, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para29 = OCVREG1C;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para30", &pmu_bat_para30, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para30 = OCVREG1D;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para31", &pmu_bat_para31, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para31 = OCVREG1E;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_bat_para32", &pmu_bat_para32, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_bat_para32 = OCVREG1F;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_usbvol_limit", &pmu_usbvol_limit, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_usbvol_limit = 1;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_usbvol", &pmu_usbvol, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_usbvol = 4400;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_usbvol_pc", &pmu_usbvol_pc, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_usbvol_pc = 4400;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_usbcur_limit", &pmu_usbcur_limit, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_usbcur_limit = 1;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_usbcur", &pmu_usbcur, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_usbcur = 0;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_usbcur_pc", &pmu_usbcur_pc, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_usbcur_pc = 0;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_pwroff_vol", &pmu_pwroff_vol, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_pwroff_vol = 3300;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_pwron_vol", &pmu_pwron_vol, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_pwron_vol = 2900;
        }
        
				ret = axp_script_parser_fetch("power_sply", "dcdc1_vol", &dcdc1_vol, sizeof(int));
				if (ret)
				{
//					printk("axp driver uning configuration failed(%d)\n", __LINE__);
					dcdc1_vol = 3000;
				}
				ret = axp_script_parser_fetch("power_sply", "dcdc2_vol", &dcdc2_vol, sizeof(int));
				if (ret)
				{
//						printk("axp driver uning configuration failed(%d)\n", __LINE__);
						dcdc2_vol = 1100;
				}
        ret = axp_script_parser_fetch("power_sply", "dcdc3_vol", &dcdc3_vol, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            dcdc3_vol = 1200;
        }
        ret = axp_script_parser_fetch("power_sply", "dcdc4_vol", &dcdc4_vol, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            dcdc4_vol = 1200;
        }
				ret = axp_script_parser_fetch("power_sply", "dcdc5_vol", &dcdc5_vol, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            dcdc5_vol = 1500;
        }
    
        ret = axp_script_parser_fetch("power_sply", "aldo2_vol", &aldo2_vol, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            aldo2_vol = 1800;
        }
        ret = axp_script_parser_fetch("power_sply", "aldo3_vol", &aldo3_vol, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            aldo3_vol = 3000;
        }
       
				ret = axp_script_parser_fetch("pmu_para", "pmu_pekoff_time", &pmu_pekoff_time, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_pekoff_time = 6000;
        }
        //offlevel restart or not 0:not restart 1:restart
        ret = axp_script_parser_fetch("pmu_para", "pmu_pekoff_func", &pmu_pekoff_func, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_pekoff_func   = 0;
        }
        //16's power restart or not 0:not restart 1:restart
        ret = axp_script_parser_fetch("pmu_para", "pmu_pekoff_en", &pmu_pekoff_en, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_pekoff_en   = 1;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_peklong_time", &pmu_peklong_time, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_peklong_time = 1500;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_pwrok_time", &pmu_pwrok_time, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
           pmu_pwrok_time    = 64;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_pekon_time", &pmu_pekon_time, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_pekon_time = 1000;
        }           
        ret = axp_script_parser_fetch("pmu_para", "pmu_battery_warning_level1", &pmu_battery_warning_level1, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_battery_warning_level1 = 15;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_battery_warning_level2", &pmu_battery_warning_level2, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_battery_warning_level2 = 0;
        }      
        ret = axp_script_parser_fetch("pmu_para", "pmu_restvol_adjust_time", &pmu_restvol_adjust_time, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_restvol_adjust_time = 30;
        }  
        ret = axp_script_parser_fetch("pmu_para", "pmu_ocv_cou_adjust_time", &pmu_ocv_cou_adjust_time, sizeof(int));
        if (ret)
        {
  //          printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_ocv_cou_adjust_time = 60;
        } 
        ret = axp_script_parser_fetch("pmu_para", "pmu_chgled_func", &pmu_chgled_func, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_chgled_func = 0;
        } 
        ret = axp_script_parser_fetch("pmu_para", "pmu_chgled_type", &pmu_chgled_type, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_chgled_type = 0;
        }
        ret = axp_script_parser_fetch("pmu_para", "pmu_vbusen_func", &pmu_vbusen_func, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_vbusen_func = 1;
        } 
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_reset", &pmu_reset, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_reset = 0;
        } 
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_IRQ_wakeup", &pmu_IRQ_wakeup, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_IRQ_wakeup = 0;
        } 
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_hot_shutdowm", &pmu_hot_shutdowm, sizeof(int));
        if (ret)
        {
 //           printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_hot_shutdowm = 1;
        } 
        
        ret = axp_script_parser_fetch("pmu_para", "pmu_inshort", &pmu_inshort, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            pmu_inshort = 0;
        }
        
        ret = axp_script_parser_fetch("pmu_para", "power_start", &power_start, sizeof(int));
        if (ret)
        {
//            printk("axp driver uning configuration failed(%d)\n", __LINE__);
            power_start = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_protect_en", &pmu_temp_protect_en, sizeof(int));
        if (ret)
        {
            pmu_temp_protect_en = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_charge_ltf", &pmu_charge_ltf, sizeof(int));
        if (ret)
        {
            pmu_charge_ltf = 0xA5;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_charge_htf", &pmu_charge_htf, sizeof(int));
        if (ret)
        {
            pmu_charge_htf = 0x1F;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_discharge_ltf", &pmu_discharge_ltf, sizeof(int));
        if (ret)
        {
            pmu_discharge_ltf = 0xFC;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_discharge_htf", &pmu_discharge_htf, sizeof(int));
        if (ret)
        {
            pmu_discharge_htf = 0x16;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para1", &pmu_temp_para1, sizeof(int));
        if (ret)
        {
            pmu_temp_para1 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para2", &pmu_temp_para2, sizeof(int));
        if (ret)
        {
            pmu_temp_para2 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para3", &pmu_temp_para3, sizeof(int));
        if (ret)
        {
            pmu_temp_para3 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para4", &pmu_temp_para4, sizeof(int));
        if (ret)
        {
            pmu_temp_para4 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para5", &pmu_temp_para5, sizeof(int));
        if (ret)
        {
            pmu_temp_para5 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para6", &pmu_temp_para6, sizeof(int));
        if (ret)
        {
            pmu_temp_para6 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para7", &pmu_temp_para7, sizeof(int));
        if (ret)
        {
            pmu_temp_para7 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para8", &pmu_temp_para8, sizeof(int));
        if (ret)
        {
            pmu_temp_para8 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para9", &pmu_temp_para9, sizeof(int));
        if (ret)
        {
            pmu_temp_para9 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para10", &pmu_temp_para10, sizeof(int));
        if (ret)
        {
            pmu_temp_para10 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para11", &pmu_temp_para11, sizeof(int));
        if (ret)
        {
            pmu_temp_para11 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para12", &pmu_temp_para12, sizeof(int));
        if (ret)
        {
            pmu_temp_para12 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para13", &pmu_temp_para13, sizeof(int));
        if (ret)
        {
            pmu_temp_para13 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para14", &pmu_temp_para14, sizeof(int));
        if (ret)
        {
            pmu_temp_para14 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para15", &pmu_temp_para15, sizeof(int));
        if (ret)
        {
            pmu_temp_para15 = 0;
        }

        ret = axp_script_parser_fetch("pmu_para", "pmu_temp_para16", &pmu_temp_para16, sizeof(int));
        if (ret)
        {
            pmu_temp_para16 = 0;
        }
        
        axp_regl_init_data[1].constraints.state_standby.uV = 700 * 1000;
        axp_regl_init_data[2].constraints.state_standby.uV = aldo2_vol * 1000;
        axp_regl_init_data[3].constraints.state_standby.uV = aldo3_vol * 1000;
        axp_regl_init_data[5].constraints.state_standby.uV = 700 * 1000;
        axp_regl_init_data[6].constraints.state_standby.uV = 700 * 1000;
        axp_regl_init_data[7].constraints.state_standby.uV = 700 * 1000;
        axp_regl_init_data[8].constraints.state_standby.uV = 700 * 1000;
        axp_regl_init_data[9].constraints.state_standby.uV = 700 * 1000;
        axp_regl_init_data[10].constraints.state_standby.uV = 700 * 1000;
        axp_regl_init_data[11].constraints.state_standby.uV = 1100 * 1000;
        axp_regl_init_data[12].constraints.state_standby.uV = dcdc1_vol * 1000;
        axp_regl_init_data[13].constraints.state_standby.uV = dcdc2_vol * 1000;
        axp_regl_init_data[14].constraints.state_standby.uV = dcdc3_vol * 1000;
        axp_regl_init_data[15].constraints.state_standby.uV = dcdc4_vol * 1000;
        axp_regl_init_data[16].constraints.state_standby.uV = dcdc5_vol * 1000;
        axp_regl_init_data[2].constraints.state_standby.enabled = (aldo2_vol)?1:0;
        axp_regl_init_data[2].constraints.state_standby.disabled = (aldo2_vol)?0:1;
        axp_regl_init_data[3].constraints.state_standby.enabled = (aldo3_vol)?1:0;
        axp_regl_init_data[3].constraints.state_standby.disabled = (aldo3_vol)?0:1;
        axp_regl_init_data[12].constraints.state_standby.enabled = (dcdc1_vol)?1:0;
        axp_regl_init_data[12].constraints.state_standby.disabled = (dcdc1_vol)?0:1;
        axp_regl_init_data[13].constraints.state_standby.enabled = (dcdc2_vol)?1:0;
        axp_regl_init_data[13].constraints.state_standby.disabled = (dcdc2_vol)?0:1;
        axp_regl_init_data[14].constraints.state_standby.enabled = (dcdc3_vol)?1:0;
        axp_regl_init_data[14].constraints.state_standby.disabled = (dcdc3_vol)?0:1;
        axp_regl_init_data[15].constraints.state_standby.enabled = (dcdc4_vol)?1:0;
        axp_regl_init_data[15].constraints.state_standby.disabled = (dcdc4_vol)?0:1;
        axp_regl_init_data[16].constraints.state_standby.enabled = (dcdc5_vol)?1:0;
        axp_regl_init_data[16].constraints.state_standby.disabled = (dcdc5_vol)?0:1;
        battery_data.voltage_max_design = pmu_init_chgvol;
        battery_data.voltage_min_design = pmu_pwroff_vol;
        battery_data.energy_full_design = pmu_battery_cap;
        axp_sply_init_data.chgcur = pmu_runtime_chgcur;
        axp_sply_init_data.chgvol = pmu_init_chgvol;
        axp_sply_init_data.chgend = pmu_init_chgend_rate;
        axp_sply_init_data.chgen = pmu_init_chg_enabled;
        axp_sply_init_data.sample_time = pmu_init_adc_freq;
        axp_sply_init_data.chgpretime = pmu_init_chg_pretime;
        axp_sply_init_data.chgcsttime = pmu_init_chg_csttime;
        axp_mfd_i2c_board_info[0].addr = pmu_twi_addr;
//        axp_mfd_i2c_board_info[0].irq = pmu_irq_id;
          
        return i2c_register_board_info(1, axp_mfd_i2c_board_info, ARRAY_SIZE(axp_mfd_i2c_board_info));
	}
	else
	{	
        return -1;
    }
}
arch_initcall(axp22_board_init);

MODULE_DESCRIPTION("X-powers axp board");
MODULE_AUTHOR("Weijin Zhong");
MODULE_LICENSE("GPL");
