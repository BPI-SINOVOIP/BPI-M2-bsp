/*
 *  arch/arm/mach-sun6i/pm/standby/ar100/standby_ar100_i.h
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

#ifndef	__STANDBY_AR100_I_H__
#define	__STANDBY_AR100_I_H__

#include <linux/power/aw_pm.h>
#include <mach/platform.h>
#include <mach/hwmsgbox.h>
#include <mach/hwspinlock.h>
#include "ar100_cfgs.h"
#include "ar100_messages.h"
#include "ar100_dbgs.h"
#include "standby_ar100.h"
#include "standby_i.h"
#include "asm-generic/errno-base.h"

extern unsigned long ar100_sram_a2_vbase;

//hwspinlock interfaces
int ar100_hwspinlock_init(void);
int ar100_hwspinlock_exit(void);
int ar100_hwspin_lock_timeout(int hwid, unsigned int timeout);
int ar100_hwspin_unlock(int hwid);

//hwmsgbox interfaces
int ar100_hwmsgbox_init(void);
int ar100_hwmsgbox_exit(void);
int ar100_hwmsgbox_send_message(struct ar100_message *pmessage, unsigned int timeout);
int ar100_hwmsgbox_feedback_message(struct ar100_message *pmessage, unsigned int timeout);
struct ar100_message *ar100_hwmsgbox_query_message(void);

//message manager interfaces
int ar100_message_manager_init(void);
int ar100_message_manager_exit(void);
struct ar100_message *ar100_message_allocate(unsigned int msg_attr);
void ar100_message_free(struct ar100_message *pmessage);

#endif	//__STANDBY_AR100_I_H__
