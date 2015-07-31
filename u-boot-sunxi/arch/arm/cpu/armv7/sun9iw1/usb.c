/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Yana <yangnaitiand@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/arch/ccmu.h>
#include <asm/arch/timer.h>
#include <asm/arch/usb.h>
#include <asm/io.h>

static s32 SUSB_Send_Dev_EP_CMD(unsigned ep, unsigned cmd, struct susb_gadget_ep_cmd_params *params);
/*
***********************************************************************************
*                     SUSB_Dev_Stop_Active_Transfer
*
* Description:
*    �����ǰacitve��EP
*
* Arguments:
*
*
* Returns:
*
* note:
*    ��
***********************************************************************************
*/
void SUSB_Dev_Stop_Active_Transfer(void)
{
    return;
}
/*
***********************************************************************************
*                     SUSB_Dev_Stop_All_Stall_Ep
*
* Description:
*    ֹͣ����stall endpoint
*
* Arguments:
*
*
* Returns:
*
* note:
*    ��
***********************************************************************************
*/
void SUSB_Dev_Stop_All_Stall_Ep(void)
{
    return;
}
/*
***********************************************************************************
*                     SUSB_Dev_SetAddress
*
* Description:
*    ���host��device����ĵ�ַ
*
* Arguments:
*    hUSB       :  input.  USBC_open_otg��õľ��, ��¼��USBC����Ҫ��һЩ�ؼ�����
*
* Returns:
*
*
* note:
*    ��
*
***********************************************************************************
*/
void SUSB_Dev_SetAddress(int address)
{
    __u32 reg_val;

	reg_val = readl(SUSB_DCFG);
	reg_val &= ~(0x07f<<3);
	reg_val |= (address<<3);
	writel(reg_val, SUSB_DCFG);

	return;
}
/*
***********************************************************************************
*                     SUSB_Dev_Test_Mode_Cofig
*
* Description:
*    ����device����normal����ģʽ������Ӧ��Testģʽ
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg��õľ��, ��¼��USBC����Ҫ��һЩ�ؼ�����
*    is_Normal_Test :  input.  000b:Test mode disable; 0001b: Test_J; 010b: Test_k
*                              011b:Test_SE0_NAK; 100b:Test_Pack; 101b:Test_Force
***********************************************************************************
*/
void SUSB_Dev_Test_Mode_Cofig(uint is_Normal_Test)
{
	__u32 reg_val;

	reg_val = readl(SUSB_DCTL);
	reg_val &= ~SUSB_DCTL_TSTCTRL_MASK;
	reg_val |= ((is_Normal_Test&0xf)<<1);
	writel(reg_val, SUSB_DCTL);
}
/*
***********************************************************************************
*                     SUSBC_Dev_ConectSwitch
*
* Description:
*    ��PCͨ�ŵĿ���
*
* Arguments:
*    hUSB   :  input.  USBC_open_otg��õľ��, ��¼��USBC����Ҫ��һЩ�ؼ�����
*    is_on  :  input.  1: �򿪺�PCͨ�ſ���. 0: �رպ�PCͨ��ͨ��
*
* Returns:
*
*
* note:
*    ��
*
***********************************************************************************
*/
void SUSB_Dev_ConectSwitch(__u32 is_on)
{
	__u32 reg_val;

	reg_val = readl(SUSB_DCTL);
	if(is_on == USBC_DEVICE_SWITCH_ON)
	{
		reg_val |= SUSB_DEV_RUN_STOP;
	}
	else
	{
		reg_val &= ~SUSB_DEV_RUN_STOP;
	}
	writel(reg_val, SUSB_DCTL);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/* Config Role Mode */
static void __SUSBC_ForceRoleToHost(void)
{
	__u32 reg_val = 0;

	reg_val = readl(SUSB_GCTL);
	reg_val &= ~(0x03<<12);
	reg_val |= (0x01<<12);
	writel(reg_val, SUSB_GCTL);

	while(((readl(SUSB_GCTL)>>12)&0x03) != 0x01);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void __SUSBC_DisableToOTG(void)
{
	__u32 reg_val = 0;

	reg_val = readl(SUSB_GCTL);
	reg_val &= ~(0x03<<12);
	reg_val |= (0x03<<12);
	writel(reg_val, SUSB_GCTL);

	while(((readl(SUSB_GCTL)>>12)&0x03) != 0x03);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void __SUSBC_ForceRoleToDevice(void)
{
	__u32 reg_val = 0;

	reg_val = readl(SUSB_GCTL);
	reg_val &= ~(0x03<<12);
	reg_val |= (0x02<<12);
	writel(reg_val, SUSB_GCTL);

	while(((readl(SUSB_GCTL)>>12)&0x03) != 0x02);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void SUSB_Force_Role(__u32 id_type)
{
    switch(id_type)
    {
		case USBC_ID_TYPE_HOST:
			__SUSBC_ForceRoleToHost();
			break;

		case USBC_ID_TYPE_DEVICE:
			__SUSBC_ForceRoleToDevice();
			break;

		default:
			__SUSBC_DisableToOTG();
			break;
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/* ��ǿusb�����ź� */
void SUSB_EnhanceSignal(void)
{
	return;
}
/*
 ***************************************************************************
 *
 * ѡ�� usb device ���ٶ����͡��� �����١����١�ȫ�١����١�
 *
 ***************************************************************************
 */
 static void __SUSBC_Dev_TsMode_Hs(void)
 {
 	__u32 reg_val = 0;

 	reg_val = readl(SUSB_DCFG);
   	reg_val &= ~(0x07<<0);
   	writel(reg_val, SUSB_DCFG);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void __SUSBC_Dev_TsMode_Fs(void)
{
	__u32 reg_val = 0;

	reg_val = readl(SUSB_DCFG);
	reg_val &= ~(0x07<<0);
	reg_val |= (0x01<<0);
	writel(reg_val, SUSB_DCFG);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void __SUSBC_Dev_TsMode_default(void)
{
	__u32 reg_val = 0;

	reg_val = readl(SUSB_DCFG);
	reg_val &= ~(0x07<<0);
	reg_val |= (0x04<<0);
	writel(reg_val, SUSB_DCFG);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void __SUSBC_Dev_TsMode_Ss(void)
{
	__u32 reg_val = 0;

	reg_val = readl(SUSB_DCFG);
	reg_val &= ~(0x07<<0);
	reg_val |= (0x04<<0);
	writel(reg_val, SUSB_DCFG);
}
/*
***********************************************************************************
*                     SUSBC_Dev_ConfigTransferMode
*
* Description:
*    ����device�Ĵ������ͺ��ٶ�ģʽ
*
* Arguments:
*    hUSB       :  input.  USBC_open_otg��õľ��, ��¼��USBC����Ҫ��һЩ�ؼ�����
*    speed_mode :  input.  �ٶ�ģʽ
*
* Returns:
*
*
* note:
*    ��
*
***********************************************************************************
*/
void SUSB_Dev_ConfigTransferMode(__u32 speed_mode)
{
     //--ѡ�����ٶ�
    switch(speed_mode)
    {
		case USBC_TS_MODE_HS:
			__SUSBC_Dev_TsMode_Hs();
			break;

		case USBC_TS_MODE_FS:
			__SUSBC_Dev_TsMode_Fs();
			break;

		case USBC_TS_MODE_SS:
			__SUSBC_Dev_TsMode_Ss();
			break;

		default:
			__SUSBC_Dev_TsMode_default();
			break;
	}
}

/*
***********************************************************************************
*                     SUSB_Dev_EnaDisa_Ep_Function
*
* Description:
*    ʹ����Ӧ��physical endpoint function
*
* Arguments:
*    hUSB       :  input.  USBC_open_otg��õľ��, ��¼��USBC����Ҫ��һЩ�ؼ�����
*    is_Ena     :  1: Enable ep,  0: Disable ep
*    bit_map    :  endpoint bit_map
*
* note:
*    ��
***********************************************************************************
*/
void SUSB_Dev_EnaDisa_Ep_Function(__u32 is_Ena, __u32 bit_map)
{
	 __u32 reg_val = 0;

	if(bit_map>31)
	{
		return;
	}

	reg_val = readl(SUSB_DALEPENA);
	if(is_Ena)
	{
		reg_val |= 1<<bit_map;
	}
	else
	{
		reg_val &= ~(1<<bit_map);
	}
	writel(reg_val, SUSB_DALEPENA);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/*Enable USB PHY*/
void SUSB_Dev_PHY_Config(__u32 is_On, __u8 speed_mode)
{
	__u32 temp = 0;
	__u32 reg = 0;
	__u32 enable_bit = 0;

	if(speed_mode == USBC_TS_MODE_SS)
	{
		reg = SUSB_GUSB3PIPECTL(0);
		enable_bit = SUSB_GUSB2PHYCFG_SUSPHY;
	}
	else
	{
		reg = SUSB_GUSB2PHYCFG(0);
		enable_bit = SUSB_GUSB3PIPECTL_SUSPHY;
	}

	temp = readl(reg);
	if(is_On)
	{
		temp &= ~enable_bit;
	}
	else
	{
		temp |= enable_bit;
	}
	writel(temp, reg);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/*Config Device FIFO SIZE*/
void SUSB_Config_Dev_Size(void)
{
	return;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/**
*
*SUSB_Dev_Core_Rest - Issues core soft reset and PHY reset
*
*@hUSB:USBC_open_otg��õľ��, ��¼��USBC����Ҫ��һЩ�ؼ�����
*
*/
void SUSB_Dev_Core_Rest(void)
{
 	__u32 reg = 0;

 	/*Put Core in Reset*/
	reg = readl(SUSB_GCTL);
	reg |= SUSB_GCTL_CORESOFTRESET;
	writel(reg, SUSB_GCTL);

 	/*Assert USB3 PHY reset*/
	reg = readl(SUSB_GUSB3PIPECTL(0));
	reg |= SUSB_GUSB3PIPECTL_PHYSOFTRST;
	writel(reg, SUSB_GUSB3PIPECTL(0));

 	/*Assert USB2 PHY reset*/
	reg = readl(SUSB_GUSB2PHYCFG(0));
	reg |= SUSB_GUSB2PHYCFG_PHYSOFTRST;
	writel(reg, SUSB_GUSB2PHYCFG(0));

  	__msdelay(10);

 	/*Clear USB3 PHY reset*/
	reg = readl(SUSB_GUSB3PIPECTL(0));
	reg &= ~SUSB_GUSB3PIPECTL_PHYSOFTRST;
	writel(reg, SUSB_GUSB3PIPECTL(0));

 	/*Clear USB2 PHY reset*/
	reg = readl(SUSB_GUSB2PHYCFG(0));
	reg &= ~SUSB_GUSB2PHYCFG_PHYSOFTRST;
	writel(reg, SUSB_GUSB2PHYCFG(0));

 	/*Clear the Core Reset*/
	reg = readl(SUSB_GCTL);
	reg &= ~SUSB_GCTL_CORESOFTRESET;
	writel(reg, SUSB_GCTL);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void SUSB_Event_Buffers_Setup(struct susb_event_buffer *event_buffer)
{
	struct susb_event_buffer	*evt;
	u32 i, num;

	num = SUSB_Get_EvtCount();
	evt = event_buffer;

	for(i=0; i<num; i++)
	{
		writel((uint)evt->buf_start_l, SUSB_GEVNTADRLO(i));
		writel(evt->length & 0xffff,   SUSB_GEVNTSIZ(i));
		writel(0, SUSB_GEVNTCOUNT(i));
		evt->lpos = 0;
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/**
 * SUSB_Event_Buffers_Setup - setup our allocated event buffers
 *
 * @usbc_base_addr:��¼��susb��basic address
 *
 * @phy_ep:the physical endpoint number that will receive the command
 *
 * @cmd:command type
 *
 * @*params:the params for this command
 */
static s32 SUSB_Send_Dev_EP_CMD(unsigned ep, unsigned cmd, struct susb_gadget_ep_cmd_params *params)
{
	__u32 reg;
	//__u32 timeout = 50000;
	//__u32 i;
	writel(params->param0, SUSB_DEPCMDPAR0(ep));
	writel(params->param1, SUSB_DEPCMDPAR1(ep));
	writel(params->param2, SUSB_DEPCMDPAR2(ep));

	while((readl(SUSB_DEPCMD(ep))&(0x01<<10)));//make sure last cmd complete
	writel(cmd | (0x01<<10),SUSB_DEPCMD(ep));
	do
	{
		reg = readl(SUSB_DEPCMD(ep));
		if(!(reg&(0x01<<10))) return 0;

		//timeout --;
		//if(!timeout)
		//{
		//	return (-1);
		//}
		//for(i=0;i<0x500;i++);
	}
	while(1);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :  the physical endpoint number that will receive the command
*					  1-A intterupt will issuce when the command complete otherwise not issuce
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
s32 SUSB_Ep_CMD_Implement(susb_mem_cfg usb_cfg, __u32 p_trb, unsigned phy_ep, unsigned cmd, unsigned is_IOC)
{
	struct susb_gadget_ep_cmd_params params;

	memset(&params, 0x00, sizeof(params));

	switch(cmd)
	{
		case SUSB_DEPCMD_SETEPCONFIG:
		{
			params.param1 = ((0x01<<10)|(0x01<<8));
			if(phy_ep == 1)
			{
			 	params.param1 |= (0x01<<25);
				params.param0 = ((0x0<<22)|(0x00<<17)|((usb_cfg.ep0_maxpkt&0x7ff)<<3)|(0x00<<1));
			}
			else if(phy_ep == usb_cfg.phy_out_ep)
			{
				params.param1 &= ~(0x01<<25);
				params.param1 |= (((phy_ep>>1)&0x0f)<<26);
				params.param0 = ((0x0<<22)|(0x00<<17)|((usb_cfg.bulk_ep_max&0x7ff)<<3)|(0x02<<1));
			}
			else if(phy_ep == usb_cfg.phy_in_ep)
			{
				params.param1 |= (0x01<<25);
				params.param1 |= (((phy_ep>>1)&0x0f)<<26);
				params.param0 = ((0x0<<22)|(1<<17)|((usb_cfg.bulk_ep_max&0x7ff)<<3)|(0x02<<1));
			}
			else
			{
				params.param1 &= ~(0x1f<<25);
				params.param0 = ((0x0<<22)|(0x00<<17)|((usb_cfg.ep0_maxpkt&0x7ff)<<3)|(0x00<<1));
			}

			cmd = SUSB_DEPCMD_SETEPCONFIG;
			break;
		}

		case SUSB_DEPCMD_SETTRANSFRESOURCE:
		{
		 	params.param0 = 1;
		 	cmd = (SUSB_DEPCMD_SETTRANSFRESOURCE & 0x0f);
			break;
		}

		case SUSB_DEPCMD_GETSEQNUMBER:
			break;

		case SUSB_DEPCMD_SETSTALL:
			break;

		case SUSB_DEPCMD_CLEARSTALL:
			break;

		case SUSB_DEPCMD_STARTTRANSFER:
		{
			pSUSBTRB tmp_trb = (pSUSBTRB)p_trb;

      		params.param1 = p_trb;
      		cmd = (SUSB_DEPCMD_STARTTRANSFER & 0x0f);

			flush_cache(tmp_trb->bpl, tmp_trb->size);

			break;
		}

		case SUSB_DEPCMD_UPDATETRANSFER:
			break;

		case SUSB_DEPCMD_ENDTRANSFER:
		{
			cmd = (SUSB_DEPCMD_ENDTRANSFER & 0x0f);
			break;
		}

		case SUSB_DEPCMD_DEPSTARTCFG:
		  	cmd = (SUSB_DEPCMD_DEPSTARTCFG & 0x0f);
		  	cmd |= ((usb_cfg.xfer_res & 0x7f)<<16);

			break;

		default:
			break;
	}
	if(is_IOC)
	{
		cmd |= (0x01<<8);
	}

	if(SUSB_Send_Dev_EP_CMD(phy_ep,cmd,&params))
	{
	  	return (-1);
	}

  	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void SUSB_Dev_Interrupt_Ctl(__u32 Ena_Val)
{
  	writel(Ena_Val, SUSB_DEVTEN);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
__u32 SUSB_Dev_Log_to_Physical_Ep(int logi_ep, int is_Out)
{
	__u8 phys_ep;

	if(is_Out)
	{
	 	phys_ep = (logi_ep<<1);
	}
	else
	{
	 	phys_ep = ((logi_ep<<1)+1);
	}

	return phys_ep;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
uint SUSB_Dev_GetEvtCount(int evt_index)
{
	return readl(SUSB_GEVNTCOUNT(evt_index));
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void SUSB_Dev_SetEvtCount(uint value, int evt_index)
{
	writel(value, SUSB_GEVNTCOUNT(evt_index));
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
uint SUSB_Get_Speed_Mode(void)
{
	uint reg_val;

	reg_val = readl(SUSB_DCFG);

	return (reg_val & 0x07);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
uint SUSB_Get_EvtCount(void)
{
	return ((readl(SUSB_GHWPARAMS1)>>15) & 0x3f);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void SUSB_Set_Epmap_Enable(int phy_ep_no)
{
	uint reg_value;

	reg_value = readl(SUSB_DALEPENA);
	reg_value |= (0x01<<phy_ep_no);
	writel(reg_value, SUSB_DALEPENA);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int usb_open_clock(void)
{
	uint reg_val;
#ifndef CONFIG_A39_FPGA
	int  i;
#endif

#if defined(CONFIG_A39_FPGA)
	reg_val = readl(0x900000 + 0xc200);
	reg_val |= (0x01<<4);
	writel(reg_val, 0x900000 + 0xc200);

	__usdelay(1);

	reg_val = readl(CCM_AHB1_RST_REG0);
	reg_val |= (0x01<<0);
	writel(reg_val, CCM_AHB1_RST_REG0);

	__usdelay(1);

	reg_val = readl(0x900000+0x10020);
	reg_val |= (0x03<<1); //Use extern vbus to phy
	reg_val |= (0x01<<24); //SSC_EN
	reg_val |= (0x01<<26); //REF_SSP_EN
	reg_val |= (0x01<<27); //Vbusvalid Force
	writel(reg_val, 0x900000+0x10020);
#else
	//OTG0 Phy release
	reg_val = readl(CCM_AHB1_RST_REG0);
	reg_val &= ~(0x01<<1);
	writel(reg_val, CCM_AHB1_RST_REG0);

	reg_val = readl(CCM_USHPHY0_SCLK_CTRL);
	reg_val |= (0x01<<31);
	writel(reg_val, CCM_USHPHY0_SCLK_CTRL);

	__usdelay(1);
	reg_val = readl(CCM_AHB1_RST_REG0);
	reg_val |= (0x01<<1);
	writel(reg_val, CCM_AHB1_RST_REG0);

	__usdelay(1);
	//OTG0 bus release
	reg_val = readl(CCM_AHB1_RST_REG0);
	reg_val &= ~(0x01<<0);
	writel(reg_val, CCM_AHB1_RST_REG0);

	reg_val = readl(CCM_AHB1_GATE0_CTRL);
	reg_val |= (0x01<<0);
	writel(reg_val, CCM_AHB1_GATE0_CTRL);

	__usdelay(1);
	reg_val = readl(CCM_AHB1_RST_REG0);
	__usdelay(1);
	writel(reg_val, CCM_AHB1_RST_REG0);

	//Controller Config
	reg_val = readl(0x900000+0x10020);
	reg_val |= (0x01<<24); //SSC_EN
	reg_val |= (0x01<<26); //REF_SSP_EN
	writel(reg_val, 0x900000+0x10020);
	writel(readl(0x900000+0x10014)|(0x01<<6), 0x900000+0x10014);

	reg_val = readl(0x900000+0x10000);
	reg_val |= (0x03<<12);
	writel(reg_val, 0x900000+0x10000);

	reg_val = readl(0x900000+0x10010);
	reg_val |= (0x01<<20);
	writel(reg_val, 0x900000+0x10010);
#endif
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int usb_close_clock(void)
{
	uint reg_val;

#ifndef CONFIG_A39_FPGA
	reg_val = readl(CCM_AHB1_RST_REG0);
	reg_val &= ~(0x01<<0);
	writel(reg_val, CCM_AHB1_RST_REG0);
#endif

	reg_val = readl(CCM_AHB1_RST_REG0);
	reg_val &= ~(0x01<<1);
	writel(reg_val, CCM_AHB1_RST_REG0);

	reg_val = readl(CCM_AHB1_GATE0_CTRL);
	reg_val &= ~(0x01<<0);
	writel(reg_val, CCM_AHB1_GATE0_CTRL);

	reg_val = readl(CCM_USHPHY0_SCLK_CTRL);
	reg_val &= ~(0x01<<31);
	writel(reg_val, CCM_USHPHY0_SCLK_CTRL);

	return 0;
}



