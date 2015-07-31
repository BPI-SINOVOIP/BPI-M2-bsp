/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : super_i.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 17:21
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __SUPER_I_H__
#define __SUPER_I_H__

#include "../../pm_config.h"
#include "../../pm_types.h"
#include "../../pm.h"
#include <linux/power/aw_pm.h>
#include <mach/platform.h>

#include "super_cfg.h"
#include "common.h"
#include "super_clock.h"
#include "super_power.h"
#include "super_twi.h"
#include "super_cpus.h"

//------------------------------------------------------------------------------
//return value defines
//------------------------------------------------------------------------------
#define	OK		(0)
#define	FAIL	(-1)
#define TRUE	(1)
#define	FALSE	(0)

#define NULL	(0)

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned short *)(addr)))
#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

extern struct aw_pm_info  pm_info;


#endif  //__SUPER_I_H__

