/*
 *  arch/arm/mach-sun6i/ar100/ar100_axp.c
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

typedef struct axp_isr
{
	ar100_cb_t   handler;
	void        *arg;
} axp_isr_t;

/* pmu isr node, record current pmu interrupt handler and argument */
axp_isr_t axp_isr_node;

/**
 * read axp register data.
 * @addr:    point of registers address;
 * @data:    point of registers data;
 * @len :    number of read registers, max len:8;
 *
 * return: result, 0 - read register successed,
 *                !0 - read register failed or the len more then max len;
 */
int ar100_axp_read_reg(unsigned char *addr, unsigned char *data, unsigned long len)
{
	int                   i;
	int                   result;
	struct ar100_message *pmessage;

	if ((addr == NULL) || (data == NULL) || (len > AXP_TRANS_BYTE_MAX)) {
		AR100_WRN("pmu read reg para error\n");
		return -EINVAL;
	}

	/* allocate a message frame */
	if (ar100_suspend_flag_query()) {
		pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	} else {
		pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_SOFTSYN);
	}

	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = AR100_AXP_READ_REGS;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |addr0~3|addr4~7|data0~3|data4~7|  len  |
	 */
	pmessage->paras[0] = 0;
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pmessage->paras[3] = 0;
	//memset(pmessage->paras, 0, sizeof(pmessage->paras));
	//memset(pmessage->paras, 0, sizeof(unsigned int) * 4);
	pmessage->paras[4] = len;
	for (i = 0; i < len; i++) {
		if (i < 4) {
			/* pack 8bit addr0~addr3 into 32bit paras[0] */
			pmessage->paras[0] |= (addr[i] << (i * 8));
		} else {
			/* pack 8bit addr4~addr7 into 32bit paras[1] */
			pmessage->paras[1] |= (addr[i] << ((i - 4) * 8));
		}
	}

	/* send message use hwmsgbox */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* copy message readout data to user data buffer */
	for (i = 0; i < len; i++) {
		if (i < 4) {
			data[i] = ((pmessage->paras[2]) >> (i * 8)) & 0xff;
		} else {
			data[i] = ((pmessage->paras[3]) >> ((i - 4) * 8)) & 0xff;
		}
	}

	/* free message */
	result = pmessage->result;
	ar100_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(ar100_axp_read_reg);


/**
 * write axp register data.
 * addr:     point of registers address;
 * data:     point of registers data;
 * len :     number of write registers, max len:8;
 *
 * return: result, 0 - write register successed,
 *                !0 - write register failedor the len more then max len;
 */
int ar100_axp_write_reg(unsigned char *addr, unsigned char *data, unsigned long len)
{
	int                   i;
	int                   result;
	struct ar100_message *pmessage;

	if ((addr == NULL) || (data == NULL) || (len > AXP_TRANS_BYTE_MAX)) {
		AR100_WRN("pmu write reg para error\n");
		return -EINVAL;
	}

	/* allocate a message frame */
	if (ar100_suspend_flag_query()) {
		pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	} else {
		pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_SOFTSYN);
	}

	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = AR100_AXP_WRITE_REGS;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |addr0~3|addr4~7|data0~3|data4~7|  len  |
	 */
	pmessage->paras[0] = 0;
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pmessage->paras[3] = 0;
	//memset(pmessage->paras, 0, sizeof(pmessage->paras));
	//memset(pmessage->paras, 0, sizeof(unsigned int) * 4);
	pmessage->paras[4] = len;
	for (i = 0; i < len; i++) {
		if (i < 4) {
			/* pack 8bit addr0~addr3 into 32bit paras[0] */
			pmessage->paras[0] |= (addr[i] << (i * 8));

			/* pack 8bit data0~data3 into 32bit paras[2] */
			pmessage->paras[2] |= (data[i] << (i * 8));
		} else {
			/* pack 8bit addr4~addr7 into 32bit paras[1] */
			pmessage->paras[1] |= (addr[i] << ((i - 4) * 8));

			/* pack 8bit data4~data7 into 32bit paras[3] */
			pmessage->paras[3] |= (data[i] << ((i - 4) * 8));
		}
	}
	/* send message use hwmsgbox */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	ar100_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(ar100_axp_write_reg);

