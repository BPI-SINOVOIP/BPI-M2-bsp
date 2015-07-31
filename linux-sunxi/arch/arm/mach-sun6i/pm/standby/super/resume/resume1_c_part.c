/* these code will be removed to sram.
 * function: open the mmu, and jump to dram, for continuing resume*/
#include "./../super_i.h"


static struct aw_mem_para mem_para_info;

extern char *__bss_start;
extern char *__bss_end;
static __s32 dcdc2, dcdc3;
static __u32 sp_backup;
static char    *tmpPtr = (char *)&__bss_start;
static __u32 status = 0; 

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#define MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#undef MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#if defined(ENTER_SUPER_STANDBY) || defined(ENTER_SUPER_STANDBY_WITH_NOMMU) || defined(WATCH_DOG_RESET)
#undef MMU_OPENED
#define POWER_OFF
#define FLUSH_TLB
#define SET_COPRO_REG
//#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#define IS_WFI_MODE(cpu)	(*(volatile unsigned int *)((((0x01f01c00)) + (0x48 + (cpu)*0x40))) & (1<<2))


int resume1_c_part(void)
{

#ifdef SET_COPRO_REG
	save_mem_status_nommu(RESUME1_START |0x01);
	set_copro_default();
#endif

#ifdef MMU_OPENED
	save_mem_status(RESUME1_START |0x02);

	//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
	mem_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1), sizeof(mem_para_info));
#else
	save_mem_status_nommu(RESUME1_START |0x02);

	if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
		//config uart
		serial_puts_nommu("resume1: 0. \n");
	}
#if 1
	/*restore freq from 408M to orignal freq.*/
	//busy_waiting();
	mem_clk_setdiv(&mem_para_info.clk_div);
	mem_clk_set_pll_factor(&mem_para_info.pll_factor);
	change_runtime_env(0);
	delay_ms(mem_para_info.suspend_delay_ms);
	
	if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
		serial_puts_nommu("resume1: 1. before restore mmu. \n");
	}
#endif	
	/*restore mmu configuration*/
	save_mem_status_nommu(RESUME1_START |0x03);
	//save_mem_status(RESUME1_START |0x03);

	restore_mmu_state(&(mem_para_info.saved_mmu_state));
	save_mem_status(RESUME1_START |0x13);

#endif

//before jump to late_resume	
#ifdef FLUSH_TLB
	save_mem_status(RESUME1_START |0x9);
	mem_flush_tlb();
#endif

#ifdef FLUSH_ICACHE
	save_mem_status(RESUME1_START |0xa);
	flush_icache();
#endif

	if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
		serial_puts("resume1: 3. after restore mmu, before jump.\n");
	}

	//busy_waiting();
	jump_to_resume((void *)mem_para_info.resume_pointer, mem_para_info.saved_runtime_context_svc);

	return;
}


/*******************************************************************************
* interface : set_pll
*	prototype		£ºvoid set_pll( void )
*	function		: adjust CPU frequence, from 24M hosc to pll1 384M
*	input para	: void
*	return value	: void
*	note:
*******************************************************************************/
void set_pll( void )
{
	__ccmu_reg_list_t   *CmuReg;
	__u32 cpu_id = 0;
	__u32 cpu1_reset = 0;
	__u32 cpu2_reset = 0;
	__u32 cpu3_reset = 0;
	__u32 pwr_reg = 0;
	__u32 mva_addr = 0x00000000;

	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r"(cpu_id)); //Read CPU ID register
	cpu_id &= 0x3;
	if(0 == cpu_id){
		/* clear bss segment */
		do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);

		/*when enter this func, state is as follow:
		*	1. mmu is disable.
		*	2. clk is 24M hosc (?)
		*
		*/
		CmuReg = (__ccmu_reg_list_t   *)mem_clk_init(0);


		save_mem_status_nommu(RESUME1_START |0x26 | (cpu_id<<8));

		//switch to 24M
		*(volatile __u32 *)(&CmuReg->SysClkDiv) = 0x00010000;

		//get mem para info
		//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
		mem_memcpy((void *)&mem_para_info, (void *)(DRAM_MEM_PARA_INFO_PA), sizeof(mem_para_info));
		//config jtag gpio
		//need to config gpio clk? apb1 clk gating?
		if(unlikely(mem_para_info.debug_mask&PM_STANDBY_ENABLE_JTAG)){
			*(volatile __u32 * )(AW_JTAG_GPIO_PA) = AW_JTAG_CONFIG_VAL;
			save_mem_status_nommu(RESUME1_START |0x27 | (cpu_id<<8));
		}
		//config pll para
		mem_clk_set_misc(&mem_para_info.clk_misc);
		//enable pll1 and setting PLL1 to 408M
		*(volatile __u32 *)(&CmuReg->Pll1Ctl) = (0x00001011) | (0x80000000); //N = 16, K=M=2 -> pll1 = 17*24 = 408M
		//setting pll6 to 600M
		//enable pll6
		*(volatile __u32 *)(&CmuReg->Pll6Ctl) = 0x80041811;

		init_perfcounters(1, 0); //need double check..
		change_runtime_env(0);
		delay_ms(10);	

		if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
			//config uart
			serial_init_nommu();
		}
	}else{
		/* execute a TLBIMVAIS operation to addr: 0x0000,0000 */
		asm volatile ("mcr p15, 0, %0, c8, c3, 1" : : "r"(mva_addr));
		asm volatile ("dsb");
		//printk_nommu("cpu_id = %x. \n", cpu_id);
		//set invalidation done flag
		writel(CPUX_INVALIDATION_DONE_FLAG, CPUX_INVALIDATION_DONE_FLAG_REG(cpu_id));
		//dsb
		asm volatile ("dsb");
		asm volatile ("SEV");
	} 

	
