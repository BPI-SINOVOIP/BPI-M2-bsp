#include "disp_hdmi.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_de.h"
#include "disp_tv.h"
#include "disp_lcd.h"
#include "disp_clk.h"

__bool hdmi_request_close_flag = 0;

__s32 Display_Hdmi_Init(void)
{
    __s32 ret;
    __u32 value;
    
    ret = OSAL_Script_FetchParser_Data("hdmi_para", "hdmi_used", &value, 1);
    if(ret == 0)
    {
        gdisp.screen[0].hdmi_used = value;
        gdisp.screen[1].hdmi_used = value;

        if(gdisp.screen[0].hdmi_used || gdisp.screen[1].hdmi_used)
        {
            ret = OSAL_Script_FetchParser_Data("hdmi_para", "hdmi_cts_compatibility", &value, 1);
		    if(ret < 0)
		    {
		        DE_INF("disp_init.hdmi_cts_compatibility not exit\n");
		    }
		    else
		    {
		        DE_INF("disp_init.hdmi_cts_compatibility = %d\n", value);
		        gdisp.init_para.hdmi_cts_compatibility = value;
		    }
			hdmi_clk_init();
        }
        gdisp.screen[0].hdmi_mode = DISP_TV_MOD_720P_50HZ;
        gdisp.screen[1].hdmi_mode = DISP_TV_MOD_720P_50HZ;
        gdisp.screen[0].hdmi_test_mode = 0xff;
        gdisp.screen[1].hdmi_test_mode = 0xff;
    }

    return DIS_SUCCESS;
}

__s32 Display_Hdmi_Exit(void)
{
    if(gdisp.screen[0].hdmi_used || gdisp.screen[1].hdmi_used)
    {
        hdmi_clk_exit();
    }
    
	return DIS_SUCCESS;
}