int ar100_axp_clr_regs_bits_sync(unsigned char *addr, unsigned char *mask, unsigned char *delay, unsigned long len)
{
	int                   i;
	int                   result;
	struct ar100_message *pmessage;

	if ((addr == NULL) || (mask == NULL) || (len > AXP_TRANS_BYTE_MAX)) {
		AR100_WRN("pmu write reg para error\n");
		return -EINVAL;
	}

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = AR100_AXP_CLR_REGS_BITS_SYNC;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |addr0~3|addr4~7|data0~3|data4~7|  len  |
	 */
	pmessage->paras[0] = 0;
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pmessage->paras[3] = 0;
	pmessage->paras[4] = 0;
	pmessage->paras[5] = 0;
	pmessage->paras[6] = 0;
	//memset(pmessage->paras, 0, sizeof(pmessage->paras));
	//memset(pmessage->paras, 0, sizeof(unsigned int) * 4);
	pmessage->paras[0] = len;
	for (i = 0; i < len; i++) {
		if (i < 4) {
			/* pack 8bit addr0~addr3 into 32bit paras[0] */
			pmessage->paras[1] |= (addr[i] << (i * 8));

			/* pack 8bit mask0~mask3 into 32bit paras[2] */
			pmessage->paras[3] |= (mask[i] << (i * 8));

			/* pack 8bit delay0~delay3 into 32bit paras[3] */
			pmessage->paras[5] |= (delay[i] << (i * 8));
		} else {
			/* pack 8bit addr4~addr7 into 32bit paras[1] */
			pmessage->paras[2] |= (addr[i] << ((i - 4) * 8));

			/* pack 8bit mask4~mask7 into 32bit paras[3] */
			pmessage->paras[4] |= (mask[i] << ((i - 4) * 8));

			/* pack 8bit delay4~delay7 into 32bit paras[5] */
			pmessage->paras[6] |= (delay[i] << ((i - 4) * 8));
		}
	}
	/* send message use hwmsgbox */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	ar100_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(ar100_axp_clr_regs_bits_sync);

int ar100_axp_set_regs_bits_sync(unsigned char *addr, unsigned char *mask, unsigned char *delay, unsigned long len)
{
	int                   i;
	int                   result;
	struct ar100_message *pmessage;

	if ((addr == NULL) || (mask == NULL) || (len > AXP_TRANS_BYTE_MAX)) {
		AR100_WRN("pmu write reg para error\n");
		return -EINVAL;
	}

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = AR100_AXP_SET_REGS_BITS_SYNC;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |addr0~3|addr4~7|data0~3|data4~7|  len  |
	 */
	pmessage->paras[0] = 0;
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pmessage->paras[3] = 0;
	pmessage->paras[4] = 0;
	pmessage->paras[5] = 0;
	pmessage->paras[6] = 0;
	//memset(pmessage->paras, 0, sizeof(pmessage->paras));
	//memset(pmessage->paras, 0, sizeof(unsigned int) * 4);
	pmessage->paras[0] = len;
	for (i = 0; i < len; i++) {
		if (i < 4) {
			/* pack 8bit addr0~addr3 into 32bit paras[0] */
			pmessage->paras[1] |= (addr[i] << (i * 8));

			/* pack 8bit mask0~mask3 into 32bit paras[2] */
			pmessage->paras[3] |= (mask[i] << (i * 8));

			/* pack 8bit delay0~delay3 into 32bit paras[3] */
			pmessage->paras[5] |= (delay[i] << (i * 8));
		} else {
			/* pack 8bit addr4~addr7 into 32bit paras[1] */
			pmessage->paras[2] |= (addr[i] << ((i - 4) * 8));

			/* pack 8bit mask4~mask7 into 32bit paras[3] */
			pmessage->paras[4] |= (mask[i] << ((i - 4) * 8));

			/* pack 8bit delay4~delay7 into 32bit paras[5] */
			pmessage->paras[6] |= (delay[i] << ((i - 4) * 8));
		}
	}
	/* send message use hwmsgbox */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	ar100_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(ar100_axp_set_regs_bits_sync);

/**
 * register call-back function, call-back function is for ar100 notify some event to ac327,
 * axp interrupt for external interrupt NMI.
 * @func:  call-back function;
 * @para:  parameter for call-back function;
 *
 * @return: result, 0 - register call-back function successed;
 *                 !0 - register call-back function failed;
 * NOTE: the function is like "int callback(void *para)";
 *       this function will execute in system ISR.
 */
int ar100_axp_cb_register(ar100_cb_t func, void *para)
{
	if (axp_isr_node.handler) {
		if(func == axp_isr_node.handler) {
			AR100_WRN("pmu interrupt handler register already\n");
			return 0;
		}
		/* just output warning message, overlay handler */
		AR100_WRN("pmu interrupt handler register already\n");
		return -EINVAL;
	}
	axp_isr_node.handler = func;
	axp_isr_node.arg     = para;

	return 0;
}
EXPORT_SYMBOL(ar100_axp_cb_register);


/**
 * unregister call-back function.
 * @func:  call-back function which need be unregister;
 */
void ar100_axp_cb_unregister(ar100_cb_t func)
{
	if ((u32)(axp_isr_node.handler) != (u32)(func)) {
		/* invalid handler */
		AR100_WRN("invalid handler for unreg\n\n");
		return ;
	}
	axp_isr_node.handler = NULL;
	axp_isr_node.arg     = NULL;
}
EXPORT_SYMBOL(ar100_axp_cb_unregister);

int ar100_disable_axp_irq(void)
{
	int                   result;
	struct ar100_message *pmessage;

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = AR100_AXP_DISABLE_IRQ;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/* send message use hwmsgbox */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	ar100_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(ar100_disable_axp_irq);

int ar100_enable_axp_irq(void)
{
	int                   result;
	struct ar100_message *pmessage;

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = AR100_AXP_ENABLE_IRQ;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/* send message use hwmsgbox */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	ar100_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(ar100_enable_axp_irq);

int ar100_axp_get_chip_id(unsigned char *chip_id)
{
	int                   i;
	int                   result;
	struct ar100_message *pmessage;

	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = AR100_AXP_GET_CHIP_ID;
	pmessage->state      = AR100_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	memset((void *)pmessage->paras, 0, 16);

	/* send message use hwmsgbox */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	/* |paras[0]    |paras[1]    |paras[2]     |paras[3]      |
	 * |chip_id[0~3]|chip_id[4~7]|chip_id[8~11]|chip_id[12~15]|
	 */
	/* copy message readout data to user data buffer */
	for (i = 0; i < 4; i++) {
			chip_id[i] = (pmessage->paras[0] >> (i * 8)) & 0xff;
			chip_id[4 + i] = (pmessage->paras[1] >> (i * 8)) & 0xff;
			chip_id[8 + i] = (pmessage->paras[2] >> (i * 8)) & 0xff;
			chip_id[12 + i] = (pmessage->paras[3] >> (i * 8)) & 0xff;
	}

	/* free message */
	result = pmessage->result;
	ar100_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(ar100_axp_get_chip_id);

int ar100_axp_int_notify(struct ar100_message *pmessage)
{
	/* call pmu interrupt handler */
	if (axp_isr_node.handler == NULL) {
		AR100_WRN("axp irq handler not install\n");
		return 1;
	}
	return (*(axp_isr_node.handler))(axp_isr_node.arg);
}
