/*
 * set/get config from sysconfig and ini/bin file
 * Author: raymonxiu
 *
 */


#include "config.h"
#include "cfg_op.h"
#include "camera_detector/camera_detector.h"

#define SIZE_OF_LSC_TBL     7*768*2
#define SIZE_OF_HDR_TBL     4*256*2
#define SIZE_OF_GAMMA_TBL   256*2

#define _SUNXI_CAM_DETECT_

#ifdef _SUNXI_CAM_DETECT_
extern void camera_export_info(char *module_name, int *i2c_addr, int index);
#endif

struct camera_info {
    char   name[64];
    int  hfilp;
    int  vfilp;
    int  is_isp_used;
    int  is_raw_sensor;
};

struct camera_info camera_info_list[] =
{
	{"gc2235",0,0,1,1},
	{"ov5647",0,0,1,1},
	{"gc2035",0,0,0,0},
	{"gc0308",0,0,0,0},
	{"gc0307",0,0,0,0},
	{"hi257",0,0,0,0},
};
int fetch_config(struct vfe_dev *dev)
{
#ifndef FPGA_VER
  int ret;
  unsigned int i,j;
  char vfe_para[16] = {0};
  char dev_para[16] = {0};

  script_item_u   val;
  script_item_value_type_e	type;

  sprintf(vfe_para, "vip%d_para", dev->id);
  /* fetch device quatity issue */
  type = script_get_item(vfe_para,"vip_dev_qty", &val);
  if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	dev->dev_qty=1;
    vfe_err("fetch csi_dev_qty from sys_config failed\n");
  } else {
	  dev->dev_qty=val.val;
	  vfe_dbg(0,"vip%d_para vip_dev_qty=%d\n",dev->id, dev->dev_qty);
  }

  for(i=0; i<dev->dev_qty; i++)
  {
    /* i2c and module name*/
    sprintf(dev_para, "vip_dev%d_twi_id", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_err("fetch vip_dev%d_twi_id from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->twi_id = val.val;
    }

    ret = strcmp(dev->ccm_cfg[i]->ccm,"");
    if((dev->ccm_cfg[i]->i2c_addr == 0xff) && (ret == 0)) //when insmod without parm
    {
      sprintf(dev_para, "vip_dev%d_twi_addr", i);
      type = script_get_item(vfe_para, dev_para, &val);
      if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        vfe_err("fetch vip_dev%d_twi_addr from sys_config failed\n", i);
      } else {
        dev->ccm_cfg[i]->i2c_addr = val.val;
      }

      sprintf(dev_para, "vip_dev%d_mname", i);
      type = script_get_item(vfe_para, dev_para, &val);
      if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
        char tmp_str[]="ov5650";
        strcpy(dev->ccm_cfg[i]->ccm,tmp_str);
        vfe_err("fetch vip_dev%d_mname from sys_config failed\n", i);
      } else {
        strcpy(dev->ccm_cfg[i]->ccm,val.str);
      }
    }

    /* isp used mode */
    sprintf(dev_para, "vip_dev%d_isp_used", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
      vfe_dbg(0,"fetch vip_dev%d_isp_used from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->is_isp_used = val.val;
    }

    /* fmt */
    sprintf(dev_para, "vip_dev%d_fmt", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_fmt from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->is_bayer_raw = val.val;
    }

    /* standby mode */
    sprintf(dev_para, "vip_dev%d_stby_mode", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_stby_mode from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->power.stby_mode = val.val;
    }

    /* fetch flip issue */
    sprintf(dev_para, "vip_dev%d_vflip", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_vflip from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->vflip = val.val;
    }

    sprintf(dev_para, "vip_dev%d_hflip", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
      vfe_dbg(0,"fetch vip_dev%d_hflip from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->hflip = val.val;
    }

    /* fetch power issue*/
    sprintf(dev_para, "vip_dev%d_iovdd", i);
    type = script_get_item(vfe_para, dev_para, &val);

    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->iovdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_iovdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->iovdd_str,val.str);
    }

    sprintf(dev_para, "vip_dev%d_iovdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.iovdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_iovdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.iovdd_vol=val.val;
	  }

    sprintf(dev_para, "vip_dev%d_avdd", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->avdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_avdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->avdd_str,val.str);
    }

    sprintf(dev_para, "vip_dev%d_avdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.avdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_avdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.avdd_vol=val.val;
	  }

    sprintf(dev_para, "vip_dev%d_dvdd", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_STR != type){
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->dvdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_dvdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->dvdd_str, val.str);
    }

		sprintf(dev_para, "vip_dev%d_dvdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.dvdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_dvdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.dvdd_vol=val.val;
	  }

    sprintf(dev_para, "vip_dev%d_afvdd", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
      char null_str[]="";
      strcpy(dev->ccm_cfg[i]->afvdd_str,null_str);
      vfe_dbg(0,"fetch vip_dev%d_afvdd from sys_config failed\n", i);
    } else {
      strcpy(dev->ccm_cfg[i]->afvdd_str, val.str);
    }

	  sprintf(dev_para, "vip_dev%d_afvdd_vol", i);
    type = script_get_item(vfe_para,dev_para, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    dev->ccm_cfg[i]->power.afvdd_vol=0;
			vfe_dbg(0,"fetch vip_dev%d_afvdd_vol from sys_config failed, default =0\n",i);
		} else {
	    dev->ccm_cfg[i]->power.afvdd_vol=val.val;
	  }

    /* fetch reset/power/standby/flash/af io issue */
    sprintf(dev_para, "vip_dev%d_reset", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.reset_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_reset from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.reset_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.reset_io.mul_sel=val.gpio.mul_sel;
    }

    sprintf(dev_para, "vip_dev%d_pwdn", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type){
      dev->ccm_cfg[i]->gpio.pwdn_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_stby from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.pwdn_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.pwdn_io.mul_sel=val.gpio.mul_sel;
    }
    sprintf(dev_para, "vip_dev%d_power_en", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.power_en_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_power_en from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.power_en_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.power_en_io.mul_sel=val.gpio.mul_sel;
    }
    sprintf(dev_para, "vip_dev%d_flash_en", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.flash_en_io.gpio = GPIO_INDEX_INVALID;
      dev->ccm_cfg[i]->flash_used=0;
      vfe_dbg(0,"fetch vip_dev%d_flash_en from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.flash_en_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.flash_en_io.mul_sel=val.gpio.mul_sel;
      dev->ccm_cfg[i]->flash_used=1;
    }

    sprintf(dev_para, "vip_dev%d_flash_mode", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.flash_mode_io.gpio = GPIO_INDEX_INVALID;
      vfe_dbg(0,"fetch vip_dev%d_flash_mode from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.flash_mode_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.flash_mode_io.mul_sel=val.gpio.mul_sel;
    }

    sprintf(dev_para, "vip_dev%d_af_pwdn", i);
    type = script_get_item(vfe_para, dev_para, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
      dev->ccm_cfg[i]->gpio.af_pwdn_io.gpio = GPIO_INDEX_INVALID;

      vfe_dbg(0,"fetch vip_dev%d_af_pwdn from sys_config failed\n", i);
    } else {
      dev->ccm_cfg[i]->gpio.af_pwdn_io.gpio=val.gpio.gpio;
      dev->ccm_cfg[i]->gpio.af_pwdn_io.mul_sel=val.gpio.mul_sel;
    }

		/* fetch actuator issue */
	  sprintf(dev_para, "vip_dev%d_act_used", i);
	  type = script_get_item(vfe_para, dev_para, &val);
	  if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		dev->ccm_cfg[i]->act_used= 0;
		vfe_dbg(0,"fetch vip_dev%d_act_used from sys_config failed\n", i);
	  } else {
		dev->ccm_cfg[i]->act_used=val.val;
	  }

    ret = strcmp(dev->ccm_cfg[i]->act_name,"");
	  if((dev->ccm_cfg[i]->act_slave == 0xff) && (ret == 0)) //when insmod without parm
	  {
  	  sprintf(dev_para, "vip_dev%d_act_name", i);
  	  type = script_get_item(vfe_para, dev_para, &val);
  	    if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
  	      char null_str[]="";
  	      strcpy(dev->ccm_cfg[i]->act_name,null_str);
  	      vfe_dbg(0,"fetch vip_dev%d_act_name from sys_config failed\n", i);
  	    } else {
  	      strcpy(dev->ccm_cfg[i]->act_name,val.str);
  	    }

  		sprintf(dev_para, "vip_dev%d_act_slave", i);
  		type = script_get_item(vfe_para, dev_para, &val);
  		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
  		  dev->ccm_cfg[i]->act_slave= 0;

  		  vfe_dbg(0,"fetch vip_dev%d_act_slave from sys_config failed\n", i);
  		} else {
  		  dev->ccm_cfg[i]->act_slave=val.val;
  		}
	  }
	  else
	  {
	    dev->ccm_cfg[i]->act_used=1;//manual set used
	  }

