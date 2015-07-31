/*
 *  arch/arm/mach-sun6i/ar100/message_manager/message_manager.c
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


#include "message_manager_i.h"

/* the start and end of message pool */
static struct ar100_message *message_start;
static struct ar100_message *message_end;

/* spinlock for this module */
static spinlock_t    msg_mgr_lock;
static unsigned long msg_mgr_flag;

/* message cache manager */
static struct ar100_message_cache message_cache;

/* semaphore cache manager */
static struct ar100_semaphore_cache sem_cache;

static atomic_t sem_used_num;

/**
 * initialize message manager.
 * @para:  none.
 *
 * returns:  0 if initialize succeeded, others if failed.
 */
int ar100_message_manager_init(void)
{
	int i;

	/* initialize message pool start and end */
	message_start = (struct ar100_message *)(ar100_sram_a2_vbase + AR100_MESSAGE_POOL_START);
	message_end   = (struct ar100_message *)(ar100_sram_a2_vbase + AR100_MESSAGE_POOL_END);

	/* initialize message_cache */
	for (i = 0; i < AR100_MESSAGE_CACHED_MAX; i++) {
		message_cache.cache[i] = NULL;
	}
	atomic_set(&(message_cache.number), 0);

	/* initialzie semaphore allocator */
	for (i = 0; i < AR100_SEM_CACHE_MAX; i++) {
		sem_cache.cache[i] = NULL;
	}
	atomic_set(&(sem_cache.number), 0);
	atomic_set(&sem_used_num, 0);

	/* initialize message manager spinlock */
	spin_lock_init(&(msg_mgr_lock));
	msg_mgr_flag = 0;

	return 0;
}

/**
 * exit message manager.
 * @para:  none.
 *
 * returns:  0 if exit succeeded, others if failed.
 */
int ar100_message_manager_exit(void)
{
	return 0;
}

static int ar100_semaphore_invalid(struct semaphore *psemaphore)
{
	/* semaphore use system kmalloc, valid range check */
	//if ((psemaphore >= ((struct semaphore *)(0xC0000000))) &&
	//  (psemaphore <  ((struct semaphore *)(0xF0000000))))
	if (psemaphore) {
		/* valid ar100 semaphore */
		return 0;
	}
	/* invalid ar100 semaphore */
	return 1;
}

static struct semaphore *ar100_semaphore_allocate(void)
{
	struct semaphore *sem = NULL;

	/* try to allocate from cache first */
	spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
	if (atomic_read(&(sem_cache.number))) {
		atomic_dec(&(sem_cache.number));
		sem = sem_cache.cache[atomic_read(&(sem_cache.number))];
		sem_cache.cache[atomic_read(&(sem_cache.number))] = NULL;
		if (ar100_semaphore_invalid(sem)) {
			AR100_ERR("allocate cache semaphore [%x] invalid\n", (u32)sem);
		}
	}
	spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);

	if (ar100_semaphore_invalid(sem)) {
		/* cache allocate fail, allocate from kmem */
		sem = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
	}
	/* check allocate semaphore valid or not */
	if (ar100_semaphore_invalid(sem)) {
		AR100_ERR("allocate semaphore [%x] invalid\n", (u32)sem);
		return NULL;
	}

	/* initialize allocated semaphore */
	sema_init(sem, 0);
	atomic_inc(&sem_used_num);

	return sem;
}

static int ar100_semaphore_free(struct semaphore *sem)
{
	struct semaphore *free_sem = sem;

	if (ar100_semaphore_invalid(free_sem)) {
		AR100_ERR("free semaphore [%x] invalid\n", (u32)free_sem);
		return -1;
	}

	/* try to free semaphore to cache */
	spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
	if (atomic_read(&(sem_cache.number)) < AR100_SEM_CACHE_MAX) {
		sem_cache.cache[atomic_read(&(sem_cache.number))] = free_sem;
		atomic_inc(&(sem_cache.number));
		free_sem = NULL;
	}
	spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);

	/* try to free semaphore to kmem if free to cache fail */
	if (free_sem) {
		/* free to kmem */
		kfree(free_sem);
	}

	atomic_dec(&sem_used_num);

	return 0;
}

int ar100_semaphore_used_num_query(void)
{
	return atomic_read(&sem_used_num);
}


static int ar100_message_invalid(struct ar100_message *pmessage)
{
	if ((pmessage >= message_start) &&
		(pmessage < message_end)) {
		/* valid ar100 message */
		return 0;
	}
	/* invalid ar100 message */
	return 1;
}

/**
 * allocate one message frame. mainly use for send message by message-box,
 * the message frame allocate form messages pool shared memory area.
 * @para:  none.
 *
 * returns:  the pointer of allocated message frame, NULL if failed;
 */
struct ar100_message *ar100_message_allocate(unsigned int msg_attr)
{
	struct ar100_message *pmessage = NULL;
	struct ar100_message *palloc   = NULL;

