/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc_dma.c
*
* Author 		: javen
*
* Description 	: DMA 操作集
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-3-3            1.0          create this file
*
*************************************************************************************
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <asm/cacheflush.h>
#include <mach/dma.h>

#include  "sw_udc_config.h"
#include  "sw_udc_board.h"
#include  "sw_udc_dma.h"

extern void sw_udc_dma_completion(struct sw_udc *dev, struct sw_udc_ep *ep, struct sw_udc_request *req);
extern int dma_working;
/*
*******************************************************************************
*                     sw_udc_switch_bus_to_dma
*
* Description:
*    切换 USB 总线给 DMA
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void sw_udc_switch_bus_to_dma(struct sw_udc_ep *ep, u32 is_tx)
{
#if 0
	if(!is_tx){ /* ep in, rx */
		USBC_SelectBus(ep->dev->sw_udc_io->usb_bsp_hdle,
			           USBC_IO_TYPE_DMA,
			           USBC_EP_TYPE_RX,
			           ep->num);
	}else{  /* ep out, tx */
		USBC_SelectBus(ep->dev->sw_udc_io->usb_bsp_hdle,
					   USBC_IO_TYPE_DMA,
					   USBC_EP_TYPE_TX,
					   ep->num);
	}
#endif
    return;
}

/*
*******************************************************************************
*                     sw_udc_switch_bus_to_pio
*
* Description:
*    切换 USB 总线给 PIO
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void sw_udc_switch_bus_to_pio(struct sw_udc_ep *ep, __u32 is_tx)
{
	//BC_SelectBus(ep->dev->sw_udc_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

    return;
}

/*
*******************************************************************************
*                     sw_udc_enable_dma_channel_irq
*
* Description:
*    使能 DMA channel 中断
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void sw_udc_enable_dma_channel_irq(struct sw_udc_ep *ep)
{
	DMSG_DBG_DMA("sw_udc_enable_dma_channel_irq\n");

    return;
}

/*
*******************************************************************************
*                     sw_udc_disable_dma_channel_irq
*
* Description:
*    禁止 DMA channel 中断
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void sw_udc_disable_dma_channel_irq(struct sw_udc_ep *ep)
{
	DMSG_DBG_DMA("sw_udc_disable_dma_channel_irq\n");

    return;
}

void printk_dma(struct dma_config_t *pcfg){

	DMSG_DBG_DMA("src_drq_type = 0x%x\n", pcfg->src_drq_type);
	DMSG_DBG_DMA("dst_drq_type = 0x%x\n", pcfg->dst_drq_type);
	DMSG_DBG_DMA("xfer_type = 0x%x\n",    pcfg->xfer_type);
	DMSG_DBG_DMA("address_type = 0x%x\n", pcfg->address_type);
	DMSG_DBG_DMA("xfer_type = 0x%x\n",    pcfg->xfer_type);
	DMSG_DBG_DMA("irq_spt = 0x%x\n",      pcfg->irq_spt);
	DMSG_DBG_DMA("src_addr = 0x%x\n",    	pcfg->src_addr);
	DMSG_DBG_DMA("dst_addr = 0x%x\n", 	pcfg->dst_addr);
	DMSG_DBG_DMA("byte_cnt = 0x%x\n", 	pcfg->byte_cnt);
	DMSG_DBG_DMA("bconti_mode = 0x%x\n", 	pcfg->bconti_mode);
	DMSG_DBG_DMA("para = 0x%x\n",  	    pcfg->para);
}


/*
*******************************************************************************
*                     sw_udc_dma_set_config
*
* Description:
*    配置 DMA
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void sw_udc_dma_set_config(struct sw_udc_ep *ep, struct sw_udc_request *req, __u32 buff_addr, __u32 len)
{
	__u32 is_tx				= 0;
	__u32 packet_size		= 0;
	__u32 para				= 0;
	__u32 fifo_addr         = 0;
	int ret 				= 0;
	enum drqsrc_type_e usbc_no = 0;
 	struct dma_config_t DmaConfig;

	ep->dma->req = req;
	ep->dma->dma_working = 1;
	ep->dma->dma_transfer_len = len;

	dma_working = 1;
    /* config dma controller */
	memset(&DmaConfig, 0, sizeof(DmaConfig));

	is_tx = is_tx_ep(ep);
	packet_size = ep->ep.maxpacket;

	//DMSG_INFO("line:%d, %s, ep = 0x%p, req = 0x%p\n", __LINE__, __func__, ep, req);

	fifo_addr = USBC_REG_EPFIFOx((u32)ep->dev->sw_udc_io->usb_vbase, ep->num);

	para =((packet_size >> 2) << 8 | 0x0f);

    switch(ep->num){
		case 1:
			usbc_no = DRQSRC_OTG_EP1;
		break;

		case 2:
			usbc_no = DRQSRC_OTG_EP2;
		break;

		case 3:
			usbc_no = DRQSRC_OTG_EP3;
		break;

		case 4:
			usbc_no = DRQSRC_OTG_EP4;
		break;

		case 5:
			usbc_no = DRQSRC_OTG_EP5;
		break;

		default:
			usbc_no = 0;
	}

	if(!is_tx){ /* ep out, rx*/
		DmaConfig.src_drq_type = usbc_no;
		DmaConfig.dst_drq_type = DRQDST_SDRAM;

		DmaConfig.xfer_type = DMAXFER_D_BBYTE_S_BBYTE;

		DmaConfig.address_type = DMAADDRT_D_LN_S_IO;

		DmaConfig.irq_spt = CHAN_IRQ_QD;

		DmaConfig.src_addr = fifo_addr & 0xfffffff;
		//DmaConfig.dst_addr = (virt_to_phys)((void *)buff_addr);
		DmaConfig.dst_addr = buff_addr;
		DmaConfig.byte_cnt = len;

		//DmaConfig.conti_mode = 1;
		DmaConfig.bconti_mode = false;
		DmaConfig.para = para;

	}else{ /* ep out, tx*/
		DmaConfig.src_drq_type = DRQDST_SDRAM;
		DmaConfig.dst_drq_type = usbc_no;

		DmaConfig.xfer_type = DMAXFER_D_BBYTE_S_BBYTE;

		DmaConfig.address_type = DMAADDRT_D_IO_S_LN;

		DmaConfig.irq_spt = CHAN_IRQ_QD;

		//DmaConfig.src_addr = (virt_to_phys)((void *)buff_addr);
		DmaConfig.src_addr = buff_addr;
		DmaConfig.dst_addr = fifo_addr & 0xfffffff;
		DmaConfig.byte_cnt = len;

		//DmaConfig.conti_mode = 1;
		DmaConfig.bconti_mode = false;
		DmaConfig.para = para;
	}

    printk_dma(&DmaConfig);

	ret = sw_dma_config((dm_hdl_t)ep->dma->dma_hdle, &DmaConfig, ENQUE_PHASE_NORMAL);
	if(ret  != 0) {
		DMSG_PANIC("ERR: sw_dma_config failed\n");
		return;
	}

    return;
}