#if 1
	if(0 == cpu_id){
		//step2: clear completion flag.
		writel(0, CPUX_INVALIDATION_COMPLETION_FLAG_REG);
		//step3: clear completion done flag for each cpux
		writel(0, CPUX_INVALIDATION_DONE_FLAG_REG(CPUCFG_CPU1));
		writel(0, CPUX_INVALIDATION_DONE_FLAG_REG(CPUCFG_CPU2));
		writel(0, CPUX_INVALIDATION_DONE_FLAG_REG(CPUCFG_CPU3));

		//step4: dsb 			
		asm volatile ("dsb");
		
		//step5: power up other cpus.
		super_enable_aw_cpu(CPUCFG_CPU1);
		super_enable_aw_cpu(CPUCFG_CPU2);
		super_enable_aw_cpu(CPUCFG_CPU3);

		//step7: check cpux's invalidation done flag.
		while(1){
			//step6 or 8: wfe
			asm volatile ("wfe");
			save_mem_status_nommu(RESUME1_START |0x35 | (cpu_id<<8));

			if(CPUX_INVALIDATION_DONE_FLAG == readl(CPUX_INVALIDATION_DONE_FLAG_REG(CPUCFG_CPU1)) && \
					CPUX_INVALIDATION_DONE_FLAG == readl(CPUX_INVALIDATION_DONE_FLAG_REG(CPUCFG_CPU2)) && \
					CPUX_INVALIDATION_DONE_FLAG == readl(CPUX_INVALIDATION_DONE_FLAG_REG(CPUCFG_CPU3)) ){
						//step9: set completion flag.
						writel(CPUX_INVALIDATION_DONE_FLAG, CPUX_INVALIDATION_COMPLETION_FLAG_REG);

						//step10: dsb
						asm volatile ("dsb");
						//sev
						asm volatile ("sev");
						break;
			}
		}
		save_mem_status_nommu(RESUME1_START |0x39 | (cpu_id<<8));

		//step 11: normal power up.
		while(1){
			if((IS_WFI_MODE(1) && IS_WFI_MODE(2) && IS_WFI_MODE(3))){
				save_mem_status_nommu(RESUME1_START |0x3a | (cpu_id<<8));
				/* step9: set up cpu1+ power-off signal */
				//printk_nommu("set up cpu1+ power-off signal.\n");
				pwr_reg = (*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG));
				pwr_reg |= (0xe); //0b1110
				(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG)) = pwr_reg;
				delay_ms(1);

				save_mem_status_nommu(RESUME1_START |0x3b | (cpu_id<<8));
				/* step10: active the power output clamp */
				//printk_nommu("active the power output clamp.\n");
				(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(1))) = 0xff;
				(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(2))) = 0xff;
				(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(3))) = 0xff;
				save_mem_status_nommu(RESUME1_START |0x3c | (cpu_id<<8));
									
				break;
			}
			if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){			
				printk_nommu("cpu1+ wfi state as follow: \n");
				printk_nommu("cpu1 wfi = %d. \n", IS_WFI_MODE(1));
				printk_nommu("cpu2 wfi = %d. \n", IS_WFI_MODE(2));
				printk_nommu("cpu3 wfi = %d. \n", IS_WFI_MODE(3));
			}


		}
		//printk_nommu("cpu1 go on wakeup the system...\n");

	}else{
		
		//just waiting until the completion flag be seted..
		while(1){
			/* step: execute a WFE instruction  */
			asm volatile ("wfe");
			save_cpux_mem_status_nommu(RESUME1_START |0x45 | (cpu_id<<8));
			if(CPUX_INVALIDATION_DONE_FLAG == readl(CPUX_INVALIDATION_COMPLETION_FLAG_REG)){
				break;
			}
		}

		save_cpux_mem_status_nommu(RESUME1_START |0x46 | (cpu_id<<8));
		//normal power down sequence.
		while(1){		
			//let the cpu1+ enter wfi state;
			/* step3: execute a CLREX instruction */
			asm("clrex" : : : "memory", "cc");

			/* step5: execute an ISB instruction */
			asm volatile ("isb");
			/* step6: execute a DSB instruction  */
			asm volatile ("dsb");

			save_cpux_mem_status_nommu(RESUME1_START |0x47 | (cpu_id<<8));
			/* step7: execute a WFI instruction */
			while(1) {
				asm("wfi" : : : "memory", "cc");
			}

		}
	}
#endif
	//switch to PLL1
	*(volatile __u32 *)(&CmuReg->SysClkDiv) = 0x00020000;
	save_mem_status_nommu(RESUME1_START |0x3d | (cpu_id<<8));
	return ;
}


