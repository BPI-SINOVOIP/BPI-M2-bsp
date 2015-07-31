/*
 * A V4L2 driver for GalaxyCore mt9p111 cameras.
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
MODULE_DESCRIPTION("A low-level driver for GalaxyCore mt9p111 sensors");
MODULE_LICENSE("GPL");

//for internel driver debug
#define DEV_DBG_EN   		1 
#if(DEV_DBG_EN == 1)		
#define csi_dev_dbg(x,arg...) printk(KERN_INFO"[CSI_DEBUG][MT9P111]"x,##arg)
#else
#define csi_dev_dbg(x,arg...) 
#endif
#define csi_dev_err(x,arg...) printk(KERN_INFO"[CSI_ERR][MT9P111]"x,##arg)
#define csi_dev_print(x,arg...) printk(KERN_INFO"[CSI][MT9P111]"x,##arg)

#define MCLK (24*1000*1000)
#define VREF_POL	CSI_HIGH
#define HREF_POL	CSI_HIGH
#define CLK_POL		CSI_RISING
#define IO_CFG		0						//0 for csi0
#define V4L2_IDENT_SENSOR 0x111

//define the voltage level of control signal
#define CSI_STBY_ON			1
#define CSI_STBY_OFF 		0
#define CSI_RST_ON			0
#define CSI_RST_OFF			1
#define CSI_PWR_ON			1
#define CSI_PWR_OFF			0

#define REG_TERM 0xff
#define VAL_TERM 0xff


#define REG_ADDR_STEP 2
#define REG_DATA_STEP 2
#define REG_STEP 			(REG_ADDR_STEP+REG_DATA_STEP)

/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define QSXGA_WIDTH		2592
#define QSXGA_HEIGHT	1936
#define QXGA_WIDTH 		2048
#define QXGA_HEIGHT		1536
#define P1080P_WIDTH	1920
#define P1080P_HEIGHT	1080
#define UXGA_WIDTH		1600
#define UXGA_HEIGHT		1200
#define P720_WIDTH 		1280
#define P720_HEIGHT		720
//SXGA: 1280*960
#define SXGA_WIDTH		1280
#define SXGA_HEIGHT		960
#define HD720_WIDTH 	1280
#define HD720_HEIGHT	720
//XGA: 1024*768
#define XGA_WIDTH		1024
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
#define SENSOR_FRAME_RATE 15

/*
 * The mt9p111 sits on i2c with ID 0x78
 */
#define I2C_ADDR 0x78

/* Registers */


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
	u8 clkrc;			/* Clock divider value */
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
	unsigned char bit;
	unsigned char reg_num[REG_ADDR_STEP];
	unsigned char value[REG_DATA_STEP];
};



