/*
 * A V4L2 driver for OV ov5640 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>//linux-3.0
#include <linux/io.h>
#include <../arch/arm/mach-sun6i/include/mach/gpio.h>
#include <../arch/arm/mach-sun6i/include/mach/sys_config.h>
#include <linux/regulator/consumer.h>
#include <mach/system.h>
//#include "../../../../power/axp_power/axp-gpio.h"
#include "../include/sunxi_csi_core.h"
#include "../include/sunxi_csi_dev.h"

MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for OV ov5640 sensors");
MODULE_LICENSE("GPL");



//for internel driver debug
#define DEV_DBG_EN   		0
#if(DEV_DBG_EN == 1)		
#define csi_dev_dbg(x,arg...) printk("[CSI_DEBUG][OV5640]"x,##arg)
#else
#define csi_dev_dbg(x,arg...) 
#endif
#define csi_dev_err(x,arg...) printk("[CSI_ERR][OV5640]"x,##arg)
#define csi_dev_print(x,arg...) printk("[CSI][OV5640]"x,##arg)

#define MCLK (24*1000*1000)
#define VREF_POL	CSI_HIGH
#define HREF_POL	CSI_HIGH
#define CLK_POL		CSI_RISING
#define IO_CFG		0						//0 for csi0
#define V4L2_IDENT_SENSOR 0x5640

//define the voltage level of control signal
#define CSI_STBY_ON			1
#define CSI_STBY_OFF 		0
#define CSI_RST_ON			0
#define CSI_RST_OFF			1
#define CSI_PWR_ON			1
#define CSI_PWR_OFF			0
#define CSI_AF_PWR_ON		1
#define CSI_AF_PWR_OFF	0

#define REG_TERM 0xff
#define VAL_TERM 0xff


#define REG_ADDR_STEP 2
#define REG_DATA_STEP 1
#define REG_STEP 			(REG_ADDR_STEP+REG_DATA_STEP)

#define CONTINUEOUS_AF
#define AUTO_FPS
#define DENOISE_LV_AUTO
#define SHARPNESS 0x10

#ifdef AUTO_FPS
//#define AF_FAST
#endif

#ifndef DENOISE_LV_AUTO
#define DENOISE_LV 0x8
#endif


//#define CSI_VER_FOR_FPGA
/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define QSXGA_WIDTH		2592
#define QSXGA_HEIGHT	1936
#define QXGA_WIDTH 		2048
#define QXGA_HEIGHT		1536
#define HD1080_WIDTH	1920
#define HD1080_HEIGHT	1080
#define UXGA_WIDTH		1600
#define UXGA_HEIGHT		1200
#define SXGA_WIDTH		1280
#define SXGA_HEIGHT		960
#define HD720_WIDTH 	1280
#define HD720_HEIGHT	720
#define XGA_WIDTH			1024
#define XGA_HEIGHT 		768
#define SVGA_WIDTH		800
#define SVGA_HEIGHT 	600
#define VGA_WIDTH			640
#define VGA_HEIGHT		480
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240
#define CIF_WIDTH			352
#define CIF_HEIGHT		288
#define QCIF_WIDTH		176
#define	QCIF_HEIGHT		144



/*
 * Our nominal (default) frame rate.
 */
#ifdef CSI_VER_FOR_FPGA
#define SENSOR_FRAME_RATE 15
#else
#define SENSOR_FRAME_RATE 30
#endif
/*
 * The ov5640 sits on i2c with ID 0x78
 */
#define I2C_ADDR 0x78

/* Registers */

static struct delayed_work sensor_s_ae_ratio_work;
static struct v4l2_subdev *glb_sd;
		
/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */
struct snesor_colorfx_struct; /* coming later */
__csi_subdev_info_t ccm_info_con = 
{
	.mclk 	= MCLK,
	.vref 	= VREF_POL,
	.href 	= HREF_POL,
	.clock	= CLK_POL,
	.iocfg	= IO_CFG,
};

struct sensor_info {
	struct v4l2_subdev sd;
	struct sensor_format_struct *fmt;  /* Current format */
	__csi_subdev_info_t *ccm_info;
	int	width;
	int	height;
	unsigned int capture_mode;		//V4L2_MODE_VIDEO/V4L2_MODE_IMAGE
	unsigned int af_first_flag;
	unsigned int init_first_flag;
	unsigned int preview_first_flag;
	unsigned int focus_status;		//0:idle 1:busy
	unsigned int low_speed;		//0:high speed 1:low speed
	int brightness;
	int	contrast;
	int saturation;
	int hue;
	int hflip;
	int vflip;
	int gain;
	int autogain;
	int exp;
	enum v4l2_exposure_auto_type autoexp;
	int autowb;
	enum v4l2_whiteblance wb;
	enum v4l2_colorfx clrfx;
	enum v4l2_flash_mode flash_mode;
	enum v4l2_power_line_frequency band_filter;
	enum v4l2_autofocus_mode af_mode;
	enum v4l2_autofocus_ctrl af_ctrl;
	struct v4l2_fract tpf;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}



/*
 * The default register settings
 *
 */

struct regval_list {
	unsigned char reg_num[REG_ADDR_STEP];
	unsigned char value[REG_DATA_STEP];
};


static struct regval_list sensor_default_regs[] = {
	{{0x31,0x03},{0x11}},//
	{{0x30,0x08},{0x82}},//reset
	{{0xff,0xff},{0x1e}},//delay 30ms
//	{{0x30,0x08},{0x42}},//power down
	{{0x31,0x03},{0x03}},//
	{{0x30,0x17},{0x00}},//
	{{0x30,0x18},{0x00}},//
	//pll and clock setting
	{{0x30,0x34},{0x18}},//
	{{0x30,0x35},{0x21}},//
	{{0x30,0x36},{0x46}},//0x46->30fps
	{{0x30,0x37},{0x13}},//////div
	{{0x31,0x08},{0x01}},//
	{{0x38,0x24},{0x01}},//
	
	{{0x36,0x30},{0x36}},//
	{{0x36,0x31},{0x0e}},//
	{{0x36,0x32},{0xe2}},//
	{{0x36,0x33},{0x12}},//
	{{0x36,0x21},{0xe0}},//
	{{0x37,0x04},{0xa0}},//
	{{0x37,0x03},{0x5a}},//
	{{0x37,0x15},{0x78}},//
	{{0x37,0x17},{0x01}},//
	{{0x37,0x0b},{0x60}},//
	{{0x37,0x05},{0x1a}},//
	{{0x39,0x05},{0x02}},//
	{{0x39,0x06},{0x10}},//
	{{0x39,0x01},{0x0a}},//
	{{0x37,0x31},{0x12}},//
	{{0x36,0x00},{0x08}},//
	{{0x36,0x01},{0x33}},//
//	{{0x30,0x2d},{0x60}},//
	{{0x36,0x20},{0x52}},//
	{{0x37,0x1b},{0x20}},//
	{{0x47,0x1c},{0x50}},//
	{{0x3a,0x13},{0x43}},//
	{{0x3a,0x18},{0x00}},//
	{{0x3a,0x19},{0xd8}},// 
	{{0x36,0x35},{0x13}},//
	{{0x36,0x36},{0x03}},//
	{{0x36,0x34},{0x40}},//
	{{0x36,0x22},{0x01}},//
	{{0x3c,0x01},{0x34}},//
	{{0x3c,0x04},{0x28}},//
	{{0x3c,0x05},{0x98}},//
	{{0x3c,0x06},{0x00}},//
	{{0x3c,0x07},{0x08}},//
	{{0x3c,0x08},{0x00}},//
	{{0x3c,0x09},{0x1c}},//
	{{0x3c,0x0a},{0x9c}},//
	{{0x3c,0x0b},{0x40}},//
	{{0x38,0x20},{0x41}},// binning
	{{0x38,0x21},{0x01}},// binning
	{{0x38,0x14},{0x31}},//
	{{0x38,0x15},{0x31}},//
	{{0x38,0x00},{0x00}},//
	{{0x38,0x01},{0x00}},//
	{{0x38,0x02},{0x00}},//
	{{0x38,0x03},{0x04}},//
	{{0x38,0x04},{0x0a}},//
	{{0x38,0x05},{0x3f}},//
	{{0x38,0x06},{0x07}},//
	{{0x38,0x07},{0x9b}},//
	{{0x38,0x08},{0x02}},//
	{{0x38,0x09},{0x80}},//
	{{0x38,0x0a},{0x01}},//
	{{0x38,0x0b},{0xe0}},//
	{{0x38,0x0c},{0x07}},//
	{{0x38,0x0d},{0x68}},//
	{{0x38,0x0e},{0x03}},//
	{{0x38,0x0f},{0xd8}},//
	{{0x38,0x10},{0x00}},//
	{{0x38,0x11},{0x10}},//
	{{0x38,0x12},{0x00}},//
	{{0x38,0x13},{0x06}},//
	{{0x36,0x18},{0x00}},//
	{{0x36,0x12},{0x29}},//
	{{0x37,0x08},{0x64}},//
	{{0x37,0x09},{0x52}},//
	{{0x37,0x0c},{0x03}},//
	{{0x3a,0x00},{0x78}},//night mode bit2
	{{0x3a,0x02},{0x03}},//
	{{0x3a,0x03},{0xd8}},//
	{{0x3a,0x08},{0x01}},//
	{{0x3a,0x09},{0x27}},//
	{{0x3a,0x0a},{0x00}},//
	{{0x3a,0x0b},{0xf6}},//
	{{0x3a,0x0e},{0x03}},//
	{{0x3a,0x0d},{0x04}},//
	{{0x3a,0x14},{0x03}},//
	{{0x3a,0x15},{0xd8}},//
	{{0x40,0x01},{0x02}},//
	{{0x40,0x04},{0x02}},//
	{{0x30,0x00},{0x00}},//
	{{0x30,0x02},{0x1c}},//
	{{0x30,0x04},{0xff}},//
	{{0x30,0x06},{0xc3}},//
	{{0x30,0x0e},{0x58}},//
//	{{0x30,0x2e},{0x00}},//
	
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x

	{{0x43,0x00},{0x30}},//
	{{0x50,0x1f},{0x00}},//
	{{0x47,0x13},{0x03}},//
	{{0x44,0x07},{0x04}},//
	{{0x44,0x0e},{0x00}},//
	{{0x46,0x0b},{0x35}},//
	{{0x46,0x0c},{0x20}},//
	{{0x48,0x37},{0x22}},
	{{0x50,0x00},{0xa7}},//
	{{0x50,0x01},{0xa3}},//
	
	{{0x47,0x40},{0x21}},//hsync,vsync,clock pol,reference to application note,spec is wrong
									
	//AWB   
	{{0x34,0x06},{0x00}}, // LA      ORG
  {{0x51,0x80},{0xff}},	// 0xff    0xff
	{{0x51,0x81},{0x50}}, // 0xf2    0x50               	
	{{0x51,0x82},{0x11}}, // 0x00    0x11              	
	{{0x51,0x83},{0x14}}, // 0x14    0x14             	
	{{0x51,0x84},{0x25}}, // 0x25    0x25             	
	{{0x51,0x85},{0x24}}, // 0x24    0x24             	
	{{0x51,0x86},{0x1c}}, // 0x09    0x1c              	
	{{0x51,0x87},{0x18}}, // 0x09    0x18              	
	{{0x51,0x88},{0x18}}, // 0x09    0x18              	
	{{0x51,0x89},{0x6e}}, // 0x75    0x6e             	
	{{0x51,0x8a},{0x68}}, // 0x54    0x68             	
	{{0x51,0x8b},{0xa8}}, // 0xe0    0xa8             	
	{{0x51,0x8c},{0xa8}}, // 0xb2    0xa8             	
	{{0x51,0x8d},{0x3d}}, // 0x42    0x3d             	
	{{0x51,0x8e},{0x3d}}, // 0x3d    0x3d             	
	{{0x51,0x8f},{0x54}}, // 0x56    0x54             	
	{{0x51,0x90},{0x54}}, // 0x46    0x54             	
	{{0x51,0x91},{0xf8}}, // 0xf8    0xf8             	
	{{0x51,0x92},{0x04}}, // 0x04    0x04              	
	{{0x51,0x93},{0x70}}, // 0x70    0x70             	
	{{0x51,0x94},{0xf0}}, // 0xf0    0xf0             	
	{{0x51,0x95},{0xf0}}, // 0xf0    0xf0             	
	{{0x51,0x96},{0x03}}, // 0x03    0x03              	
	{{0x51,0x97},{0x01}}, // 0x01    0x01              	
	{{0x51,0x98},{0x05}}, // 0x04    0x05              	
	{{0x51,0x99},{0x7c}}, // 0x12    0x7c
	{{0x51,0x9a},{0x04}}, // 0x04    0x04
	{{0x51,0x9b},{0x00}}, // 0x00    0x00
	{{0x51,0x9c},{0x06}}, // 0x06    0x06
	{{0x51,0x9d},{0x79}}, // 0x82    0x79
	{{0x51,0x9e},{0x38}}, // 0x38    0x38
	 //Color              // LA      ORG      
	{{0x53,0x81},{0x1e}}, // 0x1e    0x1e
	{{0x53,0x82},{0x5b}}, // 0x5b    0x5b
	{{0x53,0x83},{0x08}}, // 0x08    0x08
	{{0x53,0x84},{0x0a}}, // 0x0a    0x05
	{{0x53,0x85},{0x7e}}, // 0x7e    0x72
	{{0x53,0x86},{0x88}}, // 0x88    0x77
	{{0x53,0x87},{0x7c}}, // 0x7c    0x6d
	{{0x53,0x88},{0x6c}}, // 0x6c    0x4d
	{{0x53,0x89},{0x10}}, // 0x10    0x20
	{{0x53,0x8a},{0x01}}, // 0x01    0x01
	{{0x53,0x8b},{0x98}}, // 0x98    0x98
	//Sharpness/Denoise 	  
	{{0x53,0x00},{0x08}}, 
	{{0x53,0x01},{0x30}},      
	{{0x53,0x02},{0x2c}}, 
	{{0x53,0x03},{0x1c}}, 
	{{0x53,0x08},{0x25}}, //sharpness/noise auto
	{{0x53,0x04},{0x08}}, 
	{{0x53,0x05},{0x30}}, 
	{{0x53,0x06},{0x1c}}, 
	{{0x53,0x07},{0x2c}}, 
	{{0x53,0x09},{0x08}}, 
	{{0x53,0x0a},{0x30}}, 
	{{0x53,0x0b},{0x04}}, 
	{{0x53,0x0c},{0x06}}, 

	//Gamma        
	{{0x54,0x80},{0x01}},  // LA     ORG
	{{0x54,0x81},{0x08}},  // 0x08     0x06
	{{0x54,0x82},{0x14}},  // 0x14     0x15
	{{0x54,0x83},{0x28}},  // 0x28     0x28
	{{0x54,0x84},{0x51}},  // 0x51     0x3b
	{{0x54,0x85},{0x65}},  // 0x65     0x50
	{{0x54,0x86},{0x71}},  // 0x71     0x5d
	{{0x54,0x87},{0x7d}},  // 0x7d     0x6a
	{{0x54,0x88},{0x87}},  // 0x87     0x75
	{{0x54,0x89},{0x91}},  // 0x91     0x80
	{{0x54,0x8a},{0x9a}},  // 0x9a     0x8a
	{{0x54,0x8b},{0xaa}},  // 0xaa     0x9b
	{{0x54,0x8c},{0xb8}},  // 0xb8     0xaa
	{{0x54,0x8d},{0xcd}},  // 0xcd     0xc0
	{{0x54,0x8e},{0xdd}},  // 0xdd     0xd5
	{{0x54,0x8f},{0xea}},  // 0xea     0xe8
	{{0x54,0x90},{0x1d}},  // 0x1d     0x20
	  
	//UV  
	{{0x55,0x80},{0x04}}, 
	{{0x55,0x83},{0x40}}, 
	{{0x55,0x84},{0x10}}, 
	{{0x55,0x89},{0x10}}, 
	{{0x55,0x8a},{0x00}}, 
	{{0x55,0x8b},{0xf8}}, 
	
//	{{0x55,0x87},{0x05}},
//	{{0x55,0x88},{0x09}},
	//Lens Shading 
	{{0x50,0x00},{0xa7}}, //LA        org
	{{0x58,0x00},{0x23}}, //0x23      0x17
	{{0x58,0x01},{0x14}}, //0x14      0x10
	{{0x58,0x02},{0x0f}}, //0x0f      0x0e
	{{0x58,0x03},{0x0f}}, //0x0f      0x0e
	{{0x58,0x04},{0x12}}, //0x12      0x11
	{{0x58,0x05},{0x26}}, //0x26      0x1b
	{{0x58,0x06},{0x0c}}, //0x0c      0x0b
	{{0x58,0x07},{0x08}}, //0x08      0x07
	{{0x58,0x08},{0x05}}, //0x05      0x05
	{{0x58,0x09},{0x05}}, //0x05      0x06
	{{0x58,0x0A},{0x08}}, //0x08      0x09
	{{0x58,0x0B},{0x0d}}, //0x0d      0x0e
	{{0x58,0x0C},{0x08}}, //0x08      0x06
	{{0x58,0x0D},{0x03}}, //0x03      0x02
	{{0x58,0x0E},{0x00}}, //0x00      0x00
	{{0x58,0x0F},{0x00}}, //0x00      0x00
	{{0x58,0x10},{0x03}}, //0x03      0x03
	{{0x58,0x11},{0x09}}, //0x09      0x09
	{{0x58,0x12},{0x07}}, //0x07      0x06
	{{0x58,0x13},{0x03}}, //0x03      0x03
	{{0x58,0x14},{0x00}}, //0x00      0x00
	{{0x58,0x15},{0x01}}, //0x01      0x00
	{{0x58,0x16},{0x03}}, //0x03      0x03
	{{0x58,0x17},{0x08}}, //0x08      0x09
	{{0x58,0x18},{0x0d}}, //0x0d      0x0b
	{{0x58,0x19},{0x08}}, //0x08      0x08
	{{0x58,0x1A},{0x05}}, //0x05      0x05
	{{0x58,0x1B},{0x06}}, //0x06      0x05
	{{0x58,0x1C},{0x08}}, //0x08      0x08
	{{0x58,0x1D},{0x0e}}, //0x0e      0x0e
	{{0x58,0x1E},{0x29}}, //0x29      0x18
	{{0x58,0x1F},{0x17}}, //0x17      0x12
	{{0x58,0x20},{0x11}}, //0x11      0x0f
	{{0x58,0x21},{0x11}}, //0x11      0x0f
	{{0x58,0x22},{0x15}}, //0x15      0x12
	{{0x58,0x23},{0x28}}, //0x28      0x1a
	{{0x58,0x24},{0x46}}, //0x46      0x0a
	{{0x58,0x25},{0x26}}, //0x26      0x0a
	{{0x58,0x26},{0x08}}, //0x08      0x0a
	{{0x58,0x27},{0x26}}, //0x26      0x0a
	{{0x58,0x28},{0x64}}, //0x64      0x46
	{{0x58,0x29},{0x26}}, //0x26      0x2a
	{{0x58,0x2A},{0x24}}, //0x24      0x24
	{{0x58,0x2B},{0x22}}, //0x22      0x44
	{{0x58,0x2C},{0x24}}, //0x24      0x24
	{{0x58,0x2D},{0x24}}, //0x24      0x28
	{{0x58,0x2E},{0x06}}, //0x06      0x08
	{{0x58,0x2F},{0x22}}, //0x22      0x42
	{{0x58,0x30},{0x40}}, //0x40      0x40
	{{0x58,0x31},{0x42}}, //0x42      0x42
	{{0x58,0x32},{0x24}}, //0x24      0x28
	{{0x58,0x33},{0x26}}, //0x26      0x0a
	{{0x58,0x34},{0x24}}, //0x24      0x26
	{{0x58,0x35},{0x22}}, //0x22      0x24
	{{0x58,0x36},{0x22}}, //0x22      0x26
	{{0x58,0x37},{0x26}}, //0x26      0x28
	{{0x58,0x38},{0x44}}, //0x44      0x4a
	{{0x58,0x39},{0x24}}, //0x24      0x0a
	{{0x58,0x3A},{0x26}}, //0x26      0x0c
	{{0x58,0x3B},{0x28}}, //0x28      0x2a
	{{0x58,0x3C},{0x42}}, //0x42      0x28
	{{0x58,0x3D},{0xce}}, //0xce      0xce
	 
//	{{0x50,0x25},{0x00}}, 
	
	//EV
	{{0x3a,0x0f},{0x40}}, 
	{{0x3a,0x10},{0x38}}, 
	{{0x3a,0x1b},{0x40}}, 
	{{0x3a,0x1e},{0x38}}, 
	{{0x3a,0x11},{0x70}}, 
	{{0x3a,0x1f},{0x14}}, 

	{{0x30,0x31},{0x08}}, //disable internal LDO
	
//	//power down release
//	{{0x30,0x08},{0x02}}, 
};                                	                         

//for capture                                                                    	    
static struct regval_list sensor_qsxga_regs[] = { //qsxga: 2592*1936
	//capture 5Mega 7.5fps
	//power down
//	{{0x30,0x08},{0x42}},
	//pll and clock setting
	{{0x30,0x34},{0x18}},                            	    
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x21}},    
#else
  {{0x30,0x35},{0x41}},                         
#endif                        	    
	{{0x30,0x36},{0x54}},                            	                            	    
	{{0x30,0x37},{0x13}},                            	    
	{{0x31,0x08},{0x01}},                            	    
	{{0x38,0x24},{0x01}},                            	    
	{{0xff,0xff},{0x05}},//delay 5ms              
	//timing                                              
	//2592*1936                                           
	{{0x38,0x08},{0x0a}}, //H size MSB                    
	{{0x38,0x09},{0x20}}, //H size LSB                    
	{{0x38,0x0a},{0x07}}, //V size MSB                    
	{{0x38,0x0b},{0x90}}, //V size LSB                    
	{{0x38,0x0c},{0x0b}}, //HTS MSB                       
	{{0x38,0x0d},{0x1c}}, //HTS LSB                       
	{{0x38,0x0e},{0x07}}, //VTS MSB                       
	{{0x38,0x0f},{0xb0}}, //LSB                           
#ifndef CSI_VER_FOR_FPGA
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x93}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x7b}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x0d}}, //50HZ step max                 
	{{0x3a,0x0d},{0x10}}, //60HZ step max                 
#else
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x49}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x3d}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x1a}}, //50HZ step max                 
	{{0x3a,0x0d},{0x20}}, //60HZ step max 
#endif	                                                                                              
//	{{0x35,0x03},{0x07}}, //AEC disable                  	                                     	                                   
	{{0x35,0x0c},{0x00}},                                   
	{{0x35,0x0d},{0x00}},         
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold                                 
	       	                                                                 
  {{0x38,0x14},{0x11}}, //horizton subsample
	{{0x38,0x15},{0x11}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x00}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9f}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x04}},	//isp vertical offset low byte 
  
//  {{0x53,0x08},{0x65}},		//sharpen manual    
//  {{0x53,0x02},{0x20}}, //sharpness      
                                                     
  {{0x40,0x02},{0xc5}},  //BLC related                  
	{{0x40,0x05},{0x1a}}, // BLC related               
	                                                                                    
	{{0x36,0x18},{0x04}},                                 
	{{0x36,0x12},{0x2b}},                                 
	{{0x37,0x09},{0x12}},                                 
	{{0x37,0x0c},{0x00}},                                 
	{{0x3a,0x02},{0x07}}, //60HZ max exposure limit MSB   
	{{0x3a,0x03},{0xb0}}, //60HZ max exposure limit LSB   
	{{0x3a,0x14},{0x07}}, //50HZ max exposure limit MSB   
	{{0x3a,0x15},{0xb0}}, //50HZ max exposure limit LSB   
	{{0x40,0x04},{0x06}}, //BLC line number               
	{{0x48,0x37},{0x2c}},//PCLK period                    
	{{0x50,0x01},{0xa3}},//ISP effect    
	
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x 	   
							
	//power down release
//	{{0x30,0x08},{0x02}}, 
//	{{0xff,0xff},{0x32}},//delay 50ms             
};

static struct regval_list sensor_qxga_regs[] = { //qxga: 2048*1536
	//capture 3Mega 7.5fps
	//power down
//	{{0x30,0x08},{0x42}},
	//pll and clock setting
	{{0x30,0x34},{0x18}},                            	 
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x21}}, 	                        
#else
  {{0x30,0x35},{0x41}},                         
