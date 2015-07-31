
#include "disp_video.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_scaler.h"
#include "disp_de.h"
#include "disp_clk.h"

frame_para_t g_video[2][4];
static __u32 maf_flag_mem_len = 2048*2/8*544*2;
static void *maf_flag_mem[2][2];//2048*2/8 *544   * 2
static dit_mode_t dit_mode_default[2];

#define VIDEO_TIME_LEN 100
static unsigned long video_time_index[2][4] = {{0,0}};
static unsigned long video_time[2][4][VIDEO_TIME_LEN];//jiffies

static interlace_para_t g_interlace[2];

__s32 disp_video_checkin(__u32 sel, __u32 id);

#if 1
static __s32 video_enhancement_start(__u32 sel, __u32 id)
{
    if(gdisp.screen[sel].output_type == DISP_OUTPUT_TYPE_LCD)//!!! assume open HDMI before video start
    {
        BSP_disp_deu_enable(sel,IDTOHAND(id),TRUE);
        gdisp.screen[sel].layer_manage[id].video_enhancement_en = 1;
    }
    
    return 0;
}

static __s32 video_enhancement_stop(__u32 sel, __u32 id)
{
    if(gdisp.screen[sel].layer_manage[id].video_enhancement_en)
    {
        BSP_disp_deu_enable(sel,IDTOHAND(id),FALSE);

        gdisp.screen[sel].layer_manage[id].video_enhancement_en = 0;;
    }

	return 0;
}
#endif

