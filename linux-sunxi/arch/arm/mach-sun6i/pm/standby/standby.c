/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 18:34
* Descript: platform standby fucntion.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"

static void restore_ccu(void);
static void backup_ccu(void);
static void destory_mmu(void);
static void restore_mmu(void);
static void cache_count_init(void);
static void cache_count_get(void);
static void cache_count_output(void);

extern char *__bss_start;
extern char *__bss_end;
extern char *__standby_start;
extern char *__standby_end;

static __u32 sp_backup;
static __u32 ttb_0r_backup = 0;
#define MMU_START	(0xc0004000)
#define MMU_END 	(0xc0007ffc) //reserve 0xffff0000 range.
__u32 mmu_backup[(MMU_END - MMU_START)>>2 + 1];

static void standby(void);

#ifdef CHECK_CACHE_TLB_MISS
int d_cache_miss_start	= 0;
int d_tlb_miss_start	= 0;
int i_tlb_miss_start	= 0;
int i_cache_miss_start	= 0;
int d_cache_miss_end	= 0;
int d_tlb_miss_end	= 0;
int i_tlb_miss_end	= 0;
int i_cache_miss_end	= 0;
#endif


/* parameter for standby, it will be transfered from sys_pwm module */
struct aw_pm_info  pm_info;
struct normal_standby_para normal_standby_para_info;

/*
*********************************************************************************************************
*                                   STANDBY MAIN PROCESS ENTRY
*
* Description: standby main process entry.
*
* Arguments  : arg  pointer to the parameter that transfered from sys_pwm module.
*
* Returns    : none
*
* Note       : the code&data may resident in cache.
*********************************************************************************************************
*/
int main(struct aw_pm_info *arg)
{
	char    *tmpPtr = (char *)&__bss_start;

	if(!arg){
		/* standby parameter is invalid */
		return -1;
	}

	/* flush data and instruction tlb, there is 32 items of data tlb and 32 items of instruction tlb,
	The TLB is normally allocated on a rotating basis. The oldest entry is always the next allocated */
	mem_flush_tlb();
	
	/* clear bss segment */
	do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);

	/* save stack pointer registger, switch stack to sram */
	sp_backup = save_sp();

	/* copy standby parameter from dram */
	standby_memcpy(&pm_info, arg, sizeof(pm_info));
	
	/* preload tlb for standby */
	mem_preload_tlb();
	
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/* init module before dram enter selfrefresh */
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    init_perfcounters(1, 0); //need double check..
	if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
		//don't need init serial ,depend kernel?
		serial_init();
		printk("pm_info.standby_para.event = 0x%x. \n", pm_info.standby_para.event);
	}

    /* initialise standby modules */
    standby_ar100_init();
    standby_clk_init();
    mem_tmr_init();

	/* init some system wake source */
	if(pm_info.standby_para.event & CPU0_WAKEUP_MSGBOX){
		if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
			printk("enable CPU0_WAKEUP_MSGBOX. \n");
		}
		mem_enable_int(INT_SOURCE_MSG_BOX);
	}
	if(pm_info.standby_para.event & CPU0_WAKEUP_KEY){
		standby_key_init();
		mem_enable_int(INT_SOURCE_LRADC);
	}

	/* process standby */
	if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_CACHE_TLB_MISS)){
		cache_count_init();
	}

	//busy_waiting();
	standby();
	
	/* check system wakeup event */
	pm_info.standby_para.event = 0;
	//actually, msg_box int will be clear by ar100-driver.
	pm_info.standby_para.event |= mem_query_int(INT_SOURCE_MSG_BOX)? 0:CPU0_WAKEUP_MSGBOX;
	pm_info.standby_para.event |= mem_query_int(INT_SOURCE_LRADC)? 0:CPU0_WAKEUP_KEY;

	//restore intc config.
	if(pm_info.standby_para.event & CPU0_WAKEUP_KEY){
		standby_key_exit();
	}

	/*check completion status: only after restore completion, access dram is allowed. */
	while(standby_ar100_check_restore_status()){
		if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
			printk("0xf1c20050 value: 0x%x. \n", *((volatile unsigned int *)0xf1c20050));
			printk("0xf1c20000 value: 0x%x. \n", *((volatile unsigned int *)0xf1c20000));
		}
		;
	}
	
	if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_CACHE_TLB_MISS)){
		cache_count_get();
		if(d_cache_miss_end || d_tlb_miss_end || i_tlb_miss_end || i_cache_miss_end){
		printk("=============================NOTICE====================================. \n");
		cache_count_output();
		}else{
			printk("no miss. \n");
			//cache_count_output();
		}
	}
	
	/* disable watch-dog */
	mem_tmr_disable_watchdog();
	if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
		printk("after mem_tmr_disable_watchdog. \n");
	}

	/* exit standby module */
	mem_tmr_exit();
	standby_clk_exit();
	standby_ar100_exit();
	
	if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
		printk("after standby_ar100_exit. \n");
	}
	/* restore stack pointer register, switch stack back to dram */
	restore_sp(sp_backup);

	if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
		printk("after restore_sp. \n");
	}
    
	if(unlikely(pm_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
               //restore serial clk & gpio config.
               serial_exit();
        }


	/* report which wake source wakeup system */
	arg->standby_para.event = pm_info.standby_para.event;
	arg->standby_para.axp_event = pm_info.standby_para.axp_event;

	//enable_cache();
	
	return 0;
}