#endif                            	 
	{{0x30,0x36},{0x54}},                            	 
	{{0x30,0x37},{0x13}},                            	 
	{{0x31,0x08},{0x01}},                            	 
	{{0x38,0x24},{0x01}},                            	 
	{{0xff,0xff},{0x05}},//delay 5ms              
	//timing                                           
	//2048*1536                                        
	{{0x38,0x08},{0x08}}, //H size MSB                 
	{{0x38,0x09},{0x00}}, //H size LSB                 
	{{0x38,0x0a},{0x06}}, //V size MSB                 
	{{0x38,0x0b},{0x00}}, //V size LSB                 
	{{0x38,0x0c},{0x0b}}, //HTS MSB                    
	{{0x38,0x0d},{0x1c}}, //HTS LSB                    
	{{0x38,0x0e},{0x07}}, //VTS MSB                    
	{{0x38,0x0f},{0xb0}}, //LSB                        
#ifndef CSI_VER_FOR_FPGA
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x93}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x7b}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x0d}}, //50HZ step max                 
	{{0x3a,0x0d},{0x10}}, //60HZ step max                 
#else
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x49}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x3d}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x1a}}, //50HZ step max                 
	{{0x3a,0x0d},{0x20}}, //60HZ step max 
#endif        
	                                                   
//	{{0x35,0x03},{0x07}}, //AEC disable                                	           
	{{0x35,0x0c},{0x00}},                              
	{{0x35,0x0d},{0x00}},                              
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold   
                                                     
	{{0x38,0x14},{0x11}}, //horizton subsample
	{{0x38,0x15},{0x11}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x00}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9f}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x04}},	//isp vertical offset low byte 
	
//	{{0x53,0x08},{0x65}},		//sharpen manual                                                   
//  {{0x53,0x02},{0x20}}, //sharpness                    
	                                     
  {{0x40,0x02},{0xc5}},  //BLC related               
	{{0x40,0x05},{0x1a}}, // BLC related                              
	                                                                             
	{{0x36,0x18},{0x04}},                              
	{{0x36,0x12},{0x2b}},                              
	{{0x37,0x09},{0x12}},                              
	{{0x37,0x0c},{0x00}}, 
	{{0x3a,0x02},{0x07}}, //60HZ max exposure limit MSB
	{{0x3a,0x03},{0xb0}}, //60HZ max exposure limit LSB
	{{0x3a,0x14},{0x07}}, //50HZ max exposure limit MSB
	{{0x3a,0x15},{0xb0}}, //50HZ max exposure limit LSB
	{{0x40,0x04},{0x06}}, //BLC line number                                         
	{{0x48,0x37},{0x2c}},//PCLK period                              
	{{0x50,0x01},{0xa3}},//ISP effect  
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x   
	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms              
};                                      

static struct regval_list sensor_uxga_regs[] = { //UXGA: 1600*1200
		//capture 2Mega 7.5fps
	//power down
//	{{0x30,0x08},{0x42}},
	//pll and clock setting                     			                                 								                                             
	{{0x30,0x34},{0x18}},                            	                  	                                                           
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x21}}, 	                        
#else
  {{0x30,0x35},{0x41}},                         
#endif                           	                  	                                                           
	{{0x30,0x36},{0x54}},                            	                  	                                                           
	{{0x30,0x37},{0x13}},                            	                  	                                                           
	{{0x31,0x08},{0x01}},                            	                  	                                                           
	{{0x38,0x24},{0x01}},                            	                  	                                                           
	{{0xff,0xff},{0x05}},//delay 5ms                      	                
	//timing                                                      	                       	                                             
	//1600*1200                                                   	                       	                                             
	{{0x38,0x08},{0x06}}, //H size MSB                                                                                             
	{{0x38,0x09},{0x40}}, //H size LSB                                                                                             
	{{0x38,0x0a},{0x04}}, //V size MSB                                                                                             
	{{0x38,0x0b},{0xb0}}, //V size LSB                                                                                             
	{{0x38,0x0c},{0x0b}}, //HTS MSB                                                                                                
	{{0x38,0x0d},{0x1c}}, //HTS LSB                                                                                                
	{{0x38,0x0e},{0x07}}, //VTS MSB                                                                                                
	{{0x38,0x0f},{0xb0}}, //LSB                                                                                                    
#ifndef CSI_VER_FOR_FPGA
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x93}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x7b}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x0d}}, //50HZ step max                 
	{{0x3a,0x0d},{0x10}}, //60HZ step max                 
#else
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x49}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x3d}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x1a}}, //50HZ step max                 
	{{0x3a,0x0d},{0x20}}, //60HZ step max 
#endif                                                                                          
	                                                              	                       	                                             
//	{{0x35,0x03},{0x07}}, //AEC disable                                                         											                                    	                                                                                              
	{{0x35,0x0c},{0x00}},                                                                                                                 
	{{0x35,0x0d},{0x00}},                                                                                                                 
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold                                                                                      
                                                                                                                                                                                                                        
  {{0x38,0x14},{0x11}}, //horizton subsample
	{{0x38,0x15},{0x11}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x00}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9f}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x04}},	//isp vertical offset low byte 
                                                                                         	                                             
  {{0x40,0x02},{0xc5}}, //BLC related                                                       	                                             
	{{0x40,0x05},{0x12}}, //BLC related                                                                                        
//  {{0x53,0x08},{0x65}},		//sharpen manual
//  {{0x53,0x02},{0x20}},//sharpness                                                                                          
	                                                                                                                                                               	                       	                                             
	{{0x36,0x18},{0x04}},                                         	                       	                                             
	{{0x36,0x12},{0x2b}},                                         	                       	                                             
	{{0x37,0x09},{0x12}},                                         	                       	                                             
	{{0x37,0x0c},{0x00}},                                         	                       	                                             
	{{0x3a,0x02},{0x07}},//60HZ max exposure limit MSB                                                                   	                                             
	{{0x3a,0x03},{0xb0}},//60HZ max exposure limit LSB                                                                                   
	{{0x3a,0x14},{0x07}},//50HZ max exposure limit MSB                                                                                   
	{{0x3a,0x15},{0xb0}},//50HZ max exposure limit LSB                                                                                   
	{{0x40,0x04},{0x06}},//BLC line number                                                                                               
                                                                                                                                                                               
                                                                                                                       
	{{0x48,0x37},{0x2c}}, //PCLK period                                                                                                  
	{{0x50,0x01},{0xa3}}, //ISP effect  
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x                                                                                  
	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};

#if 0
static struct regval_list sensor_sxga_regs[] = { //SXGA: 1280*960
	//capture 1.3Mega 7.5fps
	//power down
//	{{0x30,0x08},{0x42}},
	//pll and clock setting                                      								                              
	{{0x30,0x34},{0x18}},                                       	              
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x21}}, 	                        
#else
  {{0x30,0x35},{0x41}},                         
#endif                                        	              
	{{0x30,0x36},{0x54}},                                       	              
	{{0x30,0x37},{0x13}},                                       	              
	{{0x31,0x08},{0x01}},                                       	              
	{{0x38,0x24},{0x01}},                                       	              
	{{0xff,0xff},{0x05}},//delay 5ms                                 	
	//timing                                                                  	                              
	//1280*960                                                                	                              
	{{0x38,0x08},{0x05}}, //H size MSB                                                                  
	{{0x38,0x09},{0x00}}, //H size LSB                                                                  
	{{0x38,0x0a},{0x03}}, //V size MSB                                                                  
	{{0x38,0x0b},{0xc0}}, //V size LSB                                                                  
	{{0x38,0x0c},{0x0b}}, //HTS MSB                                                                     
	{{0x38,0x0d},{0x1c}}, //HTS LSB                                                                     
	{{0x38,0x0e},{0x07}}, //VTS MSB                                                                     
	{{0x38,0x0f},{0xb0}}, //LSB                                                                         
#ifndef CSI_VER_FOR_FPGA
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x93}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x7b}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x0d}}, //50HZ step max                 
	{{0x3a,0x0d},{0x10}}, //60HZ step max                 
#else
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x49}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x3d}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x1a}}, //50HZ step max                 
	{{0x3a,0x0d},{0x20}}, //60HZ step max 
#endif                                                                      
	                                                                          	                              
//	{{0x35,0x03},{0x07}}, //AEC disable                                            											                     	                                                                  
	{{0x35,0x0c},{0x00}},                                                                                     
	{{0x35,0x0d},{0x00}},                                                                                     
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold                                                          
                                                                                                                                                                
  {{0x38,0x14},{0x11}}, //horizton subsample
	{{0x38,0x15},{0x11}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x00}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9f}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x04}},	//isp vertical offset low byte                                                                          	                              
                                                                            	                
  {{0x40,0x02},{0xc5}}, //BLC related                                                           
	{{0x40,0x05},{0x12}}, //BLC related                                                             
//  {{0x53,0x08},{0x65}},		//sharpen manual
//  {{0x53,0x02},{0x20}},//sharpness                                                                            
	                                                                          	                                                                                  	                              
	{{0x36,0x18},{0x04}},                                                     	                              
	{{0x36,0x12},{0x2b}},                                                     	                              
	{{0x37,0x09},{0x12}},                                                     	                              
	{{0x37,0x0c},{0x00}},                                                     	 
	{{0x3a,0x02},{0x07}}, //60HZ max exposure limit MSB                                                      
	{{0x3a,0x03},{0xb0}}, //60HZ max exposure limit LSB                                                      
	{{0x3a,0x14},{0x07}}, //50HZ max exposure limit MSB                                                      
	{{0x3a,0x15},{0xb0}}, //50HZ max exposure limit LSB                                                      
	{{0x40,0x04},{0x06}}, //BLC line number                                                                                  
                                                               	             
	{{0x48,0x37},{0x2c}}, //PCLK period
	{{0x50,0x01},{0xa3}}, //ISP effect   
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x   
	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};
#else
static struct regval_list sensor_sxga_regs[] = { //1280*960
  //for video
//	//power down
//	{{0x30,0x08},{0x42}},
//	//pll and clock setting
	{{0x30,0x34},{0x14}},
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x31}},	//0x11:60fps 0x21:30fps 0x41:15fps
#else
	{{0x30,0x35},{0x61}},	//0x11:60fps 0x21:30fps 0x41:15fps 0xa1:7.5fps
#endif
	{{0x30,0x36},{0x54}},
	{{0x30,0x37},{0x13}},
	{{0x31,0x08},{0x01}},
	{{0x38,0x24},{0x01}},
	{{0xff,0xff},{0x05}},//delay 5ms
	//timing
	//1280x960
	{{0x38,0x08},{0x05}},	//H size MSB
	{{0x38,0x09},{0x00}},	//H size LSB
	{{0x38,0x0a},{0x03}},	//V size MSB
	{{0x38,0x0b},{0xc0}},	//V size LSB
	{{0x38,0x0c},{0x07}},	//HTS MSB        
	{{0x38,0x0d},{0x64}},	//HTS LSB   
	{{0x38,0x0e},{0x03}},	//VTS MSB        
	{{0x38,0x0f},{0xd8}},	//LSB       
#ifndef CSI_VER_FOR_FPGA
	//banding step  
	{{0x3a,0x08},{0x01}},//50HZ step MSB 
	{{0x3a,0x09},{0x27}},//50HZ step LSB 
	{{0x3a,0x0a},{0x00}},//60HZ step MSB 
	{{0x3a,0x0b},{0xf6}},//60HZ step LSB 
	{{0x3a,0x0e},{0x03}},//50HZ step max 
	{{0x3a,0x0d},{0x04}},//60HZ step max 
#else
	//banding step 
	{{0x3a,0x08},{0x00}},//50HZ step MSB 
	{{0x3a,0x09},{0x93}},//50HZ step LSB 
	{{0x3a,0x0a},{0x00}},//60HZ step MSB 
	{{0x3a,0x0b},{0x7b}},//60HZ step LSB 
	{{0x3a,0x0e},{0x06}},//50HZ step max 
	{{0x3a,0x0d},{0x08}},//60HZ step max 
#endif	
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold   
	{{0x38,0x14},{0x31}}, //horizton subsample
	{{0x38,0x15},{0x31}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x04}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9b}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x06}},	//isp vertical offset low byte
	
//	{{0x53,0x08},{0x65}},		//sharpen manual
//	{{0x53,0x02},{0x00}},		//sharpen offset 1
	{{0x40,0x02},{0x45}},		//BLC related
	{{0x40,0x05},{0x18}},		//BLC related
	
	{{0x36,0x18},{0x00}},
	{{0x36,0x12},{0x29}},
	{{0x37,0x09},{0x52}},
	{{0x37,0x0c},{0x03}},
	{{0x3a,0x02},{0x02}}, //60HZ max exposure limit MSB 
	{{0x3a,0x03},{0xe0}}, //60HZ max exposure limit LSB 
	{{0x3a,0x14},{0x02}}, //50HZ max exposure limit MSB 
	{{0x3a,0x15},{0xe0}}, //50HZ max exposure limit LSB 
	
	{{0x40,0x04},{0x02}}, //BLC line number
	{{0x30,0x02},{0x1c}}, //reset JFIFO SFIFO JPG
	{{0x30,0x06},{0xc3}}, //enable xx clock
	{{0x46,0x0b},{0x37}},	//debug mode
	{{0x46,0x0c},{0x20}}, //PCLK Manuale
	{{0x48,0x37},{0x16}}, //PCLK period
	{{0x50,0x01},{0x83}}, //ISP effect
//	{{0x35,0x03},{0x00}},//AEC enable
	
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x 
//	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};
#endif

static struct regval_list sensor_xga_regs[] = { //XGA: 1024*768
	//capture 1Mega 7.5fps
	//power down
//	{{0x30,0x08},{0x42}},
	//pll and clock setting
	{{0x30,0x34},{0x18}},
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x21}}, 	                        
#else
  {{0x30,0x35},{0x41}},                         
#endif
	{{0x30,0x36},{0x54}},
	{{0x30,0x37},{0x13}},
	{{0x31,0x08},{0x01}},
	{{0x38,0x24},{0x01}},
	{{0xff,0xff},{0x05}},//delay 5ms
	//timing
	//1024*768
	{{0x38,0x08},{0x04}}, //H size MSB
	{{0x38,0x09},{0x00}}, //H size LSB
	{{0x38,0x0a},{0x03}}, //V size MSB
	{{0x38,0x0b},{0x00}}, //V size LSB
	{{0x38,0x0c},{0x0b}}, //HTS MSB    
	{{0x38,0x0d},{0x1c}}, //HTS LSB     
	{{0x38,0x0e},{0x07}}, //VTS MSB    
	{{0x38,0x0f},{0xb0}}, //LSB
#ifndef CSI_VER_FOR_FPGA
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x93}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x7b}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x0d}}, //50HZ step max                 
	{{0x3a,0x0d},{0x10}}, //60HZ step max                 
#else
	//banding step                                        
	{{0x3a,0x08},{0x00}}, //50HZ step MSB                 
	{{0x3a,0x09},{0x49}}, //50HZ step LSB                 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB                 
	{{0x3a,0x0b},{0x3d}}, //60HZ step LSB                 
	{{0x3a,0x0e},{0x1a}}, //50HZ step max                 
	{{0x3a,0x0d},{0x20}}, //60HZ step max 
#endif
//	{{0x35,0x03},{0x07}}, //AEC disable                 											                    	                                  
	{{0x35,0x0c},{0x00}},                              
	{{0x35,0x0d},{0x00}},                              
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold   
                                         	           	

	{{0x38,0x14},{0x11}}, //horizton subsample
	{{0x38,0x15},{0x11}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x00}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9f}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x04}},	//isp vertical offset low byte
	
//	{{0x53,0x08},{0x65}},		//sharpen manual
//	{{0x53,0x02},{0x20}},		//sharpen offset 1
	{{0x40,0x02},{0xc5}},		//BLC related
	{{0x40,0x05},{0x12}},		//BLC related
	   
	{{0x36,0x18},{0x00}},      
	{{0x36,0x12},{0x29}},      
	{{0x37,0x09},{0x52}},      
	{{0x37,0x0c},{0x03}},      
	{{0x3a,0x02},{0x03}},  //60HZ max exposure limit MSB 
	{{0x3a,0x03},{0xd8}},  //60HZ max exposure limit LSB     
	{{0x3a,0x14},{0x03}},  //50HZ max exposure limit MSB     
	{{0x3a,0x15},{0xd8}},  //50HZ max exposure limit LSB     
	{{0x40,0x04},{0x02}},  //BLC line number    
	
	{{0x48,0x37},{0x22}},  //PCLK period    
	{{0x50,0x01},{0xa3}},  //ISP effect
	                                       	               	                                                 	                            	           
	{{0x36,0x18},{0x04}},                  	           
	{{0x36,0x12},{0x2b}},                  	           
	{{0x37,0x09},{0x12}},                  	           
	{{0x37,0x0c},{0x00}},                  	           
	{{0x3a,0x02},{0x07}}, //60HZ max exposure limit MSB
	{{0x3a,0x03},{0xb0}}, //60HZ max exposure limit LSB
	{{0x3a,0x14},{0x07}}, //50HZ max exposure limit MSB
	{{0x3a,0x15},{0xb0}}, //50HZ max exposure limit LSB
	{{0x40,0x04},{0x06}}, //BLC line number            
	{{0x48,0x37},{0x2c}}, //PCLK period 
	{{0x50,0x01},{0xa3}}, //ISP effect
	
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x 
	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};

//for video
static struct regval_list sensor_1080p_regs[] = { //1080: 1920*1080 
	//power down
//	{{0x30,0x08},{0x42}},
	//pll and clock setting
	{{0x30,0x34},{0x18}},
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x21}},	//0x11:30fps 0x21:15fps
#else
	{{0x30,0x35},{0x41}},	//0x11:30fps 0x21:15fps 0x41:7.5fps
#endif	
	{{0x30,0x36},{0x54}},
	{{0x30,0x37},{0x13}},
	{{0x31,0x08},{0x01}},
	{{0x38,0x24},{0x01}},
	{{0xff,0xff},{0x05}},//delay 5ms
	//timing
	//1920x1080
	{{0x38,0x08},{0x07}},	//H size MSB
	{{0x38,0x09},{0x80}},	//H size LSB
	{{0x38,0x0a},{0x04}},	//V size MSB
	{{0x38,0x0b},{0x38}},	//V size LSB
	{{0x38,0x0c},{0x09}},	//HTS MSB        
	{{0x38,0x0d},{0xc4}},	//HTS LSB   
	{{0x38,0x0e},{0x04}},	//VTS MSB        
	{{0x38,0x0f},{0x60}},	//VTS LSB       
#ifndef CSI_VER_FOR_FPGA
	//banding step
	{{0x3a,0x08},{0x00}}, //50HZ step MSB 
	{{0x3a,0x09},{0xa8}}, //50HZ step LSB 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB 
	{{0x3a,0x0b},{0x8c}}, //60HZ step LSB 
	{{0x3a,0x0e},{0x06}}, //50HZ step max 
	{{0x3a,0x0d},{0x08}}, //60HZ step max 
#else
	//banding step
	{{0x3a,0x08},{0x00}}, //50HZ step MSB 
	{{0x3a,0x09},{0x54}}, //50HZ step LSB 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB 
	{{0x3a,0x0b},{0x46}}, //60HZ step LSB 
	{{0x3a,0x0e},{0x0d}}, //50HZ step max 
	{{0x3a,0x0d},{0x10}}, //60HZ step max 
#endif	
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold   
	{{0x38,0x14},{0x11}}, //horizton subsample
	{{0x38,0x15},{0x11}}, //vertical subsample
	{{0x38,0x00},{0x01}}, //x address start high byte
	{{0x38,0x01},{0x50}}, //x address start low byte  
	{{0x38,0x02},{0x01}},	//y address start high byte 
	{{0x38,0x03},{0xb2}}, //y address start low byte 
	{{0x38,0x04},{0x08}}, //x address end high byte
	{{0x38,0x05},{0xef}}, //x address end low byte 
	{{0x38,0x06},{0x05}}, //y address end high byte
	{{0x38,0x07},{0xf1}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x04}},	//isp vertical offset low byte
	
//	{{0x53,0x08},{0x65}},		//sharpen manual
//	{{0x53,0x02},{0x00}},		//sharpen offset 1
	{{0x40,0x02},{0x45}},		//BLC related
	{{0x40,0x05},{0x18}},		//BLC related
	
	{{0x36,0x18},{0x04}},
	{{0x36,0x12},{0x2b}},
	{{0x37,0x09},{0x12}},
	{{0x37,0x0c},{0x00}},
	{{0x3a,0x02},{0x04}}, //60HZ max exposure limit MSB 
	{{0x3a,0x03},{0x60}}, //60HZ max exposure limit LSB 
	{{0x3a,0x14},{0x04}}, //50HZ max exposure limit MSB 
	{{0x3a,0x15},{0x60}}, //50HZ max exposure limit LSB 
	
	{{0x40,0x04},{0x06}}, //BLC line number
	{{0x30,0x02},{0x1c}}, //reset JFIFO SFIFO JPG
	{{0x30,0x06},{0xc3}}, //enable xx clock
	{{0x46,0x0b},{0x37}},	//debug mode
	{{0x46,0x0c},{0x20}}, //PCLK Manuale
	{{0x48,0x37},{0x16}}, //PCLK period
	{{0x50,0x01},{0x83}}, //ISP effect
//	{{0x35,0x03},{0x00}},//AEC enable
	
	{{0x30,0x2c},{0x82}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x 
	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};

static struct regval_list sensor_720p_regs[] = { //1280*720
//	//power down
//	{{0x30,0x08},{0x42}},
//	//pll and clock setting
	{{0x30,0x34},{0x18}},
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x21}},	//0x11:60fps 0x21:30fps 0x41:15fps
#else
	{{0x30,0x35},{0x41}},	//0x11:60fps 0x21:30fps 0x41:15fps 0xa1:7.5fps
#endif
	{{0x30,0x36},{0x54}},
	{{0x30,0x37},{0x13}},
	{{0x31,0x08},{0x01}},
	{{0x38,0x24},{0x01}},
	{{0xff,0xff},{0x05}},//delay 5ms
	//timing
	//1280x720
	{{0x38,0x08},{0x05}},	//H size MSB
	{{0x38,0x09},{0x00}},	//H size LSB
	{{0x38,0x0a},{0x02}},	//V size MSB
	{{0x38,0x0b},{0xd0}},	//V size LSB
	{{0x38,0x0c},{0x07}},	//HTS MSB        
	{{0x38,0x0d},{0x64}},	//HTS LSB   
	{{0x38,0x0e},{0x02}},	//VTS MSB        
	{{0x38,0x0f},{0xe4}},	//LSB       
#ifndef CSI_VER_FOR_FPGA
	//banding step
	{{0x3a,0x08},{0x00}}, //50HZ step MSB 
	{{0x3a,0x09},{0xdd}}, //50HZ step LSB 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB 
	{{0x3a,0x0b},{0xb8}}, //60HZ step LSB 
	{{0x3a,0x0e},{0x03}}, //50HZ step max 
	{{0x3a,0x0d},{0x04}}, //60HZ step max 
#else
	//banding step
	{{0x3a,0x08},{0x00}}, //50HZ step MSB 
	{{0x3a,0x09},{0x6e}}, //50HZ step LSB 
	{{0x3a,0x0a},{0x00}}, //60HZ step MSB 
	{{0x3a,0x0b},{0x5c}}, //60HZ step LSB 
	{{0x3a,0x0e},{0x06}}, //50HZ step max 
	{{0x3a,0x0d},{0x08}}, //60HZ step max 
#endif	
	{{0x3c,0x07},{0x07}}, //light meter 1 thereshold   
	{{0x38,0x14},{0x31}}, //horizton subsample
	{{0x38,0x15},{0x31}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0xfa}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x06}}, //y address end high byte
	{{0x38,0x07},{0xa9}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x04}},	//isp vertical offset low byte
	
//	{{0x53,0x08},{0x65}},		//sharpen manual
//	{{0x53,0x02},{0x00}},		//sharpen offset 1
	{{0x40,0x02},{0x45}},		//BLC related
	{{0x40,0x05},{0x18}},		//BLC related
	
	{{0x36,0x18},{0x00}},
	{{0x36,0x12},{0x29}},
	{{0x37,0x09},{0x52}},
	{{0x37,0x0c},{0x03}},
	{{0x3a,0x02},{0x02}}, //60HZ max exposure limit MSB 
	{{0x3a,0x03},{0xe0}}, //60HZ max exposure limit LSB 
	{{0x3a,0x14},{0x02}}, //50HZ max exposure limit MSB 
	{{0x3a,0x15},{0xe0}}, //50HZ max exposure limit LSB 
	
	{{0x40,0x04},{0x02}}, //BLC line number
	{{0x30,0x02},{0x1c}}, //reset JFIFO SFIFO JPG
	{{0x30,0x06},{0xc3}}, //enable xx clock
	{{0x46,0x0b},{0x37}},	//debug mode
	{{0x46,0x0c},{0x20}}, //PCLK Manuale
	{{0x48,0x37},{0x16}}, //PCLK period
	{{0x50,0x01},{0x83}}, //ISP effect
