/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hcd_dma.c
*
* Author 		: javen
*
* Description 	: dma操作
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
*
*************************************************************************************
*/

#include  "../include/sw_hcd_core.h"
#include  "../include/sw_hcd_dma.h"
#include <asm/cacheflush.h>
#include <mach/dma.h>

#ifdef  SW_USB_FPGA

//static struct sw_hcd_qh sw_hcd_dma_qh;
static struct sw_hcd_qh *sw_hcd_dma_qh = NULL;

static int is_start = 0;

extern void sw_hcd_dma_completion(struct sw_hcd *sw_hcd, u8 epnum, u8 transmit);

static void hcd_CleanFlushDCacheRegion(void *adr, __u32 bytes)
{
	__cpuc_flush_dcache_area(adr, bytes + (1 << 5) * 2 - 2);
}

/*
*******************************************************************************
*                     sw_hcd_switch_bus_to_dma
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
void sw_hcd_switch_bus_to_dma(struct sw_hcd_qh *qh, u32 is_in)
{
	DMSG_DBG_DMA("sw_hcd_switch_bus_to_dma\n");
#if 0
	if(is_in){ /* ep in, rx */
		USBC_SelectBus(qh->hw_ep->sw_hcd->sw_hcd_io->usb_bsp_hdle,
			           USBC_IO_TYPE_DMA,
			           USBC_EP_TYPE_RX,
			           qh->hw_ep->epnum);
	}else{  /* ep out, tx */
		USBC_SelectBus(qh->hw_ep->sw_hcd->sw_hcd_io->usb_bsp_hdle,
					   USBC_IO_TYPE_DMA,
					   USBC_EP_TYPE_TX,
					   qh->hw_ep->epnum);
	}
#endif
    return;
}
EXPORT_SYMBOL(sw_hcd_switch_bus_to_dma);

/*
*******************************************************************************
*                     sw_hcd_switch_bus_to_pio
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
void sw_hcd_switch_bus_to_pio(struct sw_hcd_qh *qh, __u32 is_in)
{
	DMSG_DBG_DMA("sw_hcd_switch_bus_to_pio\n");

	//USBC_SelectBus(qh->hw_ep->sw_hcd->sw_hcd_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

    return;
}
EXPORT_SYMBOL(sw_hcd_switch_bus_to_pio);

/*
*******************************************************************************
*                     sw_hcd_enable_dma_channel_irq
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
void sw_hcd_enable_dma_channel_irq(struct sw_hcd_qh *qh)
{
	DMSG_DBG_DMA("sw_hcd_enable_dma_channel_irq\n");

    return;
}
EXPORT_SYMBOL(sw_hcd_enable_dma_channel_irq);

/*
*******************************************************************************
*                     sw_hcd_disable_dma_channel_irq
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
void sw_hcd_disable_dma_channel_irq(struct sw_hcd_qh *qh)
{
	DMSG_DBG_DMA("sw_hcd_disable_dma_channel_irq\n");

    return;
}
EXPORT_SYMBOL(sw_hcd_disable_dma_channel_irq);

/*
*******************************************************************************
*                     sw_hcd_dma_set_config
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
void sw_hcd_dma_set_config(struct sw_hcd_qh *qh, __u32 buff_addr, __u32 len)
{

	__u32 is_in				= 0;
	__u32 packet_size		= 0;
	__u32 usbc_base 		= 0;
	__u32 fifo_addr 	   	= 0;
	u32 para                = 0;
	int ret                 = 0;

	enum drqsrc_type_e usbc_no = 0;

 	struct dma_config_t DmaConfig;

	memset(&DmaConfig, 0, sizeof(DmaConfig));

	is_in = is_direction_in(qh);
	packet_size = qh->maxpacket;

	usbc_base = (__u32)qh->hw_ep->sw_hcd->sw_hcd_io->usb_vbase; //0xf1c19000
	fifo_addr = USBC_REG_EPFIFOx(usbc_base, qh->hw_ep->epnum);

	DMSG_DBG_DMA("line:%d %s fifo_addr(0x%x, 0x%p), buff_addr = 0x%x, usbc_base = 0x%x, ep_num = %d\n",
			  __LINE__, __func__, fifo_addr, qh->hw_ep->fifo, buff_addr, usbc_base, qh->hw_ep->epnum);


	para =((packet_size >> 2) << 8 | 0x0f);

    switch(qh->hw_ep->epnum){
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

	if(is_in){ /* ep in, rx*/

		DmaConfig.src_drq_type = usbc_no;
		DmaConfig.dst_drq_type = DRQDST_SDRAM;

		DmaConfig.xfer_type = DMAXFER_D_BBYTE_S_BBYTE;

		DmaConfig.address_type = DMAADDRT_D_LN_S_IO;

		DmaConfig.irq_spt = CHAN_IRQ_QD;

		DmaConfig.src_addr = fifo_addr & 0xfffffff;
		DmaConfig.dst_addr = (virt_to_phys)((void *)buff_addr);
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

		DmaConfig.src_addr = (virt_to_phys)((void *)buff_addr);
		DmaConfig.dst_addr = fifo_addr & 0xfffffff;
		DmaConfig.byte_cnt = len;

		//DmaConfig.conti_mode = 1;
		DmaConfig.bconti_mode = false;
		DmaConfig.para = para;
	}

	//memcpy(&sw_hcd_dma_qh, qh, sizeof(sw_hcd_dma_qh));
	qh->dma_transfer_len = len;
	sw_hcd_dma_qh = qh;

	DMSG_DBG_DMA("line:%d %s dma_hdle = 0x%x qh = 0x%p, sw_hcd_dma_qh = 0x%p\n", __LINE__, __func__, qh->hw_ep->sw_hcd->sw_hcd_dma.dma_hdle, qh, sw_hcd_dma_qh);

	hcd_CleanFlushDCacheRegion((void *)buff_addr, len);

	ret = sw_dma_config((dm_hdl_t)qh->hw_ep->sw_hcd->sw_hcd_dma.dma_hdle, &DmaConfig, ENQUE_PHASE_NORMAL);
	if(ret  != 0) {
		DMSG_PANIC("ERR: sw_dma_config failed\n");
		return;
	}

    return;
}
EXPORT_SYMBOL(sw_hcd_dma_set_config);

