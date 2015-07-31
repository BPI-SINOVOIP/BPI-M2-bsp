/*
 *  arch/arm/mach-sun6i/ar100/ar100_standby.c
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
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <mach/sys_config.h>

/* record super-standby wakeup event */
static unsigned long wakeup_event = 0;
static unsigned long dram_crc_error = 0;
static unsigned long dram_crc_total_count = 0;
static unsigned long dram_crc_error_count = 0;

extern unsigned int ar100_debug_dram_crc_en;

/**
 * enter super standby.
 * @para:  parameter for enter normal standby.
 *
 * return: result, 0 - super standby successed,
 *                !0 - super standby failed;
 */
int ar100_standby_super(struct super_standby_para *para, ar100_cb_t cb, void *cb_arg)
{
	struct ar100_message *pmessage;

	/* allocate a message frame */
	pmessage = ar100_message_allocate(0);
	if (pmessage == NULL) {
		AR100_ERR("allocate message for super-standby request failed\n");
		return -ENOMEM;
	}

	/* check super_standby_para size valid or not */
	if (sizeof(struct super_standby_para) > sizeof(pmessage->paras)) {
		AR100_ERR("super-standby parameters number too long\n");
		return -EINVAL;
	}
	/* initialize message */
	if (para->pextended_standby == NULL) {
		pmessage->type       = AR100_SSTANDBY_ENTER_REQ;
	} else {
		pmessage->type       = AR100_ESSTANDBY_ENTER_REQ;
	}
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;
	memcpy(pmessage->paras, para, sizeof(struct super_standby_para));
	pmessage->state      = AR100_MESSAGE_INITIALIZED;

	/* send enter super-standby request to ar100 */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	return 0;
}
EXPORT_SYMBOL(ar100_standby_super);


/**
 * query super-standby wakeup source.
 * @para:  point of buffer to store wakeup event informations.
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int ar100_query_wakeup_source(unsigned long *event)
{
	*event = wakeup_event;

	return 0;
}
EXPORT_SYMBOL(ar100_query_wakeup_source);

/**
 * query super-standby dram crc result.
 * @para:  point of buffer to store dram crc result informations.
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int ar100_query_dram_crc_result(unsigned long *perror, unsigned long *ptotal_count,
	unsigned long *perror_count)
{
	*perror = dram_crc_error;
	*ptotal_count = dram_crc_total_count;
	*perror_count = dram_crc_error_count;

	return 0;
}
EXPORT_SYMBOL(ar100_query_dram_crc_result);

int ar100_set_dram_crc_result(unsigned long error, unsigned long total_count,
	unsigned long error_count)
{
	dram_crc_error = error;
	dram_crc_total_count = total_count;
	dram_crc_error_count = error_count;

	return 0;
}
EXPORT_SYMBOL(ar100_set_dram_crc_result);

/**
 * notify ar100 cpux restored.
 * @para:  none.
 *
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int ar100_cpux_ready_notify(void)
{
	struct ar100_message *pmessage;

	/* notify hwspinlock and hwmsgbox resume first */
	ar100_hwmsgbox_standby_resume();
	ar100_hwspinlock_standby_resume();

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type     = AR100_SSTANDBY_RESTORE_NOTIFY;
	pmessage->state    = AR100_MESSAGE_INITIALIZED;

	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* record wakeup event */
	wakeup_event   = pmessage->paras[0];
	if (ar100_debug_dram_crc_en) {
		dram_crc_error = pmessage->paras[1];
		dram_crc_total_count++;
		dram_crc_error_count += (dram_crc_error ? 1 : 0);
	}

	/* free message */
	ar100_message_free(pmessage);

	return 0;
}
EXPORT_SYMBOL(ar100_cpux_ready_notify);

/**
 * enter talk standby.
 * @para:  parameter for enter talk standby.
 *
 * return: result, 0 - talk standby successed,
 *                !0 - talk standby failed;
 */
int ar100_standby_talk(struct super_standby_para *para, ar100_cb_t cb, void *cb_arg)
{
	struct ar100_message *pmessage;

	/* allocate a message frame */
	pmessage = ar100_message_allocate(0);
	if (pmessage == NULL) {
		AR100_ERR("allocate message for talk-standby request failed\n");
		return -ENOMEM;
	}

	/* check super_standby_para size valid or not */
	if (sizeof(struct super_standby_para) > sizeof(pmessage->paras)) {
		AR100_ERR("talk-standby parameters number too long\n");
		return -EINVAL;
	}

	/* initialize message */
	pmessage->type       = AR100_TSTANDBY_ENTER_REQ;
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;
	memcpy(pmessage->paras, para, sizeof(struct super_standby_para));
	pmessage->state      = AR100_MESSAGE_INITIALIZED;

	/* send enter super-standby request to ar100 */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	return 0;
}
EXPORT_SYMBOL(ar100_standby_talk);

