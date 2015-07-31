/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc_board.c
*
* Author 		: javen
*
* Description 	: 板级控制
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
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
#include <linux/err.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>

#include  <mach/clock.h>
#include  <mach/platform.h>
#include  "sw_udc_config.h"
#include  "sw_udc_board.h"

//---------------------------------------------------------------
//  宏 定义
//---------------------------------------------------------------

#define res_size(_r) (((_r)->end - (_r)->start) + 1)

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
u32  open_usb_clock(sw_udc_io_t *sw_udc_io)
{
 	DMSG_INFO_UDC("open_usb_clock\n");

	if(sw_udc_io->ahb_otg && sw_udc_io->mod_usbotg && sw_udc_io->mod_usbphy && !sw_udc_io->clk_is_open){
	   	clk_enable(sw_udc_io->ahb_otg);
		udelay(100);

	    //clk_enable(sw_udc_io->mod_usbotg); /*NO SCLK_GATING_OTG */
		clk_reset(sw_udc_io->mod_usbotg, AW_CCU_CLK_NRESET);

	    clk_enable(sw_udc_io->mod_usbphy);
	    clk_reset(sw_udc_io->mod_usbphy, AW_CCU_CLK_NRESET);
		udelay(100);

		sw_udc_io->clk_is_open = 1;
	}else{
		DMSG_PANIC("ERR: clock handle is null, ahb_otg(0x%p), mod_usbotg(0x%p), mod_usbphy(0x%p), open(%d)\n",
			       sw_udc_io->ahb_otg, sw_udc_io->mod_usbotg, sw_udc_io->mod_usbphy, sw_udc_io->clk_is_open);
	}

	UsbPhyInit(0);

/*
	DMSG_INFO("[udc0]: open, 0x60(0x%x), 0xcc(0x%x), 0x2c0(0x%x)\n",
		      (u32)USBC_Readl(sw_udc_io->clock_vbase + 0x60),
		      (u32)USBC_Readl(sw_udc_io->clock_vbase + 0xcc),
			  (u32)USBC_Readl(sw_udc_io->clock_vbase + 0x2c0));
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
u32 close_usb_clock(sw_udc_io_t *sw_udc_io)
{
	DMSG_INFO_UDC("close_usb_clock\n");

	if(sw_udc_io->ahb_otg && sw_udc_io->mod_usbotg && sw_udc_io->mod_usbphy && sw_udc_io->clk_is_open){
		sw_udc_io->clk_is_open = 0;

	    clk_reset(sw_udc_io->mod_usbphy, AW_CCU_CLK_RESET);
	    clk_disable(sw_udc_io->mod_usbphy);

	    //clk_disable(sw_udc_io->mod_usbotg);  /*NO SCLK_GATING_OTG */
		clk_reset(sw_udc_io->mod_usbotg, AW_CCU_CLK_RESET);

	    clk_disable(sw_udc_io->ahb_otg);
	}else{
		DMSG_PANIC("ERR: clock handle is null, ahb_otg(0x%p), mod_usbotg(0x%p), mod_usbphy(0x%p), open(%d)\n",
			       sw_udc_io->ahb_otg, sw_udc_io->mod_usbotg, sw_udc_io->mod_usbphy, sw_udc_io->clk_is_open);
	}

/*
	DMSG_INFO("[udc0]: close, 0x60(0x%x), 0xcc(0x%x),0x2c0(0x%x)\n",
		      (u32)USBC_Readl(sw_udc_io->clock_vbase + 0x60),
		      (u32)USBC_Readl(sw_udc_io->clock_vbase + 0xcc),
			  (u32)USBC_Readl(sw_udc_io->clock_vbase + 0x2c0));
*/
	return 0;
}

