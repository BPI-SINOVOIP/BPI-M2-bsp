#include "disp_layer.h"
#include "disp_de.h"
#include "disp_display.h"
#include "disp_scaler.h"
#include "disp_event.h"
#include "disp_clk.h"

__u32 layer_offset = 0;

static __s32 Layer_Get_Idle_Hid(__u32 sel)
{
    __s32 i;

    for(i = 0;i<gdisp.screen[sel].max_layers;i++)
    {
        if(!(gdisp.screen[sel].layer_manage[i].status & LAYER_USED))
        {
            return i;
        }
    }

    return (__s32)DIS_NO_RES;
}


static __s32 Layer_Get_Idle_Prio(__u32 sel)
{
    __s32 i,j;

    for(i = 0;i < gdisp.screen[sel].max_layers;i++)//check every prio(0~MAX_LAYERS-1)
    {
        for(j = 0;j < gdisp.screen[sel].max_layers;j++)//check every layer
        {
            if(gdisp.screen[sel].layer_manage[j].para.prio == i)//the prio is used by a layer
            {
               break;
            }
            else if(j == gdisp.screen[sel].max_layers-1)//no layer use this prio
            {
                return i;
            }
        }
    }
    return DIS_PRIO_ERROR;
}


__u32 Layer_Get_Prio(__u32 sel, __u32 hid)
{
    if(gdisp.screen[sel].layer_manage[hid].status & LAYER_USED)
    {
        return gdisp.screen[sel].layer_manage[hid].para.prio;
    }

    return (__u32)DIS_PARA_FAILED;
}

__disp_pixel_type_t get_fb_type(__disp_pixel_fmt_t  format)
{
    if(format == DISP_FORMAT_YUV444 || format == DISP_FORMAT_YUV422 ||
        format == DISP_FORMAT_YUV420 || format == DISP_FORMAT_YUV411)
    {
        return DISP_FB_TYPE_YUV;
    }
    else
    {
        return DISP_FB_TYPE_RGB;
    }
}

// 0: yuv channel format
// 1: yuv channel pixel sequence
// 3: image0 pixel sequence
__s32 img_sw_para_to_reg(__u8 type, __u8 mode, __u8 value)
{
    if(type == 0)//yuv channel format
    {
        if(mode == DISP_MOD_NON_MB_PLANAR && value == DISP_FORMAT_YUV411)
        {
            return 0;
        }
        else if(mode == DISP_MOD_NON_MB_PLANAR && value == DISP_FORMAT_YUV422)
        {
            return 1;
        }
        else if(mode == DISP_MOD_NON_MB_PLANAR && value == DISP_FORMAT_YUV444)
        {
            return 2;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_FORMAT_YUV422)
        {
            return 3;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_FORMAT_YUV444)
        {
            return 4;
        }
	    else
	    {
	        DE_WRN("not supported yuv channel format:%d in img_sw_para_to_reg\n",value);
	        return 0;
	    }
    }
    else if(type == 1)//yuv channel pixel sequence
    {
        if(mode == DISP_MOD_NON_MB_PLANAR && value == DISP_SEQ_P3210)
        {
            return 0;
        }
        else if(mode == DISP_MOD_NON_MB_PLANAR && value == DISP_SEQ_P0123)
        {
            return 1;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_SEQ_UYVY)
        {
            return 0;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_SEQ_YUYV)
        {
            return 1;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_SEQ_VYUY)
        {
            return 2;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_SEQ_YVYU)
        {
            return 3;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_SEQ_AYUV)
        {
            return 0;
        }
        else if(mode == DISP_MOD_INTERLEAVED && value == DISP_SEQ_VUYA)
        {
            return 1;
        }
	    else
	    {
	        DE_WRN("not supported yuv channel pixel sequence:%d in img_sw_para_to_reg\n",value);
	        return 0;
	    }
    }    
    else if(type == 3)//image0 pixel sequence
    {
        if(value == DISP_SEQ_ARGB)
        {
            return 0;
        }
        else if(value == DISP_SEQ_BGRA)
        {
            return 2;
        }
        else if(value == DISP_SEQ_P10)
        {
            return 0;
        }
        else if(value == DISP_SEQ_P01)
        {
            return 1;
        }
        else if(value == DISP_SEQ_P3210)
        {
            return 0;
        }
        else if(value == DISP_SEQ_P0123)
        {
            return 1;
        }
        else if(value == DISP_SEQ_P76543210)
        {
            return 0;
        }
        else if(value == DISP_SEQ_P67452301)
        {
            return 1;
        }
        else if(value == DISP_SEQ_P10325476)
        {
            return 2;
        }
        else if(value == DISP_SEQ_P01234567)
        {
            return 3;
        }
        else if(value == DISP_SEQ_2BPP_BIG_BIG)
        {
            return 0;
        }
        else if(value == DISP_SEQ_2BPP_BIG_LITTER)
        {
            return 1;
        }
        else if(value == DISP_SEQ_2BPP_LITTER_BIG)
        {
            return 2;
        }
        else if(value == DISP_SEQ_2BPP_LITTER_LITTER)
        {
            return 3;
        }
        else if(value == DISP_SEQ_1BPP_BIG_BIG)
        {
            return 0;
        }
        else if(value == DISP_SEQ_1BPP_BIG_LITTER)
        {
            return 1;
        }
        else if(value == DISP_SEQ_1BPP_LITTER_BIG)
        {
            return 2;
        }
        else if(value == DISP_SEQ_1BPP_LITTER_LITTER)
        {
            return 3;
        }
	    else
	    {
	        DE_WRN("not supported image0 pixel sequence:%d in img_sw_para_to_reg\n",value);
	        return 0;
	    }
    }

    DE_WRN("not supported type:%d in img_sw_para_to_reg\n",type);
    return 0;
}

