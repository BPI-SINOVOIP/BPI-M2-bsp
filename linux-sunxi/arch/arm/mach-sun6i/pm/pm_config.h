#ifndef _PM_CONFIG_H
#define _PM_CONFIG_H

#include "mach/memory.h"
#include "asm-generic/sizes.h"
//#include <generated/autoconf.h>
#define CONFIG_AW_ASIC_EVB_PLATFORM 1

/*
* Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published by
* the Free Software Foundation.
*/
#ifdef CONFIG_ARCH_SUN6I
#undef CONFIG_ARCH_SUN6I
#endif

#define CONFIG_ARCH_SUN6I
#define ENABLE_SUPER_STANDBY

#ifndef CONFIG_AW_ASIC_EVB_PLATFORM
#define SUN6I_FPGA_SIM
#else
#if 0 == CONFIG_AW_ASIC_EVB_PLATFORM 
#define SUN6I_FPGA_SIM
#endif
#endif


//#define CHECK_IC_VERSION

//#define RETURN_FROM_RESUME0_WITH_MMU    //suspend: 0xf000, resume0: 0xc010, resume1: 0xf000
//#define RETURN_FROM_RESUME0_WITH_NOMMU // suspend: 0x0000, resume0: 0x4010, resume1: 0x0000
//#define DIRECT_RETURN_FROM_SUSPEND //not support yet
#define ENTER_SUPER_STANDBY    //suspend: 0xf000, resume0: 0x4010, resume1: 0x0000
//#define ENTER_SUPER_STANDBY_WITH_NOMMU //not support yet, suspend: 0x0000, resume0: 0x4010, resume1: 0x0000
//#define WATCH_DOG_RESET

//NOTICE: only need one definiton
#define RESUME_FROM_RESUME1

/**start address for function run in sram*/
#define SRAM_FUNC_START    	(0xf0000000)
#define SRAM_FUNC_START_PA 	(0x00000000)

#define DRAM_BASE_ADDR      	(0xc0000000)
#define DRAM_BASE_ADDR_PA	(0x40000000)
#define DRAM_TRANING_SIZE   	(16)

#define DRAM_MEM_PARA_INFO_PA			(SUPER_STANDBY_MEM_BASE)	//0x43010000, 0x43010000+2k;
#define DRAM_MEM_PARA_INFO_SIZE			((SUPER_STANDBY_MEM_SIZE)>>1) 	//DRAM_MEM_PARA_INFO_SIZE = 1K bytes. 

#define DRAM_EXTENDED_STANDBY_INFO_PA		(DRAM_MEM_PARA_INFO_PA + DRAM_MEM_PARA_INFO_SIZE)
#define DRAM_EXTENDED_STANDBY_INFO_SIZE		((SUPER_STANDBY_MEM_SIZE)>>1)

#define RUNTIME_CONTEXT_SIZE (14 * sizeof(__u32)) //r0-r13

#define DRAM_COMPARE_DATA_ADDR (0xc0100000) //1Mbytes offset
#define DRAM_COMPARE_SIZE (0x10000) //?

//for mem mapping
#define MEM_SW_VA_SRAM_BASE (0x00000000)
#define MEM_SW_PA_SRAM_BASE (0x00000000)

#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _AT(T,X)	((T)(X))
#define UL(x) _AC(x, UL)
#define IO_ADDRESS(x)		((x) + 0xf0000000)

#define SUSPEND_FREQ (720000)	//720M
#define SUSPEND_DELAY_MS (10)

#define AW_JTAG_GPIO_PA		(0x01c20800 + 0x100)
#define AW_JTAG_CONFIG_VAL	(0x00033330)
#endif /*_PM_CONFIG_H*/
