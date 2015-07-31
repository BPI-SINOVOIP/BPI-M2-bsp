/*
 *  arch/arm/mach-sun6i/ar100/include/ar100_bgs.h
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

#ifndef	__AR100_DBGS_H__
#define	__AR100_DBGS_H__

//debug level define,
//level 0 : dump debug information--none;
//level 1 : dump debug information--error;
//level 2 : dump debug information--error+warning;
//level 3 : dump debug information--error+warning+information;
#define DEBUG_ENABLE

//extern void printk(const char *, ...);
#if		(AR100_DEBUG_LEVEL == 0)
#define	AR100_INF(...)
#define	AR100_WRN(...)
#define	AR100_ERR(...)
#define	AR100_LOG(...)
#elif 	(AR100_DEBUG_LEVEL == 1)
#define	AR100_INF(...)
#define	AR100_WRN(...)
#define	AR100_ERR(...)		printk(__VA_ARGS__)
#define	AR100_LOG(...)		printk(__VA_ARGS__)
#elif 	(AR100_DEBUG_LEVEL == 2)
#define	AR100_INF(...)
#define	AR100_WRN(...)		printk(__VA_ARGS__)
#define	AR100_ERR(...)		printk(__VA_ARGS__)
#define	AR100_LOG(...)		printk(__VA_ARGS__)
#elif 	(AR100_DEBUG_LEVEL == 3)
#define	AR100_INF(...)		printk(__VA_ARGS__)
#define	AR100_WRN(...)		printk(__VA_ARGS__)
#define	AR100_ERR(...)		printk(__VA_ARGS__)
#define	AR100_LOG(...)		printk(__VA_ARGS__)
#endif	//AR100_DEBUG_LEVEL

#endif	//__AR100_DBGS_H__
