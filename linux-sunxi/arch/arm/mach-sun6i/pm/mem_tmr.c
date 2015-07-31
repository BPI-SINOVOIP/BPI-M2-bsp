#include "pm_types.h"
#include "pm.h"

static __mem_tmr_reg_t  *TmrReg;
static __u32 WatchDog1_Config_Reg_Bak, WatchDog1_Mod_Reg_Bak, WatchDog1_Irq_En_Bak;

/*
*********************************************************************************************************
*                                     TIMER save
*
* Description: save timer for mem.
*
* Arguments  : none
*
* Returns    : EPDK_TRUE/EPDK_FALSE;
*********************************************************************************************************
*/
__s32 mem_tmr_save(__mem_tmr_reg_t *ptmr_state)
{

	/* set timer register base */
	TmrReg = (__mem_tmr_reg_t *)IO_ADDRESS(AW_TIMER_BASE);
	
	/* backup timer registers */
	ptmr_state->IntCtl   = TmrReg->IntCtl;
	ptmr_state->Tmr0Ctl     = TmrReg->Tmr0Ctl;
	ptmr_state->Tmr0IntVal  = TmrReg->Tmr0IntVal;
	ptmr_state->Tmr0CntVal  = TmrReg->Tmr0CntVal;
	ptmr_state->Tmr1Ctl     = TmrReg->Tmr1Ctl;
	ptmr_state->Tmr1IntVal  = TmrReg->Tmr1IntVal;
	ptmr_state->Tmr1CntVal  = TmrReg->Tmr1CntVal;
	
	return 0;
}


/*
*********************************************************************************************************
*                                     TIMER restore
*
* Description: restore timer for mem.
*
* Arguments  : none
*
* Returns    : EPDK_TRUE/EPDK_FALSE;
*********************************************************************************************************
*/
__s32 mem_tmr_restore(__mem_tmr_reg_t *ptmr_state)
{
	/* restore timer0 parameters */
	TmrReg->Tmr0IntVal  = ptmr_state->Tmr0IntVal;
	TmrReg->Tmr0CntVal  = ptmr_state->Tmr0CntVal;
	TmrReg->Tmr0Ctl     = ptmr_state->Tmr0Ctl;
	TmrReg->Tmr1IntVal  = ptmr_state->Tmr1IntVal;
	TmrReg->Tmr1CntVal  = ptmr_state->Tmr1CntVal;
	TmrReg->Tmr1Ctl     = ptmr_state->Tmr1Ctl;
	TmrReg->IntCtl      = ptmr_state->IntCtl;
	
	return 0;
}

//=================================================use for normal standby ============================
/*
*********************************************************************************************************
*                                     TIMER INIT
*
* Description: initialise timer for mem.
*
* Arguments  : none
*
* Returns    : EPDK_TRUE/EPDK_FALSE;
*********************************************************************************************************
*/
__s32 mem_tmr_init(void)
{
	/* set timer register base */
	TmrReg = (__mem_tmr_reg_t *)(IO_ADDRESS(AW_TIMER_BASE));
	
	WatchDog1_Config_Reg_Bak = (TmrReg->WDog1_Cfg_Reg);
	WatchDog1_Mod_Reg_Bak = (TmrReg->WDog1_Mode_Reg);
	WatchDog1_Irq_En_Bak = (TmrReg->WDog1_Irq_En);

	return 0;
}


/*
*********************************************************************************************************
*                                     TIMER EXIT
*
* Description: exit timer for mem.
*
* Arguments  : none
*
* Returns    : EPDK_TRUE/EPDK_FALSE;
*********************************************************************************************************
*/
__s32 mem_tmr_exit(void)
{
	(TmrReg->WDog1_Cfg_Reg) = WatchDog1_Config_Reg_Bak;
	(TmrReg->WDog1_Mode_Reg) = WatchDog1_Mod_Reg_Bak;
	(TmrReg->WDog1_Irq_En) = WatchDog1_Irq_En_Bak;

	return 0;
}


/*
*********************************************************************************************************
*                           mem_tmr_enable_watchdog
*
*Description: enable watch-dog.
*
*Arguments  : none.
*
*Return     : none;
*
*Notes      :
*
*********************************************************************************************************
*/
void mem_tmr_enable_watchdog(void)
{

	/* set watch-dog reset to whole system*/ 
	(TmrReg->WDog1_Cfg_Reg) &= ~(0x3);
	(TmrReg->WDog1_Cfg_Reg) |= 0x1;
	/*  timeout is 16 seconds */
	(TmrReg->WDog1_Mode_Reg) = (0xb<<4);
	
	/* enable watch-dog interrupt*/
	(TmrReg->WDog1_Irq_En) |= (1<<0);
	
	/* enable watch-dog */
	(TmrReg->WDog1_Mode_Reg) |= (1<<0);

	return;
}


/*
*********************************************************************************************************
*                           mem_tmr_disable_watchdog
*
*Description: disable watch-dog.
*
*Arguments  : none.
*
*Return     : none;
*
*Notes      :
*
*********************************************************************************************************
*/
void mem_tmr_disable_watchdog(void)
{
	/* disable watch-dog reset: only intterupt */
	(TmrReg->WDog1_Cfg_Reg) &= ~(0x3);
	(TmrReg->WDog1_Cfg_Reg) |= 0x2;
	
	/* disable watch-dog intterupt */
	(TmrReg->WDog1_Irq_En) &= ~(1<<0);
	
	/* disable watch-dog */
	TmrReg->WDog1_Mode_Reg &= ~(1<<0);
}


