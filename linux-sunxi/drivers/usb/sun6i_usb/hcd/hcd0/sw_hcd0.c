/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hcd_core.c
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/irqs.h>
#include <mach/platform.h>

#include  <mach/clock.h>
#include <linux/gpio.h>
#include <mach/sys_config.h>

#include  "../include/sw_hcd_config.h"
#include  "../include/sw_hcd_core.h"
#include  "../include/sw_hcd_dma.h"

//---------------------------------------------------------------
//  ȫ����Ϣ ����
//---------------------------------------------------------------

#define  DRIVER_AUTHOR      "Javen"
#define  DRIVER_DESC        "sw_hcd Host Controller Driver"
#define  sw_hcd_VERSION       "1.0"

#define DRIVER_INFO DRIVER_DESC ", v" sw_hcd_VERSION

#define SW_HCD_DRIVER_NAME    "sw_hcd_host0"
static const char sw_hcd_driver_name[] = SW_HCD_DRIVER_NAME;

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" SW_HCD_DRIVER_NAME);

enum fifo_style { FIFO_RXTX = 0, FIFO_TX, FIFO_RX } __attribute__ ((packed));
enum buf_mode { BUF_SINGLE = 0, BUF_DOUBLE } __attribute__ ((packed));

struct fifo_cfg {
	u8		hw_ep_num;
	enum fifo_style	style;
	enum buf_mode	mode;
	u16		maxpacket;
};

/*
 * tables defining fifo_mode values.  define more if you like.
 * for host side, make sure both halves of ep1 are set up.
 */
static struct fifo_cfg mode_4_cfg[] = {
	{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  3, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  3, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  4, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  5, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  5, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
};

static struct fifo_cfg ep0_cfg = {
	.style = FIFO_RXTX, .maxpacket = 64,
};

static sw_hcd_io_t g_sw_hcd_io;
static __u32 usbc_no = 0;

#ifdef  CONFIG_USB_SW_SUN6I_USB0_OTG
static struct platform_device *g_hcd0_pdev = NULL;
extern atomic_t thread_suspend_flag;
#endif

static struct sw_hcd_context_registers sw_hcd_context;
static struct sw_hcd *g_sw_hcd0 = NULL;


//---------------------------------------------------------------
//  ������
//---------------------------------------------------------------

#define res_size(_r) (((_r)->end - (_r)->start) + 1)

static void sw_hcd_save_context(struct sw_hcd *sw_hcd);
static void sw_hcd_restore_context(struct sw_hcd *sw_hcd);

static s32 usb_clock_init(sw_hcd_io_t *sw_hcd_io)
{
	sw_hcd_io->ahb_otg = clk_get(NULL, CLK_AHB_OTG);
	if (IS_ERR(sw_hcd_io->ahb_otg)){
		DMSG_PANIC("ERR: get usb sie clk failed.\n");
		goto failed;
	}

	sw_hcd_io->mod_usbotg = clk_get(NULL, CLK_MOD_USBOTG);
	if (IS_ERR(sw_hcd_io->mod_usbotg)){
		DMSG_PANIC("ERR: get usb phy clk failed.\n");
		goto failed;
	}

	sw_hcd_io->mod_usbphy = clk_get(NULL, CLK_MOD_USBPHY0);
	if (IS_ERR(sw_hcd_io->mod_usbphy)){
		DMSG_PANIC("ERR: get usb phy0 clk failed.\n");
		goto failed;
	}

	return 0;

failed:
	if(sw_hcd_io->ahb_otg){
		clk_put(sw_hcd_io->ahb_otg);
		sw_hcd_io->ahb_otg = NULL;
	}

	if(sw_hcd_io->mod_usbotg){
		clk_put(sw_hcd_io->mod_usbotg);
		sw_hcd_io->mod_usbotg = NULL;
	}

	if(sw_hcd_io->mod_usbphy){
		clk_put(sw_hcd_io->mod_usbphy);
		sw_hcd_io->mod_usbphy = NULL;
	}

	return -1;
}

static s32 usb_clock_exit(sw_hcd_io_t *sw_hcd_io)
{
	if(sw_hcd_io->ahb_otg){
		clk_put(sw_hcd_io->ahb_otg);
		sw_hcd_io->ahb_otg = NULL;
	}

	if(sw_hcd_io->mod_usbotg){
		clk_put(sw_hcd_io->mod_usbotg);
		sw_hcd_io->mod_usbotg = NULL;
	}

	if(sw_hcd_io->mod_usbphy){
		clk_put(sw_hcd_io->mod_usbphy);
		sw_hcd_io->mod_usbphy = NULL;
	}

	return 0;
}

/*
*******************************************************************************
*                     open_usb_clock
*
* Description:
*
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
static s32 open_usb_clock(sw_hcd_io_t *sw_hcd_io)
{
 	DMSG_INFO_HCD0("open_usb_clock\n");

	if(sw_hcd_io->ahb_otg && sw_hcd_io->mod_usbotg && sw_hcd_io->mod_usbphy && !sw_hcd_io->clk_is_open){
	   	clk_enable(sw_hcd_io->ahb_otg);
		udelay(100);

	    //clk_enable(sw_hcd_io->mod_usbotg); /*NO SCLK_GATING_OTG */
		clk_reset(sw_hcd_io->mod_usbotg, AW_CCU_CLK_NRESET);
	    clk_enable(sw_hcd_io->mod_usbphy);
		clk_reset(sw_hcd_io->mod_usbphy, AW_CCU_CLK_NRESET);
		udelay(100);

		sw_hcd_io->clk_is_open = 1;
	}else{
		DMSG_INFO("ERR: open usb clock failed, (0x%p, 0x%p, 0x%p, %d)\n",
			      sw_hcd_io->ahb_otg, sw_hcd_io->mod_usbotg, sw_hcd_io->mod_usbphy, sw_hcd_io->clk_is_open);
	}

	UsbPhyInit(0);
/*
	DMSG_INFO("[hcd0]: open, 0x60(0x%x), 0xcc(0x%x), 0x2c0(0x%x)\n",
		      (u32)USBC_Readl(sw_hcd_io->clock_vbase + 0x60),
		      (u32)USBC_Readl(sw_hcd_io->clock_vbase + 0xcc),
		      (u32)USBC_Readl(sw_hcd_io->clock_vbase + 0x2c0));
*/
	return 0;
}

/*
*******************************************************************************
*                     close_usb_clock
*
* Description:
*
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
static s32 close_usb_clock(sw_hcd_io_t *sw_hcd_io)
{
 	DMSG_INFO_HCD0("close_usb_clock\n");

	if(sw_hcd_io->ahb_otg && sw_hcd_io->mod_usbotg && sw_hcd_io->mod_usbphy && sw_hcd_io->clk_is_open){
		clk_reset(sw_hcd_io->mod_usbphy, AW_CCU_CLK_RESET);
	    clk_disable(sw_hcd_io->mod_usbphy);

		clk_reset(sw_hcd_io->mod_usbotg, AW_CCU_CLK_RESET);
	   // clk_disable(sw_hcd_io->mod_usbotg); /*NO SCLK_GATING_OTG */
	    clk_disable(sw_hcd_io->ahb_otg);
		sw_hcd_io->clk_is_open = 0;
	}else{
		DMSG_INFO("ERR: close usb clock failed, (0x%p, 0x%p, 0x%p, %d)\n",
			      sw_hcd_io->ahb_otg, sw_hcd_io->mod_usbotg, sw_hcd_io->mod_usbphy, sw_hcd_io->clk_is_open);
	}

/*
	DMSG_INFO("[hcd0]: close, 0x60(0x%x), 0xcc(0x%x), 0x2c0(0x%x)\n",
		      (u32)USBC_Readl(sw_hcd_io->clock_vbase + 0x60),
		      (u32)USBC_Readl(sw_hcd_io->clock_vbase + 0xcc),
		      (u32)USBC_Readl(sw_hcd_io->clock_vbase + 0x2c0));
*/
	return 0;
}

