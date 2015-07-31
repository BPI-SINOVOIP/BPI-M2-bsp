/*
 *  arch/arm/mach-sun6i/ar100/include/ar100_cfgs.h
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

#ifndef	__AR100_CFGS_H__
#define __AR100_CFGS_H__

#ifndef NULL
#define NULL 0
#endif

//debugger system
#define	AR100_DEBUG_LEVEL			(3)		//debug level

//the max number of cached message frame
#define	AR100_MESSAGE_CACHED_MAX	(4)

//the start address of message pool
#define AR100_MESSAGE_POOL_START	(0x13000)
#define AR100_MESSAGE_POOL_END		(0x14000)

//spinlock max timeout, base on ms
#define AR100_SPINLOCK_TIMEOUT		(10)

//send message max timeout, base on ms
#define AR100_SEND_MSG_TIMEOUT		(10)

//hwmsgbox channels configure
#define	AR100_HWMSGBOX_AR100_ASYN_TX_CH	(0)
#define	AR100_HWMSGBOX_AR100_ASYN_RX_CH	(1)
#define	AR100_HWMSGBOX_AR100_SYN_TX_CH	(2)
#define	AR100_HWMSGBOX_AR100_SYN_RX_CH	(3)
#define	AR100_HWMSGBOX_AC327_SYN_TX_CH	(4)
#define	AR100_HWMSGBOX_AC327_SYN_RX_CH	(5)

#endif //__AR100_CFGS_H__