//	{{0x35,0x03},{0x00}},//AEC enable
	
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x 
//	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};

static struct regval_list sensor_svga_regs[] = { //SVGA: 800*600
//	//power down
//	{{0x30,0x08},{0x42}},
//	//pll and clock setting
	{{0x30,0x34},{0x14}},                
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x31}}, 
#else
	{{0x30,0x35},{0x61}}, //0x31:30fps 0x61:15fps
#endif               
	{{0x30,0x36},{0x54}},                
	{{0x30,0x37},{0x13}},                
	{{0x31,0x08},{0x01}},                
	{{0x38,0x24},{0x01}},                
	{{0xff,0xff},{0x05}},//delay 5ms
	//timing                             
	//800x600                            
	{{0x38,0x08},{0x3 }}, //H size MSB   
	{{0x38,0x09},{0x20}}, //H size LSB   
	{{0x38,0x0a},{0x2 }}, //V size MSB   
	{{0x38,0x0b},{0x58}}, //V size LSB   
	{{0x38,0x0c},{0x07}}, //HTS MSB      
	{{0x38,0x0d},{0x68}}, //HTS LSB      
	{{0x38,0x0e},{0x03}}, //VTS MSB      
	{{0x38,0x0f},{0xd8}}, //LSB          
#ifndef CSI_VER_FOR_FPGA
	//banding step  
	{{0x3a,0x08},{0x01}},//50HZ step MSB 
	{{0x3a,0x09},{0x27}},//50HZ step LSB 
	{{0x3a,0x0a},{0x00}},//60HZ step MSB 
	{{0x3a,0x0b},{0xf6}},//60HZ step LSB 
	{{0x3a,0x0e},{0x03}},//50HZ step max 
	{{0x3a,0x0d},{0x04}},//60HZ step max 
#else
	//banding step 
	{{0x3a,0x08},{0x00}},//50HZ step MSB 
	{{0x3a,0x09},{0x93}},//50HZ step LSB 
	{{0x3a,0x0a},{0x00}},//60HZ step MSB 
	{{0x3a,0x0b},{0x7b}},//60HZ step LSB 
	{{0x3a,0x0e},{0x06}},//50HZ step max 
	{{0x3a,0x0d},{0x08}},//60HZ step max 
#endif	
	
//	{{0x35,0x03},{0x00}},  //AEC enable
	{{0x3c,0x07},{0x08}},   //light meter 1 thereshold   
  
	{{0x38,0x14},{0x31}}, //horizton subsample
	{{0x38,0x15},{0x31}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x04}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9b}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x06}},	//isp vertical offset low byte
	
//	{{0x53,0x08},{0x65}},		//sharpen manual
//	{{0x53,0x02},{0x00}},		//sharpen offset 1
	{{0x40,0x02},{0x45}},		//BLC related
	{{0x40,0x05},{0x18}},		//BLC related
	   
	{{0x36,0x18},{0x00}},      
	{{0x36,0x12},{0x29}},      
	{{0x37,0x09},{0x52}},      
	{{0x37,0x0c},{0x03}},      
	{{0x3a,0x02},{0x03}},  //60HZ max exposure limit MSB 
	{{0x3a,0x03},{0xd8}},  //60HZ max exposure limit LSB     
	{{0x3a,0x14},{0x03}},  //50HZ max exposure limit MSB     
	{{0x3a,0x15},{0xd8}},  //50HZ max exposure limit LSB     
	{{0x40,0x04},{0x02}},  //BLC line number    
	
	{{0x48,0x37},{0x22}},  //PCLK period    
	{{0x50,0x01},{0xa3}},  //ISP effect
	
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x 
//	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};

static struct regval_list sensor_vga_regs[] = { //VGA:  640*480
	
	//timing                             
	//640x480   
	//power down
//	{{0x30,0x08},{0x42}},
//	//pll and clock setting
	{{0x30,0x34},{0x1a}},                
#ifndef CSI_VER_FOR_FPGA
	{{0x30,0x35},{0x11}},                             
#else
	{{0x30,0x35},{0x21}},                            
#endif    
	{{0x30,0x36},{0x46}},                
	{{0x30,0x37},{0x13}},                
	{{0x31,0x08},{0x01}},                
	{{0x38,0x24},{0x01}},                
	{{0xff,0xff},{0x05}}, //delay 50ms 
	                         
	{{0x38,0x08},{0x02}}, //H size MSB   
	{{0x38,0x09},{0x80}}, //H size LSB   
	{{0x38,0x0a},{0x01}}, //V size MSB   
	{{0x38,0x0b},{0xe0}}, //V size LSB   
	{{0x38,0x0c},{0x07}}, //HTS MSB      
	{{0x38,0x0d},{0x68}}, //HTS LSB      
	{{0x38,0x0e},{0x03}}, //VTS MSB      
	{{0x38,0x0f},{0xd8}}, //LSB          
              
#ifndef CSI_VER_FOR_FPGA
	//banding step  
	{{0x3a,0x08},{0x01}},//50HZ step MSB 
	{{0x3a,0x09},{0x27}},//50HZ step LSB 
	{{0x3a,0x0a},{0x00}},//60HZ step MSB 
	{{0x3a,0x0b},{0xf6}},//60HZ step LSB 
	{{0x3a,0x0e},{0x03}},//50HZ step max 
	{{0x3a,0x0d},{0x04}},//60HZ step max 
#else
	//banding step 
	{{0x3a,0x08},{0x00}},//50HZ step MSB 
	{{0x3a,0x09},{0x93}},//50HZ step LSB 
	{{0x3a,0x0a},{0x00}},//60HZ step MSB 
	{{0x3a,0x0b},{0x7b}},//60HZ step LSB 
	{{0x3a,0x0e},{0x06}},//50HZ step max 
	{{0x3a,0x0d},{0x08}},//60HZ step max 
#endif	
	{{0x36,0x18},{0x00}},      
	{{0x36,0x12},{0x29}},      
	{{0x37,0x09},{0x52}},      
	{{0x37,0x0c},{0x03}},      
	{{0x3a,0x02},{0x03}},  //60HZ max exposure limit MSB 
	{{0x3a,0x03},{0xd8}},  //60HZ max exposure limit LSB     
	{{0x3a,0x14},{0x03}},  //50HZ max exposure limit MSB     
	{{0x3a,0x15},{0xd8}},  //50HZ max exposure limit LSB     
	{{0x40,0x04},{0x02}},  //BLC line number 
	
//	{{0x35,0x03},{0x00}},  //AEC eanble
	{{0x3c,0x07},{0x08}},   //light meter 1 thereshold   
  
	{{0x38,0x14},{0x31}}, //horizton subsample
	{{0x38,0x15},{0x31}}, //vertical subsample
	{{0x38,0x00},{0x00}}, //x address start high byte
	{{0x38,0x01},{0x00}}, //x address start low byte  
	{{0x38,0x02},{0x00}},	//y address start high byte 
	{{0x38,0x03},{0x04}}, //y address start low byte 
	{{0x38,0x04},{0x0a}}, //x address end high byte
	{{0x38,0x05},{0x3f}}, //x address end low byte 
	{{0x38,0x06},{0x07}}, //y address end high byte
	{{0x38,0x07},{0x9b}}, //y address end low byte 
	{{0x38,0x10},{0x00}}, //isp hortizontal offset high byte
	{{0x38,0x11},{0x10}}, //isp hortizontal offset low byte
	{{0x38,0x12},{0x00}}, //isp vertical offset high byte
	{{0x38,0x13},{0x06}},	//isp vertical offset low byte
	
//	{{0x53,0x08},{0x65}},		//sharpen manual
//	{{0x53,0x02},{0x00}},		//sharpen offset 1
	{{0x40,0x02},{0x45}},		//BLC related
	{{0x40,0x05},{0x18}},		//BLC related
	   
   
	
	{{0x48,0x37},{0x22}},  //PCLK period    
	{{0x50,0x01},{0xa3}},  //ISP effect
	
	{{0x30,0x2c},{0xc2}},//bit[7:6]: output drive capability
						//00: 1x   01: 2x  10: 3x  11: 4x 
//	//power down release
//	{{0x30,0x08},{0x02}},     
//	{{0xff,0xff},{0x32}},//delay 50ms
};

#ifdef AUTO_FPS	
//auto framerate mode
static struct regval_list sensor_auto_fps_mode[] = {
	{{0x30,0x08},{0x42}},
	{{0x3a,0x00},{0x7c}},  //night mode bit2
	{{0x3a,0x02},{0x07}},  //60HZ max exposure limit MSB
	{{0x3a,0x03},{0xb0}},  //60HZ max exposure limit LSB 
	{{0x3a,0x14},{0x07}},  //50HZ max exposure limit MSB  
	{{0x3a,0x15},{0xb0}},  //50HZ max exposure limit LSB
	{{0x30,0x08},{0x02}},
};

//auto framerate mode
static struct regval_list sensor_fix_fps_mode[] = {
	{{0x3a,0x00},{0x78}},//night mode bit2
};
#endif

//misc
static struct regval_list sensor_oe_disable_regs[] = {
{{0x30,0x17},{0x00}},
{{0x30,0x18},{0x00}},
};

static struct regval_list sensor_oe_enable_regs[] = {
{{0x30,0x17},{0x7f}},
{{0x30,0x18},{0xfc}},
};