/*
*******************************************************************************
*                     pin_init
*
* Description:
*    void
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
static __s32 pin_init(sw_hcd_io_t *sw_hcd_io)
{
#ifndef  SW_USB_FPGA
	__s32 ret = 0;
    script_item_value_type_e type = 0;
	script_item_u item_temp;

	/* get usb_host_init_state */
	type = script_get_item(SET_USB0, KEY_USB_HOST_INIT_STATE, &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sw_hcd_io->host_init_state = item_temp.val;
	}else{
		DMSG_PANIC("ERR: get host_init_state failed\n");
		sw_hcd_io->host_init_state = 1;
	}

	/* usbc drv_vbus */
	type = script_get_item(SET_USB0, KEY_USB_DRVVBUS_GPIO, &sw_hcd_io->drv_vbus_gpio_set);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
        sw_hcd_io->drv_vbus_valid = 1;
	}else{
	    DMSG_PANIC("ERR: get usbc0(drv vbus) failed\n");
        sw_hcd_io->drv_vbus_valid = 0;
	}

    if(sw_hcd_io->drv_vbus_valid){
    	ret = gpio_request(sw_hcd_io->drv_vbus_gpio_set.gpio.gpio, "otg_drv_vbus");
    	if(ret != 0){
    		DMSG_PANIC("ERR: gpio_request failed\n");
    		sw_hcd_io->drv_vbus_valid = 0;
    	}else{
        	/* set config, ouput */
        	//sw_gpio_setcfg(sw_hcd_io->drv_vbus_gpio_set.gpio.gpio, 1);

        	/* reserved is pull down */
        	//sw_gpio_setpull(sw_hcd_io->drv_vbus_gpio_set.gpio.gpio, 2);
			gpio_direction_output(sw_hcd_io->drv_vbus_gpio_set.gpio.gpio, 0);
    	}
	}

#else
		sw_hcd_io->host_init_state = 1;

#endif
	return 0;
}

/*
*******************************************************************************
*                     pin_exit
*
* Description:
*    void
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
static __s32 pin_exit(sw_hcd_io_t *sw_hcd_io)
{
#ifndef	SW_USB_FPGA
	if(sw_hcd_io->drv_vbus_valid){
		gpio_free(sw_hcd_io->drv_vbus_gpio_set.gpio.gpio);
		sw_hcd_io->drv_vbus_valid = 0;
	}

	if(sw_hcd_io->usb_restrict_valid){
		gpio_free(sw_hcd_io->restrict_gpio_set.gpio.gpio);
		sw_hcd_io->usb_restrict_valid = 0;
	}
#endif
	return 0;
}

/*
*******************************************************************************
*                     sw_hcd_board_set_vbus
*
* Description:
*    void
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
static void sw_hcd_board_set_vbus(struct sw_hcd *sw_hcd, int is_on)
{
#ifndef  SW_USB_FPGA
	u32 on_off = 0;
#endif

	DMSG_INFO("[%s]: Set USB Power %s\n", sw_hcd->driver_name, (is_on ? "ON" : "OFF"));

#ifndef  SW_USB_FPGA
    /* set power */
    if(sw_hcd->sw_hcd_io->drv_vbus_gpio_set.gpio.data == 0){
        on_off = is_on ? 1 : 0;
    }else{
        on_off = is_on ? 0 : 1;
    }

	/* set gpio data */
	if(sw_hcd->sw_hcd_io->drv_vbus_valid){
		__gpio_set_value(sw_hcd->sw_hcd_io->drv_vbus_gpio_set.gpio.gpio, on_off);
	}
#endif

	if(is_on){
		USBC_Host_StartSession(sw_hcd->sw_hcd_io->usb_bsp_hdle);
		USBC_ForceVbusValid(sw_hcd->sw_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);
	}else{
		USBC_Host_EndSession(sw_hcd->sw_hcd_io->usb_bsp_hdle);
		USBC_ForceVbusValid(sw_hcd->sw_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_DISABLE);
	}

	return;
}

/*
*******************************************************************************
*                     sw_hcd_bsp_init
*
* Description:
*    initialize usb bsp
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
static __s32 sw_hcd_bsp_init(__u32 usbc_no, sw_hcd_io_t *sw_hcd_io)
{
    memset(&sw_hcd_io->usbc, 0, sizeof(bsp_usbc_t));

	sw_hcd_io->usbc.usbc_info[usbc_no].num  = usbc_no;
	sw_hcd_io->usbc.usbc_info[usbc_no].base = (u32)sw_hcd_io->usb_vbase;
	sw_hcd_io->usbc.sram_base = (u32)sw_hcd_io->sram_vbase;

//	USBC_init(&sw_hcd_io->usbc);
	sw_hcd_io->usb_bsp_hdle = USBC_open_otg(usbc_no);
	if(sw_hcd_io->usb_bsp_hdle == 0){
		DMSG_PANIC("ERR: sw_hcd_init: USBC_open_otg failed\n");
		return -1;
	}

#ifdef  SW_USB_FPGA
	clear_usb_reg((u32)sw_hcd_io->usb_vbase);
	fpga_config_use_otg((u32)sw_hcd_io->sram_vbase);
#endif
    return 0;
}

/*
*******************************************************************************
*                     sw_hcd_bsp_exit
*
* Description:
*    void
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
static __s32 sw_hcd_bsp_exit(__u32 usbc_no, sw_hcd_io_t *sw_hcd_io)
{
	USBC_close_otg(sw_hcd_io->usb_bsp_hdle);
	sw_hcd_io->usb_bsp_hdle = 0;

//	USBC_exit(&sw_hcd_io->usbc);

    return 0;
}

/*
*******************************************************************************
*                     sw_hcd_io_init
*
* Description:
*    void
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
static __s32 sw_hcd_io_init(__u32 usbc_no, struct platform_device *pdev, sw_hcd_io_t *sw_hcd_io)
{
	__s32 ret = 0;
	spinlock_t lock;
	unsigned long flags = 0;

	sw_hcd_io->usb_vbase  = (void __iomem *)AW_VIR_USB_OTG_BASE;
	sw_hcd_io->sram_vbase = (void __iomem *)AW_VIR_SRAMCTRL_BASE;

//	DMSG_INFO_HCD0("[usb host]: usb_vbase    = 0x%x\n", (u32)sw_hcd_io->usb_vbase);
//	DMSG_INFO_HCD0("[usb host]: sram_vbase   = 0x%x\n", (u32)sw_hcd_io->sram_vbase);

    /* open usb lock */
	ret = usb_clock_init(sw_hcd_io);
	if(ret != 0){
		DMSG_PANIC("ERR: usb_clock_init failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	open_usb_clock(sw_hcd_io);

    /* initialize usb bsp */
	sw_hcd_bsp_init(usbc_no, sw_hcd_io);

	/* config usb fifo */
	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);
	USBC_ConfigFIFO_Base(sw_hcd_io->usb_bsp_hdle, (u32)sw_hcd_io->sram_vbase, USBC_FIFO_MODE_8K);
	spin_unlock_irqrestore(&lock, flags);

	/* config drv_vbus pin */
	ret = pin_init(sw_hcd_io);
	if(ret != 0){
		DMSG_PANIC("ERR: pin_init failed\n");
		ret = -ENOMEM;
		goto io_failed1;
	}

	DMSG_INFO("[sw_hcd0]: host_init_state = %d\n", sw_hcd_io->host_init_state);

	return 0;

io_failed1:
	sw_hcd_bsp_exit(usbc_no, sw_hcd_io);
	close_usb_clock(sw_hcd_io);
	usb_clock_exit(sw_hcd_io);

io_failed:
	sw_hcd_io->usb_vbase = 0;
	sw_hcd_io->sram_vbase = 0;

	return ret;
}

/*
*******************************************************************************
*                     sw_hcd_exit
*
* Description:
*    void
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
static __s32 sw_hcd_io_exit(__u32 usbc_no, struct platform_device *pdev, sw_hcd_io_t *sw_hcd_io)
{
	sw_hcd_bsp_exit(usbc_no, sw_hcd_io);

	/* config drv_vbus pin */
	pin_exit(sw_hcd_io);

	close_usb_clock(sw_hcd_io);

	usb_clock_exit(sw_hcd_io);

	sw_hcd_io->usb_vbase = 0;
	sw_hcd_io->sram_vbase = 0;

	return 0;
}

