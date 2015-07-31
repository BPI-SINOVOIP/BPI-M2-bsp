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
 
__s32 Disp_TVEC_Init(__u32 sel)
{
    __s32 ret = 0, value = 0;
    char primary_key[25];

    sprintf(primary_key, "tv%d_para", sel);
    ret = OSAL_Script_FetchParser_Data(primary_key, "tv_used", &value, 1);
    if(ret == 0)
    {
        gdisp.screen[sel].tv_used = value;
    }

    gdisp.screen[sel].tv_mode = DISP_TV_MOD_PAL;
    return DIS_SUCCESS;
}
      

__s32 Disp_TVEC_Exit(__u32 sel)
{
    if(gdisp.screen[sel].tv_used == 1)
    {

    }

    return DIS_SUCCESS;
}

__s32 Disp_TVEC_Open(__u32 sel)
{
	return DIS_SUCCESS;
}

__s32 Disp_TVEC_Close(__u32 sel)
{
	return DIS_SUCCESS;
}

static void Disp_TVEC_DacCfg(__u32 sel, __u8 mode)
{

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

static __s32 disp_tv_pin_cfg(__u32 on_off)
{
    disp_gpio_set_t  gpio_info[1];
    __hdle lcd_pin_hdl;
    __u32 i;

    for(i=3; i<=7; i++)
    {
        gpio_info->gpio = GPIOD(0+i);
        gpio_info->mul_sel = (on_off)?2:7;
        gpio_info->drv_level = 3;
        gpio_info->pull = 0;
        gpio_info->data = 0;
        lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
        OSAL_GPIO_Release(lcd_pin_hdl, 2);
    }
    for(i=10; i<=12; i++)
    {
        gpio_info->gpio = GPIOD(0+i);
        gpio_info->mul_sel = (on_off)?2:7;
        gpio_info->drv_level = 3;
        gpio_info->pull = 0;
        gpio_info->data = 0;
        lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
        OSAL_GPIO_Release(lcd_pin_hdl, 2);
    }
    gpio_info->gpio = GPIOD(24); //dclk
    gpio_info->mul_sel = (on_off)?2:7;
    gpio_info->drv_level = 3;
    gpio_info->pull = 0;
    gpio_info->data = 0;
    lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
    OSAL_GPIO_Release(lcd_pin_hdl, 2);

    gpio_info->gpio = GPIOD(26); //hsync
    gpio_info->mul_sel = (on_off)?2:7;
    gpio_info->drv_level = 3;
    gpio_info->pull = 0;
    gpio_info->data = 0;
    lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
    OSAL_GPIO_Release(lcd_pin_hdl, 2);

    gpio_info->gpio = GPIOD(27); //vsync
    gpio_info->mul_sel = (on_off)?2:7;
    gpio_info->drv_level = 3;
    gpio_info->pull = 0;
    gpio_info->data = 0;
    lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
    OSAL_GPIO_Release(lcd_pin_hdl, 2);

    return 0;
}
__s32 BSP_disp_tv_open(__u32 sel)
{
    if((!(gdisp.screen[sel].status & TV_ON)))
    {
        __disp_tv_mode_t     tv_mod;
        __panel_para_t para;
        memset(&para, 0, sizeof(__panel_para_t));

        tv_mod = gdisp.screen[sel].tv_mode;

        lcdc_clk_on(sel, 0, 0);
        disp_clk_cfg(sel, DISP_OUTPUT_TYPE_TV, tv_mod);
        lcdc_clk_on(sel, 0, 1);
        drc_clk_open(sel,0);
        tcon_init(sel);
        image_clk_on(sel, 1);
        Image_open(sel);//set image normal channel start bit , because every de_clk_off( )will reset this bit

        BSP_disp_set_output_csc(sel, DISP_OUT_CSC_TYPE_LCD, BSP_disp_drc_get_input_csc(sel)); //LCD -->GM7121,   rgb fmt

        DE_BE_set_display_size(sel, tv_mode_to_width(tv_mod), tv_mode_to_height(tv_mod));
        DE_BE_Output_Select(sel, sel);
        disp_tv_get_timing(&para, tv_mod);
        tcon0_cfg(sel,(__panel_para_t*)&para);
        tcon0_src_select(sel,4);

        if(gdisp.screen[sel].tv_ops.tv_power_on)
        {
            gdisp.screen[sel].tv_ops.tv_power_on(1);
            msleep(500);
        }
        disp_tv_pin_cfg(1);

        tcon0_open(sel,(__panel_para_t*)&para);
        if(gdisp.screen[sel].tv_ops.tv_open)
        {
            gdisp.screen[sel].tv_ops.tv_open();
        }
        Disp_Switch_Dram_Mode(DISP_OUTPUT_TYPE_TV, tv_mod);

        gdisp.screen[sel].b_out_interlace = Disp_get_screen_scan_mode(tv_mod);
        gdisp.screen[sel].status = TV_ON;
        gdisp.screen[sel].lcdc_status |= LCDC_TCON0_USED;
        gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_TV;
        gdisp.screen[sel].output_csc_type = DISP_OUT_CSC_TYPE_LCD;//LCD -->GM7121,   rgb fmt

        if(BSP_disp_cmu_get_enable(sel) ==1)
        {
            IEP_CMU_Set_Imgsize(sel, BSP_disp_get_screen_width(sel), BSP_disp_get_screen_height(sel));
        }
        Disp_set_out_interlace(sel);
#ifdef __LINUX_OSAL__
        Display_set_fb_timming(sel);
#endif
        tcon0_src_select(sel,0);
    }
    return DIS_SUCCESS;
}
      

__s32 BSP_disp_tv_close(__u32 sel)
{
    if((gdisp.screen[sel].status & TV_ON))
    {
        tcon0_src_select(sel,4);
        //msleep(100);
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
		msleep(900);
    }
    return DIS_SUCCESS;
}

__s32 BSP_disp_tv_set_mode(__u32 sel, __disp_tv_mode_t tv_mod)
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


__s32 BSP_disp_tv_get_mode(__u32 sel)
{   
    return gdisp.screen[sel].tv_mode;
}
      

__s32 BSP_disp_tv_get_interface(__u32 sel)
{
    return  DIS_SUCCESS;
}
      
      

__s32 BSP_disp_tv_get_dac_status(__u32 sel, __u32 index)
{
	return DIS_SUCCESS;
}

__s32 BSP_disp_tv_set_dac_source(__u32 sel, __u32 index, __disp_tv_dac_source source)
{
    return  0;
}

__s32 BSP_disp_tv_get_dac_source(__u32 sel, __u32 index)
{
    return 0;
}

__s32 BSP_disp_tv_auto_check_enable(__u32 sel)
{
    return DIS_SUCCESS;
}


__s32 BSP_disp_tv_auto_check_disable(__u32 sel)
{
    return DIS_SUCCESS;
}

__s32 BSP_disp_tv_set_src(__u32 sel, __disp_lcdc_src_t src)
{
    return DIS_SUCCESS;
}


__s32 BSP_disp_restore_tvec_reg(__u32 sel)
{
    return 0;
}

__s32 BSP_disp_set_tv_func(__u32 sel, __disp_tv_func *func)
{
    pr_warn("[TV]BSP_disp_set_tv_func\n");

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
            msleep(500);
        }

        if(gdisp.screen[sel].tv_ops.tv_set_mode)
        {
            gdisp.screen[sel].tv_ops.tv_set_mode(BSP_disp_tv_get_mode(sel));
        }

        if(gdisp.screen[sel].tv_ops.tv_open)
        {
            gdisp.screen[sel].tv_ops.tv_open();
        }
    }

    return 0;
}

__s32 BSP_disp_set_av_support_flag(__u32 screen_id, __s32 value)
{
    gdisp.screen[screen_id].av_support[screen_id] = value;
    return 0;
}

__s32 BSP_disp_get_av_support_flag(__u32 screen_id)
{
    return gdisp.screen[screen_id].av_support[screen_id];
}
