/*
 *  arch/arm/mach-sun6i/clock/ccmu/ccm_sys_clk.c
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * kevin.z.m (kevin@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <mach/clock.h>
#include <asm/delay.h>
#include "ccm_i.h"



/*
*********************************************************************************************************
*                           sys_clk_get_parent
*
*Description: get parent clock for system clock;
*
*Arguments  : id    system clock id;
*
*Return     : parent id;
*
*Notes      :
*
*********************************************************************************************************
*/
static __aw_ccu_clk_id_e sys_clk_get_parent(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_SYS_CLK_PLL2X8:
            return AW_SYS_CLK_PLL2;
        case AW_SYS_CLK_PLL6x2:
            return AW_SYS_CLK_PLL6;
        case AW_SYS_CLK_PLL3X2:
            return AW_SYS_CLK_PLL3;
        case AW_SYS_CLK_PLL7X2:
            return AW_SYS_CLK_PLL7;
        case AW_SYS_CLK_MIPIPLL:
            return aw_ccu_reg->MipiPllCtl.PllSrc? AW_SYS_CLK_PLL7:AW_SYS_CLK_PLL3;
        case AW_SYS_CLK_AC327:
            switch(aw_ccu_reg->SysClkDiv.CpuClkSrc)
            {
                case AC327_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case AC327_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                default:
                    return AW_SYS_CLK_PLL1;
            }
        case AW_SYS_CLK_AR100:
            switch(aw_cpus_reg->CpusCfg.ClkSrc)
            {
                case AR100_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case AR100_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                default:
                    return AW_SYS_CLK_PLL6;
            }
        case AW_SYS_CLK_AXI:
            return AW_SYS_CLK_AC327;
        case AW_SYS_CLK_AHB0:
            return AW_SYS_CLK_AR100;
        case AW_SYS_CLK_AHB1:
            switch(aw_ccu_reg->Ahb1Div.Ahb1ClkSrc)
            {
                case AHB1_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case AHB1_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                case AHB1_CLKSRC_AXI:
                    return AW_SYS_CLK_AXI;
                case AHB1_CLKSRC_PLL6:
                    return AW_SYS_CLK_PLL6;
            }
        case AW_SYS_CLK_APB0:
            return AW_SYS_CLK_AHB0;
        case AW_SYS_CLK_APB1:
            return AW_SYS_CLK_AHB1;
        case AW_SYS_CLK_APB2:
            switch(aw_ccu_reg->Apb2Div.ClkSrc)
            {
                case APB2_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case APB2_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                default:
                    return AW_SYS_CLK_PLL6;
            }
        default:
            return AW_SYS_CLK_NONE;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_get_status
*
*Description: get system clock on/off status.
*
*Arguments  : id    system clock id;
*
*Return     : system clock status;
*               0, clock is off;
*              !0, clock is on;
*
*Notes      :
*
*********************************************************************************************************
*/
static __aw_ccu_clk_onff_e sys_clk_get_status(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_SYS_CLK_LOSC:
            return AW_CCU_CLK_ON;
        case AW_SYS_CLK_HOSC:
            return AW_CCU_CLK_ON;
        case AW_SYS_CLK_PLL1:
            return PLL1_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL2:
        case AW_SYS_CLK_PLL2X8:
            return PLL2_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL3X2:
            return PLL3_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL4:
            return PLL4_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL5:
            return PLL5_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL6:
        case AW_SYS_CLK_PLL6x2:
            return PLL6_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL7X2:
            return PLL7_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL8:
            return PLL8_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL9:
            return PLL9_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL10:
            return PLL10_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_MIPIPLL:
            return aw_ccu_reg->MipiPllCtl.PLLEn? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_AC327:
        case AW_SYS_CLK_AR100:
        case AW_SYS_CLK_AXI:
        case AW_SYS_CLK_AHB0:
        case AW_SYS_CLK_AHB1:
        case AW_SYS_CLK_APB0:
        case AW_SYS_CLK_APB1:
        case AW_SYS_CLK_APB2:
        default:
            return AW_CCU_CLK_ON;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_get_rate
*
*Description: get clock rate for system clock;
*
*Arguments  : id    system clock id;
*
*Return     : clock rate;
*
*Notes      :
*
*********************************************************************************************************
*/
static __u64 sys_clk_get_rate(__aw_ccu_clk_id_e id)
{
    __u64   tmp_rate;

    switch(id)
    {
        case AW_SYS_CLK_NONE:
            return 1;
        case AW_SYS_CLK_LOSC:
            return 32768;
        case AW_SYS_CLK_HOSC:
            return 24000000;
        case AW_SYS_CLK_PLL1:
            tmp_rate = (__u64)24000000*PLL1_FACTOR_N*PLL1_FACTOR_K;
            do_div(tmp_rate, PLL1_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL2:
            if((aw_ccu_reg->Pll2Ctl.FactorM == 20)
               && (aw_ccu_reg->Pll2Ctl.FactorN == 85)
               && (aw_ccu_reg->Pll2Ctl.FactorP == 3))
            {
                /* 24000000 * 86 / (21 * 4) = 24571000 */
                return 24576000;
            }
            else if((aw_ccu_reg->Pll2Ctl.FactorM == 20)
               && (aw_ccu_reg->Pll2Ctl.FactorN == 78)
               && (aw_ccu_reg->Pll2Ctl.FactorP == 3))
            {
                /* 24000000 * 79 / (21 * 4) = 22571000 */
                return 22579200;
            }
            else
            {
                aw_ccu_reg->Pll2Ctl.FactorM = 20;
                aw_ccu_reg->Pll2Ctl.FactorN = 78;
                aw_ccu_reg->Pll2Ctl.FactorP = 3;
                return 22579200;
            }
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL4:
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL8:
        case AW_SYS_CLK_PLL9:
        case AW_SYS_CLK_PLL10:
        {
            volatile __ccmu_media_pll_t  *tmp_reg;
	    __ccmu_media_pll_t  tmp_pll;

            if(id == AW_SYS_CLK_PLL3)
                tmp_reg = &aw_ccu_reg->Pll3Ctl;
            else if(id == AW_SYS_CLK_PLL4)
                tmp_reg = &aw_ccu_reg->Pll4Ctl;
            else if(id == AW_SYS_CLK_PLL7)
                tmp_reg = &aw_ccu_reg->Pll7Ctl;
            else if(id == AW_SYS_CLK_PLL8)
                tmp_reg = &aw_ccu_reg->Pll8Ctl;
            else if(id == AW_SYS_CLK_PLL9)
                tmp_reg = &aw_ccu_reg->Pll9Ctl;
            else
                tmp_reg = &aw_ccu_reg->Pll10Ctl;

            if(!tmp_reg->ModeSel)
            {
	    	tmp_rate = tmp_reg->FracMod ? 297000000 : 270000000;
		ccm_get_pllx_para(&tmp_pll, tmp_rate);
		tmp_reg->FactorM = tmp_pll.FactorM;
		tmp_reg->FactorN = tmp_pll.FactorN;
		return tmp_rate;
            }
            else
            {
                tmp_rate = (__u64)24000000*(tmp_reg->FactorN+1);
                do_div(tmp_rate, tmp_reg->FactorM+1);
		if(tmp_rate == 297000000 || tmp_rate == 270000000) {
		    /* set pll to frac mode */
		    tmp_reg->ModeSel = 0;
		    tmp_reg->FracMod = (tmp_rate == 297000000) ? 1 : 0;
		    ccm_get_pllx_para(&tmp_pll, tmp_rate);
		    tmp_reg->FactorM = tmp_pll.FactorM;
		    tmp_reg->FactorN = tmp_pll.FactorN;
		}
                return tmp_rate;
            }
        }

        case AW_SYS_CLK_PLL5:
            tmp_rate = (__u64)24000000*PLL5_FACTOR_N*PLL5_FACTOR_K;
            do_div(tmp_rate, PLL5_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL6:
            tmp_rate = (__u64)24000000*PLL6_FACTOR_N*PLL6_FACTOR_K;
            return tmp_rate/2;
        case AW_SYS_CLK_PLL2X8:
            return sys_clk_get_rate(AW_SYS_CLK_PLL2) * 8;
        case AW_SYS_CLK_PLL3X2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL3) * 2;
        case AW_SYS_CLK_PLL6x2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL6) * 2;
        case AW_SYS_CLK_PLL7X2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL7) * 2;
        case AW_SYS_CLK_MIPIPLL:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            if(aw_ccu_reg->MipiPllCtl.VfbSel == 0) {
                tmp_rate *= (aw_ccu_reg->MipiPllCtl.FactorN+1) * (aw_ccu_reg->MipiPllCtl.FactorK+1);
                do_div(tmp_rate, aw_ccu_reg->MipiPllCtl.FactorM+1);
            } else {
                tmp_rate *= aw_ccu_reg->MipiPllCtl.SDiv2+1;
                if(aw_ccu_reg->MipiPllCtl.FracMode == 0) {
                    tmp_rate *= aw_ccu_reg->MipiPllCtl.FeedBackDiv? 7:5;
                    do_div(tmp_rate, aw_ccu_reg->MipiPllCtl.FactorM + 1);
                } else {
                    tmp_rate *= aw_ccu_reg->MipiPllCtl.Sel625Or750? 750:625;
                    do_div(tmp_rate, ((aw_ccu_reg->MipiPllCtl.FactorM+1)*100));
                }
            }
            return tmp_rate;

        case AW_SYS_CLK_AC327:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AC327_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AR100:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AR100_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AXI:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AXI_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AHB0:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AHB0_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AHB1:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AHB1_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_APB0:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, APB0_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_APB1:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, APB1_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_APB2:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, APB2_CLK_DIV);
            return tmp_rate;
        default:
            CCU_DBG("system clock id is:%d\n", id);
            return 0;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_set_parent
*
*Description: set parent clock id for system clock;
*
*Arguments  : id        system clock id whose parent need be set;
*             parent    parent id to be set;
*
*Return     : result,
*               0,  set parent successed;
*              !0,  set parent failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s32 sys_clk_set_parent(__aw_ccu_clk_id_e id, __aw_ccu_clk_id_e parent)
{
    switch(id)
    {
        case AW_SYS_CLK_PLL2X8:
            return (parent == AW_SYS_CLK_PLL2)? 0:-1;
        case AW_SYS_CLK_PLL3X2:
            return (parent == AW_SYS_CLK_PLL3)? 0:-1;
        case AW_SYS_CLK_PLL6x2:
            return (parent == AW_SYS_CLK_PLL6)? 0:-1;
        case AW_SYS_CLK_PLL7X2:
            return (parent == AW_SYS_CLK_PLL7)? 0:-1;
        case AW_SYS_CLK_MIPIPLL:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->MipiPllCtl.PllSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->MipiPllCtl.PllSrc = 1;
            else
                return -1;
            return 0;
        case AW_SYS_CLK_AC327:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_ccu_reg->SysClkDiv.CpuClkSrc = AC327_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_ccu_reg->SysClkDiv.CpuClkSrc = AC327_CLKSRC_HOSC;
                    return 0;
                case AW_SYS_CLK_PLL1:
                    aw_ccu_reg->SysClkDiv.CpuClkSrc = AC327_CLKSRC_PLL1;
                    return 0;
                default:
                    CCU_ERR("ac327 clock source is ivalid!\n");
                    return -1;
            }
        case AW_SYS_CLK_AR100:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_cpus_reg->CpusCfg.ClkSrc = AR100_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_cpus_reg->CpusCfg.ClkSrc = AR100_CLKSRC_HOSC;
                    return 0;
                case AW_SYS_CLK_PLL1:
                    aw_cpus_reg->CpusCfg.ClkSrc = AR100_CLKSRC_PLL6;
                    return 0;
                default:
                    CCU_ERR("ar100 clock source is ivalid!\n");
                    return -1;
            }
        case AW_SYS_CLK_AXI:
            return (parent == AW_SYS_CLK_AC327)? 0:-1;
        case AW_SYS_CLK_AHB0:
            return (parent == AW_SYS_CLK_AR100)? 0:-1;
        case AW_SYS_CLK_AHB1:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AHB1_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AW_SYS_CLK_HOSC;
                    return 0;
                case AW_SYS_CLK_AXI:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AHB1_CLKSRC_AXI;
                    return 0;
                case AW_SYS_CLK_PLL6:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AHB1_CLKSRC_PLL6;
                    return 0;
                default:
                    CCU_ERR("axi clock source is ivalid!\n");
                    return -1;
            }
        case AW_SYS_CLK_APB0:
            return (parent == AW_SYS_CLK_AHB0)? 0:-1;
        case AW_SYS_CLK_APB1:
            return (parent == AW_SYS_CLK_AHB1)? 0:-1;
        case AW_SYS_CLK_APB2:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_ccu_reg->Apb2Div.ClkSrc = APB2_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_ccu_reg->Apb2Div.ClkSrc = APB2_CLKSRC_HOSC;
                    return 0;
                case AW_SYS_CLK_PLL6:
                    aw_ccu_reg->Apb2Div.ClkSrc = APB2_CLKSRC_PLL6;
                    return 0;
                default:
                    CCU_ERR("apb2 clock source is ivalid!\n");
                    return -1;
            }

        case AW_SYS_CLK_LOSC:
        case AW_SYS_CLK_HOSC:
        case AW_SYS_CLK_PLL1:
        case AW_SYS_CLK_PLL2:
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL4:
        case AW_SYS_CLK_PLL5:
        case AW_SYS_CLK_PLL6:
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL8:
        case AW_SYS_CLK_PLL9:
        case AW_SYS_CLK_PLL10:
        default:
            return (parent == AW_SYS_CLK_NONE)? 0:-1;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_set_status
