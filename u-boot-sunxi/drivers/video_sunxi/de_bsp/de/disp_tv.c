#include "disp_tv.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_de.h"
#include "disp_lcd.h"
#include "disp_clk.h"

__s32 Disp_Switch_Dram_Mode(__u32 type, __u8 tv_mod)
{
	return DIS_SUCCESS;
}

__s32 Disp_TVEC_Init(__u32 screen_id)
{
	__s32 ret = 0, value = 0;

	tve_clk_init(screen_id);
	tve_clk_on(screen_id);
	TVE_init(screen_id);
	tve_clk_off(screen_id);

	gdisp.screen[screen_id].dac_source[0] = DISP_TV_DAC_SRC_Y;
	gdisp.screen[screen_id].dac_source[1] = DISP_TV_DAC_SRC_PB;
	gdisp.screen[screen_id].dac_source[2] = DISP_TV_DAC_SRC_PR;
	gdisp.screen[screen_id].dac_source[3] = DISP_TV_DAC_SRC_COMPOSITE;

	ret = OSAL_Script_FetchParser_Data("tv_para", "dac_used", &value, 1);
	if(ret == 0) {
		if(value != 0) {
			__s32 i = 0;
			char sub_key[20];

			for(i=0; i<4; i++) {
				sprintf(sub_key, "dac%d_src", i);

				ret = OSAL_Script_FetchParser_Data("tv_out_dac_para", sub_key, &value, 1);
				if(ret == 0) {
					gdisp.screen[screen_id].dac_source[i] = value;
				}
			}
		}
	}

	gdisp.screen[screen_id].tv_mode = DISP_TV_MOD_720P_50HZ;
	return DIS_SUCCESS;
}


__s32 Disp_TVEC_Exit(__u32 screen_id)
{
	TVE_exit(screen_id);
	tve_clk_exit(screen_id);

	return DIS_SUCCESS;
}

__s32 Disp_TVEC_Open(__u32 screen_id)
{
	TVE_open(screen_id);
	return DIS_SUCCESS;
}

__s32 Disp_TVEC_Close(__u32 screen_id)
{
	TVE_dac_disable(screen_id, 0);
	TVE_dac_disable(screen_id, 1);
	TVE_dac_disable(screen_id, 2);
	TVE_dac_disable(screen_id, 3);

	TVE_close(screen_id);

	return DIS_SUCCESS;
}