__s32 de_format_to_bpp(__disp_pixel_fmt_t fmt)
{
    switch(fmt)
    {
    case DISP_FORMAT_1BPP:
        return 1;
        
    case DISP_FORMAT_2BPP:
        return 2;
        
    case DISP_FORMAT_4BPP:
        return 4;
        
    case DISP_FORMAT_8BPP:
        return 8;
        
    case DISP_FORMAT_RGB655:
    case DISP_FORMAT_RGB565:
    case DISP_FORMAT_RGB556:
    case DISP_FORMAT_ARGB1555:
    case DISP_FORMAT_RGBA5551:
    case DISP_FORMAT_ARGB4444:
        return 16;
        
    case DISP_FORMAT_RGB888:
        return 24;
        
    case DISP_FORMAT_ARGB8888:
        return 32;
        
    case DISP_FORMAT_YUV444:
        return 24;
        
    case DISP_FORMAT_YUV422:
        return 16;
        
    case DISP_FORMAT_YUV420:
    case DISP_FORMAT_YUV411:
        return 12;
        
    case DISP_FORMAT_CSIRGB:
        return 32;//?
        
    default:
        return 0;
    }
}

static __s32 Yuv_Channel_Request(__u32 sel, __u8 hid)
{    
	if(!(gdisp.screen[sel].status & YUV_CH_USED))
	{
		DE_BE_YUV_CH_Enable(sel, TRUE);
		DE_BE_Layer_Yuv_Ch_Enable(sel, hid,TRUE);

		gdisp.screen[sel].layer_manage[hid].byuv_ch = TRUE;
		gdisp.screen[sel].status |= YUV_CH_USED;
		return DIS_SUCCESS;
	}
	return DIS_NO_RES;
}

static __s32 Yuv_Channel_Release(__u32 sel, __u8 hid)
{
    de_yuv_ch_src_t yuv_src;

    memset(&yuv_src, 0 ,sizeof(de_yuv_ch_src_t));
    DE_BE_YUV_CH_Set_Src(sel, &yuv_src);
    DE_BE_YUV_CH_Enable(sel, FALSE);
    DE_BE_Layer_Yuv_Ch_Enable(sel, hid,FALSE);

    gdisp.screen[sel].layer_manage[hid].byuv_ch = FALSE;
    gdisp.screen[sel].status &= YUV_CH_USED_MASK;

    return DIS_SUCCESS;
}

__s32 Yuv_Channel_Set_framebuffer(__u32 sel, __disp_fb_t * pfb, __u32 xoffset, __u32 yoffset)
{
    de_yuv_ch_src_t yuv_src;
    
    yuv_src.format = img_sw_para_to_reg(0,pfb->mode,pfb->format);
    yuv_src.mode = (__u8)pfb->mode;
    yuv_src.pixseq = img_sw_para_to_reg(1,pfb->mode,pfb->seq);
    yuv_src.ch0_base = (__u32)OSAL_VAtoPA((void*)pfb->addr[0]);
    yuv_src.ch1_base = (__u32)OSAL_VAtoPA((void*)pfb->addr[1]);
    yuv_src.ch2_base = (__u32)OSAL_VAtoPA((void*)pfb->addr[2]);
    yuv_src.line_width= pfb->size.width;
    yuv_src.offset_x = xoffset;
    yuv_src.offset_y = yoffset;
    yuv_src.cs_mode = pfb->cs_mode;
    DE_BE_YUV_CH_Set_Src(sel, &yuv_src);

    return DIS_SUCCESS;
}

__s32 Yuv_Channel_adjusting(__u32 sel, __u32 mode,__u32 format, __s32 *src_x, __u32 *scn_width)
{
	__u32 w_shift;
	__u32 reg_format;

	reg_format = img_sw_para_to_reg(0,mode,format);

	if(reg_format == 0x0) //planar yuv411
	{
		w_shift = 4;
	}
	else if(reg_format == 0x1) //planar yuv422
	{
		w_shift = 3;
	}
	else if(reg_format == 0x2) //planar yuv444
	{
		w_shift = 2;
	}
	else
	{
		w_shift = 0;
	}
	*src_x = (*src_x>>w_shift)<<w_shift;
	*scn_width = (*scn_width>>w_shift)<<w_shift;
		
	 return DIS_SUCCESS;
}