static struct regval_list sensor_default_regs[] = {
//[Step1-Reset]

{16,	{0x00,0x1A}, {0x00,0x19}},	//Reset
{16,	{0xff,0xff}, {0x00,0x64}},	//Delay = 20
{16,	{0x00,0x1A}, {0x00,0x18}},
{16,	{0xff,0xff}, {0x00,0x64}},	//Delay = 20

//[Step2-PLL_Timing]
//pll fine tune to 372MHz
{16,	{0x00,0x10}, {0x03,0x1f}},	//PLL Dividers = 800
{16,	{0x00,0x12}, {0x00,0x70}},	//PLL P Dividers = 112
{16,	{0x00,0x14}, {0x20,0x25}},	//PLL Control: TEST_BYPASS off = 8229
{16,	{0x00,0x1E}, {0x07,0x77}},	//Pad Slew Pad Config = 1911
{16,	{0x00,0x22}, {0x01,0xE0}},	//VDD_DIS counter delay
{16,	{0x00,0x2A}, {0x7F,0x59}},	//PLL P Dividers 4-5-6 = 31558
{16,	{0x00,0x2C}, {0x00,0x00}},	//PLL P Dividers 7 = 0
{16,	{0x00,0x2E}, {0x00,0x01}},	//Sensor Clock Divider = 0
{16,	{0x00,0x18}, {0x40,0x08}},	//Standby Control and Status: Out of standby  //read
{16,	{0xff,0xff}, {0x00,0x0a}}, //DELAY=10
{16,	{0x30,0xD4}, {0x90,0x80}},	//Disable Double Samplings

////pll divider
//16,	0x3CA4, 0x0001,
              
{16,	{0x09,0x8E}, {0x10,0x00}},
//Context A
{16,	{0xC8,0x6C}, {0x05,0x18}},	//Output Width (A) = 1304
{16,	{0xC8,0x6E}, {0x03,0xD4}},	//Output Height (A) = 980
{16,	{0xC8,0x3A}, {0x00,0x0C}},	//Row Start (A) = 12
{16,	{0xC8,0x3C}, {0x00,0x18}},	//Column Start (A) = 24
{16,	{0xC8,0x3E}, {0x07,0xB1}},	//Row End (A) = 1969
{16,	{0xC8,0x40}, {0x0A,0x45}},	//Column End (A) = 2629
{16,	{0xC8,0x42}, {0x00,0x01}},	//Row Speed (A) = 1
{16,	{0xC8,0x44}, {0x01,0x03}},	//Core Skip X (A) = 259
{16,	{0xC8,0x46}, {0x01,0x03}},	//Core Skip Y (A) = 259
{16,	{0xC8,0x48}, {0x01,0x03}},	//Pipe Skip X (A) = 259
{16,	{0xC8,0x4A}, {0x01,0x03}},	//Pipe Skip Y (A) = 259
{16,	{0xC8,0x4C}, {0x00,0xF6}},	//Power Mode (A) = 246
{16,	{0xC8,0x4E}, {0x00,0x01}},	//Bin Mode (A) = 1
{8 ,	{0xC8,0x50}, {0x00}},				//Orientation (A) = 0
{8 ,	{0xC8,0x51}, {0x00}},				//Pixel Order (A) = 0
{16,	{0xC8,0x52}, {0x01,0x9C}},	//Fine Correction (A) = 412
{16,	{0xC8,0x54}, {0x07,0x32}},	//Fine IT Min (A) = 1842
{16,	{0xC8,0x56}, {0x04,0x8E}},	//Fine IT Max Margin (A) = 1166
{16,	{0xC8,0x58}, {0x00,0x02}},	//Coarse IT Min (A) = 2
{16,	{0xC8,0x5A}, {0x00,0x01}},	//Coarse IT Max Margin (A) = 1

{16,	{0xC8,0x5C}, {0x04,0x23}},	//Min Frame Lines (A) = 1059
{16,	{0xC8,0x5E}, {0xFF,0xFF}},	//Max Frame Lines (A) = 65535
{16,	{0xC8,0x60}, {0x04,0x23}},	//Base Frame Lines (A) = 1059
{16,	{0xC8,0x62}, {0x0E,0x2A}},	//Min Line Length (A) = 3504
{16,	{0xC8,0x64}, {0xFF,0xFE}},	//Max Line Length (A) = 65534
{16,	{0xC8,0x66}, {0x7F,0x59}},	//P456 Divider (A) = 31558
{16,	{0xC8,0x68}, {0x04,0x23}},	//Frame Lines (A) = 1059
{16,	{0xC8,0x6A}, {0x0E,0x2A}},	//Line Length (A) = 3504
{16,	{0xC8,0x70}, {0x00,0x14}},	//RX FIFO Watermark (A) = 20
{16,	{0xC8,0xAA}, {0x02,0x80}},	//Output_0 Image Width = 640
{16,	{0xC8,0xAC}, {0x01,0xe0}},	//Output_0 Image Height = 480
{16,	{0xC8,0xAE}, {0x00,0x01}},	//Output_0 Image Format = 1
{16,	{0xC8,0xB0}, {0x00,0x00}},	//Output_0 Format Order = 0
{16,	{0xC8,0xB8}, {0x00,0x04}},	//Output_0 JPEG control = 4


//Context B
{16,	{0xC8,0xA4}, {0x0A,0x28}},	//Output Width (B) = 2600
{16,	{0xC8,0xA6}, {0x07,0xA0}},	//Output Height (B) = 1952
{16,	{0xC8,0x72}, {0x00,0x10}},	//Row Start (B) = 16
{16,	{0xC8,0x74}, {0x00,0x1C}},	//Column Start (B) = 28
{16,	{0xC8,0x76}, {0x07,0xAF}},	//Row End (B) = 1967
{16,	{0xC8,0x78}, {0x0A,0x43}},	//Column End (B) = 2627
{16,	{0xC8,0x7A}, {0x00,0x01}},	//Row Speed (B) = 1
{16,	{0xC8,0x7C}, {0x01,0x01}},	//Core Skip X (B) = 257
{16,	{0xC8,0x7E}, {0x01,0x01}},	//Core Skip Y (B) = 257
{16,	{0xC8,0x80}, {0x01,0x01}},	//Pipe Skip X (B) = 257
{16,	{0xC8,0x82}, {0x01,0x01}},	//Pipe Skip Y (B) = 257
{16,	{0xC8,0x84}, {0x00,0xF2}},	//Power Mode (B) = 242
{16,	{0xC8,0x86}, {0x00,0x00}},	//Bin Mode (B) = 0
{8 ,	{0xC8,0x88}, {0x00}},				//Orientation (B) = 0
{8 ,	{0xC8,0x89}, {0x00}},				//Pixel Order (B) = 0
{16,	{0xC8,0x8A}, {0x00,0x9C}},	//Fine Correction (B) = 156  			
{16,	{0xC8,0x8C}, {0x03,0x4A}},	//Fine IT Min (B) = 842						
{16,	{0xC8,0x8E}, {0x02,0xA6}},	//Fine IT Max Margin (B) = 678		
{16,	{0xC8,0x90}, {0x00,0x02}},	//Coarse IT Min (B) = 2						
{16,	{0xC8,0x92}, {0x00,0x01}},	//Coarse IT Max Margin (B) = 1		
{16,	{0xC8,0x94}, {0x07,0xEF}},	//Min Frame Lines (B) = 2031			
{16,	{0xC8,0x96}, {0xFF,0xFF}},	//Max Frame Lines (B) = 65535
{16,	{0xC8,0x98}, {0x07,0xEF}},	//Base Frame Lines (B) = 2031
{16,	{0xC8,0x9A}, {0x2C,0x50}},	//Min Line Length (B) = 14276
{16,	{0xC8,0x9C}, {0xFF,0xFE}},	//Max Line Length (B) = 65534
{16,	{0xC8,0x9E}, {0x7F,0x59}},	//P456 Divider (B) = 31558
{16,	{0xC8,0xA0}, {0x07,0xEF}},	//Frame Lines (B) = 2031
{16,	{0xC8,0xA2}, {0x2C,0x50}},	//Line Length (B) = 14276
{16,	{0xC8,0xA8}, {0x00,0x14}},	//RX FIFO Watermark (B) = 20
{16,	{0xC8,0xC0}, {0x0A,0x20}},	//Output_1 Image Width = 2592
{16,	{0xC8,0xC2}, {0x07,0x98}},	//Output_1 Image Height = 1944
{16,	{0xC8,0xC4}, {0x00,0x01}},	//Output_1 Image Format = 1
{16,	{0xC8,0xC6}, {0x00,0x00}},	//Output_1 Format Order = 0
{16,	{0xC8,0xCE}, {0x00,0x04}},	//Output_1 JPEG control = 4
{16,	{0xA0,0x10}, {0x01,0x34}},	//fd_min_expected50hz_flicker_period = 303
{16,	{0xA0,0x12}, {0x01,0x48}},	//fd_max_expected50hz_flicker_period = 323
{16,	{0xA0,0x14}, {0x00,0xFF}},	//fd_min_expected60hz_flicker_period = 251
{16,	{0xA0,0x16}, {0x01,0x13}},	//fd_max_expected60hz_flicker_period = 271
{16,	{0xA0,0x18}, {0x01,0x3E}},	//fd_expected50hz_flicker_period (A) = 313
{16,	{0xA0,0x1A}, {0x00,0x66}},	//fd_expected50hz_flicker_period (B) = 77
{16,	{0xA0,0x1C}, {0x01,0x09}},	//fd_expected60hz_flicker_period (A) = 261
{16,	{0xA0,0x1E}, {0x00,0x55}},	//fd_expected60hz_flicker_period (B) = 64
{8 ,	{0xDC,0x0A}, {0x06}},				//Scaler Allow Zoom Ratio = 6
{16,	{0xDC,0x1C}, {0x27,0x10}},	//System Zoom Ratio = 10000
{16,	{0xE0,0x04}, {0x05,0xA0}},	//I2C Master Clock Divider = 3840
//
{16,	{0xff,0xff}, {0x00,0x32}},	//Delay = 50


//[Step3-Recommended]

//k28a_rev03_patch01_basic_REV5
{16,	{0x09,0x82}, {0x00,0x00}}, 	// ACCESS_CTL_STAT
{16,	{0x09,0x8A}, {0x00,0x00}}, 	// PHYSICAL_ADDRESS_ACCESS
{16,	{0x88,0x6C}, {0xC0,0xF1}},
{16,	{0x88,0x6E}, {0xC5,0xE1}},
{16,	{0x88,0x70}, {0x24,0x6A}},
{16,	{0x88,0x72}, {0x12,0x80}},
{16,	{0x88,0x74}, {0xC4,0xE1}},
{16,	{0x88,0x76}, {0xD2,0x0F}},
{16,	{0x88,0x78}, {0x20,0x69}},
{16,	{0x88,0x7A}, {0x00,0x00}},
{16,	{0x88,0x7C}, {0x6A,0x62}},
{16,	{0x88,0x7E}, {0x13,0x03}},
{16,	{0x88,0x80}, {0x00,0x84}},
{16,	{0x88,0x82}, {0x17,0x34}},
{16,	{0x88,0x84}, {0x70,0x05}},
{16,	{0x88,0x86}, {0xD8,0x01}},
{16,	{0x88,0x88}, {0x8A,0x41}},
{16,	{0x88,0x8A}, {0xD9,0x00}},
{16,	{0x88,0x8C}, {0x0D,0x5A}},
{16,	{0x88,0x8E}, {0x06,0x64}},
{16,	{0x88,0x90}, {0x8B,0x61}},
{16,	{0x88,0x92}, {0xE8,0x0B}},
{16,	{0x88,0x94}, {0x00,0x0D}},
{16,	{0x88,0x96}, {0x00,0x20}},
{16,	{0x88,0x98}, {0xD5,0x08}},
{16,	{0x88,0x9A}, {0x15,0x04}},
{16,	{0x88,0x9C}, {0x14,0x00}},
{16,	{0x88,0x9E}, {0x78,0x40}},
{16,	{0x88,0xA0}, {0xD0,0x07}},
{16,	{0x88,0xA2}, {0x0D,0xFB}},
{16,	{0x88,0xA4}, {0x90,0x04}},
{16,	{0x88,0xA6}, {0xC4,0xC1}},
{16,	{0x88,0xA8}, {0x20,0x29}},
{16,	{0x88,0xAA}, {0x03,0x00}},
{16,	{0x88,0xAC}, {0x02,0x19}},
{16,	{0x88,0xAE}, {0x06,0xC4}},
{16,	{0x88,0xB0}, {0xFF,0x80}},
{16,	{0x88,0xB2}, {0x08,0xC4}},
{16,	{0x88,0xB4}, {0xFF,0x80}},
{16,	{0x88,0xB6}, {0x08,0x6C}},
{16,	{0x88,0xB8}, {0xFF,0x80}},
{16,	{0x88,0xBA}, {0x08,0xC0}},
{16,	{0x88,0xBC}, {0xFF,0x80}},
{16,	{0x88,0xBE}, {0x08,0xC4}},
{16,	{0x88,0xC0}, {0xFF,0x80}},
{16,	{0x88,0xC2}, {0x09,0x7C}},
{16,	{0x88,0xC4}, {0x00,0x01}},
{16,	{0x88,0xC6}, {0x00,0x05}},
{16,	{0x88,0xC8}, {0x00,0x00}},
{16,	{0x88,0xCA}, {0x00,0x00}},
{16,	{0x88,0xCC}, {0xC0,0xF1}},
{16,	{0x88,0xCE}, {0x09,0x76}},
{16,	{0x88,0xD0}, {0x06,0xC4}},
{16,	{0x88,0xD2}, {0xD6,0x39}},
{16,	{0x88,0xD4}, {0x77,0x08}},
{16,	{0x88,0xD6}, {0x8E,0x01}},
{16,	{0x88,0xD8}, {0x16,0x04}},
{16,	{0x88,0xDA}, {0x10,0x91}},
{16,	{0x88,0xDC}, {0x20,0x46}},
{16,	{0x88,0xDE}, {0x00,0xC1}},
{16,	{0x88,0xE0}, {0x20,0x2F}},
{16,	{0x88,0xE2}, {0x20,0x47}},
{16,	{0x88,0xE4}, {0xAE,0x21}},
{16,	{0x88,0xE6}, {0x0F,0x8F}},
{16,	{0x88,0xE8}, {0x14,0x40}},
{16,	{0x88,0xEA}, {0x8E,0xAA}},
{16,	{0x88,0xEC}, {0x8E,0x0B}},
{16,	{0x88,0xEE}, {0x22,0x4A}},
{16,	{0x88,0xF0}, {0x20,0x40}},
{16,	{0x88,0xF2}, {0x8E,0x2D}},
{16,	{0x88,0xF4}, {0xBD,0x08}},
{16,	{0x88,0xF6}, {0x7D,0x05}},
{16,	{0x88,0xF8}, {0x8E,0x0C}},
{16,	{0x88,0xFA}, {0xB8,0x08}},
{16,	{0x88,0xFC}, {0x78,0x25}},
{16,	{0x88,0xFE}, {0x75,0x10}},
{16,	{0x89,0x00}, {0x22,0xC2}},
{16,	{0x89,0x02}, {0x24,0x8C}},
{16,	{0x89,0x04}, {0x08,0x1D}},
{16,	{0x89,0x06}, {0x03,0x63}},
{16,	{0x89,0x08}, {0xD9,0xFF}},
{16,	{0x89,0x0A}, {0x25,0x02}},
{16,	{0x89,0x0C}, {0x10,0x02}},
{16,	{0x89,0x0E}, {0x2A,0x05}},
{16,	{0x89,0x10}, {0x03,0xFE}},
{16,	{0x89,0x12}, {0x0A,0x16}},
{16,	{0x89,0x14}, {0x06,0xE4}},
{16,	{0x89,0x16}, {0x70,0x2F}},
{16,	{0x89,0x18}, {0x78,0x10}},
{16,	{0x89,0x1A}, {0x7D,0x02}},
{16,	{0x89,0x1C}, {0x7D,0xB0}},
{16,	{0x89,0x1E}, {0xF0,0x0B}},
{16,	{0x89,0x20}, {0x78,0xA2}},
{16,	{0x89,0x22}, {0x28,0x05}},
{16,	{0x89,0x24}, {0x03,0xFE}},
{16,	{0x89,0x26}, {0x0A,0x02}},
{16,	{0x89,0x28}, {0x06,0xE4}},
{16,	{0x89,0x2A}, {0x70,0x2F}},
{16,	{0x89,0x2C}, {0x78,0x10}},
{16,	{0x89,0x2E}, {0x65,0x1D}},
{16,	{0x89,0x30}, {0x7D,0xB0}},
{16,	{0x89,0x32}, {0x7D,0xAF}},
{16,	{0x89,0x34}, {0x8E,0x08}},
{16,	{0x89,0x36}, {0xBD,0x06}},
{16,	{0x89,0x38}, {0xD1,0x20}},
{16,	{0x89,0x3A}, {0xB8,0xC3}},
{16,	{0x89,0x3C}, {0x78,0xA5}},
{16,	{0x89,0x3E}, {0xB8,0x8F}},
{16,	{0x89,0x40}, {0x19,0x08}},
{16,	{0x89,0x42}, {0x00,0x24}},
{16,	{0x89,0x44}, {0x28,0x41}},
{16,	{0x89,0x46}, {0x02,0x01}},
{16,	{0x89,0x48}, {0x1E,0x26}},
{16,	{0x89,0x4A}, {0x10,0x42}},
{16,	{0x89,0x4C}, {0x0F,0x15}},
{16,	{0x89,0x4E}, {0x14,0x63}},
{16,	{0x89,0x50}, {0x1E,0x27}},
{16,	{0x89,0x52}, {0x10,0x02}},
{16,	{0x89,0x54}, {0x22,0x4C}},
{16,	{0x89,0x56}, {0xA0,0x00}},
{16,	{0x89,0x58}, {0x22,0x4A}},
{16,	{0x89,0x5A}, {0x20,0x40}},
{16,	{0x89,0x5C}, {0x22,0xC2}},
{16,	{0x89,0x5E}, {0x24,0x82}},
{16,	{0x89,0x60}, {0x20,0x4F}},
{16,	{0x89,0x62}, {0x20,0x40}},
{16,	{0x89,0x64}, {0x22,0x4C}},
{16,	{0x89,0x66}, {0xA0,0x00}},
{16,	{0x89,0x68}, {0xB8,0xA2}},
{16,	{0x89,0x6A}, {0xF2,0x04}},
{16,	{0x89,0x6C}, {0x20,0x45}},
{16,	{0x89,0x6E}, {0x21,0x80}},
{16,	{0x89,0x70}, {0xAE,0x01}},
{16,	{0x89,0x72}, {0x0D,0x9E}},
{16,	{0x89,0x74}, {0xFF,0xE3}},
{16,	{0x89,0x76}, {0x70,0xE9}},
{16,	{0x89,0x78}, {0x01,0x25}},
{16,	{0x89,0x7A}, {0x06,0xC4}},
{16,	{0x89,0x7C}, {0xC0,0xF1}},
{16,	{0x89,0x7E}, {0xD0,0x10}},
{16,	{0x89,0x80}, {0xD1,0x10}},
{16,	{0x89,0x82}, {0xD2,0x0D}},
{16,	{0x89,0x84}, {0xA0,0x20}},
{16,	{0x89,0x86}, {0x8A,0x00}},
{16,	{0x89,0x88}, {0x08,0x09}},
{16,	{0x89,0x8A}, {0x01,0xDE}},
{16,	{0x89,0x8C}, {0xB8,0xA7}},
{16,	{0x89,0x8E}, {0xAA,0x00}},
{16,	{0x89,0x90}, {0xDB,0xFF}},
{16,	{0x89,0x92}, {0x2B,0x41}},
{16,	{0x89,0x94}, {0x02,0x00}},
{16,	{0x89,0x96}, {0xAA,0x0C}},
{16,	{0x89,0x98}, {0x12,0x28}},
{16,	{0x89,0x9A}, {0x00,0x80}},
{16,	{0x89,0x9C}, {0xAA,0x6D}},
{16,	{0x89,0x9E}, {0x08,0x15}},
{16,	{0x89,0xA0}, {0x01,0xDE}},
{16,	{0x89,0xA2}, {0xB8,0xA7}},
{16,	{0x89,0xA4}, {0x1A,0x28}},
{16,	{0x89,0xA6}, {0x00,0x02}},
{16,	{0x89,0xA8}, {0x81,0x23}},
{16,	{0x89,0xAA}, {0x79,0x60}},
{16,	{0x89,0xAC}, {0x12,0x28}},
{16,	{0x89,0xAE}, {0x00,0x80}},
{16,	{0x89,0xB0}, {0xC0,0xD1}},
{16,	{0x89,0xB2}, {0x7E,0xE0}},
{16,	{0x89,0xB4}, {0xFF,0x80}},
{16,	{0x89,0xB6}, {0x01,0x58}},
{16,	{0x89,0xB8}, {0xFF,0x00}},
{16,	{0x89,0xBA}, {0x06,0x18}},
{16,	{0x89,0xBC}, {0x80,0x00}},
{16,	{0x89,0xBE}, {0x00,0x08}},
{16,	{0x89,0xC0}, {0xFF,0x80}},
{16,	{0x89,0xC2}, {0x0A,0x08}},
{16,	{0x89,0xC4}, {0xE2,0x80}},
{16,	{0x89,0xC6}, {0x24,0xCA}},
{16,	{0x89,0xC8}, {0x70,0x82}},
{16,	{0x89,0xCA}, {0x78,0xE0}},
{16,	{0x89,0xCC}, {0x20,0xE8}},
{16,	{0x89,0xCE}, {0x01,0xA2}},
{16,	{0x89,0xD0}, {0x10,0x02}},
{16,	{0x89,0xD2}, {0x0D,0x02}},
{16,	{0x89,0xD4}, {0x19,0x02}},
{16,	{0x89,0xD6}, {0x00,0x94}},
{16,	{0x89,0xD8}, {0x7F,0xE0}},
{16,	{0x89,0xDA}, {0x70,0x28}},
{16,	{0x89,0xDC}, {0x73,0x08}},
{16,	{0x89,0xDE}, {0x10,0x00}},
{16,	{0x89,0xE0}, {0x09,0x00}},
{16,	{0x89,0xE2}, {0x79,0x04}},
{16,	{0x89,0xE4}, {0x79,0x47}},
{16,	{0x89,0xE6}, {0x1B,0x00}},
{16,	{0x89,0xE8}, {0x00,0x64}},
{16,	{0x89,0xEA}, {0x7E,0xE0}},
{16,	{0x89,0xEC}, {0xE2,0x80}},
{16,	{0x89,0xEE}, {0x24,0xCA}},
{16,	{0x89,0xF0}, {0x70,0x82}},
{16,	{0x89,0xF2}, {0x78,0xE0}},
{16,	{0x89,0xF4}, {0x20,0xE8}},
{16,	{0x89,0xF6}, {0x01,0xA2}},
{16,	{0x89,0xF8}, {0x11,0x02}},
{16,	{0x89,0xFA}, {0x05,0x02}},
{16,	{0x89,0xFC}, {0x18,0x02}},
{16,	{0x89,0xFE}, {0x00,0xB4}},
{16,	{0x8A,0x00}, {0x7F,0xE0}},
{16,	{0x8A,0x02}, {0x70,0x28}},
{16,	{0x8A,0x04}, {0x00,0x00}},
{16,	{0x8A,0x06}, {0x00,0x00}},
{16,	{0x8A,0x08}, {0xFF,0x80}},
{16,	{0x8A,0x0A}, {0x09,0x7C}},
{16,	{0x8A,0x0C}, {0xFF,0x80}},
{16,	{0x8A,0x0E}, {0x08,0xCC}},
{16,	{0x8A,0x10}, {0x00,0x00}},
{16,	{0x8A,0x12}, {0x08,0xDC}},
{16,	{0x8A,0x14}, {0x00,0x00}},
{16,	{0x8A,0x16}, {0x09,0x98}},


{16,	{0x09,0x8E}, {0x00,0x16}}, 	// LOGICAL_ADDRESS_ACCESS [MON_ADDRESS_LO]
{16,	{0x80,0x16}, {0x08,0x6C}}, 	// MON_ADDRESS_LO
{16,	{0x80,0x02}, {0x00,0x01}}, 	// MON_CMD
//  POLL  MON_PATCH_0 =>  0x01
{16,	{0xff,0xff}, {0x00,0xfa}},	//Delay =250
{16,	{0x09,0x8E}, {0xC4,0x0C}}, 	// LOGICAL_ADDRESS_ACCESS
{16,	{0xC4,0x0C}, {0x00,0xFF}}, 	// AFM_POS_MAX
{16,	{0xC4,0x0A}, {0x00,0x00}}, 	// AFM_POS_MIN


// Patch-specific defaults that need to changed
{16,	{0x09,0x8E}, {0xC4,0x0C}},     // LOGICAL_ADDRESS_ACCESS
{16,	{0xC4,0x0C}, {0x00,0xFF}},     // AFM_POS_MAX
{16,	{0xC4,0x0A}, {0x00,0x00}},     // AFM_POS_MIN


{16,	{0x30,0xD4}, {0x90,0x80}}, 	// COLUMN_CORRECTION
{16,	{0x31,0x6E}, {0xCA,0xFF}}, 	// DAC_ECL
{16,	{0x30,0x5E}, {0x10,0xA0}}, 	// GLOBAL_GAIN
{16,	{0x3E,0x00}, {0x00,0x10}}, 	// SAMP_CONTROL
{16,	{0x3E,0x02}, {0xED,0x02}}, 	// SAMP_ADDR_EN
{16,	{0x3E,0x04}, {0xC8,0x8C}}, 	// SAMP_RD1_SIG
{16,	{0x3E,0x06}, {0xC8,0x8C}}, 	// SAMP_RD1_SIG_BOOST
{16,	{0x3E,0x08}, {0x70,0x0A}}, 	// SAMP_RD1_RST
{16,	{0x3E,0x0A}, {0x70,0x1E}}, 	// SAMP_RD1_RST_BOOST
{16,	{0x3E,0x0C}, {0x00,0xFF}}, 	// SAMP_RST1_EN
{16,	{0x3E,0x0E}, {0x00,0xFF}}, 	// SAMP_RST1_BOOST
{16,	{0x3E,0x10}, {0x00,0xFF}}, 	// SAMP_RST1_CLOOP_SH
{16,	{0x3E,0x12}, {0x00,0x00}}, 	// SAMP_RST_BOOST_SEQ
{16,	{0x3E,0x14}, {0xC7,0x8C}}, 	// SAMP_SAMP1_SIG
{16,	{0x3E,0x16}, {0x6E,0x06}}, 	// SAMP_SAMP1_RST
{16,	{0x3E,0x18}, {0xA5,0x8C}}, 	// SAMP_TX_EN
{16,	{0x3E,0x1A}, {0xA5,0x8E}}, 	// SAMP_TX_BOOST
{16,	{0x3E,0x1C}, {0xA5,0x8E}}, 	// SAMP_TX_CLOOP_SH
{16,	{0x3E,0x1E}, {0xC0,0xD0}}, 	// SAMP_TX_BOOST_SEQ
{16,	{0x3E,0x20}, {0xEB,0x00}}, 	// SAMP_VLN_EN
{16,	{0x3E,0x22}, {0x00,0xFF}}, 	// SAMP_VLN_HOLD
{16,	{0x3E,0x24}, {0xEB,0x02}}, 	// SAMP_VCL_EN
{16,	{0x3E,0x26}, {0xEA,0x02}}, 	// SAMP_COLCLAMP
{16,	{0x3E,0x28}, {0xEB,0x0A}}, 	// SAMP_SH_VCL
{16,	{0x3E,0x2A}, {0xEC,0x01}}, 	// SAMP_SH_VREF
{16,	{0x3E,0x2C}, {0xEB,0x01}}, 	// SAMP_SH_VBST
{16,	{0x3E,0x2E}, {0x00,0xFF}}, 	// SAMP_SPARE
{16,	{0x3E,0x30}, {0x00,0xF3}}, 	// SAMP_READOUT
{16,	{0x3E,0x32}, {0x3D,0xFA}}, 	// SAMP_RESET_DONE
{16,	{0x3E,0x34}, {0x00,0xFF}}, 	// SAMP_VLN_CLAMP
{16,	{0x3E,0x36}, {0x00,0xF3}}, 	// SAMP_ASC_INT
{16,	{0x3E,0x38}, {0x00,0x00}}, 	// SAMP_RS_CLOOP_SH_R
{16,	{0x3E,0x3A}, {0xF8,0x02}}, 	// SAMP_RS_CLOOP_SH
{16,	{0x3E,0x3C}, {0x0F,0xFF}}, 	// SAMP_RS_BOOST_SEQ
{16,	{0x3E,0x3E}, {0xEA,0x10}}, 	// SAMP_TXLO_GND
{16,	{0x3E,0x40}, {0xEB,0x05}}, 	// SAMP_VLN_PER_COL
{16,	{0x3E,0x42}, {0xE5,0xC8}}, 	// SAMP_RD2_SIG
{16,	{0x3E,0x44}, {0xE5,0xC8}}, 	// SAMP_RD2_SIG_BOOST
{16,	{0x3E,0x46}, {0x8C,0x70}}, 	// SAMP_RD2_RST
{16,	{0x3E,0x48}, {0x8C,0x71}}, 	// SAMP_RD2_RST_BOOST
{16,	{0x3E,0x4A}, {0x00,0xFF}}, 	// SAMP_RST2_EN
{16,	{0x3E,0x4C}, {0x00,0xFF}}, 	// SAMP_RST2_BOOST
{16,	{0x3E,0x4E}, {0x00,0xFF}}, 	// SAMP_RST2_CLOOP_SH
{16,	{0x3E,0x50}, {0xE3,0x8D}}, 	// SAMP_SAMP2_SIG
{16,	{0x3E,0x52}, {0x8B,0x0A}}, 	// SAMP_SAMP2_RST
{16,	{0x3E,0x58}, {0xEB,0x0A}}, 	// SAMP_PIX_CLAMP_EN
{16,	{0x3E,0x5C}, {0x0A,0x00}}, 	// SAMP_PIX_PULLUP_EN
{16,	{0x3E,0x5E}, {0x00,0xFF}}, 	// SAMP_PIX_PULLDOWN_EN_R
{16,	{0x3E,0x60}, {0x00,0xFF}}, 	// SAMP_PIX_PULLDOWN_EN_S
{16,	{0x3E,0x90}, {0x3C,0x01}}, 	// RST_ADDR_EN
{16,	{0x3E,0x92}, {0x00,0xFF}}, 	// RST_RST_EN
{16,	{0x3E,0x94}, {0x00,0xFF}}, 	// RST_RST_BOOST
{16,	{0x3E,0x96}, {0x3C,0x00}}, 	// RST_TX_EN
{16,	{0x3E,0x98}, {0x3C,0x00}}, 	// RST_TX_BOOST
{16,	{0x3E,0x9A}, {0x3C,0x00}}, 	// RST_TX_CLOOP_SH
{16,	{0x3E,0x9C}, {0xC0,0xE0}}, 	// RST_TX_BOOST_SEQ
{16,	{0x3E,0x9E}, {0x00,0xFF}}, 	// RST_RST_CLOOP_SH
{16,	{0x3E,0xA0}, {0x00,0x00}}, 	// RST_RST_BOOST_SEQ
{16,	{0x3E,0xA6}, {0x3C,0x00}}, 	// RST_PIX_PULLUP_EN
{16,	{0x3E,0xD8}, {0x30,0x57}}, 	// DAC_LD_12_13
{16,	{0x31,0x6C}, {0xB4,0x4F}}, 	// DAC_TXLO
{16,	{0x31,0x6E}, {0xCA,0xFF}}, 	// DAC_ECL
{16,	{0x3E,0xD2}, {0xEA,0x0A}}, 	// DAC_LD_6_7
{16,	{0x3E,0xD4}, {0x00,0xA3}}, 	// DAC_LD_8_9
{16,	{0x3E,0xDC}, {0x60,0x20}}, 	// DAC_LD_16_17
{16,	{0x3E,0xE6}, {0xA5,0x41}}, 	// DAC_LD_26_27
{16,	{0x31,0xE0}, {0x00,0x00}}, 	// PIX_DEF_ID
{16,	{0x3E,0xD0}, {0x24,0x09}}, 	// DAC_LD_4_5
{16,	{0x3E,0xDE}, {0x0A,0x49}}, 	// DAC_LD_18_19
{16,	{0x3E,0xE0}, {0x49,0x10}}, 	// DAC_LD_20_21
{16,	{0x3E,0xE2}, {0x09,0xD2}}, 	// DAC_LD_22_23
{16,	{0x30,0xB6}, {0x00,0x06}}, 	// AUTOLR_CONTROL
//REG= 0x098E, 0x8404 	// LOGICAL_ADDRESS_ACCESS [SEQ_CMD]
{8,	{0x84,0x04}, {0x06}},		// SEQ_CMD
{16,	{0xff,0xff}, {0x00,0x64}},		//delay = 100
{16,	{0x33,0x7C}, {0x00,0x06}}, 	// YUV_YCBCR_CONTROL

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//[Step4-PGA]
#if 0
{16,	{0x36,0x40}, {0x03,0xD0}},
{16,	{0x36,0x42}, {0x00,0xCD}},
{16,	{0x36,0x44}, {0x37,0xF1}},
{16,	{0x36,0x46}, {0x75,0xED}},
{16,	{0x36,0x48}, {0x9F,0xD1}},
{16,	{0x36,0x4A}, {0x05,0x90}},
{16,	{0x36,0x4C}, {0x9B,0x0E}},
{16,	{0x36,0x4E}, {0x41,0x30}},
{16,	{0x36,0x50}, {0x41,0xCF}},
{16,	{0x36,0x52}, {0xCB,0x30}},
{16,	{0x36,0x54}, {0x03,0xF0}},
{16,	{0x36,0x56}, {0x3F,0x0C}},
{16,	{0x36,0x58}, {0x6C,0xF0}},
{16,	{0x36,0x5A}, {0x75,0x0C}},
{16,	{0x36,0x5C}, {0xD3,0x90}},
{16,	{0x36,0x5E}, {0x03,0x10}},
{16,	{0x36,0x60}, {0xB9,0x4E}},
{16,	{0x36,0x62}, {0x24,0xD1}},
{16,	{0x36,0x64}, {0x29,0xAF}},
{16,	{0x36,0x66}, {0x98,0x31}},
{16,	{0x36,0x80}, {0x3D,0xCD}},
{16,	{0x36,0x82}, {0x8B,0xF0}},
{16,	{0x36,0x84}, {0x8D,0x0D}},
{16,	{0x36,0x86}, {0x07,0x31}},
{16,	{0x36,0x88}, {0x5F,0x10}},
{16,	{0x36,0x8A}, {0x10,0x8F}},
{16,	{0x36,0x8C}, {0xC4,0x0F}},
{16,	{0x36,0x8E}, {0x9B,0x4F}},
{16,	{0x36,0x90}, {0x59,0xB0}},
{16,	{0x36,0x92}, {0x30,0xAF}},
{16,	{0x36,0x94}, {0x1D,0xEC}},
{16,	{0x36,0x96}, {0xCA,0x2E}},
{16,	{0x36,0x98}, {0x05,0x6F}},
{16,	{0x36,0x9A}, {0x5B,0xEF}},
{16,	{0x36,0x9C}, {0x5C,0xAB}},
{16,	{0x36,0x9E}, {0x23,0xAD}},
{16,	{0x36,0xA0}, {0x98,0x90}},
{16,	{0x36,0xA2}, {0x07,0x4F}},
{16,	{0x36,0xA4}, {0x0E,0x91}},
{16,	{0x36,0xA6}, {0x75,0xED}},
{16,	{0x36,0xC0}, {0x62,0x31}},
{16,	{0x36,0xC2}, {0xF4,0x2F}},
{16,	{0x36,0xC4}, {0xFE,0x30}},
{16,	{0x36,0xC6}, {0x48,0x32}},
{16,	{0x36,0xC8}, {0xDD,0x53}},
{16,	{0x36,0xCA}, {0x39,0xD1}},
{16,	{0x36,0xCC}, {0xE4,0x70}},
{16,	{0x36,0xCE}, {0x85,0x32}},
{16,	{0x36,0xD0}, {0x3A,0x72}},
{16,	{0x36,0xD2}, {0xC9,0x91}},
{16,	{0x36,0xD4}, {0x41,0xB1}},
{16,	{0x36,0xD6}, {0x45,0x8D}},
{16,	{0x36,0xD8}, {0x5F,0xCE}},
{16,	{0x36,0xDA}, {0x79,0x51}},
{16,	{0x36,0xDC}, {0xF5,0x33}},
{16,	{0x36,0xDE}, {0x74,0x11}},
{16,	{0x36,0xE0}, {0x99,0xB1}},
{16,	{0x36,0xE2}, {0xD1,0x50}},
{16,	{0x36,0xE4}, {0x0B,0x73}},
{16,	{0x36,0xE6}, {0xE9,0x13}},
{16,	{0x37,0x00}, {0x3B,0x30}},
{16,	{0x37,0x02}, {0x13,0x11}},
{16,	{0x37,0x04}, {0x24,0x50}},
{16,	{0x37,0x06}, {0xDD,0xD1}},
{16,	{0x37,0x08}, {0x88,0xD1}},
{16,	{0x37,0x0A}, {0x06,0xF0}},
{16,	{0x37,0x0C}, {0x59,0xF0}},
{16,	{0x37,0x0E}, {0xE6,0xCF}},
{16,	{0x37,0x10}, {0xD7,0xD1}},
{16,	{0x37,0x12}, {0x7B,0x92}},
{16,	{0x37,0x14}, {0x58,0x0E}},
{16,	{0x37,0x16}, {0x6A,0x2F}},
{16,	{0x37,0x18}, {0x38,0xD1}},
{16,	{0x37,0x1A}, {0x7D,0x6D}},
{16,	{0x37,0x1C}, {0xB9,0x91}},
{16,	{0x37,0x1E}, {0x7D,0x8F}},
{16,	{0x37,0x20}, {0x19,0xB0}},
{16,	{0x37,0x22}, {0x23,0x30}},
{16,	{0x37,0x24}, {0xC8,0x8F}},
{16,	{0x37,0x26}, {0xA5,0x8F}},
{16,	{0x37,0x40}, {0xCB,0x51}},
{16,	{0x37,0x42}, {0x18,0x13}},
{16,	{0x37,0x44}, {0xFB,0x74}},
{16,	{0x37,0x46}, {0x9D,0xF5}},
{16,	{0x37,0x48}, {0x5C,0x16}},
{16,	{0x37,0x4A}, {0xFB,0x71}},
{16,	{0x37,0x4C}, {0x2B,0xF3}},
{16,	{0x37,0x4E}, {0xBB,0xF3}},
{16,	{0x37,0x50}, {0x92,0x35}},
{16,	{0x37,0x52}, {0x1A,0x36}},
{16,	{0x37,0x54}, {0xA6,0xF1}},
{16,	{0x37,0x56}, {0x58,0xF2}},
{16,	{0x37,0x58}, {0xFA,0x74}},
{16,	{0x37,0x5A}, {0x80,0xB5}},
{16,	{0x37,0x5C}, {0x51,0x36}},
{16,	{0x37,0x5E}, {0xDC,0x71}},
{16,	{0x37,0x60}, {0x4E,0x53}},
{16,	{0x37,0x62}, {0x84,0x75}},
{16,	{0x37,0x64}, {0xAF,0x55}},
{16,	{0x37,0x66}, {0x64,0xD6}},
{16,	{0x37,0x82}, {0x03,0xB4}},
{16,	{0x37,0x84}, {0x05,0x1C}}, 	// CENTER_COLUMN
#else
{16, {0x36,0x40}, {0x04,0x50}},
{16, {0x36,0x42}, {0x29,0xAD}},
{16, {0x36,0x44}, {0x14,0x91}},
{16, {0x36,0x46}, {0x0B,0x0C}},
{16, {0x36,0x48}, {0xA1,0x50}},
{16, {0x36,0x4A}, {0x03,0x10}},
{16, {0x36,0x4C}, {0x91,0xCE}},
{16, {0x36,0x4E}, {0x67,0xB0}},
{16, {0x36,0x50}, {0x69,0xEE}},
{16, {0x36,0x52}, {0x9C,0xF0}},
{16, {0x36,0x54}, {0x03,0xB0}},
{16, {0x36,0x56}, {0x02,0xAD}},
{16, {0x36,0x58}, {0x74,0x0F}},
{16, {0x36,0x5A}, {0x87,0x2E}},
{16, {0x36,0x5C}, {0x49,0xCB}},
{16, {0x36,0x5E}, {0x02,0x70}},
{16, {0x36,0x60}, {0x99,0x8E}},
{16, {0x36,0x62}, {0x1A,0xB1}},
{16, {0x36,0x64}, {0x43,0x8D}},
{16, {0x36,0x66}, {0xB3,0x10}},
{16, {0x36,0x80}, {0x84,0x4D}},
{16, {0x36,0x82}, {0xA0,0x2E}},
{16, {0x36,0x84}, {0x0C,0x2D}},
{16, {0x36,0x86}, {0x4F,0xEF}},
{16, {0x36,0x88}, {0x97,0x70}},
{16, {0x36,0x8A}, {0x4E,0x6C}},
{16, {0x36,0x8C}, {0x02,0xCE}},
{16, {0x36,0x8E}, {0x81,0xEA}},
{16, {0x36,0x90}, {0x92,0x0E}},
{16, {0x36,0x92}, {0xE0,0x8E}},
{16, {0x36,0x94}, {0x92,0xEE}},
{16, {0x36,0x96}, {0x5B,0xAD}},
{16, {0x36,0x98}, {0x4E,0xCE}},
{16, {0x36,0x9A}, {0xDE,0x6E}},
{16, {0x36,0x9C}, {0xEA,0x6E}},
{16, {0x36,0x9E}, {0xD0,0x6D}},
{16, {0x36,0xA0}, {0xAB,0xEE}},
{16, {0x36,0xA2}, {0x11,0x6F}},
{16, {0x36,0xA4}, {0x30,0x0F}},
{16, {0x36,0xA6}, {0x9F,0x90}},
{16, {0x36,0xC0}, {0x27,0x91}},
{16, {0x36,0xC2}, {0xF2,0x8F}},
{16, {0x36,0xC4}, {0x79,0x50}},
{16, {0x36,0xC6}, {0x10,0x32}},
{16, {0x36,0xC8}, {0x97,0x53}},
{16, {0x36,0xCA}, {0x0B,0x11}},
{16, {0x36,0xCC}, {0xAD,0x30}},
{16, {0x36,0xCE}, {0x42,0x50}},
{16, {0x36,0xD0}, {0x75,0x11}},
{16, {0x36,0xD2}, {0xC1,0x92}},
{16, {0x36,0xD4}, {0x69,0xD0}},
{16, {0x36,0xD6}, {0x8E,0x8E}},
{16, {0x36,0xD8}, {0x64,0x71}},
{16, {0x36,0xDA}, {0x5A,0x31}},
{16, {0x36,0xDC}, {0x91,0x53}},
{16, {0x36,0xDE}, {0x34,0x71}},
{16, {0x36,0xE0}, {0x8C,0x31}},
{16, {0x36,0xE2}, {0x42,0x11}},
{16, {0x36,0xE4}, {0x47,0x32}},
{16, {0x36,0xE6}, {0xBC,0x33}},
{16, {0x37,0x00}, {0x8F,0xED}},
{16, {0x37,0x02}, {0x80,0x28}},
{16, {0x37,0x04}, {0xF9,0x6F}},
{16, {0x37,0x06}, {0x90,0x11}},
{16, {0x37,0x08}, {0x30,0x52}},
{16, {0x37,0x0A}, {0x23,0x4D}},
{16, {0x37,0x0C}, {0xAF,0x6E}},
{16, {0x37,0x0E}, {0xA1,0xF0}},
{16, {0x37,0x10}, {0x24,0x90}},
{16, {0x37,0x12}, {0x78,0xAF}},
{16, {0x37,0x14}, {0xFD,0xEE}},
{16, {0x37,0x16}, {0xDF,0xAE}},
{16, {0x37,0x18}, {0x61,0xEF}},
{16, {0x37,0x1A}, {0x51,0x0F}},
{16, {0x37,0x1C}, {0x52,0xAD}},
{16, {0x37,0x1E}, {0xEC,0x4E}},
{16, {0x37,0x20}, {0x48,0xAB}},
{16, {0x37,0x22}, {0x28,0xF0}},
{16, {0x37,0x24}, {0x96,0x51}},
{16, {0x37,0x26}, {0x26,0xB0}},
{16, {0x37,0x40}, {0xDF,0xEF}},
{16, {0x37,0x42}, {0x1E,0x72}},
{16, {0x37,0x44}, {0x8F,0x74}},
{16, {0x37,0x46}, {0xCC,0xD4}},
{16, {0x37,0x48}, {0x3C,0x55}},
{16, {0x37,0x4A}, {0x82,0xF0}},
{16, {0x37,0x4C}, {0x14,0x72}},
{16, {0x37,0x4E}, {0xE6,0xB3}},
{16, {0x37,0x50}, {0x93,0x34}},
{16, {0x37,0x52}, {0x0E,0xF5}},
{16, {0x37,0x54}, {0xA3,0xAB}},
{16, {0x37,0x56}, {0x24,0x91}},
{16, {0x37,0x58}, {0x86,0xD4}},
{16, {0x37,0x5A}, {0x8D,0xB4}},
{16, {0x37,0x5C}, {0x1D,0x35}},
{16, {0x37,0x5E}, {0xFF,0xCF}},
{16, {0x37,0x60}, {0x55,0x52}},
{16, {0x37,0x62}, {0xA1,0x34}},
{16, {0x37,0x64}, {0xDD,0x94}},
{16, {0x37,0x66}, {0x4E,0x15}},
{16, {0x37,0x82}, {0x03,0xDC}},
{16, {0x37,0x84}, {0x04,0xB8}},

#endif
{16, {0x32,0x10}, {0x49,0xB8}}, 	// COLOR_PIPELINE_CONTROL
{16, {0xff,0xff},	{0x00,0x01}},		//delay = 1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//[Step5-AWB_CCM]
{16,	{0x09,0x8E}, {0xAC,0x01}}, 	// LOGICAL_ADDRESS_ACCESS [AWB_MODE]
{8,	  {0xAC,0x01}, {0xAB}},// AWB_MODE
{16,	{0xAC,0x46}, {0x02,0x21}}, 	// AWB_LEFT_CCM_0
{16,	{0xAC,0x48}, {0xFE,0xAE}}, 	// AWB_LEFT_CCM_1
{16,	{0xAC,0x4A}, {0x00,0x32}}, 	// AWB_LEFT_CCM_2
{16,	{0xAC,0x4C}, {0xFF,0xC5}}, 	// AWB_LEFT_CCM_3
{16,	{0xAC,0x4E}, {0x01,0x54}}, 	// AWB_LEFT_CCM_4
{16,	{0xAC,0x50}, {0xFF,0xE7}}, 	// AWB_LEFT_CCM_5
{16,	{0xAC,0x52}, {0xFF,0xB1}}, 	// AWB_LEFT_CCM_6
{16,	{0xAC,0x54}, {0xFE,0xC5}}, 	// AWB_LEFT_CCM_7
{16,	{0xAC,0x56}, {0x02,0x8A}}, 	// AWB_LEFT_CCM_8
{16,	{0xAC,0x58}, {0x01,0x30}}, 	// AWB_LEFT_CCM_R2BRATIO
{16,	{0xAC,0x5C}, {0x01,0xCD}}, 	// AWB_RIGHT_CCM_0
{16,	{0xAC,0x5E}, {0xFF,0x63}}, 	// AWB_RIGHT_CCM_1
{16,	{0xAC,0x60}, {0xFF,0xD0}}, 	// AWB_RIGHT_CCM_2
{16,	{0xAC,0x62}, {0xFF,0xCD}}, 	// AWB_RIGHT_CCM_3
{16,	{0xAC,0x64}, {0x01,0x3B}}, 	// AWB_RIGHT_CCM_4
{16,	{0xAC,0x66}, {0xFF,0xF8}}, 	// AWB_RIGHT_CCM_5
{16,	{0xAC,0x68}, {0xFF,0xFB}}, 	// AWB_RIGHT_CCM_6
{16,	{0xAC,0x6A}, {0xFF,0x78}}, 	// AWB_RIGHT_CCM_7
{16,	{0xAC,0x6C}, {0x01,0x8D}}, 	// AWB_RIGHT_CCM_8
{16,	{0xAC,0x6E}, {0x00,0x55}}, 	// AWB_RIGHT_CCM_R2BRATIO
{16,	{0xB8,0x42}, {0x00,0x37}}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_X
{16,	{0xB8,0x44}, {0x00,0x44}}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_Y
{16,	{0x32,0x40}, {0x00,0x24}}, 	// AWB_XY_SCALE
{16,	{0x32,0x40}, {0x00,0x24}}, 	// AWB_XY_SCALE
{16,	{0x32,0x42}, {0x00,0x00}}, 	// AWB_WEIGHT_R0
{16,	{0x32,0x44}, {0x00,0x00}}, 	// AWB_WEIGHT_R1
{16,	{0x32,0x46}, {0x00,0x00}}, 	// AWB_WEIGHT_R2
{16,	{0x32,0x48}, {0x7F,0x00}}, 	// AWB_WEIGHT_R3
{16,	{0x32,0x4A}, {0xA5,0x00}}, 	// AWB_WEIGHT_R4
{16,	{0x32,0x4C}, {0x15,0x40}}, 	// AWB_WEIGHT_R5
{16,	{0x32,0x4E}, {0x01,0xAC}}, 	// AWB_WEIGHT_R6
{16,	{0x32,0x50}, {0x00,0x3E}}, 	// AWB_WEIGHT_R7
{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD

{16,	{0xff,0xff},	{0x00,0x64}},	//delay = 100

{8,	{0xAC,0x3C}, {0x2E}},// AWB_MIN_ACCEPTED_PRE_AWB_R2G_RATIO
{8,	{0xAC,0x3D}, {0x84}},// AWB_MAX_ACCEPTED_PRE_AWB_R2G_RATIO
{8,	{0xAC,0x3E}, {0x11}},// AWB_MIN_ACCEPTED_PRE_AWB_B2G_RATIO
{8,	{0xAC,0x3F}, {0x63}},// AWB_MAX_ACCEPTED_PRE_AWB_B2G_RATIO
{8,	{0xAC,0xB0}, {0x2B}},// AWB_RG_MIN
{8,	{0xAC,0xB1}, {0x84}},// AWB_RG_MAX
{8,	{0xAC,0xB4}, {0x11}},// AWB_BG_MIN
{8,	{0xAC,0xB5}, {0x63}},// AWB_BG_MAX


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//[Step6-CPIPE_Calibration]
//REG= 0x098E, 0xD80F 	// LOGICAL_ADDRESS_ACCESS [JPEG_QSCALE_0]
{8 ,	{0xD8,0x0F}, {0x04}},// JPEG_QSCALE_0
{8 ,	{0xD8,0x10}, {0x08}},// JPEG_QSCALE_1
{8 ,	{0xC8,0xD2}, {0x04}},// CAM_OUTPUT_1_JPEG_QSCALE_0
{8 ,	{0xC8,0xD3}, {0x08}},// CAM_OUTPUT_1_JPEG_QSCALE_1
{8 ,	{0xC8,0xBC}, {0x04}},// CAM_OUTPUT_0_JPEG_QSCALE_0
{8 ,	{0xC8,0xBD}, {0x08}},// CAM_OUTPUT_0_JPEG_QSCALE_1
{16,	{0x30,0x1A}, {0x10,0xF4}}, 	// RESET_REGISTER
{16,	{0x30,0x1E}, {0x00,0x00}}, 	// DATA_PEDESTAL
{16,	{0x30,0x1A}, {0x10,0xFC}}, 	// RESET_REGISTER
{8 ,	{0xDC,0x33}, {0x00}},// SYS_FIRST_BLACK_LEVEL
{8 ,	{0xDC,0x35}, {0x04}},// SYS_UV_COLOR_BOOST
{16,	{0x32,0x6E}, {0x00,0x06}}, 	// LOW_PASS_YUV_FILTER
{8 ,	{0xDC,0x37}, {0x62}},// SYS_BRIGHT_COLORKILL
{16,	{0x35,0xA4}, {0x05,0x96}}, 	// BRIGHT_COLOR_KILL_CONTROLS
{16,	{0x35,0xA2}, {0x00,0x94}}, 	// DARK_COLOR_KILL_CONTROLS
{8 ,	{0xDC,0x36}, {0x23}},// SYS_DARK_COLOR_KILL
{8 ,	{0x84,0x04}, {0x06}},// SEQ_CMD

{16,	{0xff,0xff},	{0x01,0x2c}},	//delay = 300

{8,	{0xBC,0x18}, {0x00}},// LL_GAMMA_CONTRAST_CURVE_0
{8,	{0xBC,0x19}, {0x11}},// LL_GAMMA_CONTRAST_CURVE_1
{8,	{0xBC,0x1A}, {0x23}},// LL_GAMMA_CONTRAST_CURVE_2
{8,	{0xBC,0x1B}, {0x3F}},// LL_GAMMA_CONTRAST_CURVE_3
{8,	{0xBC,0x1C}, {0x67}},// LL_GAMMA_CONTRAST_CURVE_4
{8,	{0xBC,0x1D}, {0x85}},// LL_GAMMA_CONTRAST_CURVE_5
{8,	{0xBC,0x1E}, {0x9B}},// LL_GAMMA_CONTRAST_CURVE_6
{8,	{0xBC,0x1F}, {0xAD}},// LL_GAMMA_CONTRAST_CURVE_7
{8,	{0xBC,0x20}, {0xBB}},// LL_GAMMA_CONTRAST_CURVE_8
{8,	{0xBC,0x21}, {0xC7}},// LL_GAMMA_CONTRAST_CURVE_9
{8,	{0xBC,0x22}, {0xD1}},// LL_GAMMA_CONTRAST_CURVE_10
{8,	{0xBC,0x23}, {0xDA}},// LL_GAMMA_CONTRAST_CURVE_11
{8,	{0xBC,0x24}, {0xE1}},// LL_GAMMA_CONTRAST_CURVE_12
{8,	{0xBC,0x25}, {0xE8}},// LL_GAMMA_CONTRAST_CURVE_13
{8,	{0xBC,0x26}, {0xEE}},// LL_GAMMA_CONTRAST_CURVE_14
{8,	{0xBC,0x27}, {0xF3}},// LL_GAMMA_CONTRAST_CURVE_15
{8,	{0xBC,0x28}, {0xF7}},// LL_GAMMA_CONTRAST_CURVE_16
{8,	{0xBC,0x29}, {0xFB}},// LL_GAMMA_CONTRAST_CURVE_17
{8,	{0xBC,0x2A}, {0xFF}},// LL_GAMMA_CONTRAST_CURVE_18
{8,	{0xBC,0x2B}, {0x00}},// LL_GAMMA_NEUTRAL_CURVE_0
{8,	{0xBC,0x2C}, {0x11}},// LL_GAMMA_NEUTRAL_CURVE_1
{8,	{0xBC,0x2D}, {0x23}},// LL_GAMMA_NEUTRAL_CURVE_2
{8,	{0xBC,0x2E}, {0x3F}},// LL_GAMMA_NEUTRAL_CURVE_3
{8,	{0xBC,0x2F}, {0x67}},// LL_GAMMA_NEUTRAL_CURVE_4
{8,	{0xBC,0x30}, {0x85}},// LL_GAMMA_NEUTRAL_CURVE_5
{8,	{0xBC,0x31}, {0x9B}},// LL_GAMMA_NEUTRAL_CURVE_6
{8,	{0xBC,0x32}, {0xAD}},// LL_GAMMA_NEUTRAL_CURVE_7
{8,	{0xBC,0x33}, {0xBB}},// LL_GAMMA_NEUTRAL_CURVE_8
{8,	{0xBC,0x34}, {0xC7}},// LL_GAMMA_NEUTRAL_CURVE_9
{8,	{0xBC,0x35}, {0xD1}},// LL_GAMMA_NEUTRAL_CURVE_10
{8,	{0xBC,0x36}, {0xDA}},// LL_GAMMA_NEUTRAL_CURVE_11
{8,	{0xBC,0x37}, {0xE1}},// LL_GAMMA_NEUTRAL_CURVE_12
{8,	{0xBC,0x38}, {0xE8}},// LL_GAMMA_NEUTRAL_CURVE_13
{8,	{0xBC,0x39}, {0xEE}},// LL_GAMMA_NEUTRAL_CURVE_14
{8,	{0xBC,0x3A}, {0xF3}},// LL_GAMMA_NEUTRAL_CURVE_15
{8,	{0xBC,0x3B}, {0xF7}},// LL_GAMMA_NEUTRAL_CURVE_16
{8,	{0xBC,0x3C}, {0xFB}},// LL_GAMMA_NEUTRAL_CURVE_17
{8,	{0xBC,0x3D}, {0xFF}},// LL_GAMMA_NEUTRAL_CURVE_18
{8,	{0xBC,0x3E}, {0x00}},// LL_GAMMA_NR_CURVE_0
{8,	{0xBC,0x3F}, {0x18}},// LL_GAMMA_NR_CURVE_1
{8,	{0xBC,0x40}, {0x25}},// LL_GAMMA_NR_CURVE_2
{8,	{0xBC,0x41}, {0x3A}},// LL_GAMMA_NR_CURVE_3
{8,	{0xBC,0x42}, {0x59}},// LL_GAMMA_NR_CURVE_4
{8,	{0xBC,0x43}, {0x70}},// LL_GAMMA_NR_CURVE_5
{8,	{0xBC,0x44}, {0x81}},// LL_GAMMA_NR_CURVE_6
{8,	{0xBC,0x45}, {0x90}},// LL_GAMMA_NR_CURVE_7
{8,	{0xBC,0x46}, {0x9E}},// LL_GAMMA_NR_CURVE_8
{8,	{0xBC,0x47}, {0xAB}},// LL_GAMMA_NR_CURVE_9
{8,	{0xBC,0x48}, {0xB6}},// LL_GAMMA_NR_CURVE_10
{8,	{0xBC,0x49}, {0xC1}},// LL_GAMMA_NR_CURVE_11
{8,	{0xBC,0x4A}, {0xCB}},// LL_GAMMA_NR_CURVE_12
{8,	{0xBC,0x4B}, {0xD5}},// LL_GAMMA_NR_CURVE_13
{8,	{0xBC,0x4C}, {0xDE}},// LL_GAMMA_NR_CURVE_14
{8,	{0xBC,0x4D}, {0xE7}},// LL_GAMMA_NR_CURVE_15
{8,	{0xBC,0x4E}, {0xEF}},// LL_GAMMA_NR_CURVE_16
{8,	{0xBC,0x4F}, {0xF7}},// LL_GAMMA_NR_CURVE_17
{8,	{0xBC,0x50}, {0xFF}},// LL_GAMMA_NR_CURVE_18
{8,	{0x84,0x04}, {0x06}},// SEQ_CMD

{16,	{0xff,0xff},	{0x00,0x64}},//delay = 100


{8,	  {0xB8,0x01}, {0xE0}},// STAT_MODE
{8,	  {0xB8,0x62}, {0x04}},// STAT_BMTRACKING_SPEED
{8,	  {0xB8,0x29}, {0x02}},// STAT_LL_BRIGHTNESS_METRIC_DIVISOR
{8,	  {0xB8,0x63}, {0x02}},// STAT_BM_MUL
{8,	  {0xB8,0x27}, {0x0F}},// STAT_AE_EV_SHIFT
{8,	  {0xA4,0x09}, {0x37}},// AE_RULE_BASE_TARGET
{16,	{0xBC,0x52}, {0x00,0xC8}}, 	// LL_START_BRIGHTNESS_METRIC
{16,	{0xBC,0x54}, {0x0A,0x28}}, 	// LL_END_BRIGHTNESS_METRIC
{16,	{0xBC,0x58}, {0x00,0xC8}}, 	// LL_START_GAIN_METRIC
{16,	{0xBC,0x5A}, {0x12,0xC0}}, 	// LL_END_GAIN_METRIC
{16,	{0xBC,0x5E}, {0x00,0xFA}}, 	// LL_START_APERTURE_GAIN_BM
{16,	{0xBC,0x60}, {0x02,0x58}}, 	// LL_END_APERTURE_GAIN_BM
{16,	{0xBC,0x66}, {0x00,0xFA}}, 	// LL_START_APERTURE_GM
{16,	{0xBC,0x68}, {0x02,0x58}}, 	// LL_END_APERTURE_GM
{16,	{0xBC,0x86}, {0x00,0xC8}}, 	// LL_START_FFNR_GM
{16,	{0xBC,0x88}, {0x06,0x40}}, 	// LL_END_FFNR_GM
{16,	{0xBC,0xBC}, {0x00,0x40}}, 	// LL_SFFB_START_GAIN
{16,	{0xBC,0xBE}, {0x01,0xFC}}, 	// LL_SFFB_END_GAIN
{16,	{0xBC,0xCC}, {0x00,0xC8}}, 	// LL_SFFB_START_MAX_GM
{16,	{0xBC,0xCE}, {0x06,0x40}}, 	// LL_SFFB_END_MAX_GM
{16,	{0xBC,0x90}, {0x00,0xC8}}, 	// LL_START_GRB_GM
{16,	{0xBC,0x92}, {0x06,0x40}}, 	// LL_END_GRB_GM
{16,	{0xBC,0x0E}, {0x00,0x01}}, 	// LL_GAMMA_CURVE_ADJ_START_POS
{16,	{0xBC,0x10}, {0x00,0x02}}, 	// LL_GAMMA_CURVE_ADJ_MID_POS
{16,	{0xBC,0x12}, {0x02,0xBC}}, 	// LL_GAMMA_CURVE_ADJ_END_POS
{16,	{0xBC,0xAA}, {0x04,0x4C}}, 	// LL_CDC_THR_ADJ_START_POS
{16,	{0xBC,0xAC}, {0x00,0xAF}}, 	// LL_CDC_THR_ADJ_MID_POS
{16,	{0xBC,0xAE}, {0x00,0x09}}, 	// LL_CDC_THR_ADJ_END_POS
{16,	{0xBC,0xD8}, {0x00,0xC8}}, 	// LL_PCR_START_BM
{16,	{0xBC,0xDA}, {0x0A,0x28}}, 	// LL_PCR_END_BM
{16,	{0x33,0x80}, {0x05,0x04}}, 	// KERNEL_CONFIG
{8,	  {0xBC,0x94}, {0x0C}},// LL_GB_START_THRESHOLD_0
{8,	  {0xBC,0x95}, {0x08}},// LL_GB_START_THRESHOLD_1
{8,	  {0xBC,0x9C}, {0x3C}},// LL_GB_END_THRESHOLD_0
{8,	  {0xBC,0x9D}, {0x28}},// LL_GB_END_THRESHOLD_1
{16,	{0x33,0xB0}, {0x2A,0x16}}, 	// FFNR_ALPHA_BETA
{8,	  {0xBC,0x8A}, {0x02}},// LL_START_FF_MIX_THRESH_Y
{8,	  {0xBC,0x8B}, {0x0F}},// LL_END_FF_MIX_THRESH_Y
{8,	  {0xBC,0x8C}, {0xFF}},// LL_START_FF_MIX_THRESH_YGAIN
{8,	  {0xBC,0x8D}, {0xFF}},// LL_END_FF_MIX_THRESH_YGAIN
{8,	  {0xBC,0x8E}, {0xFF}},// LL_START_FF_MIX_THRESH_GAIN
{8,	  {0xBC,0x8F}, {0x00}},// LL_END_FF_MIX_THRESH_GAIN
{8,	  {0xBC,0xB2}, {0x20}},// LL_CDC_DARK_CLUS_SLOPE
{8,	  {0xBC,0xB3}, {0x3A}},// LL_CDC_DARK_CLUS_SATUR
{8,	  {0xBC,0xB4}, {0x39}},// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SLOPE
{8,	  {0xBC,0xB7}, {0x39}},// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SATUR
{8,	  {0xBC,0xB5}, {0x20}},// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SLOPE
{8,	  {0xBC,0xB8}, {0x3A}},// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SATUR
{8,	  {0xBC,0xB6}, {0x80}},// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SLOPE
{8,	  {0xBC,0xB9}, {0x24}},// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SATUR
{16,	{0xBC,0xAA}, {0x03,0xE8}}, 	// LL_CDC_THR_ADJ_START_POS
{16,	{0xBC,0xAC}, {0x01,0x2C}}, 	// LL_CDC_THR_ADJ_MID_POS
{16,	{0xBC,0xAE}, {0x00,0x09}}, 	// LL_CDC_THR_ADJ_END_POS
{16,	{0x33,0xBA}, {0x00,0x84}}, 	// APEDGE_CONTROL
{16,	{0x33,0xBE}, {0x00,0x00}}, 	// UA_KNEE_L
{16,	{0x33,0xC2}, {0x88,0x00}}, 	// UA_WEIGHTS
{16,	{0xBC,0x5E}, {0x01,0x54}}, 	// LL_START_APERTURE_GAIN_BM
{16,	{0xBC,0x60}, {0x06,0x40}}, 	// LL_END_APERTURE_GAIN_BM
{8,	  {0xBC,0x62}, {0x0E}},// LL_START_APERTURE_KPGAIN
{8,	  {0xBC,0x63}, {0x14}},// LL_END_APERTURE_KPGAIN
{8,	  {0xBC,0x64}, {0x0E}},// LL_START_APERTURE_KNGAIN
{8,	  {0xBC,0x65}, {0x14}},// LL_END_APERTURE_KNGAIN
{8,	  {0xBC,0xE2}, {0x0A}},// LL_START_POS_KNEE
{8,	  {0xBC,0xE3}, {0x2B}},// LL_END_POS_KNEE
{8,	  {0xBC,0xE4}, {0x0A}},// LL_START_NEG_KNEE
{8,	  {0xBC,0xE5}, {0x2B}},// LL_END_NEG_KNEE
{16,	{0x32,0x10}, {0x49,0xB8}}, 	// COLOR_PIPELINE_CONTROL
{8,	  {0xBC,0xC0}, {0x1F}},// LL_SFFB_RAMP_START
{8,	  {0xBC,0xC1}, {0x03}},// LL_SFFB_RAMP_STOP
{8,	  {0xBC,0xC2}, {0x2C}},// LL_SFFB_SLOPE_START
{8,	  {0xBC,0xC3}, {0x10}},// LL_SFFB_SLOPE_STOP
{8,	  {0xBC,0xC4}, {0x07}},// LL_SFFB_THSTART
{8,	  {0xBC,0xC5}, {0x0B}},// LL_SFFB_THSTOP
{16,	{0xBC,0xBA}, {0x00,0x09}}, 	// LL_SFFB_CONFIG
{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD

{16,	{0xff,0xff},	{0x00,0x64}},//delay = 100

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//[Step7-CPIPE_Preference]
//REG= 0x098E, 0x3C14 	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_FADE_TO_BLACK_START_POS]
{16,	{0xBC,0x14}, {0xFF,0xFE}}, 	// LL_GAMMA_FADE_TO_BLACK_START_POS
{16,	{0xBC,0x16}, {0xFF,0xFE}}, 	// LL_GAMMA_FADE_TO_BLACK_END_POS
{16,	{0xBC,0x66}, {0x01,0x54}}, 	// LL_START_APERTURE_GM
{16,	{0xBC,0x68}, {0x07,0xD0}}, 	// LL_END_APERTURE_GM
{8,	  {0xBC,0x6A}, {0x04}},// LL_START_APERTURE_INTEGER_GAIN
{8,	  {0xBC,0x6B}, {0x00}},// LL_END_APERTURE_INTEGER_GAIN
{8,	  {0xBC,0x6C}, {0x00}},// LL_START_APERTURE_EXP_GAIN
{8,	  {0xBC,0x6D}, {0x00}},// LL_END_APERTURE_EXP_GAIN
{16,	{0xA8,0x1C}, {0x00,0x40}}, 	// AE_TRACK_MIN_AGAIN
{16,	{0xA8,0x20}, {0x01,0xFC}}, 	// AE_TRACK_MAX_AGAIN
{16,	{0xA8,0x22}, {0x00,0x80}}, 	// AE_TRACK_MIN_DGAIN
{16,	{0xA8,0x24}, {0x01,0x00}}, 	// AE_TRACK_MAX_DGAIN
{8,	  {0xBC,0x56}, {0x98}},// LL_START_CCM_SATURATION
{8,	  {0xBC,0x57}, {0x1E}},// LL_END_CCM_SATURATION
{8,	  {0xBC,0xDE}, {0x03}},// LL_START_SYS_THRESHOLD
{8,	  {0xBC,0xDF}, {0x50}},// LL_STOP_SYS_THRESHOLD
{8,	  {0xBC,0xE0}, {0x08}},// LL_START_SYS_GAIN
{8,	  {0xBC,0xE1}, {0x03}},// LL_STOP_SYS_GAIN
{16,	{0xBC,0xD0}, {0x00,0x0A}}, 	// LL_SFFB_SOBEL_FLAT_START
{16,	{0xBC,0xD2}, {0x00,0xFE}}, 	// LL_SFFB_SOBEL_FLAT_STOP
{16,	{0xBC,0xD4}, {0x00,0x1E}}, 	// LL_SFFB_SOBEL_SHARP_START
{16,	{0xBC,0xD6}, {0x00,0xFF}}, 	// LL_SFFB_SOBEL_SHARP_STOP
{8,	  {0xBC,0xC6}, {0x00}},// LL_SFFB_SHARPENING_START
{8,	  {0xBC,0xC7}, {0x00}},// LL_SFFB_SHARPENING_STOP
{8,	  {0xBC,0xC8}, {0x20}},// LL_SFFB_FLATNESS_START
{8,	  {0xBC,0xC9}, {0x40}},// LL_SFFB_FLATNESS_STOP
{8,	  {0xBC,0xCA}, {0x04}},// LL_SFFB_TRANSITION_START
{8,	  {0xBC,0xCB}, {0x00}},// LL_SFFB_TRANSITION_STOP
{8,	  {0xBC,0xE6}, {0x03}},// LL_SFFB_ZERO_ENABLE
{8,	  {0xBC,0xE6}, {0x03}},// LL_SFFB_ZERO_ENABLE
{8,	  {0xA4,0x10}, {0x04}},// AE_RULE_TARGET_AE_6
{8,	  {0xA4,0x11}, {0x06}},// AE_RULE_TARGET_AE_7

{8,	  {0x84,0x04}, {0x06}}	,// SEQ_CMD

{16,	{0xff,0xff},	{0x00,0x64}},//delay = 100
//++++++++++++++++++++++++++++++++++

//[Step8-Features]
//REG= 0x098E, 0xC8BC 	// LOGICAL_ADDRESS_ACCESS [CAM_OUTPUT_0_JPEG_QSCALE_0]
{8,	{0xC8,0xBC}, {0x04}},// CAM_OUTPUT_0_JPEG_QSCALE_0
{8,	{0xC8,0xBD}, {0x0A}},// CAM_OUTPUT_0_JPEG_QSCALE_1
{8,	{0xC8,0xD2}, {0x04}},// CAM_OUTPUT_1_JPEG_QSCALE_0
{8,	{0xC8,0xD3}, {0x0A}},// CAM_OUTPUT_1_JPEG_QSCALE_1
{8,	{0xDC,0x3A}, {0x23}},// SYS_SEPIA_CR
{8,	{0xDC,0x3B}, {0xB2}},// SYS_SEPIA_CB

// Frame rate control   Set Normal mode fixed 15fps
{16,	{0xA8,0x18}, {0x07,0x56}}, // AE_TRACK_TARGET_INT_TIME_ROWS	
{16,	{0xA8,0x1A}, {0x07,0x56}}, // AE_TRACK_MAX_INT_TIME_ROWS	
{8,  	{0x84,0x04}, {0x06}}, // SEQ_CMD

//50HZ
{16,	{0x09,0x8E}, {0x84,0x17}}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_1_FD]
{8,	  {0x84,0x17}, {0x01}},// SEQ_STATE_CFG_1_FD                           //VAR8=1
{8,	  {0xA0,0x04}, {0x32}},// FD_EXPECTED_FLICKER_SOURCE_FREQUENCY         //VAR8=8
{8,	  {0x84,0x04}, {0x06}}, // SEQ_CMD 

//[VCM_Enable  Continue scan]
{16,	{0x09,0x8E}, {0xC4,0x00}}, 	// LOGICAL_ADDRESS_ACCESS [AFM_ALGO]
{8,	  {0xC4,0x00}, {0x88}},// AFM_ALGO
{8,	  {0x84,0x19}, {0x03}},// SEQ_STATE_CFG_1_AF
{8,	  {0xC4,0x00}, {0x08}},// AFM_ALGO
{16,	{0xB0,0x02}, {0x03,0x47}}, 	// AF_MODE
{16,	{0xB0,0x04}, {0x00,0x42}}, 	// AF_ALGO
//0xB008, 0x0003FFFF 	// AF_ZONE_WEIGHTS_HI
//0xB00C, 0xFFFFFFFF 	// AF_ZONE_WEIGHTS_LO
{16,	{0xC4,0x0C}, {0x00,0xF0}}, 	// AFM_POS_MAX
{16,	{0xC4,0x0A}, {0x00,0x10}}, 	// AFM_POS_MIN
{8,		{0xB0,0x18}, {0x20}},// AF_FS_POS_0
{8,		{0xB0,0x19}, {0x40}},// AF_FS_POS_1
{8,		{0xB0,0x1A}, {0x5E}},// AF_FS_POS_2
{8,		{0xB0,0x1B}, {0x7C}},// AF_FS_POS_3
{8,		{0xB0,0x1C}, {0x98}},// AF_FS_POS_4
{8,		{0xB0,0x1D}, {0xB3}},// AF_FS_POS_5
{8,		{0xB0,0x1E}, {0xCD}},// AF_FS_POS_6
{8,		{0xB0,0x1F}, {0xE5}},// AF_FS_POS_7
{8,		{0xB0,0x20}, {0xFB}},// AF_FS_POS_8
{8,		{0xB0,0x12}, {0x09}},// AF_FS_NUM_STEPS
{8,		{0xB0,0x13}, {0x77}},// AF_FS_NUM_STEPS2
{8,		{0xB0,0x14}, {0x05}},// AF_FS_STEP_SIZE
{16,	{0x09,0x8E}, {0x84,0x04}}, 	// LOGICAL_ADDRESS_ACCESS
{8,		{0x84,0x04}, {0x05}},// SEQ_CMD

//[AF Trigger]
{16,	{0x09,0x8E}, {0xB0,0x06}}, 	// LOGICAL_ADDRESS_ACCESS [AF_PROGRESS]
{8,	  {0xB0,0x06}, {0x01}},// AF_PROGRESS

{16,	{0x00,0x16}, {0x04,0x47}},// CLOCKS_CONTROL
};

static struct regval_list sensor_qsxga_regs[] = {
//go to capture
{16, {0x09,0x8E}, {0x84,0x3C}},    // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_5_MAX_FRAME_CNT]
{8	,{0x84,0x3C}, {0xFF}},    // SEQ_STATE_CFG_5_MAX_FRAME_CNT
{8	,{0x84,0x04}, {0x02}},    // SEQ_CMD
{16, {0xff,0xff}, {0x01,0x2c}},//delay 300ms
};

//static struct regval_list sensor_1080p_regs[] = { 
//};

static struct regval_list sensor_720p_regs[] = {
{16,	{0x09,0x8E}, {0x10,0x00}},
{16,	{0xC8,0x5C}, {0x04,0x63}},    // CAM_CORE_A_MIN_FRAME_LENGTH_LINES
{16,	{0xC8,0x60}, {0x04,0x63}},     // CAM_CORE_A_BASE_FRAME_LENGTH_LINES
{16,	{0xC8,0x62}, {0x0D,0xB0}},    // CAM_CORE_A_MIN_LINE_LENGTH_PCLK
{16,	{0xC8,0x68}, {0x04,0x63}},     // CAM_CORE_A_FRAME_LENGTH_LINES	
{16,	{0xC8,0x6A}, {0x0D,0xB0}},    // CAM_CORE_A_LINE_LENGTH_PCK
{16,	{0xC8,0xAA}, {0x05,0x00}},    // CAM_OUTPUT_0_IMAGE_WIDTH
{16,	{0xC8,0xAC}, {0x02,0xD0}},   // CAM_OUTPUT_0_IMAGE_HEIGHT
{16,	{0xC8,0x94}, {0x07,0xF0}},     // CAM_CORE_B_MIN_FRAME_LENGTH_LINES
{16,	{0xC8,0x98}, {0x07,0xF0}},     // CAM_CORE_B_BASE_FRAME_LENGTH_LINES	
{16,	{0xC8,0x9A}, {0x0F,0x24}},     // CAM_CORE_B_MIN_LINE_LENGTH_PCLK
{16,	{0xC8,0xA0}, {0x07,0xF0}},     // CAM_CORE_B_FRAME_LENGTH_LINES
{16,	{0xC8,0xA2}, {0x0F,0x24}},     // CAM_CORE_B_LINE_LENGTH_PCK	
{16,	{0xA0,0x10}, {0x01,0x47}},     // FD_MIN_EXPECTED50HZ_FLICKER_PERIOD	
{16,	{0xA0,0x12}, {0x01,0x5B}},     // FD_MAX_EXPECTED50HZ_FLICKER_PERIOD
{16,	{0xA0,0x14}, {0x01,0x0F}},     // FD_MIN_EXPECTED60HZ_FLICKER_PERIOD
{16,	{0xA0,0x16}, {0x01,0x23}},     // FD_MAX_EXPECTED60HZ_FLICKER_PERIOD	
{16,	{0xA0,0x18}, {0x01,0x51}},     // FD_EXPECTED50HZ_FLICKER_PERIOD_IN_CONTEXT_A	
{16,	{0xA0,0x1C}, {0x01,0x18}},     // FD_EXPECTED60HZ_FLICKER_PERIOD_IN_CONTEXT_A	
{16,	{0xA0,0x1E}, {0x00,0xFE}},     // FD_EXPECTED60HZ_FLICKER_PERIOD_IN_CONTEXT_B
{8,		{0x84,0x04}, {0x06}},        // SEQ_CMD
{16, {0xff,0xff}, {0x01,0x2c}},//delay 300ms
};

//static struct regval_list sensor_svga_regs[] = {
//};

static struct regval_list sensor_vga_regs[] = {
//[Preview]
{16,	{0x09,0x8E}, {0x10,0x00}},
{16,	{0xC8,0x5C}, {0x04,0x23}},	//Min Frame Lines (A) = 1059
{16,	{0xC8,0x60}, {0x04,0x23}},	//Base Frame Lines (A) = 1059
{16,	{0xC8,0x62}, {0x0E,0x2A}},	//Min Line Length (A) = 3504
{16,	{0xC8,0x68}, {0x04,0x23}},	//Frame Lines (A) = 1059
{16,	{0xC8,0x6A}, {0x0E,0x2A}},	//Line Length (A) = 3504
{16,	{0xC8,0x70}, {0x00,0x14}},	//RX FIFO Watermark (A) = 20
{16,	{0xC8,0xAA}, {0x02,0x80}},	//Output_0 Image Width = 640
{16,	{0xC8,0xAC}, {0x01,0xe0}},	//Output_0 Image Height = 480
{16,	{0xC8,0x94}, {0x07,0xEF}},	//Min Frame Lines (B) = 2031			
{16,	{0xC8,0x98}, {0x07,0xEF}},	//Base Frame Lines (B) = 2031
{16,	{0xC8,0x9A}, {0x2C,0x50}},	//Min Line Length (B) = 14276
{16,	{0xC8,0xA0}, {0x07,0xEF}},	//Frame Lines (B) = 2031
{16,	{0xC8,0xA2}, {0x2C,0x50}},	//Line Length (B) = 14276
{16,	{0xA0,0x10}, {0x01,0x34}},	//fd_min_expected50hz_flicker_period = 303
{16,	{0xA0,0x12}, {0x01,0x48}},	//fd_max_expected50hz_flicker_period = 323
{16,	{0xA0,0x14}, {0x00,0xFF}},	//fd_min_expected60hz_flicker_period = 251
{16,	{0xA0,0x16}, {0x01,0x13}},	//fd_max_expected60hz_flicker_period = 271
{16,	{0xA0,0x18}, {0x01,0x3E}},	//fd_expected50hz_flicker_period (A) = 313
{16,	{0xA0,0x1C}, {0x01,0x09}},	//fd_expected60hz_flicker_period (A) = 261
{16,	{0xA0,0x1E}, {0x00,0x55}},	//fd_expected60hz_flicker_period (B) = 64

{16,	{0x09,0x8E}, {0x84,0x3C}},// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_5_MAX_FRAME_CNT]
{8,	  {0x84,0x3C}, {0x01}},// SEQ_STATE_CFG_5_MAX_FRAME_CNT
{8,	  {0x84,0x04}, {0x01}},// SEQ_CMD
{16, {0xff,0xff}, {0x01,0x2c}},//delay 300ms
};



/*
 * The white balance settings
 * Here only tune the R G B channel gain. 
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_auto_regs[] = {
//{16,	{0x09,0x8E}, {0x84,0x10}}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]
//{8,	  {0x84,0x10}, {0x02}},// SEQ_STATE_CFG_0_AWB
//{8,	  {0x84,0x18}, {0x02}},// SEQ_STATE_CFG_1_AWB
//{8,	  {0x84,0x20}, {0x02}},// SEQ_STATE_CFG_2_AWB
//{8,	  {0xAC,0x44}, {0x00}},// AWB_LEFT_CCM_POS_RANGE_LIMIT
//{8,	  {0xAC,0x45}, {0x7F}},// AWB_RIGHT_CCM_POS_RANGE_LIMIT
//{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD	
};

static struct regval_list sensor_wb_cloud_regs[] = {
//{16,	{0x09,0x8E}, {0x84,0x10}}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]
//{8,	  {0x84,0x10}, {0x01}},// SEQ_STATE_CFG_0_AWB
//{8,	  {0x84,0x18}, {0x01}},// SEQ_STATE_CFG_1_AWB
//{8,	  {0x84,0x20}, {0x01}},// SEQ_STATE_CFG_2_AWB
//{8,	  {0xAC,0x44}, {0x7F}},// AWB_LEFT_CCM_POS_RANGE_LIMIT
//{8,	  {0xAC,0x45}, {0x7F}},// AWB_RIGHT_CCM_POS_RANGE_LIMIT
//{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD
//{8,	  {0xAC,0x04}, {0x3D}},// AWB_PRE_AWB_R2G_RATIO
//{8,	  {0xAC,0x05}, {0x47}},// AWB_PRE_AWB_B2G_RATIO
};

static struct regval_list sensor_wb_daylight_regs[] = {
//	//tai yang guang
//{16,	{0x09,0x8E}, {0x84,0x10}}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]
//{8,	  {0x84,0x10}, {0x01}},// SEQ_STATE_CFG_0_AWB
//{8,	  {0x84,0x18}, {0x01}},// SEQ_STATE_CFG_1_AWB
//{8,	  {0x84,0x20}, {0x01}},// SEQ_STATE_CFG_2_AWB
//{8,	  {0xAC,0x44}, {0x79}},// AWB_LEFT_CCM_POS_RANGE_LIMIT
//{8,	  {0xAC,0x45}, {0x79}},// AWB_RIGHT_CCM_POS_RANGE_LIMIT
//{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD
//{8,	  {0xAC,0x04}, {0x41}},// AWB_PRE_AWB_R2G_RATIO
//{8,	  {0xAC,0x05}, {0x44}},// AWB_PRE_AWB_B2G_RATIO
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	//bai re guang

};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	//ri guang deng

};

static struct regval_list sensor_wb_tungsten_regs[] = {
	//wu si deng

};

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
//{16,	{0x09,0x8E}, {0xDC,0x38}}, 	// LOGICAL_ADDRESS_ACCESS [SYS_SELECT_FX]
//{8,	  {0xDC,0x38}, {0x00}},	// SYS_SELECT_FX
//{16,	{0xDC,0x02}, {0x30,0x2E}}, 	// SYS_ALGO
//{8,	  {0x84,0x04}, {0x06}},	// SEQ_CMD   
};

static struct regval_list sensor_colorfx_bw_regs[] = {
//{16,	{0x09,0x8E}, {0xDC,0x38}}, 	// LOGICAL_ADDRESS_ACCESS [SYS_SELECT_FX]
//{8,	  {0xDC,0x38}, {0x01}},// SYS_SELECT_FX
//{16,	{0xDC,0x02}, {0x30,0x6E}}, 	// SYS_ALGO
//{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
//{16,	{0x09,0x8E}, {0xDC,0x38}}, 	// LOGICAL_ADDRESS_ACCESS [SYS_SELECT_FX]
//{8,	  {0xDC,0x38}, {0x02}},// SYS_SELECT_FX
//{16,	{0xDC,0x02}, {0x30,0x6E}}, 	// SYS_ALGO
//{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD
};

static struct regval_list sensor_colorfx_negative_regs[] = {
//{16,	{0x09,0x8E}, {0xDC,0x38}}, 	// LOGICAL_ADDRESS_ACCESS [SYS_SELECT_FX]
//{8,	  {0xDC,0x38}, {0x03}},// SYS_SELECT_FX
//{16,	{0xDC,0x02}, {0x30,0x6E}}, 	// SYS_ALGO
//{8,	  {0x84,0x04}, {0x06}},// SEQ_CMD
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {

};

static struct regval_list sensor_colorfx_grass_green_regs[] = {

};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
//NULL
};

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
// Set the AE target
{8,	{0xA4,0x09}, {0x18}},//0x37 ,	// AE_RULE_BASE_TARGET
};

static struct regval_list sensor_ev_neg3_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x20}},//0x37 ,	// AE_RULE_BASE_TARGET
};

static struct regval_list sensor_ev_neg2_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x28}},//0x37 ,	// AE_RULE_BASE_TARGET
};

static struct regval_list sensor_ev_neg1_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x30}},// AE_RULE_BASE_TARGET
};                     

static struct regval_list sensor_ev_zero_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x38}},// AE_RULE_BASE_TARGET
};

static struct regval_list sensor_ev_pos1_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x40}},// AE_RULE_BASE_TARGET
};

static struct regval_list sensor_ev_pos2_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x48}},// AE_RULE_BASE_TARGET
};

static struct regval_list sensor_ev_pos3_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x50}},// AE_RULE_BASE_TARGET
};

static struct regval_list sensor_ev_pos4_regs[] = {
// Set the AE target
{8,	{0xA4,0x09}, {0x58}},// AE_RULE_BASE_TARGET
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	//YCbYCr
	{16,	{0x33,0x2e}, {0x00,0x02}},
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	//YCrYCb
	{16,	{0x33,0x2e}, {0x00,0x03}},
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	//CrYCbY
	{16,	{0x33,0x2e}, {0x00,0x01}},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	//CbYCrY
	{16,	{0x33,0x2e}, {0x00,0x00}},
};

static struct regval_list sensor_fmt_raw[] = {
	
};



/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read_word(struct v4l2_subdev *sd, unsigned char *reg,
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


static int sensor_write_word(struct v4l2_subdev *sd, unsigned char *reg,
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
	msg.len = 4;
	msg.buf = data;

	
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
		csi_dev_err("sensor_write error!\n");
	}
	return ret;
}

static int sensor_write_byte(struct v4l2_subdev *sd, unsigned char *reg,
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
	msg.len = 3;
	msg.buf = data;
	
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
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
	
	if (size == 0)
		return -EINVAL;
		
	for(i = 0; i < size ; i++)
	{
		if(vals->bit==16) {
			if(vals->reg_num[0]==0xff && vals->reg_num[1]==0xff) {
					mdelay(vals->value[0]*256 + vals->value[1]);
			}
			else {
				ret = sensor_write_word(sd, vals->reg_num, vals->value);
				if (ret < 0)
				{
					csi_dev_err("sensor_write_err!\n");
					return ret;
				}
			}
		} else if (vals->bit==8) {
			ret = sensor_write_byte(sd, vals->reg_num, vals->value);
			if (ret < 0)
			{
				csi_dev_err("sensor_write_err!\n");
				return ret;
			}
		}
		
		vals++;
		
		udelay(100);
	}

	return 0;
}

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
 * Stuff that knows about the sensor.
 */
 
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
  //make sure that no device can access i2c bus during sensor initial or power down
  //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
  i2c_lock_adapter(client->adapter);

