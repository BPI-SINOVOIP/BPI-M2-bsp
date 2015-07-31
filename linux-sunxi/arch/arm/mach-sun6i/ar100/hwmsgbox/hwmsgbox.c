/*
 *  arch/arm/mach-sun6i/ar100/hwmsgbox/hwmsgbox.c
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

#include "hwmsgbox_i.h"

/* spinlock for syn and asyn channel */
static spinlock_t syn_channel_lock;
static spinlock_t asyn_channel_lock;


/**
 * initialize hwmsgbox.
 * @para:  none.
 *
 * returns:  0 if initialize hwmsgbox succeeded, others if failed.
 */
int ar100_hwmsgbox_init(void)
{
	int ret;

	/* register msgbox interrupt */
	ret = request_irq(AW_IRQ_MBOX, ar100_hwmsgbox_int_handler,
			IRQF_NO_SUSPEND, "ar100_hwmsgbox_irq", NULL);
	if (ret) {
		AR100_ERR("request_irq error, return %d\n", ret);
		return ret;
	}

	/* initialize syn and asyn spinlock */
	spin_lock_init(&(syn_channel_lock));
	spin_lock_init(&(asyn_channel_lock));

	return 0;
}

/**
 * exit hwmsgbox.
 * @para:  none.
 *
 * returns:  0 if exit hwmsgbox succeeded, others if failed.
 */
int ar100_hwmsgbox_exit(void)
{
	return 0;
}

/**
 * send one message to another processor by hwmsgbox.
 * @pmessage:  the pointer of sended message frame.
 * @timeout:   the wait time limit when message fifo is full,                             it is valid only when parameter mode = HWMSG_SEND_WAIT_TIMEOUT.
 *
 * returns:   0 if send message succeeded, other if failed.
 */
int ar100_hwmsgbox_send_message(struct ar100_message *pmessage, unsigned int timeout)
{
	volatile unsigned long value;
	unsigned long          expire;

	expire = msecs_to_jiffies(timeout) + jiffies;

	if (pmessage == NULL) {
		return -EINVAL;
	}
	if (pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN) {
		/* use ac327 hwsyn transmit channel */
		spin_lock(&syn_channel_lock);
		while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(AR100_HWMSGBOX_AC327_SYN_TX_CH))) == 1) {
			/* message-queue fifo is full */
			if (time_is_before_eq_jiffies(expire)) {
				AR100_ERR("hw message queue fifo full timeout\n");
				spin_unlock(&syn_channel_lock);
				return -ETIMEDOUT;
			}
		}
		value = ((volatile unsigned long)pmessage) - ar100_sram_a2_vbase;
		AR100_INF("ac327 send hard syn message : %x\n", (unsigned int)value);
		writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AC327_SYN_TX_CH)));

		/* hwsyn messsage must feedback use syn rx channel */
		while (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(AR100_HWMSGBOX_AC327_SYN_RX_CH))) == 0) {
			if (time_is_before_eq_jiffies(expire)) {
				AR100_ERR("wait hard syn message time out\n");
				spin_unlock(&syn_channel_lock);
				return -ETIMEDOUT;
			}
		}
		/* check message valid */
		if (value != (readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AC327_SYN_RX_CH))))) {
			AR100_ERR("hard syn message error [%x, %x]\n", (u32)value, (u32)(readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AC327_SYN_RX_CH)))));
			spin_unlock(&syn_channel_lock);
			return -EINVAL;
		}
		AR100_INF("ac327 hard syn message [%x, %x] feedback\n", (unsigned int)value, (unsigned int)pmessage->type);
		/* if error call the callback function. by superm */
		if(pmessage->result != 0) {
			if (pmessage->cb.handler == NULL) {
				AR100_WRN("callback not install\n");
			} else {
				/* call callback function */
				AR100_WRN("call the callback function\n");
				(*(pmessage->cb.handler))(pmessage->cb.arg);
			}
		}
		spin_unlock(&syn_channel_lock);
		return 0;
	}

	/* use ac327 asyn transmit channel */
	spin_lock(&asyn_channel_lock);
	while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(AR100_HWMSGBOX_AR100_ASYN_RX_CH))) == 1) {
		/* message-queue fifo is full */
		if (time_is_before_eq_jiffies(expire)) {
			AR100_ERR("wait asyn message time out\n");
			spin_unlock(&asyn_channel_lock);
			return -ETIMEDOUT;
		}
	}
	/* write message to message-queue fifo */
	value = ((volatile unsigned long)pmessage) - ar100_sram_a2_vbase;
	AR100_INF("ac327 send message : %x\n", (unsigned int)value);
	writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AR100_ASYN_RX_CH)));

	spin_unlock(&asyn_channel_lock);

	/* syn messsage must wait message feedback */
	if (pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) {
		ar100_hwmsgbox_wait_message_feedback(pmessage);
	}
	return 0;
}