*
*Description: set on/off status for system clock;
*
*Arguments  : id        system clock id;
*             status    on/off status;
*                           AW_CCU_CLK_OFF - off
*                           AW_CCU_CLK_ON - on
*
*Return     : result;
*               0,  set status successed;
*              !0,  set status failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s32 sys_clk_set_status(__aw_ccu_clk_id_e id, __aw_ccu_clk_onff_e status)
{
    switch(id)
    {
        case AW_SYS_CLK_LOSC:
            return 0;
        case AW_SYS_CLK_HOSC:
            return 0;
        case AW_SYS_CLK_PLL1:
            PLL1_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL2:
            PLL2_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL3:
            PLL3_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL4:
            PLL4_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL5:
            PLL5_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL6:
            PLL6_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL7:
            PLL7_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL8:
            PLL8_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL9:
            PLL9_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL10:
            PLL10_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_MIPIPLL:
            aw_ccu_reg->MipiPllCtl.PLLEn = (status == AW_CCU_CLK_ON)? 1:0;
            return 0;
        case AW_SYS_CLK_PLL2X8:
            return (sys_clk_get_status(AW_SYS_CLK_PLL2) == status)? 0 : -1;
        case AW_SYS_CLK_PLL3X2:
            return (sys_clk_get_status(AW_SYS_CLK_PLL3) == status)? 0 : -1;
        case AW_SYS_CLK_PLL6x2:
            return (sys_clk_get_status(AW_SYS_CLK_PLL6) == status)? 0 : -1;
        case AW_SYS_CLK_PLL7X2:
            return (sys_clk_get_status(AW_SYS_CLK_PLL7) == status)? 0 : -1;

        default:
            return 0;
    }
}


