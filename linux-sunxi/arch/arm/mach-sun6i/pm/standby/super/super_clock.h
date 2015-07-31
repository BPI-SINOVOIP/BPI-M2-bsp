/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : super_clock.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 21:05
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __SUPER_CLOCK_H__
#define __SUPER_CLOCK_H__

#include "super_cfg.h"
#include <mach/ccmu.h>

#define REGS_BASE_PA	   			(0x01C00000)		//�Ĵ��������ַ
#define CCMU_REGS_BASE_PA         		(REGS_BASE_PA + 0x20000)    //clock manager unit

extern __u32   cpu_ms_loopcnt;

#endif  /* __SUPER_CLOCK_H__ */

