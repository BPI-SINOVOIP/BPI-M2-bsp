/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hcd_core.h
*
* Author 		: javen
*
* Description 	: ��������������
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_HCD_CORE_H__
#define  __SW_HCD_CORE_H__

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>

//#include <mach/dma.h>
#include <linux/dma-mapping.h>

#include <linux/usb.h>
#include <linux/usb/hcd.h>

#include  "sw_hcd_config.h"

//---------------------------------------------------------------
//  Ԥ ����
//---------------------------------------------------------------
struct sw_hcd;
struct sw_hcd_hw_ep;

#include  "sw_hcd_regs_i.h"

#include  "sw_hcd_board.h"
#include  "sw_hcd_host.h"
#include  "sw_hcd_virt_hub.h"
#include  "sw_hcd_dma.h"




//---------------------------------------------------------------
//  �� ����
//---------------------------------------------------------------

#define is_host_active(m)		((m)->is_host)



//---------------------------------------------------------------
//  ���ݽṹ ����
//---------------------------------------------------------------

#define	is_host_capable()			(1)
#define SW_HCD_C_NUM_EPS      		USBC_MAX_EP_NUM
#define	is_host_enabled(sw_usb)		is_host_capable()

/* host side ep0 states */
enum sw_hcd_h_ep0_state {
	SW_HCD_EP0_IDLE,
	SW_HCD_EP0_START,			/* expect ack of setup */
	SW_HCD_EP0_IN,			/* expect IN DATA */
	SW_HCD_EP0_OUT,			/* expect ack of OUT DATA */
	SW_HCD_EP0_STATUS,		/* expect ack of STATUS */
}__attribute__ ((packed));


/*
 * struct sw_hcd_hw_ep - endpoint hardware (bidirectional)
 *
 * Ordered slightly for better cacheline locality.
 */
typedef struct sw_hcd_hw_ep{
	struct sw_hcd *sw_hcd;              /* ep ������                    */
	void __iomem *fifo;             /* fifo�Ļ�ַ                   */
	void __iomem *regs;             /* USB ��������ַ               */

	u8 epnum;      	                /* index in sw_hcd->endpoints[]   */

	/* hardware configuration, possibly dynamic */
	bool is_shared_fifo;            /* �Ƿ��� fifo                */
	bool tx_double_buffered;        /* Flag. �Ƿ���˫fifo?          */
	bool rx_double_buffered;        /* Flag. �Ƿ���˫fifo?          */
	u16 max_packet_sz_tx;           /* �����С                   */
	u16 max_packet_sz_rx;           /* �����С                   */

	void __iomem *target_regs;      /* hub �ϵ�Ŀ���豸�ĵ�ַ       */

	/* currently scheduled peripheral endpoint */
	struct sw_hcd_qh *in_qh;          /* ��� in ep �ĵ�����Ϣ        */
	struct sw_hcd_qh *out_qh;         /* ��� out ep �ĵ�����Ϣ       */

	u8 rx_reinit;                   /* flag. �Ƿ����³�ʼ��         */
	u8 tx_reinit;                   /* flag. �Ƿ����³�ʼ��         */
}sw_hcd_hw_ep_t;


/*
 * struct sw_hcd - Driver instance data.
 */
