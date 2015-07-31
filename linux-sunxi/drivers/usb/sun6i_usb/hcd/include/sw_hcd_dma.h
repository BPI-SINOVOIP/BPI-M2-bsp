/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hcd_dma.h
*
* Author 		: javen
*
* Description 	: dma����
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_HCD_DMA_H__
#define  __SW_HCD_DMA_H__

//---------------------------------------------------------------
//  �� ����
//---------------------------------------------------------------
#if 1
#define  is_hcd_support_dma(usbc_no)   0
#else
#define  is_hcd_support_dma(usbc_no)    (usbc_no == 0)
#endif

/* ʹ��DMA������: 1����������  2��DMA���� 3����ep0 */
#define  is_sw_hcd_dma_capable(usbc_no, len, maxpacket, epnum)	(is_hcd_support_dma(usbc_no) \
        	                                             		 && (len > maxpacket) \
        	                                             		 && epnum)

//---------------------------------------------------------------
//  ���ݽṹ ����
//---------------------------------------------------------------
typedef struct sw_hcd_dma{
	char name[32];
	//struct sw_dma_client dma_client;

	int dma_hdle;	/* dma ��� */
}sw_hcd_dma_t;

//---------------------------------------------------------------
//  ���� ����
//---------------------------------------------------------------
void sw_hcd_switch_bus_to_dma(struct sw_hcd_qh *qh, u32 is_in);
void sw_hcd_switch_bus_to_pio(struct sw_hcd_qh *qh, __u32 is_in);

void sw_hcd_dma_set_config(struct sw_hcd_qh *qh, __u32 buff_addr, __u32 len);
__u32 sw_hcd_dma_is_busy(struct sw_hcd_qh *qh);

void sw_hcd_dma_start(struct sw_hcd_qh *qh, __u32 fifo, __u32 buffer, __u32 len);
void sw_hcd_dma_stop(struct sw_hcd_qh *qh);
__u32 sw_hcd_dma_transmit_length(struct sw_hcd_qh *qh, __u32 is_in, __u32 buffer_addr);

__s32 sw_hcd_dma_probe(struct sw_hcd *sw_hcd);
__s32 sw_hcd_dma_remove(struct sw_hcd *sw_hcd);

#endif   //__SW_HCD_DMA_H__


