#include "disp_display.h"
#include "disp_de.h"
#include "disp_lcd.h"
#include "disp_tv.h"
#include "disp_event.h"
#include "disp_sprite.h"
#include "disp_combined.h"
#include "disp_scaler.h"
#include "disp_video.h"
#include "disp_clk.h"
#include "disp_hdmi.h"

__disp_dev_t gdisp;
extern __panel_para_t              gpanel_info[2];


__s32 BSP_disp_init(__disp_bsp_init_para * para)
{
    __u32 i = 0, screen_id = 0;

    memset(&gdisp,0x00,sizeof(__disp_dev_t));

    for(screen_id = 0; screen_id < 2; screen_id++)
    {
        gdisp.screen[screen_id].max_layers = 4;
        for(i = 0;i < gdisp.screen[screen_id].max_layers;i++)
        {
            gdisp.screen[screen_id].layer_manage[i].para.prio = IDLE_PRIO;
        }
        gdisp.screen[screen_id].image_output_type = IMAGE_OUTPUT_LCDC;
        
        gdisp.screen[screen_id].bright = 50;
        gdisp.screen[screen_id].contrast = 50;
        gdisp.screen[screen_id].saturation = 50;
        gdisp.screen[screen_id].hue = 50;
        
        gdisp.scaler[screen_id].bright = 50;
        gdisp.scaler[screen_id].contrast = 50;
        gdisp.scaler[screen_id].saturation = 50;
        gdisp.scaler[screen_id].hue = 50;

        gdisp.screen[screen_id].lcd_cfg.backlight_bright = 197;
        gdisp.screen[screen_id].lcd_cfg.backlight_dimming = 256;
        gdisp.screen[screen_id].layer_en = 1;
#ifdef __LINUX_OSAL__
        spin_lock_init(&gdisp.screen[screen_id].flag_lock);
#endif
    }
    memcpy(&gdisp.init_para,para,sizeof(__disp_bsp_init_para));
		gdisp.print_level = DEFAULT_PRINT_LEVLE;

    DE_Set_Reg_Base(0, para->base_image0);
    DE_Set_Reg_Base(1, para->base_image1);
    DE_SCAL_Set_Reg_Base(0, para->base_scaler0);
    DE_SCAL_Set_Reg_Base(1, para->base_scaler1);
    tcon_set_reg_base(0,para->base_lcdc0);
    tcon_set_reg_base(1,para->base_lcdc1);
    IEP_Deu_Set_Reg_base(0, para->base_deu0);
    IEP_Deu_Set_Reg_base(1, para->base_deu1);
    IEP_Drc_Set_Reg_Base(0, para->base_drc0);
    IEP_Drc_Set_Reg_Base(1, para->base_drc1);
    IEP_CMU_Set_Reg_Base(0, para->base_cmu0);
    IEP_CMU_Set_Reg_Base(1, para->base_cmu1);
    dsi_set_reg_base(0, para->base_dsi0);
    BSP_disp_close_lcd_backlight(0);
    BSP_disp_close_lcd_backlight(1);

    Scaler_Init(0);
    Scaler_Init(1);
    Image_init(0);
    Image_init(1);
    Disp_lcdc_init(0);
    Disp_lcdc_init(1);
    Display_Hdmi_Init();

    iep_init(0);   
    iep_init(1);

    disp_video_init();

    disp_pll_init();

    return DIS_SUCCESS;
}

__s32 BSP_disp_exit(__u32 mode)
{
    if(mode == DISP_EXIT_MODE_CLEAN_ALL)
    {
        BSP_disp_close();
        
        Scaler_Exit(0);
        Scaler_Exit(1);
        Image_exit(0);
        Image_exit(1);
        Disp_lcdc_exit(0);
        Disp_lcdc_exit(1);
        Disp_TVEC_Exit(0);
        Disp_TVEC_Exit(1);
        Display_Hdmi_Exit();
        iep_exit(0);
        iep_exit(1);

        disp_video_exit();
    }
    else if(mode == DISP_EXIT_MODE_CLEAN_PARTLY)
    {
        OSAL_InterruptDisable(INTC_IRQNO_LCDC0);
        OSAL_UnRegISR(INTC_IRQNO_LCDC0,Disp_lcdc_event_proc,(void*)0);

        OSAL_InterruptDisable(INTC_IRQNO_LCDC1);
        OSAL_UnRegISR(INTC_IRQNO_LCDC1,Disp_lcdc_event_proc,(void*)0);

        OSAL_InterruptDisable(INTC_IRQNO_SCALER0);
        OSAL_UnRegISR(INTC_IRQNO_SCALER0,Scaler_event_proc,(void*)0);

        OSAL_InterruptDisable(INTC_IRQNO_SCALER1);
        OSAL_UnRegISR(INTC_IRQNO_SCALER1,Scaler_event_proc,(void*)0);

        OSAL_InterruptDisable(INTC_IRQNO_DSI);
        OSAL_UnRegISR(INTC_IRQNO_DSI,Disp_lcdc_event_proc,(void*)0);
    }
    
    return DIS_SUCCESS;
}

__s32 BSP_disp_open(void)
{
    return DIS_SUCCESS;
}