static void Disp_TVEC_DacCfg(__u32 screen_id, __u8 mode)
{
	__u32 i = 0;

	TVE_dac_disable(screen_id, 0);
	TVE_dac_disable(screen_id, 1);
	TVE_dac_disable(screen_id, 2);
	TVE_dac_disable(screen_id, 3);

	switch(mode) {
	case DISP_TV_MOD_NTSC:
	case DISP_TV_MOD_PAL:
	case DISP_TV_MOD_PAL_M:
	case DISP_TV_MOD_PAL_NC:
	{
		for(i=0; i<4; i++) {
			if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_COMPOSITE);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}
		}
	}
	break;

	case DISP_TV_MOD_NTSC_SVIDEO:
	case DISP_TV_MOD_PAL_SVIDEO:
	case DISP_TV_MOD_PAL_M_SVIDEO:
	case DISP_TV_MOD_PAL_NC_SVIDEO:
	{
		for(i=0; i<4; i++) {
			if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_LUMA) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_LUMA);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_CHROMA) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_CHROMA);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}
		}
	}
	break;

	case DISP_TV_MOD_480I:
	case DISP_TV_MOD_576I:
	case DISP_TV_MOD_480P:
	case DISP_TV_MOD_576P:
	case DISP_TV_MOD_720P_50HZ:
	case DISP_TV_MOD_720P_60HZ:
	case DISP_TV_MOD_1080I_50HZ:
	case DISP_TV_MOD_1080I_60HZ:
	case DISP_TV_MOD_1080P_50HZ:
	case DISP_TV_MOD_1080P_60HZ:
	{
		for(i=0; i<4; i++) {
			if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_Y) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_Y);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_PB) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_PB);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_PR) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_PR);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE) {
				  TVE_dac_set_source(1-screen_id, i, DISP_TV_DAC_SRC_COMPOSITE);
				  TVE_dac_sel(1-screen_id, i, i);
			}
		}
	}
		break;

	default:
		break;
	}
}
static __s32 disp_tv_pin_cfg(__u32 on_off)
{
    disp_gpio_set_t  gpio_info[1];
    __hdle lcd_pin_hdl;
    __u32 i;

    for(i=3; i<=7; i++)
    {
        gpio_info->port = 4;
		gpio_info->port_num = i;
        gpio_info->mul_sel = (on_off)?2:7;
        gpio_info->drv_level = 3;
        gpio_info->pull = 0;
        gpio_info->data = 0;
        lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
        OSAL_GPIO_Release(lcd_pin_hdl, 2);
    }
    for(i=10; i<=12; i++)
    {
        gpio_info->port = 4;
		gpio_info->port_num = i;
        gpio_info->mul_sel = (on_off)?2:7;
        gpio_info->drv_level = 3;
        gpio_info->pull = 0;
        gpio_info->data = 0;
        lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
        OSAL_GPIO_Release(lcd_pin_hdl, 2);
    }
    gpio_info->port = 4;
	gpio_info->port_num = 24; //dclk
    gpio_info->mul_sel = (on_off)?2:7;
    gpio_info->drv_level = 3;
    gpio_info->pull = 0;
    gpio_info->data = 0;
    lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
    OSAL_GPIO_Release(lcd_pin_hdl, 2);

    gpio_info->port = 4;
	gpio_info->port_num = 26;//hsync
    gpio_info->mul_sel = (on_off)?2:7;
    gpio_info->drv_level = 3;
    gpio_info->pull = 0;
    gpio_info->data = 0;
    lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
    OSAL_GPIO_Release(lcd_pin_hdl, 2);

    gpio_info->port = 4;
	gpio_info->port_num = 27; //vsync
    gpio_info->mul_sel = (on_off)?2:7;
    gpio_info->drv_level = 3;
    gpio_info->pull = 0;
    gpio_info->data = 0;
    lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
    OSAL_GPIO_Release(lcd_pin_hdl, 2);

    return 0;
}

static __s32 disp_tv_get_timing(__panel_para_t *para, __disp_tv_mode_t tv_mode)
{
    __s32 ret = -1;

    if(tv_mode == DISP_TV_MOD_PAL)
    {
        para->lcd_if = 0;
        para->lcd_x = 720;
        para->lcd_y = 576;
        para->lcd_hv_if = LCD_HV_IF_CCIR656_2CYC;
        para->lcd_dclk_freq = 27;
        para->lcd_ht = 864;
        para->lcd_hbp = 139;
        para->lcd_hspw = 2;
        para->lcd_vt = 625;
        para->lcd_vbp = 22;
        para->lcd_vspw = 2;
        para->lcd_hv_syuv_fdly = LCD_HV_SRGB_FDLY_3LINE;
        para->lcd_hv_syuv_seq = LCD_HV_SYUV_SEQ_UYUV;
        ret = 0;
    }else if(tv_mode == DISP_TV_MOD_NTSC)
    {
        para->lcd_if = 0;
        para->lcd_x = 720;
        para->lcd_y = 480;
        para->lcd_hv_if = LCD_HV_IF_CCIR656_2CYC;
        para->lcd_dclk_freq = 27;
        para->lcd_ht = 858;
        para->lcd_hbp = 118;
        para->lcd_hspw = 2;
        para->lcd_vt = 525;
        para->lcd_vbp = 18;
        para->lcd_vspw = 2;
        para->lcd_hv_syuv_fdly = LCD_HV_SRGB_FDLY_2LINE;
        para->lcd_hv_syuv_seq = LCD_HV_SYUV_SEQ_UYUV;
        ret = 0;
    }

    return ret;
}

