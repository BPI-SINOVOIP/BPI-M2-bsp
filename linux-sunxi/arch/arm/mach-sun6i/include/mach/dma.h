/*
 * arch/arm/mach-sun6i/include/mach/dma.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma driver header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SW_DMA_H
#define __SW_DMA_H

#include <mach/hardware.h>

/* burst length */
#define X_SIGLE   0
#define X_BURST   1
#define X_TIPPL	  2

/* data width */
#define X_BYTE    0
#define X_HALF    1
#define X_WORD    2

/* address mode */
#define A_LN      0x0
#define A_IO      0x1

/*
 * data width and burst length combination
 * index for xfer_arr[]
 */
enum xferunit_e {
	/* des:X_SIGLE  src:X_SIGLE */
	DMAXFER_D_SBYTE_S_SBYTE,
	DMAXFER_D_SBYTE_S_SHALF,
	DMAXFER_D_SBYTE_S_SWORD,
	DMAXFER_D_SHALF_S_SBYTE,
	DMAXFER_D_SHALF_S_SHALF,
	DMAXFER_D_SHALF_S_SWORD,
	DMAXFER_D_SWORD_S_SBYTE,
	DMAXFER_D_SWORD_S_SHALF,
	DMAXFER_D_SWORD_S_SWORD,

	/* des:X_SIGLE  src:X_BURST */
	DMAXFER_D_SBYTE_S_BBYTE,
	DMAXFER_D_SBYTE_S_BHALF,
	DMAXFER_D_SBYTE_S_BWORD,
	DMAXFER_D_SHALF_S_BBYTE,
	DMAXFER_D_SHALF_S_BHALF,
	DMAXFER_D_SHALF_S_BWORD,
	DMAXFER_D_SWORD_S_BBYTE,
	DMAXFER_D_SWORD_S_BHALF,
	DMAXFER_D_SWORD_S_BWORD,

	/* des:X_SIGLE   src:X_TIPPL */
	DMAXFER_D_SBYTE_S_TBYTE,
	DMAXFER_D_SBYTE_S_THALF,
	DMAXFER_D_SBYTE_S_TWORD,
	DMAXFER_D_SHALF_S_TBYTE,
	DMAXFER_D_SHALF_S_THALF,
	DMAXFER_D_SHALF_S_TWORD,
	DMAXFER_D_SWORD_S_TBYTE,
	DMAXFER_D_SWORD_S_THALF,
	DMAXFER_D_SWORD_S_TWORD,

	/* des:X_BURST  src:X_BURST */
	DMAXFER_D_BBYTE_S_BBYTE,
	DMAXFER_D_BBYTE_S_BHALF,
	DMAXFER_D_BBYTE_S_BWORD,
	DMAXFER_D_BHALF_S_BBYTE,
	DMAXFER_D_BHALF_S_BHALF,
	DMAXFER_D_BHALF_S_BWORD,
	DMAXFER_D_BWORD_S_BBYTE,
	DMAXFER_D_BWORD_S_BHALF,
	DMAXFER_D_BWORD_S_BWORD,

	/* des:X_BURST   src:X_SIGLE */
	DMAXFER_D_BBYTE_S_SBYTE,
	DMAXFER_D_BBYTE_S_SHALF,
	DMAXFER_D_BBYTE_S_SWORD,
	DMAXFER_D_BHALF_S_SBYTE,
	DMAXFER_D_BHALF_S_SHALF,
	DMAXFER_D_BHALF_S_SWORD,
	DMAXFER_D_BWORD_S_SBYTE,
	DMAXFER_D_BWORD_S_SHALF,
	DMAXFER_D_BWORD_S_SWORD,

	/* des:X_BURST   src:X_TIPPL */
	DMAXFER_D_BBYTE_S_TBYTE,
	DMAXFER_D_BBYTE_S_THALF,
	DMAXFER_D_BBYTE_S_TWORD,
	DMAXFER_D_BHALF_S_TBYTE,
	DMAXFER_D_BHALF_S_THALF,
	DMAXFER_D_BHALF_S_TWORD,
	DMAXFER_D_BWORD_S_TBYTE,
	DMAXFER_D_BWORD_S_THALF,
	DMAXFER_D_BWORD_S_TWORD,

	/* des:X_TIPPL   src:X_TIPPL */
	DMAXFER_D_TBYTE_S_TBYTE,
	DMAXFER_D_TBYTE_S_THALF,
	DMAXFER_D_TBYTE_S_TWORD,
	DMAXFER_D_THALF_S_TBYTE,
	DMAXFER_D_THALF_S_THALF,
	DMAXFER_D_THALF_S_TWORD,
	DMAXFER_D_TWORD_S_TBYTE,
	DMAXFER_D_TWORD_S_THALF,
	DMAXFER_D_TWORD_S_TWORD,

