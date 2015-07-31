/*
 * arch/arm/mach-sun6i/dma/dma.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma driver interface
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

/**
 * __dma_irq_hdl - dma irq process function
 * @irq:	dma physical irq num
 * @dev:	para passed in request_irq function
 *
 * we cannot lock __dma_irq_hdl through,
 * because sw_dma_enqueue maybe called in cb,
 * which will result in deadlock
 */
irqreturn_t __dma_irq_hdl(int irq, void *dev)
{
	dma_channel_t *pchans = (dma_channel_t *)dev;
	dma_channel_t *pchan;
	u32 upend_bits = 0;
	int i;

	DMA_DBG("%s, line %d, dma en0 0x%08x, en1 0x%08x, pd0 0x%08x, pd1 0x%08x\n", __func__, __LINE__, \
		DMA_READ_REG(DMA_IRQ_EN_REG0), DMA_READ_REG(DMA_IRQ_EN_REG1), \
		DMA_READ_REG(DMA_IRQ_PEND_REG0), DMA_READ_REG(DMA_IRQ_PEND_REG1));

	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &pchans[i];
		upend_bits = csp_dma_chan_get_irqpend(pchan);
		if(0 == upend_bits)
			continue;

		if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
			dma_irq_hdl_single(pchan, upend_bits);
		else if(DMA_WORK_MODE_CHAIN == pchan->work_mode)
			dma_irq_hdl_chain(pchan, upend_bits);
	}
	return IRQ_HANDLED;
}

/**
 * __dma_init - initial the dma manager, request irq
 * @device:	platform device pointer
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int __dma_init(struct platform_device *device)
{
	dma_channel_t *pchan = NULL;
	int ret = 0;
	int i = 0;

	/* init dma controller */
	csp_dma_init();

	/* initial the dma manager */
	memset(dma_chnl, 0, sizeof(dma_chnl));
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan 		= &dma_chnl[i];
		pchan->used 	= 0;
		pchan->id 	= i;
		pchan->reg_base = (u32)DMA_EN_REG(i);
		pchan->irq_spt 	= CHAN_IRQ_NO;
		pchan->bconti_mode = false;
		pchan->work_mode = DMA_WORK_MODE_INVALID;
		DMA_CHAN_LOCK_INIT(&pchan->lock);
	}

	/* alloc dma pool for des list */
	g_des_pool = dmam_pool_create("dma_des_pool", &device->dev, sizeof(des_item), 4, 0);
	if(NULL == g_des_pool) {
		ret = __LINE__;
		goto end;
	}
	DMA_INF("%s(%d): g_des_pool 0x%08x\n", __func__, __LINE__, (u32)g_des_pool);

	/* register dma interrupt */
	ret = request_irq(AW_IRQ_DMA, __dma_irq_hdl, IRQF_DISABLED, "dma_irq", (void *)dma_chnl);
	if(ret) {
		DMA_ERR("%s err: request_irq return %d\n", __func__, ret);
		ret = __LINE__;
		goto end;
	}
	DMA_INF("%s, line %d\n", __func__, __LINE__);

end:
	if(0 != ret) {
		DMA_ERR("%s err, line %d\n", __func__, ret);
		if (NULL != g_des_pool) {
			dma_pool_destroy(g_des_pool);
			g_des_pool = NULL;
		}
		for(i = 0; i < DMA_CHAN_TOTAL; i++)
			DMA_CHAN_LOCK_DEINIT(&dma_chnl[i].lock);
	}
	return ret;
}

/**
 * __dma_deinit - deinit the dma manager, free irq
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int __dma_deinit(void)
{
	u32 	i = 0;

	DMA_INF("%s, line %d\n", __func__, __LINE__);

	free_irq(AW_IRQ_DMA, (void *)dma_chnl);

	if (NULL != g_des_pool) {
		dma_pool_destroy(g_des_pool);
		g_des_pool = NULL;
	}
	for(i = 0; i < DMA_CHAN_TOTAL; i++)
		DMA_CHAN_LOCK_DEINIT(&dma_chnl[i].lock);

	memset(dma_chnl, 0, sizeof(dma_chnl));
	return 0;
}

/**
 * dma_drv_probe - dma driver inital function.
 * @dev:	platform device pointer
 *
 * Returns 0 if success, otherwise return the err line number.
 */