__s32 BSP_disp_layer_request(__u32 sel, __disp_layer_work_mode_t mode)
{
    __s32   hid;
    __s32   prio = 0;
    __u32   cpu_sr;
    __layer_man_t * layer_man;

    OSAL_IrqLock(&cpu_sr);
    hid = Layer_Get_Idle_Hid(sel);
    if(hid == DIS_NO_RES)
    {
        DE_WRN("all layer resource used!\n");
        OSAL_IrqUnLock(cpu_sr);
        return DIS_NULL;
    }
    prio=Layer_Get_Idle_Prio(sel);
	if(prio < 0)
	{
        DE_WRN("all layer prio used!\n");
        OSAL_IrqUnLock(cpu_sr);
		return DIS_NULL;
	}
	OSAL_IrqUnLock(cpu_sr);

    BSP_disp_cfg_start(sel);
    
    DE_BE_Layer_Enable(sel, hid, FALSE);
    DE_BE_Layer_Set_Prio(sel, hid,prio);
    DE_BE_Layer_Set_Work_Mode(sel, hid, DISP_LAYER_WORK_MODE_NORMAL);
    DE_BE_Layer_Video_Enable(sel, hid, FALSE);

    BSP_disp_cfg_finish(sel);
    
    OSAL_IrqLock(&cpu_sr);
    layer_man = &gdisp.screen[sel].layer_manage[hid];	
    memset(&layer_man->para,0,sizeof(__disp_layer_info_t));
    layer_man->para.mode = DISP_LAYER_WORK_MODE_NORMAL;
    layer_man->para.prio = prio;
    layer_man->byuv_ch = 0;
	layer_man->status = LAYER_USED;
	OSAL_IrqUnLock(cpu_sr);

    return IDTOHAND(hid);
}


__s32 BSP_disp_layer_release(__u32 sel, __u32 hid)
{
    __u32   cpu_sr;
    __layer_man_t * layer_man;
    layer_src_t layer_src;

    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    BSP_disp_cfg_start(sel);
    
    if(BSP_disp_video_get_start(sel,IDTOHAND(hid)))
    {
        BSP_disp_video_stop(sel, IDTOHAND(hid));
    }

    layer_man = &gdisp.screen[sel].layer_manage[hid];

    if(layer_man->status & LAYER_USED)
    {
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            if(layer_man->para.b_from_screen)
            {
                Image_close(1-sel);
                image_clk_off(1-sel, 1);
                gdisp.screen[1-sel].image_output_type = 0;
            }
          
            //pr_warn("BSP_disp_cmu_layer_enable, ====1======sel=%d,hid=%d\n",sel,hid);
            BSP_disp_cmu_layer_enable(sel, IDTOHAND(hid),FALSE);
            disp_cmu_layer_clear(sel);

            Scaler_Release(layer_man->scaler_index, TRUE);      /*release a scaler object */
            //pr_warn("BSP_disp_deu_disable, ====1======sel=%d,hid=%d\n",sel,hid);
            BSP_disp_deu_enable(sel, IDTOHAND(hid), FALSE);
            disp_deu_clear(sel, IDTOHAND(hid));
        }
        else
        {
        	if(layer_man->byuv_ch)
        	{
            	Yuv_Channel_Release(sel, hid);
            }
        }
    }
    
    memset(&layer_src, 0, sizeof(layer_src_t));
    DE_BE_Layer_Set_Framebuffer(sel, hid, &layer_src);

    memset(layer_man, 0 ,sizeof(__layer_man_t));
    layer_man->para.scn_win.width  = 0x1;
    layer_man->para.scn_win.height = 0x1;
    DE_BE_Layer_Enable(sel, hid, FALSE);
    DE_BE_Layer_Video_Enable(sel, hid, FALSE);
    DE_BE_Layer_Video_Ch_Sel(sel, hid, 0);
    DE_BE_Layer_Yuv_Ch_Enable(sel, hid,FALSE);
    DE_BE_Layer_Set_Screen_Win(sel, hid, &(layer_man->para.scn_win));
    DE_BE_Layer_Set_Prio(sel, hid, 0);
    DE_BE_Layer_Set_Pipe(sel, hid, 0);
    DE_BE_Layer_Alpha_Enable(sel, hid, FALSE);
    DE_BE_Layer_Set_Alpha_Value(sel, hid, 0);
    DE_BE_Layer_ColorKey_Enable(sel, hid, FALSE);

    BSP_disp_cfg_finish(sel);
    
    OSAL_IrqLock(&cpu_sr);
    layer_man->para.prio = IDLE_PRIO;
    layer_man->status &= LAYER_USED_MASK&LAYER_OPEN_MASK;
    OSAL_IrqUnLock(cpu_sr);

    return DIS_SUCCESS;
}

