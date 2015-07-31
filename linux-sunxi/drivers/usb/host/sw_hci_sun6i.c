/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hci_sun6i.c
*
* Author 		: javen
*
* Description 	: Include file for AW1623 HCI Host Controller Driver
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    yangnaitian      2011-5-24            1.0          create this file
*    javen            2011-7-18            1.1          �����ʱ�ӿ��غ͹��翪��
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
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>
#include <mach/platform.h>

#include  <mach/clock.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>

#include  "sw_hci_sun6i.h"

#ifndef  SW_USB_FPGA
static char* usbc_name[4] 			= {"usbc0", "usbc1", "usbc2", "usbc3"};
#endif

static char* usbc_ahb_ehci_name[3]  = {"", CLK_AHB_EHCI0, CLK_AHB_EHCI1};
static char* usbc_ahb_ohci_name[4]  = {"", CLK_AHB_OHCI0, CLK_AHB_OHCI1, CLK_AHB_OHCI2};

static char* usbc_ehci_name[3]  = {"", CLK_MOD_USBEHCI0, CLK_MOD_USBEHCI1};
static char* usbc_ohci_name[4]  = {"", CLK_MOD_USBOHCI0, CLK_MOD_USBOHCI1, CLK_MOD_USBOHCI2};

static char* usbc_phy_name[3] 	= {CLK_MOD_USBPHY0, CLK_MOD_USBPHY1, CLK_MOD_USBPHY2};

static u32 usbc_base[4] 			= {AW_VIR_USB_OTG_BASE, AW_VIR_USB_EHCI0_BASE, AW_VIR_USB_EHCI1_BASE, AW_VIR_USB_OHCI2_BASE};
static u32 ehci_irq_no[3] 			= {0, AW_IRQ_USB_EHCI0, AW_IRQ_USB_EHCI1};
static u32 ohci_irq_no[4] 			= {0, AW_IRQ_USB_OHCI0, AW_IRQ_USB_OHCI1, AW_IRQ_USB_OHCI2};

static u32 usb1_set_vbus_cnt = 0;
static u32 usb2_set_vbus_cnt = 0;
static u32 usb3_set_vbus_cnt = 0;

static u32 usb1_enable_passly_cnt = 0;
static u32 usb2_enable_passly_cnt = 0;
static u32 usb3_enable_passly_cnt = 0;

static void sw_usb_3g_config(struct sw_hci_hcd *sw_hci)
{
    script_item_value_type_e type = 0;
	script_item_u item_temp;
    u32 usb_3g_used      = 0;
    u32 usb_3g_usbc_num  = 0;
    u32 usb_3g_usbc_type = 0;

    /* 3g_used */
    type = script_get_item("3g_para", "3g_used", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        usb_3g_used = item_temp.val;
    }else{
        //DMSG_PANIC("WRN: script_parser_fetch usb_3g_used failed\n");
        return;
    }

    /* 3g_usbc_num */
    type = script_get_item("3g_para", "3g_usbc_num", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        usb_3g_usbc_num = item_temp.val;
    }else{
        //DMSG_PANIC("WRN: script_parser_fetch usb_3g_usbc_num failed\n");
        return;
    }

    /* 3g_usbc_type */
    type = script_get_item("3g_para", "3g_usbc_type", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        usb_3g_usbc_type = item_temp.val;
    }else{
        //DMSG_PANIC("WRN: script_parser_fetch usb_3g_usbc_type failed\n");
        return;
    }

    /* ֻ��3Gʹ�õ��Ǹ������� */
    if(sw_hci->usbc_no == usb_3g_usbc_num){
        sw_hci->used = 0;
        if(sw_hci->usbc_type == usb_3g_usbc_type){
            sw_hci->used = 1;
        }
    }

    return;
}