int ar100_hwmsgbox_feedback_message(struct ar100_message *pmessage, unsigned int timeout)
{
	volatile unsigned long value;
	unsigned long          expire;

	expire = msecs_to_jiffies(timeout) + jiffies;

	if (pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN) {
		/* use ac327 hard syn receiver channel */
		spin_lock(&syn_channel_lock);
		while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(AR100_HWMSGBOX_AR100_SYN_RX_CH))) == 1) {
			/* message-queue fifo is full */
			if (time_is_before_eq_jiffies(expire)) {
				AR100_ERR("wait syn message-queue fifo full timeout\n");
				return -ETIMEDOUT;
			}
		}
		value = ((volatile unsigned long)pmessage) - ar100_sram_a2_vbase;
		AR100_INF("ar100 feedback hard syn message : %x\n", (unsigned int)value);
		writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AR100_SYN_RX_CH)));
		spin_unlock(&syn_channel_lock);
		return 0;
	}
	/* soft syn use asyn tx channel */
	if (pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) {
		spin_lock(&asyn_channel_lock);
		while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(AR100_HWMSGBOX_AR100_ASYN_RX_CH))) == 1) {
			/* fifo is full */
			if (time_is_before_eq_jiffies(expire)) {
				AR100_ERR("wait asyn message-queue fifo full timeout\n");
				return -ETIMEDOUT;
			}
		}
		/* write message to message-queue fifo */
		value = ((volatile unsigned long)pmessage) - ar100_sram_a2_vbase;
		AR100_INF("ar100 send asyn or soft syn message : %x\n", (unsigned int)value);
		writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AR100_ASYN_RX_CH)));
		spin_unlock(&asyn_channel_lock);
		return 0;
	}

	/* invalid syn message */
	return -EINVAL;
}

/**
 * enbale the receiver interrupt of message-queue.
 * @queue:  the number of message-queue which we want to enable interrupt.
 * @user:   the user which we want to enable interrupt.
 *
 * returns:  0 if enable interrupt succeeded, others if failed.
 */
int ar100_hwmsgbox_enable_receiver_int(int queue, int user)
{
	volatile unsigned int value;

	value  =  readl(IO_ADDRESS(AW_MSGBOX_IRQ_EN_REG(user)));
	value &= ~(0x1 << (queue * 2));
	value |=  (0x1 << (queue * 2));
	writel(value, IO_ADDRESS(AW_MSGBOX_IRQ_EN_REG(user)));

	return 0;
}

/**
 * disbale the receiver interrupt of message-queue.
 * @queue:  the number of message-queue which we want to enable interrupt.
 * @user:   the user which we want to enable interrupt.
 *
 * returns:  0 if disable interrupt succeeded, others if failed.
 */
int ar100_hwmsgbox_disable_receiver_int(int queue, int user)
{
	volatile unsigned int value;

	value  =  readl(IO_ADDRESS(AW_MSGBOX_IRQ_EN_REG(user)));
	value &= ~(0x1 << (queue * 2));
	writel(value, IO_ADDRESS(AW_MSGBOX_IRQ_EN_REG(user)));

	return 0;
}

/**
 * query the receiver interrupt pending of message-queue.
 * @queue:  the number of message-queue which we want to query.
 * @user:   the user which we want to query.
 *
 * returns:  0 if query pending succeeded, others if failed.
 */
int ar100_hwmsgbox_query_receiver_pending(int queue, int user)
{
	volatile unsigned long value;

	value  =  readl(IO_ADDRESS((AW_MSGBOX_IRQ_STATUS_REG(user))));

	return value & (0x1 << (queue * 2));
}

