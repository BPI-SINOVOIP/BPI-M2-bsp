/*
 *  arch/arm/mach-sun6i/ar100/include/ar100_message_manager.h
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

#ifndef __AR100_MESSAGE_MANAGER_H
#define __AR100_MESSAGE_MANAGER_H

/**
 * initialize message manager.
 * @para:  none.
 *
 * returns:  OK if initialize succeeded, others if failed.
 */
int ar100_message_manager_init(void);

/**
 * exit message manager.
 * para:  none.
 *
 * returns:  OK if exit succeeded, others if failed.
 */
int ar100_message_manager_exit(void);

/**
 * allocate one message frame. mainly use for send message by message-box,
 * the message frame allocate form messages pool shared memory area.
 * @para:  none.
 *
 * returns:  the pointer of allocated message frame, NULL if failed;
 */
struct ar100_message *ar100_message_allocate(unsigned int msg_attr);

/**
 * free one message frame. mainly use for process message finished,
 * free it to messages pool or add to free message queue.
 * @pmessage:  the pointer of free message frame.
 *
 * returns:  none.
 */
void ar100_message_free(struct ar100_message *pmessage);

/**
 * notify system that one message coming.
 * @pmessage : the pointer of coming message frame.
 *
 * returns:  OK if notify succeeded, other if failed.
 */
int ar100_message_coming_notify(struct ar100_message *pmessage);

int ar100_semaphore_used_num_query(void);

#endif  /* __MESSAGE_MANAGER_H */