__s32 BSP_disp_layer_open(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
        if(!(layer_man->status & LAYER_OPENED))
        {
            BSP_disp_cfg_start(sel);
            DE_BE_Layer_Enable(sel, hid,TRUE);
            BSP_disp_cfg_finish(sel);
            layer_man->status |= LAYER_OPENED;
        }
        return DIS_SUCCESS;
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

__s32 BSP_disp_layer_close(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
        if(layer_man->status & LAYER_OPENED)
        {
            BSP_disp_cfg_start(sel);
            DE_BE_Layer_Enable(sel, hid,FALSE);
            BSP_disp_cfg_finish(sel);
            layer_man->status &= LAYER_OPEN_MASK;
        }
        return DIS_SUCCESS;
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

__s32 BSP_disp_layer_is_open(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->status & LAYER_OPENED)) {
		return 1;
	}	else {
		return 0;
	}
}

__s32 BSP_disp_layer_set_framebuffer(__u32 sel, __u32 hid, __disp_fb_t * pfb)//keep the src window offset x/y
{
    __s32           ret;
    layer_src_t     layer_fb;
    __u32           cpu_sr;
    __layer_man_t * layer_man;
    __u32 size;

    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(pfb == NULL)
    {
        return DIS_PARA_FAILED;
    }

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
    	BSP_disp_cfg_start(sel);
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            ret = Scaler_Set_Framebuffer(layer_man->scaler_index, pfb); 
            BSP_disp_cfg_finish(sel);
            return ret;
        }
        else
        {
        	if(get_fb_type(pfb->format) == DISP_FB_TYPE_YUV)
        	{
	            if(layer_man->byuv_ch==FALSE)
                {
            		ret = Yuv_Channel_Request(sel, hid);
            		if(ret != DIS_SUCCESS)
            		{
            		    DE_WRN("request yuv channel fail\n");
            		    BSP_disp_cfg_finish(sel);
            			return ret;
            		}
            	}
            	Yuv_Channel_adjusting(sel , pfb->mode, pfb->format, &layer_man->para.src_win.x, &layer_man->para.scn_win.width);
        		Yuv_Channel_Set_framebuffer(sel, pfb, layer_man->para.src_win.x, layer_man->para.src_win.y);
        	}
        	else
        	{        	    
                layer_fb.fb_addr    = (__u32)OSAL_VAtoPA((void*)pfb->addr[0]);
                layer_fb.pixseq     = img_sw_para_to_reg(3,0,pfb->seq);
                layer_fb.br_swap    = pfb->br_swap;
                layer_fb.fb_width   = pfb->size.width;
                layer_fb.offset_x   = layer_man->para.src_win.x;
                layer_fb.offset_y   = layer_man->para.src_win.y;
                layer_fb.format = pfb->format;
                layer_fb.pre_multiply = pfb->pre_multiply;
                DE_BE_Layer_Set_Framebuffer(sel, hid,&layer_fb);
            }

            OSAL_IrqLock(&cpu_sr);
            memcpy(&layer_man->para.fb,pfb,sizeof(__disp_fb_t));
            OSAL_IrqUnLock(cpu_sr);

            size = (pfb->size.width * layer_man->para.src_win.height * de_format_to_bpp(pfb->format) + 7)/8;
            OSAL_CacheRangeFlush((void *)pfb->addr[0],size ,CACHE_CLEAN_FLUSH_D_CACHE_REGION);

			if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
			{
            	gdisp.scaler[layer_man->scaler_index].b_reg_change = TRUE;
            }
			BSP_disp_cfg_finish(sel);
			
            return DIS_SUCCESS;
        }
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

__s32 BSP_disp_layer_get_framebuffer(__u32 sel, __u32 hid,__disp_fb_t * pfb)
{
    __layer_man_t * layer_man;
    
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(pfb == NULL)
    {
        return DIS_PARA_FAILED;
    }

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            return Scaler_Get_Framebuffer(layer_man->scaler_index, pfb);
        }
        else
        {
            memcpy(pfb,&layer_man->para.fb,sizeof(__disp_fb_t));
            return DIS_SUCCESS;
        }
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}