/*
*******************************************************************************
*                     sw_udc_dma_start
*
* Description:
*    开始 DMA 传输
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void sw_udc_dma_start(struct sw_udc_ep *ep, __u32 fifo, __u32 buffer, __u32 len)
{
 	int ret = 0;

	if(!ep->dma->is_start){
		DMSG_DBG_DMA("%s:%d: \n", __func__, __LINE__);

		ep->dma->is_start = 1;
	    ret = sw_dma_ctl((dm_hdl_t)ep->dma->dma_hdle, DMA_OP_START, NULL);
		if(ret != 0) {
			DMSG_PANIC("ERR: sw_dma_ctl start  failed\n");
			return;
		}
	}

    return;
}

/*
*******************************************************************************
*                     sw_udc_dma_stop
*
* Description:
*    终止 DMA 传输
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void sw_udc_dma_stop(struct sw_udc_ep *ep)
{
 	int ret = 0;

	DMSG_DBG_DMA("%s:%d: \n", __func__, __LINE__);

	ep->dma->req = NULL;
	ep->dma->is_start = 0;
	ep->dma->dma_working = 0;
	ep->dma->dma_transfer_len = 0;

	dma_working = 0;
	if(ep->dma->dma_hdle){
		ret = sw_dma_ctl((dm_hdl_t)ep->dma->dma_hdle, DMA_OP_STOP, NULL);
		if(ret != 0) {
			DMSG_PANIC("ERR: sw_dma_ctl stop  failed\n");
			return;
		}
	}
    return;
}

/*
*******************************************************************************
*                     sw_udc_dma_transmit_length
*
* Description:
*    查询 DMA 已经传输的长度
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__u32 sw_udc_dma_transmit_length(struct sw_udc_ep *ep, __u32 is_in, __u32 buffer_addr)
{
    return ep->dma->dma_transfer_len;
}

/*
*******************************************************************************
*                     sw_udc_dma_probe
*
* Description:
*    DMA 初始化
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int sw_udc_dma_is_busy(struct sw_udc_ep *ep)
{
	if(ep->dma){
		return ep->dma->dma_working;
	}else{
		return 0;
	}
}

/*
*******************************************************************************
*                     sw_udc_dma_probe
*
* Description:
*    DMA 初始化
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static u32 sw_udc_dma_callback(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	struct sw_udc_dma *dma = NULL;
	struct sw_udc_request *req = NULL;
	struct sw_udc_ep *ep = NULL;

    /* DMA异常就直接退出 */
	if(cause == DMA_CB_ABORT){
		DMSG_PANIC("ERR: sw_udc_dma_callback, dma callback abort\n");
		return 0;
	}

	dma = (struct sw_udc_dma *)parg;
	if((dma == NULL) || ((int)dma_hdl != dma->dma_hdle)) {
		DMSG_PANIC("ERR: sw_udc_dma_callback parg is invalid\n");
		return 0;
	}

	if(!dma->available){
		DMSG_PANIC("ERR: sw_udc_dma_callback, dma channel is available\n");
		return 0;
	}

    /* find ep */
	ep = dma->ep;
	if(ep){
        /* find req */
		if(likely (!list_empty(&ep->queue))){
			req = list_entry(ep->queue.next, struct sw_udc_request, queue);
		}else{
			req = NULL;
		}

		if(req != dma->req){
			DMSG_PANIC("ERR: sw_udc_dma_callback req is invalid\n");
			return 0;
		}

        /* call back */
		if(req){
		    sw_udc_dma_completion(dma->dev, ep, req);
		}
	}else{
		DMSG_PANIC("ERR: sw_udc_dma_callback: dma is remove, but dma irq is happened\n");
	}

	return 0;
}