/*
*********************************************************************************************************
*                                     SYSTEM PWM ENTER STANDBY MODE
*
* Description: enter standby mode.
*
* Arguments  : none
*
* Returns    : none;
*********************************************************************************************************
*/
static void standby(void)
{
	/*backup clk freq and voltage*/
	backup_ccu();
	
	/*notify ar100 enter normal standby*/
	normal_standby_para_info.event = pm_info.standby_para.axp_event;
	normal_standby_para_info.timeout = pm_info.standby_para.timeout;
	normal_standby_para_info.gpio_enable_bitmap = pm_info.standby_para.gpio_enable_bitmap;
	
	
	standby_ar100_standby_normal((&normal_standby_para_info));

	/* cpu enter sleep, wait wakeup by interrupt */
	asm("WFI");

	/*restore cpu0 ccu: enable hosc and change to 24M. */
	restore_ccu();

	/*query wakeup src*/
	standby_ar100_query_wakeup_src((unsigned long *)&(pm_info.standby_para.axp_event));
	/* enable watch-dog to prevent in case dram training failed */
	mem_tmr_enable_watchdog();
	
	/* notify for cpus to: restore cpus freq and volt, restore dram */
	standby_ar100_notify_restore(STANDBY_AR100_ASYNC);	

	return;
}

static void backup_ccu(void)
{
	return;
}

/*change clk src to hosc*/
static void restore_ccu(void)
{
	
#if(ALLOW_DISABLE_HOSC)
		/* enable LDO, ldo1, enable HOSC */
		standby_clk_ldoenable();
		standby_clk_pll1enable();
		/* delay 10ms for power be stable */
		standby_delay_cycle(1); //?ms
		//switch to 24M src
		standby_clk_core2hosc();
#endif
	
		return;
}

/*
*********************************************************************************************************
*                                    destory_mmu
*
* Description: to destory the mmu mapping, so, the tlb miss will result in an data/cache abort 
*              while not accessing dram.
* Arguments  : none
*
* Returns    : none;
*********************************************************************************************************
*/
static void destory_mmu(void)
{
	__u32 ttb_1r = 0;
	int i = 0;
	volatile  __u32 * p_mmu = (volatile  __u32 *)MMU_START;
	
	for(p_mmu = (volatile  __u32 *)MMU_START; p_mmu < (volatile  __u32 *)MMU_END; p_mmu++, i++)
	{		
		mmu_backup[i] = *p_mmu;	
		*p_mmu = 0;			
	}
	flush_dcache();

	//u need to set ttbr0 to 0xc0004000?
	//backup
	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttb_0r_backup));
	//get ttbr1
	asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r"(ttb_1r));
	//use ttbr1 to set ttbr0
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttb_1r));
	asm volatile ("dsb");
	asm volatile ("isb");

	return;
}

static void restore_mmu(void)
{
	volatile  __u32 * p_mmu = (volatile  __u32 *)MMU_START;
	int i = 0;
	
	//restore ttbr0
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttb_0r_backup));
	asm volatile ("dsb");
	asm volatile ("isb");

	for(p_mmu = (volatile  __u32 *)MMU_START; p_mmu < (volatile  __u32 *)MMU_END; p_mmu++, i++)
	{
			*p_mmu = mmu_backup[i];			
	}

	flush_dcache();
	return;
}

#ifdef CHECK_CACHE_TLB_MISS

static void cache_count_init(void)
{
	set_event_counter(D_CACHE_MISS);
	set_event_counter(D_TLB_MISS);
	set_event_counter(I_CACHE_MISS);
	set_event_counter(I_TLB_MISS);
	init_event_counter(1, 0);
	d_cache_miss_start = get_event_counter(D_CACHE_MISS);
	d_tlb_miss_start = get_event_counter(D_TLB_MISS);
	i_tlb_miss_start = get_event_counter(I_TLB_MISS);
	i_cache_miss_start = get_event_counter(I_CACHE_MISS);

	return;
}

static void cache_count_get(void)
{
	d_cache_miss_end = get_event_counter(D_CACHE_MISS);
	d_tlb_miss_end = get_event_counter(D_TLB_MISS);
	i_tlb_miss_end = get_event_counter(I_TLB_MISS);
	i_cache_miss_end = get_event_counter(I_CACHE_MISS);

	return;
}

static void cache_count_output(void)
{
	printk("d_cache_miss_start = %d, d_cache_miss_end= %d. \n", d_cache_miss_start, d_cache_miss_end);
	printk("d_tlb_miss_start = %d, d_tlb_miss_end= %d. \n", d_tlb_miss_start, d_tlb_miss_end);
	printk("i_cache_miss_start = %d, i_cache_miss_end= %d. \n", i_cache_miss_start, i_cache_miss_end);
	printk("i_tlb_miss_start = %d, i_tlb_miss_end= %d. \n", i_tlb_miss_start, i_tlb_miss_end);

	return;
}

#else
static void cache_count_init(void)
{
	return;
}

static void cache_count_get(void)
{
	return;
}

static void cache_count_output(void)
{
	return;
}


#endif