/*
*******************************************************************************
*                     get_usb_cfg
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
static s32 get_usb_cfg(struct sw_hci_hcd *sw_hci)
{
#ifndef  SW_USB_FPGA

    script_item_value_type_e type = 0;
	script_item_u item_temp;

	/* usbc enable */
	type = script_get_item(usbc_name[sw_hci->usbc_no], "usb_used", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sw_hci->used = item_temp.val;
	}else{
		DMSG_INFO("ERR: get %s usbc enable failed\n" ,sw_hci->hci_name);
		sw_hci->used = 0;
	}

	/* usbc restrict_gpio */
	type = script_get_item(usbc_name[sw_hci->usbc_no], "usb_restrict_gpio", &sw_hci->restrict_gpio_set);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
		sw_hci->usb_restrict_valid = 1;
	}else{
		DMSG_INFO("ERR: %s(restrict_gpio) is invalid\n", sw_hci->hci_name);
		sw_hci->usb_restrict_valid = 0;
	}

	/* usbc drv_vbus */
	type = script_get_item(usbc_name[sw_hci->usbc_no], "usb_drv_vbus_gpio", &sw_hci->drv_vbus_gpio_set);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
		sw_hci->drv_vbus_gpio_valid = 1;
	}else{
		DMSG_INFO("ERR: %s(drv vbus) is invalid\n", sw_hci->hci_name);
		sw_hci->drv_vbus_gpio_valid = 0;
	}

	/* host_init_state */
	type = script_get_item(usbc_name[sw_hci->usbc_no], "usb_host_init_state", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sw_hci->host_init_state = item_temp.val;
	}else{
		DMSG_INFO("ERR: script_parser_fetch host_init_state failed\n");
		sw_hci->host_init_state = 1;
	}


	/* get usb_restrict_flag */
	type = script_get_item(usbc_name[sw_hci->usbc_no], "usb_restric_flag", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sw_hci->usb_restrict_flag = item_temp.val;
	}else{
		DMSG_INFO("ERR: get usb_restrict_flag failed\n");
		sw_hci->usb_restrict_flag = 0;
	}

	type = script_get_item(usbc_name[sw_hci->usbc_no], "usb_not_suspend", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sw_hci->not_suspend = item_temp.val;
	}else{
		DMSG_INFO("ERR: get usb_restrict_flag failed\n");
		sw_hci->not_suspend = 0;
	}

	sw_usb_3g_config(sw_hci);

	/* wifi_used */
	if(sw_hci->host_init_state == 0){
	    u32 usb_wifi_used      = 0;
	    u32 usb_wifi_usbc_num  = 0;
	    u32 usb_wifi_usbc_type = 0;

		type = script_get_item("wifi_para", "wifi_used", &item_temp);
		if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
			usb_wifi_used = item_temp.val;
		}else{
			DMSG_PANIC("ERR: script_parser_fetch wifi_used failed\n");
			usb_wifi_used = 0;
		}

	    if(usb_wifi_used){
	        /* wifi_usbc_num */
			type = script_get_item("wifi_para", "wifi_usbc_id", &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				usb_wifi_usbc_num = item_temp.val;
			}else{
				DMSG_PANIC("ERR: script_parser_fetch wifi_usbc_id failed\n");
				usb_wifi_usbc_num = 0;
			}

	        /* wifi_usbc_type */
			type = script_get_item("wifi_para", "wifi_usbc_type", &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				usb_wifi_usbc_type = item_temp.val;
			}else{
				DMSG_PANIC("ERR: script_parser_fetch wifi_usbc_type failed\n");
				usb_wifi_usbc_type = 0;
			}

	        /* ֻ��wifiʹ�õ��Ǹ�ģ�� */
	        if(sw_hci->usbc_no == usb_wifi_usbc_num){
	            sw_hci->used = 0;
	            if(sw_hci->usbc_type == usb_wifi_usbc_type){
	                sw_hci->used = 1;
	            }
	        }
	    }
	}
#else
	sw_hci->used = 1;
	sw_hci->host_init_state = 1;
#endif

	return 0;
}

/*
*******************************************************************************
*                     USBC_Phy_GetCsr
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
static __u32 USBC_Phy_GetCsr(__u32 usbc_no)
{
	__u32 val = 0x0;

	switch(usbc_no){
		case 0:
			val = AW_VIR_USB_OTG_BASE + 0x404;
		break;

		case 1:
			val = AW_VIR_USB_OTG_BASE + 0x404;
		break;

		case 2:
			val = AW_VIR_USB_OTG_BASE + 0x404;
		break;

		default:
		break;
	}

	return val;
}

/*
*******************************************************************************
*                     USBC_Phy_TpRead
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
#if 0
static __u32 USBC_Phy_TpRead(__u32 usbc_no, __u32 addr, __u32 len)
{
	__u32 temp = 0, ret = 0;
	__u32 i=0;
	__u32 j=0;

	for(j = len; j > 0; j--)
	{
		/* set  the bit address to be read */
		temp = USBC_Readl(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0xff << 8);
		temp |= ((addr + j -1) << 8);
		USBC_Writel(temp, USBC_Phy_GetCsr(usbc_no));

		for(i = 0; i < 0x4; i++);

		temp = USBC_Readl(USBC_Phy_GetCsr(usbc_no));
		ret <<= 1;
		ret |= ((temp >> (16 + usbc_no)) & 0x1);
	}

	return ret;
}
#endif