#ifdef _SUNXI_CAM_DETECT_
	  /*fetch cam detect para*/
		type = script_get_item("camera_list_para", "camera_list_para_used", &val);
    if ((SCIRPT_ITEM_VALUE_TYPE_INT == type) && (val.val == 1))
	{
      //camera_export_info(dev->ccm_cfg[i]->ccm, &dev->ccm_cfg[i]->i2c_addr,i);
		unsigned char cam_name[20];
		unsigned int address;
		camera_export_info(cam_name, &address,i);

		if( (strcmp(cam_name,"")!=0)&&(address!=0) )
		{
			strcpy(dev->ccm_cfg[i]->ccm, cam_name);
			dev->ccm_cfg[i]->i2c_addr=address;
		}
		else
		{
			vfe_warn("detect none sensor in list, use sysconfig setting!\n");
		}
		for(j=0;j<ARRAY_SIZE(camera_info_list);j++)
		{
			if(strcmp(camera_info_list[j].name,dev->ccm_cfg[i]->ccm) == 0)
			{
				dev->ccm_cfg[i]->is_bayer_raw = camera_info_list[j].is_raw_sensor;
				dev->ccm_cfg[i]->is_isp_used = camera_info_list[j].is_isp_used;
				if(camera_info_list[j].vfilp)
				{
					dev->ccm_cfg[i]->vflip = (~dev->ccm_cfg[i]->vflip)&0x1;
				}
				if(camera_info_list[j].hfilp)
				{
					dev->ccm_cfg[i]->hflip= (~dev->ccm_cfg[i]->hflip)&0x1;
				}
				printk("dev->ccm_cfg[%d] = %s\n",i,dev->ccm_cfg[i]->ccm);
			}
		}
    }
#endif
  //vfe_dbg(0,"act_used=%d, name=%s, slave=0x%x\n",dev->ccm_cfg[0]->act_used,
  //	dev->ccm_cfg[0]->act_name, dev->ccm_cfg[0]->act_slave);
  }