static char sensor_af_fw_regs[] = {
	0x02, 0x0f, 0xd6, 0x02, 0x0a, 0x39, 0xc2, 0x01, 0x22, 0x22, 0x00, 0x02, 0x0f, 0xb2, 0xe5, 0x1f, //0x8000,
	0x70, 0x72, 0xf5, 0x1e, 0xd2, 0x35, 0xff, 0xef, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xe4, 0xf6, 0x08, //0x8010,
	0xf6, 0x0f, 0xbf, 0x34, 0xf2, 0x90, 0x0e, 0x93, 0xe4, 0x93, 0xff, 0xe5, 0x4b, 0xc3, 0x9f, 0x50, //0x8020,
	0x04, 0x7f, 0x05, 0x80, 0x02, 0x7f, 0xfb, 0x78, 0xbd, 0xa6, 0x07, 0x12, 0x0f, 0x04, 0x40, 0x04, //0x8030,
	0x7f, 0x03, 0x80, 0x02, 0x7f, 0x30, 0x78, 0xbc, 0xa6, 0x07, 0xe6, 0x18, 0xf6, 0x08, 0xe6, 0x78, //0x8040,
	0xb9, 0xf6, 0x78, 0xbc, 0xe6, 0x78, 0xba, 0xf6, 0x78, 0xbf, 0x76, 0x33, 0xe4, 0x08, 0xf6, 0x78, //0x8050,
	0xb8, 0x76, 0x01, 0x75, 0x4a, 0x02, 0x78, 0xb6, 0xf6, 0x08, 0xf6, 0x74, 0xff, 0x78, 0xc1, 0xf6, //0x8060,
	0x08, 0xf6, 0x75, 0x1f, 0x01, 0x78, 0xbc, 0xe6, 0x75, 0xf0, 0x05, 0xa4, 0xf5, 0x4b, 0x12, 0x0a, //0x8070,
	0xff, 0xc2, 0x37, 0x22, 0x78, 0xb8, 0xe6, 0xd3, 0x94, 0x00, 0x40, 0x02, 0x16, 0x22, 0xe5, 0x1f, //0x8080,
	0xb4, 0x05, 0x23, 0xe4, 0xf5, 0x1f, 0xc2, 0x01, 0x78, 0xb6, 0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x78, //0x8090,
	0x4e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0xa2, 0x37, 0xe4, 0x33, 0xf5, 0x3c, 0x90, 0x30, 0x28, 0xf0, //0x80a0,
	0x75, 0x1e, 0x10, 0xd2, 0x35, 0x22, 0xe5, 0x4b, 0x75, 0xf0, 0x05, 0x84, 0x78, 0xbc, 0xf6, 0x90, //0x80b0,
	0x0e, 0x8c, 0xe4, 0x93, 0xff, 0x25, 0xe0, 0x24, 0x0a, 0xf8, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x78, //0x80c0,
	0xbc, 0xe6, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0xef, 0x12, 0x0f, 0x0b, //0x80d0,
	0xd3, 0x78, 0xb7, 0x96, 0xee, 0x18, 0x96, 0x40, 0x0d, 0x78, 0xbc, 0xe6, 0x78, 0xb9, 0xf6, 0x78, //0x80e0,
	0xb6, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x90, 0x0e, 0x8c, 0xe4, 0x93, 0x12, 0x0f, 0x0b, 0xc3, 0x78, //0x80f0,
	0xc2, 0x96, 0xee, 0x18, 0x96, 0x50, 0x0d, 0x78, 0xbc, 0xe6, 0x78, 0xba, 0xf6, 0x78, 0xc1, 0xa6, //0x8100,
	0x06, 0x08, 0xa6, 0x07, 0x78, 0xb6, 0xe6, 0xfe, 0x08, 0xe6, 0xc3, 0x78, 0xc2, 0x96, 0xff, 0xee, //0x8110,
	0x18, 0x96, 0x78, 0xc3, 0xf6, 0x08, 0xa6, 0x07, 0x90, 0x0e, 0x95, 0xe4, 0x18, 0x12, 0x0e, 0xe9, //0x8120,
	0x40, 0x02, 0xd2, 0x37, 0x78, 0xbc, 0xe6, 0x08, 0x26, 0x08, 0xf6, 0xe5, 0x1f, 0x64, 0x01, 0x70, //0x8130,
	0x4a, 0xe6, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xdf, 0x40, 0x05, 0x12, 0x0e, 0xda, 0x40, 0x39, 0x12, //0x8140,
	0x0f, 0x02, 0x40, 0x04, 0x7f, 0xfe, 0x80, 0x02, 0x7f, 0x02, 0x78, 0xbd, 0xa6, 0x07, 0x78, 0xb9, //0x8150,
	0xe6, 0x24, 0x03, 0x78, 0xbf, 0xf6, 0x78, 0xb9, 0xe6, 0x24, 0xfd, 0x78, 0xc0, 0xf6, 0x12, 0x0f, //0x8160,
	0x02, 0x40, 0x06, 0x78, 0xc0, 0xe6, 0xff, 0x80, 0x04, 0x78, 0xbf, 0xe6, 0xff, 0x78, 0xbe, 0xa6, //0x8170,
	0x07, 0x75, 0x1f, 0x02, 0x78, 0xb8, 0x76, 0x01, 0x02, 0x02, 0x4a, 0xe5, 0x1f, 0x64, 0x02, 0x60, //0x8180,
	0x03, 0x02, 0x02, 0x2a, 0x78, 0xbe, 0xe6, 0xff, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xe0, 0x40, 0x08, //0x8190,
	0x12, 0x0e, 0xda, 0x50, 0x03, 0x02, 0x02, 0x28, 0x12, 0x0f, 0x02, 0x40, 0x04, 0x7f, 0xff, 0x80, //0x81a0,
	0x02, 0x7f, 0x01, 0x78, 0xbd, 0xa6, 0x07, 0x78, 0xb9, 0xe6, 0x04, 0x78, 0xbf, 0xf6, 0x78, 0xb9, //0x81b0,
	0xe6, 0x14, 0x78, 0xc0, 0xf6, 0x18, 0x12, 0x0f, 0x04, 0x40, 0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, //0x81c0,
	0x00, 0x78, 0xbf, 0xa6, 0x07, 0xd3, 0x08, 0xe6, 0x64, 0x80, 0x94, 0x80, 0x40, 0x04, 0xe6, 0xff, //0x81d0,
	0x80, 0x02, 0x7f, 0x00, 0x78, 0xc0, 0xa6, 0x07, 0xc3, 0x18, 0xe6, 0x64, 0x80, 0x94, 0xb3, 0x50, //0x81e0,
	0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, 0x33, 0x78, 0xbf, 0xa6, 0x07, 0xc3, 0x08, 0xe6, 0x64, 0x80, //0x81f0,
	0x94, 0xb3, 0x50, 0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, 0x33, 0x78, 0xc0, 0xa6, 0x07, 0x12, 0x0f, //0x8200,
	0x02, 0x40, 0x06, 0x78, 0xc0, 0xe6, 0xff, 0x80, 0x04, 0x78, 0xbf, 0xe6, 0xff, 0x78, 0xbe, 0xa6, //0x8210,
	0x07, 0x75, 0x1f, 0x03, 0x78, 0xb8, 0x76, 0x01, 0x80, 0x20, 0xe5, 0x1f, 0x64, 0x03, 0x70, 0x26, //0x8220,
	0x78, 0xbe, 0xe6, 0xff, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xe0, 0x40, 0x05, 0x12, 0x0e, 0xda, 0x40, //0x8230,
	0x09, 0x78, 0xb9, 0xe6, 0x78, 0xbe, 0xf6, 0x75, 0x1f, 0x04, 0x78, 0xbe, 0xe6, 0x75, 0xf0, 0x05, //0x8240,
	0xa4, 0xf5, 0x4b, 0x02, 0x0a, 0xff, 0xe5, 0x1f, 0xb4, 0x04, 0x10, 0x90, 0x0e, 0x94, 0xe4, 0x78, //0x8250,
	0xc3, 0x12, 0x0e, 0xe9, 0x40, 0x02, 0xd2, 0x37, 0x75, 0x1f, 0x05, 0x22, 0x30, 0x01, 0x03, 0x02, //0x8260,
	0x04, 0xc0, 0x30, 0x02, 0x03, 0x02, 0x04, 0xc0, 0x90, 0x51, 0xa5, 0xe0, 0x78, 0x93, 0xf6, 0xa3, //0x8270,
	0xe0, 0x08, 0xf6, 0xa3, 0xe0, 0x08, 0xf6, 0xe5, 0x1f, 0x70, 0x3c, 0x75, 0x1e, 0x20, 0xd2, 0x35, //0x8280,
	0x12, 0x0c, 0x7a, 0x78, 0x7e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8b, 0xa6, 0x09, 0x18, 0x76, //0x8290,
	0x01, 0x12, 0x0c, 0x5b, 0x78, 0x4e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8b, 0xe6, 0x78, 0x6e, //0x82a0,
	0xf6, 0x75, 0x1f, 0x01, 0x78, 0x93, 0xe6, 0x78, 0x90, 0xf6, 0x78, 0x94, 0xe6, 0x78, 0x91, 0xf6, //0x82b0,
	0x78, 0x95, 0xe6, 0x78, 0x92, 0xf6, 0x22, 0x79, 0x90, 0xe7, 0xd3, 0x78, 0x93, 0x96, 0x40, 0x05, //0x82c0,
	0xe7, 0x96, 0xff, 0x80, 0x08, 0xc3, 0x79, 0x93, 0xe7, 0x78, 0x90, 0x96, 0xff, 0x78, 0x88, 0x76, //0x82d0,
	0x00, 0x08, 0xa6, 0x07, 0x79, 0x91, 0xe7, 0xd3, 0x78, 0x94, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, //0x82e0,
	0x80, 0x08, 0xc3, 0x79, 0x94, 0xe7, 0x78, 0x91, 0x96, 0xff, 0x12, 0x0c, 0x8e, 0x79, 0x92, 0xe7, //0x82f0,
	0xd3, 0x78, 0x95, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, 0x80, 0x08, 0xc3, 0x79, 0x95, 0xe7, 0x78, //0x8300,
	0x92, 0x96, 0xff, 0x12, 0x0c, 0x8e, 0x12, 0x0c, 0x5b, 0x78, 0x8a, 0xe6, 0x25, 0xe0, 0x24, 0x4e, //0x8310,
	0xf8, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8a, 0xe6, 0x24, 0x6e, 0xf8, 0xa6, 0x09, 0x78, 0x8a, //0x8320,
	0xe6, 0x24, 0x01, 0xff, 0xe4, 0x33, 0xfe, 0xd3, 0xef, 0x94, 0x0f, 0xee, 0x64, 0x80, 0x94, 0x80, //0x8330,
	0x40, 0x04, 0x7f, 0x00, 0x80, 0x05, 0x78, 0x8a, 0xe6, 0x04, 0xff, 0x78, 0x8a, 0xa6, 0x07, 0xe5, //0x8340,
	0x1f, 0xb4, 0x01, 0x0a, 0xe6, 0x60, 0x03, 0x02, 0x04, 0xc0, 0x75, 0x1f, 0x02, 0x22, 0x12, 0x0c, //0x8350,
	0x7a, 0x78, 0x80, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x12, 0x0c, 0x7a, 0x78, 0x82, 0xa6, 0x06, 0x08, //0x8360,
	0xa6, 0x07, 0x78, 0x6e, 0xe6, 0x78, 0x8c, 0xf6, 0x78, 0x6e, 0xe6, 0x78, 0x8d, 0xf6, 0x7f, 0x01, //0x8370,
	0xef, 0x25, 0xe0, 0x24, 0x4f, 0xf9, 0xc3, 0x78, 0x81, 0xe6, 0x97, 0x18, 0xe6, 0x19, 0x97, 0x50, //0x8380,
	0x0a, 0x12, 0x0c, 0x82, 0x78, 0x80, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0x74, 0x6e, 0x2f, 0xf9, 0x78, //0x8390,
	0x8c, 0xe6, 0xc3, 0x97, 0x50, 0x08, 0x74, 0x6e, 0x2f, 0xf8, 0xe6, 0x78, 0x8c, 0xf6, 0xef, 0x25, //0x83a0,
	0xe0, 0x24, 0x4f, 0xf9, 0xd3, 0x78, 0x83, 0xe6, 0x97, 0x18, 0xe6, 0x19, 0x97, 0x40, 0x0a, 0x12, //0x83b0,
	0x0c, 0x82, 0x78, 0x82, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0x74, 0x6e, 0x2f, 0xf9, 0x78, 0x8d, 0xe6, //0x83c0,
	0xd3, 0x97, 0x40, 0x08, 0x74, 0x6e, 0x2f, 0xf8, 0xe6, 0x78, 0x8d, 0xf6, 0x0f, 0xef, 0x64, 0x10, //0x83d0,
	0x70, 0x9e, 0xc3, 0x79, 0x81, 0xe7, 0x78, 0x83, 0x96, 0xff, 0x19, 0xe7, 0x18, 0x96, 0x78, 0x84, //0x83e0,
	0xf6, 0x08, 0xa6, 0x07, 0xc3, 0x79, 0x8c, 0xe7, 0x78, 0x8d, 0x96, 0x08, 0xf6, 0xd3, 0x79, 0x81, //0x83f0,
	0xe7, 0x78, 0x7f, 0x96, 0x19, 0xe7, 0x18, 0x96, 0x40, 0x05, 0x09, 0xe7, 0x08, 0x80, 0x06, 0xc3, //0x8400,
	0x79, 0x7f, 0xe7, 0x78, 0x81, 0x96, 0xff, 0x19, 0xe7, 0x18, 0x96, 0xfe, 0x78, 0x86, 0xa6, 0x06, //0x8410,
	0x08, 0xa6, 0x07, 0x79, 0x8c, 0xe7, 0xd3, 0x78, 0x8b, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, 0x80, //0x8420,
	0x08, 0xc3, 0x79, 0x8b, 0xe7, 0x78, 0x8c, 0x96, 0xff, 0x78, 0x8f, 0xa6, 0x07, 0xe5, 0x1f, 0x64, //0x8430,
	0x02, 0x70, 0x69, 0x90, 0x0e, 0x91, 0x93, 0xff, 0x18, 0xe6, 0xc3, 0x9f, 0x50, 0x72, 0x12, 0x0c, //0x8440,
	0x4a, 0x12, 0x0c, 0x2f, 0x90, 0x0e, 0x8e, 0x12, 0x0c, 0x38, 0x78, 0x80, 0x12, 0x0c, 0x6b, 0x7b, //0x8450,
	0x04, 0x12, 0x0c, 0x1d, 0xc3, 0x12, 0x06, 0x45, 0x50, 0x56, 0x90, 0x0e, 0x92, 0xe4, 0x93, 0xff, //0x8460,
	0x78, 0x8f, 0xe6, 0x9f, 0x40, 0x02, 0x80, 0x11, 0x90, 0x0e, 0x90, 0xe4, 0x93, 0xff, 0xd3, 0x78, //0x8470,
	0x89, 0xe6, 0x9f, 0x18, 0xe6, 0x94, 0x00, 0x40, 0x03, 0x75, 0x1f, 0x05, 0x12, 0x0c, 0x4a, 0x12, //0x8480,
	0x0c, 0x2f, 0x90, 0x0e, 0x8f, 0x12, 0x0c, 0x38, 0x78, 0x7e, 0x12, 0x0c, 0x6b, 0x7b, 0x40, 0x12, //0x8490,
	0x0c, 0x1d, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x18, 0x75, 0x1f, 0x05, 0x22, 0xe5, 0x1f, 0xb4, 0x05, //0x84a0,
	0x0f, 0xd2, 0x01, 0xc2, 0x02, 0xe4, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, 0x33, 0xd2, 0x36, //0x84b0,
	0x22, 0xef, 0x8d, 0xf0, 0xa4, 0xa8, 0xf0, 0xcf, 0x8c, 0xf0, 0xa4, 0x28, 0xce, 0x8d, 0xf0, 0xa4, //0x84c0,
	0x2e, 0xfe, 0x22, 0xbc, 0x00, 0x0b, 0xbe, 0x00, 0x29, 0xef, 0x8d, 0xf0, 0x84, 0xff, 0xad, 0xf0, //0x84d0,
	0x22, 0xe4, 0xcc, 0xf8, 0x75, 0xf0, 0x08, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, 0xec, 0x33, 0xfc, //0x84e0,
	0xee, 0x9d, 0xec, 0x98, 0x40, 0x05, 0xfc, 0xee, 0x9d, 0xfe, 0x0f, 0xd5, 0xf0, 0xe9, 0xe4, 0xce, //0x84f0,
	0xfd, 0x22, 0xed, 0xf8, 0xf5, 0xf0, 0xee, 0x84, 0x20, 0xd2, 0x1c, 0xfe, 0xad, 0xf0, 0x75, 0xf0, //0x8500,
	0x08, 0xef, 0x2f, 0xff, 0xed, 0x33, 0xfd, 0x40, 0x07, 0x98, 0x50, 0x06, 0xd5, 0xf0, 0xf2, 0x22, //0x8510,
	0xc3, 0x98, 0xfd, 0x0f, 0xd5, 0xf0, 0xea, 0x22, 0xe8, 0x8f, 0xf0, 0xa4, 0xcc, 0x8b, 0xf0, 0xa4, //0x8520,
	0x2c, 0xfc, 0xe9, 0x8e, 0xf0, 0xa4, 0x2c, 0xfc, 0x8a, 0xf0, 0xed, 0xa4, 0x2c, 0xfc, 0xea, 0x8e, //0x8530,
	0xf0, 0xa4, 0xcd, 0xa8, 0xf0, 0x8b, 0xf0, 0xa4, 0x2d, 0xcc, 0x38, 0x25, 0xf0, 0xfd, 0xe9, 0x8f, //0x8540,
	0xf0, 0xa4, 0x2c, 0xcd, 0x35, 0xf0, 0xfc, 0xeb, 0x8e, 0xf0, 0xa4, 0xfe, 0xa9, 0xf0, 0xeb, 0x8f, //0x8550,
	0xf0, 0xa4, 0xcf, 0xc5, 0xf0, 0x2e, 0xcd, 0x39, 0xfe, 0xe4, 0x3c, 0xfc, 0xea, 0xa4, 0x2d, 0xce, //0x8560,
	0x35, 0xf0, 0xfd, 0xe4, 0x3c, 0xfc, 0x22, 0x75, 0xf0, 0x08, 0x75, 0x82, 0x00, 0xef, 0x2f, 0xff, //0x8570,
	0xee, 0x33, 0xfe, 0xcd, 0x33, 0xcd, 0xcc, 0x33, 0xcc, 0xc5, 0x82, 0x33, 0xc5, 0x82, 0x9b, 0xed, //0x8580,
	0x9a, 0xec, 0x99, 0xe5, 0x82, 0x98, 0x40, 0x0c, 0xf5, 0x82, 0xee, 0x9b, 0xfe, 0xed, 0x9a, 0xfd, //0x8590,
	0xec, 0x99, 0xfc, 0x0f, 0xd5, 0xf0, 0xd6, 0xe4, 0xce, 0xfb, 0xe4, 0xcd, 0xfa, 0xe4, 0xcc, 0xf9, //0x85a0,
	0xa8, 0x82, 0x22, 0xb8, 0x00, 0xc1, 0xb9, 0x00, 0x59, 0xba, 0x00, 0x2d, 0xec, 0x8b, 0xf0, 0x84, //0x85b0,
	0xcf, 0xce, 0xcd, 0xfc, 0xe5, 0xf0, 0xcb, 0xf9, 0x78, 0x18, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, //0x85c0,
	0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xeb, 0x33, 0xfb, 0x10, 0xd7, 0x03, 0x99, 0x40, 0x04, 0xeb, //0x85d0,
	0x99, 0xfb, 0x0f, 0xd8, 0xe5, 0xe4, 0xf9, 0xfa, 0x22, 0x78, 0x18, 0xef, 0x2f, 0xff, 0xee, 0x33, //0x85e0,
	0xfe, 0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xc9, 0x33, 0xc9, 0x10, 0xd7, 0x05, 0x9b, 0xe9, 0x9a, //0x85f0,
	0x40, 0x07, 0xec, 0x9b, 0xfc, 0xe9, 0x9a, 0xf9, 0x0f, 0xd8, 0xe0, 0xe4, 0xc9, 0xfa, 0xe4, 0xcc, //0x8600,
	0xfb, 0x22, 0x75, 0xf0, 0x10, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, 0xed, 0x33, 0xfd, 0xcc, 0x33, //0x8610,
	0xcc, 0xc8, 0x33, 0xc8, 0x10, 0xd7, 0x07, 0x9b, 0xec, 0x9a, 0xe8, 0x99, 0x40, 0x0a, 0xed, 0x9b, //0x8620,
	0xfd, 0xec, 0x9a, 0xfc, 0xe8, 0x99, 0xf8, 0x0f, 0xd5, 0xf0, 0xda, 0xe4, 0xcd, 0xfb, 0xe4, 0xcc, //0x8630,
	0xfa, 0xe4, 0xc8, 0xf9, 0x22, 0xeb, 0x9f, 0xf5, 0xf0, 0xea, 0x9e, 0x42, 0xf0, 0xe9, 0x9d, 0x42, //0x8640,
	0xf0, 0xe8, 0x9c, 0x45, 0xf0, 0x22, 0xe8, 0x60, 0x0f, 0xec, 0xc3, 0x13, 0xfc, 0xed, 0x13, 0xfd, //0x8650,
	0xee, 0x13, 0xfe, 0xef, 0x13, 0xff, 0xd8, 0xf1, 0x22, 0xe8, 0x60, 0x0f, 0xef, 0xc3, 0x33, 0xff, //0x8660,
	0xee, 0x33, 0xfe, 0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xd8, 0xf1, 0x22, 0xe4, 0x93, 0xfc, 0x74, //0x8670,
	0x01, 0x93, 0xfd, 0x74, 0x02, 0x93, 0xfe, 0x74, 0x03, 0x93, 0xff, 0x22, 0xe6, 0xfb, 0x08, 0xe6, //0x8680,
	0xf9, 0x08, 0xe6, 0xfa, 0x08, 0xe6, 0xcb, 0xf8, 0x22, 0xec, 0xf6, 0x08, 0xed, 0xf6, 0x08, 0xee, //0x8690,
	0xf6, 0x08, 0xef, 0xf6, 0x22, 0xa4, 0x25, 0x82, 0xf5, 0x82, 0xe5, 0xf0, 0x35, 0x83, 0xf5, 0x83, //0x86a0,
	0x22, 0xd0, 0x83, 0xd0, 0x82, 0xf8, 0xe4, 0x93, 0x70, 0x12, 0x74, 0x01, 0x93, 0x70, 0x0d, 0xa3, //0x86b0,
	0xa3, 0x93, 0xf8, 0x74, 0x01, 0x93, 0xf5, 0x82, 0x88, 0x83, 0xe4, 0x73, 0x74, 0x02, 0x93, 0x68, //0x86c0,
	0x60, 0xef, 0xa3, 0xa3, 0xa3, 0x80, 0xdf, 0x90, 0x38, 0x04, 0x78, 0x52, 0x12, 0x0b, 0xfd, 0x90, //0x86d0,
	0x38, 0x00, 0xe0, 0xfe, 0xa3, 0xe0, 0xfd, 0xed, 0xff, 0xc3, 0x12, 0x0b, 0x9e, 0x90, 0x38, 0x10, //0x86e0,
	0x12, 0x0b, 0x92, 0x90, 0x38, 0x06, 0x78, 0x54, 0x12, 0x0b, 0xfd, 0x90, 0x38, 0x02, 0xe0, 0xfe, //0x86f0,
	0xa3, 0xe0, 0xfd, 0xed, 0xff, 0xc3, 0x12, 0x0b, 0x9e, 0x90, 0x38, 0x12, 0x12, 0x0b, 0x92, 0xa3, //0x8700,
	0xe0, 0xb4, 0x31, 0x07, 0x78, 0x52, 0x79, 0x52, 0x12, 0x0c, 0x13, 0x90, 0x38, 0x14, 0xe0, 0xb4, //0x8710,
	0x71, 0x15, 0x78, 0x52, 0xe6, 0xfe, 0x08, 0xe6, 0x78, 0x02, 0xce, 0xc3, 0x13, 0xce, 0x13, 0xd8, //0x8720,
	0xf9, 0x79, 0x53, 0xf7, 0xee, 0x19, 0xf7, 0x90, 0x38, 0x15, 0xe0, 0xb4, 0x31, 0x07, 0x78, 0x54, //0x8730,
	0x79, 0x54, 0x12, 0x0c, 0x13, 0x90, 0x38, 0x15, 0xe0, 0xb4, 0x71, 0x15, 0x78, 0x54, 0xe6, 0xfe, //0x8740,
	0x08, 0xe6, 0x78, 0x02, 0xce, 0xc3, 0x13, 0xce, 0x13, 0xd8, 0xf9, 0x79, 0x55, 0xf7, 0xee, 0x19, //0x8750,
	0xf7, 0x79, 0x52, 0x12, 0x0b, 0xd9, 0x09, 0x12, 0x0b, 0xd9, 0xaf, 0x47, 0x12, 0x0b, 0xb2, 0xe5, //0x8760,
	0x44, 0xfb, 0x7a, 0x00, 0xfd, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x5a, 0xa6, 0x06, 0x08, 0xa6, //0x8770,
	0x07, 0xaf, 0x45, 0x12, 0x0b, 0xb2, 0xad, 0x03, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x56, 0xa6, //0x8780,
	0x06, 0x08, 0xa6, 0x07, 0xaf, 0x48, 0x78, 0x54, 0x12, 0x0b, 0xb4, 0xe5, 0x43, 0xfb, 0xfd, 0x7c, //0x8790,
	0x00, 0x12, 0x04, 0xd3, 0x78, 0x5c, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0xaf, 0x46, 0x7e, 0x00, 0x78, //0x87a0,
	0x54, 0x12, 0x0b, 0xb6, 0xad, 0x03, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x58, 0xa6, 0x06, 0x08, //0x87b0,
	0xa6, 0x07, 0xc3, 0x78, 0x5b, 0xe6, 0x94, 0x08, 0x18, 0xe6, 0x94, 0x00, 0x50, 0x05, 0x76, 0x00, //0x87c0,
	0x08, 0x76, 0x08, 0xc3, 0x78, 0x5d, 0xe6, 0x94, 0x08, 0x18, 0xe6, 0x94, 0x00, 0x50, 0x05, 0x76, //0x87d0,
	0x00, 0x08, 0x76, 0x08, 0x78, 0x5a, 0x12, 0x0b, 0xc6, 0xff, 0xd3, 0x78, 0x57, 0xe6, 0x9f, 0x18, //0x87e0,
	0xe6, 0x9e, 0x40, 0x0e, 0x78, 0x5a, 0xe6, 0x13, 0xfe, 0x08, 0xe6, 0x78, 0x57, 0x12, 0x0c, 0x08, //0x87f0,
	0x80, 0x04, 0x7e, 0x00, 0x7f, 0x00, 0x78, 0x5e, 0x12, 0x0b, 0xbe, 0xff, 0xd3, 0x78, 0x59, 0xe6, //0x8800,
	0x9f, 0x18, 0xe6, 0x9e, 0x40, 0x0e, 0x78, 0x5c, 0xe6, 0x13, 0xfe, 0x08, 0xe6, 0x78, 0x59, 0x12, //0x8810,
	0x0c, 0x08, 0x80, 0x04, 0x7e, 0x00, 0x7f, 0x00, 0xe4, 0xfc, 0xfd, 0x78, 0x62, 0x12, 0x06, 0x99, //0x8820,
	0x78, 0x5a, 0x12, 0x0b, 0xc6, 0x78, 0x57, 0x26, 0xff, 0xee, 0x18, 0x36, 0xfe, 0x78, 0x66, 0x12, //0x8830,
	0x0b, 0xbe, 0x78, 0x59, 0x26, 0xff, 0xee, 0x18, 0x36, 0xfe, 0xe4, 0xfc, 0xfd, 0x78, 0x6a, 0x12, //0x8840,
	0x06, 0x99, 0x12, 0x0b, 0xce, 0x78, 0x66, 0x12, 0x06, 0x8c, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x08, //0x8850,
	0x12, 0x0b, 0xce, 0x78, 0x66, 0x12, 0x06, 0x99, 0x78, 0x54, 0x12, 0x0b, 0xd0, 0x78, 0x6a, 0x12, //0x8860,
	0x06, 0x8c, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x0a, 0x78, 0x54, 0x12, 0x0b, 0xd0, 0x78, 0x6a, 0x12, //0x8870,
	0x06, 0x99, 0x78, 0x61, 0xe6, 0x90, 0x60, 0x01, 0xf0, 0x78, 0x65, 0xe6, 0xa3, 0xf0, 0x78, 0x69, //0x8880,
	0xe6, 0xa3, 0xf0, 0x78, 0x55, 0xe6, 0xa3, 0xf0, 0x7d, 0x01, 0x78, 0x61, 0x12, 0x0b, 0xe9, 0x24, //0x8890,
	0x01, 0x12, 0x0b, 0xa6, 0x78, 0x65, 0x12, 0x0b, 0xe9, 0x24, 0x02, 0x12, 0x0b, 0xa6, 0x78, 0x69, //0x88a0,
	0x12, 0x0b, 0xe9, 0x24, 0x03, 0x12, 0x0b, 0xa6, 0x78, 0x6d, 0x12, 0x0b, 0xe9, 0x24, 0x04, 0x12, //0x88b0,
	0x0b, 0xa6, 0x0d, 0xbd, 0x05, 0xd4, 0xc2, 0x0e, 0xc2, 0x06, 0x22, 0x85, 0x08, 0x41, 0x90, 0x30, //0x88c0,
	0x24, 0xe0, 0xf5, 0x3d, 0xa3, 0xe0, 0xf5, 0x3e, 0xa3, 0xe0, 0xf5, 0x3f, 0xa3, 0xe0, 0xf5, 0x40, //0x88d0,
	0xa3, 0xe0, 0xf5, 0x3c, 0xd2, 0x34, 0xe5, 0x41, 0x12, 0x06, 0xb1, 0x09, 0x31, 0x03, 0x09, 0x35, //0x88e0,
	0x04, 0x09, 0x3b, 0x05, 0x09, 0x3e, 0x06, 0x09, 0x41, 0x07, 0x09, 0x4a, 0x08, 0x09, 0x5b, 0x12, //0x88f0,
	0x09, 0x73, 0x18, 0x09, 0x89, 0x19, 0x09, 0x5e, 0x1a, 0x09, 0x6a, 0x1b, 0x09, 0xad, 0x80, 0x09, //0x8900,
	0xb2, 0x81, 0x0a, 0x1d, 0x8f, 0x0a, 0x09, 0x90, 0x0a, 0x1d, 0x91, 0x0a, 0x1d, 0x92, 0x0a, 0x1d, //0x8910,
	0x93, 0x0a, 0x1d, 0x94, 0x0a, 0x1d, 0x98, 0x0a, 0x17, 0x9f, 0x0a, 0x1a, 0xec, 0x00, 0x00, 0x0a, //0x8920,
	0x38, 0x12, 0x0f, 0x74, 0x22, 0x12, 0x0f, 0x74, 0xd2, 0x03, 0x22, 0xd2, 0x03, 0x22, 0xc2, 0x03, //0x8930,
	0x22, 0xa2, 0x37, 0xe4, 0x33, 0xf5, 0x3c, 0x02, 0x0a, 0x1d, 0xc2, 0x01, 0xc2, 0x02, 0xc2, 0x03, //0x8940,
	0x12, 0x0d, 0x0d, 0x75, 0x1e, 0x70, 0xd2, 0x35, 0x02, 0x0a, 0x1d, 0x02, 0x0a, 0x04, 0x85, 0x40, //0x8950,
	0x4a, 0x85, 0x3c, 0x4b, 0x12, 0x0a, 0xff, 0x02, 0x0a, 0x1d, 0x85, 0x4a, 0x40, 0x85, 0x4b, 0x3c, //0x8960,
	0x02, 0x0a, 0x1d, 0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x85, 0x40, 0x31, 0x85, 0x3f, 0x30, 0x85, 0x3e, //0x8970,
	0x2f, 0x85, 0x3d, 0x2e, 0x12, 0x0f, 0x46, 0x80, 0x1f, 0x75, 0x22, 0x00, 0x75, 0x23, 0x01, 0x74, //0x8980,
	0xff, 0xf5, 0x2d, 0xf5, 0x2c, 0xf5, 0x2b, 0xf5, 0x2a, 0x12, 0x0f, 0x46, 0x85, 0x2d, 0x40, 0x85, //0x8990,
	0x2c, 0x3f, 0x85, 0x2b, 0x3e, 0x85, 0x2a, 0x3d, 0xe4, 0xf5, 0x3c, 0x80, 0x70, 0x12, 0x0f, 0x16, //0x89a0,
	0x80, 0x6b, 0x85, 0x3d, 0x45, 0x85, 0x3e, 0x46, 0xe5, 0x47, 0xc3, 0x13, 0xff, 0xe5, 0x45, 0xc3, //0x89b0,
	0x9f, 0x50, 0x02, 0x8f, 0x45, 0xe5, 0x48, 0xc3, 0x13, 0xff, 0xe5, 0x46, 0xc3, 0x9f, 0x50, 0x02, //0x89c0,
	0x8f, 0x46, 0xe5, 0x47, 0xc3, 0x13, 0xff, 0xfd, 0xe5, 0x45, 0x2d, 0xfd, 0xe4, 0x33, 0xfc, 0xe5, //0x89d0,
	0x44, 0x12, 0x0f, 0x90, 0x40, 0x05, 0xe5, 0x44, 0x9f, 0xf5, 0x45, 0xe5, 0x48, 0xc3, 0x13, 0xff, //0x89e0,
	0xfd, 0xe5, 0x46, 0x2d, 0xfd, 0xe4, 0x33, 0xfc, 0xe5, 0x43, 0x12, 0x0f, 0x90, 0x40, 0x05, 0xe5, //0x89f0,
	0x43, 0x9f, 0xf5, 0x46, 0x12, 0x06, 0xd7, 0x80, 0x14, 0x85, 0x40, 0x48, 0x85, 0x3f, 0x47, 0x85, //0x8a00,
	0x3e, 0x46, 0x85, 0x3d, 0x45, 0x80, 0x06, 0x02, 0x06, 0xd7, 0x12, 0x0d, 0x7e, 0x90, 0x30, 0x24, //0x8a10,
	0xe5, 0x3d, 0xf0, 0xa3, 0xe5, 0x3e, 0xf0, 0xa3, 0xe5, 0x3f, 0xf0, 0xa3, 0xe5, 0x40, 0xf0, 0xa3, //0x8a20,
	0xe5, 0x3c, 0xf0, 0x90, 0x30, 0x23, 0xe4, 0xf0, 0x22, 0xc0, 0xe0, 0xc0, 0x83, 0xc0, 0x82, 0xc0, //0x8a30,
	0xd0, 0x90, 0x3f, 0x0c, 0xe0, 0xf5, 0x32, 0xe5, 0x32, 0x30, 0xe3, 0x74, 0x30, 0x36, 0x66, 0x90, //0x8a40,
	0x60, 0x19, 0xe0, 0xf5, 0x0a, 0xa3, 0xe0, 0xf5, 0x0b, 0x90, 0x60, 0x1d, 0xe0, 0xf5, 0x14, 0xa3, //0x8a50,
	0xe0, 0xf5, 0x15, 0x90, 0x60, 0x21, 0xe0, 0xf5, 0x0c, 0xa3, 0xe0, 0xf5, 0x0d, 0x90, 0x60, 0x29, //0x8a60,
	0xe0, 0xf5, 0x0e, 0xa3, 0xe0, 0xf5, 0x0f, 0x90, 0x60, 0x31, 0xe0, 0xf5, 0x10, 0xa3, 0xe0, 0xf5, //0x8a70,
	0x11, 0x90, 0x60, 0x39, 0xe0, 0xf5, 0x12, 0xa3, 0xe0, 0xf5, 0x13, 0x30, 0x01, 0x06, 0x30, 0x33, //0x8a80,
	0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x09, 0x30, 0x02, 0x06, 0x30, 0x33, 0x03, 0xd3, 0x80, 0x01, //0x8a90,
	0xc3, 0x92, 0x0a, 0x30, 0x33, 0x0c, 0x30, 0x03, 0x09, 0x20, 0x02, 0x06, 0x20, 0x01, 0x03, 0xd3, //0x8aa0,
	0x80, 0x01, 0xc3, 0x92, 0x0b, 0x90, 0x30, 0x01, 0xe0, 0x44, 0x40, 0xf0, 0xe0, 0x54, 0xbf, 0xf0, //0x8ab0,
	0xe5, 0x32, 0x30, 0xe1, 0x14, 0x30, 0x34, 0x11, 0x90, 0x30, 0x22, 0xe0, 0xf5, 0x08, 0xe4, 0xf0, //0x8ac0,
	0x30, 0x00, 0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x08, 0xe5, 0x32, 0x30, 0xe5, 0x12, 0x90, 0x56, //0x8ad0,
	0xa1, 0xe0, 0xf5, 0x09, 0x30, 0x31, 0x09, 0x30, 0x05, 0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x0d, //0x8ae0,
	0x90, 0x3f, 0x0c, 0xe5, 0x32, 0xf0, 0xd0, 0xd0, 0xd0, 0x82, 0xd0, 0x83, 0xd0, 0xe0, 0x32, 0x90, //0x8af0,
	0x0e, 0x7e, 0xe4, 0x93, 0xfe, 0x74, 0x01, 0x93, 0xff, 0xc3, 0x90, 0x0e, 0x7c, 0x74, 0x01, 0x93, //0x8b00,
	0x9f, 0xff, 0xe4, 0x93, 0x9e, 0xfe, 0xe4, 0x8f, 0x3b, 0x8e, 0x3a, 0xf5, 0x39, 0xf5, 0x38, 0xab, //0x8b10,
	0x3b, 0xaa, 0x3a, 0xa9, 0x39, 0xa8, 0x38, 0xaf, 0x4b, 0xfc, 0xfd, 0xfe, 0x12, 0x05, 0x28, 0x12, //0x8b20,
	0x0d, 0xe1, 0xe4, 0x7b, 0xff, 0xfa, 0xf9, 0xf8, 0x12, 0x05, 0xb3, 0x12, 0x0d, 0xe1, 0x90, 0x0e, //0x8b30,
	0x69, 0xe4, 0x12, 0x0d, 0xf6, 0x12, 0x0d, 0xe1, 0xe4, 0x85, 0x4a, 0x37, 0xf5, 0x36, 0xf5, 0x35, //0x8b40,
	0xf5, 0x34, 0xaf, 0x37, 0xae, 0x36, 0xad, 0x35, 0xac, 0x34, 0xa3, 0x12, 0x0d, 0xf6, 0x8f, 0x37, //0x8b50,
	0x8e, 0x36, 0x8d, 0x35, 0x8c, 0x34, 0xe5, 0x3b, 0x45, 0x37, 0xf5, 0x3b, 0xe5, 0x3a, 0x45, 0x36, //0x8b60,
	0xf5, 0x3a, 0xe5, 0x39, 0x45, 0x35, 0xf5, 0x39, 0xe5, 0x38, 0x45, 0x34, 0xf5, 0x38, 0xe4, 0xf5, //0x8b70,
	0x22, 0xf5, 0x23, 0x85, 0x3b, 0x31, 0x85, 0x3a, 0x30, 0x85, 0x39, 0x2f, 0x85, 0x38, 0x2e, 0x02, //0x8b80,
	0x0f, 0x46, 0xe0, 0xa3, 0xe0, 0x75, 0xf0, 0x02, 0xa4, 0xff, 0xae, 0xf0, 0xc3, 0x08, 0xe6, 0x9f, //0x8b90,
	0xf6, 0x18, 0xe6, 0x9e, 0xf6, 0x22, 0xff, 0xe5, 0xf0, 0x34, 0x60, 0x8f, 0x82, 0xf5, 0x83, 0xec, //0x8ba0,
	0xf0, 0x22, 0x78, 0x52, 0x7e, 0x00, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x02, 0x04, 0xc1, 0xe4, 0xfc, //0x8bb0,
	0xfd, 0x12, 0x06, 0x99, 0x78, 0x5c, 0xe6, 0xc3, 0x13, 0xfe, 0x08, 0xe6, 0x13, 0x22, 0x78, 0x52, //0x8bc0,
	0xe6, 0xfe, 0x08, 0xe6, 0xff, 0xe4, 0xfc, 0xfd, 0x22, 0xe7, 0xc4, 0xf8, 0x54, 0xf0, 0xc8, 0x68, //0x8bd0,
	0xf7, 0x09, 0xe7, 0xc4, 0x54, 0x0f, 0x48, 0xf7, 0x22, 0xe6, 0xfc, 0xed, 0x75, 0xf0, 0x04, 0xa4, //0x8be0,
	0x22, 0x12, 0x06, 0x7c, 0x8f, 0x48, 0x8e, 0x47, 0x8d, 0x46, 0x8c, 0x45, 0x22, 0xe0, 0xfe, 0xa3, //0x8bf0,
	0xe0, 0xfd, 0xee, 0xf6, 0xed, 0x08, 0xf6, 0x22, 0x13, 0xff, 0xc3, 0xe6, 0x9f, 0xff, 0x18, 0xe6, //0x8c00,
	0x9e, 0xfe, 0x22, 0xe6, 0xc3, 0x13, 0xf7, 0x08, 0xe6, 0x13, 0x09, 0xf7, 0x22, 0xad, 0x39, 0xac, //0x8c10,
	0x38, 0xfa, 0xf9, 0xf8, 0x12, 0x05, 0x28, 0x8f, 0x3b, 0x8e, 0x3a, 0x8d, 0x39, 0x8c, 0x38, 0xab, //0x8c20,
	0x37, 0xaa, 0x36, 0xa9, 0x35, 0xa8, 0x34, 0x22, 0x93, 0xff, 0xe4, 0xfc, 0xfd, 0xfe, 0x12, 0x05, //0x8c30,
	0x28, 0x8f, 0x37, 0x8e, 0x36, 0x8d, 0x35, 0x8c, 0x34, 0x22, 0x78, 0x84, 0xe6, 0xfe, 0x08, 0xe6, //0x8c40,
	0xff, 0xe4, 0x8f, 0x37, 0x8e, 0x36, 0xf5, 0x35, 0xf5, 0x34, 0x22, 0x90, 0x0e, 0x8c, 0xe4, 0x93, //0x8c50,
	0x25, 0xe0, 0x24, 0x0a, 0xf8, 0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x22, 0xe6, 0xfe, 0x08, 0xe6, 0xff, //0x8c60,
	0xe4, 0x8f, 0x3b, 0x8e, 0x3a, 0xf5, 0x39, 0xf5, 0x38, 0x22, 0x78, 0x4e, 0xe6, 0xfe, 0x08, 0xe6, //0x8c70,
	0xff, 0x22, 0xef, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x22, 0x78, 0x89, //0x8c80,
	0xef, 0x26, 0xf6, 0x18, 0xe4, 0x36, 0xf6, 0x22, 0x75, 0x89, 0x03, 0x75, 0xa8, 0x01, 0x75, 0xb8, //0x8c90,
	0x04, 0x75, 0x34, 0xff, 0x75, 0x35, 0x0e, 0x75, 0x36, 0x15, 0x75, 0x37, 0x0d, 0x12, 0x0e, 0x9a, //0x8ca0,
	0x12, 0x00, 0x09, 0x12, 0x0f, 0x16, 0x12, 0x00, 0x06, 0xd2, 0x00, 0xd2, 0x34, 0xd2, 0xaf, 0x75, //0x8cb0,
	0x34, 0xff, 0x75, 0x35, 0x0e, 0x75, 0x36, 0x49, 0x75, 0x37, 0x03, 0x12, 0x0e, 0x9a, 0x30, 0x08, //0x8cc0,
	0x09, 0xc2, 0x34, 0x12, 0x08, 0xcb, 0xc2, 0x08, 0xd2, 0x34, 0x30, 0x0b, 0x09, 0xc2, 0x36, 0x12, //0x8cd0,
	0x02, 0x6c, 0xc2, 0x0b, 0xd2, 0x36, 0x30, 0x09, 0x09, 0xc2, 0x36, 0x12, 0x00, 0x0e, 0xc2, 0x09, //0x8ce0,
	0xd2, 0x36, 0x30, 0x0e, 0x03, 0x12, 0x06, 0xd7, 0x30, 0x35, 0xd3, 0x90, 0x30, 0x29, 0xe5, 0x1e, //0x8cf0,
	0xf0, 0xb4, 0x10, 0x05, 0x90, 0x30, 0x23, 0xe4, 0xf0, 0xc2, 0x35, 0x80, 0xc1, 0xe4, 0xf5, 0x4b, //0x8d00,
	0x90, 0x0e, 0x7a, 0x93, 0xff, 0xe4, 0x8f, 0x37, 0xf5, 0x36, 0xf5, 0x35, 0xf5, 0x34, 0xaf, 0x37, //0x8d10,
	0xae, 0x36, 0xad, 0x35, 0xac, 0x34, 0x90, 0x0e, 0x6a, 0x12, 0x0d, 0xf6, 0x8f, 0x37, 0x8e, 0x36, //0x8d20,
	0x8d, 0x35, 0x8c, 0x34, 0x90, 0x0e, 0x72, 0x12, 0x06, 0x7c, 0xef, 0x45, 0x37, 0xf5, 0x37, 0xee, //0x8d30,
	0x45, 0x36, 0xf5, 0x36, 0xed, 0x45, 0x35, 0xf5, 0x35, 0xec, 0x45, 0x34, 0xf5, 0x34, 0xe4, 0xf5, //0x8d40,
	0x22, 0xf5, 0x23, 0x85, 0x37, 0x31, 0x85, 0x36, 0x30, 0x85, 0x35, 0x2f, 0x85, 0x34, 0x2e, 0x12, //0x8d50,
	0x0f, 0x46, 0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x90, 0x0e, 0x72, 0x12, 0x0d, 0xea, 0x12, 0x0f, 0x46, //0x8d60,
	0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x90, 0x0e, 0x6e, 0x12, 0x0d, 0xea, 0x02, 0x0f, 0x46, 0xe5, 0x40, //0x8d70,
	0x24, 0xf2, 0xf5, 0x37, 0xe5, 0x3f, 0x34, 0x43, 0xf5, 0x36, 0xe5, 0x3e, 0x34, 0xa2, 0xf5, 0x35, //0x8d80,
	0xe5, 0x3d, 0x34, 0x28, 0xf5, 0x34, 0xe5, 0x37, 0xff, 0xe4, 0xfe, 0xfd, 0xfc, 0x78, 0x18, 0x12, //0x8d90,
	0x06, 0x69, 0x8f, 0x40, 0x8e, 0x3f, 0x8d, 0x3e, 0x8c, 0x3d, 0xe5, 0x37, 0x54, 0xa0, 0xff, 0xe5, //0x8da0,
	0x36, 0xfe, 0xe4, 0xfd, 0xfc, 0x78, 0x07, 0x12, 0x06, 0x56, 0x78, 0x10, 0x12, 0x0f, 0x9a, 0xe4, //0x8db0,
	0xff, 0xfe, 0xe5, 0x35, 0xfd, 0xe4, 0xfc, 0x78, 0x0e, 0x12, 0x06, 0x56, 0x12, 0x0f, 0x9d, 0xe4, //0x8dc0,
	0xff, 0xfe, 0xfd, 0xe5, 0x34, 0xfc, 0x78, 0x18, 0x12, 0x06, 0x56, 0x78, 0x08, 0x12, 0x0f, 0x9a, //0x8dd0,
	0x22, 0x8f, 0x3b, 0x8e, 0x3a, 0x8d, 0x39, 0x8c, 0x38, 0x22, 0x12, 0x06, 0x7c, 0x8f, 0x31, 0x8e, //0x8de0,
	0x30, 0x8d, 0x2f, 0x8c, 0x2e, 0x22, 0x93, 0xf9, 0xf8, 0x02, 0x06, 0x69, 0x00, 0x00, 0x00, 0x00, //0x8df0,
	0x12, 0x01, 0x17, 0x08, 0x31, 0x15, 0x53, 0x54, 0x44, 0x20, 0x20, 0x20, 0x20, 0x20, 0x13, 0x01, //0x8e00,
	0x10, 0x01, 0x56, 0x40, 0x1a, 0x30, 0x29, 0x7e, 0x00, 0x30, 0x04, 0x20, 0xdf, 0x30, 0x05, 0x40, //0x8e10,
	0xbf, 0x50, 0x03, 0x00, 0xfd, 0x50, 0x27, 0x01, 0xfe, 0x60, 0x00, 0x11, 0x00, 0x3f, 0x05, 0x30, //0x8e20,
	0x00, 0x3f, 0x06, 0x22, 0x00, 0x3f, 0x01, 0x2a, 0x00, 0x3f, 0x02, 0x00, 0x00, 0x36, 0x06, 0x07, //0x8e30,
	0x00, 0x3f, 0x0b, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x40, 0xbf, 0x30, 0x01, 0x00, //0x8e40,
	0xbf, 0x30, 0x29, 0x70, 0x00, 0x3a, 0x00, 0x00, 0xff, 0x3a, 0x00, 0x00, 0xff, 0x36, 0x03, 0x36, //0x8e50,
	0x02, 0x41, 0x44, 0x58, 0x20, 0x18, 0x10, 0x0a, 0x04, 0x04, 0x00, 0x03, 0xff, 0x64, 0x00, 0x00, //0x8e60,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x06, 0x06, 0x00, 0x03, 0x51, 0x00, 0x7a, //0x8e70,
	0x50, 0x3c, 0x28, 0x1e, 0x10, 0x10, 0x50, 0x2d, 0x28, 0x16, 0x10, 0x10, 0x02, 0x00, 0x10, 0x0c, //0x8e80,
	0x10, 0x04, 0x0c, 0x6e, 0x06, 0x05, 0x00, 0xa5, 0x5a, 0x00, 0xae, 0x35, 0xaf, 0x36, 0xe4, 0xfd, //0x8e90,
	0xed, 0xc3, 0x95, 0x37, 0x50, 0x33, 0x12, 0x0f, 0xe2, 0xe4, 0x93, 0xf5, 0x38, 0x74, 0x01, 0x93, //0x8ea0,
	0xf5, 0x39, 0x45, 0x38, 0x60, 0x23, 0x85, 0x39, 0x82, 0x85, 0x38, 0x83, 0xe0, 0xfc, 0x12, 0x0f, //0x8eb0,
	0xe2, 0x74, 0x03, 0x93, 0x52, 0x04, 0x12, 0x0f, 0xe2, 0x74, 0x02, 0x93, 0x42, 0x04, 0x85, 0x39, //0x8ec0,
	0x82, 0x85, 0x38, 0x83, 0xec, 0xf0, 0x0d, 0x80, 0xc7, 0x22, 0x78, 0xbe, 0xe6, 0xd3, 0x08, 0xff, //0x8ed0,
	0xe6, 0x64, 0x80, 0xf8, 0xef, 0x64, 0x80, 0x98, 0x22, 0x93, 0xff, 0x7e, 0x00, 0xe6, 0xfc, 0x08, //0x8ee0,
	0xe6, 0xfd, 0x12, 0x04, 0xc1, 0x78, 0xc1, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0xd3, 0xef, 0x9d, 0xee, //0x8ef0,
	0x9c, 0x22, 0x78, 0xbd, 0xd3, 0xe6, 0x64, 0x80, 0x94, 0x80, 0x22, 0x25, 0xe0, 0x24, 0x0a, 0xf8, //0x8f00,
	0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x22, 0xe5, 0x3c, 0xd3, 0x94, 0x00, 0x40, 0x0b, 0x90, 0x0e, 0x88, //0x8f10,
	0x12, 0x0b, 0xf1, 0x90, 0x0e, 0x86, 0x80, 0x09, 0x90, 0x0e, 0x82, 0x12, 0x0b, 0xf1, 0x90, 0x0e, //0x8f20,
	0x80, 0xe4, 0x93, 0xf5, 0x44, 0xa3, 0xe4, 0x93, 0xf5, 0x43, 0xd2, 0x06, 0x30, 0x06, 0x03, 0xd3, //0x8f30,
	0x80, 0x01, 0xc3, 0x92, 0x0e, 0x22, 0xa2, 0xaf, 0x92, 0x32, 0xc2, 0xaf, 0xe5, 0x23, 0x45, 0x22, //0x8f40,
	0x90, 0x0e, 0x5d, 0x60, 0x0e, 0x12, 0x0f, 0xcb, 0xe0, 0xf5, 0x2c, 0x12, 0x0f, 0xc8, 0xe0, 0xf5, //0x8f50,
	0x2d, 0x80, 0x0c, 0x12, 0x0f, 0xcb, 0xe5, 0x30, 0xf0, 0x12, 0x0f, 0xc8, 0xe5, 0x31, 0xf0, 0xa2, //0x8f60,
	0x32, 0x92, 0xaf, 0x22, 0xd2, 0x01, 0xc2, 0x02, 0xe4, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, //0x8f70,
	0x33, 0xd2, 0x36, 0xd2, 0x01, 0xc2, 0x02, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, 0x33, 0x22, //0x8f80,
	0xfb, 0xd3, 0xed, 0x9b, 0x74, 0x80, 0xf8, 0x6c, 0x98, 0x22, 0x12, 0x06, 0x69, 0xe5, 0x40, 0x2f, //0x8f90,
	0xf5, 0x40, 0xe5, 0x3f, 0x3e, 0xf5, 0x3f, 0xe5, 0x3e, 0x3d, 0xf5, 0x3e, 0xe5, 0x3d, 0x3c, 0xf5, //0x8fa0,
	0x3d, 0x22, 0xc0, 0xe0, 0xc0, 0x83, 0xc0, 0x82, 0x90, 0x3f, 0x0d, 0xe0, 0xf5, 0x33, 0xe5, 0x33, //0x8fb0,
	0xf0, 0xd0, 0x82, 0xd0, 0x83, 0xd0, 0xe0, 0x32, 0x90, 0x0e, 0x5f, 0xe4, 0x93, 0xfe, 0x74, 0x01, //0x8fc0,
	0x93, 0xf5, 0x82, 0x8e, 0x83, 0x22, 0x78, 0x7f, 0xe4, 0xf6, 0xd8, 0xfd, 0x75, 0x81, 0xcd, 0x02, //0x8fd0,
	0x0c, 0x98, 0x8f, 0x82, 0x8e, 0x83, 0x75, 0xf0, 0x04, 0xed, 0x02, 0x06, 0xa5, //0x8fe0
};

