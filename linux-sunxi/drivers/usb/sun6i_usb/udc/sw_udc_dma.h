/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc_dma.h
*
* Author 		: javen
*
* Description 	: DMA ������
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-3-3            1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_UDC_DMA_H__
#define  __SW_UDC_DMA_H__

//---------------------------------------------------------------
//  �� ����
//---------------------------------------------------------------
#ifdef  SW_UDC_DMA
#define  is_udc_support_dma()       1
#else
#define  is_udc_support_dma()       0
#endif


//---------------------------------------------------------------
//  ���� ����
//---------------------------------------------------------------
void sw_udc_switch_bus_to_dma(struct sw_udc_ep *ep, u32 is_tx);
void sw_udc_switch_bus_to_pio(struct sw_udc_ep *ep, __u32 is_tx);

void sw_udc_enable_dma_channel_irq(struct sw_udc_ep *ep);
void sw_udc_disable_dma_channel_irq(struct sw_udc_ep *ep);

void sw_udc_dma_set_config(struct sw_udc_ep *ep, struct sw_udc_request *req, __u32 buff_addr, __u32 len);
void sw_udc_dma_start(struct sw_udc_ep *ep, __u32 fifo, __u32 buffer, __u32 len);
void sw_udc_dma_stop(struct sw_udc_ep *ep);
__u32 sw_udc_dma_transmit_length(struct sw_udc_ep *ep, __u32 is_in, __u32 buffer_addr);
int sw_udc_dma_is_busy(struct sw_udc_ep *ep);

int sw_udc_dma_channel_init(struct sw_udc *dev, struct sw_udc_ep *udc_ep);
void sw_udc_dma_channel_exit(struct sw_udc *dev, struct sw_udc_ep *udc_ep);
int sw_udc_dma_channel_available(struct sw_udc_ep *udc_ep);

__s32 sw_udc_dma_probe(struct sw_udc *dev);
__s32 sw_udc_dma_remove(struct sw_udc *dev);

#endif   //__SW_UDC_DMA_H__