typedef struct sw_hcd{
    /* device lock */
	spinlock_t lock;                    /* ������       */
	irqreturn_t (*isr)(int, void *);    /*  */
	struct work_struct irq_work;        /*  */

	char driver_name[32];
	__u32 usbc_no;

/* this hub status bit is reserved by USB 2.0 and not seen by usbcore */
#define SW_HCD_PORT_STAT_RESUME	(1 << 31)
	u32 port1_status;                   /* ���� hub �Ķ˿�״̬  */
	unsigned long rh_timer;             /* root hub ��delayʱ�� */

	enum sw_hcd_h_ep0_state ep0_stage;    /* ep0 ��״̬           */

	/* bulk traffic normally dedicates endpoint hardware, and each
	 * direction has its own ring of host side endpoints.
	 * we try to progress the transfer at the head of each endpoint's
	 * queue until it completes or NAKs too much; then we try the next
	 * endpoint.
	 */
	struct sw_hcd_hw_ep *bulk_ep;

	struct list_head control;	        /* of sw_hcd_qh           */
	struct list_head in_bulk;	        /* of sw_hcd_qh           */
	struct list_head out_bulk;	        /* of sw_hcd_qh           */

    /* called with IRQs blocked; ON/nonzero implies starting a session,
	 * and waiting at least a_wait_vrise_tmout.
	 */
	void (*board_set_vbus)(struct sw_hcd *, int is_on);

	sw_hcd_dma_t sw_hcd_dma;

	struct device *controller;          /*  */
	void __iomem *ctrl_base;            /* USB ��������ַ       */
	void __iomem *mregs;                /* USB ��������ַ       */

	/* passed down from chip/board specific irq handlers */
	u8 int_usb;                         /* USB �ж�             */
	u16 int_rx;                         /* rx �ж�              */
	u16 int_tx;                         /* tx �ж�              */

	int nIrq;                           /* �жϺ�               */
	unsigned irq_wake:1;                /* flag. �ж�ʹ�ܱ�־   */

	struct sw_hcd_hw_ep endpoints[SW_HCD_C_NUM_EPS];    /* sw_hcd ���� ep ����Ϣ */
#define control_ep endpoints

#define VBUSERR_RETRY_COUNT	3
	u16 vbuserr_retry;                  /* vbus error ��host retry�Ĵ���  */
	u16 epmask;                         /* ep���룬bitn = 1, ��ʾ epn ��Ч  */
	u8 nr_endpoints;                    /* ��Ч ep �ĸ���                   */

	u8 board_mode;		                /* enum sw_hcd_mode                   */

	int (*board_set_power)(int state);
	int (*set_clock)(struct clk *clk, int is_active);

	u8 min_power;	                    /* vbus for periph, in mA/2         */

	bool is_host;                       /* flag. �Ƿ��� host �����־       */
	int a_wait_bcon;	                /* VBUS timeout in msecs            */
	unsigned long idle_timeout;	        /* Next timeout in jiffies          */

	/* active means connected and not suspended */
	unsigned is_active:1;
	unsigned is_connected:1;
	unsigned is_reset:1;
	unsigned is_suspend:1;

	unsigned is_multipoint:1;           /* flag. is multiple transaction ep? */
	unsigned ignore_disconnect:1;	    /* during bus resets                */

	unsigned bulk_split:1;
#define	can_bulk_split(sw_usb, type)       (((type) == USB_ENDPOINT_XFER_BULK) && (sw_usb)->bulk_split)

	unsigned bulk_combine:1;
#define	can_bulk_combine(sw_usb, type)     (((type) == USB_ENDPOINT_XFER_BULK) && (sw_usb)->bulk_combine)

	struct sw_hcd_config	*config;        /* sw_hcd ��������Ϣ                  */

	sw_hcd_io_t	*sw_hcd_io;
	u32 enable;
	u32 init_controller;
	u32 suspend;
       u32 session_req_flag;
       u32 reset_flag;
       u32 vbus_error_flag;
}sw_hcd_t;

struct sw_hcd_ep_reg{
	__u32 USB_CSR0;
	__u32 USB_TXCSR;
	__u32 USB_RXCSR;
	__u32 USB_COUNT0;
	__u32 USB_RXCOUNT;
	__u32 USB_ATTR0;
	__u32 USB_EPATTR;
	__u32 USB_TXFIFO;
	__u32 USB_RXFIFO;
	__u32 USB_FADDR;
	__u32 USB_TXFADDR;
	__u32 USB_RXFADDR;
};

struct sw_hcd_context_registers {
	/* FIFO Entry for Endpoints */
	__u32 USB_EPFIFO0;
	__u32 USB_EPFIFO1;
	__u32 USB_EPFIFO2;
	__u32 USB_EPFIFO3;
	__u32 USB_EPFIFO4;
	__u32 USB_EPFIFO5;

	/* Common Register */
	__u32 USB_GCS;
	__u32 USB_EPINTF;
	__u32 USB_EPINTE;
	__u32 USB_BUSINTF;
	__u32 USB_BUSINTE;
	__u32 USB_FNUM;
	__u32 USB_TESTC;

	/* Endpoint Index Register */
	struct sw_hcd_ep_reg ep_reg[SW_HCD_C_NUM_EPS];

	/* Configuration Register */
	__u32 USB_CONFIGINFO;
	__u32 USB_LINKTIM;
	__u32 USB_OTGTIM;

