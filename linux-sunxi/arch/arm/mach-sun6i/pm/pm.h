#ifndef _PM_H
#define _PM_H

/*
 * Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

//#include "pm_types.h" 
#include "pm_config.h"
#include "pm_errcode.h"
#include "pm_debug.h"
#include "mem_cpu.h"
#include "mem_serial.h"
#include "mem_printk.h"
#include "mach/platform.h"
#include "mem_divlibc.h"
#include "mem_int.h"
#include "mem_tmr.h"
#include <mach/ccmu.h>
#include "mem_clk.h"
#include "mem_timing.h"

typedef enum{
	PM_STANDBY_PRINT_STANDBY = (1U << 0),
	PM_STANDBY_PRINT_RESUME = (1U << 1),
	PM_STANDBY_PRINT_IO_STATUS = (1U << 2),
	PM_STANDBY_PRINT_CACHE_TLB_MISS = (1U << 3),
	PM_STANDBY_PRINT_CCU_STATUS = (1U << 4),
	PM_STANDBY_PRINT_CPUS_IO_STATUS = (1U << 5),
	PM_STANDBY_PRINT_RESUME_IO_STATUS = (1U << 6),
	PM_STANDBY_ENABLE_JTAG		= (1U << 7)
}debug_mask_flag;

#ifdef CONFIG_ARCH_SUN4I
#define INT_REG_LENGTH	((0x90+0x4)>>2)
#define GPIO_REG_LENGTH	((0x218+0x4)>>2)
#define SRAM_REG_LENGTH	((0x94+0x4)>>2)
#elif defined CONFIG_ARCH_SUN5I
#define INT_REG_LENGTH	((0x94+0x4)>>2)
#define GPIO_REG_LENGTH	((0x218+0x4)>>2)
#define SRAM_REG_LENGTH	((0x94+0x4)>>2)
#elif defined CONFIG_ARCH_SUN6I
#define GPIO_REG_LENGTH		((0x278+0x4)>>2)
#define CPUS_GPIO_REG_LENGTH	((0x238+0x4)>>2)
#define SRAM_REG_LENGTH		((0x94+0x4)>>2)
#define CCU_REG_LENGTH		((0x308+0x4)>>2)

#elif defined CONFIG_ARCH_SUN7I
#define GPIO_REG_LENGTH	((0x218+0x4)>>2)
#define SRAM_REG_LENGTH	((0x94+0x4)>>2)
#endif

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

struct mmu_state {
	/* CR0 */
	__u32 cssr;	/* Cache Size Selection */
	/* CR1 */
	__u32 cr;		/* Control */
	__u32 cacr;	/* Coprocessor Access Control */
	/* CR2 */
	__u32  ttb_0r;	/* Translation Table Base 0 */
	__u32  ttb_1r;	/* Translation Table Base 1 */
	__u32  ttbcr;	/* Translation Talbe Base Control */
	
	/* CR3 */
	__u32 dacr;	/* Domain Access Control */

	/*cr10*/
	__u32 prrr;	/* Primary Region Remap Register */
	__u32 nrrr;	/* Normal Memory Remap Register */
};

/**
*@brief struct of super mem
*/
struct aw_mem_para{
	void **resume_pointer;
	volatile __u32 mem_flag;
	__u32 axp_event;
	__u32 sys_event;
	__u32 cpus_gpio_wakeup;
	__u32 debug_mask;
	__u32 suspend_delay_ms;
	__u32 saved_runtime_context_svc[RUNTIME_CONTEXT_SIZE];
	struct clk_div_t clk_div;
	struct clk_misc_t clk_misc;	//miscellaneous para.
	struct pll_factor_t pll_factor;
	struct mmu_state saved_mmu_state;
	struct saved_context saved_cpu_context;
};

typedef  int (*suspend_func)(void);

/*mem_mmu_pc_asm.S*/
extern unsigned int save_sp_nommu(void);
extern unsigned int save_sp(void);
extern void clear_reg_context(void);
extern void restore_sp(unsigned int sp);

//cache
extern void invalidate_dcache(void);
extern void flush_icache(void);
extern void flush_dcache(void);
extern void disable_cache(void);
extern void disable_dcache(void);
extern void disable_l2cache(void);
extern void enable_cache(void);
extern void enable_icache(void);

extern void disable_program_flow_prediction(void);
extern void invalidate_branch_predictor(void);
extern void enable_program_flow_prediction(void);

extern void mem_flush_tlb(void);
extern void mem_preload_tlb(void);

void disable_mmu(void);
void enable_mmu(void);

extern int jump_to_resume(void* pointer, __u32 *addr);
extern int jump_to_resume0(void* pointer);
void jump_to_suspend(__u32 ttbr1, suspend_func p);
extern int jump_to_resume0_nommu(void* pointer);

/*mmu_pc.c*/
extern void save_mmu_state(struct mmu_state *saved_mmu_state);
extern void restore_mmu_state(struct mmu_state *saved_mmu_state);
void set_ttbr0(void);
extern void invalidate_dcache(void);

#endif /*_PM_H*/