static __inline __s32 Hal_Set_Frame(__u32 sel, __u32 tcon_index, __u32 id)
{	
    __u32 cur_line = 0, start_delay = 0;
    unsigned long flags;

    cur_line = tcon_get_cur_line(sel, tcon_index);
    start_delay = tcon_get_start_delay(sel, tcon_index);
	if(cur_line > start_delay-5)
	{
	    //DE_INF("cur_line(%d) >= start_delay(%d)-3 in Hal_Set_Frame\n", cur_line, start_delay);
		return DIS_FAIL;
	}

#ifdef __LINUX_OSAL__
    spin_lock_irqsave(&g_video[sel][id].flag_lock, flags);
#endif
    if(g_video[sel][id].display_cnt == 0)
    {
	    g_video[sel][id].pre_frame_addr_luma = g_video[sel][id].video_cur.addr[0];
        g_video[sel][id].pre_frame_addr_chroma= g_video[sel][id].video_cur.addr[1];
        memcpy(&g_video[sel][id].video_cur, &g_video[sel][id].video_new, sizeof(__disp_video_fb_t));
        g_video[sel][id].cur_maf_flag_addr ^=  g_video[sel][id].pre_maf_flag_addr;
        g_video[sel][id].pre_maf_flag_addr ^=  g_video[sel][id].cur_maf_flag_addr;
        g_video[sel][id].cur_maf_flag_addr ^=  g_video[sel][id].pre_maf_flag_addr;
        disp_video_checkin(sel, id);
    }
    g_video[sel][id].display_cnt++;
#ifdef __LINUX_OSAL__
    spin_unlock_irqrestore(&g_video[sel][id].flag_lock, flags);
#endif
    if(gdisp.screen[sel].layer_manage[id].para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        __u32 scaler_index;
    	__scal_buf_addr_t scal_addr;
        __scal_src_size_t in_size;
        __scal_out_size_t out_size;
        __scal_src_type_t in_type;
        __scal_out_type_t out_type;
        __scal_scan_mod_t in_scan;
        __scal_scan_mod_t out_scan;
        __disp_scaler_t * scaler;
        __u32 pre_frame_addr_luma = 0, pre_frame_addr_chroma = 0;
        __u32 maf_linestride = 0;
        __u32 size;

        scaler_index = gdisp.screen[sel].layer_manage[id].scaler_index;
        
        scaler = &(gdisp.scaler[scaler_index]);

    	if(g_video[sel][id].video_cur.interlace == TRUE)
    	{
    	    if((!(gdisp.screen[sel].de_flicker_status & DE_FLICKER_USED)) && 
    	        (scaler->in_fb.format == DISP_FORMAT_YUV420 && scaler->in_fb.mode == DISP_MOD_MB_UV_COMBINED)
    	        && (dit_mode_default[scaler_index] != 0xff) && (scaler->in_fb.size.width < 1920))//todo , full size of 3d mode < 1920
    	    {
    		    g_video[sel][id].dit_enable = TRUE;
    		}else
            {
                g_video[sel][id].dit_enable = FALSE;
            }

            g_video[sel][id].fetch_field = FALSE;
        	if(g_video[sel][id].display_cnt == 0)
        	{
        	    g_video[sel][id].fetch_bot = (g_video[sel][id].video_cur.top_field_first)?0:1;
        	}
        	else
        	{
        		g_video[sel][id].fetch_bot = (g_video[sel][id].video_cur.top_field_first)?1:0;
        	}

    		if(g_video[sel][id].dit_enable == TRUE)
    		{
				g_video[sel][id].dit_mode = dit_mode_default[scaler_index];
        		maf_linestride = (((scaler->src_win.width + 31) & 	0xffffffe0)*2/8 + 31) & 0xffffffe0; 
        		// //g_video[sel][id].video_cur.flag_stride;//todo? ( (£¨720 + 31£©&0xffffffe0 ) * 2/8  + 31) & 0xffffffe0

    			if(g_video[sel][id].video_cur.pre_frame_valid == TRUE)
    			{
                    g_video[sel][id].tempdiff_en = TRUE;
                    pre_frame_addr_luma= (__u32)OSAL_VAtoPA((void*)g_video[sel][id].pre_frame_addr_luma);
                    pre_frame_addr_chroma= (__u32)OSAL_VAtoPA((void*)g_video[sel][id].pre_frame_addr_chroma);
    			}
    			else
    			{
    				g_video[sel][id].tempdiff_en = FALSE;
    			}
    			g_video[sel][id].diagintp_en = TRUE;
    		}
    		else
    		{
        	    g_video[sel][id].dit_mode = DIT_MODE_WEAVE;
        	    g_video[sel][id].tempdiff_en = FALSE;
        	    g_video[sel][id].diagintp_en = FALSE;
    		}
    	}
    	else
    	{
    		g_video[sel][id].dit_enable = FALSE;
    	    g_video[sel][id].fetch_field = FALSE;
    	    g_video[sel][id].fetch_bot = FALSE;
    	    g_video[sel][id].dit_mode = DIT_MODE_WEAVE;
    	    g_video[sel][id].tempdiff_en = FALSE;
    	    g_video[sel][id].diagintp_en = FALSE;
    	}
        
    	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
        in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
        in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
    	in_type.byte_seq = 0;
    	in_type.sample_method = 0;

    	scal_addr.ch0_addr= (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr[0]));
    	scal_addr.ch1_addr= (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr[1]));
    	scal_addr.ch2_addr= (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr[2]));

    	in_size.src_width = scaler->in_fb.size.width;
    	in_size.src_height = scaler->in_fb.size.height;
    	in_size.x_off =  scaler->src_win.x;
    	in_size.y_off =  scaler->src_win.y;
    	in_size.scal_height=  scaler->src_win.height;
    	in_size.scal_width=  scaler->src_win.width;

    	out_type.byte_seq =  scaler->out_fb.seq;
    	out_type.fmt =  scaler->out_fb.format;

    	out_size.width =  scaler->out_size.width;
    	out_size.height =  scaler->out_size.height;

    	in_scan.field = g_video[sel][id].fetch_field;
    	in_scan.bottom = g_video[sel][id].fetch_bot;

    	out_scan.field = (gdisp.screen[sel].de_flicker_status & DE_FLICKER_USED)?0: gdisp.screen[sel].b_out_interlace;
        
    	if(scaler->out_fb.cs_mode > DISP_VXYCC)
    	{
    		scaler->out_fb.cs_mode = DISP_BT601;
    	}

        if(scaler->in_fb.b_trd_src)
        {
            __scal_3d_inmode_t inmode;
            __scal_3d_outmode_t outmode = 0;
            __scal_buf_addr_t scal_addr_right;

            inmode = Scaler_3d_sw_para_to_reg(0, scaler->in_fb.trd_mode, 0);
            outmode = Scaler_3d_sw_para_to_reg(1, scaler->out_trd_mode, gdisp.screen[sel].b_out_interlace);
            
            DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
            if(scaler->b_trd_out)
            {
                DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
            }

        	scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr_right[0]));
        	scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr_right[1]));
        	scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr_right[2]));

            DE_SCAL_Set_3D_Ctrl(scaler_index, scaler->b_trd_out, inmode, outmode);
            DE_SCAL_Config_3D_Src(scaler_index, &scal_addr, &in_size, &in_type, inmode, &scal_addr_right);
						DE_SCAL_Agth_Config(sel, &in_type, &in_size, &out_size, 0, scaler->b_trd_out, outmode);
        }
        else
        {
    	    DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
					DE_SCAL_Agth_Config(sel, &in_type, &in_size, &out_size, 0, 0, 0);
    	}

    	DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, g_video[sel][id].dit_enable);
    	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
    	//DE_SCAL_Set_Scaling_Coef_for_video(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, 0x00000101);
    	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);
    	DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);
    	DE_SCAL_Set_Di_Ctrl(scaler_index,g_video[sel][id].dit_enable,g_video[sel][id].dit_mode,g_video[sel][id].diagintp_en,g_video[sel][id].tempdiff_en);
    	DE_SCAL_Set_Di_PreFrame_Addr(scaler_index, pre_frame_addr_luma, pre_frame_addr_chroma);
			DE_SCAL_Set_Di_MafFlag_Src(scaler_index, OSAL_VAtoPA((void*)g_video[sel][id].cur_maf_flag_addr),
			    OSAL_VAtoPA((void*)g_video[sel][id].pre_maf_flag_addr), maf_linestride);

        if(g_video[sel][id].display_cnt == 0)
        {
            size = (scaler->in_fb.size.width * scaler->src_win.height * de_format_to_bpp(scaler->in_fb.format) + 7)/8;
            //OSAL_CacheRangeFlush((void *)scal_addr.ch0_addr,size ,CACHE_CLEAN_FLUSH_D_CACHE_REGION);
        }
        
        DE_SCAL_Set_Reg_Rdy(scaler_index);
    }
    else
    {
        __layer_man_t * layer_man;
        __disp_fb_t fb;
        layer_src_t layer_fb;

        memset(&layer_fb, 0, sizeof(layer_src_t));
        layer_man = &gdisp.screen[sel].layer_manage[id];

        BSP_disp_layer_get_framebuffer(sel, id, &fb);
        fb.addr[0] = (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr[0]));
        fb.addr[1] = (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr[1]));
        fb.addr[2] = (__u32)OSAL_VAtoPA((void*)(g_video[sel][id].video_cur.addr[2]));

    	if(get_fb_type(fb.format) == DISP_FB_TYPE_YUV)
    	{
        	Yuv_Channel_adjusting(sel , fb.mode, fb.format, &layer_man->para.src_win.x, &layer_man->para.scn_win.width);
    		Yuv_Channel_Set_framebuffer(sel, &fb, layer_man->para.src_win.x, layer_man->para.src_win.y);
    	}
    	else
    	{        	    
            layer_fb.fb_addr    = (__u32)OSAL_VAtoPA((void*)fb.addr[0]);
            layer_fb.pixseq     = img_sw_para_to_reg(3,0,fb.seq);
            layer_fb.br_swap    = fb.br_swap;
            layer_fb.fb_width   = fb.size.width;
            layer_fb.offset_x   = layer_man->para.src_win.x;
            layer_fb.offset_y   = layer_man->para.src_win.y;
            layer_fb.format = fb.format;
            DE_BE_Layer_Set_Framebuffer(sel, id,&layer_fb);
        }
    }
    gdisp.screen[sel].layer_manage[id].para.fb.addr[0] = g_video[sel][id].video_cur.addr[0];
    gdisp.screen[sel].layer_manage[id].para.fb.addr[1] = g_video[sel][id].video_cur.addr[1];
    gdisp.screen[sel].layer_manage[id].para.fb.addr[2] = g_video[sel][id].video_cur.addr[2];
    gdisp.screen[sel].layer_manage[id].para.fb.trd_right_addr[0] = g_video[sel][id].video_cur.addr_right[0];
    gdisp.screen[sel].layer_manage[id].para.fb.trd_right_addr[1] = g_video[sel][id].video_cur.addr_right[1];
    gdisp.screen[sel].layer_manage[id].para.fb.trd_right_addr[2] = g_video[sel][id].video_cur.addr_right[2];
    
	return DIS_SUCCESS;
}


