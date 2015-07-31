/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_usb_config.h
*
* Author 		: javen
*
* Description 	:
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-4-14            1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_USB_CONFIG_H__
#define  __SW_USB_CONFIG_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include  "sw_usb_typedef.h"
#include  "sw_usb_debug.h"
#include  "sun6i_usb_bsp.h"
#include  "sun6i_sys_reg.h"

#include <mach/sys_config.h>

#include  "sw_usb_board.h"
#include  "sw_udc.h"
#include  "sw_hcd.h"

#if defined (CONFIG_AW_FPGA_V4_PLATFORM) || defined (CONFIG_AW_FPGA_V7_PLATFORM)
#define   SW_USB_FPGA
#endif

#endif   //__SW_USB_CONFIG_H__