/*
*******************************************************************************
*                     sw_hcd_shutdown
*
* Description:
*    void
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
static void sw_hcd_shutdown(struct platform_device *pdev)
{
    struct sw_hcd 	*sw_hcd = NULL;
    unsigned long   flags = 0;

    if(pdev == NULL){
        DMSG_INFO("err: Invalid argment\n");
    	return ;
    }

    sw_hcd = dev_to_sw_hcd(&pdev->dev);
    if(sw_hcd == NULL){
        DMSG_INFO("err: sw_hcd is null\n");
    	return ;
    }

    if(!sw_hcd->enable){
    	DMSG_INFO("wrn: hcd is disable, need not shutdown\n");
    	return ;
    }

    DMSG_INFO_HCD0("sw_hcd shutdown start\n");

    spin_lock_irqsave(&sw_hcd->lock, flags);
    sw_hcd_platform_disable(sw_hcd);
    sw_hcd_generic_disable(sw_hcd);
    spin_unlock_irqrestore(&sw_hcd->lock, flags);

    sw_hcd_port_suspend_ex(sw_hcd);
    sw_hcd_set_vbus(sw_hcd, 0);
    close_usb_clock(&g_sw_hcd_io);

    DMSG_INFO_HCD0("Set aside some time to AXP\n");

    /* Set aside some time to AXP */
    msleep(200);

    DMSG_INFO_HCD0("sw_hcd shutdown end\n");

    return;
}

/*
*******************************************************************************
*                     fifo_setup
*
* Description:
*    configure a fifo; for non-shared endpoints, this may be called
* once for a tx fifo and once for an rx fifo.
*
* Parameters:
*    void
*
* Return value:
*    returns negative errno or offset for next fifo.
*
* note:
*    void
*
*******************************************************************************
*/
static int fifo_setup(struct sw_hcd *sw_hcd,
                     struct sw_hcd_hw_ep *hw_ep,
                     const struct fifo_cfg *cfg,
                     u16 offset)
{
	void __iomem    *usbc_base  	= NULL;
	u16             maxpacket   	= 0;
    u32 			ep_fifo_size 	= 0;
    u16				old_ep_index 	= 0;
	u32             is_double_fifo 	= 0;

    /* check argment */
    if(sw_hcd == NULL || hw_ep == NULL || cfg == NULL){
        DMSG_PANIC("ERR: invalid argment\n");
	    return -1;
    }

    /* initialize parameter */
    usbc_base = sw_hcd->mregs;
    maxpacket = cfg->maxpacket;

	/* expect hw_ep has already been zero-initialized */
	if (cfg->mode == BUF_DOUBLE) {
		ep_fifo_size = offset + (maxpacket << 1);
		is_double_fifo = 1;
	} else {
		ep_fifo_size = offset + maxpacket;
	}

	if (ep_fifo_size > sw_hcd->config->ram_size){
	    DMSG_PANIC("ERR: fifo_setup, free is not enough, ep_fifo_size = %d, ram_size = %d\n",
			       ep_fifo_size, sw_hcd->config->ram_size);
		return -EMSGSIZE;
	}

	DMSG_DBG_HCD("hw_ep->epnum   = 0x%x\n", hw_ep->epnum);
	DMSG_DBG_HCD("is_double_fifo = 0x%x\n", is_double_fifo);
	DMSG_DBG_HCD("ep_fifo_size   = 0x%x\n", ep_fifo_size);
	DMSG_DBG_HCD("hw_ep->fifo    = 0x%x\n", (u32)hw_ep->fifo);
	DMSG_DBG_HCD("maxpacket      = 0x%x\n", maxpacket);

	/* configure the FIFO */
	old_ep_index = USBC_GetActiveEp(sw_hcd->sw_hcd_io->usb_bsp_hdle);
	USBC_SelectActiveEp(sw_hcd->sw_hcd_io->usb_bsp_hdle, hw_ep->epnum);

    /* EP0 reserved endpoint for control, bidirectional;
	 * EP1 reserved for bulk, two unidirection halves.
	 */
	if (hw_ep->epnum == 1){
		sw_hcd->bulk_ep = hw_ep;
	}

	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
	switch (cfg->style) {
	    case FIFO_TX:
	    {
			USBC_ConfigFifo(sw_hcd->sw_hcd_io->usb_bsp_hdle,
				            USBC_EP_TYPE_TX,
				            is_double_fifo,
				            maxpacket,
				            offset);

    		hw_ep->tx_double_buffered = is_double_fifo;
    		hw_ep->max_packet_sz_tx = maxpacket;
	    }
		break;

	    case FIFO_RX:
	    {
			USBC_ConfigFifo(sw_hcd->sw_hcd_io->usb_bsp_hdle,
				            USBC_EP_TYPE_RX,
				            is_double_fifo,
				            maxpacket,
				            offset);

    		hw_ep->rx_double_buffered = is_double_fifo;
    		hw_ep->max_packet_sz_rx = maxpacket;
	    }
		break;

	    case FIFO_RXTX:
	    {
			if(hw_ep->epnum == 0){
				USBC_ConfigFifo(sw_hcd->sw_hcd_io->usb_bsp_hdle,
					            USBC_EP_TYPE_EP0,
					            is_double_fifo,
					            maxpacket,
					            offset);

		   		hw_ep->tx_double_buffered = 0;
				hw_ep->rx_double_buffered = 0;

		    	hw_ep->max_packet_sz_tx = maxpacket;
				hw_ep->max_packet_sz_rx = maxpacket;
			}else{
				DMSG_PANIC("ERR: fifo_setup, FIFO_RXTX not support\n");
			}

    		hw_ep->is_shared_fifo = true;
	    }
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */
	sw_hcd->epmask |= (1 << hw_ep->epnum);

	return ep_fifo_size;
}

/*
*******************************************************************************
*                     ep_config_from_table
*
* Description:
*    void
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
static int ep_config_from_table(struct sw_hcd *sw_hcd)
{
	const struct fifo_cfg	*cfg    = NULL;
	unsigned                i       = 0;
	unsigned                n       = 0;
	int                     offset  = 0;
	struct sw_hcd_hw_ep    	*hw_ep  = sw_hcd->endpoints;

    cfg = mode_4_cfg;
    n = ARRAY_SIZE(mode_4_cfg);

    /* assert(offset > 0) */
    offset = fifo_setup(sw_hcd, hw_ep, &ep0_cfg, 0);

	for (i = 0; i < n; i++) {
		u8 epn = cfg->hw_ep_num;

		DMSG_DBG_HCD("i=%d, cfg->hw_ep_num = 0x%x\n", i, cfg->hw_ep_num);

		if (epn >= sw_hcd->config->num_eps) {
			DMSG_PANIC("ERR: %s: invalid ep%d, max ep is ep%d\n",
				       sw_hcd_driver_name, epn, sw_hcd->config->num_eps);
			return -EINVAL;
		}

		offset = fifo_setup(sw_hcd, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			DMSG_PANIC("ERR: %s: mem overrun, ep %d\n", sw_hcd_driver_name, epn);
			return -EINVAL;
		}

		epn++;
		sw_hcd->nr_endpoints = max(epn, sw_hcd->nr_endpoints);
	}

	DMSG_DBG_HCD("ep_config_from_table: %s: %d/%d max ep, %d/%d memory\n",
                 sw_hcd_driver_name,
			     n + 1, sw_hcd->config->num_eps * 2 - 1,
			     offset, sw_hcd->config->ram_size);

	if (!sw_hcd->bulk_ep) {
		DMSG_PANIC("ERR: %s: missing bulk\n", sw_hcd_driver_name);
		return -EINVAL;
	}

	return 0;
}