/**
 * notify ar100 cpux talk-standby restored.
 * @para:  none.
 *
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int ar100_cpux_talkstandby_ready_notify(void)
{
	struct ar100_message *pmessage;

	/* notify hwspinlock and hwmsgbox resume first */
	ar100_hwmsgbox_standby_resume();
	ar100_hwspinlock_standby_resume();

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type     = AR100_TSTANDBY_RESTORE_NOTIFY;
	pmessage->state    = AR100_MESSAGE_INITIALIZED;

	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* record wakeup event */
	wakeup_event   = pmessage->paras[0];
	if (ar100_debug_dram_crc_en) {
		dram_crc_error = pmessage->paras[1];
		dram_crc_total_count++;
		dram_crc_error_count += (dram_crc_error ? 1 : 0);
	}

	/* free message */
	ar100_message_free(pmessage);

	return 0;
}
EXPORT_SYMBOL(ar100_cpux_talkstandby_ready_notify);

/**
 * enter fake power off.
 */
void ar100_fake_power_off(void)
{
	struct ar100_message *pmessage;
	script_item_u script_val;
	script_item_value_type_e type;

	/* allocate a message frame */
	pmessage = ar100_message_allocate(0);
	if (pmessage == NULL) {
		AR100_ERR("allocate message for fake power off request failed\n");
	}

	type = script_get_item("pmu_para", "pmu_ir_power_key_code", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("get ir power off key config type err!");
		script_val.val = 0;
	}

	/* initialize message */
	pmessage->type       = AR100_FAKE_POWER_OFF_REQ;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->paras[0]   = script_val.val;

	/* send enter fake power off request to ar100 */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);
}
EXPORT_SYMBOL(ar100_fake_power_off);

int ar100_get_ir_cfg(char *main, char *sub, u32 *val)
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		AR100_ERR("ar100 ir power key code config type err!");
		return -EINVAL;
	}
	*val = script_val.val;
	AR100_INF("ar100 ir power key code config [%s] [%s] : %d\n", main, sub, *val);
	return 0;
}

int ar100_config_ir_paras(void)
{
	u32    ir_power_key_code = 0;
	u32    ir_addr_code = 0;
	int    result = 0;
	struct ar100_message *pmessage;

	/* parse ir power key code */
	if (ar100_get_ir_cfg("ir_para", "ir_power_key_code", &ir_power_key_code)) {
		AR100_WRN("parse ir power key code fail\n");
		return -EINVAL;
	}

	/* parse ir address code */
	if (ar100_get_ir_cfg("ir_para", "ir_addr_code", &ir_addr_code)) {
		AR100_WRN("parse ir address code fail\n");
		return -EINVAL;
	}

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = AR100_SET_IR_PARAS;
	pmessage->paras[0]   = ir_power_key_code;
	pmessage->paras[1]   = ir_addr_code;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	AR100_INF("ir power key code:0x%x\n", pmessage->paras[0]);
	AR100_INF("ir address code:0x%x\n", pmessage->paras[1]);

	/* send request message */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	//check config fail or not
	if (pmessage->result) {
		AR100_WRN("config ir power key code [%d] fail\n", pmessage->paras[0]);
		result = -EINVAL;
	}

	/* free allocated message */
	ar100_message_free(pmessage);

	return result;
}

int ar100_get_pmu_cfg(char *main, char *sub, u32 *val)
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		AR100_ERR("ar100 pmu paras config type err!");
		return -EINVAL;
	}
	*val = script_val.val;
	AR100_INF("ar100 pmu paras config [%s] [%s] : %d\n", main, sub, *val);
	return 0;
}

int ar100_config_pmu_paras(void)
{
	u32 pmu_discharge_ltf = 0;
	u32 pmu_discharge_htf = 0;
	int result = 0;
	struct ar100_message *pmessage;

	/* parse pmu temperature paras */
	if (ar100_get_pmu_cfg("pmu_para", "pmu_discharge_ltf", &pmu_discharge_ltf)) {
		AR100_WRN("parse pmu discharge ltf fail\n");
	}
	if (ar100_get_pmu_cfg("pmu_para", "pmu_discharge_htf", &pmu_discharge_htf)) {
		AR100_WRN("parse pmu discharge htf fail\n");
	}

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = AR100_AXP_SET_PARAS;
	pmessage->paras[0]   = pmu_discharge_ltf;
	pmessage->paras[1]   = pmu_discharge_htf;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	AR100_INF("pmu_discharge_ltf:0x%x\n", pmessage->paras[0]);
	AR100_INF("pmu_discharge_htf:0x%x\n", pmessage->paras[1]);

	/* send request message */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	//check config fail or not
	if (pmessage->result) {
		AR100_WRN("config pmu paras [%d] [%d] fail\n", pmessage->paras[0], pmessage->paras[1]);
		result = -EINVAL;
	}

	/* free allocated message */
	ar100_message_free(pmessage);

	return result;
}