int sw_udc_dma_channel_init(struct sw_udc *dev, struct sw_udc_ep *udc_ep)
{
	int i = 0;

	DMSG_DBG_DMA("%s:%d: \n", __func__, __LINE__);

	for(i = 0; i < SW_UDC_DMA_CHANNEL_NUM; i++){
		if(dev->dma_channel[i].channel_used == 0){ //未锟斤拷占锟斤拷
			udc_ep->dma = &dev->dma_channel[i].dma;

			dev->dma_channel[i].dma.ep = udc_ep;
			dev->dma_channel[i].dma.available = 1;
			dev->dma_channel[i].channel_used = 1;

			DMSG_DBG_DMA("%s:%d: channel=%d\n", __func__, __LINE__, i);

			return 0;
		}
	}

	return -1;
}

void sw_udc_dma_channel_exit(struct sw_udc *dev, struct sw_udc_ep *udc_ep)
{
	int i = 0;

	DMSG_DBG_DMA("%s:%d: \n", __func__, __LINE__);

	if(udc_ep->dma == NULL){
		DMSG_PANIC("%s dma channel is already exit\n", udc_ep->ep.name);
		return;
	}
	dma_working = 0;
	for(i = 0; i < SW_UDC_DMA_CHANNEL_NUM; i++){
		if(dev->dma_channel[i].channel_used
			&& (&dev->dma_channel[i].dma == udc_ep->dma)){
			dev->dma_channel[i].channel_used = 0;

			sw_udc_dma_stop(udc_ep);

			dev->dma_channel[i].dma.ep = NULL;
			dev->dma_channel[i].dma.req = NULL;
			dev->dma_channel[i].dma.available = 0;
			dev->dma_channel[i].dma.is_start = 0;
			dev->dma_channel[i].dma.dma_working = 0;
			dev->dma_channel[i].dma.dma_transfer_len = 0;

			udc_ep->dma = NULL;
		}
	}
}