/*
*******************************************************************************
*                     USBC_Phy_TpWrite
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
static __u32 USBC_Phy_TpWrite(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	__u32 temp = 0, dtmp = 0;
//	__u32 i=0;
	__u32 j=0;

	dtmp = data;
	for(j = 0; j < len; j++)
	{
		/* set  the bit address to be write */
		temp = USBC_Readl(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0xff << 8);
		temp |= ((addr + j) << 8);
		USBC_Writel(temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		temp &= ~(0x1 << (usbc_no << 1));
		USBC_Writeb(temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp |= (0x1 << (usbc_no << 1));
		USBC_Writeb( temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0x1 << (usbc_no <<1 ));
		USBC_Writeb(temp, USBC_Phy_GetCsr(usbc_no));
		dtmp >>= 1;
	}

	return data;
}

/*
*******************************************************************************
*                     USBC_Phy_Read
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
#if 0
static __u32 USBC_Phy_Read(__u32 usbc_no, __u32 addr, __u32 len)
{
	return USBC_Phy_TpRead(usbc_no, addr, len);
}
#endif

/*
*******************************************************************************
*                     USBC_Phy_Write
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
static __u32 USBC_Phy_Write(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	return USBC_Phy_TpWrite(usbc_no, addr, data, len);
}


/*
*******************************************************************************
*                     UsbPhyInit
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
static void UsbPhyInit(__u32 usbc_no)
{
//	DMSG_INFO("csr1: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Readl(USBC_Phy_GetCsr(usbc_no)));

    /* ����45ŷ�迹 */
	if(usbc_no == 0){
	    USBC_Phy_Write(usbc_no, 0x0c, 0x01, 1);
	}

//	DMSG_INFO("csr2-0: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Phy_Read(usbc_no, 0x0c, 1));

    /* ���� USB0 PHY �ķ��Ⱥ����� */
	USBC_Phy_Write(usbc_no, 0x20, 0x14, 5);

//	DMSG_INFO("csr2-1: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Phy_Read(usbc_no, 0x20, 5));

    /* ���� disconnect ��ֵ */
	USBC_Phy_Write(usbc_no, 0x2a, 3, 2);    /*by wangjx*/

//	DMSG_INFO("csr2: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Phy_Read(usbc_no, 0x2a, 2));
//	DMSG_INFO("csr3: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Readl(USBC_Phy_GetCsr(usbc_no)));

	return;
}

/*
*******************************************************************************
*                     clock_init
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
static s32 clock_init(struct sw_hci_hcd *sw_hci, u32 ohci)
{
    if(ohci){  /* ohci */
    	sw_hci->ahb = clk_get(NULL, usbc_ahb_ohci_name[sw_hci->usbc_no]);
    	if (IS_ERR(sw_hci->ahb)){
    		DMSG_PANIC("ERR: get ohci%d abh clk failed.\n", (sw_hci->usbc_no - 1));
    		goto failed;
    	}

    	sw_hci->mod_usb = clk_get(NULL, usbc_ohci_name[sw_hci->usbc_no]);
    	if (IS_ERR(sw_hci->mod_usb)){
    		DMSG_PANIC("ERR: get ohci%d gate clk failed.\n", (sw_hci->usbc_no - 1));
    		goto failed;
    	}
	}else{  /* ehci */
    	sw_hci->ahb = clk_get(NULL, usbc_ahb_ehci_name[sw_hci->usbc_no]);
    	if (IS_ERR(sw_hci->ahb)){
    		DMSG_PANIC("ERR: get ehci%d abh clk failed.\n", (sw_hci->usbc_no - 1));
    		goto failed;
    	}

    	sw_hci->mod_usb = clk_get(NULL, usbc_ehci_name[sw_hci->usbc_no]);
    	if (IS_ERR(sw_hci->mod_usb)){
    		DMSG_PANIC("ERR: get ohci%d gate clk failed.\n", (sw_hci->usbc_no - 1));
    		goto failed;
    	}

	}

	sw_hci->mod_usbphy = clk_get(NULL, usbc_phy_name[sw_hci->usbc_no]);
	if (IS_ERR(sw_hci->mod_usbphy)){
		DMSG_PANIC("ERR: get usb%d mod_usbphy failed.\n", sw_hci->usbc_no);
		goto failed;
	}

	return 0;

failed:
	if(sw_hci->ahb){
		clk_put(sw_hci->ahb);
		sw_hci->ahb = NULL;
	}

	if(sw_hci->mod_usbphy){
		clk_put(sw_hci->mod_usbphy);
		sw_hci->mod_usbphy = NULL;
	}

	if(sw_hci->mod_usb){
	    clk_put(sw_hci->mod_usb);
		sw_hci->mod_usb = NULL;
	}

	return -1;
}