/*
*******************************************************************************
*                     ep_config_from_hw
*
* Description:
*    ep_config_from_hw - when sw_hcd_C_DYNFIFO_DEF is false
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
static int ep_config_from_hw(struct sw_hcd *sw_hcd)
{
	u8                  epnum       = 0;
	struct sw_hcd_hw_ep   *hw_ep      = NULL;
	void                *usbc_base  = sw_hcd->mregs;
	int                 ret         = 0;

	/* FIXME pick up ep0 maxpacket size */

	for (epnum = 1; epnum < sw_hcd->config->num_eps; epnum++) {
		sw_hcd_ep_select(usbc_base, epnum);
		hw_ep = sw_hcd->endpoints + epnum;

		ret = sw_hcd_read_fifosize(sw_hcd, hw_ep, epnum);
		if (ret < 0){
			break;
		}

		/* FIXME set up hw_ep->{rx,tx}_double_buffered */

		/* pick an RX/TX endpoint for bulk */
		if (hw_ep->max_packet_sz_tx < 512
				|| hw_ep->max_packet_sz_rx < 512)
			continue;

		/* REVISIT:  this algorithm is lazy, we should at least
		 * try to pick a double buffered endpoint.
		 */
		if (sw_hcd->bulk_ep)
			continue;
		sw_hcd->bulk_ep = hw_ep;
	}

	if (!sw_hcd->bulk_ep) {
		DMSG_PANIC("ERR: %s: missing bulk\n", sw_hcd_driver_name);
		return -EINVAL;
	}

	return 0;
}

enum { SW_HCD_CONTROLLER_MHDRC, SW_HCD_CONTROLLER_HDRC, };

/*
*******************************************************************************
*                     sw_hcd_core_init
*
* Description:
*    Initialize USB hardware subsystem;
* configure endpoints, or take their config from silicon
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
static int sw_hcd_core_init(u16 sw_hcd_type, struct sw_hcd *sw_hcd)
{
	u8              reg         = 0;
	char            *type       = NULL;
	void __iomem    *usbc_base  = sw_hcd->mregs;
	int             status      = 0;
	int             i           = 0;

	/* log core options (read using indexed model) */
	sw_hcd_ep_select(usbc_base, 0);
	reg = sw_hcd_read_configdata(usbc_base);

	if (SW_HCD_CONTROLLER_MHDRC == sw_hcd_type) {
		sw_hcd->is_multipoint = 1;
		type = "M";
	} else {
	    sw_hcd->is_multipoint = 0;
		type = "";

		DMSG_INFO_HCD0("%s: kernel must blacklist external hubs\n", sw_hcd_driver_name);
	}

	/* configure ep0 */
	sw_hcd_configure_ep0(sw_hcd);

	/* discover endpoint configuration */
	sw_hcd->nr_endpoints = 1;
	sw_hcd->epmask = 1;

	if (reg & (1 << USBC_BP_CONFIGDATA_DYNFIFO_SIZING)) {
		if(sw_hcd->config->dyn_fifo){
			status = ep_config_from_table(sw_hcd);
		}else{
			DMSG_PANIC("ERR: reconfigure software for Dynamic FIFOs\n");
			status = -ENODEV;
		}
	} else {
		if(!sw_hcd->config->dyn_fifo){
			status = ep_config_from_hw(sw_hcd);
		}else{
			DMSG_PANIC("ERR: reconfigure software for static FIFOs\n");
			return -ENODEV;
		}
	}

	if (status < 0){
	    DMSG_PANIC("ERR: sw_hcd_core_init, config failed\n");
		return status;
    }

	/* finish init, and print endpoint config */
	for (i = 0; i < sw_hcd->nr_endpoints; i++) {
		struct sw_hcd_hw_ep *hw_ep = sw_hcd->endpoints + i;

        hw_ep->fifo         = (void __iomem *)USBC_REG_EPFIFOx(usbc_base, i);
        hw_ep->regs         = usbc_base;
        hw_ep->target_regs  = sw_hcd_read_target_reg_base(i, usbc_base);
		hw_ep->rx_reinit    = 1;
		hw_ep->tx_reinit    = 1;

/*
		if (hw_ep->max_packet_sz_tx) {
			DMSG_INFO_HCD0("%s: hw_ep %d%s, %smax %d\n",
        				sw_hcd_driver_name, i,
        				(hw_ep->is_shared_fifo ? "shared" : "tx"),
        				(hw_ep->tx_double_buffered ? "doublebuffer, " : ""),
        				hw_ep->max_packet_sz_tx);
		}

        if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {
			DMSG_INFO_HCD0("%s: hw_ep %d%s, %smax %d\n",
        				sw_hcd_driver_name, i,
        				"rx",
        				(hw_ep->rx_double_buffered ? "doublebuffer, " : ""),
        				hw_ep->max_packet_sz_rx);
		}

        if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx)){
			DMSG_INFO_HCD0("hw_ep %d not configured\n", i);
        }
*/
    }

    return 0;
}

/*
*******************************************************************************
*                     sw_hcd_irq_work
*
* Description:
*    Only used to provide driver mode change events
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
static void sw_hcd_irq_work(struct work_struct *data)
{
	struct sw_hcd *sw_hcd = container_of(data, struct sw_hcd, irq_work);
	void __iomem *usbc_base = sw_hcd->mregs;
	int reg_val = 0;

	if( sw_hcd == NULL || !sw_hcd->enable){
		DMSG_PANIC("ERR: sw_hcd_irq_work: sw_hcd is null\n");
		return;
	}

	if(sw_hcd->vbus_error_flag){
		sw_hcd->vbus_error_flag = 0;
		/* power down */
		sw_hcd_set_vbus(sw_hcd, 0);

		USBC_INT_EnableUsbMiscUint(sw_hcd->sw_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_VBUS_ERROR));
	}

	if(sw_hcd->session_req_flag){
		sw_hcd->session_req_flag = 0;
		/* power down */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val &= ~(1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sw_hcd->sw_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_LOW);

		sw_hcd_set_vbus(sw_hcd, 0);

		/* delay */
		msleep(100);

		/* power on */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val |= (1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sw_hcd->sw_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

		sw_hcd_set_vbus(sw_hcd, 1);

		USBC_INT_EnableUsbMiscUint(sw_hcd->sw_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSBE_EN_SESSION_REQ));
	}

	if(sw_hcd->reset_flag){
		sw_hcd->reset_flag = 0;
		/* power down */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val &= ~(1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sw_hcd->sw_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_LOW);

		sw_hcd_set_vbus(sw_hcd, 0);

		/* delay */
		msleep(100);

		/* power on */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val |= (1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sw_hcd->sw_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

		sw_hcd_set_vbus(sw_hcd, 1);

		/* disconnect */
		sw_hcd->ep0_stage = SW_HCD_EP0_START;
		usb_hcd_resume_root_hub(sw_hcd_to_hcd(sw_hcd));
		sw_hcd_root_disconnect(sw_hcd);

		USBC_INT_EnableUsbMiscUint(sw_hcd->sw_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_RESET));

		sw_hcd->is_connected = 0;
	}

	sysfs_notify(&sw_hcd->controller->kobj, NULL, "mode");

	return;
}

static const struct hc_driver sw_hcd_hc_driver = {
	.description		= "sw_hcd-hcd",
	.product_desc		= "sw_hcd host driver",
	.hcd_priv_size		= sizeof(struct sw_hcd),
//	.flags              = HCD_USB2 | HCD_MEMORY,
	.flags				= HCD_USB11 | HCD_MEMORY,

	/* not using irq handler or reset hooks from usbcore, since
	 * those must be shared with peripheral code for OTG configs
	 */

	.start              = sw_hcd_h_start,
	.stop               = sw_hcd_h_stop,

	.get_frame_number	= sw_hcd_h_get_frame_number,

	.urb_enqueue		= sw_hcd_urb_enqueue,
	.urb_dequeue		= sw_hcd_urb_dequeue,
	.endpoint_disable	= sw_hcd_h_disable,

	.hub_status_data	= sw_hcd_hub_status_data,
	.hub_control		= sw_hcd_hub_control,
	.bus_suspend		= sw_hcd_bus_suspend,
	.bus_resume		    = sw_hcd_bus_resume,
};

