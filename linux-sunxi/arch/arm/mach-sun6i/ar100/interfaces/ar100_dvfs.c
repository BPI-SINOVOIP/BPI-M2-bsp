/*
 *  arch/arm/mach-sun6i/ar100/interface/ar100_dvfs.c
 *
 * Copyright (c) 2012 Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../ar100_i.h"

typedef struct ar100_freq_voltage
{
	u32 freq;       //cpu frequency
	u32 voltage;    //voltage for the frequency
	u32 axi_div;    //the divide ratio of axi bus
} ar100_freq_voltage_t;

//cpu voltage-freq table
static struct ar100_freq_voltage ar100_vf_table[AR100_DVFS_VF_TABLE_MAX] =
{
	//freq          //voltage   //axi_div
	{900000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
};

int ar100_dvfs_get_cfg(char *main, char *sub, u32 *val)
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		AR100_ERR("ar100 dvfs config type err!");
		return -EINVAL;
	}
	*val = script_val.val;
	AR100_INF("ar100 dvfs config [%s] [%s] : %d\n", main, sub, *val);
	return 0;
}

int ar100_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    result = 0;
	int    vf_table_size = 0;
	char   vf_table_main_key[64];
	char   vf_table_sub_key[64];
	struct ar100_message *pmessage;
	enum sw_ic_ver  ic_ver;

	ic_ver = sw_get_ic_ver();

	if (ic_ver == MAGIC_VER_D) {
		if (ar100_dvfs_get_cfg("ver_d_dvfs_table", "LV_count", &vf_table_size)) {
			strcpy(vf_table_main_key, "dvfs_table");
		} else {
			strcpy(vf_table_main_key, "ver_d_dvfs_table");
		}
	} else {
		strcpy(vf_table_main_key, "dvfs_table");
	}

	/* parse system config v-f table information */
	if (ar100_dvfs_get_cfg(vf_table_main_key, "LV_count", &vf_table_size)) {
		AR100_WRN("parse system config dvfs_table size fail\n");
	}
	for (index = 0; index < vf_table_size; index++) {
		sprintf(vf_table_sub_key, "LV%d_freq", index + 1);
		if (ar100_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			ar100_vf_table[index].freq = value;
		}
		sprintf(vf_table_sub_key, "LV%d_volt", index + 1);
		if (ar100_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			// if (value > 1400) {
				// /* cpu_vdd must < 1.4V */
				// AR100_WRN("v-f table voltage [%d] > 1400mV\n", value);
				// value = 1400;
			// }
			ar100_vf_table[index].voltage = value;
		}
	}

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	for (index = 0; index < AR100_DVFS_VF_TABLE_MAX; index++) {
		/* initialize message */
		pmessage->type       = AR100_CPUX_DVFS_CFG_VF_REQ;
		pmessage->paras[0]   = index;
		pmessage->paras[1]   = ar100_vf_table[index].freq;
		pmessage->paras[2]   = ar100_vf_table[index].voltage;
		pmessage->paras[3]   = ar100_vf_table[index].axi_div;
		pmessage->state      = AR100_MESSAGE_INITIALIZED;
		pmessage->cb.handler = NULL;
		pmessage->cb.arg     = NULL;

		AR100_INF("v-f table: index %d freq %d vol %d axi_div %d\n",
		pmessage->paras[0], pmessage->paras[1], pmessage->paras[2], pmessage->paras[3]);

		/* send request message */
		ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

		//check config fail or not
		if (pmessage->result) {
			AR100_WRN("config dvfs v-f table [%d] fail\n", index);
			result = -EINVAL;
			break;
		}
	}
	/* free allocated message */
	ar100_message_free(pmessage);

	return result;
}

/*
 * set target frequency.
 * @freq:    target frequency to be set, based on KHZ;
 * @mode:    the attribute of message, whether syn or asyn;
 * @cb:      callback handler;
 * @cb_arg:  callback handler arguments;
 *
 * return: result, 0 - set frequency successed,
 *                !0 - set frequency failed;
 */
int ar100_dvfs_set_cpufreq(unsigned int freq, unsigned long mode, ar100_cb_t cb, void *cb_arg)
{
	unsigned int          msg_attr = 0;
	struct ar100_message *pmessage;
	int                   result = 0;

	if (mode & AR100_DVFS_SYN) {
		msg_attr |= AR100_MESSAGE_ATTR_HARDSYN;
	}

	/* allocate a message frame */
	pmessage = ar100_message_allocate(msg_attr);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = AR100_CPUX_DVFS_REQ;
	pmessage->paras[0]   = freq;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;

	AR100_INF("ar100 dvfs request : %d\n", freq);
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* dvfs mode : syn or not */
	if (mode & AR100_DVFS_SYN) {
		result = pmessage->result;
		ar100_message_free(pmessage);
	}

	return result;
}
EXPORT_SYMBOL(ar100_dvfs_set_cpufreq);