static int __devinit dma_drv_probe(struct platform_device *dev)
{
	return __dma_init(dev);
}

/**
 * dma_drv_remove - dma driver deinital function.
 * @dev:	platform device pointer
 *
 * Returns 0 if success, otherwise means err.
 */
static int __devexit dma_drv_remove(struct platform_device *dev)
{
	return __dma_deinit();
}

/**
 * dma_drv_suspend - dma driver suspend function.
 * @dev:	platform device pointer
 * @state:	power state
 *
 * Returns 0 if success, otherwise means err.
 */
int dma_drv_suspend(struct device *dev)
{
	if(NORMAL_STANDBY == standby_type) {
 		DMA_INF("%s: normal standby, line %d\n", __func__, __LINE__);
		/* close dma mode clock */
		if(NULL != g_dma_mod_clk && !IS_ERR(g_dma_mod_clk)) {
			if(0 != clk_reset(g_dma_mod_clk, AW_CCU_CLK_RESET))
				printk("%s err: clk_reset failed\n", __func__);
			clk_disable(g_dma_mod_clk);
			clk_put(g_dma_mod_clk);
			g_dma_mod_clk = NULL;
			DMA_INF("%s: close dma mod clock success\n", __func__);
		}
	} else if(SUPER_STANDBY == standby_type) {
 		DMA_INF("%s: super standby, line %d\n", __func__, __LINE__);

		if(0 != dma_clk_deinit())
			DMA_ERR("%s err, dma_clk_deinit failed\n", __func__);
	}
	return 0;
}

/**
 * dma_drv_resume - dma driver resume function.
 * @dev:	platform device pointer
 *
 * Returns 0 if success, otherwise means err.
 */
int dma_drv_resume(struct device *dev)
{
	if(NORMAL_STANDBY == standby_type) {
 		DMA_INF("%s: normal standby, line %d\n", __func__, __LINE__);
		/* enable dma mode clock */
		g_dma_mod_clk = clk_get(NULL, CLK_MOD_DMA);
		if(NULL == g_dma_mod_clk || IS_ERR(g_dma_mod_clk)) {
			printk("%s err: clk_get %s failed\n", __func__, CLK_MOD_DMA);
			return -EPERM;
		}
		WARN_ON(0 != clk_enable(g_dma_mod_clk));
		WARN_ON(0 != clk_reset(g_dma_mod_clk, AW_CCU_CLK_NRESET));
		DMA_INF("%s: open dma mod clock success\n", __func__);
	} else if(SUPER_STANDBY == standby_type) {
 		DMA_INF("%s: super standby, line %d\n", __func__, __LINE__);

		if(0 != dma_clk_init())
			DMA_ERR("%s err, dma_clk_init failed\n", __func__);
	}
	return 0;
}

static const struct dev_pm_ops sw_dmac_pm = {
	.suspend	= dma_drv_suspend,
	.resume		= dma_drv_resume,
};
static struct platform_driver sw_dmac_driver = {
	.probe          = dma_drv_probe,
	.remove         = __devexit_p(dma_drv_remove),
	.driver         = {
		.name   = "sw_dmac",
		.owner  = THIS_MODULE,
		.pm 	= &sw_dmac_pm,
		},
};

/**
 * drv_dma_init - dma driver register function
 *
 * Returns 0 if success, otherwise means err.
 */
static int __init drv_dma_init(void)
{
	if(platform_driver_register(&sw_dmac_driver))
		printk("%s(%d) err: platform_driver_register failed\n", __func__, __LINE__);
	return 0;
}
arch_initcall(drv_dma_init);