__s32 bsp_disp_tv_open(__u32 sel)
{
	if(!(gdisp.screen[sel].status & TV_ON))
    {
        __disp_tv_mode_t  tv_mod;
        __panel_para_t    para;
        memset(&para, 0, sizeof(__panel_para_t));

        tv_mod = gdisp.screen[sel].tv_mode;

        lcdc_clk_on(sel, 0, 0);
        disp_clk_cfg(sel, DISP_OUTPUT_TYPE_TV, tv_mod);

        lcdc_clk_on(sel, 0, 1);
        drc_clk_open(sel,0);
        tcon_init(sel);
        image_clk_on(sel, 1);
        Image_open(sel);//set image normal channel start bit , because every de_clk_off( )will reset this bit

        bsp_disp_set_output_csc(sel, DISP_OUT_CSC_TYPE_LCD, bsp_disp_drc_get_input_csc(sel)); //LCD -->GM7121,   rgb fmt

        DE_BE_set_display_size(sel, tv_mode_to_width(tv_mod), tv_mode_to_height(tv_mod));
        DE_BE_Output_Select(sel, sel);
		disp_tv_get_timing(&para, tv_mod);
        tcon0_cfg(sel,(__panel_para_t*)&para);
        if(gdisp.screen[sel].tv_ops.tv_power_on)
        {
            gdisp.screen[sel].tv_ops.tv_power_on(1);
            __msdelay(500);
        }
        disp_tv_pin_cfg(1);

        tcon0_open(sel,(__panel_para_t*)&para);
        if(gdisp.screen[sel].tv_ops.tv_open)
        {
            gdisp.screen[sel].tv_ops.tv_open();
        }

        Disp_Switch_Dram_Mode(DISP_OUTPUT_TYPE_TV, tv_mod);

        gdisp.screen[sel].b_out_interlace = disp_get_screen_scan_mode(tv_mod);
        gdisp.screen[sel].status |= TV_ON;
        gdisp.screen[sel].lcdc_status |= LCDC_TCON0_USED;
        gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_TV;

        if(bsp_disp_cmu_get_enable(sel) ==1)
        {
            IEP_CMU_Set_Imgsize(sel, bsp_disp_get_screen_width(sel), bsp_disp_get_screen_height(sel));
        }
        Disp_set_out_interlace(sel);
#ifdef __LINUX_OSAL__
        Display_set_fb_timming(sel);
#endif
    }
    return DIS_SUCCESS;
}

__s32 bsp_disp_tv_close(__u32 sel)
{
	if(gdisp.screen[sel].status & TV_ON)
		{
			tcon0_close(sel);
			Image_close(sel);
			image_clk_off(sel, 1);
			lcdc_clk_off(sel);
			drc_clk_close(sel,0);

			if(gdisp.screen[sel].tv_ops.tv_close)
			{
				gdisp.screen[sel].tv_ops.tv_close();
			}
			disp_tv_pin_cfg(0);
			if(gdisp.screen[sel].tv_ops.tv_power_on)
			{
				gdisp.screen[sel].tv_ops.tv_power_on(0);
			}

			gdisp.screen[sel].b_out_interlace = 0;
			gdisp.screen[sel].status &= TV_OFF;
			gdisp.screen[sel].lcdc_status &= LCDC_TCON0_USED_MASK;
			gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_NONE;
			gdisp.screen[sel].pll_use_status &= ((gdisp.screen[sel].pll_use_status == VIDEO_PLL0_USED)? VIDEO_PLL0_USED_MASK : VIDEO_PLL1_USED_MASK);

			Disp_set_out_interlace(sel);
		}
		return DIS_SUCCESS;

}

