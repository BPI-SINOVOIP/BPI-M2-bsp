/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : pm.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-27 14:08
* Descript: power manager for allwinners chips platform.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/cpufreq.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/console.h>
#include <linux/mfd/axp-mfd-22.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <linux/power/aw_pm.h>
#include <asm/mach/map.h>
#include <asm/cacheflush.h>
#include "pm_i.h"

#include <mach/sys_config.h>
#include <mach/system.h>
#include <mach/ar100.h>
#include <linux/extended_standby.h>

//#define CROSS_MAPPING_STANDBY

#define AW_PM_DBG   1
#undef PM_DBG
#if(AW_PM_DBG)
    #define PM_DBG(format,args...)   printk("[pm]"format,##args)
#else
    #define PM_DBG(format,args...)   do{}while(0)
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

#ifdef ENTER_SUPER_STANDBY
#undef PRE_DISABLE_MMU
#endif

#ifdef ENTER_SUPER_STANDBY_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#undef PRE_DISABLE_MMU
#endif

#ifdef WATCH_DOG_RESET
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

//#define VERIFY_RESTORE_STATUS

/* define major number for power manager */
#define AW_PMU_MAJOR    267

static int debug_mask = 0; //PM_STANDBY_PRINT_STANDBY | PM_STANDBY_PRINT_RESUME | PM_STANDBY_ENABLE_JTAG;
static int suspend_freq = SUSPEND_FREQ;
static int suspend_delay_ms = SUSPEND_DELAY_MS;

extern char *standby_bin_start;
extern char *standby_bin_end;
extern char *suspend_bin_start;
extern char *suspend_bin_end;

#ifdef RESUME_FROM_RESUME1
extern char *resume1_bin_start;
extern char *resume1_bin_end;
#endif

/*mem_cpu_asm.S*/
extern int mem_arch_suspend(void);
extern int mem_arch_resume(void);
extern asmlinkage int mem_clear_runtime_context(void);
extern void save_runtime_context(__u32 *addr);
extern void clear_reg_context(void);

/*mem_mapping.c*/
void create_mapping(void);
void save_mapping(unsigned long vaddr);
void restore_mapping(unsigned long vaddr);

int (*mem)(void) = 0;

#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
extern void cpufreq_user_event_notify(void);
#endif

static struct aw_pm_info standby_info = {
    .standby_para = {
		.event = CPU0_WAKEUP_MSGBOX,
		.axp_event = CPUS_MEM_WAKEUP,
		.timeout = 0,
    },
    .pmu_arg = {
        .twi_port = 0,
        .dev_addr = 10,
    },
};

static struct clk_state saved_clk_state;
static __mem_tmr_reg_t saved_tmr_state;
static struct twi_state saved_twi_state;
static struct gpio_state saved_gpio_state;
static struct sram_state saved_sram_state;

#ifdef GET_CYCLE_CNT
static int start = 0;
static int resume0_period = 0;
static int resume1_period = 0;

static int pm_start = 0;
static int invalidate_data_time = 0;
static int invalidate_instruct_time = 0;
static int before_restore_processor = 0;
static int after_restore_process = 0;
//static int restore_runtime_peroid = 0;

//late_resume timing
static int late_resume_start = 0;
static int backup_area_start = 0;
static int backup_area1_start = 0;
static int backup_area2_start = 0;
static int clk_restore_start = 0;
static int gpio_restore_start = 0;
static int twi_restore_start = 0;
static int int_restore_start = 0;
static int tmr_restore_start = 0;
static int sram_restore_start = 0;
static int late_resume_end = 0;
#endif

struct aw_mem_para mem_para_info;
struct super_standby_para super_standby_para_info;

standby_type_e standby_type = NON_STANDBY;
EXPORT_SYMBOL(standby_type);
standby_level_e standby_level = STANDBY_INITIAL;
EXPORT_SYMBOL(standby_level);

//static volatile int enter_flag = 0;
static int standby_mode = 0;
static int suspend_status_flag = 0;

unsigned int normal_standby_wakesource = 0;
EXPORT_SYMBOL(normal_standby_wakesource);