/*
 * The white balance settings
 * Here only tune the R G B channel gain. 
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_auto_regs[] = {
	//simple awb
//	{{0x34,0x06},{0x0}},
//	{{0x51,0x83},{0x94}},
//	{{0x51,0x91},{0xff}},
//	{{0x51,0x92},{0x00}},

	//advanced awb
//	{{0x34,0x06},{0x00}},
	{{0x51,0x92},{0x04}},
	{{0x51,0x91},{0xf8}},
	{{0x51,0x93},{0x70}},
	{{0x51,0x94},{0xf0}},
	{{0x51,0x95},{0xf0}},
	{{0x51,0x8d},{0x3d}},
	{{0x51,0x8f},{0x54}},
	{{0x51,0x8e},{0x3d}},
	{{0x51,0x90},{0x54}},
	{{0x51,0x8b},{0xa8}},
	{{0x51,0x8c},{0xa8}},
	{{0x51,0x87},{0x18}},
	{{0x51,0x88},{0x18}},
	{{0x51,0x89},{0x6e}},
	{{0x51,0x8a},{0x68}},
	{{0x51,0x86},{0x1c}},
	{{0x51,0x81},{0x50}},
	{{0x51,0x84},{0x25}},
	{{0x51,0x82},{0x11}},
	{{0x51,0x83},{0x14}},
	{{0x51,0x84},{0x25}},
	{{0x51,0x85},{0x24}},
};

static struct regval_list sensor_wb_cloud_regs[] = {	
	{{0x34,0x06},{0x1 }},
	{{0x34,0x00},{0x6 }},
	{{0x34,0x01},{0x48}},
	{{0x34,0x02},{0x4 }},
	{{0x34,0x03},{0x0 }},
	{{0x34,0x04},{0x4 }},
	{{0x34,0x05},{0xd3}},
};

static struct regval_list sensor_wb_daylight_regs[] = {
	//tai yang guang
	{{0x34,0x06},{0x1 }},
	{{0x34,0x00},{0x6 }},
	{{0x34,0x01},{0x1c}},
	{{0x34,0x02},{0x4 }},
	{{0x34,0x03},{0x0 }},
	{{0x34,0x04},{0x4 }},
	{{0x34,0x05},{0xf3}},
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	//bai re guang
	{{0x34,0x06},{0x1 }},
	{{0x34,0x00},{0x4 }},
	{{0x34,0x01},{0x10}},
	{{0x34,0x02},{0x4 }},
	{{0x34,0x03},{0x0 }},
	{{0x34,0x04},{0x8 }},
	{{0x34,0x05},{0xb6}},
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	//ri guang deng
	{{0x34,0x06},{0x1 }},
	{{0x34,0x00},{0x5 }},
	{{0x34,0x01},{0x48}},
	{{0x34,0x02},{0x4 }},
	{{0x34,0x03},{0x0 }},
	{{0x34,0x04},{0x7 }},
	{{0x34,0x05},{0xcf}},
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	//wu si deng

};

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
//	{{0x50,0x01},{0x7f}},
	{{0x55,0x80},{0x04}},
};

static struct regval_list sensor_colorfx_bw_regs[] = {
//	{{0x50,0x01},{0xff}},
	{{0x55,0x80},{0x18}},
	{{0x55,0x83},{0x80}},
	{{0x55,0x84},{0x80}},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
//	{{0x50,0x01},{0xff}},
	{{0x55,0x80},{0x18}},
	{{0x55,0x83},{0x40}},
	{{0x55,0x84},{0xa0}},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
//	{{0x50,0x01},{0xff}},
	{{0x55,0x80},{0x40}},
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
//	{{0x50,0x01},{0xff}},
	{{0x55,0x80},{0x18}},
	{{0x55,0x83},{0x80}},
	{{0x55,0x84},{0xc0}},
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
//	{{0x50,0x01},{0xff}},
	{{0x55,0x80},{0x18}},
	{{0x55,0x83},{0x80}},
	{{0x55,0x84},{0xc0}},
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
//	{{0x50,0x01},{0xff}},
	{{0x55,0x80},{0x18}},
	{{0x55,0x83},{0xa0}},
	{{0x55,0x84},{0x40}},
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
//	{{0x50,0x01},{0xff}},
	{{0x55,0x80},{0x18}},
	{{0x55,0x83},{0x60}},
	{{0x55,0x84},{0x60}},
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
//NULL
};

#if 1
static struct regval_list sensor_sharpness_auto_regs[] = {
//	{{0x53,0x08},{0x25}},
	{{0x53,0x00},{0x08}},
	{{0x53,0x01},{0x30}},
	{{0x53,0x02},{0x10}},
	{{0x53,0x03},{0x00}},
	{{0x53,0x09},{0x08}},
	{{0x53,0x0a},{0x30}},
	{{0x53,0x0b},{0x04}},
	{{0x53,0x0c},{0x06}},
};
#endif

#if 1
static struct regval_list sensor_denoise_auto_regs[] = {
  {{0x53,0x04},{0x08}}, 
	{{0x53,0x05},{0x30}}, 
	{{0x53,0x06},{0x1c}}, 
	{{0x53,0x07},{0x2c}},
};
#endif
/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_zero_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos4_regs[] = {
//NULL
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_zero_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos4_regs[] = {
//NULL
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_zero_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos4_regs[] = {
//NULL
};

/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {
	{{0x3a,0x0f},{0x10}},	//-1.7EV
	{{0x3a,0x10},{0x08}},
	{{0x3a,0x1b},{0x10}},
	{{0x3a,0x1e},{0x08}},
	{{0x3a,0x11},{0x20}},
	{{0x3a,0x1f},{0x10}},
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{{0x3a,0x0f},{0x18}},	//-1.3EV
	{{0x3a,0x10},{0x10}},
	{{0x3a,0x1b},{0x18}},
	{{0x3a,0x1e},{0x10}},
	{{0x3a,0x11},{0x30}},
	{{0x3a,0x1f},{0x10}},
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{{0x3a,0x0f},{0x20}},	//-1.0EV
	{{0x3a,0x10},{0x18}},
	{{0x3a,0x11},{0x41}},
	{{0x3a,0x1b},{0x20}},
	{{0x3a,0x1e},{0x18}},
	{{0x3a,0x1f},{0x10}},
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{{0x3a,0x0f},{0x30}},	//-0.7EV
	{{0x3a,0x10},{0x28}},
	{{0x3a,0x11},{0x51}},
	{{0x3a,0x1b},{0x30}},
	{{0x3a,0x1e},{0x28}},
	{{0x3a,0x1f},{0x10}},
};                     

static struct regval_list sensor_ev_zero_regs[] = {
	{{0x3a,0x0f},{0x38}},		//default
	{{0x3a,0x10},{0x30}},
	{{0x3a,0x11},{0x61}},
	{{0x3a,0x1b},{0x38}},
	{{0x3a,0x1e},{0x30}},
	{{0x3a,0x1f},{0x10}},
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{{0x3a,0x0f},{0x48}},	//0.7EV
	{{0x3a,0x10},{0x40}},
	{{0x3a,0x11},{0x80}},
	{{0x3a,0x1b},{0x48}},
	{{0x3a,0x1e},{0x40}},
	{{0x3a,0x1f},{0x20}},
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{{0x3a,0x0f},{0x50}},	//1.0EV
	{{0x3a,0x10},{0x48}},
	{{0x3a,0x11},{0x90}},
	{{0x3a,0x1b},{0x50}},
	{{0x3a,0x1e},{0x48}},
	{{0x3a,0x1f},{0x20}},
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{{0x3a,0x0f},{0x58}},	//1.3EV
	{{0x3a,0x10},{0x50}},
	{{0x3a,0x11},{0x91}},
	{{0x3a,0x1b},{0x58}},
	{{0x3a,0x1e},{0x50}},
	{{0x3a,0x1f},{0x20}},
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{{0x3a,0x0f},{0x60}},	//1.7EV
	{{0x3a,0x10},{0x58}},
	{{0x3a,0x11},{0xa0}},
	{{0x3a,0x1b},{0x60}},
	{{0x3a,0x1e},{0x58}},
	{{0x3a,0x1f},{0x20}},	
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {	
	{{0x43,	0x00} , {0x30}},	//YUYV
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{{0x43,	0x00} , {0x31}},	//YVYU
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{{0x43,	0x00} , {0x33}},	//VYUY
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{{0x43,	0x00} , {0x32}},	//UYVY
};

//static struct regval_list sensor_fmt_raw[] = {
//	
//};



/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read(struct v4l2_subdev *sd, unsigned char *reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 data[REG_STEP];
	struct i2c_msg msg;
	int ret,i;
	
	for(i = 0; i < REG_ADDR_STEP; i++)
		data[i] = reg[i];
	
	for(i = REG_ADDR_STEP; i < REG_STEP; i++)
		data[i] = 0xff;
	/*
	 * Send out the register address...
	 */
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = REG_ADDR_STEP;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		csi_dev_err("Error %d on register write\n", ret);
		return ret;
	}
	/*
	 * ...then read back the result.
	 */
	
	msg.flags = I2C_M_RD;
	msg.len = REG_DATA_STEP;
	msg.buf = &data[REG_ADDR_STEP];
	
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret >= 0) {
		for(i = 0; i < REG_DATA_STEP; i++)
			value[i] = data[i+REG_ADDR_STEP];
		ret = 0;
	}
	else {
		csi_dev_err("Error %d on register read\n", ret);
	}
	return ret;
}

static int sensor_read_im(struct v4l2_subdev *sd, unsigned int addr,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 data[REG_STEP];
	struct i2c_msg msg;
	int ret,i,j;
	unsigned int retry_cnt = 0;

sensor_read_retry:	
	for(i = 0, j = REG_ADDR_STEP-1; i < REG_ADDR_STEP; i++,j--)
		data[i] = (addr&(0xff<<(j*8)))>>(j*8);
	
	for(i = REG_ADDR_STEP; i < REG_STEP; i++)
		data[i] = 0xff;
	/*
	 * Send out the register address...
	 */
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = REG_ADDR_STEP;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		csi_dev_err("Error %d on register write\n", ret);
		goto sensor_read_end;
	}
	/*
	 * ...then read back the result.
	 */
	
	msg.flags = I2C_M_RD;
	msg.len = REG_DATA_STEP;
	msg.buf = &data[REG_ADDR_STEP];
	
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret >= 0) {
		for(i = 0,j = REG_DATA_STEP-1; i < REG_DATA_STEP; i++,j--)
			*((unsigned char*)(value)+j) = data[i+REG_ADDR_STEP];
//		*value = data[2]*256+data[3];
		ret = 0;
	}
	else {
		csi_dev_err("Error %d on register read\n", ret);
	}