__s32 bsp_disp_tv_set_mode(__u32 sel, __disp_tv_mode_t tv_mod)
{
	if(tv_mod >= DISP_TV_MODE_NUM)
    {
        DE_WRN("unsupported tv mode:%d in BSP_disp_tv_set_mode\n", tv_mod);
        return DIS_FAIL;
    }

    gdisp.screen[sel].tv_mode = tv_mod;
    if(gdisp.screen[sel].tv_ops.tv_set_mode)
    {
        gdisp.screen[sel].tv_ops.tv_set_mode(tv_mod);
    }
    return DIS_SUCCESS;
}


__s32 bsp_disp_tv_get_mode(__u32 screen_id)
{
	return gdisp.screen[screen_id].tv_mode;
}


__s32 bsp_disp_tv_get_interface(__u32 screen_id)
{
	__u8 dac[4] = {0};
	__s32 i = 0;
	__u32  ret = DISP_TV_NONE;

	for(i=0; i<4; i++) {
		dac[i] = TVE_get_dac_status(i);
		if(dac[i]>1) {
			DE_WRN("dac %d short to ground\n", i);
			dac[i] = 0;
		}

		if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE && dac[i] == 1) {
			ret |= DISP_TV_CVBS;
		}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_Y && dac[i] == 1) {
			ret |= DISP_TV_YPBPR;
		}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_LUMA && dac[i] == 1)	{
			ret |= DISP_TV_SVIDEO;
		}
	}

	return  ret;
}



__s32 bsp_disp_tv_get_dac_status(__u32 screen_id, __u32 index)
{
	return TVE_get_dac_status(index);
}

__s32 bsp_disp_tv_set_dac_source(__u32 screen_id, __u32 index, __disp_tv_dac_source source)
{
	gdisp.screen[screen_id].dac_source[index] = source;

	if(gdisp.screen[screen_id].status & TV_ON) {
		Disp_TVEC_DacCfg(screen_id, gdisp.screen[screen_id].tv_mode);
	}

	return  0;
}

__s32 bsp_disp_tv_get_dac_source(__u32 screen_id, __u32 index)
{
	return (__s32)gdisp.screen[screen_id].dac_source[index];
}

__s32 bsp_disp_tv_auto_check_enable(__u32 screen_id)
{

	return DIS_SUCCESS;
}


__s32 bsp_disp_tv_auto_check_disable(__u32 screen_id)
{
    return DIS_SUCCESS;
}

__s32 bsp_disp_tv_set_src(__u32 screen_id, __disp_lcdc_src_t src)
{
    return DIS_SUCCESS;
}


__s32 bsp_disp_restore_tvec_reg(__u32 screen_id)
{
	TVE_init(screen_id);

	return 0;
}

__s32 bsp_disp_set_tv_func(__u32 sel, __disp_tv_func *func)
{
    __wrn("[TV]bsp_disp_set_tv_func\n");

    gdisp.screen[sel].tv_ops.tv_power_on = func->tv_power_on;
    gdisp.screen[sel].tv_ops.tv_open = func->tv_open;
    gdisp.screen[sel].tv_ops.tv_close = func->tv_close;
    gdisp.screen[sel].tv_ops.tv_get_hpd_status = func->tv_get_hpd_status;
    gdisp.screen[sel].tv_ops.tv_set_mode = func->tv_set_mode;
    gdisp.screen[sel].tv_ops.tv_get_mode_support = func->tv_get_mode_support;

    if((gdisp.screen[sel].status & TV_ON))
    {
        if(gdisp.screen[sel].tv_ops.tv_power_on)
        {
            gdisp.screen[sel].tv_ops.tv_power_on(1);
            __msdelay(500);
        }

        if(gdisp.screen[sel].tv_ops.tv_set_mode)
        {
            gdisp.screen[sel].tv_ops.tv_set_mode(bsp_disp_tv_get_mode(sel));
        }

        if(gdisp.screen[sel].tv_ops.tv_open)
        {
            gdisp.screen[sel].tv_ops.tv_open();
        }
    }

    return 0;
}