/*
*******************************************************************************
*                     sw_hcd_dma_start
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
void sw_hcd_dma_start(struct sw_hcd_qh *qh, __u32 fifo, __u32 buffer, __u32 len)
{
	int ret = 0;
	DMSG_DBG_DMA("sw_hcd_dma_star:qh = 0x%p\n", qh);

	if(!is_start){
		is_start = 1;
	    ret = sw_dma_ctl((dm_hdl_t)qh->hw_ep->sw_hcd->sw_hcd_dma.dma_hdle, DMA_OP_START, NULL);
		if(ret != 0) {
			DMSG_PANIC("ERR: sw_dma_ctl start  failed\n");
			return;
		}
	}
	qh->dma_working = 1;
    return;
}
EXPORT_SYMBOL(sw_hcd_dma_start);

/*
*******************************************************************************
*                     sw_hcd_dma_stop
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
void sw_hcd_dma_stop(struct sw_hcd_qh *qh)
{
	int ret = 0;

	DMSG_DBG_DMA("sw_hcd_dma_stop\n");

	ret = sw_dma_ctl((dm_hdl_t)qh->hw_ep->sw_hcd->sw_hcd_dma.dma_hdle, DMA_OP_STOP, NULL);
	if(ret != 0) {
		DMSG_PANIC("ERR: sw_dma_ctl stop  failed\n");
		return;
	}

	is_start = 0;

	qh->dma_working = 0;
	qh->dma_transfer_len = 0;

	//memset(&sw_hcd_dma_qh, 0, sizeof(sw_udc_dma_parg_t));

    return;
}
EXPORT_SYMBOL(sw_hcd_dma_stop);

/*
*******************************************************************************
*                     sw_hcd_dma_transmit_length
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
#if 0
static __u32 sw_hcd_dma_left_length(struct sw_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{
    u32 src = 0;
    u32 dst = 0;
	__u32 dma_buffer = 0;
	__u32 left_len = 0;

	sw_dma_getposition((dm_hdl_t)qh->hw_ep->sw_hcd->sw_hcd_dma.dma_hdle, &src, &dst);
	if(is_in){
		dma_buffer = dst;
	}else{
		dma_buffer = src;
	}

	left_len = buffer_addr - dma_buffer;

	DMSG_DBG_DMA("dma transfer lenght, buffer_addr(0x%x), dma_buffer(0x%x), left_len(%d), want(%d)\n",
		      buffer_addr, dma_buffer, left_len, qh->dma_transfer_len);

    return left_len;
}

__u32 sw_hcd_dma_transmit_length(struct sw_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{

	if(qh->dma_transfer_len){
		DMSG_DBG_DMA("line:%d %s\n", __LINE__,__func__);
		return (qh->dma_transfer_len - sw_hcd_dma_left_length(qh, is_in, buffer_addr));
	}else{
		DMSG_DBG_DMA("line:%d %s\n", __LINE__,__func__);
		return qh->dma_transfer_len;
	}
}

#else
__u32 sw_hcd_dma_transmit_length(struct sw_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{
	DMSG_DBG_DMA("sw_hcd_dma_transmit_length: dma_transfer_len = %d\n", qh->dma_transfer_len);

	return qh->dma_transfer_len;
}
#endif

EXPORT_SYMBOL(sw_hcd_dma_transmit_length);

/*
*******************************************************************************
*                     sw_hcd_dma_probe
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

	struct sw_hcd_qh *qh = sw_hcd_dma_qh;

	DMSG_DBG_DMA("line:%d, %s,qh = 0x%p\n", __LINE__, __func__, qh);

	if(qh){
		qh->dma_working = 0;
		sw_hcd_dma_completion(qh->hw_ep->sw_hcd, qh->hw_ep->epnum, !is_direction_in(qh));
	}else{
		DMSG_PANIC("ERR: sw_hcd_dma_callback, dma is remove, but dma irq is happened, (0x%x, 0x%p)\n",
			       qh->hw_ep->sw_hcd->sw_hcd_dma.dma_hdle, qh);
	}

	return 0;
}
/*
*******************************************************************************
*                     sw_hcd_dma_probe
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
__u32 sw_hcd_dma_is_busy(struct sw_hcd_qh *qh)
{
	return qh->dma_working;
}
EXPORT_SYMBOL(sw_hcd_dma_is_busy);

/*
*******************************************************************************
*                     sw_hcd_dma_probe
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
__s32 sw_hcd_dma_probe(struct sw_hcd *sw_hcd)
{

	int ret = 0;
	struct dma_cb_t done_cb;
	DMSG_INFO("line:%d %s\n", __LINE__, __func__);

	strcpy(sw_hcd->sw_hcd_dma.name, sw_hcd->driver_name);
	strcat(sw_hcd->sw_hcd_dma.name, "_DMA");

	sw_hcd->sw_hcd_dma.dma_hdle = (int)sw_dma_request(sw_hcd->sw_hcd_dma.name, DMA_WORK_MODE_SINGLE);
	if(sw_hcd->sw_hcd_dma.dma_hdle == 0) {
		DMSG_PANIC("ERR: sw_dma_request failed\n");
		return -1;
	}

	DMSG_INFO("line:%d %s dma_hdle = 0x%x\n", __LINE__, __func__, sw_hcd->sw_hcd_dma.dma_hdle);

	/*set callback */
	memset(&done_cb, 0, sizeof(done_cb));

	done_cb.func = sw_udc_dma_callback;
	done_cb.parg = NULL;
	ret = sw_dma_ctl((dm_hdl_t)sw_hcd->sw_hcd_dma.dma_hdle, DMA_OP_SET_QD_CB, (void *)&done_cb);
	if(ret != 0){
		DMSG_PANIC("ERR: set callback failed\n");
		return -1;
	}


    return 0;
}
EXPORT_SYMBOL(sw_hcd_dma_probe);