/*
*********************************************************************************************************
*                           aw_pm_valid
*
*Description: determine if given system sleep state is supported by the platform;
*
*Arguments  : state     suspend state;
*
*Return     : if the state is valid, return 1, else return 0;
*
*Notes      : this is a call-back function, registered into PM core;
*
*********************************************************************************************************
*/
static int aw_pm_valid(suspend_state_t state)
{
#ifdef CHECK_IC_VERSION
	enum sw_ic_ver version = MAGIC_VER_NULL;
#endif

    PM_DBG("valid\n");

    if(!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))){
        PM_DBG("state (%d) invalid!\n", state);
        return 0;
    }

#ifdef CHECK_IC_VERSION
	if(1 == standby_mode){
			version = sw_get_ic_ver();
			if(!(MAGIC_VER_A13B == version || MAGIC_VER_A12B == version || MAGIC_VER_A10SB == version)){
				pr_info("ic version: %d not support super standby. \n", version);
				standby_mode = 0;
			}
	}
#endif

	//if 1 == standby_mode, actually, mean mem corresponding with super standby
	if(PM_SUSPEND_STANDBY == state){
		if(1 == standby_mode){
			standby_type = NORMAL_STANDBY;
		}else{
			standby_type = SUPER_STANDBY;
		}
	}else if(PM_SUSPEND_MEM == state || PM_SUSPEND_BOOTFAST == state){
		if(1 == standby_mode){
			standby_type = SUPER_STANDBY;
		}else{
			standby_type = NORMAL_STANDBY;
		}
	}

#ifdef GET_CYCLE_CNT
		// init counters:
		init_perfcounters (1, 0);
#endif

    return 1;

}


