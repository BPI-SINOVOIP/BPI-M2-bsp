/*
 * arch/arm/mach-sun6i/dma/dma_regs.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma regs defination
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_REGS_H
#define __DMA_REGS_H

/* dma reg off from DMAC_IO_BASE */
#define DMA_IRQ_EN_REG0_OFF            		( 0x0000                        )
#define DMA_IRQ_EN_REG1_OFF            		( 0x0004                        )
#define DMA_IRQ_PEND_REG0_OFF            	( 0x0010                        )
#define DMA_IRQ_PEND_REG1_OFF            	( 0x0014                        )
#define DMA_STATE_REG_OFF            		( 0x0030                        )

#define DMA_EN_REG_OFF(chan)            	( 0x100 + ((chan) << 6)        ) /* ( 0x100 + (chan) * 0x40        ) */
#define DMA_PAUSE_REG_OFF(chan)            	( 0x100 + ((chan) << 6) + 0x4  )
#define DMA_START_REG_OFF(chan)            	( 0x100 + ((chan) << 6) + 0x8  )
#define DMA_CFG_REG_OFF(chan)            	( 0x100 + ((chan) << 6) + 0xC  )
#define DMA_CUR_SRC_REG_OFF(chan)            	( 0x100 + ((chan) << 6) + 0x10 )
#define DMA_CUR_DST_REG_OFF(chan)            	( 0x100 + ((chan) << 6) + 0x14 )
#define DMA_BCNT_LEFT_REG_OFF(chan)            	( 0x100 + ((chan) << 6) + 0x18 )
#define DMA_PARA_REG_OFF(chan)            	( 0x100 + ((chan) << 6) + 0x1C )

/* reg offset from channel base */
#define DMA_OFF_REG_EN            		( 0x0000                       )
#define DMA_OFF_REG_PAUSE            		( 0x0004                       )
#define DMA_OFF_REG_START            		( 0x0008                       )
#define DMA_OFF_REG_CFG            		( 0x000C                       )
#define DMA_OFF_REG_CUR_SRC            		( 0x0010                       )
#define DMA_OFF_REG_CUR_DST            		( 0x0014                       )
#define DMA_OFF_REG_BCNT_LEFT            	( 0x0018                       )
#define DMA_OFF_REG_PARA            		( 0x001C                       )

/* bits offset */
#define DMA_OFF_BITS_SDRQ            		( 0                            )
#define DMA_OFF_BITS_DDRQ            		( 16                           )

/* dma reg addr */
#define DMA_IRQ_EN_REG0            		( AW_VIR_DMA_BASE + DMA_IRQ_EN_REG0_OFF 	)
#define DMA_IRQ_EN_REG1            		( AW_VIR_DMA_BASE + DMA_IRQ_EN_REG1_OFF 	)
#define DMA_IRQ_PEND_REG0            		( AW_VIR_DMA_BASE + DMA_IRQ_PEND_REG0_OFF 	)
#define DMA_IRQ_PEND_REG1            		( AW_VIR_DMA_BASE + DMA_IRQ_PEND_REG1_OFF 	)
#define DMA_STATE_REG            		( AW_VIR_DMA_BASE + DMA_STATE_REG_OFF		)
#define DMA_EN_REG(chan)            		( AW_VIR_DMA_BASE + DMA_EN_REG_OFF(chan)	)
#define DMA_PAUSE_REG(chan)            		( AW_VIR_DMA_BASE + DMA_PAUSE_REG_OFF(chan)	)
#define DMA_START_REG(chan)            		( AW_VIR_DMA_BASE + DMA_START_REG_OFF(chan)	)
#define DMA_CFG_REG(chan)            		( AW_VIR_DMA_BASE + DMA_CFG_REG_OFF(chan)	)
#define DMA_CUR_SRC_REG(chan)            	( AW_VIR_DMA_BASE + DMA_CUR_SRC_REG_OFF(chan)	)
#define DMA_CUR_DST_REG(chan)            	( AW_VIR_DMA_BASE + DMA_CUR_DST_REG_OFF(chan)	)
#define DMA_BCNT_LEFT_REG(chan)            	( AW_VIR_DMA_BASE + DMA_BCNT_LEFT_REG_OFF(chan))
#define DMA_PARA_REG(chan)            		( AW_VIR_DMA_BASE + DMA_PARA_REG_OFF(chan)	)

/* dma reg rw */
#define DMA_READ_REG(reg)			readl(reg)
#define DMA_WRITE_REG(val, reg)			writel(val, reg)

#endif  /* __DMA_REGS_H */

