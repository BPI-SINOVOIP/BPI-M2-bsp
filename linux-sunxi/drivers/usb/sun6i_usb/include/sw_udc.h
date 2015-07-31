/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc.h
*
* Author 		: javen
*
* Description 	: USB Device ??????????
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-3-3            1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_UDC_H__
#define  __SW_UDC_H__

#include <linux/usb.h>
#include <linux/usb/gadget.h>
//#include <mach/dma.h>
#include <linux/dma-mapping.h>

//---------------------------------------------------------------
//
//---------------------------------------------------------------

#define  SW_UDC_DMA_CHANNEL_NUM  	4   //udc����dma channel�ĸ���

typedef struct sw_udc_dma{
	char name[32];
	int dma_hdle;

	struct sw_udc *dev;
	struct sw_udc_ep *ep;
	struct sw_udc_request *req;

	unsigned int available : 1;         /* flag. is dma channel available? */
	unsigned int is_start : 1;
	unsigned int dma_working : 1;		/* flag. is dma busy? 		*/

	__u32 dma_transfer_len;	/* dma want transfer length */
}sw_udc_dma_t;

/* dma ������� */
typedef struct sw_udc_dma_channel{
	char channel_used;

	struct sw_udc_dma dma;
}sw_udc_dma_parg_t;

/* hardware ep */
typedef struct sw_udc_ep {
	struct list_head		queue;
	unsigned long			last_io;	/* jiffies timestamp */
	struct usb_gadget		*gadget;
	struct sw_udc		    *dev;
	const struct usb_endpoint_descriptor *desc;
	struct usb_ep			ep;
	u8				        num;

	unsigned short			fifo_size;
	u8				        bEndpointAddress;
	u8				        bmAttributes;

	unsigned			    halted : 1;
	unsigned			    already_seen : 1;
	unsigned			    setup_stage : 1;
	struct sw_udc_dma		*dma;
}sw_udc_ep_t;


/* Warning : ep0 has a fifo of 16 bytes */
/* Don't try to set 32 or 64            */
/* also testusb 14 fails  wit 16 but is */
/* fine with 8                          */
//#define  EP0_FIFO_SIZE		    8
#define  EP0_FIFO_SIZE		    64

#define  SW_UDC_EP_FIFO_SIZE	    512

#define	 SW_UDC_EP_CTRL_INDEX			0x00
#define  SW_UDC_EP_BULK_IN_INDEX		0x01
#define  SW_UDC_EP_BULK_OUT_INDEX		0x02

#ifdef  SW_UDC_DOUBLE_FIFO
#define  SW_UDC_FIFO_NUM			1
#else
#define  SW_UDC_FIFO_NUM			0
#endif

#define SW_UDC_TEST_J           0x0100
#define SW_UDC_TEST_K           0x0200
#define SW_UDC_TEST_SE0_NAK     0x0300
#define SW_UDC_TEST_PACKET      0x0400

static const char ep0name [] = "ep0";
static const char ep1in_bulk_name []  = "ep1in-bulk";
static const char ep1out_bulk_name [] = "ep1out-bulk";
static const char ep2in_bulk_name []  = "ep2in-bulk";
static const char ep2out_bulk_name [] = "ep2out-bulk";
static const char ep3_iso_name []     = "ep3-iso";
static const char ep4_int_name []     = "ep4-int";
static const char ep5in_bulk_name []  = "ep5in-bulk";
static const char ep5out_bulk_name [] = "ep5out-bulk";

struct sw_udc_fifo{
    const char *name;

	u32 fifo_addr;
	u32 fifo_size;
	u8  double_fifo;
};

//fifo 8k
/*
 *    ep	   		fifo_addr  	 fifo_size
 * "ep0",	       	 	0,      	0.5k
 * "ep1in-bulk",		0.5k,  		1k
 * "ep1out-bulk",		1.5k,  		1k
 * "ep2in-bulk",		2.5k,  		1k
 * "ep2out-bulk",		3.5k, 		1k
 * "ep3-iso",			4.5k, 		1k
 * "ep4-int",			5.5k, 		0.5k
 * "ep5in-bulk",		6k,			1k
 * "ep5out-bulk",		7k,			1k
 */
#if 0
static const struct sw_udc_fifo ep_fifo[] = {
	{ep0name,          0,    512,  0},
	{ep1in_bulk_name,  512,  1024, 1},
	{ep1out_bulk_name, 1536, 1024, 1},
	{ep2in_bulk_name,  2560, 1024, 1},
	{ep2out_bulk_name, 3584, 1024, 1},
	{ep3_iso_name,     4608, 2048, 1},
	{ep4_int_name,     6656, 512,  0},
	{ep5in_bulk_name,  7168, 512,  0},
	{ep5out_bulk_name, 7680, 512,  0},
};
#else
static const struct sw_udc_fifo ep_fifo[] = {
	{ep0name,          0,    512,  0},
	{ep1in_bulk_name,  512,  1024, 1},
	{ep1out_bulk_name, 1536, 1024, 1},
	{ep2in_bulk_name,  2560, 1024, 1},
	{ep2out_bulk_name, 3584, 1024, 1},
	{ep3_iso_name,     4608, 1024, 0},
	{ep4_int_name,     5632, 512,  0},
	{ep5in_bulk_name,  6144, 1024, 1},
	{ep5out_bulk_name, 7168, 1024, 1},

};
#endif
/*
 * ep_fifo_in[i] = {n} i : ��ʾ����ep���, n : ��ʾ����ep��Ӧ��ep_fifo�е�λ��
 *
 * ���� : ep_fifo_in[2] = {3}  ��ʾ ep2_in ������ ep_fifo[3] ��
 *
 * ep3_iso_name �� ep4_int_name������ͬʱΪ tx or rx
 *
 */