sensor_read_end:
  if(ret < 0) {
    if(retry_cnt < 3) {
      csi_dev_err("sensor_read retry %d\n",retry_cnt);
      retry_cnt++;
      goto sensor_read_retry;
    }
  }
	return ret;
}

static int sensor_write(struct v4l2_subdev *sd, unsigned char *reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[REG_STEP];
	int ret,i;
	
	for(i = 0; i < REG_ADDR_STEP; i++)
			data[i] = reg[i];
	for(i = REG_ADDR_STEP; i < REG_STEP; i++)
			data[i] = value[i-REG_ADDR_STEP];
	
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = REG_STEP;
	msg.buf = data;

	
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
		csi_dev_print("addr = 0x%4x, value = 0x%2x\n ",reg[0]*256+reg[1],value[0]);
		csi_dev_err("sensor_write error!\n");
	}
	return ret;
}

static int sensor_write_im(struct v4l2_subdev *sd, unsigned int addr,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[REG_STEP];
	int ret;
	unsigned int retry=0;
	
	data[0] = (addr&0xff00)>>8;
	data[1] = addr&0x00ff;
	data[2] = value;
	
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = REG_STEP;
	msg.buf = data;

sensor_write_im_transfer:
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
		if(retry<3) {
			retry++;
			csi_dev_err("sensor_write retry %d!\n",retry);
			goto sensor_write_im_transfer;
		}
		
		csi_dev_err("addr = 0x%4x, value = 0x%4x\n ",addr,value);
		csi_dev_err("sensor_write error!\n");
	}
	return ret;
}

/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *vals , uint size)
{
	int i,ret;
	unsigned int cnt;
//	unsigned char rd;
	if (size == 0)
		return 0;

	for(i = 0; i < size ; i++)
	{
		if(vals->reg_num[0] == 0xff && vals->reg_num[1] == 0xff) {
			mdelay(vals->value[0]);
		}	
		else {	
			cnt=0;
			ret = sensor_write(sd, vals->reg_num, vals->value);
			while( ret < 0 && cnt < 3)
			{
				if(ret<0)
					csi_dev_err("sensor_write_err!\n");
				ret = sensor_write(sd, vals->reg_num, vals->value);
				cnt++;
			}
			if(cnt>0)
				csi_dev_err("csi i2c retry cnt=%d\n",cnt);
			
			if(ret<0 && cnt >=3)
				return ret;
		}
		vals++;
	}
	
	return 0;
}
#if 1
static int sensor_write_continuous(struct v4l2_subdev *sd, int addr, char vals[] , uint size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[REG_STEP+32];
	char *p = vals;
	int ret,i;
	unsigned int retry = 0;
	
	while (size > 0) {
		int len = size > 32 ? 32 : size;
		data[0] = (addr&0xff00) >> 8;
		data[1] = (addr&0x00ff);
		
		for(i = REG_ADDR_STEP; i < REG_ADDR_STEP+len; i++)
			data[i] = *p++;

		msg.addr = client->addr;
		msg.flags = 0;	
		msg.len = REG_ADDR_STEP+len;
		msg.buf = data;
		retry = 0;
sensor_write_cont_retry:	
		ret = i2c_transfer(client->adapter, &msg, 1);
		
		if (ret > 0) {
			ret = 0;
		} else if (ret < 0) {
		  csi_dev_err("sensor_write_continuous error!\n");
		  if(retry < 3) {
		    retry++;
			  csi_dev_err("sensor_write_continuous retry %d!\n",retry);
			  mdelay(1);
			  goto sensor_write_cont_retry;
		  }
		}
		addr += len;
		size -= len;
	}
	return ret;
}
#else
static int sensor_write_continuous(struct v4l2_subdev *sd, int addr, char vals[] , uint size)
{
	int i,ret;
	struct regval_list reg_addr;
	
	if (size == 0)
		return -EINVAL;
	
	for(i = 0; i < size ; i++)
	{
		reg_addr.reg_num[0] = (addr&0xff00)>>8;
		reg_addr.reg_num[1] = (addr&0x00ff);
		
		ret = sensor_write(sd, reg_addr.reg_num, &vals[i]);
		if (ret < 0)
		{
			csi_dev_err("sensor_write_err!\n");
			return ret;
		}
		addr++;
	}
	
	return 0;
}
#endif

/*
 * CSI GPIO control
 */
static void csi_gpio_write(struct v4l2_subdev *sd, struct gpio_config *gpio, int level)
{
//	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
  if(gpio->gpio==GPIO_INDEX_INVALID)
  {
    csi_dev_dbg("invalid gpio\n");
    return;
  }
  
	if(gpio->mul_sel==1)
	{
	  gpio_direction_output(gpio->gpio, level);
	  gpio->data=level;
	} else {
	  csi_dev_dbg("gpio is not in output function\n");
	}
}

static void csi_gpio_set_status(struct v4l2_subdev *sd, struct gpio_config *gpio, int status)
{
//	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	
	if(1 == status && gpio->gpio!=0) {  /* output */
		if(0 != gpio_direction_output(gpio->gpio, gpio->data))
			csi_dev_dbg("gpio_direction_output failed\n");
		else {
		  csi_dev_dbg("gpio_direction_output gpio[%d]=%d\n",gpio->gpio, gpio->data);
			gpio->mul_sel=status;
		}
	} else if(0 == status && gpio->gpio!=0) {  /* input */
	  if(0 != gpio_direction_input(gpio->gpio) )
	    csi_dev_dbg("gpio_direction_input failed\n");
	  else
	    gpio->mul_sel=status;
	}
}


/* 
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;
	
	regs.reg_num[0] = 0x38;
	regs.reg_num[1] = 0x21;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_hflip!\n");
		return ret;
	}
	
	regs.value[0] &= (1<<1);
	regs.value[0] >>= 1;
		
	*value = regs.value[0];

	info->hflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	if(info->hflip == value)
		return 0;
	
	ret = sensor_read_im(sd, 0x3821, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_hflip!\n");
		return ret;
	}

	switch (value) {
		case 0:
		  rdval &= 0xf9;
			break;
		case 1:
			rdval |= 0x06;
			break;
		default:
			return -EINVAL;
	}

	ret = sensor_write_im(sd, 0x3821, rdval);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_hflip!\n");
		return ret;
	}
	mdelay(10);

	info->hflip = value;
	
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	ret = sensor_read_im(sd, 0x3820, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_vflip!\n");
		return ret;
	}
	
	rdval &= (1<<1);	
	*value = rdval;
	rdval >>= 1;
	
	info->vflip = *value;
	
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	if(info->vflip == value)
		return 0;
	
	ret = sensor_read_im(sd, 0x3820, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_vflip!\n");
		return ret;
	}

	switch (value) {
		case 0:
		  rdval &= 0xf9;
			break;
		case 1:
			rdval |= 0x06;
			break;
		default:
			return -EINVAL;
	}

	ret = sensor_write_im(sd, 0x3820, rdval);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}
	
	mdelay(10);
	
	info->vflip = value;
	
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	ret = sensor_read_im(sd, 0x3503, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_autoexp!\n");
		return ret;
	}

	if ((rdval&0x02) == 0x02) {
		*value = 0;
	}
	else
	{
		*value = 1;
	}
	
	info->autogain = *value;
	
	return 0;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	ret = sensor_read_im(sd, 0x3503, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_autogain!\n");
		return ret;
	}

	switch (value) {
		case 0:
		  rdval |= 0x02;
			break;
		case 1:
			rdval &= 0xfd;
			break;
		default:
			return -EINVAL;
	}
		
	ret = sensor_write_im(sd, 0x3503, rdval);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_autogain!\n");
		return ret;
	}
//	msleep(10);
	info->autogain = value;
	
	return 0;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	ret = sensor_read_im(sd, 0x3503, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_autoexp!\n");
		return ret;
	}

	if ((rdval&0x01) == 0x01) {
		*value = V4L2_EXPOSURE_MANUAL;
	}
	else
	{
		*value = V4L2_EXPOSURE_AUTO;
	}
	
	info->autoexp = *value;
	
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	ret = sensor_read_im(sd, 0x3503, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_autoexp!\n");
		return ret;
	}

	switch (value) {
		case V4L2_EXPOSURE_AUTO:
		  rdval &= 0xfe;
			break;
		case V4L2_EXPOSURE_MANUAL:
			rdval |= 0x01;
			break;
		case V4L2_EXPOSURE_SHUTTER_PRIORITY:
			return -EINVAL;    
		case V4L2_EXPOSURE_APERTURE_PRIORITY:
			return -EINVAL;
		default:
			return -EINVAL;
	}
		
	ret = sensor_write_im(sd, 0x3503, rdval);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}
//	msleep(10);
	info->autoexp = value;
	
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	ret = sensor_read_im(sd, 0x3406, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_autowb!\n");
		return ret;
	}

	rdval &= (1<<1);
	rdval = rdval>>1;		//0x3406 bit0 is awb enable
		
	*value = (rdval == 1)?0:1;
	info->autowb = *value;
	
	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;

	if(info->autowb == value)
		return 0;
	
	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}
	
	ret = sensor_read_im(sd, 0x3406, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_autowb!\n");
		return ret;
	}

	switch(value) {
	case 0:
		rdval |= 0x01;
		break;
	case 1:
		rdval &= 0xfe;
		break;
	default:
		break;
	}
	
	ret = sensor_write_im(sd, 0x3406, rdval);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}
	
	//msleep(10);		
	info->autowb = value;
	
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_band_filter(struct v4l2_subdev *sd, 
		__s32 *value)
{
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	int ret = 0;
	
	ret = sensor_read_im(sd, 0x3a00, &rdval);
	if (ret < 0)
		csi_dev_err("sensor_read err at sensor_g_band_filter!\n");
	
	if((rdval & (1<<5))== (1<<5))
		info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
	else {

		ret = sensor_read_im(sd, 0x3c00, &rdval);
		if (ret < 0)
			csi_dev_err("sensor_read err at sensor_g_band_filter!\n");
		
		if((rdval & (1<<2))== (1<<2))
			info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
		else
			info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
	}
	return ret;
}

static int sensor_s_band_filter(struct v4l2_subdev *sd, 
		enum v4l2_power_line_frequency value)
{
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	int ret = 0;

	if(info->band_filter == value)
		return 0;
	
	switch(value) {
		case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:	
			sensor_read_im(sd,0x3a00,&rdval);
			ret = sensor_write_im(sd,0x3a00,rdval&0xdf);//turn off band filter	
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
			sensor_write_im(sd,0x3c00,0x04);//50hz	
			sensor_write_im(sd,0x3c01,0x80);//manual band filter
			sensor_read_im(sd,0x3a00,&rdval);
			ret = sensor_write_im(sd,0x3a00,rdval|0x20);//turn on band filter
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
			sensor_write_im(sd,0x3c00,0x00);//60hz	
			sensor_write_im(sd,0x3c01,0x80);//manual band filter
			sensor_read_im(sd,0x3a00,&rdval);
			ret = sensor_write_im(sd,0x3a00,rdval|0x20);//turn on band filter
		  break;
		case V4L2_CID_POWER_LINE_FREQUENCY_AUTO:
			break;
		default:
			break;
	}
	//msleep(10);
	info->band_filter = value;
	return ret;
}

/* stuff about exposure when capturing image and video*/
static int sensor_s_denoise_value(struct v4l2_subdev *sd, unsigned char value);
unsigned char ogain,oexposurelow,oexposuremid,oexposurehigh;
unsigned int preview_exp_line,preview_fps;
unsigned long preview_pclk;

static int sensor_set_capture_exposure(struct v4l2_subdev *sd)
{
	unsigned long lines_10ms;
	unsigned int capture_expLines;
	unsigned int preview_explines;
	unsigned long previewExposure;
	unsigned long capture_Exposure;
	unsigned long capture_exposure_gain;
	unsigned long capture_gain;
	unsigned char gain,exposurelow,exposuremid,exposurehigh;
#ifndef CSI_VER_FOR_FPGA
	unsigned int capture_fps = 75;
//	unsigned int preview_fps = 3000;
#else
	unsigned int capture_fps = 25;
//	unsigned int preview_fps = 750;
#endif
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;

	csi_dev_dbg("sensor_set_capture_exposure\n");
	
//	if(info->tpf.numerator!=0)
//		preview_fps = info->tpf.denominator/info->tpf.numerator*100; 
  
	preview_fps = preview_fps*10;
	
	if(info->low_speed == 1) {
	  //preview_fps = preview_fps/2;
		capture_fps = capture_fps/2;
	}
		
	preview_explines = preview_exp_line;//984;
	capture_expLines = 1968;
	lines_10ms = capture_fps * capture_expLines *1000/10000;//*12/12;

	previewExposure = ((unsigned int)(oexposurehigh))<<12 ;
	previewExposure += ((unsigned int)oexposuremid)<<4 ;
	previewExposure += (oexposurelow >>4);
	
	if(0 == preview_explines || 0== lines_10ms)
	{
		return 0;
	}
	
	if(preview_explines == 0 || preview_fps == 0)
	  return -EFAULT;
	  
	capture_Exposure =
		((previewExposure*(capture_fps)*(capture_expLines))/
	(((preview_explines)*(preview_fps))))*5/5; 

	capture_gain = ogain ;
	
	if(0) {	//NIGHT MODE
		capture_exposure_gain = capture_Exposure * capture_gain * 2; 
	} else {
		capture_exposure_gain = capture_Exposure * capture_gain;
	}
	
	csi_dev_dbg("capture_exposure_gain = %lx\n",capture_exposure_gain);
	
	if(capture_exposure_gain < ((signed int)(capture_expLines)*16)) {
		capture_Exposure = capture_exposure_gain/16;
	} else {
		capture_Exposure = capture_expLines;
	}
	
	//banding
	capture_Exposure = capture_Exposure * 1000;
	if (capture_Exposure  > lines_10ms)
	{
		capture_Exposure /= lines_10ms;
		capture_Exposure *= lines_10ms;
	}
	capture_Exposure = capture_Exposure / 1000;
	
	if(capture_Exposure == 0)
		capture_Exposure = 1;
	
	csi_dev_dbg("capture_Exposure = %lx\n",capture_Exposure);
	
	capture_gain = (capture_exposure_gain*2/capture_Exposure + 1)/2;
	exposurelow = ((unsigned char)capture_Exposure)<<4;
	exposuremid = (unsigned char)(capture_Exposure >> 4) & 0xff;
	exposurehigh = (unsigned char)(capture_Exposure >> 12);
	gain =(unsigned char) capture_gain;
	
	sensor_read_im(sd, 0x3503, &rdval);
	csi_dev_dbg("capture:agc/aec:0x%x,gain:0x%x,exposurelow:0x%x,exposuremid:0x%x,exposurehigh:0x%x\n",\
									rdval,gain,exposurelow,exposuremid,exposurehigh);

#ifdef DENOISE_LV_AUTO	
	sensor_s_denoise_value(sd,gain*gain/0x100); //denoise via gain
#else
  sensor_s_denoise_value(sd,DENOISE_LV); //denoise fix value
#endif
	
	sensor_write_im(sd, 0x350b, gain);
	sensor_write_im(sd, 0x3502, exposurelow);
	sensor_write_im(sd, 0x3501, exposuremid);
	sensor_write_im(sd, 0x3500, exposurehigh);

	return 0;
}

static int sensor_get_pclk(struct v4l2_subdev *sd)
{
  unsigned long pclk;
  unsigned char pre_div,mul,sys_div,pll_rdiv,bit_div,sclk_rdiv;
  
  sensor_read_im(sd, 0x3037, &pre_div);
  pre_div = pre_div & 0x0f;
  
  if(pre_div == 0)
    pre_div = 1;
  
  sensor_read_im(sd, 0x3036, &mul);
  if(mul < 128)
    mul = mul;
  else
    mul = mul/2*2;
  
  sensor_read_im(sd, 0x3035, &sys_div);
  sys_div = (sys_div & 0xf0) >> 4;
  
  sensor_read_im(sd, 0x3037, &pll_rdiv);
  pll_rdiv = (pll_rdiv & 0x10) >> 4;
  pll_rdiv = pll_rdiv + 1;
  
  sensor_read_im(sd, 0x3034, &bit_div);
  bit_div = (bit_div & 0x0f);
  
  sensor_read_im(sd, 0x3108, &sclk_rdiv);
  sclk_rdiv = (sclk_rdiv & 0x03);
  sclk_rdiv = sclk_rdiv << sclk_rdiv;
  
  csi_dev_dbg("pre_div = %d,mul = %d,sys_div = %d,pll_rdiv = %d,sclk_rdiv = %d\n",\
          pre_div,mul,sys_div,pll_rdiv,sclk_rdiv);
  
  if((pre_div&&sys_div&&pll_rdiv&&sclk_rdiv) == 0)
    return -EFAULT;
  
  if(bit_div == 8)
    pclk = MCLK / pre_div * mul / sys_div / pll_rdiv / 2 / sclk_rdiv;
  else if(bit_div == 10)
    pclk = MCLK / pre_div * mul / sys_div / pll_rdiv * 2 / 5 / sclk_rdiv;
  else
    pclk = MCLK / pre_div * mul / sys_div / pll_rdiv / 1 / sclk_rdiv;
  
  csi_dev_dbg("pclk = %ld\n",pclk);
  
  preview_pclk = pclk;
  return 0;
}

static int sensor_get_fps(struct v4l2_subdev *sd)
{
  unsigned char vts_low,vts_high,hts_low,hts_high,vts_extra_high,vts_extra_low;
  unsigned long vts,hts,vts_extra;
  
  sensor_read_im(sd, 0x380c, &hts_high);
  sensor_read_im(sd, 0x380d, &hts_low);
  sensor_read_im(sd, 0x380e, &vts_high);
  sensor_read_im(sd, 0x380f, &vts_low);
  sensor_read_im(sd, 0x350c, &vts_extra_high);
  sensor_read_im(sd, 0x350d, &vts_extra_low);
   
  hts = hts_high * 256 + hts_low;
  vts = vts_high * 256 + vts_low;
  vts_extra = vts_extra_high * 256 + vts_extra_low;
  
  if((hts&&(vts+vts_extra)) == 0)
    return -EFAULT;
    
  if(sensor_get_pclk(sd))
    csi_dev_err("get pclk error!\n");

  preview_fps = preview_pclk / ((vts_extra+vts) * hts);
  csi_dev_dbg("preview fps = %d\n",preview_fps);
  
  return 0;
}

static int sensor_get_preview_exposure(struct v4l2_subdev *sd)
{
	unsigned char vts_low,vts_high,vts_extra_high,vts_extra_low;
	unsigned long vts,vts_extra;
	
	sensor_read_im(sd, 0x350b, &ogain);
	sensor_read_im(sd, 0x3502, &oexposurelow);
	sensor_read_im(sd, 0x3501, &oexposuremid);
	sensor_read_im(sd, 0x3500, &oexposurehigh);	
	sensor_read_im(sd, 0x380e, &vts_high);
  sensor_read_im(sd, 0x380f, &vts_low);
	sensor_read_im(sd, 0x350c, &vts_extra_high);
	sensor_read_im(sd, 0x350d, &vts_extra_low);
	
	vts = vts_high * 256 + vts_low;
	vts_extra = vts_extra_high * 256 + vts_extra_low;
	preview_exp_line = vts + vts_extra;
  
	csi_dev_dbg("preview_exp_line = %d\n",preview_exp_line);
	
	csi_dev_dbg("preview:gain:0x%x,exposurelow:0x%x,exposuremid:0x%x,exposurehigh:0x%x\n",\
									ogain,oexposurelow,oexposuremid,oexposurehigh);
	
	return 0;
}

static void sensor_s_ae_ratio(struct work_struct *work)
{
	csi_dev_dbg("sensor_s_ae_ratio\n");
	sensor_write_im(glb_sd, 0x3a05, 0x30);//normal aec ratio
}

static int sensor_set_preview_exposure(struct v4l2_subdev *sd)
{	
	unsigned char rdval;
	sensor_read_im(sd, 0x3503, &rdval);
	csi_dev_dbg("preview:agc/aec:0x%x,gain:0x%x,exposurelow:0x%x,exposuremid:0x%x,exposurehigh:0x%x\n",\
									rdval,ogain,oexposurelow,oexposuremid,oexposurehigh);
	
//	sensor_read_im(sd, 0x3001, &rdval);
//	sensor_write_im(sd, 0x3001, rdval|0x3);	//reset AE
//	msleep(10);
//	sensor_write_im(sd, 0x3001, rdval);	//release reset AE
	
	sensor_write_im(sd, 0x350b, ogain);
	sensor_write_im(sd, 0x3502, oexposurelow);
	sensor_write_im(sd, 0x3501, oexposuremid);
	sensor_write_im(sd, 0x3500, oexposurehigh);
	
//	sensor_write_im(sd, 0x3a05, 0x3f);//max aec ratio
//	csi_dev_dbg("set max aec ratio\n");
//	schedule_delayed_work(&sensor_s_ae_ratio_work, msecs_to_jiffies(500));
	return 0;
}

/* stuff about auto focus */

static int sensor_download_af_fw(struct v4l2_subdev *sd)
{
	int ret,cnt;
	unsigned char rdval;
	int reload_cnt = 0;
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	
	struct regval_list af_fw_reset_reg[] = {
		{{0x30,0x00},{0x20}},
	};
	struct regval_list af_fw_start_reg[] = {
		{{0x30,0x22},{0x00}},
		{{0x30,0x23},{0x00}},
		{{0x30,0x24},{0x00}},
		{{0x30,0x25},{0x00}},
		{{0x30,0x26},{0x00}},
		{{0x30,0x27},{0x00}},
		{{0x30,0x28},{0x00}},
		{{0x30,0x29},{0x7f}},
		{{0x30,0x00},{0x00}},	//start firmware for af
	};
		
reload_af_fw:
	//reset sensor MCU
	ret = sensor_write_array(sd, af_fw_reset_reg, ARRAY_SIZE(af_fw_reset_reg));
	if(ret < 0) {
		csi_dev_err("reset sensor MCU error\n");
		goto af_dl_end;
	}
		
	//download af fw
	ret =sensor_write_continuous(sd, 0x8000, sensor_af_fw_regs, ARRAY_SIZE(sensor_af_fw_regs));
	if(ret < 0) {
		csi_dev_err("download af fw error\n");
		goto af_dl_end;
	}
	//start af firmware
	ret = sensor_write_array(sd, af_fw_start_reg, ARRAY_SIZE(af_fw_start_reg));
	if(ret < 0) {
		csi_dev_err("start af firmware error\n");
		goto af_dl_end;
	}
	
	msleep(10);
	//check the af firmware status
	rdval = 0xff;
	cnt = 0;

	while(rdval!=0x70) {
		ret = sensor_read_im(sd, 0x3029, &rdval);
		if (ret < 0)
		{
			csi_dev_err("sensor check the af firmware status err !\n");
			goto af_dl_end;
		}
		cnt++;
		if(cnt > 3) {
			csi_dev_err("AF firmware check status time out !\n");	
			ret = -EFAULT;
			goto af_dl_end;
		}
		mdelay(5);
	}
	csi_dev_print("AF firmware check status complete,0x3029 = 0x%x\n",rdval);
	
#if DEV_DBG_EN == 1	
	sensor_read_im(sd, 0x3000, &rdval);
	csi_dev_print("0x3000 = 0x%x\n",rdval);
	sensor_read_im(sd, 0x3004, &rdval);
	csi_dev_print("0x3004 = 0x%x\n",rdval);
	sensor_read_im(sd, 0x3001, &rdval);
	csi_dev_print("0x3001 = 0x%x\n",rdval);
	sensor_read_im(sd, 0x3005, &rdval);
	csi_dev_print("0x3005 = 0x%x\n",rdval);
#endif

af_dl_end: 
  if(ret) {
    reload_cnt++;
  	if(reload_cnt < 3) {
  		csi_dev_err("AF reload retry cnt = %d!\n",reload_cnt);
  		csi_gpio_write(sd,&dev->standby_io,CSI_STBY_ON);
  		mdelay(10);
  		csi_gpio_write(sd,&dev->standby_io,CSI_STBY_OFF);
  		mdelay(10);
  		goto reload_af_fw;
  	}
  }
  return ret;
}