  //insure that clk_disable() and clk_enable() are called in pair 
  //when calling CSI_SUBDEV_STBY_ON/OFF and CSI_SUBDEV_PWR_ON/OFF  
  switch(on)
	{
		case CSI_SUBDEV_STBY_ON:
			csi_dev_dbg("CSI_SUBDEV_STBY_ON\n");
			//reset off io
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(10);
			//standby on io
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_ON);
			mdelay(100);
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_OFF);
			mdelay(100);
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_ON);
			mdelay(100);
			//inactive mclk after stadby in
			clk_disable(dev->csi_module_clk);
			//reset on io
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
			mdelay(10);
			break;
		case CSI_SUBDEV_STBY_OFF:
			csi_dev_dbg("CSI_SUBDEV_STBY_OFF\n");
			//active mclk before stadby out
			clk_enable(dev->csi_module_clk);
			mdelay(10);
			//standby off io
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_OFF);
			mdelay(10);
			//reset off io
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(10);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
			mdelay(100);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(100);
			break;
		case CSI_SUBDEV_PWR_ON:
			csi_dev_dbg("CSI_SUBDEV_PWR_ON\n");
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
			mdelay(10);
			if(dev->dvdd) {
				regulator_enable(dev->dvdd);
				mdelay(10);
			}
			if(dev->avdd) {
				regulator_enable(dev->avdd);
				mdelay(10);
			}
			if(dev->iovdd) {
				regulator_enable(dev->iovdd);
				mdelay(10);
			}
			//standby off io
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_OFF);
			mdelay(10);
			//reset after power on
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(10);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
			mdelay(100);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(100);
			break;
		case CSI_SUBDEV_PWR_OFF:
			csi_dev_dbg("CSI_SUBDEV_PWR_OFF\n");
			//standby and reset io
			csi_gpio_write(sd,&dev->standby_io,CSI_STBY_ON);
			mdelay(100);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_ON);
			mdelay(100);
			//power supply off
			if(dev->iovdd) {
				regulator_disable(dev->iovdd);
				mdelay(10);
			}
			if(dev->avdd) {
				regulator_disable(dev->avdd);
				mdelay(10);
			}
			if(dev->dvdd) {
				regulator_disable(dev->dvdd);
				mdelay(10);	
			}
			csi_gpio_write(sd,&dev->power_io,CSI_PWR_OFF);
			mdelay(10);
			//inactive mclk after power off
			clk_disable(dev->csi_module_clk);
			//set the io to hi-z
			csi_gpio_set_status(sd,&dev->reset_io,0);//set the gpio to input
			csi_gpio_set_status(sd,&dev->standby_io,0);//set the gpio to input
			break;
		default:
			return -EINVAL;
	}		

	//remember to unlock i2c adapter, so the device can access the i2c bus again
	i2c_unlock_adapter(client->adapter);	
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
			mdelay(100);
			csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);
			mdelay(100);
			break;
		default:
			return -EINVAL;
	}
		
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret;
	struct regval_list regs;

	printk("[MT9P111]sensor_detect\n");
	
	regs.reg_num[0] = 0x00;
	regs.reg_num[1] = 0x00;
	ret = sensor_read_word(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_detect!\n");
		return ret;
	}
	
	printk("chip id is %x%x\n",regs.value[0],regs.value[1]);
	
	if(regs.value[0] != 0x28 && regs.value[1] != 0x80)
		return -ENODEV;
	
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	csi_dev_dbg("sensor_init\n");
	
	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		csi_dev_err("chip found is not an target chip.\n");
		return ret;
	}
	
	return sensor_write_array(sd, sensor_default_regs , ARRAY_SIZE(sensor_default_regs));
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
	
			csi_dev_dbg("ccm_info.mclk=%x\n ",info->ccm_info->mclk);
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
			
			csi_dev_dbg("ccm_info.mclk=%x\n ",info->ccm_info->mclk);
			csi_dev_dbg("ccm_info.vref=%x\n ",info->ccm_info->vref);
			csi_dev_dbg("ccm_info.href=%x\n ",info->ccm_info->href);
			csi_dev_dbg("ccm_info.clock=%x\n ",info->ccm_info->clock);
			csi_dev_dbg("ccm_info.iocfg=%x\n ",info->ccm_info->iocfg);
			
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
	{
		.desc		= "Raw RGB Bayer",
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,//linux-3.0
		.regs 		= sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp		= 1
	},
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
	/* qsxga: 2590*1944 */
	{
		.width			= QSXGA_WIDTH,
		.height 		= QSXGA_HEIGHT,
		.regs			  = sensor_qsxga_regs,
		.regs_size	= ARRAY_SIZE(sensor_qsxga_regs),
		.set_size		= NULL,
	},
	/* 720p */
	{
		.width			= P720_WIDTH,
		.height			= P720_HEIGHT,
		.regs 			= sensor_720p_regs,
		.regs_size	= ARRAY_SIZE(sensor_720p_regs),
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
	csi_dev_dbg("sensor_try_fmt_internal\n");
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
	csi_dev_dbg("sensor_s_fmt\n");
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;
	
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
	
	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);
	
	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	
	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{	
	struct v4l2_captureparm *cp = &parms->parm.capture;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = SENSOR_FRAME_RATE;
	
	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}


/* 
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
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
//	case V4L2_CID_EXPOSURE_AUTO:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_DO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 5, 1, 0);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 0);
//	case V4L2_CID_CAMERA_FLASH_MODE:
//	  return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{	
	return -EINVAL;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{	
	return -EINVAL;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	
	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}
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
	mdelay(10);
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
	mdelay(10);
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
	mdelay(10);
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
	mdelay(10);
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
	
	if (value == V4L2_WB_AUTO) {
		ret = sensor_s_autowb(sd, 1);
		return ret;
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
	
	mdelay(10);
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
	
	mdelay(10);
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

	printk("[MT9P111]sensor_probe\n");
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &sensor_ops);

	info->fmt = &sensor_formats[0];
	info->ccm_info = &ccm_info_con;
	
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp = 0;
	info->autoexp = 0;
	info->autowb = 1;
	info->wb = 0;
	info->clrfx = 0;
//	info->clkrc = 1;	/* 30fps */

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
	{ "mt9p111", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

//linux-3.0
static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
	.name = "mt9p111",
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
	printk("[MT9P111]init_sensor\n");
	return i2c_add_driver(&sensor_driver);
}

static __exit void exit_sensor(void)
{
  i2c_del_driver(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
