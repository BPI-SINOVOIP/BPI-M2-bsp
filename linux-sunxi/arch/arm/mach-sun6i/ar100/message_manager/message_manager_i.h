/*
 *  arch/arm/mach-sun6i/ar100/message_manager/message_manager_i.h
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

#ifndef __AR100_MESSAGE_MANAGER_I_H
#define __AR100_MESSAGE_MANAGER_I_H

#include "../include/ar100_includes.h"
#include <asm/atomic.h>

#define AR100_SEM_CACHE_MAX (8)

struct ar100_semaphore_cache
{
	atomic_t          number;
	struct semaphore *cache[AR100_SEM_CACHE_MAX];
};

/*
 *the strcuture of message cache,
 *main for messages cache management.
 */
typedef struct ar100_message_cache
{
	atomic_t              number;                           /* valid message number */
	struct ar100_message *cache[AR100_MESSAGE_CACHED_MAX];  /* message cache table */
} ar100_message_cache_t;

#endif  /* __AR100_MESSAGE_MANAGER_I_H */
