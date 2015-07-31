/*
 *  arch/arm/mach-sun6i/ar100/hwmsgbox/hwmsgbox_i.h
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

#ifndef __AR100_HWMSGBOX_I_H
#define __AR100_HWMSGBOX_I_H

#include "../include/ar100_includes.h"

/* local functions */
irqreturn_t ar100_hwmsgbox_int_handler(int irq, void *dev);
int ar100_hwmsgbox_clear_receiver_pending(int queue, int user);
int ar100_hwmsgbox_query_receiver_pending(int queue, int user);
int ar100_hwmsgbox_enable_receiver_int(int queue, int user);
int ar100_hwmsgbox_set_receiver(int queue, int user);
int ar100_hwmsgbox_set_transmitter(int queue, int user);
int ar100_hwmsgbox_wait_message_feedback(struct ar100_message *pmessage);
int ar100_hwmsgbox_message_feedback(struct ar100_message *pmessage);
int ar100_message_valid(struct ar100_message *pmessage);

#endif  /* __AR100_HWMSGBOX_I_H */
