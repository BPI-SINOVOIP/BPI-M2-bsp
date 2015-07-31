/*
 *  arch/arm/mach-sun6i/ar100/interfaces/ar100_debug_level.c
 *
 * Copyright (c) 2012 Allwinner.
 * Superm (superm@allwinnertech.com)
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

/**
 * set ar100 debug level.
 * @level: ar100 debug level;
 *
 * return: 0 - set ar100 debug level successed, !0 - set ar100 debug level failed;
 */
int ar100_set_debug_level(unsigned int level)
{
	struct ar100_message *pmessage;

	/* allocate a message frame */
	pmessage = ar100_message_allocate(0);
	if (pmessage == NULL) {
		AR100_ERR("allocate message for power management request failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type     = AR100_SET_DEBUG_LEVEL;
	pmessage->paras[0] = level;
	pmessage->state    = AR100_MESSAGE_INITIALIZED;

	/* send set debug level request to ar100 */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);

	return 0;
}