#else
  int type;
  unsigned int i2c_addr_vip0[2] = {0x6c,0x00};
  unsigned int i2c_addr_vip1[2] = {0x78,0x42};
  unsigned int i;
  unsigned char ccm_vip0_dev0[] = {"ov8825",};
  unsigned char ccm_vip0_dev1[] = {"",};
  unsigned char ccm_vip1_dev0[] = {"ov5650",};
  unsigned char ccm_vip1_dev1[] = {"gc0308",};
  unsigned int i2c_addr[2];
  unsigned char *ccm_name[2];

  if(dev->id==0) {
    dev->dev_qty = 1;
    i2c_addr[0] = i2c_addr_vip0[0];
    i2c_addr[1] = i2c_addr_vip0[1];
    ccm_name[0] = ccm_vip0_dev0;
    ccm_name[1] = ccm_vip0_dev1;
  } else if (dev->id == 1) {
    dev->dev_qty = 1;
    i2c_addr[0] = i2c_addr_vip1[0];
    i2c_addr[1] = i2c_addr_vip1[1];
    ccm_name[0] = ccm_vip1_dev0;
    ccm_name[1] = ccm_vip1_dev1;
  }

  for(i=0; i<dev->dev_qty; i++)
  {
    dev->ccm_cfg[i]->twi_id = 1;
    type = strcmp(dev->ccm_cfg[i]->ccm,"");
    if((dev->ccm_cfg[i]->i2c_addr == 0xff) && (ret == 0)) //when insmod without parm
    {
      dev->ccm_cfg[i]->i2c_addr = i2c_addr[i];
      strcpy(dev->ccm_cfg[i]->ccm, ccm_name[i]);
    }
    dev->ccm_cfg[i]->power.stby_mode = 0;
    dev->ccm_cfg[i]->vflip = 0;
    dev->ccm_cfg[i]->hflip = 0;
  }
#endif

  for(i=0; i<dev->dev_qty; i++)
  {
    vfe_dbg(0,"dev->ccm_cfg[%d]->ccm = %s\n",i,dev->ccm_cfg[i]->ccm);
    vfe_dbg(0,"dev->ccm_cfg[%d]->twi_id = %x\n",i,dev->ccm_cfg[i]->twi_id);
    vfe_dbg(0,"dev->ccm_cfg[%d]->i2c_addr = %x\n",i,dev->ccm_cfg[i]->i2c_addr);
    vfe_dbg(0,"dev->ccm_cfg[%d]->vflip = %x\n",i,dev->ccm_cfg[i]->vflip);
    vfe_dbg(0,"dev->ccm_cfg[%d]->hflip = %x\n",i,dev->ccm_cfg[i]->hflip);
    vfe_dbg(0,"dev->ccm_cfg[%d]->iovdd_str = %s\n",i,dev->ccm_cfg[i]->iovdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->avdd_str = %s\n",i,dev->ccm_cfg[i]->avdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->dvdd_str = %s\n",i,dev->ccm_cfg[i]->dvdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->afvdd_str = %s\n",i,dev->ccm_cfg[i]->afvdd_str);
    vfe_dbg(0,"dev->ccm_cfg[%d]->act_used = %d\n",i,dev->ccm_cfg[i]->act_used);
    vfe_dbg(0,"dev->ccm_cfg[%d]->act_name = %s\n",i,dev->ccm_cfg[i]->act_name);
    vfe_dbg(0,"dev->ccm_cfg[%d]->act_slave = 0x%x\n",i,dev->ccm_cfg[i]->act_slave);
  }

  return 0;
}

struct isp_init_config isp_init_def_cfg = {
    .isp_test_settings =
    {
      /*isp test param */
      .isp_test_mode       = 1,
      .isp_test_exptime    = 0,
      .exp_line_start        = 1000  ,
      .exp_line_step         = 16    ,
      .exp_line_end          = 10000 ,
      .exp_change_interval   = 5     ,

      .isp_test_gain         = 0     ,
      .gain_start            = 16    ,
      .gain_step             = 1     ,
      .gain_end              = 256   ,
      .gain_change_interval  = 3     ,

      .isp_test_focus        = 0     ,
      .focus_start           = 0     ,
      .focus_step            = 10    ,
      .focus_end             = 800   ,
      .focus_change_interval = 2     ,

      .isp_dbg_level       = 0,
      .isp_focus_len       = 0,
      .isp_gain               = 64,
      .isp_exp_line        = 28336,

      /*isp enable param */
      .sprite_en            = 0,
      .lsc_en               = 0,
      .ae_en                = 0,
      .af_en                = 0,
      .awb_en               = 0,
      .drc_en               = 0,
      .defog_en             = 0,
      .satur_en             = 0,
      .tdf_en             = 0,
      .pri_contrast_en      = 0,
      .hdr_gamma_en         = 0,
    },

