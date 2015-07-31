/*
 * arch/arm/mach-sun6i/include/mach/memory.h
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * liugang (liugang@allwinnertech.com)
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
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define PLAT_PHYS_OFFSET        UL(0x40000000)
#define PLAT_MEM_SIZE           (CONFIG_SUNXI_MEMORY_SIZE * SZ_1M)

#define SYS_CONFIG_MEMBASE      (PLAT_PHYS_OFFSET + SZ_32M + SZ_16M)      /* 0x43000000 */
#define SYS_CONFIG_MEMSIZE      (SZ_64K)                                  /* 0x00010000 */

#define SUPER_STANDBY_MEM_BASE  (SYS_CONFIG_MEMBASE + SYS_CONFIG_MEMSIZE) /* 0x43010000 */
#define SUPER_STANDBY_MEM_SIZE  (SZ_2K)                                 /* 2K */

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
#define SW_RAM_CONSOLE_BASE (PLAT_PHYS_OFFSET + PLAT_MEM_SIZE - SZ_64K)
#define SW_RAM_CONSOLE_SIZE  SZ_64K
#endif

#define HW_RESERVED_MEM_BASE    (0x43100000)	/* 0x43100000 */
#define HW_RESERVED_MEM_SIZE    (CONFIG_SUNXI_MEMORY_RESERVED_SIZE_1G * SZ_1M) /* 232M for A31(DE+VE(CSI)+MP: SZ_128M + SZ_64M + SZ_32M + SZ_8M)
                                                                    168M for A31s*/

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
#define ION_CARVEOUT_MEM_BASE   (HW_RESERVED_MEM_BASE + HW_RESERVED_MEM_SIZE) /* 0x51900000 */
#define ION_CARVEOUT_MEM_SIZE   (CONFIG_ION_SUNXI_CARVEOUT_SIZE_1G * SZ_1M)      /* in Mbytes */
#endif

#endif