/*
*******************************************************************************
*                     clock_exit
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
static s32 clock_exit(struct sw_hci_hcd *sw_hci, u32 ohci)
{
	if(sw_hci->mod_usb){
		clk_put(sw_hci->mod_usb);
		sw_hci->mod_usb = NULL;
	}

	if(sw_hci->ahb){
		clk_put(sw_hci->ahb);
		sw_hci->ahb = NULL;
	}

	if(sw_hci->mod_usbphy){
		clk_put(sw_hci->mod_usbphy);
		sw_hci->mod_usbphy = NULL;
	}

	return 0;
}
/*
*******************************************************************************
*                     open_clock
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
static int open_clock(struct sw_hci_hcd *sw_hci, u32 ohci)
{
 	DMSG_INFO("[%s]: open clock\n", sw_hci->hci_name);

    if(sw_hci->ahb && sw_hci->mod_usb
       && sw_hci->mod_usbphy && !sw_hci->clk_is_open){
        sw_hci->clk_is_open = 1;

	    clk_enable(sw_hci->mod_usbphy);
		clk_reset(sw_hci->mod_usbphy, AW_CCU_CLK_NRESET);

        if(ohci){
            clk_enable(sw_hci->mod_usb);
        }
		clk_reset(sw_hci->mod_usb, AW_CCU_CLK_NRESET);

        udelay(100);

   	    clk_enable(sw_hci->ahb);

        udelay(100);

    	UsbPhyInit(sw_hci->usbc_no);
    }else{
		DMSG_PANIC("[%s]: wrn: open clock failed, (0x%p, 0x%p, %d, 0x%p)\n",
			      sw_hci->hci_name,
			      sw_hci->ahb, sw_hci->mod_usbphy, sw_hci->clk_is_open,
			      sw_hci->mod_usb);
	}
	return 0;
}

/*
*******************************************************************************
*                     close_clock
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
static int close_clock(struct sw_hci_hcd *sw_hci, u32 ohci)
{
 	DMSG_INFO("[%s]: close clock\n", sw_hci->hci_name);

    if(sw_hci->ahb && sw_hci->mod_usb
       && sw_hci->mod_usbphy && sw_hci->clk_is_open){

    	sw_hci->clk_is_open = 0;

        if(ohci){
	        clk_disable(sw_hci->mod_usb);
	    }
		clk_reset(sw_hci->mod_usb, AW_CCU_CLK_RESET);

	    clk_disable(sw_hci->mod_usbphy);
		clk_reset(sw_hci->mod_usbphy, AW_CCU_CLK_RESET);

	    clk_disable(sw_hci->ahb);
    }else{
		DMSG_PANIC("[%s]: wrn: open clock failed, (0x%p, 0x%p, %d, 0x%p)\n",
			      sw_hci->hci_name,sw_hci->ahb,
			      sw_hci->mod_usbphy, sw_hci->clk_is_open,
			      sw_hci->mod_usb);
	}

	return 0;
}

/*
*******************************************************************************
*                     enable_usb_passby
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
static void usb_passby(struct sw_hci_hcd *sw_hci, u32 enable)
{
	unsigned long reg_value = 0;
	spinlock_t lock;
	unsigned long flags = 0;

	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);

	/*enable passby*/
	if(sw_hci->usbc_no == 1){
		reg_value = USBC_Readl(sw_hci->usb_vbase + SW_USB_PMU_IRQ_ENABLE);
		if(enable && usb1_enable_passly_cnt == 0){
	    	reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
	    	reg_value |= (1 << 9);     	/* AHB Master interface burst type INCR4 enable */
	    	reg_value |= (1 << 8);     	/* AHB Master interface INCRX align enable */
	    	reg_value |= (1 << 0);     	/* ULPI bypass enable */
		}else if(!enable && usb1_enable_passly_cnt == 1){
	    	reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
	    	reg_value &= ~(1 << 9);     /* AHB Master interface burst type INCR4 disable */
	    	reg_value &= ~(1 << 8);     /* AHB Master interface INCRX align disable */
	    	reg_value &= ~(1 << 0);     /* ULPI bypass disable */
		}
        USBC_Writel(reg_value, (sw_hci->usb_vbase + SW_USB_PMU_IRQ_ENABLE));

        if(enable){
            usb1_enable_passly_cnt++;
        }else{
            usb1_enable_passly_cnt--;
        }
	}else if(sw_hci->usbc_no == 2){
		reg_value = USBC_Readl(sw_hci->usb_vbase + SW_USB_PMU_IRQ_ENABLE);
		if(enable && usb2_enable_passly_cnt == 0){
	    	reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
	    	reg_value |= (1 << 9);     	/* AHB Master interface burst type INCR4 enable */
	    	reg_value |= (1 << 8);     	/* AHB Master interface INCRX align enable */
	    	reg_value |= (1 << 0);     	/* ULPI bypass enable */
		}else if(!enable && usb2_enable_passly_cnt == 1){
	    	reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
	    	reg_value &= ~(1 << 9);     /* AHB Master interface burst type INCR4 disable */
	    	reg_value &= ~(1 << 8);     /* AHB Master interface INCRX align disable */
	    	reg_value &= ~(1 << 0);     /* ULPI bypass disable */
		}
	    USBC_Writel(reg_value, (sw_hci->usb_vbase + SW_USB_PMU_IRQ_ENABLE));

        if(enable){
            usb2_enable_passly_cnt++;
        }else{
            usb2_enable_passly_cnt--;
        }
    }else if(sw_hci->usbc_no == 3){
		reg_value = USBC_Readl(sw_hci->usb_vbase + SW_USB_PMU_IRQ_ENABLE);
		if(enable && usb3_enable_passly_cnt == 0){
	    	reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
	    	reg_value |= (1 << 9);     	/* AHB Master interface burst type INCR4 enable */
	    	reg_value |= (1 << 8);     	/* AHB Master interface INCRX align enable */
	    	reg_value |= (1 << 0);     	/* ULPI bypass enable */
		}else if(!enable && usb3_enable_passly_cnt == 1){
	    	reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
	    	reg_value &= ~(1 << 9);     /* AHB Master interface burst type INCR4 disable */
	    	reg_value &= ~(1 << 8);     /* AHB Master interface INCRX align disable */
	    	reg_value &= ~(1 << 0);     /* ULPI bypass disable */
		}
	    USBC_Writel(reg_value, (sw_hci->usb_vbase + SW_USB_PMU_IRQ_ENABLE));

        if(enable){
            usb3_enable_passly_cnt++;
        }else{
            usb3_enable_passly_cnt--;
        }
    }else{
		DMSG_PANIC("EER: unkown usbc_no(%d)\n", sw_hci->usbc_no);
		return;
	}

	spin_unlock_irqrestore(&lock, flags);

    return;
}

