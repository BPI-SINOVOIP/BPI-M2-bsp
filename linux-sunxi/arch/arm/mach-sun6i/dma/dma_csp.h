/*
 * arch/arm/mach-sun6i/dma/dma_csp.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma csp header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_CSP_H
#define __DMA_CSP_H

extern struct clk *g_dma_ahb_clk;
extern struct clk *g_dma_mod_clk;

u32 dma_clk_init(void);
u32 dma_clk_deinit(void);

void csp_dma_init(void);

/**
 * csp_dma_chan_set_startaddr - set the des's start phys addr to start reg, then we can start the dma
 * @pchan:	dma channel handle
 * @ustart_addr: dma channel start physcal address to set
 */
static void inline csp_dma_chan_set_startaddr(dma_channel_t * pchan, u32 ustart_addr)
{
	DMA_WRITE_REG(ustart_addr, pchan->reg_base + DMA_OFF_REG_START);
}

/**
 * csp_dma_chan_start - start the dma channel
 * @pchan:	dma channel handle
 */
static void inline csp_dma_chan_start(dma_channel_t * pchan)
{
	DMA_WRITE_REG(1, pchan->reg_base + DMA_OFF_REG_EN);
}

/**
 * csp_dma_chan_pause - pause the dma channel
 * @pchan:	dma channel handle
 */
static void inline csp_dma_chan_pause(dma_channel_t * pchan)
{
	DMA_WRITE_REG(1, pchan->reg_base + DMA_OFF_REG_PAUSE);
}

/**
 * csp_dma_chan_resume - resume the dma channel
 * @pchan:	dma channel handle
 */
static void inline csp_dma_chan_resume(dma_channel_t * pchan)
{
	DMA_WRITE_REG(0, pchan->reg_base + DMA_OFF_REG_PAUSE);
}

/**
 * csp_dma_chan_stop - stop the dma channel
 * @pchan:	dma channel handle
 */
static void inline csp_dma_chan_stop(dma_channel_t * pchan)
{
	DMA_WRITE_REG(0, pchan->reg_base + DMA_OFF_REG_EN);
}

/**
 * csp_dma_chan_get_status - get dma channel status
 * @pchan:	dma channel handle
 *
 * Returns 1 indicate channel is busy, 0 idle
 */
static u32 inline csp_dma_chan_get_status(dma_channel_t * pchan)
{
	return ((DMA_READ_REG(DMA_STATE_REG) >> pchan->id) & 1);
}

/**
 * csp_dma_chan_get_cur_srcaddr - get dma channel's cur src addr reg value
 *
 * Returns the channel's cur src addr reg value
 */
static u32 inline csp_dma_chan_get_cur_srcaddr(dma_channel_t * pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_SRC);
}

/**
 * csp_dma_chan_get_cur_dstaddr - get dma channel's cur dst addr reg value
 * @pchan:	dma channel handle
 *
 * Returns the channel's cur dst addr reg value
 */
static u32 inline csp_dma_chan_get_cur_dstaddr(dma_channel_t * pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_DST);
}

/**
 * csp_dma_chan_get_left_bytecnt - get dma channel's left byte cnt
 * @pchan:	dma channel handle
 *
 * Returns the channel's left byte cnt
 */
static u32 inline csp_dma_chan_get_left_bytecnt(dma_channel_t * pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_BCNT_LEFT);
}

/**
 * csp_dma_chan_get_startaddr - get dma channel's start address reg value
 * @pchan:	dma channel handle
 *
 * Returns the dma channel's start address reg value
 */
static u32 inline csp_dma_chan_get_startaddr(dma_channel_t *pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_START);
}

/**
 * csp_dma_chan_irq_enable - enable dma channel irq
 * @pchan:	dma channel handle
 * @irq_type:	irq type that will be enabled
 */
static void inline csp_dma_chan_irq_enable(dma_channel_t * pchan, u32 irq_type)
{
	u32 	uTemp = 0;

	if(pchan->id < 8) {
		uTemp = DMA_READ_REG(DMA_IRQ_EN_REG0);
		uTemp &= (~(0xf << (pchan->id << 2)));
		uTemp |= (irq_type << (pchan->id << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_EN_REG0);
	} else {
		uTemp = DMA_READ_REG(DMA_IRQ_EN_REG1);
		uTemp &= (~(0xf << ((pchan->id - 8) << 2)));
		uTemp |= (irq_type << ((pchan->id - 8) << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_EN_REG1);
	}
}

/**
 * csp_dma_chan_get_status - get dma channel irq pending val
 * @pchan:	dma channel handle
 *
 * Returns the irq pend value, eg: 0b101
 */
static u32 inline csp_dma_chan_get_irqpend(dma_channel_t * pchan)
{
	u32 	uret = 0;
	u32 	utemp = 0;

	if(pchan->id < 8) {
		utemp = DMA_READ_REG(DMA_IRQ_PEND_REG0);
		uret = (utemp >> (pchan->id << 2)) & 0x7;
	} else {
		utemp = DMA_READ_REG(DMA_IRQ_PEND_REG1);
		uret = (utemp >> ((pchan->id - 8) << 2)) & 0x7;
	}
	return uret;
}

/**
 * csp_dma_chan_clear_irqpend - clear the dma channel irq pending
 * @pchan:	dma channel handle
 * @irq_type:	irq type that willbe cleared, eg: CHAN_IRQ_HD|CHAN_IRQ_FD
 */
static void inline csp_dma_chan_clear_irqpend(dma_channel_t * pchan, u32 irq_type)
{
	u32 	uTemp = 0;

	if(pchan->id < 8) {
		uTemp = DMA_READ_REG(DMA_IRQ_PEND_REG0);
		uTemp &= (irq_type << (pchan->id << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_PEND_REG0);
	} else {
		uTemp = DMA_READ_REG(DMA_IRQ_PEND_REG1);
		uTemp &= (irq_type << ((pchan->id - 8) << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_PEND_REG1);
	}
}

/**
 * csp_dma_clear_irqpend - clear dma irq pending register
 * @index:	irq pend reg index, 0 or 1
 *
 * Returns the irq pend value before cleared, 0xffffffff if failed
 */
static u32 inline csp_dma_clear_irqpend(u32 index)
{
	u32 	uret = 0;
	u32 	ureg_addr = 0;

	if(0 == index)
		ureg_addr = (u32)DMA_IRQ_PEND_REG0;
	else if(1 == index)
		ureg_addr = (u32)DMA_IRQ_PEND_REG1;
	else {
		DMA_ERR("%s err, line %d\n", __func__, __LINE__);
		return 0xffffffff;
	}

	uret = DMA_READ_REG(ureg_addr);
	DMA_WRITE_REG(uret, ureg_addr);
	return uret;
}

#endif  /* __DMA_CSP_H */

