/*
 * Sunxi Camera register read/write interface
 * Author:raymonxiu
*/

#include <linux/io.h>
#include <linux/delay.h>

#include "sunxi_csi_reg.h"
#include "../include/sunxi_csi_core.h"

unsigned int CSI_VBASE;

void bsp_csi_set_base_addr(unsigned int addr)
{
	CSI_VBASE = addr ;
}

/* open module */
void bsp_csi_open(void)
{
	S(CSI_VBASE+CSI_EN, 0x1);
}

void bsp_csi_close(void)
{
  C(CSI_VBASE+CSI_EN, 0x1);
}

/* configure */
void bsp_csi_if_configure(__csi_if_conf_t *csi_if_cfg)
{
	W(CSI_VBASE+CSI_IF_CFG, csi_if_cfg->src_type << 21 | 
							  csi_if_cfg->field_pol<< 19 | 
							  csi_if_cfg->vref << 18 | 
							  csi_if_cfg->href << 17 |
							  ((csi_if_cfg->clock==CSI_FALLING)?1:0)<< 16 |
							  csi_if_cfg->data_width << 8  | 
							  csi_if_cfg->csi_if << 0
      );
}

void bsp_csi_fmt_configure(unsigned int ch, __csi_fmt_conf_t *csi_fmt_cfg)
{
	W(CSI_VBASE+CSI_CFG, csi_fmt_cfg->input_fmt << 20 | 
							  csi_fmt_cfg->output_fmt<< 16 | 
							  csi_fmt_cfg->field_sel << 10 | 
							  csi_fmt_cfg->seq       << 8
      );
}

/* buffer */
void inline bsp_csi_set_buffer_address(unsigned int ch, __csi_buf_t buf, unsigned int addr)
{
	//bufer0a +4 = buffer0b, bufer0a +8 = buffer1a
    W(CSI_VBASE+CSI_BUF0_A + (buf<<2), addr); 
}

unsigned int inline bsp_csi_get_buffer_address(unsigned int ch, __csi_buf_t buf)
{
	unsigned int t;
	t = R(CSI_VBASE+CSI_BUF0_A + (buf<<2));
	return t;
}

/* capture */
void bsp_csi_capture_video_start(unsigned int ch)
{
    S(CSI_VBASE+CSI_CAP, 0X1<<1);
}

void bsp_csi_capture_video_stop(unsigned int ch)
{
    C(CSI_VBASE+CSI_CAP, 0X1<<1);
}

void bsp_csi_capture_picture(unsigned int ch)
{
    S(CSI_VBASE+CSI_CAP, 0X1<<0);
}

void bsp_csi_capture_get_status(unsigned int ch, __csi_capture_status * status)
{
    unsigned int t;
    t = R(CSI_VBASE+CSI_STATUS);
    status->picture_in_progress = t&0x1;
    status->video_in_progress   = (t>>1)&0x1;
}

/* size */
void bsp_csi_set_size(unsigned int ch, unsigned int length_h, unsigned int length_v, unsigned int buf_length_y, unsigned int buf_length_c)
{
	/* make sure yuv422 input 2 byte(clock) output 1 pixel */
		unsigned int t;
		
		t = R(CSI_VBASE+CSI_RESIZE_H);
		t = (t&0x0000ffff)|(length_h<<16);
    W(CSI_VBASE+CSI_RESIZE_H, t);
    
    t = R(CSI_VBASE+CSI_RESIZE_V);
    t = (t&0x0000ffff)|(length_v<<16);
    W(CSI_VBASE+CSI_RESIZE_V, t);
    
    W(CSI_VBASE+CSI_BUF_LENGTH, buf_length_y + (buf_length_c<<16));
}


/* offset */
void bsp_csi_set_offset(unsigned int ch, unsigned int start_h, unsigned int start_v)
{
    unsigned int t;
    
    t = R(CSI_VBASE+CSI_RESIZE_H);
    t = (t&0xffff0000)|start_h;
    W(CSI_VBASE+CSI_RESIZE_H, t);
    
    t = R(CSI_VBASE+CSI_RESIZE_V);
    t = (t&0xffff0000)|start_v;
    W(CSI_VBASE+CSI_RESIZE_V, t);
}


/* interrupt */
void bsp_csi_int_enable(unsigned int ch, __csi_int_t interrupt)
{
    S(CSI_VBASE+CSI_INT_EN, interrupt);
}

void bsp_csi_int_disable(unsigned int ch, __csi_int_t interrupt)
{
    C(CSI_VBASE+CSI_INT_EN, interrupt);
}

void inline bsp_csi_int_get_status(unsigned int ch, __csi_int_status_t * status)
{
    unsigned int t;
    t = R(CSI_VBASE+CSI_INT_STATUS);

    status->capture_done     = t&CSI_INT_CAPTURE_DONE;
    status->frame_done       = t&CSI_INT_FRAME_DONE;
    status->buf_0_overflow   = t&CSI_INT_BUF_0_OVERFLOW;
    status->buf_1_overflow   = t&CSI_INT_BUF_1_OVERFLOW;
    status->buf_2_overflow   = t&CSI_INT_BUF_2_OVERFLOW;
    status->protection_error = t&CSI_INT_PROTECTION_ERROR;
    status->hblank_overflow  = t&CSI_INT_HBLANK_OVERFLOW;
    status->vsync_trig		 = t&CSI_INT_VSYNC_TRIG;

}

void inline bsp_csi_int_clear_status(unsigned int ch, __csi_int_t interrupt)
{
    W(CSI_VBASE+CSI_INT_STATUS, interrupt);
}