/**
 * clear the receiver interrupt pending of message-queue.
 * @queue:  the number of message-queue which we want to clear.
 * @user:   the user which we want to clear.
 *
 * returns:  0 if clear pending succeeded, others if failed.
 */
int ar100_hwmsgbox_clear_receiver_pending(int queue, int user)
{
	writel((0x1 << (queue * 2)), IO_ADDRESS(AW_MSGBOX_IRQ_STATUS_REG(user)));

	return 0;
}

/**
 * the interrupt handler for message-queue 1 receiver.
 * @parg: the argument of this handler.
 *
 * returns:  TRUE if handle interrupt succeeded, others if failed.
 */
irqreturn_t ar100_hwmsgbox_int_handler(int irq, void *dev)
{
	AR100_INF("ac327 msgbox interrupt handler...\n");

	/* process ac327 asyn received channel, process all received messages */
	while (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(AR100_HWMSGBOX_AR100_ASYN_TX_CH)))) {
		volatile unsigned long value;
		struct ar100_message *pmessage;
		value = readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AR100_ASYN_TX_CH))) + ar100_sram_a2_vbase;
		pmessage = (struct ar100_message *)value;
		if (ar100_message_valid(pmessage)) {
			/* message state switch */
			if (pmessage->state == AR100_MESSAGE_PROCESSED) {
				/* if error call the callback function. by superm */
				if (pmessage->result != 0) {
					if (pmessage->cb.handler == NULL) {
						AR100_WRN("message [%x] error, callback not install\n",
								  (unsigned int)pmessage->type);
					} else {
						/* call callback function */
						AR100_WRN("messgae [%x] error, call message callback function\n",
								  (unsigned int)pmessage->type);
						(*(pmessage->cb.handler))(pmessage->cb.arg);
					}
				}
				/*
				 * AR100_MESSAGE_PROCESSED->AR100_MESSAGE_FEEDBACKED,
				 * process feedback message
				 */
				pmessage->state = AR100_MESSAGE_FEEDBACKED;
				if (pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN)
					ar100_hwmsgbox_message_feedback(pmessage);
				else if (pmessage->attr == 0)
					ar100_message_free(pmessage);
			} else {
				/*
				 * AR100_MESSAGE_INITIALIZED->AR100_MESSAGE_RECEIVED,
				 * notify new message coming.
				 */
				pmessage->state = AR100_MESSAGE_RECEIVED;
				ar100_message_coming_notify(pmessage);
			}
		} else {
			AR100_ERR("invalid message received: pmessage = 0x%x. \n", (__u32)pmessage);
		}
	}
	/* clear pending */
	ar100_hwmsgbox_clear_receiver_pending(AR100_HWMSGBOX_AR100_ASYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	/* process ac327 syn received channel, process only one message */
	if (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(AR100_HWMSGBOX_AR100_SYN_TX_CH)))) {
		volatile unsigned long value;
		struct ar100_message *pmessage;
		value = readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AR100_SYN_TX_CH))) + ar100_sram_a2_vbase;
		pmessage = (struct ar100_message *)value;
		if (ar100_message_valid(pmessage)) {
			/* message state switch */
			if (pmessage->state == AR100_MESSAGE_PROCESSED) {
				/*
				 * AR100_MESSAGE_PROCESSED->AR100_MESSAGE_FEEDBACKED,
				 * process feedback message.
				 */
				pmessage->state = AR100_MESSAGE_FEEDBACKED;
				ar100_hwmsgbox_message_feedback(pmessage);
			} else {
				/*
				 * AR100_MESSAGE_INITIALIZED->AR100_MESSAGE_RECEIVED,
				 * notify new message coming.
				 */
				pmessage->state = AR100_MESSAGE_RECEIVED;
				ar100_message_coming_notify(pmessage);
			}
		} else {
			AR100_ERR("invalid message received: pmessage = 0x%x. \n", (__u32)pmessage);
		}
	}
	/* clear pending */
	ar100_hwmsgbox_clear_receiver_pending(AR100_HWMSGBOX_AR100_SYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	return IRQ_HANDLED;
}

/**
 * query message of hwmsgbox by hand, mainly for.
 * @para:  none.
 *
 * returns:  the point of message, NULL if timeout.
 */
struct ar100_message *ar100_hwmsgbox_query_message(void)
{
	struct ar100_message *pmessage = NULL;