/*
*******************************************************************************
*                     allocate_instance
*
* Description:
*    Init struct sw_hcd
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
static struct sw_hcd *allocate_instance(struct device *dev,
                                             struct sw_hcd_config *config,
                                             void __iomem *mbase)
{
	struct sw_hcd         *sw_hcd = NULL;
	struct sw_hcd_hw_ep   *ep = NULL;
	int                 epnum = 0;
	struct usb_hcd      *hcd = NULL;

	hcd = usb_create_hcd(&sw_hcd_hc_driver, dev, dev_name(dev));
	if (!hcd){
	    DMSG_PANIC("ERR: usb_create_hcd failed\n");
		return NULL;
	}

	/* usbcore sets dev->driver_data to hcd, and sometimes uses that... */
	sw_hcd = hcd_to_sw_hcd(hcd);
	if(sw_hcd == NULL){
		DMSG_PANIC("ERR: hcd_to_sw_hcd failed\n");
		return NULL;
	}

	memset(sw_hcd, 0, sizeof(struct sw_hcd));

	INIT_LIST_HEAD(&sw_hcd->control);
	INIT_LIST_HEAD(&sw_hcd->in_bulk);
	INIT_LIST_HEAD(&sw_hcd->out_bulk);

    hcd->uses_new_polling   = 1;
	sw_hcd->vbuserr_retry     = VBUSERR_RETRY_COUNT;
    sw_hcd->mregs             = mbase;
	sw_hcd->ctrl_base         = mbase;
	sw_hcd->nIrq              = -ENODEV;
	sw_hcd->config            = config;
	sw_hcd->ignore_disconnect = false;

#ifndef  CONFIG_USB_SW_SUN6I_USB0_OTG
	g_sw_hcd0 = sw_hcd;
	sw_hcd->enable = 1;
#else
    if(sw_hcd->config->port_info->port_type == USB_PORT_TYPE_HOST){
        g_sw_hcd0 = sw_hcd;
    	sw_hcd->enable = 1;
    }
#endif

	strcpy(sw_hcd->driver_name, sw_hcd_driver_name);

	for (epnum = 0, ep = sw_hcd->endpoints;
			epnum < sw_hcd->config->num_eps;
			epnum++, ep++) {
		ep->sw_hcd = sw_hcd;
		ep->epnum = epnum;
	}

	sw_hcd->controller = dev;
	sw_hcd->sw_hcd_io    = &g_sw_hcd_io;
	sw_hcd->usbc_no	 = usbc_no;

	return sw_hcd;
}

/*
*******************************************************************************
*                     sw_hcd_free
*
* Description:
*    void
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
static void sw_hcd_free(struct sw_hcd *sw_hcd)
{
    void __iomem    *usbc_base  = sw_hcd->mregs;

	/* this has multiple entry modes. it handles fault cleanup after
	 * probe(), where things may be partially set up, as well as rmmod
	 * cleanup after everything's been de-activated.
	 */
    if (sw_hcd->nIrq >= 0) {
		if (sw_hcd->irq_wake) {
			disable_irq_wake(sw_hcd->nIrq);
		}

		free_irq(sw_hcd->nIrq, sw_hcd);
	}

	if (is_hcd_support_dma(sw_hcd->usbc_no)) {
		sw_hcd_dma_remove(sw_hcd);
	}

	USBC_Writeb(0x00, USBC_REG_DEVCTL(usbc_base));
	sw_hcd_platform_exit(sw_hcd);
	USBC_Writeb(0x00, USBC_REG_DEVCTL(usbc_base));

    usb_put_hcd(sw_hcd_to_hcd(sw_hcd));

    return;
}

/*
*******************************************************************************
*                     hcd0_generic_interrupt
*
* Description:
*    Perform generic per-controller initialization.
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
static irqreturn_t hcd0_generic_interrupt(int irq, void *__hci)
{
	DMSG_DBG_HCD("irq: %s\n", sw_hcd_driver_name);

	return generic_interrupt(irq, __hci);
}

/*
*******************************************************************************
*                     sw_hcd_init_controller
*
* Description:
*    Perform generic per-controller initialization.
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
static int sw_hcd_init_controller(struct device *dev, int nIrq, void __iomem *ctrl)
{
	int                        	status  = 0;
	struct sw_hcd              	*sw_hcd	= 0;
	struct sw_hcd_platform_data	*plat   = dev->platform_data;

	/* The driver might handle more features than the board; OK.
	 * Fail when the board needs a feature that's not enabled.
	 */
	if (!plat) {
		DMSG_PANIC("ERR: no platform_data?\n");
		return -ENODEV;
	}

    switch (plat->mode) {
	    case SW_HCD_HOST:
            DMSG_INFO_HCD0("platform is usb host\n");
		break;

    	default:
    		DMSG_PANIC("ERR: unkown platform mode(%d)\n", plat->mode);
    		return -EINVAL;
	}

	/* allocate */
	sw_hcd = allocate_instance(dev, plat->config, ctrl);
	if (!sw_hcd){
	    DMSG_PANIC("ERR: allocate_instance failed\n");
		return -ENOMEM;
	}

	spin_lock_init(&sw_hcd->lock);
	sw_hcd->board_mode        = plat->mode;
	sw_hcd->board_set_power   = plat->set_power;
	sw_hcd->set_clock         = plat->set_clock;
	sw_hcd->min_power         = plat->min_power;
	sw_hcd->board_set_vbus    = sw_hcd_board_set_vbus;
	sw_hcd->init_controller = 1;

	/* assume vbus is off */

	/* platform adjusts sw_hcd->mregs and sw_hcd->isr if needed,
	 * and activates clocks
	 */
	sw_hcd->isr = hcd0_generic_interrupt;
	status = sw_hcd_platform_init(sw_hcd);
	if (status < 0){
	    DMSG_PANIC("ERR: sw_hcd_platform_init failed\n");
		goto fail;
	}

	if (!sw_hcd->isr) {
	    DMSG_PANIC("ERR: sw_hcd->isr is null\n");
		status = -ENODEV;
		goto fail2;
	}

    if (is_hcd_support_dma(sw_hcd->usbc_no)) {
		status = sw_hcd_dma_probe(sw_hcd);
		if (status < 0){
		    DMSG_PANIC("ERR: sw_hcd_dma_probe failed\n");
			goto fail2;
		}
	}

	/* be sure interrupts are disabled before connecting ISR */
	sw_hcd_platform_disable(sw_hcd);
	sw_hcd_generic_disable(sw_hcd);

	/* setup sw_hcd parts of the core (especially endpoints) */
	status = sw_hcd_core_init(plat->config->multipoint ? SW_HCD_CONTROLLER_MHDRC : SW_HCD_CONTROLLER_HDRC, sw_hcd);
	if (status < 0){
	    DMSG_PANIC("ERR: sw_hcd_core_init failed\n");
		goto fail2;
	}

	/* Init IRQ workqueue before request_irq */
	INIT_WORK(&sw_hcd->irq_work, sw_hcd_irq_work);

	/* attach to the IRQ */
	if (request_irq(nIrq, sw_hcd->isr, 0, dev_name(dev), sw_hcd)) {
		DMSG_PANIC("ERR: request_irq %d failed!\n", nIrq);
		status = -ENODEV;
		goto fail2;
	}

	sw_hcd->nIrq = nIrq;

    /* FIXME this handles wakeup irqs wrong */
	if (enable_irq_wake(nIrq) == 0) {
		sw_hcd->irq_wake = 1;
		device_init_wakeup(dev, 1);
	} else {
		sw_hcd->irq_wake = 0;
	}

	DMSG_INFO_HCD0("sw_hcd_init_controller: %s: USB %s mode controller at %p using %s, IRQ %d\n",
        			sw_hcd_driver_name,
        			"Host",
        			ctrl,
        			is_hcd_support_dma(sw_hcd->usbc_no) ? "DMA" : "PIO",
        			sw_hcd->nIrq);

	/* host side needs more setup, except for no-host modes */
	if (sw_hcd->board_mode != SW_HCD_PERIPHERAL) {
		struct usb_hcd	*hcd = sw_hcd_to_hcd(sw_hcd);

		hcd->power_budget = 2 * (plat->power ? : 250);
	}

	/* For the host-only role, we can activate right away.
	 * (We expect the ID pin to be forcibly grounded!!)
	 * Otherwise, wait till the gadget driver hooks up.
	 */
	if (is_host_enabled(sw_hcd)) {
		SW_HCD_HST_MODE(sw_hcd);
		sw_hcd->enable = 1;
		status = usb_add_hcd(sw_hcd_to_hcd(sw_hcd), -1, 0);
		if (status){
		    DMSG_PANIC("ERR: usb_add_hcd failed\n");
		    goto fail;
		}
    }

	sw_hcd->init_controller = 0;

    return 0;