/* ahb1 clock division table */
/* bit0~bit7:div(1<<n), bit8~bit15:pre-div(n+1), bit16~bit23: ahb div*/
static __u32 ahb1_div_tbl[10] = {
    (1<<16) |(0<<8)|(0<<0),
    (2<<16) |(0<<8)|(1<<0),
    (3<<16) |(2<<8)|(0<<0),
    (4<<16) |(3<<8)|(0<<0),
    (6<<16) |(2<<8)|(1<<0),
    (8<<16) |(3<<8)|(1<<0),
    (12<<16)|(2<<8)|(2<<0),
    (16<16) |(3<<8)|(2<<0),
    (24<<16)|(2<<8)|(3<<0),
    (32<<16)|(3<<8)|(3<<0),
};

/*
*********************************************************************************************************
*                           sys_clk_set_rate
*
*Description: set clock rate for system clock;
*
*Arguments  : id    system clock id;
*             rate  clock rate for system clock;
*
*Return     : result,
*               0,  set system clock rate successed;
*              !0,  set system clock rate failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s32 sys_clk_set_rate(__aw_ccu_clk_id_e id, __u64 rate)
{
    CCU_INF("try to switch %s rate to %llu\n", aw_ccu_clk_tbl[id].name, rate);

    switch(id)
    {
        case AW_SYS_CLK_LOSC:
            return (rate == 32768)? 0 : -1;
        case AW_SYS_CLK_HOSC:
            return (rate == 24000000)? 0 : -1;
        case AW_SYS_CLK_PLL1:
        {
            __ccmu_pll1_reg0000_t       tmp_pll;

            tmp_pll = aw_ccu_reg->Pll1Ctl;
            if(ccm_get_pll1_para(&tmp_pll, rate))
            {
                CCU_ERR("(%s:%d)try to get pll1 rate(%llu) config failed!\n", __FILE__, __LINE__, rate);
                return -1;
            }
            aw_ccu_reg->Pll1Ctl = tmp_pll;

            #ifdef CONFIG_AW_ASIC_EVB_PLATFORM
            if(aw_ccu_reg->Pll1Ctl.PLLEn) {
                while(!aw_ccu_reg->Pll1Ctl.Lock);
            }
            #endif

            return 0;
        }
        case AW_SYS_CLK_PLL2:
        {
            if(rate == 22579200)
            {
                aw_ccu_reg->Pll2Ctl.FactorN = 78;
                aw_ccu_reg->Pll2Ctl.FactorM = 20;
                aw_ccu_reg->Pll2Ctl.FactorP = 3;
            }
            else if(rate == 24576000)
            {
                aw_ccu_reg->Pll2Ctl.FactorN = 85;
                aw_ccu_reg->Pll2Ctl.FactorM = 20;
                aw_ccu_reg->Pll2Ctl.FactorP = 3;
            }
            else
            {
                return -1;
            }

            #ifdef CONFIG_AW_ASIC_EVB_PLATFORM
            if(aw_ccu_reg->Pll2Ctl.PLLEn) {
                while(!aw_ccu_reg->Pll2Ctl.Lock);
            }
            #endif

            return 0;
        }
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL4:
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL8:
        case AW_SYS_CLK_PLL9:
        case AW_SYS_CLK_PLL10:
        {
            volatile __ccmu_media_pll_t  *tmp_reg;
            __ccmu_media_pll_t  tmp_pll;

            if(id == AW_SYS_CLK_PLL3)
                tmp_reg = &aw_ccu_reg->Pll3Ctl;
            else if(id == AW_SYS_CLK_PLL4)
                tmp_reg = &aw_ccu_reg->Pll4Ctl;
            else if(id == AW_SYS_CLK_PLL7)
                tmp_reg = &aw_ccu_reg->Pll7Ctl;
            else if(id == AW_SYS_CLK_PLL8)
                tmp_reg = &aw_ccu_reg->Pll8Ctl;
            else if(id == AW_SYS_CLK_PLL9)
                tmp_reg = &aw_ccu_reg->Pll9Ctl;
            else
                tmp_reg = &aw_ccu_reg->Pll10Ctl;

            if(rate == 1000000000)
            {
                /* special frquency, control by de */
                tmp_reg->CtlMode = 1;
                return 0;
            }

            tmp_reg->CtlMode = 0;
            if((rate == 270000000) || (rate == 297000000))
            {
                tmp_reg->ModeSel = 0;
                tmp_reg->FracMod = (rate == 270000000)?0:1;
                ccm_get_pllx_para(&tmp_pll, rate);
                tmp_reg->FactorM = tmp_pll.FactorM;
                tmp_reg->FactorN = tmp_pll.FactorN;
                return 0;
            }
            else
            {
                __ccmu_media_pll_t  tmp_cfg;

                if(ccm_get_pllx_para(&tmp_cfg, rate)) {
                    CCU_ERR("(%s:%d)try to get pll (%d, %llu) configuration failed!\n", __FILE__, __LINE__, id, rate);
                    return -1;
                }

                /* write register */
                tmp_reg->ModeSel = 1;
                if(tmp_reg->FactorM < tmp_cfg.FactorM) {
                    tmp_reg->FactorM = tmp_cfg.FactorM;
                    tmp_reg->FactorN = tmp_cfg.FactorN;
                } else {
                    tmp_reg->FactorN = tmp_cfg.FactorN;
                    tmp_reg->FactorM = tmp_cfg.FactorM;
                }

                #ifdef CONFIG_AW_ASIC_EVB_PLATFORM
                if(tmp_reg->PLLEn) {
                    while(!tmp_reg->Lock);
                }
                #endif

                return 0;
            }
        }
        case AW_SYS_CLK_PLL5:
        {
            __ccmu_pll5_reg0020_t       tmp_pll;

            tmp_pll = aw_ccu_reg->Pll5Ctl;
            if(ccm_get_pll1_para((__ccmu_pll1_reg0000_t *)&tmp_pll, rate))
            {
                CCU_ERR("(%s:%d)try to get pll5 rate(%llu) config failed!\n", __FILE__, __LINE__, rate);
                return -1;
            }
            aw_ccu_reg->Pll5Ctl = tmp_pll;

            #ifdef CONFIG_AW_ASIC_EVB_PLATFORM
            if(aw_ccu_reg->Pll5Ctl.PLLEn) {
                while(!aw_ccu_reg->Pll5Ctl.Lock);
            }
            #endif

            return 0;
        }
        case AW_SYS_CLK_PLL6:
        {
            do_div(rate, 12000000);
            if(rate > 32*4)
            {
                CCU_ERR("Rate(%lld) is invalid when set pll6 rate!\n", rate);
                return -1;
            }
            else if(rate > 32*3)
            {
                aw_ccu_reg->Pll6Ctl.FactorK = 3;
                do_div(rate, 4);
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;

            }
            else if(rate > 32*2)
            {
                aw_ccu_reg->Pll6Ctl.FactorK = 2;
                do_div(rate, 3);
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;
            }
            else if(rate > 32)
            {
                aw_ccu_reg->Pll6Ctl.FactorK = 1;
                do_div(rate, 2);
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;
            }
            else
            {
                aw_ccu_reg->Pll6Ctl.FactorK = 0;
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;
            }

            #ifdef CONFIG_AW_ASIC_EVB_PLATFORM
            if(aw_ccu_reg->Pll6Ctl.PLLEn) {
                while(!aw_ccu_reg->Pll6Ctl.Lock);
            }
            #endif

            return 0;
        }
        case AW_SYS_CLK_PLL2X8:
        case AW_SYS_CLK_PLL3X2:
        case AW_SYS_CLK_PLL6x2:
        case AW_SYS_CLK_PLL7X2:
            return 0;
        case AW_SYS_CLK_MIPIPLL:
            //#error 'how to set mipi pll?'
            return -1;
        case AW_SYS_CLK_AC327:
            return rate == sys_clk_get_rate(sys_clk_get_parent(id))? 0 : -1;
        case AW_SYS_CLK_AXI:
        {
            __u64   tmp_rate;
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, rate);
            aw_ccu_reg->SysClkDiv.AXIClkDiv = tmp_rate;
            return 0;
        }

        case AW_SYS_CLK_AR100:
        {
            __u64   tmp_rate;

            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, rate);
            if(tmp_rate > 32*8) {
                return -1;
            } else if(tmp_rate > 32*4) {
                aw_cpus_reg->CpusCfg.Div = 3;
                aw_cpus_reg->CpusCfg.PostDiv = do_div(tmp_rate, 8)-1;
            } else if(tmp_rate > 32*2) {
                aw_cpus_reg->CpusCfg.Div = 2;
                aw_cpus_reg->CpusCfg.PostDiv = do_div(tmp_rate, 4)-1;
            } else if(tmp_rate > 32*1) {
                aw_cpus_reg->CpusCfg.Div = 1;
                aw_cpus_reg->CpusCfg.PostDiv = do_div(tmp_rate, 2)-1;
            } else {
                aw_cpus_reg->CpusCfg.Div = 0;
                aw_cpus_reg->CpusCfg.PostDiv = tmp_rate-1;
            }
            return 0;
        }
        case AW_SYS_CLK_AHB0:
            return rate == sys_clk_get_rate(sys_clk_get_parent(id))? 0 : -1;
        case AW_SYS_CLK_APB0:
        {
            __u64   tmp_rate;

            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, rate);

            if(tmp_rate > 8) {
                return -1;
            } else if(tmp_rate > 4) {
                aw_cpus_reg->Apb0Div.Div = 3;
            } else if(tmp_rate > 2) {
                aw_cpus_reg->Apb0Div.Div = 2;
            } else {
                aw_cpus_reg->Apb0Div.Div = 0;
            }

            return 0;
        }

        case AW_SYS_CLK_AHB1:
        {
            __u64   tmp_rate;
            __aw_ccu_clk_id_e   parent;

            parent = sys_clk_get_parent(id);
            tmp_rate = sys_clk_get_rate(parent);
            do_div(tmp_rate, rate);

            if(parent == AW_SYS_CLK_PLL6) {
                int     i;
                for(i=0; i<10; i++) {
                    if(tmp_rate <= ((ahb1_div_tbl[i]>>16) & 0xff)) {
                        aw_ccu_reg->Ahb1Div.Ahb1PreDiv = (ahb1_div_tbl[i]>>8) & 0xff;
                        aw_ccu_reg->Ahb1Div.Ahb1Div = (ahb1_div_tbl[i]>>0) & 0xff;
                        return 0;
                    }
                }
                return -1;
            } else {
                if(tmp_rate > 8) {
                    return -1;
                } else if (tmp_rate > 4) {
                    aw_ccu_reg->Ahb1Div.Ahb1Div = 3;
                } else if (tmp_rate > 2) {
                    aw_ccu_reg->Ahb1Div.Ahb1Div = 2;
                } else if (tmp_rate > 1) {
                    aw_ccu_reg->Ahb1Div.Ahb1Div = 1;
                } else {
                    aw_ccu_reg->Ahb1Div.Ahb1Div = 0;
                }
            }
        }

        case AW_SYS_CLK_APB1:
        {
            __u64   tmp_rate;

            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, rate);

            if(tmp_rate > 8) {
                return -1;
            } else if(tmp_rate > 4) {
                aw_ccu_reg->Ahb1Div.Apb1Div = 3;
            } else if(tmp_rate > 2) {
                aw_ccu_reg->Ahb1Div.Apb1Div = 2;
            } else {
                aw_ccu_reg->Ahb1Div.Apb1Div = 0;
            }

            return 0;
        }
        case AW_SYS_CLK_APB2:
        {
            __u64   tmp_rate;

            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, rate);
            if(tmp_rate < 32) {
                aw_ccu_reg->Apb2Div.DivN = 0;
                aw_ccu_reg->Apb2Div.DivM = tmp_rate-1;
           } else if(tmp_rate < 32*2) {
                aw_ccu_reg->Apb2Div.DivN = 1;
                aw_ccu_reg->Apb2Div.DivM = (tmp_rate>>1)-1;
            } else if(tmp_rate < 32*4) {
                aw_ccu_reg->Apb2Div.DivN = 2;
                aw_ccu_reg->Apb2Div.DivM = (tmp_rate>>2)-1;
            } else if(tmp_rate < 32*8) {
                aw_ccu_reg->Apb2Div.DivN = 3;
                aw_ccu_reg->Apb2Div.DivM = (tmp_rate>>3)-1;
            } else {
                return -1;
            }

            return 0;
        }

        default:
        {
            CCU_ERR("clock id(%d) is invaid when set rate!\n", (__s32)id);
            return -1;
        }
    }
}

