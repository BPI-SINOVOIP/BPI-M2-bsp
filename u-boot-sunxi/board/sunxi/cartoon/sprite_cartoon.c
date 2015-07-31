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

#include "sprite_cartoon.h"
#include "sprite_cartoon_i.h"
#include "sprite_cartoon_color.h"
#include <asm/arch/drv_display.h>
#include <sunxi_board.h>

sprite_cartoon_source  sprite_source;
static uint  progressbar_hd;
static int   last_rate;

/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sprite_cartoon_screen_set
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
int sprite_cartoon_screen_set(void)
{
	uint arg[4] = { 0 };
	/* ��ʼ��ͼ�β��� */
	sprite_source.screen_width  = disp_ioctl(NULL, DISP_CMD_SCN_GET_WIDTH, (void*)arg);
	sprite_source.screen_height = disp_ioctl(NULL, DISP_CMD_SCN_GET_HEIGHT, (void*)arg);
	if((sprite_source.screen_width < 40) || (sprite_source.screen_height < 40))
	{
		printf("sunxi cartoon error: invalid screen width or height\n");

		return -1;
	}
	sprite_source.screen_size   = sprite_source.screen_width * sprite_source.screen_height * 4;
    sprite_source.screen_buf 	= malloc(sprite_source.screen_size);
	sprite_source.color         = SPRITE_CARTOON_GUI_GREEN;

    if(!sprite_source.screen_buf)
    {
        return -1;
    }
    memset(sprite_source.screen_buf, 0, sprite_source.screen_size);

	board_display_framebuffer_set(sprite_source.screen_width, sprite_source.screen_height, 32, (void *)sprite_source.screen_buf);

    return 0;
}


/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sprite_cartoon_screen_set
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
int sprite_cartoon_test(void)
{
	int i;
	uint progressbar_hd;
	int screen_width, screen_height;
	uint arg[4] = { 0 };
	int x1, x2, y1, y2;

	sprite_cartoon_screen_set();
	board_display_show_until_lcd_open(0);

	screen_width  = disp_ioctl(NULL, DISP_CMD_SCN_GET_WIDTH, (void*)arg);
	screen_height = disp_ioctl(NULL, DISP_CMD_SCN_GET_HEIGHT, (void*)arg);

	printf("screen_width = %d\n", screen_width);
    printf("screen_height = %d\n", screen_height);

	x1 = screen_width/4;
	x2 = x1 * 3;

	y1 = screen_height/2 - 40;
	y2 = screen_height/2 + 40;

	printf("bar x1: %d y1: %d\n", x1, y1);
	printf("bar x2: %d y2: %d\n", x2, y2);

	progressbar_hd = sprite_cartoon_progressbar_create(x1, y1, x2, y2);
	sprite_cartoon_progressbar_config(progressbar_hd, SPRITE_CARTOON_GUI_RED, SPRITE_CARTOON_GUI_GREEN, 2);
	sprite_cartoon_progressbar_active(progressbar_hd);

	sprite_uichar_init(24);
	sprite_uichar_printf("this is for test\n");

	sprite_uichar_printf("bar x1: %d y1: %d\n", x1, y1);
	sprite_uichar_printf("bar x2: %d y2: %d\n", x2, y2);

	do
	{
		for(i=0;i<100;i+=50)
		{
			sprite_cartoon_progressbar_upgrate(progressbar_hd, i);
			__msdelay(500);
			sprite_uichar_printf("here %d\n", i);
		}

		sprite_uichar_printf("up %d\n", i);
		for(i=99;i>0;i-=50)
		{
			sprite_cartoon_progressbar_upgrate(progressbar_hd, i);
			__msdelay(500);
		}
		sprite_uichar_printf("down %d\n", i);
	}

	while(0);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sprite_cartoon_start
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
uint sprite_cartoon_create(void)
{
	int screen_width, screen_height;
	uint arg[4] = { 0 };
	int x1, x2, y1, y2;

	if(sprite_cartoon_screen_set())
	{
		printf("sprite cartoon create fail\n");

		return -1;
	}
	board_display_show_until_lcd_open(0);

	screen_width  = disp_ioctl(NULL, DISP_CMD_SCN_GET_WIDTH, (void*)arg);
	screen_height = disp_ioctl(NULL, DISP_CMD_SCN_GET_HEIGHT, (void*)arg);

	printf("screen_width = %d\n", screen_width);
    printf("screen_height = %d\n", screen_height);

	x1 = screen_width/4;
	x2 = x1 * 3;

	y1 = screen_height/2 - 40;
	y2 = screen_height/2 + 40;

	printf("bar x1: %d y1: %d\n", x1, y1);
	printf("bar x2: %d y2: %d\n", x2, y2);

	progressbar_hd = sprite_cartoon_progressbar_create(x1, y1, x2, y2);
	sprite_cartoon_progressbar_config(progressbar_hd, SPRITE_CARTOON_GUI_RED, SPRITE_CARTOON_GUI_GREEN, 2);
	sprite_cartoon_progressbar_active(progressbar_hd);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sprite_cartoon_start
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
int sprite_cartoon_upgrade(int rate)
{
	if(last_rate == rate)
	{
		return 0;
	}
	last_rate = rate;

	sprite_cartoon_progressbar_upgrate(progressbar_hd, rate);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sprite_cartoon_start
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
int sprite_cartoon_destroy(void)
{
	sprite_cartoon_progressbar_destroy(progressbar_hd);

	return 0;
}