__s32 BSP_disp_layer_set_src_window(__u32 sel, __u32 hid,__disp_rect_t *regn)//if not scaler mode, ignore the src window width&height.
{
    __u32           cpu_sr;
    __layer_man_t * layer_man;

    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(regn == NULL)
    {
        return DIS_PARA_FAILED;
    }
	if(regn->width <= 0 || regn->height <= 0)
    {
        return DIS_PARA_FAILED;
    }

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
    	BSP_disp_cfg_start(sel);
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            __s32 ret = 0;
            
            ret = Scaler_Set_SclRegn(layer_man->scaler_index, regn);
            gdisp.scaler[layer_man->scaler_index].b_reg_change = TRUE;

            OSAL_IrqLock(&cpu_sr);
            layer_man->para.src_win.x = regn->x;
            layer_man->para.src_win.y = regn->y;
            layer_man->para.src_win.width = regn->width;
            layer_man->para.src_win.height = regn->height;
            OSAL_IrqUnLock(cpu_sr);

            BSP_disp_cfg_finish(sel);
            return ret;
        }
        else
        {
        	if(get_fb_type(layer_man->para.fb.format) == DISP_FB_TYPE_YUV)
        	{        
        		Yuv_Channel_adjusting(sel,layer_man->para.fb.mode, layer_man->para.fb.format, &layer_man->para.src_win.x, &layer_man->para.scn_win.width);
        		Yuv_Channel_Set_framebuffer(sel, &(layer_man->para.fb), regn->x, regn->y);
        	}
        	else
        	{
        	    layer_src_t layer_fb;
        	    
                layer_fb.fb_addr    = (__u32)OSAL_VAtoPA((void*)layer_man->para.fb.addr[0]);
                layer_fb.format     = layer_man->para.fb.format;
                layer_fb.pixseq     = img_sw_para_to_reg(3,0,layer_man->para.fb.seq);
                layer_fb.br_swap    = layer_man->para.fb.br_swap;
                layer_fb.fb_width   = layer_man->para.fb.size.width;
                layer_fb.offset_x   = regn->x;
                layer_fb.offset_y   = regn->y;
                layer_fb.format = layer_man->para.fb.format;
                layer_fb.pre_multiply = layer_man->para.fb.pre_multiply;

                DE_BE_Layer_Set_Framebuffer(sel, hid,&layer_fb);
            }

            OSAL_IrqLock(&cpu_sr);
            layer_man->para.src_win.x = regn->x;
            layer_man->para.src_win.y = regn->y;
            layer_man->para.src_win.width = regn->width;
            layer_man->para.src_win.height = regn->height;
            OSAL_IrqUnLock(cpu_sr);

		    BSP_disp_cfg_finish(sel);
		    
            return DIS_SUCCESS;
        }
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}


__s32 BSP_disp_layer_get_src_window(__u32 sel, __u32 hid,__disp_rect_t *regn)
{
    __layer_man_t * layer_man;

    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(regn == NULL)
    {
        DE_WRN("input parameter can't be null!\n");
        return DIS_PARA_FAILED;
    }

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            return Scaler_Get_SclRegn(layer_man->scaler_index, regn);
        }
        else
        {
            regn->x = layer_man->para.src_win.x;
            regn->y = layer_man->para.src_win.y;
            regn->width = layer_man->para.scn_win.width;
            regn->height = layer_man->para.scn_win.height;
            return 0;
        }
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

__s32 BSP_disp_layer_set_screen_window(__u32 sel, __u32 hid,__disp_rect_t * regn)
{
    __disp_rectsz_t      outsize;
    __u32           cpu_sr;
    __layer_man_t * layer_man;

    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(regn == NULL)
    {
    	DE_WRN("para is null in BSP_disp_layer_set_screen_window\n");
        return DIS_PARA_FAILED;
    }
	if(regn->width <= 0 || regn->height <= 0)
    {
        DE_WRN("width:%x,height:%x in BSP_disp_layer_set_screen_window\n", regn->width, regn->height);
        return DIS_PARA_FAILED;
    }
	
    layer_man = &gdisp.screen[sel].layer_manage[hid];    

    if(layer_man->status & LAYER_USED)
    {
    	BSP_disp_cfg_start(sel);
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            __s32           ret;

            //when scaler display on a interlace screen(480i, ntsc etc), scaler window must be even vertical offset
            regn->y &= ((gdisp.screen[sel].b_out_interlace== 1)?0xfffffffe:0xffffffff);
            
            outsize.height = regn->height;
            outsize.width = regn->width;

            ret = Scaler_Set_Output_Size(layer_man->scaler_index, &outsize);
            if(ret != DIS_SUCCESS)
            {
                DE_WRN("Scaler_Set_Output_Size fail!\n");
                BSP_disp_cfg_finish(sel);
                return ret;
            }
            if(BSP_disp_cmu_layer_get_enable(sel, IDTOHAND(hid)) ==1)
            {
                IEP_CMU_Set_Imgsize(sel, regn->width, regn->height);
            }
        }
        if(get_fb_type(layer_man->para.fb.format) == DISP_FB_TYPE_YUV && layer_man->para.mode != DISP_LAYER_WORK_MODE_SCALER)
        {
        	Yuv_Channel_adjusting(sel, layer_man->para.fb.mode, layer_man->para.fb.format, &layer_man->para.src_win.x , &regn->width);
        }
        DE_BE_Layer_Set_Screen_Win(sel, hid, regn);
	    OSAL_IrqLock(&cpu_sr);
	    layer_man->para.scn_win.x = regn->x;
	    layer_man->para.scn_win.y = regn->y;
	    layer_man->para.scn_win.width = regn->width;
	    layer_man->para.scn_win.height = regn->height;
	    OSAL_IrqUnLock(cpu_sr);

	    if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
	    {
	    	gdisp.scaler[layer_man->scaler_index].b_reg_change = TRUE;
	    }
		BSP_disp_cfg_finish(sel);
		
	    return DIS_SUCCESS;
    }
    else
    {
    	DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }

}


