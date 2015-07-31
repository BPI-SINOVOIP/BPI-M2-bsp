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

#include <common.h>
#include <malloc.h>
#include <bat.h>
#include <asm/arch/drv_display.h>
#include "bat_cartoon.h"
#include "de.h"

DECLARE_GLOBAL_DATA_PTR;

sunxi_bmp_store_t  bat_bmp_store[SUNXI_BAT_BMP_MAX];
static  int bat_catoon_has_init = 0;
static  int pre_bat_cal = 0;
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
int battery_charge_cartoon_init(int rate)
{
	int   i;
	char  filename[32];
	char *const bat_pic_name[SUNXI_BAT_BMP_MAX + 1] = {"bat//bat0.bmp", "bat//bat1.bmp", "bat//bat2.bmp", "bat//bat3.bmp",
													  "bat//bat4.bmp", "bat//bat5.bmp", "bat//bat6.bmp", "bat//bat7.bmp",
													  "bat//bat8.bmp", "bat//bat9.bmp", "bat//bat10.bmp", NULL};
    char *const bmp_argv[6] = { "fatload", "sunxi_flash", "0", "0x40000000", filename, NULL };

    memset(bat_bmp_store, 0, sizeof(sunxi_bmp_store_t) * SUNXI_BAT_BMP_MAX);
	for(i=0; i < SUNXI_BAT_BMP_MAX; i++)
	{
		memset(filename, 0, 32);
		strcpy(filename, bat_pic_name[i]);
	    if(do_fat_fsload(0, 0, 5, bmp_argv))
		{
		   tick_printf("sunxi bmp info error : unable to open bmp file %s\n", bat_pic_name[i]);

		   return -1;
	    }
		bat_bmp_store[i].buffer = malloc(1024 * 1024);
		if(!bat_bmp_store[i].buffer)
		{
			tick_printf("cartoon init fail: cant malloc memory for bat %d \n", i);

			return -2;
		}
	    if(sunxi_bmp_decode(0x40000000, &bat_bmp_store[i]))
	    {
	    	tick_printf("cartoon init fail: unable to decode %s\n", filename);

	    	return -3;
	    }
	}
	//���ò���
	if(board_display_framebuffer_set(bat_bmp_store[rate].x, bat_bmp_store[rate].y, bat_bmp_store[rate].bit, (void *)bat_bmp_store[rate].buffer))
	{
		tick_printf("cartoon init fail: set frame buffer error\n");

		return -4;
	}
	//��ʾͼƬ
	board_display_show_until_lcd_open(0);
	bat_catoon_has_init = 1;
	pre_bat_cal = 0;

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
int battery_charge_cartoon_exit(void)
{
	int i;

    if(bat_catoon_has_init)
    {
		board_display_layer_close();
    }
	for(i=0;i<SUNXI_BAT_BMP_MAX;i++)
	{
		if(bat_bmp_store[i].buffer)
		{
			free(bat_bmp_store[i].buffer);
			bat_bmp_store[i].buffer = NULL;
		}
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
int battery_charge_cartoon_rate(int rate)
{
	if(bat_catoon_has_init)
	{
		if(pre_bat_cal != rate)
		{
			flush_cache((uint)bat_bmp_store[rate].buffer, bat_bmp_store[rate].x * bat_bmp_store[rate].y * bat_bmp_store[rate].bit/4);
			board_display_framebuffer_change(bat_bmp_store[rate].buffer);
			pre_bat_cal = rate;
		}
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
int battery_charge_cartoon_reset(void)
{
	__disp_layer_info_t *layer_para;

	layer_para = (__disp_layer_info_t *)gd->layer_para;

    layer_para->alpha_en = 1;
    layer_para->alpha_val = 255;
    __msdelay(50);
    board_display_layer_para_set();

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
int battery_charge_cartoon_degrade(int alpha_step)
{
	int  alpha, delay_time;
	__disp_layer_info_t *layer_para;

	layer_para = (__disp_layer_info_t *)gd->layer_para;

	delay_time = 50;
	alpha = layer_para->alpha_val;

	layer_para->alpha_en = 1;
	alpha -= alpha_step;
	if(alpha > 0)
	{
		board_display_layer_para_set();
		__msdelay(delay_time);
		layer_para->alpha_val = alpha;
	}
	else
	{
		return -1;
	}

	return 0;
}