static int sensor_g_single_af(struct v4l2_subdev *sd)
{
	unsigned char rdval;
	int ret;
	struct sensor_info *info = to_state(sd);
	
	csi_dev_dbg("sensor_g_single_af\n");
	
	rdval = 0xff;
	
	ret = sensor_read_im(sd, 0x3029, &rdval);
	if (ret < 0)
	{
		csi_dev_err("sensor get af focused status err !\n");
		info->focus_status = 0;	//idle
		return ret;
	}

	if(rdval == 0x10)
	{
		info->focus_status = 0;	//idle
		csi_dev_print("Single AF focus ok,value = 0x%x\n",rdval);
#ifdef AF_FAST
		sensor_write_array(sd, sensor_auto_fps_mode , ARRAY_SIZE(sensor_auto_fps_mode));
#endif
		return 0;
	}	
	
	csi_dev_dbg("Single AF focus is running,value = 0x%x\n",rdval);
	
	return EBUSY;
}

static int sensor_g_autofocus_ctrl(struct v4l2_subdev *sd,
		struct v4l2_control *ctrl);

static int sensor_s_single_af(struct v4l2_subdev *sd)
{
	int ret;
	struct sensor_info *info = to_state(sd);
#if 0
	struct v4l2_control ctrl;
	unsigned int cnt;
#endif
	
	csi_dev_print("sensor_s_single_af\n");
	//trig single af
		
	info->focus_status = 0;	//idle	
		
	ret = sensor_write_im(sd, 0x3022, 0x03);
	if (ret < 0) {
		csi_dev_err("sensor tigger single af err !\n");
		return ret;
	}

#if 0	
	//wait for af complete
	cnt = 0;
	ctrl.id = V4L2_CID_CAMERA_AF_CTRL;
	ctrl.value = V4L2_AF_TRIG_SINGLE;
	ret = -1;
	while(ret != 0)
	{
		msleep(100);
		ret = sensor_g_autofocus_ctrl(sd,&ctrl);
		cnt++;
		if(cnt>20) {
			csi_dev_err("Single AF is timeout\n");
			return -EFAULT;
		}
	}
	
	csi_dev_print("Single AF is complete\n");
#endif
	
#ifdef AF_FAST
  sensor_write_array(sd, sensor_fix_fps_mode , ARRAY_SIZE(sensor_fix_fps_mode));
#endif
	
	info->focus_status = 1;	//busy
  return 0;
}

static int sensor_s_continueous_af(struct v4l2_subdev *sd)
{
	int ret;
	csi_dev_print("sensor_s_continueous_af\n");

	ret = sensor_write_im(sd, 0x3022, 0x04);
	if (ret < 0)
	{
		csi_dev_err("sensor tigger continueous af err !\n");
		return ret;
	}
  return 0;
}

static int sensor_s_pause_af(struct v4l2_subdev *sd)
{
	int ret;
	
	//pause af poisition
	csi_dev_print("sensor_s_pause_af\n");

	ret = sensor_write_im(sd, 0x3022, 0x06);
	if (ret < 0)
	{
		csi_dev_err("sensor pause af err !\n");
		return ret;
	}
		
	msleep(5);

	return 0;
}

static int sensor_s_release_af(struct v4l2_subdev *sd)
{
	int ret;
	//release focus
	csi_dev_print("sensor_s_release_af\n");
	
	//release single af
	ret = sensor_write_im(sd, 0x3022, 0x08);
	if (ret < 0)
	{
		csi_dev_err("release focus err !\n");
		return ret;
	}
	return 0;
}

static int sensor_s_af_zone(struct v4l2_subdev *sd, unsigned int xc, unsigned int yc)
{
	struct sensor_info *info = to_state(sd);
	int ret;
	
	//csi_dev_print("sensor_s_af_zone\n");
	csi_dev_dbg("af zone input xc=%d,yc=%d\n",xc,yc);
	
	if(info->width == 0 || info->height == 0) {
		csi_dev_err("current width or height is zero!\n");
		return -EINVAL;
	}
		
	if(info->focus_status == 1)	//can not set af zone when focus is busy
		return 0;
	
	xc = (xc * 80 * 2 / info->width + 1) / 2;
	if((info->width == HD720_WIDTH && info->height == HD720_HEIGHT) || \
		 (info->width == HD1080_WIDTH && info->height == HD1080_HEIGHT)) {
		yc = (yc * 45 * 2 / info->height + 1) / 2;
	} else {
		yc = (yc * 60 * 2 / info->height + 1) / 2;
	}
	
	csi_dev_dbg("af zone after xc=%d,yc=%d\n",xc,yc);
	
	//set x center
	ret = sensor_write_im(sd, 0x3024, xc);
	if (ret < 0)
	{
		csi_dev_err("sensor_s_af_zone_xc error!\n");
		return ret;
	}
	//set y center
	ret = sensor_write_im(sd, 0x3025, yc);
	if (ret < 0)
	{
		csi_dev_err("sensor_s_af_zone_yc error!\n");
		return ret;
	}
	
	//msleep(5);
	
	//set af zone
	ret = sensor_write_im(sd, 0x3022, 0x81);
	if (ret < 0)
	{
		csi_dev_err("sensor_s_af_zone error!\n");
		return ret;
	}

	//msleep(5);
	
	return 0;
}

#if 1
static int sensor_s_relaunch_af_zone(struct v4l2_subdev *sd)
{
	int ret;
	//relaunch defalut af zone
	csi_dev_print("sensor_s_relaunch_af_zone\n");
	ret = sensor_write_im(sd, 0x3022, 0x80);
	if (ret < 0)
	{
		csi_dev_err("relaunch defalut af zone err !\n");
		return ret;
	}
	msleep(5);
	return 0;
}
#endif

#if 1
static int sensor_s_sharpness_auto(struct v4l2_subdev *sd)
{
	unsigned char rdval;
	sensor_read_im(sd,0x5308,&rdval);
	sensor_write_im(sd,0x5308,rdval&0xbf); //bit6 is sharpness manual enable
	return sensor_write_array(sd, sensor_sharpness_auto_regs, ARRAY_SIZE(sensor_sharpness_auto_regs));
}
#endif

static int sensor_s_sharpness_value(struct v4l2_subdev *sd, unsigned char value)
{
	unsigned char rdval;
	sensor_read_im(sd,0x5308,&rdval);
	sensor_write_im(sd,0x5308,rdval|0x40); //bit6 is sharpness manual enable
	return sensor_write_im(sd,0x5302,value); 
}

#if 1
static int sensor_s_denoise_auto(struct v4l2_subdev *sd)
{
	unsigned char rdval;
	sensor_read_im(sd,0x5308,&rdval);
	sensor_write_im(sd,0x5308,rdval&0xef); //bit4 is denoise manual enable
	return sensor_write_array(sd, sensor_denoise_auto_regs, ARRAY_SIZE(sensor_denoise_auto_regs));
}
#endif

static int sensor_s_denoise_value(struct v4l2_subdev *sd, unsigned char value)
{
	unsigned char rdval;
	sensor_read_im(sd,0x5308,&rdval);
	sensor_write_im(sd,0x5308,rdval|0x10); //bit4 is denoise manual enable
	return sensor_write_im(sd,0x5306,value); 
}

/* *********************************************end of ******************************************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	if(info->brightness == value)
		return 0;
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_brightness_neg4_regs, ARRAY_SIZE(sensor_brightness_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_brightness_neg3_regs, ARRAY_SIZE(sensor_brightness_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_brightness_neg2_regs, ARRAY_SIZE(sensor_brightness_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_brightness_neg1_regs, ARRAY_SIZE(sensor_brightness_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_brightness_zero_regs, ARRAY_SIZE(sensor_brightness_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_brightness_pos1_regs, ARRAY_SIZE(sensor_brightness_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_brightness_pos2_regs, ARRAY_SIZE(sensor_brightness_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_brightness_pos3_regs, ARRAY_SIZE(sensor_brightness_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_brightness_pos4_regs, ARRAY_SIZE(sensor_brightness_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_brightness!\n");
		return ret;
	}
//	mdelay(10);
	info->brightness = value;
	return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	if(info->contrast == value)
		return 0;
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_contrast_neg4_regs, ARRAY_SIZE(sensor_contrast_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_contrast_neg3_regs, ARRAY_SIZE(sensor_contrast_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_contrast_neg2_regs, ARRAY_SIZE(sensor_contrast_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_contrast_neg1_regs, ARRAY_SIZE(sensor_contrast_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_contrast_zero_regs, ARRAY_SIZE(sensor_contrast_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_contrast_pos1_regs, ARRAY_SIZE(sensor_contrast_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_contrast_pos2_regs, ARRAY_SIZE(sensor_contrast_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_contrast_pos3_regs, ARRAY_SIZE(sensor_contrast_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_contrast_pos4_regs, ARRAY_SIZE(sensor_contrast_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_contrast!\n");
		return ret;
	}
//	mdelay(10);
	info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	if(info->saturation == value)
		return 0;
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_saturation_neg4_regs, ARRAY_SIZE(sensor_saturation_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_saturation_neg3_regs, ARRAY_SIZE(sensor_saturation_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_saturation_neg2_regs, ARRAY_SIZE(sensor_saturation_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_saturation_neg1_regs, ARRAY_SIZE(sensor_saturation_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_saturation_zero_regs, ARRAY_SIZE(sensor_saturation_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_saturation_pos1_regs, ARRAY_SIZE(sensor_saturation_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_saturation_pos2_regs, ARRAY_SIZE(sensor_saturation_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_saturation_pos3_regs, ARRAY_SIZE(sensor_saturation_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_saturation_pos4_regs, ARRAY_SIZE(sensor_saturation_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_saturation!\n");
		return ret;
	}
//	mdelay(10);
	info->saturation = value;
	return 0;
}

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->exp;
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	if(info->exp == value)
		return 0;
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_ev_neg4_regs, ARRAY_SIZE(sensor_ev_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_ev_neg3_regs, ARRAY_SIZE(sensor_ev_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_ev_neg2_regs, ARRAY_SIZE(sensor_ev_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_ev_neg1_regs, ARRAY_SIZE(sensor_ev_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_ev_zero_regs, ARRAY_SIZE(sensor_ev_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_ev_pos1_regs, ARRAY_SIZE(sensor_ev_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_ev_pos2_regs, ARRAY_SIZE(sensor_ev_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_ev_pos3_regs, ARRAY_SIZE(sensor_ev_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_ev_pos4_regs, ARRAY_SIZE(sensor_ev_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_exp!\n");
		return ret;
	}
//	mdelay(10);
	info->exp = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_whiteblance *wb_type = (enum v4l2_whiteblance*)value;
	
	*wb_type = info->wb;
	
	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
		enum v4l2_whiteblance value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	if(info->capture_mode == V4L2_MODE_IMAGE)
		return 0;
	
	if(info->wb == value)
		return 0;
	
	if (value == V4L2_WB_AUTO) {
		ret = sensor_s_autowb(sd, 1);
//		ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	} 
	else {
		ret = sensor_s_autowb(sd, 0);
		if(ret < 0) {
			csi_dev_err("sensor_s_autowb error, return %x!\n",ret);
			return ret;
		}
		
		switch (value) {
			case V4L2_WB_CLOUD:
			  ret = sensor_write_array(sd, sensor_wb_cloud_regs, ARRAY_SIZE(sensor_wb_cloud_regs));
				break;
			case V4L2_WB_DAYLIGHT:
				ret = sensor_write_array(sd, sensor_wb_daylight_regs, ARRAY_SIZE(sensor_wb_daylight_regs));
				break;
			case V4L2_WB_INCANDESCENCE:
				ret = sensor_write_array(sd, sensor_wb_incandescence_regs, ARRAY_SIZE(sensor_wb_incandescence_regs));
				break;    
			case V4L2_WB_FLUORESCENT:
				ret = sensor_write_array(sd, sensor_wb_fluorescent_regs, ARRAY_SIZE(sensor_wb_fluorescent_regs));
				break;
			case V4L2_WB_TUNGSTEN:   
				ret = sensor_write_array(sd, sensor_wb_tungsten_regs, ARRAY_SIZE(sensor_wb_tungsten_regs));
				break;
			default:
				return -EINVAL;
		} 
	}
	
	if (ret < 0) {
		csi_dev_err("sensor_s_wb error, return %x!\n",ret);
		return ret;
	}
	
//	mdelay(10);
	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
		__s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx*)value;
	
	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
		enum v4l2_colorfx value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	if(info->clrfx == value)
		return 0;
	
	switch (value) {
	case V4L2_COLORFX_NONE:
	  ret = sensor_write_array(sd, sensor_colorfx_none_regs, ARRAY_SIZE(sensor_colorfx_none_regs));
		break;
	case V4L2_COLORFX_BW:
		ret = sensor_write_array(sd, sensor_colorfx_bw_regs, ARRAY_SIZE(sensor_colorfx_bw_regs));
		break;  
	case V4L2_COLORFX_SEPIA:
		ret = sensor_write_array(sd, sensor_colorfx_sepia_regs, ARRAY_SIZE(sensor_colorfx_sepia_regs));
		break;   
	case V4L2_COLORFX_NEGATIVE:
		ret = sensor_write_array(sd, sensor_colorfx_negative_regs, ARRAY_SIZE(sensor_colorfx_negative_regs));
		break;
	case V4L2_COLORFX_EMBOSS:   
		ret = sensor_write_array(sd, sensor_colorfx_emboss_regs, ARRAY_SIZE(sensor_colorfx_emboss_regs));
		break;
	case V4L2_COLORFX_SKETCH:     
		ret = sensor_write_array(sd, sensor_colorfx_sketch_regs, ARRAY_SIZE(sensor_colorfx_sketch_regs));
		break;
	case V4L2_COLORFX_SKY_BLUE:
		ret = sensor_write_array(sd, sensor_colorfx_sky_blue_regs, ARRAY_SIZE(sensor_colorfx_sky_blue_regs));
		break;
	case V4L2_COLORFX_GRASS_GREEN:
		ret = sensor_write_array(sd, sensor_colorfx_grass_green_regs, ARRAY_SIZE(sensor_colorfx_grass_green_regs));
		break;
	case V4L2_COLORFX_SKIN_WHITEN:
		ret = sensor_write_array(sd, sensor_colorfx_skin_whiten_regs, ARRAY_SIZE(sensor_colorfx_skin_whiten_regs));
		break;
	case V4L2_COLORFX_VIVID:
		ret = sensor_write_array(sd, sensor_colorfx_vivid_regs, ARRAY_SIZE(sensor_colorfx_vivid_regs));
		break;
	default:
		return -EINVAL;
	}
	
	if (ret < 0) {
		csi_dev_err("sensor_s_colorfx error, return %x!\n",ret);
		return ret;
	}
//	mdelay(10);
	info->clrfx = value;
	
	return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd,
    __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_flash_mode *flash_mode = (enum v4l2_flash_mode*)value;
	
	*flash_mode = info->flash_mode;
	return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
    enum v4l2_flash_mode value)
{
	struct sensor_info *info = to_state(sd);
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	int flash_on,flash_off;
	
	flash_on = (dev->flash_pol!=0)?1:0;
	flash_off = (flash_on==1)?0:1;
	
	switch (value) {
	case V4L2_FLASH_MODE_OFF:
		csi_gpio_write(sd,&dev->flash_io,flash_off);
		break;
	case V4L2_FLASH_MODE_AUTO:
		return -EINVAL;
		break;  
	case V4L2_FLASH_MODE_ON:
		csi_gpio_write(sd,&dev->flash_io,flash_on);
		break;   
	case V4L2_FLASH_MODE_TORCH:
		return -EINVAL;
		break;
	case V4L2_FLASH_MODE_RED_EYE:   
		return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	
	info->flash_mode = value;
	return 0;
}

static int sensor_g_autofocus_mode(struct v4l2_subdev *sd,
    __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->af_mode;
	
	return 0;
}

static int sensor_s_autofocus_mode(struct v4l2_subdev *sd,
    enum v4l2_autofocus_mode value)
{
	struct sensor_info *info = to_state(sd);
	int ret;
	
	csi_dev_dbg("sensor_s_autofocus_mode = %d\n",value);
	
	switch(value) {
		case V4L2_AF_FIXED:
			break;
		case V4L2_AF_INFINITY:
			ret = sensor_s_release_af(sd);
			if (ret < 0)
			{
				csi_dev_err("sensor_s_release_af err when sensor_s_autofocus_mode !\n");
				return ret;
			}
			break;
		case V4L2_AF_MACRO:
		case V4L2_AF_AUTO:
		case V4L2_AF_TOUCH:
		case V4L2_AF_FACE:
//			ret = sensor_s_continueous_af(sd);
//			if (ret < 0)
//			{
//				csi_dev_err("sensor_s_continueous_af err when sensor_s_autofocus_mode!\n");
//				return ret;
//			}
			break;
	}
	
	info->af_mode = value;
	
	return 0;
}

static int sensor_g_autofocus_ctrl(struct v4l2_subdev *sd,
		struct v4l2_control *ctrl)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_autofocus_ctrl af_ctrl = ctrl->value;
	
	switch(af_ctrl) {
		case V4L2_AF_INIT:
			return sensor_g_single_af(sd);
			break;
		case V4L2_AF_RELEASE:
			break;
		case V4L2_AF_TRIG_SINGLE:
			return sensor_g_single_af(sd);
		case V4L2_AF_TRIG_CONTINUEOUS:
			break;
		case V4L2_AF_LOCK:
			break;
		case V4L2_AF_WIN_XY:
			break;
		case V4L2_AF_WIN_NUM:
			break;
	}
	
	ctrl->value = info->af_ctrl;
	return 0;	
}

static int sensor_s_autofocus_ctrl(struct v4l2_subdev *sd,
		struct v4l2_control *ctrl)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_autofocus_ctrl af_ctrl = ctrl->value;
	struct v4l2_pix_size *pix;
		
	csi_dev_dbg("sensor_s_autofocus_ctrl = %d\n",af_ctrl);
	
	switch(af_ctrl) {
		case V4L2_AF_INIT:
			if(info->af_first_flag == 1) {
				csi_dev_print("af first flag true\n");
				csi_dev_print("sensor_download_af_fw start\n");
				info->af_first_flag = 0;
				return sensor_download_af_fw(sd);
			} else {
				csi_dev_print("af first flag false\n");
				return 0;
			}
		case V4L2_AF_RELEASE:
			return sensor_s_release_af(sd);
		case V4L2_AF_TRIG_SINGLE:
			return sensor_s_single_af(sd);
		case V4L2_AF_TRIG_CONTINUEOUS:
			return sensor_s_continueous_af(sd);
		case V4L2_AF_LOCK:
			return sensor_s_pause_af(sd);
		case V4L2_AF_WIN_XY:
			pix = (struct v4l2_pix_size*)ctrl->user_pt;			
			return sensor_s_af_zone(sd,pix->width,pix->height);
			break;
		case V4L2_AF_WIN_NUM:
			break;
	}
	
	info->af_ctrl = ctrl->value;
	return 0;
}


/*
 * Stuff that knows about the sensor.
 */
 
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	struct sensor_info *info = to_state(sd);
  
  //insure that clk_disable() and clk_enable() are called in pair 
  //when calling CSI_SUBDEV_STBY_ON/OFF and CSI_SUBDEV_PWR_ON/OFF
  ret = 0;
  switch(on)
	{
		case CSI_SUBDEV_STBY_ON:
			csi_dev_dbg("CSI_SUBDEV_STBY_ON!\n");
//			//initial sensor
//			ret = sensor_write_array(sd, sensor_default_regs , ARRAY_SIZE(sensor_default_regs));
//			if(ret < 0)
//				csi_dev_err("initial sensor error when standby on!\n");
			//disable io oe
			csi_dev_print("disalbe oe!\n");
			ret = sensor_write_array(sd, sensor_oe_disable_regs , ARRAY_SIZE(sensor_oe_disable_regs));
			if(ret < 0)
				csi_dev_err("disalbe oe falied!\n");
			//make sure that no device can access i2c bus during sensor initial or power down
			//when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
			i2c_lock_adapter(client->adapter);
			//standby on io
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_ON);
			//remember to unlock i2c adapter, so the device can access the i2c bus again
			i2c_unlock_adapter(client->adapter);	
			//inactive mclk after stadby in
			clk_disable(dev->csi_module_clk);
			//reset on io
//			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
//			mdelay(10);
			break;
		case CSI_SUBDEV_STBY_OFF:
			csi_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
			//make sure that no device can access i2c bus during sensor initial or power down
			//when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
			i2c_lock_adapter(client->adapter);		
			//active mclk before stadby out
			clk_enable(dev->csi_module_clk);
			mdelay(10);
			//standby off io
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_OFF);
			mdelay(10);
			//remember to unlock i2c adapter, so the device can access the i2c bus again
			i2c_unlock_adapter(client->adapter);	
			
//			csi_dev_print("enable oe!\n");
//			ret = sensor_write_array(sd, sensor_oe_enable_regs , ARRAY_SIZE(sensor_oe_enable_regs));
//			if(ret < 0)
//				csi_dev_err("enable oe falied!\n");
				
//			//reset off io
//			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
//			mdelay(10);
			break;
		case CSI_SUBDEV_PWR_ON:
			csi_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
			info->af_first_flag = 1;
			info->init_first_flag=1;
			//make sure that no device can access i2c bus during sensor initial or power down
			//when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
			i2c_lock_adapter(client->adapter);
			//power on reset
			csi_gpio_set_status(sd,&dev->standby_io,1);//set the gpio to output
			csi_gpio_set_status(sd,&dev->reset_io,1);//set the gpio to output
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_ON);
			//reset on io
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
			mdelay(1);
			//active mclk before power on
			clk_enable(dev->csi_module_clk);
			mdelay(10);
			//power supply
			csi_gpio_write(sd,&dev->power_io,CSI_PWR_ON);		
			if(dev->iovdd) {
				regulator_enable(dev->iovdd);
			}
			if(dev->avdd) {
				regulator_enable(dev->avdd);
			}
			if(dev->dvdd) {
				regulator_enable(dev->dvdd);
			}
			csi_gpio_write(sd,&dev->af_power_io,CSI_AF_PWR_ON);
			mdelay(10);
			
			//standby off io
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_OFF);
			mdelay(10);
			//reset after power on
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(30);

			//remember to unlock i2c adapter, so the device can access the i2c bus again
			i2c_unlock_adapter(client->adapter);	
			break;
		case CSI_SUBDEV_PWR_OFF:
			csi_dev_dbg("CSI_SUBDEV_PWR_OFF!\n");
			//make sure that no device can access i2c bus during sensor initial or power down
			//when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
			i2c_lock_adapter(client->adapter);
			//inactive mclk before power off
			clk_disable(dev->csi_module_clk);
			//power supply off
			csi_gpio_write(sd,&dev->power_io,CSI_PWR_OFF);
			csi_gpio_write(sd,&dev->af_power_io,CSI_AF_PWR_OFF);
			if(dev->dvdd) {
				regulator_disable(dev->dvdd);
			}
			if(dev->avdd) {
				regulator_disable(dev->avdd);
			}
			if(dev->iovdd) {
				regulator_disable(dev->iovdd);
			}
	
			//standby and reset io
			mdelay(10);
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_OFF);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);

			//set the io to hi-z
			csi_gpio_set_status(sd,&dev->reset_io,0);//set the gpio to input
			csi_gpio_set_status(sd,&dev->standby_io,0);//set the gpio to input
			//remember to unlock i2c adapter, so the device can access the i2c bus again
			i2c_unlock_adapter(client->adapter);	
			break;
		default:
			return -EINVAL;
	}		

	return 0;
}
 