fail2:
	if(sw_hcd->sw_hcd_dma.dma_hdle < 0){
		sw_hcd_dma_remove(sw_hcd);
	}

	sw_hcd_platform_exit(sw_hcd);

fail:
	DMSG_PANIC("ERR: sw_hcd_init_controller failed with status %d\n", status);

	device_init_wakeup(dev, 0);
	sw_hcd_free(sw_hcd);

	sw_hcd->init_controller = 0;

	return status;
}

/*
*******************************************************************************
*                     sw_usb_host0_enable
*
* Description:
*    void
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
int sw_usb_host0_enable(void)
{
#if CONFIG_USB_SW_SUN6I_USB0_OTG
	struct platform_device 	*pdev 	= NULL;
	struct device   		*dev  	= NULL;
	struct sw_hcd 			*sw_hcd	= NULL;
	unsigned long   		flags 	= 0;

	DMSG_INFO_HCD0("sw_usb_host0_enable start\n");

	pdev = g_hcd0_pdev;
	if(pdev == NULL){
		DMSG_PANIC("ERR: pdev is null\n");
		return -1;
	}

	dev = &pdev->dev;
	if(dev == NULL){
		DMSG_PANIC("ERR: dev is null\n");
		return -1;
	}

	sw_hcd = dev_to_sw_hcd(&pdev->dev);
	if(sw_hcd == NULL){
		DMSG_PANIC("ERR: sw_hcd is null\n");
		return -1;
	}

	g_sw_hcd0 = sw_hcd;

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd->enable       = 1;
	sw_hcd->is_connected = 0;
	sw_hcd->is_reset     = 0;
	sw_hcd->is_suspend   = 0;
	spin_unlock_irqrestore(&sw_hcd->lock, flags);

	/* request usb irq */
	INIT_WORK(&sw_hcd->irq_work, sw_hcd_irq_work);

	if (request_irq(sw_hcd->nIrq, sw_hcd->isr, 0, dev_name(dev), sw_hcd)) {
		DMSG_PANIC("ERR: request_irq %d failed!\n", sw_hcd->nIrq);
		return -1;
	}

	sw_hcd_soft_disconnect(sw_hcd);
	sw_hcd_io_init(usbc_no, pdev, &g_sw_hcd_io);

	/* enable usb controller */
	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd_platform_init(sw_hcd);
	sw_hcd_restore_context(sw_hcd);
	sw_hcd_start(sw_hcd);
	spin_unlock_irqrestore(&sw_hcd->lock, flags);
    /* port power on */
	sw_hcd_set_vbus(sw_hcd, 1);

	DMSG_INFO_HCD0("sw_usb_host0_enable end\n");
#endif
    return 0;
}
EXPORT_SYMBOL(sw_usb_host0_enable);

static void sw_hcd_wait_for_disconnect(struct sw_hcd *sw_hcd)
{
	int cnt = 0;

	while(cnt < 500){
		/*
		 * 1. disconnect
		 * 2. not reset
		 * 3. not suspend
		 */
		if (!sw_hcd->is_connected && !sw_hcd->is_reset && !sw_hcd->is_suspend){
			break;
		}

		cnt++;
		msleep(1);
	}

	DMSG_INFO_HCD0("sw_hcd_wait_for_disconnect cnt=%d\n", cnt);
}

/*
*******************************************************************************
*                     sw_usb_host0_disable
*
* Description:
*    void
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
int sw_usb_host0_disable(void)
{
#if CONFIG_USB_SW_SUN6I_USB0_OTG
	struct platform_device 	*pdev 	= NULL;
	struct sw_hcd 			*sw_hcd	= NULL;
	unsigned long   		flags 	= 0;

	DMSG_INFO_HCD0("sw_usb_host0_disable start\n");

	pdev = g_hcd0_pdev;
	if(pdev == NULL){
		DMSG_PANIC("ERR: pdev is null\n");
		return -1;
	}

	sw_hcd = dev_to_sw_hcd(&pdev->dev);
	if(sw_hcd == NULL){
		DMSG_PANIC("ERR: sw_hcd is null\n");
		return -1;
	}

	if(sw_hcd->suspend){
	    DMSG_PANIC("wrn: sw_hcd is suspend, can not disable\n");
		return -EBUSY;
	}

	sw_hcd_wait_for_disconnect(sw_hcd);

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd->enable = 0;
	spin_unlock_irqrestore(&sw_hcd->lock, flags);
	sw_hcd_set_vbus(sw_hcd, 0);

	/* nuke all urb and disconnect */
	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd_soft_disconnect(sw_hcd);
	//sw_hcd_port_suspend_ex(sw_hcd);
	sw_hcd_stop(sw_hcd);
	spin_unlock_irqrestore(&sw_hcd->lock, flags);

	/* release usb irq */
	if (sw_hcd->nIrq >= 0) {
		if (sw_hcd->irq_wake) {
			disable_irq_wake(sw_hcd->nIrq);
		}

		free_irq(sw_hcd->nIrq, sw_hcd);
	}

	/* disable usb controller */
	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd_save_context(sw_hcd);
	sw_hcd_platform_exit(sw_hcd);
	spin_unlock_irqrestore(&sw_hcd->lock, flags);

	sw_hcd_io_exit(usbc_no, pdev, &g_sw_hcd_io);

	spin_lock_irqsave(&sw_hcd->lock, flags);
	//sw_hcd->enable = 0;
	g_sw_hcd0 = NULL;
	spin_unlock_irqrestore(&sw_hcd->lock, flags);

	DMSG_INFO_HCD0("sw_usb_host0_disable end\n");
#endif
	return 0;
}
EXPORT_SYMBOL(sw_usb_host0_disable);