__s32 BSP_disp_layer_get_screen_window(__u32 sel, __u32 hid,__disp_rect_t *regn)
{
    __layer_man_t * layer_man;
    
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(regn==NULL)
    {
        return DIS_PARA_FAILED;
    }

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
		regn->x = layer_man->para.scn_win.x;
		regn->y = layer_man->para.scn_win.y;
		regn->width = layer_man->para.scn_win.width;
		regn->height = layer_man->para.scn_win.height;

        return DIS_SUCCESS;
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

    

__s32 BSP_disp_layer_set_para(__u32 sel, __u32 hid,__disp_layer_info_t *player)
{
    __s32 ret;
    __u32 cpu_sr;
    __layer_man_t * layer_man;
    __u32 prio_tmp = 0;
    __u32 size;    
        
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(player->b_from_screen)
    {
        player->mode = DISP_LAYER_WORK_MODE_SCALER;
    }
    
    if(layer_man->status & LAYER_USED)
    {
    	BSP_disp_cfg_start(sel);
        if(player->mode != DISP_LAYER_WORK_MODE_NORMAL || get_fb_type(player->fb.format) != DISP_FB_TYPE_YUV)
        {
            if(layer_man->byuv_ch)
            {
                Yuv_Channel_Release(sel, hid);
            }
        }
        if(player->mode != DISP_LAYER_WORK_MODE_SCALER)
        {
            if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
            {
                if(BSP_disp_cmu_layer_get_enable(sel, IDTOHAND(hid)) ==1)
                {
                    BSP_disp_cmu_layer_enable(sel, IDTOHAND(hid), FALSE);
                    disp_cmu_layer_clear(sel);
                }
                if(BSP_disp_deu_get_enable(sel,IDTOHAND(hid)) ==1)
                {
                    BSP_disp_deu_enable(sel,IDTOHAND(hid),FALSE);
                    disp_deu_clear(sel, IDTOHAND(hid));
                }
                Scaler_Release(layer_man->scaler_index, TRUE);
                DE_BE_Layer_Video_Enable(sel, hid, FALSE);
                DE_BE_Layer_Video_Ch_Sel(sel, hid, 0);
                layer_man->para.mode = DISP_LAYER_WORK_MODE_NORMAL;
            }
        }

        if(player->mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            __disp_scaler_t * scaler;

            if(layer_man->para.mode != DISP_LAYER_WORK_MODE_SCALER)
            {
        	    layer_src_t layer_src;

        	    ret = Scaler_Request(0xff);
        	    if(ret < 0)
        	    {
        	        DE_WRN("request scaler layer fail!\n");
        	        BSP_disp_cfg_finish(sel);
        	        return DIS_NO_RES;
        	    }
        	    DE_SCAL_Start(ret);

        	    DE_BE_Layer_Video_Enable(sel, hid, TRUE);   
        	    DE_BE_Layer_Video_Ch_Sel(sel, hid, ret);
                memset(&layer_src, 0, sizeof(layer_src_t));
                layer_src.format = DISP_FORMAT_ARGB8888;
                layer_src.br_swap = FALSE;
                layer_src.pixseq = DISP_SEQ_ARGB;
                DE_BE_Layer_Set_Framebuffer(sel, hid, &layer_src);

        	    layer_man->scaler_index = ret;
        	    layer_man->para.mode = DISP_LAYER_WORK_MODE_SCALER;
        	    gdisp.scaler[ret].screen_index = sel;
                gdisp.scaler[ret].layer_id = hid;
        	}
        	scaler = &(gdisp.scaler[layer_man->scaler_index]) ;

        	player->scn_win.y &= ((gdisp.screen[sel].b_out_interlace== 1)?0xfffffffe:0xffffffff);
            
            if(scaler->deu.enable)
            {
                scaler->out_fb.seq= DISP_SEQ_P3210;
                scaler->out_fb.format= DISP_FORMAT_YUV444;
                scaler->out_fb.mode = DISP_MOD_NON_MB_PLANAR;
            }else
            {
                scaler->out_fb.seq= DISP_SEQ_ARGB;
                scaler->out_fb.format= DISP_FORMAT_ARGB8888;//DISP_FORMAT_RGB888;//
                scaler->out_fb.mode = DISP_MOD_INTERLEAVED;
            }
/*            
            scaler->out_fb.seq= DISP_SEQ_ARGB;
            scaler->out_fb.format= DISP_FORMAT_RGB888;
 */
            scaler->out_size.height  = player->scn_win.height;
            scaler->out_size.width   = player->scn_win.width;
        	if(player->b_from_screen)
        	{
        	    scaler->src_win.x = 0;
        	    scaler->src_win.y = 0;
        	    scaler->src_win.width = BSP_disp_get_screen_width(1-sel);
        	    scaler->src_win.height = BSP_disp_get_screen_height(1-sel);
        	    scaler->in_fb.addr[0] = 0;
        	    scaler->in_fb.size.width = BSP_disp_get_screen_width(1-sel);
        	    scaler->in_fb.size.height = BSP_disp_get_screen_height(1-sel);
        	    scaler->in_fb.format = DISP_FORMAT_ARGB8888;
        	    scaler->in_fb.seq = DISP_SEQ_ARGB;
        	    scaler->in_fb.mode = DISP_MOD_INTERLEAVED;
        	    scaler->in_fb.br_swap = FALSE;
        	    scaler->in_fb.cs_mode = DISP_BT601;
        	    image_clk_on(sel, 0);
        	    Image_open(1 - sel);
        	    DE_BE_Output_Select(1-sel, 6+layer_man->scaler_index);
        	    DE_SCAL_Input_Select(layer_man->scaler_index, 6 + (1-sel));
        	    gdisp.screen[1-sel].image_output_type = IMAGE_OUTPUT_SCALER;
        	}
        	else
        	{
                scaler->src_win.x       = player->src_win.x;
                scaler->src_win.y       = player->src_win.y;
                scaler->src_win.width   = player->src_win.width;
                scaler->src_win.height  = player->src_win.height;
                memcpy(&scaler->in_fb, &player->fb, sizeof(__disp_fb_t));
                DE_SCAL_Input_Select(layer_man->scaler_index, 0);
            }
            scaler->b_trd_out = player->b_trd_out;
            scaler->out_trd_mode = player->out_trd_mode;
            DE_SCAL_Output_Select(layer_man->scaler_index, sel);
            disp_deu_output_select(sel, IDTOHAND(hid), sel);
            Scaler_Set_Para(layer_man->scaler_index, scaler);
            if(BSP_disp_cmu_layer_get_enable(sel, IDTOHAND(hid)) ==1)
            {
                IEP_CMU_Set_Imgsize(sel, player->scn_win.width, player->scn_win.height);
            }
            disp_set_interlace_info(sel, layer_man->scaler_index, hid, scaler);

        }
        else
        {
            if(get_fb_type(player->fb.format) == DISP_FB_TYPE_YUV)//yuv channel
            {
        	    if(layer_man->byuv_ch == FALSE)
        	    {
            	    __s32 err = 0;

            		err = Yuv_Channel_Request(sel, hid);
            		if(err != DIS_SUCCESS)
            		{
            		    DE_WRN("request yuv channel fail\n");
            		    BSP_disp_cfg_finish(sel);
            			return err;
            		}
        		}
        		Yuv_Channel_adjusting(sel, player->fb.mode, player->fb.format, &player->src_win.x, &player->scn_win.width);
        		Yuv_Channel_Set_framebuffer(sel, &(player->fb), player->src_win.x, player->src_win.y);
            }
            else//normal rgb
            {
        	    layer_src_t layer_fb;
        	    __u32 bpp, size;
        	    
                layer_fb.fb_addr    = (__u32)OSAL_VAtoPA((void*)player->fb.addr[0]);
                layer_fb.format = player->fb.format;
                layer_fb.pixseq     = img_sw_para_to_reg(3,0,player->fb.seq);
                layer_fb.br_swap    = player->fb.br_swap;
                layer_fb.fb_width   = player->fb.size.width;
                layer_fb.offset_x   = player->src_win.x;
                layer_fb.offset_y   = player->src_win.y;
                layer_fb.pre_multiply = player->fb.pre_multiply;

	            bpp = DE_BE_Format_To_Bpp(sel, layer_fb.format);
                size = (player->fb.size.width * player->scn_win.height * bpp + 7)/8;
                OSAL_CacheRangeFlush((void *)player->fb.addr[0], size,CACHE_CLEAN_FLUSH_D_CACHE_REGION);
                DE_BE_Layer_Set_Framebuffer(sel, hid,&layer_fb);
            }
        }

        DE_BE_Layer_Set_Work_Mode(sel, hid, player->mode);
        DE_BE_Layer_Set_Pipe(sel, hid, player->pipe);
        DE_BE_Layer_Alpha_Enable(sel, hid, player->alpha_en);
        DE_BE_Layer_Set_Alpha_Value(sel, hid, player->alpha_val);
        DE_BE_Layer_ColorKey_Enable(sel, hid, player->ck_enable);
        DE_BE_Layer_Set_Screen_Win(sel,hid,&player->scn_win);

        OSAL_IrqLock(&cpu_sr);
        prio_tmp = layer_man->para.prio;
        memcpy(&(layer_man->para),player,sizeof(__disp_layer_info_t));
        layer_man->para.prio = prio_tmp;//ignore the prio setting
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            layer_man->para.src_win.width = player->src_win.width;
            layer_man->para.src_win.height = player->src_win.height;
            layer_man->para.b_from_screen = player->b_from_screen;
        }
        OSAL_IrqUnLock(cpu_sr);

        size = (player->fb.size.width * player->src_win.height * de_format_to_bpp(player->fb.format) + 7)/8;
        OSAL_CacheRangeFlush((void *)player->fb.addr[0],size ,CACHE_CLEAN_FLUSH_D_CACHE_REGION);

        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
        	gdisp.scaler[layer_man->scaler_index].b_reg_change = TRUE;
        }
		BSP_disp_cfg_finish(sel);
		
        return DIS_SUCCESS;
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}