	/* first find in message_cache */
	spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
	if (atomic_read(&(message_cache.number))) {
		AR100_INF("ar100 message_cache.number = 0x%x.\n", atomic_read(&(message_cache.number)));
		atomic_dec(&(message_cache.number));
		palloc = message_cache.cache[atomic_read(&(message_cache.number))];
		AR100_INF("message [%x] allocate from message_cache\n", (u32)palloc);
		if (ar100_message_invalid(palloc)) {
			AR100_ERR("allocate cache message [%x] invalid\n", (u32)palloc);
		}
	}
	spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);
	if (ar100_message_invalid(palloc)) {
		/*
		 * cached message_cache finded fail,
		 * use spinlock 0 to exclusive with ar100.
		 */
		ar100_hwspin_lock_timeout(0, AR100_SPINLOCK_TIMEOUT);

		/* seach from the start of message pool every time. */
		pmessage = message_start;
		while (pmessage < message_end) {
			if (pmessage->state == AR100_MESSAGE_FREED) {
				/* find free message in message pool, allocate it */
				palloc = pmessage;
				palloc->state = AR100_MESSAGE_ALLOCATED;
				AR100_INF("message [%x] allocate from message pool\n", (u32)palloc);
				break;
			}
			/* next message frame */
			pmessage++;
		}
		/* unlock hwspinlock 0 */
		ar100_hwspin_unlock(0);
	}
	if (ar100_message_invalid(palloc)) {
		AR100_ERR("allocate message [%x] frame is invalid\n", (u32)palloc);
		return NULL;
	}
	/* initialize messgae frame */
	palloc->next = 0;
	palloc->attr = msg_attr;
	if (msg_attr & AR100_MESSAGE_ATTR_SOFTSYN) {
		/* syn message,allocate one semaphore for private */
		palloc->private = ar100_semaphore_allocate();
	} else {
		palloc->private = NULL;
	}
	return palloc;
}

/**
 * free one message frame. mainly use for process message finished,
 * free it to messages pool or add to free message queue.
 * @pmessage:  the pointer of free message frame.
 *
 * returns:  none.
 */
void ar100_message_free(struct ar100_message *pmessage)
{
	struct ar100_message *free_message = pmessage;

	/* check this message valid or not */
	if (ar100_message_invalid(free_message)) {
		AR100_ERR("free invalid ar100 message [%x]\n", (u32)free_message);
		return;
	}
	if (free_message->attr & AR100_MESSAGE_ATTR_SOFTSYN) {
		/* free message semaphore first */
		ar100_semaphore_free((struct semaphore *)(free_message->private));
		free_message->private = NULL;
	}
	/* try to free to free_list first */
	spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
	if (atomic_read(&(message_cache.number)) < AR100_MESSAGE_CACHED_MAX) {
		AR100_INF("insert message [%x] to message_cache\n", (unsigned int)free_message);
		AR100_INF("message_cache number : %d\n", atomic_read(&(message_cache.number)));
		/* cached this message, message state: ALLOCATED */
		message_cache.cache[atomic_read(&(message_cache.number))] = free_message;
		atomic_inc(&(message_cache.number));
		free_message->next = NULL;
		free_message->state = AR100_MESSAGE_ALLOCATED;
		free_message = NULL;
	}
	spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);

	/*  try to free message to pool if free to cache fail */
	if (free_message) {
		/* free to message pool,set message state as FREED. */
		ar100_hwspin_lock_timeout(0, AR100_SPINLOCK_TIMEOUT);
		free_message->state = AR100_MESSAGE_FREED;
		free_message->next  = NULL;
		ar100_hwspin_unlock(0);
	}
}

/**
 * notify system that one message coming.
 * @pmessage:  the pointer of coming message frame.
 *
 * returns:  0 if notify succeeded, other if failed.
 */
int ar100_message_coming_notify(struct ar100_message *pmessage)
{
	int   ret;

	/* ac327 receive message to ar100 */
	AR100_INF("-------------------------------------------------------------\n");
	AR100_INF("                MESSAGE FROM AR100                           \n");
	AR100_INF("message addr : %x\n", (u32)pmessage);
	AR100_INF("message type : %x\n", pmessage->type);
	AR100_INF("message attr : %x\n", pmessage->attr);
	AR100_INF("-------------------------------------------------------------\n");

	/* message per-process */
	pmessage->state = AR100_MESSAGE_PROCESSING;

	/* process message */
	switch (pmessage->type) {
		case AR100_AXP_INT_COMING_NOTIFY: {
			AR100_INF("pmu interrupt coming notify\n");
			ret = ar100_axp_int_notify(pmessage);
			pmessage->result = ret;
			break;
		}
		default : {
			AR100_ERR("invalid message type for ac327 process\n");
			ret = -EINVAL;
			break;
		}
	}
	/* message post process */
	pmessage->state = AR100_MESSAGE_PROCESSED;
	if ((pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) ||
		(pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN)) {
		/* synchronous message, should feedback process result */
		ar100_hwmsgbox_feedback_message(pmessage, AR100_SEND_MSG_TIMEOUT);
	} else {
		/*
		 * asyn message, no need feedback message result,
		 * free message directly.
		 */
		ar100_message_free(pmessage);
	}

	return ret;
}