/*
*********************************************************************************************************
*                           sys_clk_round_rate
*
*Description: round a rate of the given clock to a valid value;
*
*Arguments  : id    system clock id;
*             rate  clock rate for system clock;
*
*Return     : result
*
*Notes      :
*
*********************************************************************************************************
*/
static __u64 sys_clk_round_rate(__aw_ccu_clk_id_e id, __u64 rate)
{
    switch(id)
    {
        case AW_SYS_CLK_LOSC:
            return 32768;
        case AW_SYS_CLK_HOSC:
            return 24000000;
        case AW_SYS_CLK_PLL1:
        {
            __ccmu_pll1_reg0000_t       tmp_pll;

            if(ccm_get_pll1_para(&tmp_pll, rate))
            {
                return rate;
            }

            return (24000000 * (tmp_pll.FactorN+1) * (tmp_pll.FactorK+1)) / (tmp_pll.FactorM+1);
        }

        case AW_SYS_CLK_PLL2:
        {
            if(rate < 23000000)
            {
                return 22579200;
            }
            else
            {
                return 24576000;
            }
        }
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL4:
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL8:
        case AW_SYS_CLK_PLL9:
        case AW_SYS_CLK_PLL10:
        {
            __ccmu_media_pll_t  tmp_pll;

            if((rate == 270000000) || (rate == 297000000))
            {
                return rate;
            }

            if(ccm_get_pllx_para(&tmp_pll, rate)) {
                return rate;
            }

            return 24000000 * (tmp_pll.FactorN + 1) / (tmp_pll.FactorM + 1);
        }

        case AW_SYS_CLK_PLL5:
        {
            __ccmu_pll5_reg0020_t       tmp_pll;

            if(ccm_get_pll1_para((__ccmu_pll1_reg0000_t *)&tmp_pll, rate))
            {
                return rate;
            }
            return (24000000 * (tmp_pll.FactorN+1) * (tmp_pll.FactorK+1));
        }
        case AW_SYS_CLK_PLL6:
        {
            if(rate > 32*4*12000000) {
                return 32*4*12000000;
            }
            else if(rate > 32*3*12000000) {
                do_div(rate, 12000000*3);
                return 12000000*3*rate;
            }
            else if(rate > 32*2*12000000) {
                do_div(rate, 12000000*2);
                return 12000000*2*rate;
            }
            else if(rate > 32*1*12000000) {
                do_div(rate, 12000000*1);
                return 12000000*1*rate;
            }
            else {
                do_div(rate, 12000000*1);
                return rate*12000000;
            }
        }
        case AW_SYS_CLK_PLL2X8:
            return sys_clk_get_rate(AW_SYS_CLK_PLL2) * 8;
        case AW_SYS_CLK_PLL3X2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL3) * 2;
        case AW_SYS_CLK_PLL6x2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL6) * 2;
        case AW_SYS_CLK_PLL7X2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL7) * 2;
        case AW_SYS_CLK_MIPIPLL:
            return rate;
        case AW_SYS_CLK_AC327:
            return sys_clk_get_rate(AW_SYS_CLK_PLL1);
        case AW_SYS_CLK_AR100:
        {
            __u64   src_rate, tmp_rate;

            src_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            tmp_rate = src_rate;
            tmp_rate += rate-1;
            do_div(tmp_rate, rate);
            if(tmp_rate > 32*8) {
                do_div(src_rate, (32*8));
            } else if(tmp_rate > 32*4) {
                do_div(tmp_rate, 8);
                tmp_rate *= 8;
                do_div(src_rate, tmp_rate);
            } else if(tmp_rate > 32*2) {
                do_div(tmp_rate, 4);
                tmp_rate *= 4;
                do_div(src_rate, tmp_rate);
            } else if(tmp_rate > 32*1) {
                do_div(tmp_rate, 2);
                tmp_rate *= 2;
                do_div(src_rate, tmp_rate);
            } else {
                do_div(src_rate, tmp_rate);
            }

            return src_rate;
        }
        case AW_SYS_CLK_AXI:
        {
            __u64   src_rate, tmp_rate;

            src_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            tmp_rate = src_rate;
            tmp_rate += rate -1;
            do_div(tmp_rate, rate);
            if(tmp_rate > 4) {
                tmp_rate = 4;
            }

            do_div(src_rate, tmp_rate);
            return src_rate;
        }
        case AW_SYS_CLK_AHB0:
            return sys_clk_get_rate(sys_clk_get_parent(id));
        case AW_SYS_CLK_AHB1:
        {
            __u64   src_rate, tmp_rate;
            __aw_ccu_clk_id_e   parent;

            parent = sys_clk_get_parent(id);
            src_rate = sys_clk_get_rate(parent);
            tmp_rate = src_rate + rate - 1;
            do_div(tmp_rate, rate);

            if(parent == AW_SYS_CLK_PLL6) {
                int     i;
                for(i=0; i<10; i++) {
                    if(tmp_rate <= ((ahb1_div_tbl[i]>>16) & 0xff)) {
                        tmp_rate = (1 << ((ahb1_div_tbl[i]>>0)&0xff)) * (((ahb1_div_tbl[i]>>8)&0xff) + 1);
                        do_div(src_rate, tmp_rate);
                        return src_rate;
                    }
                }
                do_div(src_rate, 32);
                return src_rate;
            } else {
                if(tmp_rate > 4) {
                    tmp_rate = 8;
                } else if(tmp_rate > 2) {
                    tmp_rate = 4;
                } else if (tmp_rate > 1) {
                    tmp_rate = 2;
                } else {
                    tmp_rate = 1;
                }
                do_div(src_rate, tmp_rate);
                return src_rate;
            }
        }
        case AW_SYS_CLK_APB0:
        case AW_SYS_CLK_APB1:
        {
            __u64   src_rate, tmp_rate;

            src_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            tmp_rate = src_rate + rate -1;
            do_div(tmp_rate, rate);

            if(tmp_rate > 4) {
                tmp_rate = 8;
            } else if(tmp_rate > 2) {
                tmp_rate = 4;
            } else if(tmp_rate > 1) {
                tmp_rate = 2;
            } else {
                tmp_rate = 1;
            }
            do_div(src_rate, tmp_rate);
            return src_rate;
        }

        case AW_SYS_CLK_APB2:
        {
            __u64   src_rate, tmp_rate;

            src_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            tmp_rate = src_rate + rate - 1;
            do_div(tmp_rate, rate);

            if(tmp_rate > 32*8) {
                tmp_rate = 32 * 8;
            }else if(tmp_rate > 32*4) {
                do_div(tmp_rate, 8);
                tmp_rate *= 8;
            }else if(tmp_rate > 32*2) {
                do_div(tmp_rate, 4);
                tmp_rate *= 4;
            }else if(tmp_rate > 32) {
                do_div(tmp_rate, 2);
                tmp_rate *= 2;
            }
            do_div(src_rate, tmp_rate);
            return src_rate;
        }

        default:
            return rate;
    }
}


__clk_ops_t sys_clk_ops = {
    .set_status = sys_clk_set_status,
    .get_status = sys_clk_get_status,
    .set_parent = sys_clk_set_parent,
    .get_parent = sys_clk_get_parent,
    .get_rate = sys_clk_get_rate,
    .set_rate = sys_clk_set_rate,
    .round_rate = sys_clk_round_rate,
    .set_reset  = 0,
    .get_reset  = 0,
};

