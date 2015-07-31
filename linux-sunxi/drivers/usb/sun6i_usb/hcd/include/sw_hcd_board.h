/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hcd_board.h
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
#ifndef  __SW_HCD_BOARD_H__
#define  __SW_HCD_BOARD_H__

#include <mach/sys_config.h>

#define  USB_SRAMC_BASE	            0x01c00000
#define  USB_CLOCK_BASE				0x01C20000
#define  USB_PIO_BASE	            0x01c20800

/* i/o 信息 */
typedef struct sw_hcd_io{
	struct resource	*usb_base_res;   	/* USB  resources 		*/
	struct resource	*usb_base_req;   	/* USB  resources 		*/
	void __iomem	*usb_vbase;			/* USB  base address 	*/

	struct resource	*sram_base_res;   	/* SRAM resources 		*/
	struct resource	*sram_base_req;   	/* SRAM resources 		*/
	void __iomem	*sram_vbase;		/* SRAM base address 	*/

	struct resource	*clock_base_res;   	/* clock resources 		*/
	struct resource	*clock_base_req;   	/* clock resources 		*/
	void __iomem	*clock_vbase;		/* clock base address 	*/

	struct resource	*pio_base_res;   	/* pio resources 		*/
	struct resource	*pio_base_req;   	/* pio resources 		*/
	void __iomem	*pio_vbase;			/* pio base address 	*/

	bsp_usbc_t usbc;					/* usb bsp config 		*/
	__hdle usb_bsp_hdle;				/* usb bsp handle 		*/

	__u32 clk_is_open;					/* is usb clock open? 	*/
	struct clk	*ahb_otg;				/* ahb clock handle 	*/
	struct clk	*mod_usbotg;			/* mod_usb otg clock handle 	*/
	struct clk	*mod_usbphy;			/* PHY0 clock handle 	*/

	unsigned drv_vbus_valid;
	unsigned usb_restrict_valid;
	script_item_u drv_vbus_gpio_set;
	script_item_u restrict_gpio_set;
	__u32 host_init_state;				/* usb 控制器的初始化状态。0 : 不工作. 1 : 工作 */
	__u32 usb_enable;
	__u32 usb_restrict_flag;
}sw_hcd_io_t;

#endif   //__SW_HCD_BOARD_H__