__s32 BSP_disp_hdmi_open(__u32 sel)
{
    if(gdisp.screen[sel].hdmi_used)
    {
        if(!(gdisp.screen[sel].status & HDMI_ON))
        {
            __disp_tv_mode_t     tv_mod;

            tv_mod = gdisp.screen[sel].hdmi_mode;

            hdmi_clk_on();
            lcdc_clk_on(sel, 1, 0);
            lcdc_clk_on(sel, 1, 1);
            drc_clk_open(sel,0);
            tcon_init(sel);
            image_clk_on(sel, 1);
            Image_open(sel);//set image normal channel start bit , because every de_clk_off( )will reset this bit
            disp_clk_cfg(sel,DISP_OUTPUT_TYPE_HDMI, tv_mod);

            //BSP_disp_set_output_csc(sel, DISP_OUTPUT_TYPE_HDMI, gdisp.screen[sel].iep_status&DRC_USED);
            if(gdisp.init_para.hdmi_cts_compatibility == 0)
            {
                DE_INF("BSP_disp_hdmi_open: disable dvi mode\n");
                BSP_disp_hdmi_dvi_enable(sel, 0);
            }
            else if(gdisp.init_para.hdmi_cts_compatibility == 1)
            {
                DE_INF("BSP_disp_hdmi_open: enable dvi mode\n");
                BSP_disp_hdmi_dvi_enable(sel, 1);
            }
            else
            {
                BSP_disp_hdmi_dvi_enable(sel, BSP_disp_hdmi_dvi_support(sel));
            }

            if(BSP_dsip_hdmi_get_input_csc(sel) == 0)
            {
                __inf("BSP_disp_hdmi_open:   hdmi output rgb\n");
                gdisp.screen[sel].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_RGB;
                BSP_disp_set_output_csc(sel, gdisp.screen[sel].output_csc_type, BSP_disp_drc_get_input_csc(sel));
            }else
            {
                __inf("BSP_disp_hdmi_open:   hdmi output yuv\n");
                gdisp.screen[sel].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_YUV;//default  yuv
                BSP_disp_set_output_csc(sel, gdisp.screen[sel].output_csc_type, BSP_disp_drc_get_input_csc(sel));
            }
            DE_BE_set_display_size(sel, tv_mode_to_width(tv_mod), tv_mode_to_height(tv_mod));
            DE_BE_Output_Select(sel, sel);

            if(BSP_disp_cmu_get_enable(sel) ==1)
            {
                IEP_CMU_Set_Imgsize(sel, BSP_disp_get_screen_width(sel), BSP_disp_get_screen_height(sel));
            }

            tcon1_set_hdmi_mode(sel,tv_mod);
            tcon1_open(sel);
            if(gdisp.init_para.hdmi_open)
            {
                gdisp.init_para.hdmi_open();
            }
            else
            {
                DE_WRN("Hdmi_open is NULL\n");
                return -1;
            }

            Disp_Switch_Dram_Mode(DISP_OUTPUT_TYPE_HDMI, tv_mod);

            gdisp.screen[sel].b_out_interlace = Disp_get_screen_scan_mode(tv_mod);
            gdisp.screen[sel].status = HDMI_ON;
            gdisp.screen[sel].lcdc_status |= LCDC_TCON1_USED;
            gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_HDMI;
            tcon0_src_select(sel,0);

            Disp_set_out_interlace(sel);
#ifdef __LINUX_OSAL__
            Display_set_fb_timming(sel);
#endif
    //dram ctrl config
            if(sel == 1)
            {
                (*((volatile __u32 *)(0xf1c62010))=(0x00400302));
                (*((volatile __u32 *)(0xf1c62074))=(0x00000310));
                (*((volatile __u32 *)(0xf1c62078))=(0x00000310));
                (*((volatile __u32 *)(0xf1c62080))=(0x00000317));
            }
        }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_hdmi_close(__u32 sel)
{
    if(gdisp.screen[sel].hdmi_used)
    {
        if(gdisp.screen[sel].status & HDMI_ON)
        {
            __u32 count = 50000;
            hdmi_request_close_flag = 1; // 1:request close,0:need not to close.
            while((count--) && hdmi_request_close_flag)
                udelay(1);
            if(hdmi_request_close_flag || count <= 0) {
                if(gdisp.init_para.hdmi_close){
                    gdisp.init_para.hdmi_close();
                }
                DE_INF("hdmi wait vb_int timeout\n");
            }
            tcon0_src_select(sel,4);
            Image_close(sel);
            tcon1_close(sel);

            image_clk_off(sel, 1);
            lcdc_clk_off(sel);
            drc_clk_close(sel,0);
            hdmi_clk_off();

            gdisp.screen[sel].b_out_interlace = 0;
            gdisp.screen[sel].lcdc_status &= LCDC_TCON1_USED_MASK;
            gdisp.screen[sel].status &= HDMI_OFF;
            gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_NONE;
            gdisp.screen[sel].pll_use_status &= ((gdisp.screen[sel].pll_use_status == VIDEO_PLL0_USED)? VIDEO_PLL0_USED_MASK : VIDEO_PLL1_USED_MASK);

            Disp_set_out_interlace(sel);
            if(sel == 1)
            {
                //dram ctrl config
                (*((volatile __u32 *)(0xf1c62010))=(0x00800302));
                (*((volatile __u32 *)(0xf1c62014))=(0x00400307));
                (*((volatile __u32 *)(0xf1c62018))=(0x00800302));
                (*((volatile __u32 *)(0xf1c6201c))=(0x00400307));
                (*((volatile __u32 *)(0xf1c62074))=(0x00010310));
                (*((volatile __u32 *)(0xf1c62078))=(0x00010310));
                (*((volatile __u32 *)(0xf1c62080))=(0x00000310));
            }
       }
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_hdmi_set_mode(__u32 sel, __disp_tv_mode_t  mode)
{
    if(mode >= DISP_TV_MODE_NUM)
    {
        DE_WRN("unsupported hdmi mode:%d in BSP_disp_hdmi_set_mode\n", mode);
        return DIS_FAIL;
    }

    if(gdisp.screen[sel].hdmi_used)
    {
        if(gdisp.init_para.hdmi_set_mode)
        {
          gdisp.init_para.hdmi_set_mode(mode);
        }
        else
        {
          DE_WRN("hdmi_set_mode is NULL\n");
          return -1;
        }

        gdisp.screen[sel].hdmi_mode = mode;
        gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_HDMI;

        return DIS_SUCCESS;
    }

	return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_hdmi_get_mode(__u32 sel)
{   
    return gdisp.screen[sel].hdmi_mode;
}

__s32 BSP_disp_hdmi_check_support_mode(__u32 sel, __u8  mode)
{
    if((gdisp.screen[sel].hdmi_used) && (gdisp.init_para.hdmi_mode_support))
	{
	    if(gdisp.screen[sel].hdmi_test_mode < DISP_TV_MODE_NUM)
        {
            if(mode == gdisp.screen[sel].hdmi_test_mode)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }else
        {
            return gdisp.init_para.hdmi_mode_support(mode);
        }
	}
	else
	{
	    DE_WRN("hdmi_mode_support is NULL\n");
	    return DIS_NOT_SUPPORT;
	}
}

__s32 BSP_disp_hdmi_get_hpd_status(__u32 sel)
{
	if((gdisp.screen[sel].hdmi_used) && (gdisp.init_para.hdmi_get_HPD_status))
	{
	    return gdisp.init_para.hdmi_get_HPD_status();
	}
	else
	{
	    DE_WRN("hdmi_get_HPD_status is NULL\n");
	    return -1;
	}
}

__s32 BSP_disp_hdmi_set_src(__u32 sel, __disp_lcdc_src_t src)
{
    if(gdisp.screen[sel].hdmi_used)
    {
        switch (src)
        {
            case DISP_LCDC_SRC_DE_CH1:
                tcon1_src_select(sel, LCD_SRC_BE0);
                break;

            case DISP_LCDC_SRC_DE_CH2:
                tcon1_src_select(sel, LCD_SRC_BE1);
                break;

            case DISP_LCDC_SRC_BLUE:
                tcon1_src_select(sel, LCD_SRC_BLUE);
                break;

            default:
                DE_WRN("not supported lcdc src:%d in BSP_disp_tv_set_src\n", src);
                return DIS_NOT_SUPPORT;
        }
     }
        return DIS_SUCCESS;
}

__s32 BSP_disp_hdmi_dvi_enable(__u32 sel, __u32 enable)
{
	__s32 ret = -1;

	if(gdisp.init_para.hdmi_dvi_enable)
	{
	    ret = gdisp.init_para.hdmi_dvi_enable(enable);
	    gdisp.screen[sel].dvi_enable = enable;
	}
	else
	{
	    DE_WRN("Hdmi_dvi_enable is NULL\n");
	}

	return ret;
}


__s32 BSP_disp_hdmi_dvi_support(__u32 sel)
{
	__s32 ret = -1;

	if(gdisp.init_para.hdmi_dvi_support)
	{
	    ret = gdisp.init_para.hdmi_dvi_support();
	}
	else
	{
	    DE_WRN("Hdmi_dvi_support is NULL\n");
	}

	return ret;
}

__s32 BSP_dsip_hdmi_get_input_csc(__u32 sel)
{
    __s32 ret = -1;

    if(gdisp.init_para.hmdi_get_input_csc)
    {
        ret = gdisp.init_para.hmdi_get_input_csc();
    }else
    {
        DE_WRN("Hdmi_get_input_csc is NULL \n");
    }

    return ret;
}

__s32 BSP_disp_hdmi_suspend(void)
{
    if(gdisp.init_para.hdmi_suspend)
    {
        return gdisp.init_para.hdmi_suspend();
    }

    return -1;
}

__s32 BSP_disp_hdmi_resume(void)
{
    if(gdisp.init_para.hdmi_resume)
    {
        return gdisp.init_para.hdmi_resume();
    }

    return -1;
}

__s32 BSP_disp_hdmi_early_suspend(void)
{
    if(gdisp.init_para.hdmi_early_suspend)
    {
        return gdisp.init_para.hdmi_early_suspend();
    }

    return -1;
}

__s32 BSP_disp_hdmi_late_resume(void)
{
    if(gdisp.init_para.hdmi_late_resume)
    {
        return gdisp.init_para.hdmi_late_resume();
    }

    return -1;
}

__s32 BSP_disp_set_hdmi_func(__disp_hdmi_func * func)
{
    gdisp.init_para.hdmi_open = func->hdmi_open;
    gdisp.init_para.hdmi_close = func->hdmi_close;
    gdisp.init_para.hdmi_set_mode = func->hdmi_set_mode;
    gdisp.init_para.hdmi_mode_support = func->hdmi_mode_support;
    gdisp.init_para.hdmi_get_HPD_status = func->hdmi_get_HPD_status;
    gdisp.init_para.hdmi_set_pll = func->hdmi_set_pll;
    gdisp.init_para.hdmi_dvi_enable = func->hdmi_dvi_enable;
    gdisp.init_para.hdmi_dvi_support = func->hdmi_dvi_support;
    gdisp.init_para.hmdi_get_input_csc = func->hdmi_get_input_csc;
    gdisp.init_para.hdmi_suspend = func->hdmi_suspend;
    gdisp.init_para.hdmi_resume = func->hdmi_resume;
    gdisp.init_para.hdmi_early_suspend = func->hdmi_early_suspend;
    gdisp.init_para.hdmi_late_resume = func->hdmi_late_resume;

    return DIS_SUCCESS;
}
//hpd: 0 plugout;  1 plugin
__s32 BSP_disp_set_hdmi_hpd(__u32 hpd)
{
    if(hpd == 1)
    {
        gdisp.screen[0].hdmi_hpd = 1;
        gdisp.screen[0].hdmi_hpd = 1;
        if(gdisp.screen[0].status & HDMI_ON)
        {
            if(BSP_dsip_hdmi_get_input_csc(0) == 0)
            {
                __inf("BSP_disp_set_hdmi_hpd:   hdmi output rgb\n");
                gdisp.screen[0].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_RGB;
                BSP_disp_set_output_csc(0, gdisp.screen[0].output_csc_type, BSP_disp_drc_get_input_csc(0));
            }else
            {
                __inf("BSP_disp_set_hdmi_hpd:   hdmi output yuv\n");//default  yuv
                gdisp.screen[0].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_YUV;
                BSP_disp_set_output_csc(0, gdisp.screen[0].output_csc_type, BSP_disp_drc_get_input_csc(0));
            }
        }
        else if(gdisp.screen[1].status & HDMI_ON)
        {
            if(BSP_dsip_hdmi_get_input_csc(1) == 0)
            {
                __inf("BSP_disp_set_hdmi_hpd:   hdmi output rgb\n");
                gdisp.screen[1].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_RGB;
                BSP_disp_set_output_csc(1, gdisp.screen[1].output_csc_type, BSP_disp_drc_get_input_csc(1));
            }else
            {
                __inf("BSP_disp_set_hdmi_hpd:   hdmi output yuv\n");
                gdisp.screen[1].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_YUV;//default  yuv
                BSP_disp_set_output_csc(1, gdisp.screen[1].output_csc_type,  BSP_disp_drc_get_input_csc(1));
            }
        }
    }
    else
    {
        gdisp.screen[0].hdmi_hpd = 0;
        gdisp.screen[0].hdmi_hpd = 0;
    }

    return 0;
}

__s32 BSP_disp_get_disp_layer_enable(__u32 sel)
{
    return gdisp.screen[sel].layer_en;
}

__s32 BSP_disp_set_disp_layer_enable(__u32 sel ,__u32 en)
{
   gdisp.screen[sel].layer_en = en;
   return 0;
}

__s32 BSP_disp_hdmi_cts_enable(__u32 en)
{
    gdisp.init_para.hdmi_cts_compatibility = en;

    return 0;
}


__s32 BSP_disp_hdmi_get_cts_enable()
{
    return gdisp.init_para.hdmi_cts_compatibility;
}


__s32 BSP_disp_hdmi_set_test_mode(__u32 sel, __u32 mode)
{
    gdisp.screen[sel].hdmi_test_mode = mode;

    return 0;
}

__s32 BSP_disp_hdmi_get_test_mode(__u32 sel)
{
    return gdisp.screen[sel].hdmi_test_mode;
}