    .isp_3a_settings =
    {
      /*isp ae param */
      .define_ae_table           = 0,
      .ae_max_lv                  = 1800,
      .fno                             = 28,
      .ae_lum_low_th            = 125,
      .ae_lum_high_th           = 135,
      .ae_window_overexp_weigth = 16,
      .ae_hist_overexp_weight   = 32,
      .ae_video_speed           = 4,
      .ae_capture_speed         = 8,
      .ae_tolerance             = 6,
      .ae_min_frame_rate        = 8,
      .exp_delay_frame          = 2,
      .gain_delay_frame         = 2,
      .adaptive_frame_rate      = 1,
      .high_quality_mode_en     = 0,
      .force_frame_rate         = 0,

      /*isp awb param */
      .awb_interval                 = 4,
      .awb_mode_select          = 1,
      .awb_color_temper_low     = 0,
      .awb_color_temper_high    = 32,
      .r_gain_2900k              = 385,
      .b_gain_2900k             = 140,
      .awb_tolerance            = 10,

      /*isp af param */
      .vcm_min_code             = 0,
      .vcm_max_code             = 650,
      .color_matrix_inv =
      {
        .matrix = {{256,0,0},{0,256,0},{0,0,256}},
        .offset = {0, 0, 0},
      },
    },
    .isp_tunning_settings =
    {
      .flash_gain = 80,
      .flicker_type = 1,
      /*isp_dpc_otf_param*/
      .dpc_th_slop               = 4,
      .dpc_otf_min_th            = 16,
      .dpc_otf_max_th            = 1024,

      .front_camera = 0,
      .defog_value   =200,
      .use_bright_contrast = 0,
      .low_bright_supp      = 324,
      .low_bright_drc       = 24,
      .color_denoise_level  = 0,

      /*isp tune param */
      .bayer_gain_offset = {256,256,256,256,0,0,0,0},
      .csc_coeff = {1024,1024,1024,1024,1024,1024},
      .lsc_center =  {2048,2048},
      .lsc_tbl = {{0},{0},{0},{0},{0},{0},{0}},
      .hdr_tbl = {{0},{0},{0},{0}},
      .gamma_tbl = {10,20,30,40,50,60,70,80},
      .color_matrix_ini =
      {
        .matrix = {{256,0,0},{0,256,0},{0,0,256}},
        .offset = {0, 0, 0},
      },
    },
};

void set_isp_test_mode(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_test_mode = *(int *)value; }
void set_isp_test_exptime(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_test_exptime = *(int *)value;}
void set_exp_line_start(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.exp_line_start      = *(int *)value; }
void set_exp_line_step(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.exp_line_step       = *(int *)value; }
void set_exp_line_end(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.exp_line_end        = *(int *)value; }
void set_exp_change_interval(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.exp_change_interval = *(int *)value; }

void set_isp_test_gain(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_test_gain = *(int *)value; }
void set_gain_start(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.gain_start           = *(int *)value; }
void set_gain_step(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.gain_step            = *(int *)value; }
void set_gain_end (struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.gain_end             = *(int *)value; }
void set_gain_change_interval(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.gain_change_interval = *(int *)value; }

void set_isp_test_focus(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_test_focus        = *(int *)value; }
void set_focus_start(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.focus_start           = *(int *)value; }
void set_focus_step(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.focus_step            = *(int *)value; }
void set_focus_end(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.focus_end             = *(int *)value; }
void set_focus_change_interval(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.focus_change_interval = *(int *)value; }

void set_isp_dbg_level(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_dbg_level = *(int *)value; }
void set_isp_focus_len(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_focus_len = *(int *)value; }
void set_isp_gain(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_gain = *(int *)value; }
void set_isp_exp_line(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.isp_exp_line = *(int *)value; }
void set_sprite_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.sprite_en = *(int *)value; }
void set_lsc_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.lsc_en  = *(int *)value; }
void set_ae_en (struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.ae_en  = *(int *)value; }
void set_af_en (struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.af_en  = *(int *)value; }
void set_awb_en(struct isp_init_config *isp_ini_cfg, void *value, int len){ isp_ini_cfg->isp_test_settings.awb_en  = *(int *)value; }
void set_drc_en(struct isp_init_config *isp_ini_cfg, void *value, int len)  { isp_ini_cfg->isp_test_settings.drc_en  = *(int *)value; }
void set_defog_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.defog_en = *(int *)value; }
void set_satur_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.satur_en = *(int *)value; }
void set_tdf_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.tdf_en = *(int *)value; }
void set_pri_contrast_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.pri_contrast_en = *(int *)value; }
void set_hdr_gamma_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_test_settings.hdr_gamma_en = *(int *)value; }
void set_define_ae_table(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.define_ae_table = *(int *)value; }
void set_ae_table_length(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_table_length = *(int *)value; }
void set_ae_max_lv(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_max_lv = *(int *)value; }
void set_fno(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.fno = *(int *)value; }
void set_ae_lum_low_th(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_lum_low_th = *(int *)value; }
void set_ae_lum_high_th(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_lum_high_th = *(int *)value; }
void set_ae_window_overexp_weigth(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_window_overexp_weigth = *(int *)value; }
void set_ae_hist_overexp_weight(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_hist_overexp_weight = *(int *)value; }
void set_ae_video_speed(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_video_speed = *(int *)value; }
void set_ae_capture_speed(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_capture_speed = *(int *)value; }
void set_ae_tolerance(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_tolerance = *(int *)value; }
void set_ae_min_frame_rate(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.ae_min_frame_rate = *(int *)value; }
void set_exp_delay_frame(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.exp_delay_frame = *(int *)value; }
void set_gain_delay_frame(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.gain_delay_frame = *(int *)value; }
void set_high_quality_mode_en(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.high_quality_mode_en = *(int *)value; }
void set_adaptive_frame_rate(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.adaptive_frame_rate = *(int *)value; }
void set_force_frame_rate(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.force_frame_rate = *(int *)value; }

void set_awb_interval(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.awb_interval = *(int *)value; }
void set_awb_mode_select(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.awb_mode_select = *(int *)value; }
void set_awb_tolerance(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.awb_tolerance = *(int *)value; }