/*
*******************************************************************************
*                     hci_port_configure
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
static void hci_port_configure(struct sw_hci_hcd *sw_hci, u32 enable)
{
	return;
}

#ifndef  SW_USB_FPGA

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
static int alloc_pin(struct sw_hci_hcd *sw_ehci)
{
    u32 ret = 1;

    if(sw_ehci->drv_vbus_gpio_valid){
    	ret = gpio_request(sw_ehci->drv_vbus_gpio_set.gpio.gpio, NULL);
    	if(ret != 0){
    		DMSG_PANIC("ERR: gpio_request failed\n");
    		sw_ehci->drv_vbus_gpio_valid = 0;
    	}else{
            /* set config, ouput */
            //sw_gpio_setcfg(sw_ehci->drv_vbus_gpio_set.gpio.gpio, 1);

            /* reserved is pull down */
           // sw_gpio_setpull(sw_ehci->drv_vbus_gpio_set.gpio.gpio, 2);
			gpio_direction_output(sw_ehci->drv_vbus_gpio_set.gpio.gpio, 0);			
    	}
	}

    if(sw_ehci->usb_restrict_valid){
		ret = gpio_request(sw_ehci->restrict_gpio_set.gpio.gpio, NULL);
		if(ret != 0){
			DMSG_PANIC("ERR: gpio_request failed\n");
			sw_ehci->usb_restrict_valid = 0;
		}else{
	        /* set config, ouput */
	        //sw_gpio_setcfg(sw_ehci->restrict_gpio_set.gpio.gpio, 1);

	        /* reserved is pull down */
	        //sw_gpio_setpull(sw_ehci->restrict_gpio_set.gpio.gpio, 2);
	        gpio_direction_output(sw_ehci->restrict_gpio_set.gpio.gpio, 0);		
		}
	}
	if(sw_ehci->usb_restrict_valid){
		if(sw_ehci->usb_restrict_flag){
			 __gpio_set_value(sw_ehci->restrict_gpio_set.gpio.gpio, 0);
		}else{
			 __gpio_set_value(sw_ehci->restrict_gpio_set.gpio.gpio, 1);
		}
	}

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
static void free_pin(struct sw_hci_hcd *sw_ehci)
{
    if(sw_ehci->drv_vbus_gpio_valid){
        gpio_free(sw_ehci->drv_vbus_gpio_set.gpio.gpio);
        sw_ehci->drv_vbus_gpio_valid = 0;
    }

	if(sw_ehci->usb_restrict_valid){
        gpio_free(sw_ehci->restrict_gpio_set.gpio.gpio);
        sw_ehci->drv_vbus_gpio_valid = 0;
    }

	return;
}