__s32 Video_Operation_In_Vblanking(__u32 sel, __u32 tcon_index)
{	
    __u32 id=0;

#if 0
	for(id = 0; id<4; id++)
	{
		if((g_video[sel][id].enable == TRUE) && (g_video[sel][id].have_got_frame == TRUE))
		{
			Hal_Set_Frame(sel, tcon_index, id);
		}
	}
#else  //de-interlace
	for(id = 0; id < 2; id++)
	{
		if((g_interlace[id].enable) && (gdisp.scaler[id].status & SCALER_USED))
		{
			unsigned long flags;
			dit_mode_t dit_mod;
			__u32 maf_linestride = 0;
			__u32 pre_frame_addr_luma = 0;
			__u32 pre_frame_addr_chroma = 0;
			__scal_src_type_t in_type;
			__scal_out_type_t out_type;
			__scal_src_size_t in_size;
			__scal_out_size_t out_size;
			__scal_scan_mod_t in_scan;
			__scal_scan_mod_t out_scan;
			__disp_scaler_t *scaler = &(gdisp.scaler[id]);
			__scal_buf_addr_t scal_addr;
			__bool fetch_bot;
			__bool  tempdiff_en;
			__bool  diagintp_en;

#ifdef __LINUX_OSAL__
			spin_lock_irqsave(&g_video[sel][0].flag_lock, flags);
#endif
			fetch_bot = scaler->in_fb.b_top_field_first ^ g_interlace[id].first_field;
			g_interlace[id].first_field = false;
#ifdef __LINUX_OSAL__
			spin_unlock_irqrestore(&g_video[sel][0].flag_lock, flags);
#endif

			dit_mod = dit_mode_default[id];
			maf_linestride = (((scaler->src_win.width + 31) & 0xffffffe0)*2/8 + 31) & 0xffffffe0;

			if(!g_interlace[id].first_frame)
			{
				if(dit_mod == DIT_MODE_MAF)
				{
					pre_frame_addr_luma = g_interlace[id].pre_frame_addr_luma;
					pre_frame_addr_chroma = g_interlace[id].pre_frame_addr_chroma;
					tempdiff_en = true;
					diagintp_en = true;
				}
				else
				{
					tempdiff_en =  false;
					diagintp_en = false;
				}
			}
			else
			{
				tempdiff_en =  false;
				g_interlace[id].first_frame = false;
			}

			in_type.fmt = Scaler_sw_para_to_reg(0,scaler->in_fb.mode,scaler->in_fb.format,scaler->in_fb.seq);
			in_type.mod = Scaler_sw_para_to_reg(1,scaler->in_fb.mode,scaler->in_fb.format,scaler->in_fb.seq);
			in_type.ps = Scaler_sw_para_to_reg(2,scaler->in_fb.mode,scaler->in_fb.format,scaler->in_fb.seq);
			in_type.byte_seq = 0;
			in_type.sample_method = 0;

			in_scan.field = false;
			in_scan.bottom = fetch_bot;
			out_scan.field = (gdisp.screen[sel].de_flicker_status & DE_FLICKER_USED) ?
				0: gdisp.screen[sel].b_out_interlace;

			in_size.src_width = scaler->in_fb.size.width;
			in_size.src_height = scaler->in_fb.size.height;
			in_size.x_off =  scaler->src_win.x;
			in_size.y_off =  scaler->src_win.y;
			in_size.scal_height=  scaler->src_win.height;
			in_size.scal_width=  scaler->src_win.width;

			out_type.byte_seq =  scaler->out_fb.seq;
			out_type.fmt =  scaler->out_fb.format;

			out_size.width =  scaler->out_size.width;
			out_size.height =  scaler->out_size.height;

			scal_addr.ch0_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[0]));
			scal_addr.ch1_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[1]));
			scal_addr.ch2_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[2]));

			DE_SCAL_Config_Src(id,&scal_addr,&in_size,&in_type,FALSE,FALSE);
			DE_SCAL_Agth_Config(sel, &in_type, &in_size, &out_size, 0, 0, 0);

			DE_SCAL_Set_Init_Phase(id, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, 1);
			DE_SCAL_Set_Scaling_Factor(id, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
			//DE_SCAL_Set_Scaling_Coef_for_video(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, 0x00000101);
			DE_SCAL_Set_Scaling_Coef(id, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);
			DE_SCAL_Set_Out_Size(id, &out_scan,&out_type, &out_size);
			DE_SCAL_Set_Di_Ctrl(id,1,dit_mod,diagintp_en,tempdiff_en);
			DE_SCAL_Set_Di_PreFrame_Addr(id,pre_frame_addr_luma,pre_frame_addr_chroma);
			DE_SCAL_Set_Di_MafFlag_Src(id,g_interlace[id].cur_maf_flag_addr,
				g_interlace[id].pre_maf_flag_addr,maf_linestride);
		}
		else
		{
			DE_SCAL_Set_Di_Ctrl(id,0,DIT_MODE_WEAVE,0,0);
		}
	}

