/*
*************************************************************************************
*                         			      Linux
*					                 USB Host Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_usb_debug.h
*
* Author 		: javen
*
* Description 	:
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_USB_DEBUG_H__
#define  __SW_USB_DEBUG_H__

#ifdef  CONFIG_USB_SW_SUN6I_USB_DEBUG

#define usb_printk		printk

#define  DMSG_INFO_UDC(format,args...)   			usb_printk("[sw_udc]: "format,##args)
#define  DMSG_INFO_HCD0(format,args...)   		usb_printk("[sw_hcd0]: "format,##args)
#define  DMSG_INFO_HCD1(format,args...)   		usb_printk("[sw_hcd1]: "format,##args)
#define  DMSG_INFO_HCD2(format,args...)   		usb_printk("[sw_hcd2]: "format,##args)
#define  DMSG_INFO_MANAGER(format,args...)   	usb_printk("[usb_manager]: "format,##args)

#else

#define  DMSG_PRINT(...)
#define  DMSG_INFO_UDC(...)
#define  DMSG_INFO_HCD0(...)
#define  DMSG_INFO_HCD1(...)
#define  DMSG_INFO_HCD2(...)
#define  DMSG_INFO_MANAGER(...)

#endif

#define  DMSG_PRINT(format,args...)   	  		printk("[sun6i_usb]: "format,##args)
#define  DMSG_ERR(format,args...)   			printk("[sun6i_usb]: "format,##args)


/* 测试 */
#if  0
    #define DMSG_TEST         			DMSG_PRINT
#else
    #define DMSG_TEST(...)
#endif

/* 代码调试 */
#if  0
    #define DMSG_MANAGER_DEBUG          DMSG_PRINT
#else
    #define DMSG_MANAGER_DEBUG(...)
#endif

#if  0
    #define DMSG_DEBUG        			DMSG_PRINT
#else
    #define DMSG_DEBUG(...)
#endif

/* 普通信息打印 */
#if  1
    #define DMSG_INFO         			DMSG_PRINT
#else
    #define DMSG_INFO(...)
#endif

/* 严重警告 */
#if	1
    #define DMSG_PANIC        			DMSG_ERR
#else
    #define DMSG_PANIC(...)
#endif

/* 普通警告 */
#if	0
    #define DMSG_WRN        			DMSG_ERR
#else
    #define DMSG_WRN(...)
#endif

/* dma 调试打印 */
#if	0
    #define DMSG_DBG_DMA     			DMSG_PRINT
#else
    #define DMSG_DBG_DMA(...)
#endif

//---------------------------------------------------------------
//  函数 定义
//---------------------------------------------------------------
void print_usb_reg_by_ep(spinlock_t *lock, __u32 usbc_base, __s32 ep_index, char *str);
void print_all_usb_reg(spinlock_t *lock, __u32 usbc_base, __s32 ep_start, __u32 ep_end, char *str);

void clear_usb_reg(__u32 usb_base);
void fpga_config_use_otg(__u32 sram_vbase);

#endif   //__SW_USB_DEBUG_H__

