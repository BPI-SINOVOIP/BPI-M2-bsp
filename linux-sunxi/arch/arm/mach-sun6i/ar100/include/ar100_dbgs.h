/*
 *  arch/arm/mach-sun6i/ar100/include/ar100_dbgs.h
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

#ifndef __AR100_DBGS_H
#define __AR100_DBGS_H

extern unsigned int ar100_debug_level;
/*
 * debug level define,
 * level 0 : dump debug information--none;
 * level 1 : dump debug information--error;
 * level 2 : dump debug information--error+warning;
 * level 3 : dump debug information--error+warning+information;
 * extern void printk(const char *, ...);
 */

#ifdef AR100_DEBUG_ON
/* debug levels */
#define DEBUG_LEVEL_INF    ((u32)1 << 0)
#define DEBUG_LEVEL_WRN    ((u32)1 << 1)
#define DEBUG_LEVEL_ERR    ((u32)1 << 2)
#define DEBUG_LEVEL_LOG    ((u32)1 << 3)

#define AR100_INF(...)                          \
	if(DEBUG_LEVEL_INF & (0xf0 >> (ar100_debug_level +1)))  \
	printk(__VA_ARGS__)

#define AR100_WRN(format, args...)                          \
	if(DEBUG_LEVEL_WRN & (0xf0 >> (ar100_debug_level +1)))  \
	printk(KERN_ERR "AR100 WARING :"format,##args);

#define AR100_ERR(format, args...)                          \
	if(DEBUG_LEVEL_ERR & (0xf0 >> (ar100_debug_level +1)))  \
	printk(KERN_ERR "AR100 ERROR :"format,##args);

#define AR100_LOG(...)                                      \
	if(DEBUG_LEVEL_LOG & (0xf0 >> (ar100_debug_level +1)))  \
	printk(__VA_ARGS__)

#else /* AR100_DEBUG_ON */
#define AR100_INF(...)
#define AR100_WRN(...)
#define AR100_ERR(...)
#define AR100_LOG(...)

#endif /* AR100_DEBUG_ON */

#endif /* __AR100_DBGS_H */