/*
*******************************************************************************
*                     sw_hcd_dma_remove
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
__s32 sw_hcd_dma_remove(struct sw_hcd *sw_hcd)
{
	__u32 ret = 0;

	DMSG_INFO("sw_hcd_dma_remove\n");

	if(sw_hcd->sw_hcd_dma.dma_hdle != 0) {
		ret = sw_dma_ctl((dm_hdl_t)sw_hcd->sw_hcd_dma.dma_hdle, DMA_OP_STOP, NULL);
		if(ret != 0) {
			DMSG_PANIC("ERR: sw_udc_dma_remove: stop failed\n");
		}

		ret = sw_dma_release((dm_hdl_t)sw_hcd->sw_hcd_dma.dma_hdle);

		if(ret != 0) {
			DMSG_PANIC("sw_udc_dma_remove: sw_dma_release failed\n");
		}
		sw_hcd->sw_hcd_dma.dma_hdle = 0;
	}

	is_start = 0;

	memset(&sw_hcd_dma_qh, 0, sizeof(sw_hcd_dma_qh));

	return 0;
}
EXPORT_SYMBOL(sw_hcd_dma_remove);

#else

/*
*******************************************************************************
*                     sw_hcd_switch_bus_to_dma
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
void sw_hcd_switch_bus_to_dma(struct sw_hcd_qh *qh, u32 is_in)
{
	DMSG_DBG_DMA("sw_hcd_switch_bus_to_dma\n");

    return;
}
EXPORT_SYMBOL(sw_hcd_switch_bus_to_dma);

/*
*******************************************************************************
*                     sw_hcd_switch_bus_to_pio
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
void sw_hcd_switch_bus_to_pio(struct sw_hcd_qh *qh, __u32 is_in)
{
    return;
}
EXPORT_SYMBOL(sw_hcd_switch_bus_to_pio);

/*
*******************************************************************************
*                     sw_hcd_enable_dma_channel_irq
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
void sw_hcd_enable_dma_channel_irq(struct sw_hcd_qh *qh)
{
	DMSG_DBG_DMA("sw_hcd_enable_dma_channel_irq\n");

    return;
}
EXPORT_SYMBOL(sw_hcd_enable_dma_channel_irq);

/*
*******************************************************************************
*                     sw_hcd_disable_dma_channel_irq
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
void sw_hcd_disable_dma_channel_irq(struct sw_hcd_qh *qh)
{
	DMSG_DBG_DMA("sw_hcd_disable_dma_channel_irq\n");

    return;
}
EXPORT_SYMBOL(sw_hcd_disable_dma_channel_irq);

/*
*******************************************************************************
*                     sw_hcd_dma_set_config
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
void sw_hcd_dma_set_config(struct sw_hcd_qh *qh, __u32 buff_addr, __u32 len)
{

    return;
}
EXPORT_SYMBOL(sw_hcd_dma_set_config);

/*
*******************************************************************************
*                     sw_hcd_dma_start
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
void sw_hcd_dma_start(struct sw_hcd_qh *qh, __u32 fifo, __u32 buffer, __u32 len)
{

    return;
}
EXPORT_SYMBOL(sw_hcd_dma_start);

/*
*******************************************************************************
*                     sw_hcd_dma_stop
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
void sw_hcd_dma_stop(struct sw_hcd_qh *qh)
{
    return;
}
EXPORT_SYMBOL(sw_hcd_dma_stop);

/*
*******************************************************************************
*                     sw_hcd_dma_transmit_length
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
#if 0
static __u32 sw_hcd_dma_left_length(struct sw_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{

    return 0;
}

__u32 sw_hcd_dma_transmit_length(struct sw_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{
	return 0;

}

#else
__u32 sw_hcd_dma_transmit_length(struct sw_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{

	return 0;
}
#endif

EXPORT_SYMBOL(sw_hcd_dma_transmit_length);



/*
*******************************************************************************
*                     sw_hcd_dma_probe
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
__u32 sw_hcd_dma_is_busy(struct sw_hcd_qh *qh)
{
	return 0;
}
EXPORT_SYMBOL(sw_hcd_dma_is_busy);

/*
*******************************************************************************
*                     sw_hcd_dma_probe
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
__s32 sw_hcd_dma_probe(struct sw_hcd *sw_hcd)
{

    return 0;
}
EXPORT_SYMBOL(sw_hcd_dma_probe);

/*
*******************************************************************************
*                     sw_hcd_dma_remove
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
__s32 sw_hcd_dma_remove(struct sw_hcd *sw_hcd)
{

	return 0;
}
EXPORT_SYMBOL(sw_hcd_dma_remove);

#endif



