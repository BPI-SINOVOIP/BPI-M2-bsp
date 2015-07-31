#include "disp_event.h"
#include "disp_display.h"
#include "disp_de.h"
#include "disp_video.h"
#include "disp_scaler.h"
#include "disp_iep.h"
#include "disp_lcd.h"

#define VINT_TIME_LEN 100
static unsigned long vint_time_index[2] = {0,0};
static unsigned long vint_time[2][VINT_TIME_LEN];//jiffies

extern __panel_para_t gpanel_info[2];

__s32 bsp_disp_cmd_cache_get(__u32 sel)
{
  return  gdisp.screen[sel].cache_flag;
}

__s32 bsp_disp_cfg_get(__u32 sel)
{
   return gdisp.screen[sel].cfg_cnt;
}

__s32 BSP_disp_cmd_cache(__u32 sel)
{
#ifdef __LINUX_OSAL__
    unsigned long flags;
    spin_lock_irqsave(&gdisp.screen[sel].flag_lock, flags);
#endif
    gdisp.screen[sel].cache_flag = TRUE;
#ifdef __LINUX_OSAL__
    spin_unlock_irqrestore(&gdisp.screen[sel].flag_lock, flags);
#endif
    return DIS_SUCCESS;
}

__s32 BSP_disp_cmd_submit(__u32 sel)
{
#ifdef __LINUX_OSAL__
    unsigned long flags;
    spin_lock_irqsave(&gdisp.screen[sel].flag_lock, flags);
#endif
    gdisp.screen[sel].cache_flag = FALSE;
#ifdef __LINUX_OSAL__
    spin_unlock_irqrestore(&gdisp.screen[sel].flag_lock, flags);
#endif
    return DIS_SUCCESS;
}

__s32 BSP_disp_cfg_start(__u32 sel)
{
#ifdef __LINUX_OSAL__
    unsigned long flags;
    spin_lock_irqsave(&gdisp.screen[sel].flag_lock, flags);
#endif
	gdisp.screen[sel].cfg_cnt++;
#ifdef __LINUX_OSAL__
    spin_unlock_irqrestore(&gdisp.screen[sel].flag_lock, flags);
#endif
	return DIS_SUCCESS;
}

__s32 BSP_disp_cfg_finish(__u32 sel)
{
#ifdef __LINUX_OSAL__
    unsigned long flags;
    spin_lock_irqsave(&gdisp.screen[sel].flag_lock, flags);
#endif
	gdisp.screen[sel].cfg_cnt--;
#ifdef __LINUX_OSAL__
    spin_unlock_irqrestore(&gdisp.screen[sel].flag_lock, flags);
#endif
	return DIS_SUCCESS;
}

__s32 BSP_disp_vsync_event_enable(__u32 sel, __bool enable)
{
    gdisp.screen[sel].vsync_event_en = enable;
    
    return DIS_SUCCESS;
}
//return 10fps
__s32 bsp_disp_get_fps(__u32 sel)
{
    __u32 pre_time_index, cur_time_index;
    __u32 pre_time, cur_time;
    __u32 fps = 0xff;

    pre_time_index = vint_time_index[sel];
    cur_time_index = (pre_time_index == 0)? (VINT_TIME_LEN -1):(pre_time_index-1);

    pre_time = vint_time[sel][pre_time_index];
    cur_time = vint_time[sel][cur_time_index];

    if(pre_time != cur_time)
    {
        fps = 1000 * 100 / (cur_time - pre_time);
    }
    
    return fps;
}

__s32 disp_vint_checkin(__u32 sel)
{
    vint_time[sel][vint_time_index[sel]] = jiffies;
    vint_time_index[sel] ++;

    vint_time_index[sel] = (vint_time_index[sel] >= VINT_TIME_LEN)? 0:vint_time_index[sel];

    return 0;
}

__s32 disp_lcd_set_fps(__u32 sel);
extern __s32 disp_capture_screen_proc(__u32 sel);
void LCD_vbi_event_proc(__u32 sel, __u32 tcon_index)
{    
    __u32 cur_line = 0, start_delay = 0;
    __u32 i = 0;

    disp_vint_checkin(sel);
    disp_lcd_set_fps(sel);
    
	Video_Operation_In_Vblanking(sel, tcon_index);
    disp_capture_screen_proc(sel);

    cur_line = TCON_get_cur_line(sel, tcon_index);
    start_delay = TCON_get_start_delay(sel, tcon_index);
    if(cur_line > start_delay-4)
	{
	    //DE_INF("int:%d,%d\n", cur_line,start_delay);
        if(gpanel_info[sel].lcd_fresh_mode == 0)//return while not  trigger mode 
		{
		    return ;
        }
	}

    if(gdisp.screen[sel].cache_flag == FALSE && gdisp.screen[sel].cfg_cnt == 0)
    {
        DE_BE_Cfg_Ready(sel);
        IEP_CMU_Operation_In_Vblanking(sel);
        for(i=0; i<2; i++)
        {            
            if((gdisp.scaler[i].status & SCALER_USED) && (gdisp.scaler[i].screen_index == sel))
            {
                __u32 hid;

                if(gdisp.scaler[i].b_close == TRUE)
                {
                    Scaler_close(i);
                    gdisp.scaler[i].b_close = FALSE;
                }
                else
                {
                    hid = gdisp.scaler[i].layer_id;
                    DE_SCAL_Set_Reg_Rdy(i);
                    //DE_SCAL_Reset(i);
                    //DE_SCAL_Start(i);
                    disp_deu_set_frame_info(sel, IDTOHAND(hid));
                    disp_deu_output_select(sel, IDTOHAND(hid), sel);
                    IEP_Deu_Operation_In_Vblanking(i);
                }
                gdisp.scaler[i].b_reg_change = false;
            }
        }

        if(DISP_OUTPUT_TYPE_LCD == BSP_disp_get_output_type(sel))
        {
            IEP_Drc_Operation_In_Vblanking(sel);
        }

		gdisp.screen[sel].have_cfg_reg = TRUE;
    }

#if 0
    cur_line = LCDC_get_cur_line(sel, tcon_index);
    
	if(cur_line > 5)
	{
    	DE_INF("%d\n", cur_line);
    }
#endif

    return ;
}

void LCD_line_event_proc(__u32 sel)
{    
    if(gdisp.screen[sel].vsync_event_en && gdisp.init_para.vsync_event)
    {
        gdisp.init_para.vsync_event(sel);
    }

	if(gdisp.screen[sel].have_cfg_reg)
	{   
        gdisp.init_para.disp_int_process(sel);
	    gdisp.screen[sel].have_cfg_reg = FALSE;
	}
}