/*
*******************************************************************************
*                     __sw_set_vbus
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
static void __sw_set_vbus(struct sw_hci_hcd *sw_hci, int is_on)
{
    u32 on_off = 0;

	DMSG_INFO("[%s]: Set USB Power %s\n", sw_hci->hci_name, (is_on ? "ON" : "OFF"));

    /* set power flag */
	sw_hci->power_flag = is_on;

    /* set power */
    if(sw_hci->drv_vbus_gpio_set.gpio.data == 0){
        on_off = is_on ? 1 : 0;
    }else{
        on_off = is_on ? 0 : 1;
    }
	if(sw_hci->drv_vbus_gpio_valid){
   		__gpio_set_value(sw_hci->drv_vbus_gpio_set.gpio.gpio, on_off);
	}

	return;
}

#else

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
static int alloc_pin(struct sw_hci_hcd *sw_ehci)
{
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
static void free_pin(struct sw_hci_hcd *sw_ehci)
{
	return;
}

static void __sw_set_vbus(struct sw_hci_hcd *sw_hci, int is_on)
{
	return;
}

#endif



/*
*******************************************************************************
*                     sw_set_vbus
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
static void sw_set_vbus(struct sw_hci_hcd *sw_hci, int is_on)
{
    DMSG_DEBUG("[%s]: sw_set_vbus cnt %d\n",
              sw_hci->hci_name,
              (sw_hci->usbc_no == 1) ? usb1_set_vbus_cnt : usb2_set_vbus_cnt);

    if(sw_hci->usbc_no == 1){
        if(is_on && usb1_set_vbus_cnt == 0){
            __sw_set_vbus(sw_hci, is_on);  /* power on */
        }else if(!is_on && usb1_set_vbus_cnt == 1){
            __sw_set_vbus(sw_hci, is_on);  /* power off */
        }

        if(is_on){
            usb1_set_vbus_cnt++;
        }else{
            usb1_set_vbus_cnt--;
        }
    }else if(sw_hci->usbc_no == 2){
        if(is_on && usb2_set_vbus_cnt == 0){
            __sw_set_vbus(sw_hci, is_on);  /* power on */
        }else if(!is_on && usb2_set_vbus_cnt == 1){
            __sw_set_vbus(sw_hci, is_on);  /* power off */
        }

        if(is_on){
            usb2_set_vbus_cnt++;
        }else{
            usb2_set_vbus_cnt--;
        }
    }else{
        if(is_on && usb3_set_vbus_cnt == 0){
            __sw_set_vbus(sw_hci, is_on);  /* power on */
        }else if(!is_on && usb3_set_vbus_cnt == 1){
            __sw_set_vbus(sw_hci, is_on);  /* power off */
        }

        if(is_on){
            usb3_set_vbus_cnt++;
        }else{
            usb3_set_vbus_cnt--;
        }
    }

	return;
}

//---------------------------------------------------------------
//  EHCI
//---------------------------------------------------------------

#define  SW_EHCI_NAME		"sw-ehci"
static const char ehci_name[] = SW_EHCI_NAME;

static struct sw_hci_hcd sw_ehci0;
static struct sw_hci_hcd sw_ehci1;

static u64 sw_ehci_dmamask = DMA_BIT_MASK(32);

static struct platform_device sw_usb_ehci_device[] = {
	[0] = {
		.name		= ehci_name,
		.id			= 1,
		.dev 		= {
			.dma_mask			= &sw_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ehci0,
		},
	},

	[1] = {
		.name		= ehci_name,
		.id			= 2,
		.dev 		= {
			.dma_mask			= &sw_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ehci1,
		},
	},
};

//---------------------------------------------------------------
//  OHCI
//---------------------------------------------------------------
#define  SW_OHCI_NAME		"sw-ohci"
static const char ohci_name[] = SW_OHCI_NAME;

static struct sw_hci_hcd sw_ohci0;
static struct sw_hci_hcd sw_ohci1;
static struct sw_hci_hcd sw_ohci2;


static u64 sw_ohci_dmamask = DMA_BIT_MASK(32);

static struct platform_device sw_usb_ohci_device[] = {
	[0] = {
		.name		= ohci_name,
		.id			= 1,
		.dev 		= {
			.dma_mask			= &sw_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ohci0,
		},
	},

