/*
 * arch/arm/mach-sun6i/dram-freq/mdfs/mdfs.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __MDFS_H__
#define __MDFS_H__

#include <mach/dram-freq-common.h>

#define SUART_BASE_VA   AW_VIR_UART0_BASE
#define SUART_THR       (SUART_BASE_VA + 0x00)
#define SUART_USR       (SUART_BASE_VA + 0x7c)

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

#if 1
#define MDFS_DBG(format,args...)    printk("[mdfs] "format,##args)
#else
#define MDFS_DBG(format,args...)
#endif

typedef signed int          __s32;
typedef unsigned int        __u32;
typedef int                 u32;
typedef unsigned int        size_t;

extern void __aeabi_idiv(void);
extern void __aeabi_idivmod(void);
extern void __aeabi_uidiv(void);
extern void __aeabi_uidivmod(void);
extern __s32 printk(const char *format, ...);

#endif  //__MDFS_H__