#endif
	return DIS_SUCCESS;
}

__s32 BSP_disp_video_set_fb(__u32 sel, __u32 hid, __disp_video_fb_t *in_addr)
{
    unsigned long flags;
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(g_video[sel][hid].enable)
    {
    	memcpy(&g_video[sel][hid].video_new, in_addr, sizeof(__disp_video_fb_t));
    	g_video[sel][hid].have_got_frame = TRUE;
#ifdef __LINUX_OSAL__
    spin_lock_irqsave(&g_video[sel][hid].flag_lock, flags);
#endif
	    g_video[sel][hid].display_cnt = 0;
#ifdef __LINUX_OSAL__
    spin_unlock_irqrestore(&g_video[sel][hid].flag_lock, flags);
#endif

    	return DIS_SUCCESS;
    }
    else
    {
        return DIS_FAIL;
    }
}


__s32 BSP_disp_video_get_frame_id(__u32 sel, __u32 hid)//get the current displaying frame id
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(g_video[sel][hid].enable)
    {
        if(g_video[sel][hid].have_got_frame == TRUE)
        {
            return g_video[sel][hid].video_cur.id;
        }
        else
        {
            return DIS_FAIL;
        }
    }
    else
    {
        return DIS_FAIL;
    }
}

__s32 BSP_disp_video_get_dit_info(__u32 sel, __u32 hid, __disp_dit_info_t * dit_info)
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(g_video[sel][hid].enable)
    {
    	dit_info->maf_enable = FALSE;
    	dit_info->pre_frame_enable = FALSE;
    	
    	if(g_video[sel][hid].dit_enable)
    	{
    		if(g_video[sel][hid].dit_mode == DIT_MODE_MAF)
    		{
    			dit_info->maf_enable = TRUE;
    		}
    		if(g_video[sel][hid].tempdiff_en)
    		{	
    			dit_info->pre_frame_enable = TRUE;
    		}
    	}
    	return DIS_SUCCESS;
	}
    else
    {
        return DIS_FAIL;
    }
}