	[1] = {
		.name		= ohci_name,
		.id			= 2,
		.dev 		= {
			.dma_mask			= &sw_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ohci1,
		},
	},

	[2] = {
		.name		= ohci_name,
		.id			= 3,
		.dev 		= {
			.dma_mask			= &sw_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ohci2,
		},
	},
};

/*
*******************************************************************************
*                     print_sw_hci
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
static void print_sw_hci(struct sw_hci_hcd *sw_hci)
{
	DMSG_DEBUG("\n------%s config------\n", sw_hci->hci_name);
	DMSG_DEBUG("hci_name             = %s\n", sw_hci->hci_name);
	DMSG_DEBUG("irq_no               = %d\n", sw_hci->irq_no);
	DMSG_DEBUG("usbc_no              = %d\n", sw_hci->usbc_no);

	DMSG_DEBUG("usb_vbase            = 0x%p\n", sw_hci->usb_vbase);
	DMSG_DEBUG("sram_vbase           = 0x%p\n", sw_hci->sram_vbase);
	DMSG_DEBUG("clock_vbase          = 0x%p\n", sw_hci->clock_vbase);
	DMSG_DEBUG("sdram_vbase          = 0x%p\n", sw_hci->sdram_vbase);

	DMSG_DEBUG("used                 = %d\n", sw_hci->used);
	DMSG_DEBUG("host_init_state      = %d\n", sw_hci->host_init_state);

	DMSG_DEBUG("gpio_name            = %s\n", sw_hci->drv_vbus_gpio_set.gpio_name);
	DMSG_DEBUG("port                 = %d\n", sw_hci->drv_vbus_gpio_set.port);
	DMSG_DEBUG("port_num             = %d\n", sw_hci->drv_vbus_gpio_set.port_num);
	DMSG_DEBUG("mul_sel              = %d\n", sw_hci->drv_vbus_gpio_set.mul_sel);
	DMSG_DEBUG("pull                 = %d\n", sw_hci->drv_vbus_gpio_set.pull);
	DMSG_DEBUG("drv_level            = %d\n", sw_hci->drv_vbus_gpio_set.drv_level);
	DMSG_DEBUG("data                 = %d\n", sw_hci->drv_vbus_gpio_set.data);

	DMSG_DEBUG("\n--------------------------\n");

    return;
}

/*
*******************************************************************************
*                     init_sw_hci
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
static int init_sw_hci(struct sw_hci_hcd *sw_hci, u32 usbc_no, u32 ohci, const char *hci_name)
{
    s32 ret = 0;

    memset(sw_hci, 0, sizeof(struct sw_hci_hcd));

    sw_hci->usbc_no         = usbc_no;
    sw_hci->usbc_type = ohci ? SW_USB_OHCI : SW_USB_EHCI;

    if(ohci){
        sw_hci->irq_no = ohci_irq_no[sw_hci->usbc_no];
    }else{
        sw_hci->irq_no = ehci_irq_no[sw_hci->usbc_no];
    }

    sprintf(sw_hci->hci_name, "%s%d", hci_name, sw_hci->usbc_no);

	sw_hci->usb_vbase		= (void __iomem	*)usbc_base[sw_hci->usbc_no];
	sw_hci->sram_vbase		= (void __iomem	*)AW_VIR_SRAMCTRL_BASE;
	sw_hci->clock_vbase     = (void __iomem	*)AW_VIR_CCM_BASE;
	sw_hci->gpio_vbase		= (void __iomem	*)AW_VIR_PIO_BASE;
	sw_hci->sdram_vbase     = (void __iomem	*)AW_VIR_SDMMC1_BASE;

	get_usb_cfg(sw_hci);
	sw_hci->open_clock          = open_clock;
	sw_hci->close_clock         = close_clock;
	sw_hci->set_power           = sw_set_vbus;
	sw_hci->usb_passby          = usb_passby;
    sw_hci->port_configure      = hci_port_configure;

#ifdef  SW_USB_FPGA
{
    u32 reg_value = 0;

	reg_value = USBC_Readl(sw_hci->sram_vbase+ 0x04);
	reg_value &= ~(0x01);
	USBC_Writel(reg_value, (sw_hci->sram_vbase+ 0x04));
}
#endif

    ret = clock_init(sw_hci, ohci);
    if(ret != 0){
        DMSG_PANIC("ERR: clock_init failed\n");
        goto failed1;
    }

    print_sw_hci(sw_hci);

    return 0;

failed1:

    return -1;
}

/*
*******************************************************************************
*                     exit_sw_hci
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
static int exit_sw_hci(struct sw_hci_hcd *sw_hci, u32 ohci)
{
    clock_exit(sw_hci, ohci);

    return 0;
}

/*
*******************************************************************************
*                     sw_hci_sun6i_init
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

static int __init sw_hci_sun6i_init(void)
{

    /* USB1 */
    init_sw_hci(&sw_ehci0, 1, 0, ehci_name);
    init_sw_hci(&sw_ohci0, 1, 1, ohci_name);
    alloc_pin(&sw_ehci0);

    /* USB2 */
    init_sw_hci(&sw_ehci1, 2, 0, ehci_name);
    init_sw_hci(&sw_ohci1, 2, 1, ohci_name);
    alloc_pin(&sw_ehci1);

	/* USB3 */
    //init_sw_hci(&sw_ohci2, 3, 1, ohci_name);
    //alloc_pin(&sw_ohci2);