__s32 BSP_disp_close(void)
{
    __u32 sel = 0;

    for(sel = 0; sel<2; sel++)
    {
       Image_close(sel);
        if(gdisp.scaler[sel].status & SCALER_USED)
        {
            Scaler_close(sel);
        }
        if(gdisp.screen[sel].lcdc_status & LCDC_TCON0_USED)
        {
            tcon0_close(sel);
            tcon_exit(sel);
        }
        else if(gdisp.screen[sel].lcdc_status & LCDC_TCON1_USED)
        {
    	    tcon1_close(sel);
    	    tcon_exit(sel);
        }
        else if(gdisp.screen[sel].status & (TV_ON | VGA_ON))
        {
        	TVE_close(sel);
        }

        gdisp.screen[sel].status &= (IMAGE_USED_MASK & LCD_OFF & TV_OFF & VGA_OFF & HDMI_OFF);
        gdisp.screen[sel].lcdc_status &= (LCDC_TCON0_USED_MASK & LCDC_TCON1_USED_MASK);
    }
    
    return DIS_SUCCESS;
}


__s32 BSP_disp_print_reg(__bool b_force_on, __u32 id)
{   
    __u32 base = 0, size = 0;
    __u32 i = 0;
    unsigned char str[20];

    switch(id)
    {
        case DISP_REG_SCALER0:
            base = gdisp.init_para.base_scaler0;
            size = 0xa18;
            sprintf(str, "scaler0:\n");
            break;
            
        case DISP_REG_SCALER1:
            base = gdisp.init_para.base_scaler1;
            size = 0xa18;
            sprintf(str, "scaler1:\n");
            break;
            
        case DISP_REG_IMAGE0:
            base = gdisp.init_para.base_image0 + 0x800;
            size = 0xdff - 0x800;
            sprintf(str, "image0:\n");
            break;
            
        case DISP_REG_IMAGE1:
            base = gdisp.init_para.base_image1 + 0x800;
            size = 0xdff - 0x800;
            sprintf(str, "image1:\n");
            break;
        case DISP_REG_LCDC0:
            base = gdisp.init_para.base_lcdc0;
            size = 0x3f0;
            sprintf(str, "lcdc0:\n");
            break;
            
        case DISP_REG_LCDC1:
            base = gdisp.init_para.base_lcdc1;
            size = 0x3f0;
            sprintf(str, "lcdc1:\n");
            break;
            
        case DISP_REG_TVEC0:
            base = gdisp.init_para.base_tvec0;
            size = 0x20c;
            sprintf(str, "tvec0:\n");
            break;
            
        case DISP_REG_TVEC1:
            base = gdisp.init_para.base_tvec1;
            size = 0x20c;
            sprintf(str, "tvec1:\n");
            break;
            
        case DISP_REG_CCMU:
            base = gdisp.init_para.base_ccmu;
            size = 0x308;
            sprintf(str, "ccmu:\n");
            break;
            
        case DISP_REG_PIOC:
            base = gdisp.init_para.base_pioc;
            size = 0x228;
            sprintf(str, "pioc:\n");
            break;
            
        case DISP_REG_PWM:
            base = gdisp.init_para.base_pwm;
            size = 0x40;
            sprintf(str, "pwm:\n");
            break;
        case DISP_REG_DEU0:
            base = gdisp.init_para.base_deu0;
            size = 0x60;
            sprintf(str, "deu0:\n");
            break;
        case DISP_REG_DEU1:
            base = gdisp.init_para.base_deu1;
            size = 0x60;
            sprintf(str, "deu1:\n");
            break;
        case DISP_REG_CMU0:
            base = gdisp.init_para.base_cmu0;
            size = 0x100;
            sprintf(str, "cmu0:\n");
            break;
        case DISP_REG_CMU1:
            base = gdisp.init_para.base_cmu1;
            size = 0x100;
            sprintf(str, "cmu1:\n");
            break;
        case DISP_REG_DRC0:
            base = gdisp.init_para.base_drc0;
            size = 0x200;
            sprintf(str, "drc0:\n");
            break;
        case DISP_REG_DRC1:
            base = gdisp.init_para.base_drc1;
            size = 0x200;
            sprintf(str, "drc1:\n");
            break;

        case DISP_REG_DSI:
            base = gdisp.init_para.base_dsi0;
            size = 0xf0;
            sprintf(str, "dsi:\n");
            break;

        case DISP_REG_DSI_DPHY:
            base = gdisp.init_para.base_dsi0+1000;
            size = 0xf4;
            sprintf(str, "dsi_dphy:\n");
            break;

        case DISP_REG_HDMI:
            base = gdisp.init_para.base_hdmi;
            size = 0x580;
            sprintf(str, "hdmi:\n");
            break;
        default:
            return DIS_FAIL;
    }

    if(b_force_on)
    {
        OSAL_PRINTF("%s", str);
    }
    else
    {
        DE_INF("%s", str);
    }
    for(i=0; i<size; i+=16)
    {
        __u32 reg[4];
        
        reg[0] = sys_get_wvalue(base + i);
        reg[1] = sys_get_wvalue(base + i + 4);
        reg[2] = sys_get_wvalue(base + i + 8);
        reg[3] = sys_get_wvalue(base + i + 12);
#ifdef __LINUX_OSAL__
        if(b_force_on)
        {
            OSAL_PRINTF("0x%08x:%08x,%08x:%08x,%08x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
        }
        else
        {
            DE_INF("0x%08x:%08x,%08x:%08x,%08x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
        }
#endif
#ifdef __BOOT_OSAL__
        if(b_force_on)
        {
            OSAL_PRINTF("0x%x:%x,%x,%x,%x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
        }
        else
        {
            DE_INF("0x%x:%x,%x:%x,%x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
        }
#endif
    }
    
    return DIS_SUCCESS;
}

__s32 bsp_disp_set_print_level(__u32 print_level)
{
    gdisp.print_level = print_level;

    return 0;
}

__s32 bsp_disp_get_print_level(void)
{
    return gdisp.print_level;
}