__s32 BSP_disp_video_start(__u32 sel, __u32 hid)
{
    __layer_man_t *layer_man;
    
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && (!g_video[sel][hid].enable))
    {
        memset(&g_video[sel][hid], 0, sizeof(frame_para_t));
        g_video[sel][hid].video_cur.id = -1;
        g_video[sel][hid].enable = TRUE;

        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            g_video[sel][hid].cur_maf_flag_addr = (__u32)maf_flag_mem[layer_man->scaler_index][0];
            g_video[sel][hid].pre_maf_flag_addr = (__u32)maf_flag_mem[layer_man->scaler_index][1];
        }
        Disp_drc_start_video_mode(sel);
        video_enhancement_start(sel,hid);
        if(sel == 1)
        {
            //disp_clk_adjust(0, 0);
        }
    	return DIS_SUCCESS;
    }
    else
    {
        return DIS_FAIL;
    }
}

__s32 BSP_disp_video_stop(__u32 sel, __u32 hid)
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(g_video[sel][hid].enable)
    {
        memset(&g_video[sel][hid], 0, sizeof(frame_para_t));
        
        Disp_drc_start_ui_mode(sel);
        video_enhancement_stop(sel,hid);
        if(sel == 1)
        {
            //disp_clk_adjust(0, 1);
        }
    	return DIS_SUCCESS;
    }
    else
    {
        return DIS_FAIL;
    }
}

