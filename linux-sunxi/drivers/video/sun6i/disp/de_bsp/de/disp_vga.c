#include "disp_vga.h"
#include "disp_de.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_tv.h"
#include "disp_lcd.h"
#include "disp_clk.h"


__s32 VGA_Init(void)
{		
	gdisp.screen[0].vga_mode = DISP_VGA_H1024_V768;
    gdisp.screen[1].vga_mode = DISP_VGA_H1024_V768;
    
	return DIS_SUCCESS;
}

__s32 VGA_Exit(void)
{
	return DIS_SUCCESS;
}

__s32 BSP_disp_vga_open(__u32 sel)
{
    if(!(gdisp.screen[sel].status & VGA_ON))
    {
    	__disp_vga_mode_t vga_mode;
        __u32 i = 0;

    	vga_mode = gdisp.screen[sel].vga_mode;
    	
    	lcdc_clk_on(sel, 1, 0);
        lcdc_clk_on(sel, 1, 1);
        tcon_init(sel);
    	image_clk_on(sel, 1);
        Image_open(sel);//set image normal channel start bit , because every de_clk_off( )will reset this bit
    	tve_clk_on(sel);
    	disp_clk_cfg(sel,DISP_OUTPUT_TYPE_VGA, vga_mode);
    	Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_VGA, 1);

        gdisp.screen[sel].output_csc_type = DISP_OUT_CSC_TYPE_VGA;
		BSP_disp_set_output_csc(sel, gdisp.screen[sel].output_csc_type, BSP_disp_drc_get_input_csc(sel));
        DE_BE_set_display_size(sel, vga_mode_to_width(vga_mode), vga_mode_to_height(vga_mode));
        DE_BE_Output_Select(sel, sel);
    	tcon1_set_vga_mode(sel,vga_mode);
        TVE_set_vga_mode(sel);

    	Disp_TVEC_Open(sel);
    	tcon1_open(sel);

        for(i=0; i<4; i++)
        {
            if(gdisp.screen[sel].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE)
            {
                TVE_dac_set_source(1-sel, i, DISP_TV_DAC_SRC_COMPOSITE);
                TVE_dac_sel(1-sel, i, i);
            }
        }

    	Disp_Switch_Dram_Mode(DISP_OUTPUT_TYPE_VGA, vga_mode);

        gdisp.screen[sel].b_out_interlace = 0;
        gdisp.screen[sel].status = VGA_ON;
    	gdisp.screen[sel].lcdc_status |= LCDC_TCON1_USED;
    	gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_VGA;
#ifdef __LINUX_OSAL__
        Display_set_fb_timming(sel);
#endif
	}
	
	return DIS_SUCCESS;
}

__s32 BSP_disp_vga_close(__u32 sel)
{
	if(gdisp.screen[sel].status & VGA_ON)
	{    
        Image_close(sel);
    	tcon1_close(sel);
    	Disp_TVEC_Close(sel);
    	
    	tve_clk_off(sel);
    	image_clk_off(sel, 1);
    	lcdc_clk_off(sel);
    	Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_VGA, 0);

        gdisp.screen[sel].b_out_interlace = 0;
    	gdisp.screen[sel].status &= VGA_OFF;	
    	gdisp.screen[sel].lcdc_status &= LCDC_TCON1_USED_MASK;
    	gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_NONE;
		gdisp.screen[sel].pll_use_status &= ((gdisp.screen[sel].pll_use_status == VIDEO_PLL0_USED)? VIDEO_PLL0_USED_MASK : VIDEO_PLL1_USED_MASK);
    }
	return DIS_SUCCESS;
}

__s32 BSP_disp_vga_set_mode(__u32 sel, __disp_vga_mode_t  mode)
{ 	
    if((mode >= DISP_VGA_MODE_NUM) || (mode == DISP_VGA_H1440_V900_RB) || (mode == DISP_VGA_H1680_V1050_RB))
    {
        DE_WRN("unsupported vga mode:%d in BSP_disp_vga_set_mode\n", mode);
        return DIS_FAIL;
    }

	gdisp.screen[sel].vga_mode = mode;//save current mode
	gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_VGA;

	return DIS_SUCCESS;
}

__s32 BSP_disp_vga_get_mode(__u32 sel)
{   
	return gdisp.screen[sel].vga_mode;
}

__s32 BSP_disp_vga_set_src(__u32 sel, __disp_lcdc_src_t src)
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
    return DIS_SUCCESS;
}

