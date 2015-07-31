/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2012-2015, young.gq China
*                                             All Rights Reserved
*
* File    : super_cpus.h
* By      : young.gq
* Version : v1.0
* Date    : 2013-3-30 17:21
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __SUPER_CPUS_H__
#define __SUPER_CPUS_H__
#include "super_i.h"

#define CPU_RESET_DEASSERT 		(0x3)
#define CPU_RESET_MASK 			(0x3)
#define	CPUCFG_CPU0        		(0)
#define	CPUCFG_CPU1        		(1)
#define	CPUCFG_CPU2        		(2)
#define	CPUCFG_CPU3        		(3)
#define CPUCFG_CPU_NUMBER		(CPUCFG_CPU3 + 1)

#define CPUX_STARTUP_ADDR		(0x0)

#define CPUX_INVALIDATION_DONE_FLAG		(0xff)
#define CPUX_INVALIDATION_DONE_FLAG_REG(n) 	(STATUS_REG_PA + 0x04 + (n)*0x4)
#define CPUX_INVALIDATION_COMPLETION_FLAG_REG 	(CPUX_INVALIDATION_DONE_FLAG_REG(CPUCFG_CPU_NUMBER))

//function declartion
__s32 cpucfg_set_cpu_reset_state(__u32 cpu_num, __s32 state);
void super_enable_aw_cpu(int cpu);

#endif  //__SUPER_CPUS_H__

