/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2011-2015, gq.yang China
*                                             All Rights Reserved
*
* File    : mem_clk.h
* By      : gq.yang
* Version : v1.0
* Date    : 2012-11-31 15:23
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __MEM_CLK_H__
#define __MEM_CLK_H__

#include <mach/ccmu.h>

struct clk_state{
	__ccmu_reg_list_t   *CmuReg;
	__u32    ccu_reg_back[15];
};

struct clk_div_t {
	__u32	axi_div;      /* division of axi clock, divide cpu clock*/
	__u32	ahb_apb_div;	/* ahb1/apb1 clock divide ratio */
};

struct clk_misc_t {
	__u32	pll1_bias;
	__u32	pll6_bias;
	__u32	pll1_tun;
};

struct pll_factor_t {
    __u8    FactorN;
    __u8    FactorK;
    __u8    FactorM;
    __u8    FactorP;
    __u32   Pll;
};

__s32 mem_clk_save(struct clk_state *pclk_state);
__s32 mem_clk_restore(struct clk_state *pclk_state);

__ccmu_reg_list_t * mem_clk_init(__u32 mmu_flag);
__s32 mem_clk_setdiv(struct clk_div_t *clk_div);
__s32 mem_clk_getdiv(struct clk_div_t  *clk_div);
__s32 mem_clk_set_pll_factor(struct pll_factor_t *pll_factor);
__s32 mem_clk_get_pll_factor(struct pll_factor_t *pll_factor);

__s32 mem_clk_get_misc(struct clk_misc_t *clk_misc);
__s32 mem_clk_set_misc(struct clk_misc_t *clk_misc);

#endif  //__MEM_CLK_H__