	/* PHY and Interface Control and Status Register */
	__u32 USB_ISCR;
	__u32 USB_PHYCTL;
	__u32 USB_PHYBIST;
};

//---------------------------------------------------------------
//
//---------------------------------------------------------------

static inline struct sw_hcd *dev_to_sw_hcd(struct device *dev)
{
	/* usbcore insists dev->driver_data is a "struct hcd *" */
	return hcd_to_sw_hcd(dev_get_drvdata(dev));
}


/* vbus ���� */
static inline void sw_hcd_set_vbus(struct sw_hcd *sw_hcd, int is_on)
{
	/* check argment */
	if (sw_hcd == NULL) {
		pr_err("ERR: invalid argment sw_hcd is null\n");
		return;
	}

	if(sw_hcd->board_set_vbus){
		sw_hcd->board_set_vbus(sw_hcd, is_on);
	}
}

/* ��ȡ fifo �Ĵ�С */
static inline int sw_hcd_read_fifosize(struct sw_hcd *sw_hcd, struct sw_hcd_hw_ep *hw_ep, u8 epnum)
{
	void *xbase = sw_hcd->mregs;
	u8 reg = 0;

	/* read from core using indexed model */
	reg = USBC_Readb(USBC_REG_TXFIFOSZ(xbase));
	/* 0's returned when no more endpoints */
	if (!reg){
	    return -ENODEV;
	}
	hw_ep->max_packet_sz_tx = 1 << (reg & 0x0f);

	sw_hcd->nr_endpoints++;
	sw_hcd->epmask |= (1 << epnum);

	/* read from core using indexed model */
	reg = USBC_Readb(USBC_REG_RXFIFOSZ(xbase));
	/* 0's returned when no more endpoints */
	if (!reg){
	    return -ENODEV;
	}
	/* shared TX/RX FIFO? */
	if ((reg & 0xf0) == 0xf0) {
		hw_ep->max_packet_sz_rx = hw_ep->max_packet_sz_tx;
		hw_ep->is_shared_fifo = true;
		return 0;
	} else {
		hw_ep->max_packet_sz_rx = 1 << ((reg & 0xf0) >> 4);
		hw_ep->is_shared_fifo = false;
	}

	return 0;
}

/* ���� ep0 */
static inline void sw_hcd_configure_ep0(struct sw_hcd *sw_hcd)
{
	sw_hcd->endpoints[0].max_packet_sz_tx = USBC_EP0_FIFOSIZE;
	sw_hcd->endpoints[0].max_packet_sz_rx = USBC_EP0_FIFOSIZE;
	sw_hcd->endpoints[0].is_shared_fifo = true;
}

#define  SW_HCD_HST_MODE(sw_hcd) 	{ (sw_hcd)->is_host = true; }
#define  is_direction_in(qh)		(qh->hep->desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK)

//---------------------------------------------------------------
//  ���� ����
//---------------------------------------------------------------
void sw_hcd_write_fifo(struct sw_hcd_hw_ep *hw_ep, u16 len, const u8 *src);
void sw_hcd_read_fifo(struct sw_hcd_hw_ep *hw_ep, u16 len, u8 *dst);
void sw_hcd_load_testpacket(struct sw_hcd *sw_hcd);
void sw_hcd_generic_disable(struct sw_hcd *sw_hcd);

irqreturn_t generic_interrupt(int irq, void *__hci);

void sw_hcd_soft_disconnect(struct sw_hcd *sw_hcd);
void sw_hcd_start(struct sw_hcd *sw_hcd);
void sw_hcd_stop(struct sw_hcd *sw_hcd);


void sw_hcd_platform_try_idle(struct sw_hcd *sw_hcd, unsigned long timeout);
void sw_hcd_platform_enable(struct sw_hcd *sw_hcd);
void sw_hcd_platform_disable(struct sw_hcd *sw_hcd);
int sw_hcd_platform_set_mode(struct sw_hcd *sw_hcd, u8 sw_hcd_mode);
int sw_hcd_platform_init(struct sw_hcd *sw_hcd);
int sw_hcd_platform_exit(struct sw_hcd *sw_hcd);
int sw_hcd_platform_suspend(struct sw_hcd *sw_hcd);
int sw_hcd_platform_resume(struct sw_hcd *sw_hcd);

#endif   //__SW_HCD_CORE_H__