/*
*******************************************************************************
*                     sw_hcd_probe_otg
*
* Description:
*    all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
* bridge to a platform device; this driver then suffices.
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
static int sw_hcd_probe_otg(struct platform_device *pdev)
{
	struct device   *dev    = &pdev->dev;
	int             irq     = AW_IRQ_USB_OTG; //platform_get_irq(pdev, 0);
	__s32 			ret 	= 0;
	__s32 			status	= 0;

	if (irq == 0){
	    DMSG_PANIC("ERR: platform_get_irq failed\n");
		return -ENODEV;
	}

	g_hcd0_pdev = pdev;
	usbc_no = 0;

    memset(&g_sw_hcd_io, 0, sizeof(sw_hcd_io_t));
	ret = sw_hcd_io_init(usbc_no, pdev, &g_sw_hcd_io);
	if(ret != 0){
		DMSG_PANIC("ERR: sw_hcd_io_init failed\n");
		status = -ENODEV;
		goto end;
	}

	ret = sw_hcd_init_controller(dev, irq, g_sw_hcd_io.usb_vbase);
	if(ret != 0){
		DMSG_PANIC("ERR: sw_hcd_init_controller failed\n");
		status = -ENODEV;
		goto end;
	}

	ret = sw_usb_host0_disable();
	if(ret != 0){
		DMSG_PANIC("ERR: sw_usb_host0_disable failed\n");
		status = -ENODEV;
		goto end;
	}

end:
    return status;
}

/*
*******************************************************************************
*                     sw_hcd_remove_otg
*
* Description:
*    void
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
static int sw_hcd_remove_otg(struct platform_device *pdev)
{
	struct sw_hcd *sw_hcd = dev_to_sw_hcd(&pdev->dev);

    sw_hcd_shutdown(pdev);
    if (sw_hcd->board_mode == SW_HCD_HOST){
		usb_remove_hcd(sw_hcd_to_hcd(sw_hcd));
	}

    sw_hcd_free(sw_hcd);
	device_init_wakeup(&pdev->dev, 0);

	pdev->dev.dma_mask = 0;

	sw_hcd_io_exit(usbc_no, pdev, &g_sw_hcd_io);
	g_hcd0_pdev = NULL;
	usbc_no = 0;

	return 0;
}

/*
*******************************************************************************
*                     sw_hcd_probe_host_only
*
* Description:
*    all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
* bridge to a platform device; this driver then suffices.
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
static int sw_hcd_probe_host_only(struct platform_device *pdev)
{
	struct device   *dev        = &pdev->dev;
	int             irq         = AW_IRQ_USB_OTG; //platform_get_irq(pdev, 0);
	__s32 			ret 		= 0;

	if (irq == 0){
	    DMSG_PANIC("ERR: platform_get_irq failed\n");
		return -ENODEV;
	}

	usbc_no = 0;

    memset(&g_sw_hcd_io, 0, sizeof(sw_hcd_io_t));
	ret = sw_hcd_io_init(usbc_no, pdev, &g_sw_hcd_io);
	if(ret != 0){
		DMSG_PANIC("ERR: sw_hcd_io_init failed\n");
		return -ENODEV;
	}

	ret = sw_hcd_init_controller(dev, irq, g_sw_hcd_io.usb_vbase);
	if(ret != 0){
		DMSG_PANIC("ERR: sw_hcd_init_controller failed\n");
		return -ENODEV;
	}

#ifdef  CONFIG_USB_SW_SUN6I_USB0_HOST
	if(!g_sw_hcd_io.host_init_state){
		sw_usb_disable_hcd0();
	}
#endif

    return 0;
}

/*
*******************************************************************************
*                     sw_hcd_remove_host_only
*
* Description:
*    void
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
static int sw_hcd_remove_host_only(struct platform_device *pdev)
{
	struct sw_hcd     *sw_hcd = dev_to_sw_hcd(&pdev->dev);

    sw_hcd_shutdown(pdev);
    if (sw_hcd->board_mode == SW_HCD_HOST){
		usb_remove_hcd(sw_hcd_to_hcd(sw_hcd));
	}

	sw_hcd->enable = 0;
	g_sw_hcd0 = NULL;

    sw_hcd_free(sw_hcd);
	device_init_wakeup(&pdev->dev, 0);

	pdev->dev.dma_mask = 0;

	sw_hcd_io_exit(usbc_no, pdev, &g_sw_hcd_io);

	return 0;
}

/*
*******************************************************************************
*                     sw_hcd_probe
*
* Description:
*    void
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
static int __init sw_hcd_probe(struct platform_device *pdev)
{
 #ifdef  CONFIG_USB_SW_SUN6I_USB0_OTG
	struct sw_hcd_platform_data	*pdata = pdev->dev.platform_data;
    switch(pdata->config->port_info->port_type){
        case USB_PORT_TYPE_HOST:
            return sw_hcd_probe_host_only(pdev);
        //break;

        case USB_PORT_TYPE_OTG:
            return sw_hcd_probe_otg(pdev);
        //break;

        default:
            DMSG_PANIC("ERR: unkown port_type(%d)\n", pdata->config->port_info->port_type);
    }

    return 0;
#else
    return sw_hcd_probe_host_only(pdev);
#endif
}

/*
*******************************************************************************
*                     sw_hcd_remove
*
* Description:
*    void
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
static int __devexit sw_hcd_remove(struct platform_device *pdev)
{
#ifdef  CONFIG_USB_SW_SUN6I_USB0_OTG
	struct sw_hcd_platform_data	*pdata = pdev->dev.platform_data;

    switch(pdata->config->port_info->port_type){
        case USB_PORT_TYPE_HOST:
            return sw_hcd_remove_host_only(pdev);
        //break;

        case USB_PORT_TYPE_OTG:
            return sw_hcd_remove_otg(pdev);
        //break;

        default:
            DMSG_PANIC("ERR: unkown port_type(%d)\n", pdata->config->port_info->port_type);
    }

    return 0;
#else
    return sw_hcd_remove_host_only(pdev);
#endif
}

/*
*******************************************************************************
*                     sw_hcd_save_context
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    only save ep status regs
*
*******************************************************************************
*/
static void sw_hcd_save_context(struct sw_hcd *sw_hcd)
{
	int i = 0;
	void __iomem *sw_hcd_base = sw_hcd->mregs;

	/* Common Register */
	for(i = 0; i < SW_HCD_C_NUM_EPS; i++){
		USBC_SelectActiveEp(sw_hcd->sw_hcd_io->usb_bsp_hdle, i);

		if(i == 0){
			sw_hcd_context.ep_reg[i].USB_CSR0 = USBC_Readl(USBC_REG_EX_USB_CSR0(sw_hcd_base));
		}else{
			sw_hcd_context.ep_reg[i].USB_TXCSR = USBC_Readl(USBC_REG_EX_USB_TXCSR(sw_hcd_base));
			sw_hcd_context.ep_reg[i].USB_RXCSR	= USBC_Readl(USBC_REG_EX_USB_RXCSR(sw_hcd_base));
		}

		if(i == 0){
			sw_hcd_context.ep_reg[i].USB_ATTR0 = USBC_Readl(USBC_REG_EX_USB_ATTR0(sw_hcd_base));
		}else{
			sw_hcd_context.ep_reg[i].USB_EPATTR = USBC_Readl(USBC_REG_EX_USB_EPATTR(sw_hcd_base));
			sw_hcd_context.ep_reg[i].USB_TXFIFO	= USBC_Readl(USBC_REG_EX_USB_TXFIFO(sw_hcd_base));
			sw_hcd_context.ep_reg[i].USB_RXFIFO	= USBC_Readl(USBC_REG_EX_USB_RXFIFO(sw_hcd_base));
		}

		sw_hcd_context.ep_reg[i].USB_TXFADDR	= USBC_Readl(USBC_REG_EX_USB_TXFADDR(sw_hcd_base));
		if(i != 0){
			sw_hcd_context.ep_reg[i].USB_RXFADDR	= USBC_Readl(USBC_REG_EX_USB_RXFADDR(sw_hcd_base));
		}
	}

	return;
}

/*
*******************************************************************************
*                     sw_hcd_save_context
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    only save ep status regs
*
*******************************************************************************
*/
static void sw_hcd_restore_context(struct sw_hcd *sw_hcd)
{
	int i = 0;
	void __iomem *sw_hcd_base = sw_hcd->mregs;

	/* Common Register */
	for(i = 0; i < SW_HCD_C_NUM_EPS; i++){
		USBC_SelectActiveEp(sw_hcd->sw_hcd_io->usb_bsp_hdle, i);

		if(i == 0){
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_CSR0, USBC_REG_EX_USB_CSR0(sw_hcd_base));
		}else{
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_TXCSR, USBC_REG_EX_USB_TXCSR(sw_hcd_base));
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_RXCSR, USBC_REG_EX_USB_RXCSR(sw_hcd_base));
		}

		if(i == 0){
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_ATTR0, USBC_REG_EX_USB_ATTR0(sw_hcd_base));
		}else{
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_EPATTR, USBC_REG_EX_USB_EPATTR(sw_hcd_base));
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_TXFIFO, USBC_REG_EX_USB_TXFIFO(sw_hcd_base));
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_RXFIFO, USBC_REG_EX_USB_RXFIFO(sw_hcd_base));
		}

		USBC_Writel(sw_hcd_context.ep_reg[i].USB_TXFADDR, USBC_REG_EX_USB_TXFADDR(sw_hcd_base));
		if(i != 0){
			USBC_Writel(sw_hcd_context.ep_reg[i].USB_RXFADDR, USBC_REG_EX_USB_RXFADDR(sw_hcd_base));
		}
	}

	return ;
}