void set_awb_color_temper_low(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.awb_color_temper_low = *(int *)value; }
void set_awb_color_temper_high(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.awb_color_temper_high = *(int *)value; }
void set_r_gain_2900k(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.r_gain_2900k = *(int *)value; }
void set_b_gain_2900k(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.b_gain_2900k = *(int *)value; }
void set_vcm_min_code(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.vcm_min_code = *(int *)value; }
void set_vcm_max_code(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_3a_settings.vcm_max_code = *(int *)value; }
void set_flash_gain(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.flash_gain = *(int *)value; }
void set_flicker_type(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.flicker_type = *(int *)value; }
void set_dpc_th_slop(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.dpc_th_slop = *(int *)value; }
void set_dpc_otf_min_th(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.dpc_otf_min_th = *(int *)value; }
void set_dpc_otf_max_th(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.dpc_otf_max_th = *(int *)value; }
void set_front_camera(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.front_camera = *(int *)value; }
void set_defog_value(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.defog_value = *(int *)value; }
void set_use_bright_contrast(struct isp_init_config *isp_ini_cfg, void *value, int len)  { isp_ini_cfg->isp_tunning_settings.use_bright_contrast= *(int *)value; }
void set_low_bright_supp(struct isp_init_config *isp_ini_cfg, void *value, int len)  { isp_ini_cfg->isp_tunning_settings.low_bright_supp = *(int *)value; }
void set_low_bright_drc(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.low_bright_drc = *(int *)value; }
void set_color_denoise_level(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.color_denoise_level = *(int *)value; }
void set_lsc_center_x(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.lsc_center[0] = *(int *)value; }
void set_lsc_center_y(struct isp_init_config *isp_ini_cfg, void *value, int len) { isp_ini_cfg->isp_tunning_settings.lsc_center[1] = *(int *)value; }

void set_ae_table_param(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp;
	tmp = (int *)value;
	for(i = 0; i < len; i++)
	{
		isp_ini_cfg->isp_3a_settings.ae_table_param[i] = tmp[i];
	}
}
void set_awb_light_param(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp;
	tmp = (int *)value;
	for(i = 0; i < len; i++)
	{
		isp_ini_cfg->isp_3a_settings.awb_light_param[i] = tmp[i];
	}
}
void set_awb_coeff(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp;
	tmp = (int *)value;
	for(i = 0; i < len; i++)
	{
		isp_ini_cfg->isp_3a_settings.awb_coeff[i] = tmp[i];
	}
}

void set_isp_iso_100_cfg(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp, *cfg_pt;
	tmp = (int *)value;
	cfg_pt = &isp_ini_cfg->isp_iso_settings.isp_iso_100_cfg.sharp_coeff[0];
	for(i = 0; i < len; i++)
	{
		cfg_pt[i] = tmp[i];
	}
}
void set_isp_iso_200_cfg(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp, *cfg_pt;
	tmp = (int *)value;
	cfg_pt = &isp_ini_cfg->isp_iso_settings.isp_iso_200_cfg.sharp_coeff[0];
	for(i = 0; i < len; i++)
	{
		cfg_pt[i] = tmp[i];
	}
}
void set_isp_iso_400_cfg(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp, *cfg_pt;
	tmp = (int *)value;
	cfg_pt = &isp_ini_cfg->isp_iso_settings.isp_iso_400_cfg.sharp_coeff[0];
	for(i = 0; i < len; i++)
	{
		cfg_pt[i] = tmp[i];
	}
}
void set_isp_iso_800_cfg(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp, *cfg_pt;
	tmp = (int *)value;
	cfg_pt = &isp_ini_cfg->isp_iso_settings.isp_iso_800_cfg.sharp_coeff[0];
	for(i = 0; i < len; i++)
	{
		cfg_pt[i] = tmp[i];
	}
}
void set_isp_iso_1600_cfg(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp, *cfg_pt;
	tmp = (int *)value;
	cfg_pt = &isp_ini_cfg->isp_iso_settings.isp_iso_1600_cfg.sharp_coeff[0];
	for(i = 0; i < len; i++)
	{
		cfg_pt[i] = tmp[i];
	}
}
void set_isp_iso_3200_cfg(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp, *cfg_pt;
	tmp = (int *)value;
	cfg_pt = &isp_ini_cfg->isp_iso_settings.isp_iso_3200_cfg.sharp_coeff[0];
	for(i = 0; i < len; i++)
	{
		cfg_pt[i] = tmp[i];
	}
}

void set_color_matrix(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp;
	short *matrix, *offset;
	tmp = (int *)value;
	matrix = &isp_ini_cfg->isp_tunning_settings.color_matrix_ini.matrix[0][0] ;
	offset = &isp_ini_cfg->isp_tunning_settings.color_matrix_ini.offset[0];
	for(i = 0; i < len; i++)
	{
		if(i<9)
			matrix[i] = tmp [i];
		else
			offset[i-9] = tmp [i];
	}
}
void set_color_matrix_inv(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp;
	short *matrix, *offset;
	tmp = (int *)value;
	matrix = &isp_ini_cfg->isp_3a_settings.color_matrix_inv.matrix[0][0] ;
	offset = &isp_ini_cfg->isp_3a_settings.color_matrix_inv.offset[0] ;
	for(i = 0; i < len; i++)
	{
		if(i<9)
			matrix[i] = tmp [i];
		else
			offset[i-9] = tmp [i];
	}
}