__s32 BSP_disp_video_get_start(__u32 sel, __u32 hid)
{
    __layer_man_t *layer_man;

    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];

    return ((layer_man->status & LAYER_USED) && (g_video[sel][hid].enable));
}

__s32 disp_video_init()
{
    memset(g_video,0,sizeof(g_video));
#ifdef __LINUX_OSAL__
    maf_flag_mem[0][0] = (void*)__pa((char __iomem *)kmalloc(maf_flag_mem_len, GFP_KERNEL |  __GFP_ZERO));
    maf_flag_mem[0][1] = (void*)__pa((char __iomem *)kmalloc(maf_flag_mem_len, GFP_KERNEL |  __GFP_ZERO));
    maf_flag_mem[1][0] = (void*)__pa((char __iomem *)kmalloc(maf_flag_mem_len, GFP_KERNEL |  __GFP_ZERO));
    maf_flag_mem[1][1] = (void*)__pa((char __iomem *)kmalloc(maf_flag_mem_len, GFP_KERNEL |  __GFP_ZERO));
    if((maf_flag_mem[0][0] == NULL) || (maf_flag_mem[0][0] == NULL) ||
        (maf_flag_mem[0][0] == NULL) || (maf_flag_mem[0][0] == NULL))
    {
        DE_WRN("disp_video_init, maf memory request fail\n");
    }
    spin_lock_init(&g_video[0][0].flag_lock);
    spin_lock_init(&g_video[0][1].flag_lock);
    spin_lock_init(&g_video[0][2].flag_lock);
    spin_lock_init(&g_video[0][3].flag_lock);
    spin_lock_init(&g_video[1][0].flag_lock);
    spin_lock_init(&g_video[1][1].flag_lock);
    spin_lock_init(&g_video[1][2].flag_lock);
    spin_lock_init(&g_video[1][3].flag_lock);
#endif
    dit_mode_default[0] = DIT_MODE_MAF;
    dit_mode_default[1] = DIT_MODE_MAF;
	memset(g_interlace, 0, sizeof(g_interlace));
	return DIS_SUCCESS;
}

__s32 disp_video_exit()
{
#ifdef __LINUX_OSAL__
    kfree(maf_flag_mem[0][0]);
    kfree(maf_flag_mem[0][1]);
    kfree(maf_flag_mem[1][0]);
    kfree(maf_flag_mem[1][1]);
#endif
    memset(g_video,0,sizeof(g_video));
    
	return DIS_SUCCESS;
}

__s32 disp_video_set_dit_mode(__u32 scaler_index, __u32 mode)
{
    dit_mode_default[scaler_index] = mode;
    return DIS_SUCCESS;
}

__s32 disp_video_get_dit_mode(__u32 scaler_index)
{
     return dit_mode_default[scaler_index];
}

__s32 BSP_disp_video_get_fb(__u32 sel, __u32 hid, __disp_video_fb_t *in_addr)
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(g_video[sel][hid].enable)
    {
    	memcpy(in_addr, &g_video[sel][hid].video_new, sizeof(__disp_video_fb_t));

    	return DIS_SUCCESS;
    }
    else
    {
        return DIS_FAIL;
    }
}

//return 10fps
__s32 bsp_disp_video_get_fps(__u32 sel, __u32 hid)
{
    __u32 pre_time_index, cur_time_index;
    __u32 pre_time, cur_time;
    __u32 fps = 0xff;

    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    pre_time_index = video_time_index[sel][hid];
    cur_time_index = (pre_time_index == 0)? (VIDEO_TIME_LEN -1):(pre_time_index-1);

    pre_time = video_time[sel][hid][pre_time_index];
    cur_time = video_time[sel][hid][cur_time_index];

    if(pre_time != cur_time)
    {
        fps = 1000 * 100 / (cur_time - pre_time);
    }
    
    return fps;
}