/*
*******************************************************************************
*                     sw_udc_bsp_init
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
__s32 sw_udc_bsp_init(__u32 usbc_no, sw_udc_io_t *sw_udc_io)
{
	sw_udc_io->usbc.usbc_info[usbc_no].num = usbc_no;
   	sw_udc_io->usbc.usbc_info[usbc_no].base = (u32)sw_udc_io->usb_vbase;
	sw_udc_io->usbc.sram_base = (u32)sw_udc_io->sram_vbase;

//	USBC_init(&sw_udc_io->usbc);
	sw_udc_io->usb_bsp_hdle = USBC_open_otg(usbc_no);
	if(sw_udc_io->usb_bsp_hdle == 0){
		DMSG_PANIC("ERR: sw_udc_init: USBC_open_otg failed\n");
		return -1;
	}
#ifdef  SW_USB_FPGA
	clear_usb_reg((u32)sw_udc_io->usb_vbase);
	fpga_config_use_otg(sw_udc_io->usbc.sram_base);
#endif

	USBC_EnhanceSignal(sw_udc_io->usb_bsp_hdle);

	USBC_EnableDpDmPullUp(sw_udc_io->usb_bsp_hdle);
    USBC_EnableIdPullUp(sw_udc_io->usb_bsp_hdle);
	USBC_ForceId(sw_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DEVICE);
	USBC_ForceVbusValid(sw_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

	USBC_SelectBus(sw_udc_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

    return 0;
}

/*
*******************************************************************************
*                     sw_udc_bsp_exit
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
__s32 sw_udc_bsp_exit(__u32 usbc_no, sw_udc_io_t *sw_udc_io)
{
    USBC_DisableDpDmPullUp(sw_udc_io->usb_bsp_hdle);
    USBC_DisableIdPullUp(sw_udc_io->usb_bsp_hdle);
	USBC_ForceId(sw_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DISABLE);
	USBC_ForceVbusValid(sw_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_DISABLE);

	USBC_close_otg(sw_udc_io->usb_bsp_hdle);
	sw_udc_io->usb_bsp_hdle = 0;

//	USBC_exit(&sw_udc_io->usbc);

    return 0;
}

/*
*******************************************************************************
*                     sw_udc_io_init
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
__s32 sw_udc_io_init(__u32 usbc_no, struct platform_device *pdev, sw_udc_io_t *sw_udc_io)
{
	__s32 ret = 0;
	spinlock_t lock;
	unsigned long flags = 0;

	sw_udc_io->usb_vbase  = (void __iomem *)AW_VIR_USB_OTG_BASE;
	sw_udc_io->sram_vbase = (void __iomem *)AW_VIR_SRAMCTRL_BASE;

	DMSG_INFO_UDC("usb_vbase  = 0x%x\n", (u32)sw_udc_io->usb_vbase);
	DMSG_INFO_UDC("sram_vbase = 0x%x\n", (u32)sw_udc_io->sram_vbase);

    /* open usb lock */
	sw_udc_io->ahb_otg = clk_get(NULL, CLK_AHB_OTG);
	if (IS_ERR(sw_udc_io->ahb_otg)){
		DMSG_PANIC("ERR: get usb ahb_otg clk failed.\n");
		goto io_failed;
	}

	sw_udc_io->mod_usbotg = clk_get(NULL, CLK_MOD_USBOTG);
	if (IS_ERR(sw_udc_io->mod_usbotg)){
		DMSG_PANIC("ERR: get usb mod_usbotg failed.\n");
		goto io_failed;
	}

	sw_udc_io->mod_usbphy = clk_get(NULL, CLK_MOD_USBPHY0);
	if (IS_ERR(sw_udc_io->mod_usbphy )){
		DMSG_PANIC("ERR: get usb mod_usbphy failed.\n");
		goto io_failed;
	}

	open_usb_clock(sw_udc_io);

    /* initialize usb bsp */
	sw_udc_bsp_init(usbc_no, sw_udc_io);

	/* config usb fifo */
	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);
	USBC_ConfigFIFO_Base(sw_udc_io->usb_bsp_hdle, (u32)sw_udc_io->sram_vbase, USBC_FIFO_MODE_8K);
	spin_unlock_irqrestore(&lock, flags);

	//print_all_usb_reg(NULL, (__u32)sw_udc_io->usb_vbase, 0, 5, "usb_u54555555555dc");

	return 0;

io_failed:
	if(sw_udc_io->ahb_otg){
		clk_put(sw_udc_io->ahb_otg);
		sw_udc_io->ahb_otg = NULL;
	}

	if(sw_udc_io->mod_usbotg){
		clk_put(sw_udc_io->mod_usbotg);
		sw_udc_io->mod_usbotg = NULL;
	}

	if(sw_udc_io->mod_usbphy){
		clk_put(sw_udc_io->mod_usbphy);
		sw_udc_io->mod_usbphy = NULL;
	}
	return ret;
}

/*
*******************************************************************************
*                     sw_udc_exit
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
__s32 sw_udc_io_exit(__u32 usbc_no, struct platform_device *pdev, sw_udc_io_t *sw_udc_io)
{
	sw_udc_bsp_exit(usbc_no, sw_udc_io);

	close_usb_clock(sw_udc_io);
#ifndef  SW_USB_FPGA
	if(sw_udc_io->ahb_otg){
		clk_put(sw_udc_io->ahb_otg);
		sw_udc_io->ahb_otg = NULL;
	}

	if(sw_udc_io->mod_usbotg){
		clk_put(sw_udc_io->mod_usbotg);
		sw_udc_io->mod_usbotg = NULL;
	}

	if(sw_udc_io->mod_usbphy){
		clk_put(sw_udc_io->mod_usbphy);
		sw_udc_io->mod_usbphy = NULL;
	}
#endif
	sw_udc_io->usb_vbase  = NULL;
	sw_udc_io->sram_vbase = NULL;

	return 0;
}





