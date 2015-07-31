/*
* (C) Copyright 2007-2013
* Allwinner Technology Co., Ltd. <www.allwinnertech.com>
* Martin zheng <zhengjiewen@allwinnertech.com>
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
#include <asm/arch/key.h>
#include <linux/types.h>
#include <asm/arch/cpu.h>
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
__s32 boot_key_get_status(void)
{
    __u32 reg_val;

    struct sunxi_lradc *sunxi_key_base = (struct sunxi_lradc *)SUNXI_LRADC_BASE;
    reg_val = sunxi_key_base->ints;
    if(reg_val & (1 << 1))     //�ж��Ƿ��ǵ�һ�ΰ���
    {
        if(reg_val & (1 << 0))  //�ǣ����жϰ����Ƿ��㹻
        {
            sunxi_key_base->ints |= (reg_val & 0x1f);//����ʱ���㹻��������Ϊ�����Ϸ�
            return 1;
        }
                               //�񣬰���ʱ�䲻�������������������
    }
    else if(reg_val & (1 << 0))//��ʾ���ǵ�һ�ΰ��£�ֱ�������pengding���������������
    {
        sunxi_key_base->ints |= (1 << 0);

        return 0;              //�����ظ���
    }
    //û���κΰ�������
    return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
__s32 boot_key_get_value(void)
{
    __u32 reg_val;
    __u32 key_val;

    struct sunxi_lradc *sunxi_key_base = (struct sunxi_lradc *)SUNXI_LRADC_BASE;
    reg_val = sunxi_key_base->ints;
    if(reg_val & (1 << 1))     //�ж��Ƿ��ǵ�һ�ΰ���
    {
        if(reg_val & (1 << 0))  //�ǣ����жϰ����Ƿ��㹻
        {
            sunxi_key_base->ints |= (reg_val & 0x1f);//����ʱ���㹻��������Ϊ�����Ϸ�
            key_val = sunxi_key_base->data0 & 0x3f;
            return key_val;
        }
                               //�񣬰���ʱ�䲻�������������������
    }
    else if(reg_val & (1 << 0))//��ʾ���ǵ�һ�ΰ��£�ֱ�������pengding���������������
    {
        sunxi_key_base->ints |= (1 << 0);
        key_val = sunxi_key_base->data0 & 0x3f;

        return key_val;              //�����ظ���
    }
    //û���κΰ�������
    return -1;
}



