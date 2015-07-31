
/* 
 ***************************************************************************************
 * 
 * bsp_isp_comm.h
 * 
 * Hawkview ISP - bsp_isp_comm.h module
 * 
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * 
 * Version		  Author         Date		    Description
 * 
 *   1.0		Yang Feng   	2013/12/25	    First Version
 * 
 ****************************************************************************************
 */
 
#ifndef __BSP__ISP__COMM__H
#define __BSP__ISP__COMM__H

#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))

/*
 *  update table
 */  
#define SUN8I_ISP_LUT_UPDATE            (1 << 3)
#define SUN8I_ISP_LENS_UPDATE           (1 << 4)
#define SUN8I_ISP_GAMMA_UPDATE          (1 << 5)
#define SUN8I_ISP_DRC_UPDATE            (1 << 6)
#define SUN8I_ISP_TABLE_UPDATE_ALL      (SUN8I_ISP_LUT_UPDATE | SUN8I_ISP_LENS_UPDATE | \
											SUN8I_ISP_GAMMA_UPDATE | SUN8I_ISP_DRC_UPDATE)

/*
 *  ISP Module enable
 */
#define SUN8I_ISP_AE_EN                 (1 << 0)
#define SUN8I_ISP_OBC_EN                (1 << 1)
#define SUN8I_ISP_LUT_DPC_EN            (1 << 2)
#define SUN8I_ISP_OTF_DPC_EN            (1 << 3)
#define SUN8I_ISP_BDNF_EN               (1 << 4)
#define SUN8I_ISP_AWB_EN                (1 << 6)
#define SUN8I_ISP_WB_EN                 (1 << 7)
#define SUN8I_ISP_LSC_EN                (1 << 8)
#define SUN8I_ISP_BGC_EN                (1 << 9)
#define SUN8I_ISP_SAP_EN                (1 << 10)
#define SUN8I_ISP_AF_EN                 (1 << 11)
#define SUN8I_ISP_RGB2RGB_EN            (1 << 12)
#define SUN8I_ISP_RGB_DRC_EN            (1 << 13)
#define SUN8I_ISP_TDF_EN                (1 << 15)
#define SUN8I_ISP_AFS_EN                (1 << 16)
#define SUN8I_ISP_HIST_EN               (1 << 17)
#define SUN8I_ISP_YCbCr_GAIN_OFFSET_EN  (1 << 18)
#define SUN8I_ISP_YCbCr_DRC_EN          (1 << 19)
#define SUN8I_ISP_TG_EN                 (1 << 20)
#define SUN8I_ISP_ROT_EN                (1 << 21)
#define SUN8I_ISP_CONTRAST_EN           (1 << 22)
#define SUN8I_ISP_SRC1_EN               (1 << 30)
#define SUN8I_ISP_SRC0_EN               (1 << 31)
#define SUN8I_ISP_MODULE_EN_ALL         (0xffffffff)

/*
 *  ISP interrupt enable
 */ 
#define FINISH_INT_EN        (1 << 0)
#define START_INT_EN         (1 << 1)
#define PARA_SAVE_INT_EN     (1 << 2)
#define PARA_LOAD_INT_EN     (1 << 3)
#define SRC0_FIFO_INT_EN     (1 << 4)
#define SRC1_FIFO_INT_EN     (1 << 5)
#define ROT_FINISH_INT_EN    (1 << 6)

#define ISP_IRQ_EN_ALL       0x7f



enum isp_channel {
  MAIN_CH        = 0,
  SUB_CH         = 1,
  ROT_CH         = 2,
  ISP_MAX_CH_NUM ,
};


struct isp_size
{
  unsigned int width;
  unsigned int height;
};

struct coor
{
  unsigned int hor;
  unsigned int ver;
};


struct isp_int_status
{
  unsigned char finish_int;
  unsigned char start_int;
  unsigned char para_save_int;
  unsigned char para_load_int;
  unsigned char src0_fifo_int;
  unsigned char src1_fifo_int;
  unsigned char rot_finish_int;
};

enum ready_flag 
{
  PARA_NOT_READY    = 0,
  PARA_READY        = 1,
};

enum enable_flag 
{
  DISABLE    = 0,
  ENABLE     = 1,
};

/*
 *  ISP interrupt status
 */ 
enum isp_irq_status {
  FINISH_PD       = 0,
  START_PD        = 1,
  PARA_SAVE_PD    = 2,
  PARA_LOAD_PD    = 3,
  SRC0_FIFO_OF_PD = 4,
  SRC1_FIFO_OF_PD = 5,
  ROT_FINISH_PD   = 6,
};