#ifdef  CONFIG_USB_SW_SUN6I_EHCI0
    if(sw_ehci0.used){
    	platform_device_register(&sw_usb_ehci_device[0]);
    }else{
		DMSG_INFO("ERR: usb%d %s is not enable\n", sw_ehci0.usbc_no, sw_ehci0.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_OHCI0
    if(sw_ohci0.used){
  	    platform_device_register(&sw_usb_ohci_device[0]);
    }else{
		DMSG_INFO("ERR: usb%d %s is not enable\n", sw_ohci0.usbc_no, sw_ohci0.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_EHCI1
    if(sw_ehci1.used){
     	platform_device_register(&sw_usb_ehci_device[1]);
    }else{
		DMSG_INFO("ERR: usb%d %s is not enable\n", sw_ehci1.usbc_no, sw_ehci1.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_OHCI1
    if(sw_ohci1.used){
     	platform_device_register(&sw_usb_ohci_device[1]);
    }else{
		DMSG_INFO("ERR: usb%d %s is not enable\n", sw_ohci1.usbc_no, sw_ohci1.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_OHCI2
		if(sw_ohci2.used){
			platform_device_register(&sw_usb_ohci_device[2]);
		}else{
			DMSG_INFO("ERR: usb%d %s is not enable\n", sw_ohci2.usbc_no, sw_ohci2.hci_name);
		}
#endif

    return 0;
}

/*
*******************************************************************************
*                     sw_hci_sun6i_exit
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
static void __exit sw_hci_sun6i_exit(void)
{

#ifdef  CONFIG_USB_SW_SUN6I_EHCI0
    if(sw_ehci0.used){
    	platform_device_unregister(&sw_usb_ehci_device[0]);
    }else{
		DMSG_PANIC("ERR: usb%d %s is disable\n", sw_ehci0.usbc_no, sw_ehci0.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_OHCI0
    if(sw_ohci0.used){
  	    platform_device_unregister(&sw_usb_ohci_device[0]);
    }else{
		DMSG_PANIC("ERR: usb%d %s is disable\n", sw_ohci0.usbc_no, sw_ohci0.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_EHCI1
    if(sw_ehci1.used){
     	platform_device_unregister(&sw_usb_ehci_device[1]);
    }else{
		DMSG_PANIC("ERR: usb%d %s is disable\n", sw_ehci1.usbc_no, sw_ehci1.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_OHCI1
    if(sw_ohci1.used){
     	platform_device_unregister(&sw_usb_ohci_device[1]);
    }else{
		DMSG_PANIC("ERR: usb%d %s is disable\n", sw_ohci1.usbc_no, sw_ohci1.hci_name);
    }
#endif

#ifdef  CONFIG_USB_SW_SUN6I_OHCI2
	if(sw_ohci2.used){
		platform_device_unregister(&sw_usb_ohci_device[2]);
	}else{
		DMSG_PANIC("ERR: usb%d %s is disable\n", sw_ohci2.usbc_no, sw_ohci2.hci_name);
	}
#endif

    /* USB1 */
    exit_sw_hci(&sw_ehci0, 0);
    exit_sw_hci(&sw_ohci0, 1);
   	free_pin(&sw_ehci0);

    /* USB2 */
    exit_sw_hci(&sw_ehci1, 0);
    exit_sw_hci(&sw_ohci1, 1);
   	free_pin(&sw_ehci1);

    /*USB3 */
    //exit_sw_hci(&sw_ohci2, 1);
   	//free_pin(&sw_ohci2);

    return ;
}

//fs_initcall(sw_hci_sun6i_init);
module_init(sw_hci_sun6i_init);
module_exit(sw_hci_sun6i_exit);

