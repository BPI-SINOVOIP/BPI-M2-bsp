/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
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
#ifndef  __UI_CHAR_I_H__
#define  __UI_CHAR_I_H__

#include <asm/types.h>
#include <asm/arch/drv_display.h>
#include "../sprite_cartoon_i.h"

typedef struct ui_char_info_t
{
    char   *crt_addr;                         //��ǰ������ʾ�ĵ�ַ
    __u32   rest_screen_height;               //ʣ��Ĵ洢��Ļ�߶ȣ�ʣ���ܸ߶�, �ַ���λ����
    __u32   rest_screen_width;                //ʣ��Ĵ洢��Ļ���, ʣ���ܿ��, �ַ���λ����
    __u32   rest_display_height;              //ʣ�����ʾ�߶�
    __u32   total_height;                     //������ʾ�ܵĸ߶�
    __u32   current_height;                   //��ǰ�Ѿ�ʹ�õĸ߶�
    __u32   x;                                //��ʾλ�õ�x����
    __u32   y;                                //��ʾλ�õ�y����
    int     word_size;						  //�ַ���С
}
_ui_char_info_t;


extern  sprite_cartoon_source  sprite_source;


#endif   //__UI_CHAR_I_H__