void set_isp_gain_offset(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp;
	tmp = (int *)value;
	for(i = 0; i < len; i++)
	{
		isp_ini_cfg->isp_tunning_settings.bayer_gain_offset[i] = tmp[i];
	}
}
void set_isp_csc(struct isp_init_config *isp_ini_cfg, void *value, int len)
{
	int i,*tmp;
	tmp = (int *)value;
	for(i = 0; i < len; i++)
	{
		isp_ini_cfg->isp_tunning_settings.csc_coeff[i] = tmp[i];
	}
}

struct IspParamAttribute
{
	char *main;
	char *sub;
	int len;
	void (*set_param)(struct isp_init_config *, void *, int len);
};

struct FileAttribute
{
	char *file_name;
	int param_len;
	struct IspParamAttribute *pIspParam;
};

static struct IspParamAttribute IspTestParam[] =
{
	{ "isp_test_cfg", "isp_test_mode"        , 1 ,  set_isp_test_mode         ,},
	{ "isp_test_cfg", "isp_test_exptime"     , 1 ,  set_isp_test_exptime      ,},
	{ "isp_test_cfg", "exp_line_start"       , 1 ,  set_exp_line_start        ,},
	{ "isp_test_cfg", "exp_line_step"        , 1 ,  set_exp_line_step         ,},
	{ "isp_test_cfg", "exp_line_end"         , 1 ,  set_exp_line_end          ,},
	{ "isp_test_cfg", "exp_change_interval"  , 1 ,  set_exp_change_interval   ,},
	{ "isp_test_cfg", "isp_test_gain"        , 1 ,  set_isp_test_gain         ,},
	{ "isp_test_cfg", "gain_start"           , 1 ,  set_gain_start            ,},
	{ "isp_test_cfg", "gain_step"            , 1 ,  set_gain_step             ,},
	{ "isp_test_cfg", "gain_end"             , 1 ,  set_gain_end              ,},
	{ "isp_test_cfg", "gain_change_interval" , 1 ,  set_gain_change_interval  ,},
	{ "isp_test_cfg", "isp_test_focus"       , 1 ,  set_isp_test_focus        ,},
	{ "isp_test_cfg", "focus_start"          , 1 ,  set_focus_start           ,},
	{ "isp_test_cfg", "focus_step"           , 1 ,  set_focus_step            ,},
	{ "isp_test_cfg", "focus_end"            , 1 ,  set_focus_end             ,},
	{ "isp_test_cfg", "focus_change_interval", 1 ,  set_focus_change_interval ,},

	{ "isp_test_cfg", "isp_dbg_level",    1 ,  set_isp_dbg_level    ,},
	{ "isp_test_cfg", "isp_focus_len",    1 ,  set_isp_focus_len    ,},
	{ "isp_test_cfg", "isp_gain",         1 ,  set_isp_gain         ,},
	{ "isp_test_cfg", "isp_exp_line",     1 ,  set_isp_exp_line     ,},

	{ "isp_en_cfg",   "sprite_en",        1 ,  set_sprite_en        ,},
	{ "isp_en_cfg",   "lsc_en",           1 ,  set_lsc_en           ,},
	{ "isp_en_cfg",   "ae_en",            1 ,  set_ae_en            ,},
	{ "isp_en_cfg",   "af_en",            1 ,  set_af_en            ,},
	{ "isp_en_cfg",   "awb_en",           1 ,  set_awb_en           ,},
	{ "isp_en_cfg",   "drc_en",           1 ,  set_drc_en           ,},
	{ "isp_en_cfg",   "defog_en",         1 ,  set_defog_en         ,},
	{ "isp_en_cfg",   "satur_en",         1 ,  set_satur_en         ,},
	{ "isp_en_cfg",   "tdf_en",		  1 ,  set_tdf_en 		,},

	{ "isp_en_cfg",   "pri_contrast_en",  1 ,  set_pri_contrast_en  ,},
	{ "isp_en_cfg",   "hdr_gamma_en",     1 ,  set_hdr_gamma_en     ,},
};

static struct IspParamAttribute Isp3aParam[] =
{
	{ "isp_ae_cfg",   "define_ae_table",          1 ,  set_define_ae_table          ,},

	{ "isp_ae_cfg",   "ae_max_lv",          1 ,  set_ae_max_lv,},
	{ "isp_ae_cfg",   "ae_table_length",          1 ,  set_ae_table_length          ,},
	{ "isp_ae_cfg",   "fno"            ,          1 ,  set_fno                      ,},
	{ "isp_ae_cfg",   "ae_table_param_",          40 , set_ae_table_param           ,},

	{ "isp_ae_cfg",   "ae_lum_low_th",            1 ,  set_ae_lum_low_th            ,},
	{ "isp_ae_cfg",   "ae_lum_high_th",           1 ,  set_ae_lum_high_th           ,},
	{ "isp_ae_cfg",   "ae_window_overexp_weigth", 1 ,  set_ae_window_overexp_weigth ,},
	{ "isp_ae_cfg",   "ae_hist_overexp_weight",   1 ,  set_ae_hist_overexp_weight   ,},
	{ "isp_ae_cfg",   "ae_video_speed",           1 ,  set_ae_video_speed           ,},
	{ "isp_ae_cfg",   "ae_capture_speed",         1 ,  set_ae_capture_speed         ,},
	{ "isp_ae_cfg",   "ae_tolerance",             1 ,  set_ae_tolerance             ,},
	{ "isp_ae_cfg",   "ae_min_frame_rate",        1 ,  set_ae_min_frame_rate        ,},
	{ "isp_ae_cfg",   "exp_delay_frame",          1 ,  set_exp_delay_frame          ,},
	{ "isp_ae_cfg",   "gain_delay_frame",         1 ,  set_gain_delay_frame         ,},