/*
*******************************************************************************
*                     sw_usb_disable_hcd0
*
* Description:
*    void
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
int sw_usb_disable_hcd0(void)
{
#ifdef  CONFIG_USB_SW_SUN6I_USB0_HOST
	struct device *dev = NULL;
	struct platform_device *pdev = NULL;
	unsigned long flags = 0;
	struct sw_hcd *sw_hcd = NULL;

	DMSG_INFO("sw_usb_disable_hcd0 start, clk_is_open = %d\n",
		      		sw_hcd->sw_hcd_io->clk_is_open);

	if(!g_sw_hcd0){
		DMSG_PANIC("WRN: hcd is disable, g_sw_hcd0 is null\n");
		return 0;
	}

	dev    = g_sw_hcd0->controller;
	pdev   = to_platform_device(dev);
	sw_hcd = dev_to_sw_hcd(&pdev->dev);

	if(sw_hcd == NULL){
		DMSG_PANIC("ERR: sw_hcd is null\n");
		return 0;
	}

#ifndef  CONFIG_USB_SW_SUN6I_USB0_HOST
	if(sw_hcd->config->port_info->port_type != USB_PORT_TYPE_HOST
	  || sw_hcd->config->port_info->host_init_state){
        DMSG_PANIC("ERR: only host mode support sw_usb_disable_hcd, (%d, %d)\n",
                   sw_hcd->config->port_info->port_type,
                   sw_hcd->config->port_info->host_init_state);
		return 0;
	}
#endif

	if(!sw_hcd->enable){
		DMSG_PANIC("WRN: hcd is disable, can not enter to disable again\n");
		return 0;
	}

	if(!sw_hcd->sw_hcd_io->clk_is_open){
		DMSG_PANIC("ERR: sw_usb_disable_hcd0, usb clock is close, can't close again\n");
		return 0;
	}

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd_port_suspend_ex(sw_hcd);
	sw_hcd_stop(sw_hcd);
	sw_hcd_save_context(sw_hcd);
	spin_unlock_irqrestore(&sw_hcd->lock, flags);
	sw_hcd_set_vbus(sw_hcd, 0);

	close_usb_clock(sw_hcd->sw_hcd_io);

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd_soft_disconnect(sw_hcd);
	sw_hcd->enable = 0;
	spin_unlock_irqrestore(&sw_hcd->lock, flags);

	DMSG_INFO("sw_usb_disable_hcd0 end\n");
#endif

	return 0;
}
EXPORT_SYMBOL(sw_usb_disable_hcd0);

/*
*******************************************************************************
*                     sw_usb_enable_hcd0
*
* Description:
*    void
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
int sw_usb_enable_hcd0(void)
{
#ifdef  CONFIG_USB_SW_SUN6I_USB0_HOST
	struct device *dev = NULL;
	struct platform_device *pdev = NULL;
	unsigned long	flags = 0;
	struct sw_hcd	*sw_hcd = NULL;

	DMSG_INFO("sw_usb_enable_hcd0 start, clk_is_open = %d\n",
		      		sw_hcd->sw_hcd_io->clk_is_open);

	if(!g_sw_hcd0){
		DMSG_PANIC("WRN: g_sw_hcd0 is null\n");
		return 0;
	}

	dev    = g_sw_hcd0->controller;
	pdev   = to_platform_device(dev);
	sw_hcd = dev_to_sw_hcd(&pdev->dev);

	if(sw_hcd == NULL){
		DMSG_PANIC("ERR: sw_hcd is null\n");
		return 0;
	}

#ifndef  CONFIG_USB_SW_SUN6I_USB0_HOST
	if(sw_hcd->config->port_info->port_type != USB_PORT_TYPE_HOST
	  || sw_hcd->config->port_info->host_init_state){
        DMSG_PANIC("ERR: only host mode support sw_usb_enable_hcd, (%d, %d)\n",
                   sw_hcd->config->port_info->port_type,
                   sw_hcd->config->port_info->host_init_state);
		return 0;
	}
#endif

	if(sw_hcd->enable == 1){
		DMSG_PANIC("WRN: hcd is already enable, can not enable again\n");
		return 0;
	}

	if(sw_hcd->sw_hcd_io->clk_is_open){
		DMSG_PANIC("ERR: sw_usb_enable_hcd0, usb clock is open, can't open again\n");
		return 0;
	}

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd->enable = 1;
	spin_unlock_irqrestore(&sw_hcd->lock, flags);

	open_usb_clock(sw_hcd->sw_hcd_io);

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd_restore_context(sw_hcd);
	sw_hcd_start(sw_hcd);
	spin_unlock_irqrestore(&sw_hcd->lock, flags);
    /* port power on */
	sw_hcd_set_vbus(sw_hcd, 1);

	DMSG_INFO("sw_usb_enable_hcd0 end\n");
#endif

	return 0;
}
EXPORT_SYMBOL(sw_usb_enable_hcd0);

/*
*******************************************************************************
*                     sw_hcd_suspend
*
* Description:
*    void
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
static int sw_hcd_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	unsigned long	flags = 0;
	struct sw_hcd	*sw_hcd = dev_to_sw_hcd(&pdev->dev);

	DMSG_INFO_HCD0("sw_hcd_suspend start\n");

	atomic_set(&thread_suspend_flag, 1);

	if(!sw_hcd->enable){
		DMSG_INFO("wrn: hcd is disable, need not enter to suspend\n");
		return 0;
	}

	if(!sw_hcd->sw_hcd_io->clk_is_open){
		DMSG_INFO("wrn: sw_hcd_suspend, usb clock is close, can't close again\n");
		return 0;
	}

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd->suspend = 1;
	sw_hcd_port_suspend_ex(sw_hcd);
	sw_hcd_stop(sw_hcd);
	sw_hcd_save_context(sw_hcd);
	spin_unlock_irqrestore(&sw_hcd->lock, flags);
	sw_hcd_set_vbus(sw_hcd, 0);

	close_usb_clock(sw_hcd->sw_hcd_io);

	DMSG_INFO_HCD0("sw_hcd_suspend end\n");

	return 0;
}

/*
*******************************************************************************
*                     sw_hcd_resume_early
*
* Description:
*    void
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
static int sw_hcd_resume(struct device *dev)
{	struct platform_device *pdev = to_platform_device(dev);
	unsigned long	flags = 0;
	struct sw_hcd	*sw_hcd = dev_to_sw_hcd(&pdev->dev);

	DMSG_INFO_HCD0("sw_hcd_resume start\n");

	atomic_set(&thread_suspend_flag, 0);

	if(!sw_hcd->enable){
		DMSG_INFO("wrn: hcd is disable, need not resume\n");
		return 0;
	}

	if(sw_hcd->sw_hcd_io->clk_is_open){
		DMSG_INFO("wrn: sw_hcd_suspend, usb clock is open, can't open again\n");
		return 0;
	}

	sw_hcd_soft_disconnect(sw_hcd);
	open_usb_clock(sw_hcd->sw_hcd_io);

	spin_lock_irqsave(&sw_hcd->lock, flags);
	sw_hcd_restore_context(sw_hcd);
	sw_hcd_start(sw_hcd);
	sw_hcd->suspend = 0;
	spin_unlock_irqrestore(&sw_hcd->lock, flags);
    /* port power on */
	sw_hcd_set_vbus(sw_hcd, 1);

	DMSG_INFO_HCD0("sw_hcd_resume_early end\n");

	return 0;
}

static const struct dev_pm_ops sw_hcd_dev_pm_ops = {
	.suspend		= sw_hcd_suspend,
	.resume     	= sw_hcd_resume,
};

static struct platform_driver sw_hcd_driver = {
	.driver = {
		.name		= (char *)sw_hcd_driver_name,
		.bus		= &platform_bus_type,
		.owner		= THIS_MODULE,
		.pm			= &sw_hcd_dev_pm_ops,
	},

	.remove		    = __devexit_p(sw_hcd_remove),
	.shutdown	    = sw_hcd_shutdown,
};

/*
*******************************************************************************
*                     sw_hcd_init
*
* Description:
*    void
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
static int __init sw_hcd_init(void)
{
	DMSG_INFO_HCD0("usb host driver initialize........\n");

    if (usb_disabled()){
        DMSG_PANIC("ERR: usb disabled\n");
		return 0;
	}


    return platform_driver_probe(&sw_hcd_driver, sw_hcd_probe);
}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
module_init(sw_hcd_init);
//fs_initcall(sw_hcd_init);

/*
*******************************************************************************
*                     sw_hcd_cleanup
*
* Description:
*    void
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
static void __exit sw_hcd_cleanup(void)
{
	platform_driver_unregister(&sw_hcd_driver);

	DMSG_INFO_HCD0("usb host driver exit........\n");
}

module_exit(sw_hcd_cleanup);