/*
*********************************************************************************************************
*                           aw_pm_begin
*
*Description: Initialise a transition to given system sleep state;
*
*Arguments  : state     suspend state;
*
*Return     : return 0 for process successed;
*
*Notes      : this is a call-back function, registered into PM core, and this function
*             will be called before devices suspened;
*********************************************************************************************************
*/
int aw_pm_begin(suspend_state_t state)
{
	int i = 0;
	struct cpufreq_policy policy;

	PM_DBG("%d state begin\n", state);

	//set freq max
#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
	//cpufreq_user_event_notify();
#endif
	
	if (cpufreq_get_policy(&policy, 0))
		goto out;

	cpufreq_driver_target(&policy, suspend_freq, CPUFREQ_RELATION_L);


	/*must init perfcounter, because delay_us and delay_ms is depandant perf counter*/
#ifndef GET_CYCLE_CNT
	backup_perfcounter();
	init_perfcounters (1, 0);
#endif

	/*init debug state*/
	i = 0;
	while(i < STATUS_REG_NUM){
		*(volatile int *)(STATUS_REG + i*4) = 0x0;
		i++;
	}
	save_mem_status(BEFORE_EARLY_SUSPEND |0x1);
	return 0;

out:
	return -1;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare
*
*Description: Prepare the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core, this function
*             will be called after devices suspended, and before device late suspend
*             call-back functions;
*********************************************************************************************************
*/
int aw_pm_prepare(void)
{
    PM_DBG("prepare\n");
	save_mem_status(BEFORE_EARLY_SUSPEND |0x2);

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare_late
*
*Description: Finish preparing the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core.
*             prepare_late is called before disabling nonboot CPUs and after
*              device drivers' late suspend callbacks have been executed;
*********************************************************************************************************
*/
int aw_pm_prepare_late(void)
{
    PM_DBG("prepare_late\n");

	save_mem_status(BEFORE_EARLY_SUSPEND |0xd);
    return 0;
}

int aw_suspend_cpu_die(void)
{
	unsigned long actlr;
	
    /* step1: disable cache */
    asm("mrc    p15, 0, %0, c1, c0, 0" : "=r" (actlr) );
    actlr &= ~(1<<2);
    asm("mcr    p15, 0, %0, c1, c0, 0\n" : : "r" (actlr));

    /* step2: clean and ivalidate L1 cache */
    flush_cache_all();

    /* step3: execute a CLREX instruction */
    asm("clrex" : : : "memory", "cc");

    /* step4: switch cpu from SMP mode to AMP mode, aim is to disable cache coherency */
    asm("mrc    p15, 0, %0, c1, c0, 1" : "=r" (actlr) );
    actlr &= ~(1<<6);
    asm("mcr    p15, 0, %0, c1, c0, 1\n" : : "r" (actlr));

    /* step5: execute an ISB instruction */
    isb();
    /* step6: execute a DSB instruction  */
    dsb();

    /* step7: execute a WFI instruction */
    asm("wfi" : : : "memory", "cc");
    
    return 0;
}

/*
*********************************************************************************************************
*                           aw_early_suspend
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      : -1: data is ok;
*			-2: data has been destory.
*********************************************************************************************************
*/
static int aw_early_suspend(void)
{
	unsigned char reg;
	unsigned char data;

#define MAX_RETRY_TIMES (5)
	const extended_standby_manager_t *extended_standby_manager = NULL;

	//backup device state
	mem_ccu_save((__ccmu_reg_list_t *)(IO_ADDRESS(AW_CCM_BASE)));
	mem_clk_save(&(saved_clk_state));
	mem_gpio_save(&(saved_gpio_state));
	mem_tmr_save(&(saved_tmr_state));
	mem_twi_save(&(saved_twi_state));
	mem_sram_save(&(saved_sram_state));

#if 0
	//backup volt and freq state, after backup device state
	mem_twi_init(AXP_IICBUS);
	/* backup voltages */
	while(-1 == (mem_para_info.suspend_dcdc2 = mem_get_voltage(POWER_VOL_DCDC2)) && --retry){
		;
	}
	if(0 == retry){
		return -1;
	}else{
		retry = MAX_RETRY_TIMES;
	}

	while(-1 == (mem_para_info.suspend_dcdc3 = mem_get_voltage(POWER_VOL_DCDC3)) && --retry){
		;
	}
	if(0 == retry){
		return -1;
	}else{
		retry = MAX_RETRY_TIMES;
	}
#endif

#if 1
	mem_clk_init(1);
	/*backup bus ratio*/
	mem_clk_getdiv(&mem_para_info.clk_div);
	/*backup pll ratio*/
	mem_clk_get_pll_factor(&mem_para_info.pll_factor);
	/*backup ccu misc*/
	mem_clk_get_misc(&mem_para_info.clk_misc);
#endif

	//backup mmu
	save_mmu_state(&(mem_para_info.saved_mmu_state));

	//backup 0x0000,0000 page entry, size?
	save_mapping(MEM_SW_VA_SRAM_BASE);

	if(DRAM_MEM_PARA_INFO_SIZE < sizeof(mem_para_info)){
		//judge the reserved space for mem para is enough or not.
		return -1;

	}

	extended_standby_manager = get_extended_standby_manager();
	extended_standby_show_state();

	//clean all the data into dram
	memcpy((void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), (void *)&mem_para_info, sizeof(mem_para_info));
	if ((NULL != extended_standby_manager) && (NULL != extended_standby_manager->pextended_standby))
		memcpy((void *)(phys_to_virt(DRAM_EXTENDED_STANDBY_INFO_PA)),
				(void *)(extended_standby_manager->pextended_standby),
				sizeof(*(extended_standby_manager->pextended_standby)));
	dmac_flush_range((void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), (void *)(phys_to_virt(DRAM_EXTENDED_STANDBY_INFO_PA) + DRAM_EXTENDED_STANDBY_INFO_SIZE));

	mem_arch_suspend();
	save_processor_state();

	//create 0x0000,0000 mapping table: 0x0000,0000 -> 0x0000,0000
	create_mapping();
	
#ifdef ENTER_SUPER_STANDBY
	//print_call_info();
	super_standby_para_info.event = mem_para_info.axp_event;
	if (NULL != extended_standby_manager) {
		super_standby_para_info.event |= extended_standby_manager->event;
		super_standby_para_info.gpio_enable_bitmap = extended_standby_manager->wakeup_gpio_map;
		super_standby_para_info.cpux_gpiog_bitmap = extended_standby_manager->wakeup_gpio_group;
	}

	if ((super_standby_para_info.event & (CPUS_WAKEUP_DESCEND | CPUS_WAKEUP_ASCEND)) == 0) {
		reg = AXP22_BUFFERB;
		data = 0x0e;
		ar100_axp_write_reg(&reg, &data, 1);
	}
#ifdef RESUME_FROM_RESUME1
	super_standby_para_info.resume_code_src = (unsigned long)(virt_to_phys((void *)&resume1_bin_start));
	super_standby_para_info.resume_code_length = ((int)&resume1_bin_end - (int)&resume1_bin_start);
	super_standby_para_info.resume_entry = SRAM_FUNC_START_PA;
	if ((NULL != extended_standby_manager) && (NULL != extended_standby_manager->pextended_standby))
		super_standby_para_info.pextended_standby = (extended_standby_t *)(DRAM_EXTENDED_STANDBY_INFO_PA);
	else
		super_standby_para_info.pextended_standby = NULL;
#endif

	super_standby_para_info.timeout = 0;

	if(unlikely(debug_mask&PM_STANDBY_PRINT_STANDBY)){
		pr_info("resume1_bin_start = 0x%x, resume1_bin_end = 0x%x. \n", (int)&resume1_bin_start, (int)&resume1_bin_end);
		pr_info("resume_code_src = 0x%lx, resume_code_length = %ld. resume_code_length = %lx \n", super_standby_para_info.resume_code_src, super_standby_para_info.resume_code_length, super_standby_para_info.resume_code_length);
	}

	//disable int to make sure the cpu0 into wfi state.
	mem_int_init();
	ar100_standby_super((struct super_standby_para *)(&super_standby_para_info), NULL, NULL);
	
	aw_suspend_cpu_die();
	pr_info("standby suspend failed\n");
	//busy_waiting();
#endif

	return -2;

}

/*
*********************************************************************************************************
*                           verify_restore
*
*Description: verify src and dest region is the same;
*
*Return     : 0: same;
*                -1: different;
*
*Notes      :
*********************************************************************************************************
*/
#ifdef VERIFY_RESTORE_STATUS
static int verify_restore(void *src, void *dest, int count)
{
	volatile char *s = (volatile char *)src;
	volatile char *d = (volatile char *)dest;

	while(count--){
		if(*(s+(count)) != *(d+(count))){
			//busy_waiting();
			return -1;
		}
	}

	return 0;
}
#endif

/*
*********************************************************************************************************
*                           aw_late_resume
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      :
*********************************************************************************************************
*/
static void aw_late_resume(void)
{
	memcpy((void *)&mem_para_info, (void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), sizeof(mem_para_info));
	mem_para_info.mem_flag = 0;
	
	//restore device state
	mem_clk_restore(&(saved_clk_state));
	mem_gpio_restore(&(saved_gpio_state));
	mem_twi_restore(&(saved_twi_state));
	mem_tmr_restore(&(saved_tmr_state));
	//mem_int_restore(&(saved_gic_state));
	mem_sram_restore(&(saved_sram_state));
	mem_ccu_restore((__ccmu_reg_list_t *)(IO_ADDRESS(AW_CCM_BASE)));

	return;
}

/*
*********************************************************************************************************
*                           aw_super_standby
*
*Description: enter super standby;
*
*Return     : return 0 is process successed;
*
*Notes      :
*********************************************************************************************************
*/
static int aw_super_standby(suspend_state_t state)
{
	int result = 0;
	int i = 0;
	suspend_status_flag = 0;

mem_enter:
	if( 1 == mem_para_info.mem_flag){
		save_mem_status(BEFORE_LATE_RESUME |0x1);
		//print_call_info();
		invalidate_branch_predictor();
		//print_call_info();
		//must be called to invalidate I-cache inner shareable?
		// I+BTB cache invalidate
		__cpuc_flush_icache_all();
		save_mem_status(BEFORE_LATE_RESUME |0x2);
		//print_call_info();
		//disable 0x0000 <---> 0x0000 mapping
		save_mem_status(BEFORE_LATE_RESUME |0x3);
		restore_processor_state();
		//destroy 0x0000 <---> 0x0000 mapping
		save_mem_status(BEFORE_LATE_RESUME |0x4);
		restore_mapping(MEM_SW_VA_SRAM_BASE);
		save_mem_status(BEFORE_LATE_RESUME |0x5);
		mem_arch_resume();
		save_mem_status(BEFORE_LATE_RESUME |0x6);
		goto resume;
	}

	save_runtime_context(mem_para_info.saved_runtime_context_svc);
	mem_para_info.mem_flag = 1;
	standby_level = STANDBY_WITH_POWER_OFF;
	mem_para_info.resume_pointer = (void *)&&mem_enter;
	mem_para_info.debug_mask = debug_mask;
	mem_para_info.suspend_delay_ms = suspend_delay_ms;
	//busy_waiting();
	if(unlikely(debug_mask&PM_STANDBY_PRINT_STANDBY)){
		pr_info("resume_pointer = 0x%x. \n", (unsigned int)(mem_para_info.resume_pointer));
	}


	/* config cpus wakeup evetn type */
	if(PM_SUSPEND_MEM == state || PM_SUSPEND_STANDBY == state){
		mem_para_info.axp_event = CPUS_MEM_WAKEUP;
	}else if(PM_SUSPEND_BOOTFAST == state){
		mem_para_info.axp_event = CPUS_BOOTFAST_WAKEUP;
	}

	result = aw_early_suspend();
	if(-2 == result){
		//mem_para_info.mem_flag = 1;
		//busy_waiting();
		suspend_status_flag = 2;
		goto mem_enter;
	}else if(-1 == result){
		suspend_status_flag = 1;
		goto suspend_err;
	}

resume:
	if(unlikely(debug_mask&PM_STANDBY_PRINT_RESUME_IO_STATUS)){
		printk("before aw_late_resume. \n");
		printk(KERN_INFO "IO status as follow:");
		for(i=0; i<(GPIO_REG_LENGTH); i++){
			printk(KERN_INFO "ADDR = %x, value = %x .\n", \
				IO_ADDRESS(AW_PIO_BASE) + i*0x04, *(volatile __u32 *)(IO_ADDRESS(AW_PIO_BASE) + i*0x04));
		}
	}
	
	aw_late_resume();

	//have been disable dcache in resume1
	//enable_cache();
	if(unlikely(debug_mask&PM_STANDBY_PRINT_RESUME)){
		print_call_info();
	}
	save_mem_status(LATE_RESUME_START |0x4);

	//before creating mapping, build the coherent between cache and memory
	//clean and flush
	__cpuc_flush_kern_all();
	__cpuc_flush_user_all();

	__cpuc_coherent_user_range(0x00000000, 0xc0000000-1);
	__cpuc_coherent_kern_range(0xc0000000, 0xffffffff-1);

suspend_err:
	if(unlikely(debug_mask&PM_STANDBY_PRINT_RESUME)){
		pr_info("suspend_status_flag = %d. \n", suspend_status_flag);
	}
	save_mem_status(LATE_RESUME_START |0x5);

	return 0;

}

/*
*********************************************************************************************************
*                           aw_pm_enter
*
*Description: Enter the system sleep state;
*
*Arguments  : state     system sleep state;
*
*Return     : return 0 is process successed;
*
*Notes      : this function is the core function for platform sleep.
*********************************************************************************************************
*/
static int aw_pm_enter(suspend_state_t state)
{
	int (*standby)(struct aw_pm_info *arg);
	int i = 0;
	unsigned char reg;
	unsigned char data;

	PM_DBG("enter state %d\n", state);

	if(unlikely(0 == console_suspend_enabled)){
		debug_mask |= (PM_STANDBY_PRINT_STANDBY | PM_STANDBY_PRINT_RESUME);
	}else{
		debug_mask &= ~(PM_STANDBY_PRINT_STANDBY | PM_STANDBY_PRINT_RESUME);
	}

	save_mem_status(BEFORE_EARLY_SUSPEND |0xf);
	
	if(unlikely(debug_mask&PM_STANDBY_PRINT_IO_STATUS)){
		printk(KERN_INFO "IO status as follow:");
		for(i=0; i<(GPIO_REG_LENGTH); i++){
			printk(KERN_INFO "ADDR = %x, value = %x .\n", \
				IO_ADDRESS(AW_PIO_BASE) + i*0x04, *(volatile __u32 *)(IO_ADDRESS(AW_PIO_BASE) + i*0x04));
		}
	}

	if(unlikely(debug_mask&PM_STANDBY_PRINT_CPUS_IO_STATUS)){
		printk(KERN_INFO "CPUS IO status as follow:");
		for(i=0; i<(CPUS_GPIO_REG_LENGTH); i++){
			printk(KERN_INFO "ADDR = %x, value = %x .\n", \
				IO_ADDRESS(AW_R_PIO_BASE) + i*0x04, *(volatile __u32 *)(IO_ADDRESS(AW_R_PIO_BASE) + i*0x04));
		}
	}

	if(unlikely(debug_mask&PM_STANDBY_PRINT_CCU_STATUS)){
		printk(KERN_INFO "CCU status as follow:");
		for(i=0; i<(CCU_REG_LENGTH); i++){
			printk(KERN_INFO "ADDR = %x, value = %x .\n", \
				IO_ADDRESS(AW_CCM_BASE) + i*0x04, *(volatile __u32 *)(IO_ADDRESS(AW_CCM_BASE) + i*0x04));
		}
	}

	if(NORMAL_STANDBY== standby_type){
		standby = (int (*)(struct aw_pm_info *arg))SRAM_FUNC_START;
		//move standby code to sram
		memcpy((void *)SRAM_FUNC_START, (void *)&standby_bin_start, (int)&standby_bin_end - (int)&standby_bin_start);
		/* config system wakeup evetn type */
		if(PM_SUSPEND_MEM == state || PM_SUSPEND_STANDBY == state){
			standby_info.standby_para.axp_event = CPUS_MEM_WAKEUP | CPUS_WAKEUP_IR;
			standby_info.standby_para.event = CPU0_MEM_WAKEUP;
		}else if(PM_SUSPEND_BOOTFAST == state){
			standby_info.standby_para.axp_event = CPUS_BOOTFAST_WAKEUP;
			standby_info.standby_para.event = CPU0_BOOTFAST_WAKEUP;
		}

		standby_info.standby_para.gpio_enable_bitmap = mem_para_info.cpus_gpio_wakeup;
		standby_info.standby_para.timeout = 0;
		standby_info.standby_para.debug_mask = debug_mask;

		// build the coherent between cache and memory
		//clean and flush: the data is already in cache, so can clean the data into sram;
		//while, in dma transfer condition, the data is not in cache, so can not use this api in dma ops.
		//at current situation, no need to clean & invalidate cache;
		//__cpuc_flush_kern_all();

		//config int src.
		mem_int_init();

		/* goto sram and run */
		standby(&standby_info);

		mem_int_exit();

		PM_DBG("platform wakeup, normal standby cpu0 wakesource is:0x%x\n, normal standby cpus wakesource is:0x%x. \n", \
			standby_info.standby_para.event, standby_info.standby_para.axp_event);
		normal_standby_wakesource = standby_info.standby_para.axp_event;

	}else if(SUPER_STANDBY == standby_type){
		aw_super_standby(state);

		ar100_cpux_ready_notify();
		ar100_query_wakeup_source((unsigned long *)(&(mem_para_info.axp_event)));
		if (mem_para_info.axp_event & (CPUS_WAKEUP_LONG_KEY)) {
			reg = AXP22_BUFFERB;
			data = 0x0;
			ar100_axp_write_reg(&reg, &data, 1);
		}
		PM_DBG("platform wakeup, super standby wakesource is:0x%x\n", mem_para_info.axp_event);	
	}

	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_wake
*
*Description: platform wakeup;
*
*Arguments  : none;
*
*Return     : none;
*
*Notes      : This function called when the system has just left a sleep state, right after
*             the nonboot CPUs have been enabled and before device drivers' early resume
*             callbacks are executed. This function is opposited to the aw_pm_prepare_late;
*********************************************************************************************************
*/
static void aw_pm_wake(void)
{
	save_mem_status(AFTER_LATE_RESUME |0x1);
    	PM_DBG("platform wakeup.\n");
	return;
}

/*
*********************************************************************************************************
*                           aw_pm_finish
*
*Description: Finish wake-up of the platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called right prior to calling device drivers' regular suspend
*              callbacks. This function is opposited to the aw_pm_prepare function.
*********************************************************************************************************
*/
void aw_pm_finish(void)
{
	save_mem_status(AFTER_LATE_RESUME |0x2);
	PM_DBG("platform wakeup finish\n");
}


/*
*********************************************************************************************************
*                           aw_pm_end
*
*Description: Notify the platform that system is in work mode now.
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called by the PM core right after resuming devices, to indicate to
*             the platform that the system has returned to the working state or
*             the transition to the sleep state has been aborted. This function is opposited to
*             aw_pm_begin function.
*********************************************************************************************************
*/
void aw_pm_end(void)
{
#ifndef GET_CYCLE_CNT
	#ifndef IO_MEASURE
			restore_perfcounter();
	#endif
#endif

	save_mem_status(AFTER_LATE_RESUME |0x4);
	PM_DBG("aw_pm_end!\n");
}


/*
*********************************************************************************************************
*                           aw_pm_recover
*
*Description: Recover platform from a suspend failure;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function alled by the PM core if the suspending of devices fails.
*             This callback is optional and should only be implemented by platforms
*             which require special recovery actions in that situation.
*********************************************************************************************************
*/
void aw_pm_recover(void)
{
	save_mem_status(AFTER_LATE_RESUME |0x3);
    PM_DBG("aw_pm_recover\n");
}


/*
    define platform_suspend_ops which is registered into PM core.
*/
static struct platform_suspend_ops aw_pm_ops = {
    .valid = aw_pm_valid,
    .begin = aw_pm_begin,
    .prepare = aw_pm_prepare,
    .prepare_late = aw_pm_prepare_late,
    .enter = aw_pm_enter,
    .wake = aw_pm_wake,
    .finish = aw_pm_finish,
    .end = aw_pm_end,
    .recover = aw_pm_recover,
};


/*
*********************************************************************************************************
*                           aw_pm_init
*
*Description: initial pm sub-system for platform;
*
*Arguments  : none;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init aw_pm_init(void)
{
	script_item_u item;
	script_item_u   *list = NULL;
	int cpu0_en = 0;
	int dram_selfresh_en = 0;
	int wakeup_src_cnt = 0;
	int wakeup_event = 0;
	unsigned gpio = 0;
	int i = 0;
	
	PM_DBG("aw_pm_init!\n");

	//get standby_mode.
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("pm_para", "standby_mode", &item)){
		pr_err("%s: script_parser_fetch err. \n", __func__);
		standby_mode = 0;
		//standby_mode = 1;
		pr_err("notice: standby_mode = %d.\n", standby_mode);
	}else{
		standby_mode = item.val;
		pr_info("standby_mode = %d. \n", standby_mode);
		if(1 != standby_mode){
			pr_err("%s: not support super standby. \n",  __func__);
		}
	}

	//get wakeup_src_para cpu_en
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("wakeup_src_para", "cpu_en", &item)){
		cpu0_en = 0;
	}else{
		cpu0_en = item.val;
	}
	pr_info("cpu0_en = %d.\n", cpu0_en);

	//get dram_selfresh en
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("wakeup_src_para", "dram_selfresh_en", &item)){
		dram_selfresh_en = 1;
	}else{
		dram_selfresh_en = item.val;
	}
	pr_info("dram_selfresh_en = %d.\n", dram_selfresh_en);

	if(0 == dram_selfresh_en && 0 == cpu0_en){
		pr_err("Notice: if u don't want the dram enter selfresh mode,\n \
				make sure the cpu0 is not allowed to be powered off.\n");
		goto script_para_err;
	}else{
		//defaultly, 0 == cpu0_en && 1 ==  dram_selfresh_en
		if(1 == cpu0_en){
			standby_mode = 0;
			pr_info("notice: only support ns, standby_mode = %d.\n", standby_mode);
		}
	}

	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("wakeup_src_para", "wakeup_event", &item)){
		wakeup_event = 0;
	}else{
		wakeup_event = item.val;
		extended_standby_enable_wakeup_src(wakeup_event, 0);
		pr_info("wakeup event is : 0x%x \n", wakeup_event);
	}
	
	//get wakeup_src_cnt
	wakeup_src_cnt = script_get_pio_list("wakeup_src_para",&list);
	pr_info("wakeup src cnt is : %d. \n", wakeup_src_cnt);

	//script_dump_mainkey("wakeup_src_para");
	mem_para_info.cpus_gpio_wakeup = 0;
	if(0 != wakeup_src_cnt){
		while(wakeup_src_cnt--){
			gpio = (list + (i++) )->gpio.gpio;
			extended_standby_enable_wakeup_src(CPUS_GPIO_SRC, gpio);
		}

		pr_info("cpus need care gpio: mem_para_info.cpus_gpio_wakeup = 0x%x. \n",\
			mem_para_info.cpus_gpio_wakeup);
	}

	suspend_set_ops(&aw_pm_ops);

	return 0;

script_para_err:
	return -1;

}


/*
*********************************************************************************************************
*                           aw_pm_exit
*
*Description: exit pm sub-system on platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      :
*
*********************************************************************************************************
*/
static void __exit aw_pm_exit(void)
{
	PM_DBG("aw_pm_exit!\n");
	suspend_set_ops(NULL);
}

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(suspend_freq, suspend_freq, int, S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(suspend_delay_ms, suspend_delay_ms, int, S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(standby_mode, standby_mode, int, S_IRUGO | S_IWUSR | S_IWGRP);
module_init(aw_pm_init);
module_exit(aw_pm_exit);