	{ "isp_ae_cfg",   "high_quality_mode_en",     1 , set_high_quality_mode_en      ,},
	{ "isp_ae_cfg",   "adaptive_frame_rate",      1 , set_adaptive_frame_rate       ,},
	{ "isp_ae_cfg",   "force_frame_rate",         1 , set_force_frame_rate          ,},

	{ "isp_awb_cfg",  "awb_interval",          1 ,  set_awb_interval          ,},

	{ "isp_awb_cfg",  "awb_mode_select",          1 ,  set_awb_mode_select          ,},

	{ "isp_awb_cfg",  "awb_tolerance",          1 ,  set_awb_tolerance          ,},
	{ "isp_awb_cfg",  "awb_light_param_",        21 ,  set_awb_light_param              ,},
	{ "isp_awb_cfg",  "awb_coeff_",               30 , set_awb_coeff                    ,},

	{ "isp_awb_cfg",   "matrix_inv_",        12 ,  set_color_matrix_inv  ,},
	{ "isp_awb_cfg",  "awb_color_temper_low",     1 ,  set_awb_color_temper_low     ,},
	{ "isp_awb_cfg",  "awb_color_temper_high",    1 ,  set_awb_color_temper_high    ,},
	{ "isp_awb_cfg",  "r_gain_2900k",             1 ,  set_r_gain_2900k             ,},
	{ "isp_awb_cfg",  "b_gain_2900k",             1 ,  set_b_gain_2900k             ,},

	{ "isp_af_cfg",   "vcm_min_code",             1 ,  set_vcm_min_code             ,},
	{ "isp_af_cfg",   "vcm_max_code",             1 ,  set_vcm_max_code             ,},
};
static struct IspParamAttribute IspIsoParam[] =
{
	{ "isp_iso_100_cfg" ,      "iso_param_",      30,  set_isp_iso_100_cfg ,},
	{ "isp_iso_200_cfg" ,      "iso_param_",      30,  set_isp_iso_200_cfg ,},
	{ "isp_iso_400_cfg" ,      "iso_param_",      30,  set_isp_iso_400_cfg ,},
	{ "isp_iso_800_cfg" ,      "iso_param_",      30 ,  set_isp_iso_800_cfg ,},
	{ "isp_iso_1600_cfg" ,     "iso_param_",      30 ,  set_isp_iso_1600_cfg,},
	{ "isp_iso_3200_cfg" ,     "iso_param_",      30 ,  set_isp_iso_3200_cfg,},
};
static struct IspParamAttribute IspTuningParam[] =
{
	{ "isp_drc_cfg" ,           "use_bright_contrast",    1 ,  set_use_bright_contrast    ,},
	{ "isp_drc_cfg" ,           "low_bright_supp",    1 ,  set_low_bright_supp    ,},
	{ "isp_drc_cfg" ,           "low_bright_drc",     1 ,  set_low_bright_drc     ,},
	{ "isp_tuning_cfg" ,        "color_denoise_level",1 ,  set_color_denoise_level,},
	{ "isp_tuning_cfg" ,        "flash_gain",         1 ,  set_flash_gain         ,},
	{ "isp_tuning_cfg" ,        "flicker_type",       1 ,  set_flicker_type       ,},
	{ "isp_tuning_cfg" ,        "front_camera",       1 ,  set_front_camera       ,},
	{ "isp_tuning_cfg" ,        "defog_value",        1 ,  set_defog_value        ,},
	{ "isp_lsc" ,               "lsc_center_x",       1 ,  set_lsc_center_x       ,},
	{ "isp_lsc" ,               "lsc_center_y",       1 ,  set_lsc_center_y       ,},
	{ "isp_gain_offset" ,       "gain_offset_",       8 ,  set_isp_gain_offset    ,},
	{ "isp_csc" ,               "csc_coeff_",         6 ,  set_isp_csc            ,},
	{ "isp_color_matrix" ,      "matrix_",            12 ,  set_color_matrix      ,},

};

static struct FileAttribute FileAttr [] =
{
	 { "isp_test_param.ini",    ARRAY_SIZE(IspTestParam)  , &IspTestParam[0],  },
	 { "isp_3a_param.ini",      ARRAY_SIZE(Isp3aParam)    , &Isp3aParam[0],    },
	 { "isp_iso_param.ini",     ARRAY_SIZE(IspIsoParam)   , &IspIsoParam[0],   },
	 { "isp_tuning_param.ini",  ARRAY_SIZE(IspTuningParam), &IspTuningParam[0],},
};