enum isp_src_interface {
  ISP_DRAM = 0,
  ISP_CSI0 = 1,
  ISP_CSI1 = 2,
  ISP_CSI2 = 3,
};

enum isp_input_tables {
  LUT_LENS_GAMMA_TABLE = 0,
  DRC_TABLE = 1,
};

enum isp_capture_mode {
  SCAP_EN = 0,
  VCAP_EN = 1,
};

enum isp_src {
  ISP_SRC0 = 0,
  ISP_SRC1 = 1,
}; 

enum isp_hist_mode{
  AVG_MODE = 0,
  MIN_MODE = 1,
  MAX_MODE = 2,
};

enum isp_bndf_mode{
  NORM_MODE   = 0,
  STRONG_MODE = 1,
};

enum isp_rgb_drc_mode {
  INDEPEND = 0,
  COMBINED = 1,
};

enum isp_lut_dpc_mode {
  HOR     = 0,
  VER     = 1,
  HOR_VER = 2,
};

enum isp_input_fmt {
  ISP_YUV420 = 0,
  ISP_YUV422 = 1,
  ISP_RAW    = 2,
};

enum isp_input_seq {
  ISP_YUYV = 0,
  ISP_YVYU = 1,
  ISP_UYVY = 2,
  ISP_VYUY = 3,
  
  ISP_BGGR = 4,
  ISP_RGGB = 5,
  ISP_GBRG = 6,
  ISP_GRBG = 7,
};

enum isp_scale_mode {
  ORIGINAL   = 0,
  RATIO_1_2  = 2,
  RATIO_1_4  = 3,
};


enum isp_output_fmt{
  ISP_YUV420_SP = 0,
  ISP_YUV422_SP = 1,
  ISP_YUV420_P = 2,
  ISP_YUV422_P = 3,
};

enum isp_output_seq
{
  ISP_UV = 0,
  ISP_VU = 1,
};

enum isp_thumb_sel {
  NORM   = 0,
  THUMB  = 1,
};

enum isp_output_speed
{
  ISP_OUTPUT_SPEED_0 = 0,
  ISP_OUTPUT_SPEED_1 = 1,	  
  ISP_OUTPUT_SPEED_2 = 2,
  ISP_OUTPUT_SPEED_3 = 3,
};

struct isp_denoise 
{
  unsigned char in_dis_min;
  unsigned char in_dis_max;  
  int  denoise_table[12];//rbw[5]+gw[7]
};


struct isp_bayer_gain_offset
{
  unsigned short r_gain;    
  unsigned short gr_gain; 
  unsigned short gb_gain;
  unsigned short b_gain;

  short r_offset;   
  short gr_offset;
  short gb_offset;
  short b_offset;
};


struct isp_yuv_gain_offset {
  unsigned short y_gain;    
  unsigned short u_gain;  
  unsigned short v_gain;
  
  short y_offset;   
  short u_offset;
  short v_offset;
};


struct isp_white_balance_gain {
  unsigned short r_gain;    
  unsigned short gr_gain; 
  unsigned short gb_gain;
  unsigned short b_gain;
};

/**
 * struct isp_rgb2rgb_gain_offset - RGB to RGB Blending
 * @matrix: 
 *              [RR] [GR] [BR]
 *              [RG] [GG] [BG]
 *              [RB] [GB] [BB]
 * @offset: Blending offset value for R,G,B.
 */
struct isp_rgb2rgb_gain_offset{
  short matrix [3][3];
  short offset[3];  
};

struct isp_lsc_config {
  unsigned short ct_x;    
  unsigned short ct_y;  
  unsigned short rs_val;
};

struct isp_awb_avp_stat{
  unsigned int avp_r;   
  unsigned int avp_g; 
  unsigned int avp_b;
  unsigned int avp_cnt;
};

struct isp_wb_diff_threshold {
  unsigned int diff_th1;    
  unsigned int diff_th2;  
};

struct isp_h3a_reg_win {
  unsigned char hor_num;    
  unsigned char ver_num;  
  unsigned int width;
  unsigned int height;
  unsigned int hor_start;
  unsigned int ver_start;
};

struct isp_h3a_coor_win {
  int x1;
  int y1;
  int x2;
  int y2;
};

struct isp_yuv_channel_addr {
  unsigned int y_addr;
  unsigned int u_addr;
  unsigned int v_addr;
};
struct isp_3d_denoise_config
{
  unsigned short filter_ch_sel;
  unsigned short filter_slope0;
  unsigned short filter_slope1;
  unsigned short y_core0_val;
  unsigned short y_core1_val;
  unsigned short uv_core0_val;
  unsigned short uv_core1_val;
};

#endif //__BSP__ISP__COMM__H

