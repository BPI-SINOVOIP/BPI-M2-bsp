/*
 * arch/arm/mach-sun6i/include/mach/ar100.h
 *
 * Copyright 2012 (c) Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "standby_ar100_i.h"

//sram a2 virtual base address
unsigned long ar100_sram_a2_vbase = (unsigned long)IO_ADDRESS(AW_SRAM_A2_BASE);

//record wakeup event information
static unsigned long wakeup_event;

//record restore status information
static unsigned long restore_completed;

int standby_ar100_init(void)
{
	//initialize wakeup_event as invalid
	wakeup_event = 0;
	
	//initialize restore as uncompleted
	restore_completed = 0;
	
	//initialize hwspinlock
	ar100_hwspinlock_init();
	
	//initialize hwmsgbox
	ar100_hwmsgbox_init();
	
	//initialize message manager
	ar100_message_manager_init();
	
	return 0;
}

int standby_ar100_exit(void)
{
	//exit message manager
	ar100_message_manager_exit();
	
	//exit hwmsgbox
	ar100_hwmsgbox_exit();
	
	//exit hwspinlock
	ar100_hwspinlock_exit();
	
	restore_completed = 0;
	
	wakeup_event = 0;
	
	return 0;
}

/*
 * standby_ar100_notify_restore: 
 * function: notify ar100 to wakeup: restore cpus freq, volt, and init_dram.
 * para:  mode.
 * STANDBY_AR100_SYNC:
 * STANDBY_AR100_ASYNC:
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int standby_ar100_notify_restore(unsigned long mode)
{
	struct ar100_message *pmessage;
	unsigned char         attr;
	
	//allocate a message frame
	pmessage = ar100_message_allocate(0);
	if (pmessage == NULL) {
		AR100_ERR("allocate message for normal-standby notify restore failed\n");
		return -ENOMEM;
	}
	//initialize message attributes
	attr = 0;
	if (mode & STANDBY_AR100_SYNC) {
		attr |= AR100_MESSAGE_ATTR_HARDSYN;
	}
	//initialize message
	pmessage->type     = AR100_NSTANDBY_RESTORE_REQ;
	pmessage->attr     = attr;
	pmessage->state    = AR100_MESSAGE_INITIALIZED;
	
	//send enter normal-standby restore request to ar100
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);
	
	//syn message must free message frame by self.
	if (mode & STANDBY_AR100_SYNC) {
		ar100_message_free(pmessage);
	}
	return 0;
}

/* 
 * standby_ar100_check_restore_status
 * function: check ar100 restore status.
 * para:  none.
 * return: result, 0 - restore completion successed, !0 - notify failed;
 */
int standby_ar100_check_restore_status(void)
{
	struct ar100_message *pmessage;
	
	//check restore completion flag first
	if (restore_completed) {
		//ar100 restore completion already.
		return 0;
	}
	//try ot query message for hwmsgbox
	pmessage = ar100_hwmsgbox_query_message();
	if (pmessage == NULL) {
		//no valid message
		return -EINVAL;
	}
	if (pmessage->type == AR100_NSTANDBY_RESTORE_COMPLETE) {
		//restore complete message received
		pmessage->state = AR100_MESSAGE_PROCESSED;
		if ((pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) || 
			(pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN)) {
			//synchronous message, should feedback process result.
			ar100_hwmsgbox_feedback_message(pmessage, AR100_SEND_MSG_TIMEOUT);
		} else {
			//asyn message, no need feedback message result,
			//free message directly.
			ar100_message_free(pmessage);	
		}
		//ar100 restore completion
		restore_completed = 1;
		return 0;
	} else {
		AR100_ERR("invalid message received when check restore status\n");
		return -EINVAL;
	}
	
	//not received restore completion
	return -EINVAL;
}

/* 
 * standby_ar100_query_wakeup_src
 * function: query standby wakeup source.
 * para:  point of buffer to store wakeup event informations.
 * return: result, 0 - query successed, !0 - query failed;
 */
int standby_ar100_query_wakeup_src(unsigned long *event)
{
	struct ar100_message *pmessage;
	
	//check parameter valid or not
	if (event == NULL) {
		AR100_WRN("invalid buffer to query wakeup source\n");
		return -EINVAL;
	}
	
	//check wakeup event received first
	if (wakeup_event) {
		//ar100 wakeup event received already.
		*event = wakeup_event;
		return 0;
	}
	//try ot query message for hwmsgbox
	pmessage = ar100_hwmsgbox_query_message();
	if (pmessage == NULL) {
		//no valid message
		return -EINVAL;
	}
	if (pmessage->type == AR100_NSTANDBY_WAKEUP_NOTIFY) {
		//wakeup notify message received
		wakeup_event = pmessage->paras[0];
		pmessage->state = AR100_MESSAGE_PROCESSED;
		if ((pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) || 
			(pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN)) {
			//synchronous message, should feedback process result.
			ar100_hwmsgbox_feedback_message(pmessage, AR100_SEND_MSG_TIMEOUT);
		} else {
			//asyn message, no need feedback message result,
			//free message directly.
			ar100_message_free(pmessage);	
		}
		//ar100 wakeup_event valid
		AR100_INF("standby wakeup source : 0x%x\n", (unsigned int)wakeup_event);
		*event = wakeup_event;
		return 0;
	} else {
		AR100_ERR("invalid message received when check restore status\n");
		return -EINVAL;
	}
	//not received wakup event
	return -EINVAL;
}

/*
 * standby_ar100_standby_normal
 * function: enter normal standby.
 * para:  parameter for enter normal standby.
 * return: result, 0 - normal standby successed, !0 - normal standby failed;
 */
int standby_ar100_standby_normal(struct normal_standby_para *para)
{
	struct ar100_message *pmessage;
	int result = 0;
	
	//allocate a message frame
	pmessage = ar100_message_allocate(0);
	if (pmessage == NULL) {
		AR100_ERR("allocate message for normal-standby request failed\n");
		return -ENOMEM;
	}
	
	//check normal_standby_para size valid or not
	if (sizeof(struct normal_standby_para) > sizeof(pmessage->paras)) {
		AR100_ERR("normal-standby parameters number too long\n");
		return -EINVAL;
	}
	//initialize message
	pmessage->type     = AR100_NSTANDBY_ENTER_REQ;
	//pmessage->attr     = AR100_MESSAGE_ATTR_HARDSYN;
	pmessage->attr       = 0;
	standby_memcpy((void *)pmessage->paras, (void *)para, sizeof(struct normal_standby_para));
	pmessage->state    = AR100_MESSAGE_INITIALIZED;
	
	//send enter normal-standby request to ar100
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	result = (int)(pmessage->result);
	//syn message, free it after feedbacked.
        ar100_message_free(pmessage);
	
	return result;
}