int fetch_isp_cfg(struct isp_init_config *isp_ini_cfg, struct cfg_section *cfg_section, struct FileAttribute *file_attr)
{
	int i, j, *array_value;
	struct cfg_subkey subkey;
	struct IspParamAttribute *param;
	char sub_name[128] = {0};
	/* fetch ISP isp_test_mode! */
	for (i = 0; i < file_attr->param_len;  i++)
	{
		param = file_attr->pIspParam + i;
		if(param->main == NULL || param->sub == NULL)
		{
			vfe_warn("param->main or param->sub is NULL!\n");
			continue;
		}
		if(param->len == 1)
		{
			if (CFG_ITEM_VALUE_TYPE_INT != cfg_get_one_subkey(cfg_section,param->main, param->sub, &subkey))
			{
				vfe_dbg(0,"Warning: %s->%s,apply default value!\n",param->main, param->sub);
			}
			else
			{
				if(param->set_param)
				{
					param->set_param(isp_ini_cfg, (void *)&subkey.value->val, param->len);
					vfe_dbg(0,"fetch_isp_cfg_single: %s->%s  = %d\n",param->main, param->sub,subkey.value->val);
				}
			}
		}
		else if(param->len > 1)
		{
			array_value = (int*)kzalloc(param->len*sizeof(int),GFP_KERNEL);
			for(j = 0;j<param->len;j++)
			{
				sprintf(sub_name, "%s%d",param->sub, j);
				if (CFG_ITEM_VALUE_TYPE_INT != cfg_get_one_subkey(cfg_section,param->main,sub_name,&subkey))
				{
					vfe_warn("fetch %s from %s failed,apply default value!\n",sub_name,param->main);
				}
				else
				{
					array_value[j] = subkey.value->val;
					if(param->set_param && j == (param->len-1))
					{
						param->set_param(isp_ini_cfg, (void *)array_value, param->len);
					}
					vfe_dbg(0,"fetch_isp_cfg_array: %s->%s  = %d\n",param->main, sub_name, subkey.value->val);
				}
			}
			if(array_value)
				kfree(array_value);
		}
	}
	 vfe_dbg(0,"fetch isp_cfg done!\n");
	return 0;
}
int fetch_isp_tbl(struct isp_init_config *isp_ini_cfg, char* tbl_patch)
{
	int len, ret = 0;
	char isp_gamma_tbl_path[128] = "\0",isp_hdr_tbl_path[128] = "\0",isp_lsc_tbl_path[128] = "\0";
	char *buf;
	strcpy(isp_gamma_tbl_path, tbl_patch);
	strcpy(isp_hdr_tbl_path, tbl_patch);
	strcpy(isp_lsc_tbl_path, tbl_patch);

	strcat(isp_gamma_tbl_path, "gamma_tbl.bin");
	strcat(isp_hdr_tbl_path, "hdr_tbl.bin");
	strcat(isp_lsc_tbl_path, "lsc_tbl.bin");

	printk("isp_tbl_path = %s\n",isp_gamma_tbl_path);
	buf = (char*)kzalloc(SIZE_OF_LSC_TBL,GFP_KERNEL);

	/* fetch gamma_tbl table! */

	len = cfg_read_file(isp_gamma_tbl_path,buf,SIZE_OF_GAMMA_TBL);
	if(len < 0)
	{
		vfe_warn("read gamma_tbl from gamma_tbl.bin failed!\n");
		ret =  -1;
	}
	else
	{
		memcpy(isp_ini_cfg->isp_tunning_settings.gamma_tbl, buf, len);
	}

	/* fetch lsc table! */
	len = cfg_read_file(isp_lsc_tbl_path,buf,SIZE_OF_LSC_TBL);
	if(len < 0)
	{
		vfe_warn("read lsc_tbl from lsc_tbl.bin failed!\n");
		ret =  -1;
	}
	else
	{
		memcpy(isp_ini_cfg->isp_tunning_settings.lsc_tbl, buf, len);
	}
	/* fetch hdr_tbl table!*/
	len = cfg_read_file(isp_hdr_tbl_path,buf,SIZE_OF_HDR_TBL);
	if(len < 0)
	{
		vfe_warn("read hdr_tbl from hdr_tbl.bin failed!\n");
		ret =  -1;
	}
	else
	{
		memcpy(isp_ini_cfg->isp_tunning_settings.hdr_tbl, buf, len);
	}

	if(buf)
	{
		kfree(buf);
	}
	return ret;
}

int read_ini_info(struct vfe_dev *dev,int isp_id)
{
	int i, ret = 0;
	char isp_cfg_path[128],isp_tbl_path[128],file_name_path[128];
	struct cfg_section *cfg_section;

	vfe_print("read ini start\n");
	if(dev->ccm_cfg[isp_id]->ccm != NULL)
	{
		sprintf(isp_cfg_path, "/system/etc/hawkview/%s/", dev->ccm_cfg[isp_id]->ccm);
		sprintf(isp_tbl_path, "/system/etc/hawkview/%s/bin/", dev->ccm_cfg[isp_id]->ccm);
	}
	else
	{
		sprintf(isp_cfg_path, "/system/etc/hawkview/camera.ini");
		sprintf(isp_tbl_path, "/system/etc/hawkview/bin/");
	}
	dev->isp_gen_set[isp_id].isp_ini_cfg = isp_init_def_cfg;
	for(i=0; i< ARRAY_SIZE(FileAttr); i++)
	{
		sprintf(file_name_path,"%s%s",isp_cfg_path,FileAttr[i].file_name);
		vfe_print("read %s start! \n",file_name_path);
		cfg_section_init(&cfg_section);
		ret = cfg_read_ini(file_name_path, &cfg_section);
		if(ret == -1)
		{
			cfg_section_release(&cfg_section);
			goto read_ini_info_end;
		}
		fetch_isp_cfg(&dev->isp_gen_set[isp_id].isp_ini_cfg, cfg_section,&FileAttr[i]);
		cfg_section_release(&cfg_section);
	}
	ret = fetch_isp_tbl(&dev->isp_gen_set[isp_id].isp_ini_cfg, &isp_tbl_path[0]);
	if(ret == -1)
    	{
    		dev->isp_gen_set[isp_id].isp_ini_cfg = isp_init_def_cfg;
        }
read_ini_info_end:
	vfe_dbg(0,"read ini end\n");
	return ret;
}