int sw_udc_dma_channel_available(struct sw_udc_ep *udc_ep)
{
	if(udc_ep->dma){
/*
		if(!udc_ep->dma->available){
			DMSG_INFO("%s:%d: \n", __func__, __LINE__);
		}
*/
		return udc_ep->dma->available;
	}else{
		return 0;
	}
}

/*
*******************************************************************************
*                     sw_udc_dma_probe
*
* Description:
*    DMA 初始化
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 sw_udc_dma_probe(struct sw_udc *dev)
{
	int ret = 0;
	int i = 0;
	struct dma_cb_t done_cb;

	for(i = 0; i < SW_UDC_DMA_CHANNEL_NUM; i++){
		memset(&dev->dma_channel[i], 0, sizeof(struct sw_udc_dma_channel));
		dev->dma_channel[i].dma.dev = dev;

		/* requset dma */
		sprintf(dev->dma_channel[i].dma.name, "%s_dma_%d", dev->driver_name, i);

		DMSG_DBG_DMA("%s:%d: name=%s\n", __func__, __LINE__, dev->dma_channel[i].dma.name);

		dev->dma_channel[i].dma.dma_hdle = (int)sw_dma_request(dev->dma_channel[i].dma.name, DMA_WORK_MODE_SINGLE);
		if(dev->dma_channel[i].dma.dma_hdle == 0) {
			DMSG_PANIC("ERR: sw_dma_request failed\n");
			return -1;
		}

		/* set callback */
		memset(&done_cb, 0, sizeof(done_cb));
		done_cb.func = sw_udc_dma_callback;
		done_cb.parg = &dev->dma_channel[i].dma;
		ret = sw_dma_ctl((dm_hdl_t)dev->dma_channel[i].dma.dma_hdle, DMA_OP_SET_QD_CB, (void *)&done_cb);
		if(ret != 0){
			DMSG_PANIC("ERR: set callback failed\n");
			sw_dma_release((dm_hdl_t)dev->dma_channel[i].dma.dma_hdle);
			return -1;
		}
	}

    return 0;
}

/*
*******************************************************************************
*                     sw_udc_dma_remove
*
* Description:
*    DMA 移除
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 sw_udc_dma_remove(struct sw_udc *dev)
{
 	int ret = 0;
	int i = 0;

	for(i = 0; i < SW_UDC_DMA_CHANNEL_NUM; i++){
		if(dev->dma_channel[i].dma.dma_hdle != 0) {
			ret = sw_dma_ctl((dm_hdl_t)dev->dma_channel[i].dma.dma_hdle, DMA_OP_STOP, NULL);
			if(ret != 0) {
				DMSG_PANIC("ERR: sw_udc_dma_remove: stop failed\n");
			}

			ret = sw_dma_release((dm_hdl_t)dev->dma_channel[i].dma.dma_hdle);
			if(ret != 0) {
				DMSG_PANIC("sw_udc_dma_remove: sw_dma_release failed\n");
			}

			memset(&dev->dma_channel[i], 0, sizeof(struct sw_udc_dma_channel));
		}
	}

	return 0;
}