static const int ep_fifo_in[] = {0, 1, 3, 5, 6, 7};
static const int ep_fifo_out[] = {0, 2, 4, 5, 6, 8};

#define SW_UDC_ENDPOINTS       ARRAY_SIZE(ep_fifo)

#define  is_tx_ep(ep)		((ep->bEndpointAddress) & USB_DIR_IN)

enum sw_buffer_map_state {
	UN_MAPPED = 0,
	PRE_MAPPED,
	SW_UDC_USB_MAPPED
};

struct sw_udc_request {
	struct list_head		queue;		/* ep's requests */
	struct usb_request		req;

	__u32 is_queue;  /* flag. ?Ƿ??Ѿ?ѹ??????? */
	enum sw_buffer_map_state map_state;
};

enum ep0_state {
        EP0_IDLE,
        EP0_IN_DATA_PHASE,
        EP0_OUT_DATA_PHASE,
        EP0_END_XFER,
        EP0_STALL,
};

/*
static const char *ep0states[]= {
        "EP0_IDLE",
        "EP0_IN_DATA_PHASE",
        "EP0_OUT_DATA_PHASE",
        "EP0_END_XFER",
        "EP0_STALL",
};
*/

//---------------------------------------------------------------
//  DMA
//---------------------------------------------------------------

/* i/o ??Ϣ */
typedef struct sw_udc_io{
	struct resource	*usb_base_res;   	/* USB  resources 		*/
	struct resource	*usb_base_req;   	/* USB  resources 		*/
	void __iomem	*usb_vbase;			/* USB  base address 	*/

	struct resource	*sram_base_res;   	/* SRAM resources 		*/
	struct resource	*sram_base_req;   	/* SRAM resources 		*/
	void __iomem	*sram_vbase;		/* SRAM base address 	*/

	struct resource	*clock_base_res;   	/* clock resources 		*/
	struct resource	*clock_base_req;   	/* clock resources 		*/
	void __iomem	*clock_vbase;		/* clock base address 	*/

	bsp_usbc_t usbc;					/* usb bsp config 		*/
	__hdle usb_bsp_hdle;				/* usb bsp handle 		*/

	__u32 clk_is_open;					/* is usb clock open? 	*/
	struct clk	*ahb_otg;				/* ahb clock handle 	*/
	struct clk	*mod_usbotg;			/* mod_usb otg clock handle 	*/
	struct clk	*mod_usbphy;			/* PHY0 clock handle 	*/
}sw_udc_io_t;

//---------------------------------------------------------------
//
//---------------------------------------------------------------
typedef struct sw_udc {
	spinlock_t			        lock;
    struct platform_device      *pdev;
	struct device		        *controller;

	struct sw_udc_ep		    ep[SW_UDC_ENDPOINTS];
	int				            address;
	struct usb_gadget		    gadget;
	struct usb_gadget_driver	*driver;

	struct sw_udc_request		fifo_req;
	u8				            fifo_buf[SW_UDC_EP_FIFO_SIZE];

	u16				            devstatus;
	u32				            port_status;
	int				            ep0state;

	unsigned			        got_irq : 1;

	unsigned			        req_std : 1;
	unsigned			        req_config : 1;
	unsigned			        req_pending : 1;
	u8				            vbus;
	struct dentry			    *regs_info;

	sw_udc_io_t					*sw_udc_io;
	char 						driver_name[32];
	__u32 						usbc_no;	/* ???????˿ں? 	*/

	struct sw_udc_dma_channel    dma_channel[SW_UDC_DMA_CHANNEL_NUM];
	u32							stoped;		/* ??????ֹͣ???? 	*/
	u32 						irq_no;		/* USB ?жϺ? 		*/
}sw_udc_t;

enum sw_udc_cmd_e {
	SW_UDC_P_ENABLE	= 1,	/* Pull-up enable        */
	SW_UDC_P_DISABLE = 2,	/* Pull-up disable       */
	SW_UDC_P_RESET	= 3,	/* UDC reset, in case of */
};

typedef struct sw_udc_mach_info {
	struct usb_port_info *port_info;
	unsigned int usbc_base;
}sw_udc_mach_info_t;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int sw_usb_device_enable(void);
int sw_usb_device_disable(void);

#endif   //__SW_UDC_H__