__s32 BSP_disp_layer_get_para(__u32 sel, __u32 hid,__disp_layer_info_t *player)//todo
{
    __layer_man_t * layer_man;

    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
		memcpy(player, &layer_man->para, sizeof(__disp_layer_info_t));

		return DIS_SUCCESS;
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

__s32 BSP_disp_layer_set_smooth(__u32 sel, __u32 hid, __disp_video_smooth_t  mode)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            BSP_disp_scaler_set_smooth(layer_man->scaler_index, mode);
            return DIS_SUCCESS;
        }
        else
        {
            DE_WRN("layer not scaler mode!\n");
            return DIS_NOT_SUPPORT;
        }
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

__s32 BSP_disp_layer_get_smooth(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if(layer_man->status & LAYER_USED)
    {
        if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
        {
            __s32 mode;
            mode = (__s32)BSP_disp_scaler_get_smooth(layer_man->scaler_index);
            return mode;
        }
        else
        {
            DE_WRN("layer not scaler mode!\n");
            return DIS_NOT_SUPPORT;
        }
    }
    else
    {
        DE_WRN("layer %d in screen %d not inited!\n", hid, sel);
        return DIS_OBJ_NOT_INITED;
    }
}

__s32 BSP_disp_layer_set_bright(__u32 sel, __u32 hid, __u32 bright)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        gdisp.scaler[layer_man->scaler_index].bright = bright;
        if(gdisp.scaler[layer_man->scaler_index].enhance_en == TRUE)
        {
            Scaler_Set_Enhance(layer_man->scaler_index, gdisp.scaler[layer_man->scaler_index].bright, gdisp.scaler[layer_man->scaler_index].contrast,
                                gdisp.scaler[layer_man->scaler_index].saturation, gdisp.scaler[layer_man->scaler_index].hue);
        }
        
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_get_bright(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        return gdisp.scaler[layer_man->scaler_index].bright;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_set_contrast(__u32 sel, __u32 hid, __u32 contrast)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        gdisp.scaler[layer_man->scaler_index].contrast = contrast;
        if(gdisp.scaler[layer_man->scaler_index].enhance_en == TRUE)
        {
            Scaler_Set_Enhance(layer_man->scaler_index, gdisp.scaler[layer_man->scaler_index].bright, gdisp.scaler[layer_man->scaler_index].contrast,
                                gdisp.scaler[layer_man->scaler_index].saturation, gdisp.scaler[layer_man->scaler_index].hue);
        }
        
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_get_contrast(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        return gdisp.scaler[layer_man->scaler_index].contrast;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_set_saturation(__u32 sel, __u32 hid, __u32 saturation)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        gdisp.scaler[layer_man->scaler_index].saturation = saturation;
        if(gdisp.scaler[layer_man->scaler_index].enhance_en == TRUE)
        {
            Scaler_Set_Enhance(layer_man->scaler_index, gdisp.scaler[layer_man->scaler_index].bright, gdisp.scaler[layer_man->scaler_index].contrast,
                                gdisp.scaler[layer_man->scaler_index].saturation, gdisp.scaler[layer_man->scaler_index].hue);
        }
        
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_get_saturation(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        return gdisp.scaler[layer_man->scaler_index].saturation;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_set_hue(__u32 sel, __u32 hid, __u32 hue)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        gdisp.scaler[layer_man->scaler_index].hue = hue;
        if(gdisp.scaler[layer_man->scaler_index].enhance_en == TRUE)
        {
            Scaler_Set_Enhance(layer_man->scaler_index, gdisp.scaler[layer_man->scaler_index].bright, gdisp.scaler[layer_man->scaler_index].contrast,
                                gdisp.scaler[layer_man->scaler_index].saturation, gdisp.scaler[layer_man->scaler_index].hue);
        }
        
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_get_hue(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        return gdisp.scaler[layer_man->scaler_index].hue;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_enhance_enable(__u32 sel, __u32 hid, __bool enable)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        if(enable == FALSE)
        {
            Scaler_Set_Enhance(layer_man->scaler_index, 32, 32,32, 32);
        }
        else
        {
            Scaler_Set_Enhance(layer_man->scaler_index, gdisp.scaler[layer_man->scaler_index].bright, gdisp.scaler[layer_man->scaler_index].contrast,
                                gdisp.scaler[layer_man->scaler_index].saturation, gdisp.scaler[layer_man->scaler_index].hue);
        }
        gdisp.scaler[layer_man->scaler_index].enhance_en = enable;
        return DIS_SUCCESS;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_layer_get_enhance_enable(__u32 sel, __u32 hid)
{
    __layer_man_t * layer_man;
    
    hid= HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    layer_man = &gdisp.screen[sel].layer_manage[hid];
    if((layer_man->status & LAYER_USED) && layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        return gdisp.scaler[layer_man->scaler_index].enhance_en;
    }
    return DIS_NOT_SUPPORT;
}

__s32 BSP_disp_get_layer_enable(__u32 sel)
{
    return gdisp.screen[sel].layer_en;
}

__s32 BSP_disp_layer_set_3Dlayer_offset(__u32 sel, __u32 offset)
{
	if(offset < 0 || offset > 1920){
		layer_offset = 0;
		return DIS_PARA_FAILED;
	}
	if(sel == 0)
		layer_offset = offset;

	return DIS_SUCCESS;
}