	/* query ac327 asyn received channel */
	if (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(AR100_HWMSGBOX_AR100_ASYN_TX_CH)))) {
		volatile unsigned long value;
		value = readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AR100_ASYN_TX_CH)));
		pmessage = (struct ar100_message *)(value + ar100_sram_a2_vbase);

		if (ar100_message_valid(pmessage)) {
			/* message state switch */
			if (pmessage->state == AR100_MESSAGE_PROCESSED) {
				/* AR100_MESSAGE_PROCESSED->AR100_MESSAGE_FEEDBACKED */
				pmessage->state = AR100_MESSAGE_FEEDBACKED;
			} else {
				/* AR100_MESSAGE_INITIALIZED->AR100_MESSAGE_RECEIVED */
				pmessage->state = AR100_MESSAGE_RECEIVED;
			}
		} else {
			AR100_ERR("invalid message received: pmessage = 0x%x. \n", (__u32)pmessage);
			return NULL;
		}
		/* clear pending */
		ar100_hwmsgbox_clear_receiver_pending(AR100_HWMSGBOX_AR100_ASYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);
		return pmessage;
	}
	/* query ac327 syn received channel */
	if (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(AR100_HWMSGBOX_AR100_SYN_TX_CH)))) {
		volatile unsigned long value;
		value = readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(AR100_HWMSGBOX_AR100_SYN_TX_CH)));
		pmessage = (struct ar100_message *)(value + ar100_sram_a2_vbase);
		if (ar100_message_valid(pmessage)) {
			/* message state switch */
			if (pmessage->state == AR100_MESSAGE_PROCESSED) {
				/* AR100_MESSAGE_PROCESSED->AR100_MESSAGE_FEEDBACKED */
				pmessage->state = AR100_MESSAGE_FEEDBACKED;
			} else {
				/* AR100_MESSAGE_INITIALIZED->AR100_MESSAGE_RECEIVED */
				pmessage->state = AR100_MESSAGE_RECEIVED;
			}
		} else {
			AR100_ERR("invalid message received: pmessage = 0x%x. \n", (__u32)pmessage);
			return NULL;
		}
		ar100_hwmsgbox_clear_receiver_pending(AR100_HWMSGBOX_AR100_SYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);
		return pmessage;
	}

	/* no valid message now */
	return NULL;
}

int ar100_hwmsgbox_wait_message_feedback(struct ar100_message *pmessage)
{
	/* linux method: wait semaphore flag to set */
	AR100_INF("down semaphore for message feedback, semp=0x%x.\n",
			   (unsigned int)(pmessage->private));
	down((struct semaphore *)(pmessage->private));

	AR100_INF("message : %x finished\n", (unsigned int)pmessage);

	return 0;
}

int ar100_hwmsgbox_message_feedback(struct ar100_message *pmessage)
{
	/* linux method: wait semaphore flag to set */
	AR100_INF("up semaphore for message feedback, sem=0x%x.\n",
			   (unsigned int)(pmessage->private));
	up((struct semaphore *)(pmessage->private));

	return 0;
}

int ar100_message_valid(struct ar100_message *pmessage)
{
	if ((((u32)pmessage) >= (AR100_MESSAGE_POOL_START + ar100_sram_a2_vbase)) &&
		(((u32)pmessage) <  (AR100_MESSAGE_POOL_END   + ar100_sram_a2_vbase))) {

		/* valid message */
		return 1;
	}

	return 0;
}

int ar100_hwmsgbox_standby_suspend(void)
{
	/* enable ar100 asyn tx interrupt */
	ar100_hwmsgbox_disable_receiver_int(AR100_HWMSGBOX_AR100_ASYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	/* enable ar100 syn tx interrupt */
	ar100_hwmsgbox_disable_receiver_int(AR100_HWMSGBOX_AR100_SYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	return 0;
}

int ar100_hwmsgbox_standby_resume(void)
{
	/* enable ar100 asyn tx interrupt */
	ar100_hwmsgbox_enable_receiver_int(AR100_HWMSGBOX_AR100_ASYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	/* enable ar100 syn tx interrupt */
	ar100_hwmsgbox_enable_receiver_int(AR100_HWMSGBOX_AR100_SYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	return 0;
}