static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);

	switch(val)
	{
		case CSI_SUBDEV_RST_OFF:
			csi_dev_dbg("CSI_SUBDEV_RST_OFF\n");
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(10);
			break;
		case CSI_SUBDEV_RST_ON:
			csi_dev_dbg("CSI_SUBDEV_RST_ON\n");
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
			mdelay(10);
			break;
		case CSI_SUBDEV_RST_PUL:
			csi_dev_dbg("CSI_SUBDEV_RST_PUL\n");
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(10);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
			mdelay(30);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(30);
			break;
		default:
			return -EINVAL;
	}
		
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret;
	unsigned char rdval;
	
	ret = sensor_read_im(sd, 0x300a, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_detect!\n");
		return ret;
	}
	
	if(rdval != 0x56)
		return -ENODEV;
	
	ret = sensor_read_im(sd, 0x300b, &rdval);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_detect!\n");
		return ret;
	}
	
	if(rdval != 0x40)
		return -ENODEV;
	
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	csi_dev_dbg("sensor_init\n");
	
	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		csi_dev_err("chip found is not an target chip.\n");
		return ret;
	}
	
	if(info->init_first_flag == 0) {
		csi_dev_print("init_first_flag = 0\n");
		return 0;
	} else {
		csi_dev_print("init_first_flag = 1\n");
	}
	
	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 0;
	info->height = 0;
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp = 0;
	info->autoexp = 1;
	info->autowb = 1;
	info->wb = V4L2_WB_AUTO;
	info->clrfx = V4L2_COLORFX_NONE;
	info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
	info->af_mode = V4L2_AF_FIXED;
	info->af_ctrl = V4L2_AF_RELEASE;
	info->tpf.numerator = 1;            
	info->tpf.denominator = 30;    /* 30fps */    
	
	ret = sensor_write_array(sd, sensor_default_regs , ARRAY_SIZE(sensor_default_regs));	
	if(ret < 0) {
		csi_dev_err("write sensor_default_regs error\n");
		return ret;
	}
		
	sensor_s_band_filter(sd, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);
	info->init_first_flag = 0;	
	info->preview_first_flag = 1;
	
	INIT_DELAYED_WORK(&sensor_s_ae_ratio_work, sensor_s_ae_ratio);
	
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret=0;
	
	switch(cmd){
		case CSI_SUBDEV_CMD_GET_INFO: 
		{
			struct sensor_info *info = to_state(sd);
			__csi_subdev_info_t *ccm_info = arg;
			
			csi_dev_dbg("CSI_SUBDEV_CMD_GET_INFO\n");
			
			ccm_info->mclk 	=	info->ccm_info->mclk ;
			ccm_info->vref 	=	info->ccm_info->vref ;
			ccm_info->href 	=	info->ccm_info->href ;
			ccm_info->clock	=	info->ccm_info->clock;
			ccm_info->iocfg	=	info->ccm_info->iocfg;
	
			csi_dev_dbg("ccm_info.mclk=%d\n ",info->ccm_info->mclk);
			csi_dev_dbg("ccm_info.vref=%x\n ",info->ccm_info->vref);
			csi_dev_dbg("ccm_info.href=%x\n ",info->ccm_info->href);
			csi_dev_dbg("ccm_info.clock=%x\n ",info->ccm_info->clock);
			csi_dev_dbg("ccm_info.iocfg=%x\n ",info->ccm_info->iocfg);
			break;
		}
		case CSI_SUBDEV_CMD_SET_INFO:
		{
			struct sensor_info *info = to_state(sd);
			__csi_subdev_info_t *ccm_info = arg;
			
			csi_dev_dbg("CSI_SUBDEV_CMD_SET_INFO\n");
			
			info->ccm_info->mclk 	=	ccm_info->mclk 	;
			info->ccm_info->vref 	=	ccm_info->vref 	;
			info->ccm_info->href 	=	ccm_info->href 	;
			info->ccm_info->clock	=	ccm_info->clock	;
			info->ccm_info->iocfg	=	ccm_info->iocfg	;
			info->ccm_info->stby_mode	=	0 ;
			
			csi_dev_dbg("ccm_info.mclk=%d\n ",info->ccm_info->mclk);
			csi_dev_dbg("ccm_info.vref=%x\n ",info->ccm_info->vref);
			csi_dev_dbg("ccm_info.href=%x\n ",info->ccm_info->href);
			csi_dev_dbg("ccm_info.clock=%x\n ",info->ccm_info->clock);
			csi_dev_dbg("ccm_info.iocfg=%x\n ",info->ccm_info->iocfg);
			csi_dev_dbg("ccm_info.stby_mode=%x\n ",info->ccm_info->stby_mode);
			break;
		}
		case CSI_SUBDEV_CMD_DETECT:
		{
			ret=sensor_detect(sd);
			break;
		}
		default:
			return -EINVAL;
	}		
		return ret;
}


/*
 * Store information about the video data format. 
 */
static struct sensor_format_struct {
	__u8 *desc;
	//__u32 pixelformat;
	enum v4l2_mbus_pixelcode mbus_code;//linux-3.0
	struct regval_list *regs;
	int	regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YVYU8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_UYVY8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_VYUY8_2X8,//linux-3.0
		.regs 		= sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp		= 2,
	},
//	{
//		.desc		= "Raw RGB Bayer",
//		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,//linux-3.0
//		.regs 		= sensor_fmt_raw,
//		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
//		.bpp		= 1
//	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

	

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size {
	int	width;
	int	height;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
	int regs_size;
	int (*set_size) (struct v4l2_subdev *sd);
/* h/vref stuff */
} sensor_win_sizes[] = {
	/* qsxga: 2592*1936 */
	{
		.width			= QSXGA_WIDTH,
		.height 		= QSXGA_HEIGHT,
		.regs			  = sensor_qsxga_regs,
		.regs_size	= ARRAY_SIZE(sensor_qsxga_regs),
		.set_size		= NULL,
	},
	/* qxga: 2048*1536 */
	{
		.width			= QXGA_WIDTH,
		.height 		= QXGA_HEIGHT,
		.regs			  = sensor_qxga_regs,
		.regs_size	= ARRAY_SIZE(sensor_qxga_regs),
		.set_size		= NULL,
	},
	/* 1080P */
	{
		.width			= HD1080_WIDTH,
		.height			= HD1080_HEIGHT,
		.regs 			= sensor_1080p_regs,
		.regs_size	= ARRAY_SIZE(sensor_1080p_regs),
		.set_size		= NULL,
	},
	/* UXGA */
	{
		.width			= UXGA_WIDTH,
		.height			= UXGA_HEIGHT,
		.regs 			= sensor_uxga_regs,
		.regs_size	= ARRAY_SIZE(sensor_uxga_regs),
		.set_size		= NULL,
	},
	/* SXGA */
	{
		.width			= SXGA_WIDTH,
		.height 		= SXGA_HEIGHT,
		.regs			  = sensor_sxga_regs,
		.regs_size	= ARRAY_SIZE(sensor_sxga_regs),
		.set_size		= NULL,
	},
	/* 720p */
	{
		.width			= HD720_WIDTH,
		.height			= HD720_HEIGHT,
		.regs 			= sensor_720p_regs,
		.regs_size	= ARRAY_SIZE(sensor_720p_regs),
		.set_size		= NULL,
	},
	/* XGA */
	{
		.width			= XGA_WIDTH,
		.height 		= XGA_HEIGHT,
		.regs			  = sensor_xga_regs,
		.regs_size	= ARRAY_SIZE(sensor_xga_regs),
		.set_size		= NULL,
	},
	/* SVGA */
	{
		.width			= SVGA_WIDTH,
		.height			= SVGA_HEIGHT,
		.regs				= sensor_svga_regs,
		.regs_size	= ARRAY_SIZE(sensor_svga_regs),
		.set_size		= NULL,
	},
	/* VGA */
	{
		.width			= VGA_WIDTH,
		.height			= VGA_HEIGHT,
		.regs				= sensor_vga_regs,
		.regs_size	= ARRAY_SIZE(sensor_vga_regs),
		.set_size		= NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
                 enum v4l2_mbus_pixelcode *code)//linux-3.0
{
//	struct sensor_format_struct *ofmt;

	if (index >= N_FMTS)//linux-3.0
		return -EINVAL;

	*code = sensor_formats[index].mbus_code;//linux-3.0
//	ofmt = sensor_formats + fmt->index;
//	fmt->flags = 0;
//	strcpy(fmt->description, ofmt->desc);
//	fmt->pixelformat = ofmt->pixelformat;
	return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
		//struct v4l2_format *fmt,
		struct v4l2_mbus_framefmt *fmt,//linux-3.0
		struct sensor_format_struct **ret_fmt,
		struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
//	struct v4l2_pix_format *pix = &fmt->fmt.pix;//linux-3.0

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)//linux-3.0
			break;

	if (index >= N_FMTS) {
		/* default to first format */
		index = 0;
		fmt->code = sensor_formats[0].mbus_code;//linux-3.0
	}
	
	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;
		
	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;//linux-3.0
	
	
	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)//linux-3.0
			break;
		
	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;//linux-3.0
	fmt->height = wsize->height;//linux-3.0
	//pix->bytesperline = pix->width*sensor_formats[index].bpp;//linux-3.0
	//pix->sizeimage = pix->height*pix->bytesperline;//linux-3.0
	
	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd, 
             struct v4l2_mbus_framefmt *fmt)//linux-3.0
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
			     struct v4l2_mbus_config *cfg)
{
	return 0;
}

/*
 * Set fps
 */

static int sensor_s_fps(struct v4l2_subdev *sd)
{
	//struct v4l2_streamparm parms;
	//struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct v4l2_fract *tpf = &cp->timeperframe;
	struct sensor_info *info = to_state(sd);
	unsigned char div,sys_div;
	unsigned char band_50_high,band_50_low,band_60_high,band_60_low;
	unsigned char band_50_step,band_60_step,vts_high,vts_low;
	int band_50,band_60,vts;
	int ret;
	struct regval_list regs_fr[] = {
		{{0x30,0x35},{0xee}},
		{{0x3a,0x08},{0xee}},//50HZ step MSB 
		{{0x3a,0x09},{0xee}},//50HZ step LSB 
		{{0x3a,0x0a},{0xee}},//60HZ step MSB 
		{{0x3a,0x0b},{0xee}},//60HZ step LSB 
		{{0x3a,0x0e},{0xee}},//50HZ step max 
		{{0x3a,0x0d},{0xee}},//60HZ step max 
	};

	csi_dev_dbg("sensor_s_fps\n");
	
	if (info->tpf.numerator == 0)
		return -EINVAL;
		
	div = info->tpf.numerator;
	
//	//power down
//	ret = sensor_write_im(sd, 0x3008, 0x42);
//	if(ret<0) {
//		csi_dev_err("power down error at sensor_s_parm!\n");
//		return ret;
//	}
	
	ret = sensor_read_im(sd, 0x3035, &sys_div);
	if(ret<0) {
		csi_dev_err("sensor_read error at sensor_s_parm!\n");
		return ret;
	}
	
	ret = sensor_read_im(sd, 0x3a08, &band_50_high);
	if(ret<0) {
		csi_dev_err("sensor_read error at sensor_s_parm!\n");
		return ret;
	}

	ret = sensor_read_im(sd, 0x3a09, &band_50_low);
	if(ret<0) {
		csi_dev_err("sensor_read error at sensor_s_parm!\n");
		return ret;
	}
	
	band_50 = band_50_high*256+band_50_low;
	
	ret = sensor_read_im(sd, 0x3a0a, &band_60_high);
	if(ret<0) {
		csi_dev_err("sensor_read error at sensor_s_parm!\n");
		return ret;
	}

	ret = sensor_read_im(sd, 0x3a0b, &band_60_low);
	if(ret<0) {
		csi_dev_err("sensor_read error at sensor_s_parm!\n");
		return ret;
	}
		
	band_60 = band_60_high*256+band_60_low;
	
	ret = sensor_read_im(sd, 0x380e, &vts_high);
	if(ret<0) {
		csi_dev_err("sensor_read error at sensor_s_parm!\n");
		return ret;
	}
	
	ret = sensor_read_im(sd, 0x380f, &vts_low);
	if(ret<0) {
		csi_dev_err("sensor_read error at sensor_s_parm!\n");
		return ret;
	}
	
	vts = vts_high*256+vts_low;
	
	csi_dev_dbg("sys_div=%x,band50=%x,band_60=%x\n",sys_div,band_50,band_60);
	
	sys_div = (sys_div & 0x0f) | ((sys_div & 0xf0)*div);
	band_50 = band_50/div;
	band_60 = band_60/div;
	band_50_step = vts/band_50;
	band_60_step = vts/band_60;
	
	csi_dev_dbg("sys_div=%x,band50=%x,band_60=%x,band_50_step=%x,band_60_step=%x\n",sys_div,band_50,band_60,band_50_step,band_60_step);
	
	regs_fr[0].value[0] = sys_div;
	regs_fr[1].value[0] = (band_50&0xff00)>>8;
	regs_fr[2].value[0] = (band_50&0x00ff)>>0;
	regs_fr[3].value[0] = (band_60&0xff00)>>8;
	regs_fr[4].value[0] = (band_60&0x00ff)>>0;
	regs_fr[5].value[0] = band_50_step;
	regs_fr[6].value[0] = band_60_step;
	
	ret = sensor_write_array(sd, regs_fr, ARRAY_SIZE(regs_fr));
	if(ret<0) {
		csi_dev_err("sensor_write_array at sensor_s_parm!\n");
		return ret;
	}
	
#if DEV_DBG_EN == 1	
	{
		int i;	
		for(i=0;i<7;i++) {
			sensor_read(sd,regs_fr[i].reg_num,regs_fr[i].value);
			csi_dev_print("address 0x%2x%2x = %4x",regs_fr[i].reg_num[0],regs_fr[i].reg_num[1],regs_fr[i].value[0]);
		}
	}
#endif
	
//	//release power down
//	ret = sensor_write_im(sd, 0x3008, 0x02);
//	if(ret<0) {
//		csi_dev_err("release power down error at sensor_s_parm!\n");
//		return ret;
//	}
	
	//msleep(500);
	csi_dev_dbg("set frame rate %d\n",info->tpf.denominator/info->tpf.numerator);
	
	return 0;
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd, 
             struct v4l2_mbus_framefmt *fmt)//linux-3.0
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	
	csi_dev_dbg("sensor_s_fmt\n");
	
	sensor_write_array(sd, sensor_oe_disable_regs , ARRAY_SIZE(sensor_oe_disable_regs));
	
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;
		
	if(info->capture_mode == V4L2_MODE_VIDEO)
	{
		//video
#if 0	
		if(info->af_mode != V4L2_AF_FIXED) {
			ret = sensor_s_release_af(sd);
			if (ret < 0)
				csi_dev_err("sensor_s_release_af err !\n");
		}
#endif      
	}
	else if(info->capture_mode == V4L2_MODE_IMAGE)
	{
		//image	
		ret = sensor_s_autoexp(sd,V4L2_EXPOSURE_MANUAL);
		if (ret < 0)
			csi_dev_err("sensor_s_autoexp off err when capturing image!\n");
		
		ret = sensor_s_autogain(sd,0);
		if (ret < 0)
			csi_dev_err("sensor_s_autogain off err when capturing image!\n");
		
		if (wsize->width > SVGA_WIDTH) {
			sensor_get_preview_exposure(sd);
			sensor_get_fps(sd);
			ret = sensor_set_capture_exposure(sd);
			if (ret < 0)
				csi_dev_err("sensor_set_capture_exposure err !\n");
		}
		
		ret = sensor_s_autowb(sd,0); //lock wb
		if (ret < 0)
			csi_dev_err("sensor_s_autowb off err when capturing image!\n");
	}
	

#if 0	
	if(info->low_speed == 1) {
		//power down
		csi_dev_print("power down\n");
		ret = sensor_write_im(sd, 0x3008, 0x42);
		if(ret<0) {
			csi_dev_err("power down error at sensor_s_parm!\n");
			return ret;
		}
	}
#endif	
	
	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);
	
	ret = 0;
	if (wsize->regs)
	{
		ret = sensor_write_array(sd, wsize->regs , wsize->regs_size);
		if (ret < 0)
			return ret;
	}
	
	if (wsize->set_size)
	{
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}
	
	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	
	csi_dev_print("s_fmt set width = %d, height = %d\n",wsize->width,wsize->height);

#if 0	
	if(info->low_speed == 1) {
		//release power down
		csi_dev_print("release power down\n");
		ret = sensor_write_im(sd, 0x3008, 0x02);
		if(ret<0) {
			csi_dev_err("release power down error at sensor_s_parm!\n");
			return ret;
		}
	}
#endif	

	if(info->capture_mode == V4L2_MODE_VIDEO || info->capture_mode == V4L2_MODE_PREVIEW)
	{
		//video
    	sensor_s_fps(sd);
    	
#ifdef AUTO_FPS		
		if(info->capture_mode == V4L2_MODE_PREVIEW) {
	      sensor_write_array(sd, sensor_auto_fps_mode , ARRAY_SIZE(sensor_auto_fps_mode));
	    } else {
	      sensor_write_array(sd, sensor_fix_fps_mode , ARRAY_SIZE(sensor_fix_fps_mode));
	    }
#endif    	
    
		ret = sensor_set_preview_exposure(sd);
		if (ret < 0)
			csi_dev_err("sensor_set_preview_exposure err !\n");
			
		ret = sensor_s_autoexp(sd,V4L2_EXPOSURE_AUTO);
		if (ret < 0)
			csi_dev_err("sensor_s_autoexp on err when capturing video!\n");
		
		ret = sensor_s_autogain(sd,1);
		if (ret < 0)
			csi_dev_err("sensor_s_autogain on err when capturing video!\n");		
		
		if (info->wb == V4L2_WB_AUTO) {
			ret = sensor_s_autowb(sd,1); //unlock wb
			if (ret < 0)
				csi_dev_err("sensor_s_autowb on err when capturing image!\n");
		}
		
		msleep(100);
		
		ret = sensor_s_relaunch_af_zone(sd);
		if (ret < 0) {
			csi_dev_err("sensor_s_relaunch_af_zone err !\n");
			return ret;
		}
		
		ret = sensor_write_im(sd, 0x3022, 0x03);		//sensor_s_single_af
		if (ret < 0) {
			csi_dev_err("sensor_s_single_af err !\n");
			return ret;
		}
		
		if(info->af_mode != V4L2_AF_FIXED) {

#if 0
			if(info->af_mode != V4L2_AF_TOUCH && info->af_mode != V4L2_AF_FACE) {				
				ret = sensor_s_relaunch_af_zone(sd);	//set af zone to default zone
				if (ret < 0) {
					csi_dev_err("sensor_s_relaunch_af_zone err !\n");
					return ret;
				}	
			}
#endif

#ifdef CONTINUEOUS_AF			
			if(info->af_mode != V4L2_AF_INFINITY) {
				ret = sensor_s_continueous_af(sd);		//set continueous af
				if (ret < 0) {
					csi_dev_err("sensor_s_continueous_af err !\n");
					return ret;
				}
			}
#endif
		}
    
    if(info->capture_mode == V4L2_MODE_VIDEO) {
  		sensor_s_sharpness_auto(sd); //sharpness auto
  		sensor_s_denoise_auto(sd);
  	} else if(info->capture_mode == V4L2_MODE_PREVIEW) {
  	  sensor_s_sharpness_value(sd,SHARPNESS); //sharpness fix value
  	  sensor_s_denoise_value(sd,8);
  	}
    
		  
		if(info->low_speed == 1) {
			if(info->preview_first_flag == 1) {
				info->preview_first_flag = 0;
				msleep(600);
			} else {
				msleep(200);
			}		
		}
		msleep(200);
	} else {
		//capture image
		sensor_s_sharpness_value(sd,SHARPNESS); //sharpness 0x0
		//sensor_s_sharpness_auto(sd); //sharpness auto
    
		if(info->low_speed == 1) {
			sensor_read_im(sd,0x3035,&rdval);
			sensor_write_im(sd,0x3035,(rdval&0x0f)|((rdval&0xf0)*2));
			//sensor_write_im(sd,0x3037,0x14);
		}
		
		msleep(200);
	}
	

#if DEV_DBG_EN == 1	
	{
		int i;
		struct regval_list dbg_regs[] = {
			{{0x30,0x34},{0xee}},
			{{0x30,0x35},{0xee}},
			{{0x30,0x36},{0xee}},
			{{0x30,0x37},{0xee}},
			{{0x31,0x08},{0xee}},
			{{0x38,0x24},{0xee}},
		};
		for(i=0;i<6;i++) {
			sensor_read(sd,dbg_regs[i].reg_num,dbg_regs[i].value);
			csi_dev_print("address 0x%2x%2x = %4x",dbg_regs[i].reg_num[0],dbg_regs[i].reg_num[1],dbg_regs[i].value[0]);
		}
	}
#endif
  sensor_write_array(sd, sensor_oe_enable_regs , ARRAY_SIZE(sensor_oe_enable_regs));
	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	
	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;
	
	cp->timeperframe.numerator = info->tpf.numerator;
	cp->timeperframe.denominator = info->tpf.denominator;
	 
	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;
	struct sensor_info *info = to_state(sd);
	unsigned char div;
	
	csi_dev_dbg("sensor_s_parm\n");
	
	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
	  {
	  csi_dev_dbg("sensor_s_parm not correct type\n");
	
		return -EINVAL;
	}
	
	if (info->tpf.numerator == 0)
	  {
	  csi_dev_dbg("sensor_s_parm tpf.numerator == 0\n");
	
		return -EINVAL;
	}
	info->capture_mode = cp->capturemode;
	
	if (info->capture_mode == V4L2_MODE_IMAGE) {
		csi_dev_dbg("capture mode is not video mode,can not set frame rate!\n");
		return 0;
	}
		
	if (tpf->numerator == 0 || tpf->denominator == 0)	{
		tpf->numerator = 1;
		tpf->denominator = SENSOR_FRAME_RATE;/* Reset to full rate */
		csi_dev_err("sensor frame rate reset to full rate!\n");
	}
	
	div = SENSOR_FRAME_RATE/(tpf->denominator/tpf->numerator);
	if(div > 15 || div == 0) 
	  {
	  csi_dev_dbg("div=%d\n",div);  
		return -EINVAL;
	}
	
	csi_dev_dbg("set frame rate %d\n",tpf->denominator/tpf->numerator);
	
	info->tpf.denominator = SENSOR_FRAME_RATE; 
	info->tpf.numerator = div;
	
	if(info->tpf.denominator/info->tpf.numerator < 30)
		info->low_speed = 1;
		
	return 0;
}


static int sensor_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */
	switch (qc->id) {
//	case V4L2_CID_BRIGHTNESS:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_CONTRAST:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_SATURATION:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_HUE:
//		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
//	case V4L2_CID_GAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
//	case V4L2_CID_AUTOGAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_DO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 5, 1, 0);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 0);
	case V4L2_CID_CAMERA_FLASH_MODE:
	  return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);	
	case V4L2_CID_CAMERA_AF_MODE:
	{
		int ret = 0;
		csi_dev_dbg("V4L2_CID_CAMERA_AF_MODE\n");
		ret = v4l2_ctrl_query_fill(qc, 0, 5, 1, 0);
	  return ret;
	}
	case V4L2_CID_CAMERA_AF_CTRL:
	{
		int ret = 0;
		csi_dev_dbg("V4L2_CID_CAMERA_AF_CTRL\n");
		ret = v4l2_ctrl_query_fill(qc, 0, 6, 1, 0);
	  return ret;
	}
	}
	csi_dev_dbg("no id\n");
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);	
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_g_autoexp(sd, &ctrl->value);
	case V4L2_CID_DO_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd,	&ctrl->value);
	case V4L2_CID_CAMERA_FLASH_MODE:
		return sensor_g_flash_mode(sd, &ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return sensor_g_band_filter(sd, &ctrl->value);
	case V4L2_CID_CAMERA_AF_MODE:
		return sensor_g_autofocus_mode(sd, &ctrl->value);
	case V4L2_CID_CAMERA_AF_CTRL:
		return sensor_g_autofocus_ctrl(sd, ctrl);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
  
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);		
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexp(sd,
				(enum v4l2_exposure_auto_type) ctrl->value);
	case V4L2_CID_DO_WHITE_BALANCE:
		return sensor_s_wb(sd,
				(enum v4l2_whiteblance) ctrl->value);	
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd,
				(enum v4l2_colorfx) ctrl->value);
	case V4L2_CID_CAMERA_FLASH_MODE:
	  return sensor_s_flash_mode(sd,
	      (enum v4l2_flash_mode) ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return sensor_s_band_filter(sd,
	      (enum v4l2_power_line_frequency) ctrl->value);
	case V4L2_CID_CAMERA_AF_MODE:
		return sensor_s_autofocus_mode(sd,
	      (enum v4l2_autofocus_mode) ctrl->value);
	case V4L2_CID_CAMERA_AF_CTRL:
		return sensor_s_autofocus_ctrl(sd, ctrl);
	}
	return -EINVAL;
}


static int sensor_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}


/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.enum_mbus_fmt = sensor_enum_fmt,//linux-3.0
	.try_mbus_fmt = sensor_try_fmt,//linux-3.0
	.s_mbus_fmt = sensor_s_fmt,//linux-3.0
	.s_parm = sensor_s_parm,//linux-3.0
	.g_parm = sensor_g_parm,//linux-3.0
	.g_mbus_config = sensor_g_mbus_config,//linux-3.3
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
};

/* ----------------------------------------------------------------------- */

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
//	int ret;
  
  csi_dev_dbg("sensor_probe");
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	glb_sd = sd;
	v4l2_i2c_subdev_init(sd, client, &sensor_ops);

	info->fmt = &sensor_formats[0];
	info->ccm_info = &ccm_info_con;
	info->af_first_flag = 1;
	info->init_first_flag = 1;

	return 0;
}


static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ "ov5640", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

//linux-3.0
static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
	.name = "ov5640",
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
	return i2c_add_driver(&sensor_driver);
}

static __exit void exit_sensor(void)
{
  i2c_del_driver(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);