	/* des:X_TIPPL   src:X_SIGLE */
	DMAXFER_D_TBYTE_S_SBYTE,
	DMAXFER_D_TBYTE_S_SHALF,
	DMAXFER_D_TBYTE_S_SWORD,
	DMAXFER_D_THALF_S_SBYTE,
	DMAXFER_D_THALF_S_SHALF,
	DMAXFER_D_THALF_S_SWORD,
	DMAXFER_D_TWORD_S_SBYTE,
	DMAXFER_D_TWORD_S_SHALF,
	DMAXFER_D_TWORD_S_SWORD,

	/* des:X_TIPPL   src:X_BURST */
	DMAXFER_D_TBYTE_S_BBYTE,
	DMAXFER_D_TBYTE_S_BHALF,
	DMAXFER_D_TBYTE_S_BWORD,
	DMAXFER_D_THALF_S_BBYTE,
	DMAXFER_D_THALF_S_BHALF,
	DMAXFER_D_THALF_S_BWORD,
	DMAXFER_D_TWORD_S_BBYTE,
	DMAXFER_D_TWORD_S_BHALF,
	DMAXFER_D_TWORD_S_BWORD,
	DMAXFER_MAX
};

/*
 * src/dst address type
 * index for addrtype_arr[]
 */
enum addrt_e {
	DMAADDRT_D_LN_S_LN,
	DMAADDRT_D_LN_S_IO,
	DMAADDRT_D_IO_S_LN,
	DMAADDRT_D_IO_S_IO,
	DMAADDRT_MAX
};

/* dma channel irq type */
enum dma_chan_irq_type {
	CHAN_IRQ_NO 	= 0,			/* none */
	CHAN_IRQ_HD	= (0b001	),	/* package half done irq */
	CHAN_IRQ_FD	= (0b010	),	/* package full done irq */
	CHAN_IRQ_QD	= (0b100	)	/* queue end irq */
};

/*
 * dma config information
 */
struct dma_config_t {
	/*
	 * data length and burst length combination in DDMA and NDMA
	 * eg: DMAXFER_D_SWORD_S_SWORD, DMAXFER_D_SBYTE_S_BBYTE
	 */
	enum xferunit_e	xfer_type;
	/*
	 * NDMA/DDMA src/dst address type
	 * eg: DMAADDRT_D_INC_S_INC(NDMA addr type),
	 *     DMAADDRT_D_LN_S_LN / DMAADDRT_D_LN_S_IO(DDMA addr type)
	 */
	enum addrt_e	address_type;
	u32		para;		/* dma para reg */
	u32 		irq_spt;	/* channel irq supported, eg: CHAN_IRQ_HD | CHAN_IRQ_FD */
	u32		src_addr;	/* src phys addr */
	u32		dst_addr;	/* dst phys addr */
	u32		byte_cnt;	/* byte cnt for src_addr/dst_addr transfer */
	bool		bconti_mode;	/* continue mode */
	u8		src_drq_type;	/* src drq type */
	u8		dst_drq_type;	/* dst drq type */
};

/* src drq type */
enum drqsrc_type_e {
	DRQSRC_SRAM		= 0,
	DRQSRC_SDRAM		= 1,
	DRQSRC_SPDIFRX		= 2,
	DRQSRC_DAUDIO_0_RX	= 3,
	DRQSRC_DAUDIO_1_RX	= 4,
	DRQSRC_NAND0		= 5,
	DRQSRC_UART0RX		= 6,
	DRQSRC_UART1RX 		= 7,
	DRQSRC_UART2RX		= 8,
	DRQSRC_UART3RX		= 9,
	DRQSRC_UART4RX		= 10,
	DRQSRC_HDMI_DDC		= 13,
	DRQSRC_HDMI_AUDIO	= 14,
	DRQSRC_AUDIO_CODEC	= 15,
	DRQSRC_SS			= 16,
	DRQSRC_OTG_EP1		= 17,
	DRQSRC_OTG_EP2		= 18,
	DRQSRC_OTG_EP3		= 19,
	DRQSRC_OTG_EP4		= 20,
	DRQSRC_OTG_EP5		= 21,
	DRQSRC_UART5RX		= 22,
	DRQSRC_SPI0RX		= 23,
	DRQSRC_SPI1RX		= 24,
	DRQSRC_SPI2RX		= 25,
	DRQSRC_SPI3RX		= 26,
	DRQSRC_TP		= 27,
	DRQSRC_NAND1		= 28,
	DRQSRC_MTC_ACC		= 29,
	DRQSRC_DIGITAL_MIC	= 30
};