__s32 disp_video_checkin(__u32 sel, __u32 id)
{
    video_time[sel][id][video_time_index[sel][id]] = jiffies;
    video_time_index[sel][id] ++;

    video_time_index[sel][id] = (video_time_index[sel][id] >= VIDEO_TIME_LEN)? 0:video_time_index[sel][id];

    return 0;
}

void disp_set_interlace_info(__u32 sel, __u32 scaler_id, __u32 layer_id, __disp_scaler_t *scaler)
{
	unsigned long flags;

	if((false == scaler->in_fb.b_interlace)
		|| (scaler->in_fb.format != DISP_FORMAT_YUV420)
		|| ((scaler->in_fb.mode != DISP_MOD_MB_UV_COMBINED)
		    && (scaler->in_fb.mode != DISP_MOD_NON_MB_UV_COMBINED))
		|| (gdisp.screen[sel].de_flicker_status & DE_FLICKER_USED)
		|| (dit_mode_default[scaler_id] == 0xff)
		//|| ((scaler->in_fb.b_trd_src == 1) && (scaler->b_trd_out == 1) && (scaler->in_fb.size.width < 1920))
		)
	{
		scaler->in_fb.b_interlace = false;
		g_interlace[scaler_id].enable = false;
		return;
	}

	if(false != g_interlace[scaler_id].enable) //not the first interlace frame
	{
		if(g_interlace[scaler_id].cur_frame_addr_luma != scaler->in_fb.addr[0]) //different frame
		{
#ifdef __LINUX_OSAL__
			spin_lock_irqsave(&g_video[sel][0].flag_lock, flags);
#endif
			g_interlace[scaler_id].layer_id = layer_id;
			g_interlace[scaler_id].pre_frame_addr_luma = g_interlace[scaler_id].cur_frame_addr_luma;
			g_interlace[scaler_id].cur_frame_addr_luma = scaler->in_fb.addr[0];
			g_interlace[scaler_id].pre_frame_addr_chroma = g_interlace[scaler_id].cur_frame_addr_chroma;
			g_interlace[scaler_id].cur_frame_addr_chroma= scaler->in_fb.addr[1];
			g_interlace[scaler_id].cur_maf_flag_addr ^=  g_interlace[scaler_id].pre_maf_flag_addr;
			g_interlace[scaler_id].pre_maf_flag_addr ^=  g_interlace[scaler_id].cur_maf_flag_addr;
			g_interlace[scaler_id].cur_maf_flag_addr ^=  g_interlace[scaler_id].pre_maf_flag_addr;
			g_interlace[scaler_id].first_field = true;
#ifdef __LINUX_OSAL__
			spin_unlock_irqrestore(&g_video[sel][0].flag_lock, flags);
#endif
		}
	}
	else  // the first interlace frame
	{
#ifdef __LINUX_OSAL__
		spin_lock_irqsave(&g_video[sel][0].flag_lock, flags);
#endif
		g_interlace[scaler_id].layer_id = layer_id;
		g_interlace[scaler_id].cur_frame_addr_luma = //scaler->in_fb.addr[0];
			g_interlace[scaler_id].pre_frame_addr_luma = scaler->in_fb.addr[0];
		g_interlace[scaler_id].cur_frame_addr_chroma = //scaler->in_fb.addr[1];
			g_interlace[scaler_id].pre_frame_addr_chroma = scaler->in_fb.addr[1];
		g_interlace[scaler_id].pre_maf_flag_addr = 
			((__u32)maf_flag_mem[scaler_id][0] > 0x40000000) ? 
			((__u32)maf_flag_mem[scaler_id][0] - 0x40000000) : 
			(__u32)maf_flag_mem[scaler_id][0];
		g_interlace[scaler_id].cur_maf_flag_addr = 
			((__u32)maf_flag_mem[scaler_id][1] > 0x40000000) ? 
			((__u32)maf_flag_mem[scaler_id][1] - 0x40000000) : 
			(__u32)maf_flag_mem[scaler_id][1];
		g_interlace[scaler_id].first_field = true;
		g_interlace[scaler_id].first_frame = true;
		g_interlace[scaler_id].enable = true;
#ifdef __LINUX_OSAL__
		spin_unlock_irqrestore(&g_video[sel][0].flag_lock, flags);
#endif
	}
}

