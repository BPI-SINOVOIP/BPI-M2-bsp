/*
 * arch/arm/mach-sun6i/dma/dma_csp.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma csp functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

struct clk 	*g_dma_ahb_clk = NULL;
struct clk 	*g_dma_mod_clk = NULL;

u32 dma_clk_init(void)
{
	if(NULL != g_dma_mod_clk || NULL != g_dma_ahb_clk) {
		DMA_INF("%s maybe err: g_dma_mod_clk(0x%08x)/g_dma_ahb_clk(0x%08x) not NULL, line %d\n",
			__func__, (u32)g_dma_mod_clk, (u32)g_dma_ahb_clk, __LINE__);
	}

	/* config dma module clock */
	g_dma_mod_clk = clk_get(NULL, CLK_MOD_DMA);
	DMA_DBG("%s: get g_dma_mod_clk 0x%08x\n", __func__, (u32)g_dma_mod_clk);
	if(NULL == g_dma_mod_clk || IS_ERR(g_dma_mod_clk)) {
		DMA_ERR("%s err: clk_get %s failed\n", __func__, CLK_MOD_DMA);
		return -EPERM;
	} else {
		if(0 != clk_enable(g_dma_mod_clk)) {
			DMA_ERR("%s err: clk_enable failed, line %d\n", __func__, __LINE__);
			return -EPERM;
		}
		DMA_DBG("%s: clk_enable g_dma_mod_clk success\n", __func__);

		if(0 != clk_reset(g_dma_mod_clk, AW_CCU_CLK_NRESET)) {
			DMA_ERR("%s err: clk_reset failed, line %d\n", __func__, __LINE__);
			return -EPERM;
		}
		DMA_DBG("%s: clk_reset g_dma_mod_clk-AW_CCU_CLK_NRESET success\n", __func__);
	}

	/* config dma ahb clock */
	g_dma_ahb_clk = clk_get(NULL, CLK_AHB_DMA);
	DMA_DBG("%s: get g_dma_ahb_clk 0x%08x\n", __func__, (u32)g_dma_ahb_clk);
	if(NULL == g_dma_ahb_clk || IS_ERR(g_dma_ahb_clk)) {
		printk("%s err: clk_get %s failed\n", __func__, CLK_AHB_DMA);
		return -EPERM;
	} else {
		if(0 != clk_enable(g_dma_ahb_clk)) {
			DMA_ERR("%s err: clk_enable failed, line %d\n", __func__, __LINE__);
			return -EPERM;
		}
		DMA_DBG("%s: clk_enable g_dma_ahb_clk success\n", __func__);
	}

	DMA_DBG("%s success\n", __func__);
	return 0;
}

u32 dma_clk_deinit(void)
{
	DMA_DBG("%s: g_dma_mod_clk 0x%08x, g_dma_ahb_clk 0x%08x\n",
		__func__, (u32)g_dma_mod_clk, (u32)g_dma_ahb_clk);

	/* release dma mode clock */
	if(NULL == g_dma_mod_clk || IS_ERR(g_dma_mod_clk)) {
		DMA_INF("%s: g_dma_mod_clk 0x%08x invalid, just return\n", __func__, (u32)g_dma_mod_clk);
		return 0;
	} else {
		if(0 != clk_reset(g_dma_mod_clk, AW_CCU_CLK_RESET)) {
			DMA_ERR("%s err: clk_reset failed\n", __func__);
		}
		DMA_DBG("%s: clk_reset g_dma_mod_clk-AW_CCU_CLK_RESET success\n", __func__);
		clk_disable(g_dma_mod_clk);
		clk_put(g_dma_mod_clk);
		g_dma_mod_clk = NULL;
	}

	/* release dma ahb clock */
	if(NULL == g_dma_ahb_clk || IS_ERR(g_dma_ahb_clk)) {
		DMA_INF("%s: g_dma_ahb_clk 0x%08x invalid, just return\n", __func__, (u32)g_dma_ahb_clk);
		return 0;
	} else {
		clk_disable(g_dma_ahb_clk);
		clk_put(g_dma_ahb_clk);
		g_dma_ahb_clk = NULL;
	}

	DMA_DBG("%s success\n", __func__);
	return 0;
}

/**
 * csp_dma_init - init dmac
 */
void csp_dma_init(void)
{
	u32 	i = 0;

	/* init dma clock */
	if(0 != dma_clk_init())
		DMA_ERR("%s err, dma_clk_init failed, line %d\n", __func__, __LINE__);

	/* Disable & clear all interrupts */
	DMA_WRITE_REG(0, DMA_IRQ_EN_REG0);
	DMA_WRITE_REG(0, DMA_IRQ_EN_REG1);
	DMA_WRITE_REG(0xffffffff, DMA_IRQ_PEND_REG0);
	DMA_WRITE_REG(0xffffffff, DMA_IRQ_PEND_REG1);

	/* init enable reg */
	for(i = 0; i < DMA_CHAN_TOTAL; i++)
		DMA_WRITE_REG(0, DMA_EN_REG(i));
}