/* dst drq type */
enum drqdst_type_e {
	DRQDST_SRAM		= 0,
	DRQDST_SDRAM		= 1,
	DRQDST_SPDIFTX		= 2,
	DRQDST_DAUDIO_0_TX	= 3,
	DRQDST_DAUDIO_1_TX	= 4,
	DRQDST_NAND0		= 5,
	DRQDST_UART0TX		= 6,
	DRQDST_UART1TX 		= 7,
	DRQDST_UART2TX		= 8,
	DRQDST_UART3TX		= 9,
	DRQDST_UART4TX		= 10,
	DRQDST_TCON0		= 11,
	DRQDST_TCON1		= 12,
	DRQDST_HDMI_DDC		= 13,
	DRQDST_HDMI_AUDIO	= 14,
	DRQDST_AUDIO_CODEC	= 15,
	DRQDST_SS			= 16,
	DRQDST_OTG_EP1		= 17,
	DRQDST_OTG_EP2		= 18,
	DRQDST_OTG_EP3		= 19,
	DRQDST_OTG_EP4		= 20,
	DRQDST_OTG_EP5		= 21,
	DRQDST_UART5TX		= 22,
	DRQDST_SPI0TX		= 23,
	DRQDST_SPI1TX		= 24,
	DRQDST_SPI2TX		= 25,
	DRQDST_SPI3TX		= 26,
	DRQDST_NAND1		= 28,
	DRQDST_MTC_ACC		= 29,
	DRQDST_DIGITAL_MIC	= 30
};

/* dma operation type */
enum dma_op_type_e {
	DMA_OP_START,  			/* start dma */
	DMA_OP_PAUSE,  			/* pause transferring */
	DMA_OP_RESUME,  		/* resume transferring */
	DMA_OP_STOP,  			/* stop dma */

	DMA_OP_GET_STATUS,  		/* get channel status: idle/busy */
	DMA_OP_GET_CUR_SRC_ADDR,  	/* get current src address */
	DMA_OP_GET_CUR_DST_ADDR,  	/* get current dst address */
	DMA_OP_GET_BYTECNT_LEFT,  	/* get byte cnt left */

	DMA_OP_SET_OP_CB,		/* set operation callback */
	DMA_OP_SET_HD_CB,		/* set half done callback */
	DMA_OP_SET_FD_CB,		/* set full done callback */
	DMA_OP_SET_QD_CB,		/* set queue done callback */
};

/* dma call back cause */
enum dma_cb_cause_e {
	DMA_CB_OK,		/* call back because success, eg: buf done */
	DMA_CB_ABORT		/* call back because abort, eg: if stop the channel,
				 * need to abort the rest buffer in buf chan
				 */
};

/*
 * phase for dma enqueue operation, i.e. when do we call enqueue operation
 */
enum dma_enque_phase_e {
	ENQUE_PHASE_NORMAL,	/* enqueued by app(dma's caller) directly, not by callback func */
	ENQUE_PHASE_HD,		/* enqueued by half_done callback function */
	ENQUE_PHASE_FD,		/* enqueued by full_done callback function */
	ENQUE_PHASE_QD		/* enqueued by queue_done callback function */
};

/* dma handle type defination */
typedef void * dm_hdl_t;

/* dma callback func */
typedef u32 (* dma_cb)(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause);
typedef u32 (* dma_op_cb)(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op);

/* dma callback struct */
struct dma_cb_t {
	dma_cb 		func;	/* dma callback fuction */
	void 		*parg;	/* args of func */
};

/* dma operation callback struct */
struct dma_op_cb_t {
	dma_op_cb 	func;	/* dma operation callback fuction */
	void 		*parg;	/* args of func */
};

/* dma channle work mode */
enum dma_work_mode_e {
	DMA_WORK_MODE_INVALID,	/* invalid work mode */
	DMA_WORK_MODE_CHAIN,	/* chain mode
				 * buffer will link in chain, hw can transfer them at one time.
				 * in this case, irq for the middle buffer maybe lost.
				 */
	DMA_WORK_MODE_SINGLE	/* single mode
				 * buffer will NOT link in chain, hw can only transfer one buffer at once.
				 * irq for every buffer will be treated.
				 */
};

/* dma export symbol */
dm_hdl_t sw_dma_request(char * name, enum dma_work_mode_e work_mode);
u32 sw_dma_release(dm_hdl_t dma_hdl);
u32 sw_dma_enqueue(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt,	enum dma_enque_phase_e phase);
u32 sw_dma_config(dm_hdl_t dma_hdl, struct dma_config_t *pcfg, enum dma_enque_phase_e phase);
u32 sw_dma_ctl(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg);
int sw_dma_getposition(dm_hdl_t dma_hdl, u32 *pSrc, u32 *pDst);
void sw_dma_dump_chan(dm_hdl_t dma_hdl);

#endif /* __SW_DMA_H */

