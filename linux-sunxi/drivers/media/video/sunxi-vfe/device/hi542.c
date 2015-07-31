/*
 * A V4L2 driver for HI542 Raw cameras.
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
#include <media/v4l2-mediabus.h>
#include <linux/io.h>


#include "camera.h"


MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for HI542 Raw sensors");
MODULE_LICENSE("GPL");

//for internel driver debug
#define DEV_DBG_EN      1 
#if(DEV_DBG_EN == 1)    
#define vfe_dev_dbg(x,arg...) printk("[HI542 Raw]"x,##arg)
#else
#define vfe_dev_dbg(x,arg...) 
#endif
#define vfe_dev_err(x,arg...) printk("[HI542 Raw]"x,##arg)
#define vfe_dev_print(x,arg...) printk("[HI542 Raw]"x,##arg)

#define LOG_ERR_RET(x)  { \
                          int ret;  \
                          ret = x; \
                          if(ret < 0) {\
                            vfe_dev_err("error at %s\n",__func__);  \
                            return ret; \
                          } \
                        }

//define module timing
#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW   
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR  0xB1



//define the voltage level of control signal
#define CSI_STBY_ON     1
#define CSI_STBY_OFF    0
#define CSI_RST_ON      1
#define CSI_RST_OFF     0
#define CSI_PWR_ON      1
#define CSI_PWR_OFF     0
#define CSI_AF_PWR_ON   1
#define CSI_AF_PWR_OFF  0
#define regval_list reg_list_a16_d8


#define REG_TERM 0xfffe
#define VAL_TERM 0xfe
#define REG_DLY  0xffff

/*
 * Our nominal (default) frame rate.
 */
#ifdef FPGA
#define SENSOR_FRAME_RATE 15
#else
#define SENSOR_FRAME_RATE 30
#endif

/*
 * The hi542 i2c address
 */
//#define I2C_ADDR 0x6c
#define HI542_WRITE_ADDR (0x40)
#define HI542_READ_ADDR  (0x41)

//static struct delayed_work sensor_s_ae_ratio_work;
static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */

struct cfg_array { /* coming later */
	struct regval_list * regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
  return container_of(sd, struct sensor_info, sd);
}


/*
 * The default register settings
 *
 */


static struct regval_list sensor_default_regs[] = 
{
/*
      //  {0x0001,0x02},			 
{0x0001,0x01},	  
{0x0001,0x00},
{0x0001,0x01},
		
	{0x0010,0x00},  
	{0x0011,0x90},  
	{0x0012,0x08},  
	{0x0013,0x00},  
	{0x0020,0x00},  
	{0x0021,0x00},  
	{0x0022,0x00},  
	{0x0023,0x00},  
	{0x0024,0x07},  
	{0x0025,0xA8},  
	{0x0026,0x0A},  
	{0x0027,0xB0},    
	{0x0038,0x02},  
	{0x0039,0x2C},  
	{0x003A,0x02},  
	{0x003B,0x2C},  
	{0x003C,0x00},  
	{0x003D,0x0C},  
	{0x003E,0x00},  
	{0x003F,0x0C},  
	{0x0040,0x00},  //Hblank H
	{0x0041,0x34},  //2E} Hblank L
	{0x0042,0x00},  
	{0x0043,0x0C},  
	{0x0045,0x07},  
	{0x0046,0x01},  
	{0x0047,0xD0},  
	{0x004A,0x02},   
	{0x004B,0xD8},    
	{0x004C,0x05},  
	{0x004D,0x08},  
	{0x0050,0x00},  
	{0x0052,0x10},  
	{0x0053,0x10},  
	{0x0054,0x10},  
	{0x0055,0x08},  
	{0x0056,0x80},  
	{0x0057,0x08},  
	{0x0058,0x08},  
	{0x0059,0x08},  
	{0x005A,0x08},  
	{0x005B,0x02},  
	{0x0070,0x03},	//EMI OFF
	{0x0080,0xC0},  
	{0x0081,0x01},  //09},//0B},BLC scheme
	{0x0082,0x23},  
	{0x0083,0x00},  
	{0x0084,0x30},  
	{0x0085,0x00},  
	{0x0086,0x00},  
	{0x008C,0x02},  
	{0x008D,0xFA},  
	{0x0090,0x0b},  
	{0x00A0,0x0f},  //0C},//0B},RAMP DC OFFSET
	{0x00A1,0x00},  
	{0x00A2,0x00},  
	{0x00A3,0x00},  
	{0x00A4,0xFF},  
	{0x00A5,0x00},  
	{0x00A6,0x00},  
	{0x00A7,0xa6},  
	{0x00A8,0x7F},  
	{0x00A9,0x7F},  
	{0x00AA,0x7F},  
	{0x00B4,0x00},  //08},BLC offset
	{0x00B5,0x00},  //08},
	{0x00B6,0x02},  //07},
	{0x00B7,0x01},  //07},
	{0x00D4,0x00},  
	{0x00D5,0xaa},  //a9},RAMP T1
	{0x00D6,0x01},  
	{0x00D7,0xc9},  
	{0x00D8,0x05},  
	{0x00D9,0x59},  
	{0x00DA,0x00},  
	{0x00DB,0xb0},  
	{0x00DC,0x01},  
	{0x00DD,0xc9},  //c5},
	{0x0119,0x00},  
	{0x011A,0x00},  
	{0x011B,0x00},  
	{0x011C,0x1F},  
	{0x011D,0xFF},  
	{0x011E,0xFF},  
	{0x011F,0xFF},  
	{0x0115,0x00},  
	{0x0116,0x16},  
	{0x0117,0x27},  
	{0x0118,0xE0},  
	{0x012A,0xFF},  
	{0x012B,0x00},  
	{0x0129,0x40},  
	{0x0210,0x00},  
	{0x0212,0x00},  
	{0x0213,0x00},  
	{0x0216,0x00},  
	{0x0217,0x40},  
	{0x0218,0x00},  
	{0x0219,0x33},  //66},Pixel bias
	{0x021A,0x15},  //15},
	{0x021B,0x55},  
	{0x021C,0x85},  
	{0x021D,0xFF},  
	{0x021E,0x01},  
	{0x021F,0x00},  
	{0x0220,0x02},  
	{0x0221,0x00},  
	{0x0222,0xA0},  
	{0x0223,0x2D},  
	{0x0224,0x24},  
	{0x0225,0x00},  
	{0x0226,0x3F},  
	{0x0227,0x0A},  
	{0x0228,0x5C},  
	{0x0229,0x2d},  //41},//00},//2C},RAMP swing range
	{0x022A,0x04},  
	{0x022B,0x9f},  
	{0x022C,0x01},  
	{0x022D,0x23},  
	{0x0232,0x10},  
	{0x0237,0x00},  
	{0x0238,0x00},  
	{0x0239,0xA5},  
	{0x023A,0x20},  
	{0x023B,0x00},  
	{0x023C,0x22},  
	{0x023E,0x00},  
	{0x023F,0x80},  
	{0x0240,0x04},  
	{0x0241,0x07},  
	{0x0242,0x00},  
	{0x0243,0x01},  
	{0x0244,0x80},  
	{0x0245,0xE0},  
	{0x0246,0x00},  
	{0x0247,0x00},  
	{0x024A,0x00},  
	{0x024B,0x14},  
	{0x024D,0x00},  
	{0x024E,0x03},  
	{0x024F,0x00},  
	{0x0250,0x53},  
	{0x0251,0x00},  
	{0x0252,0x07},  
	{0x0253,0x00},  
	{0x0254,0x4F},  
	{0x0255,0x00},  
	{0x0256,0x07},  
	{0x0257,0x00},  
	{0x0258,0x4F},  
	{0x0259,0x0C},  
	{0x025A,0x0C},  
	{0x025B,0x0C},  
	{0x026C,0x00},  
	{0x026D,0x09},  
	{0x026E,0x00},  
	{0x026F,0x4B},  
	{0x0270,0x00},  
	{0x0271,0x09},  
	{0x0272,0x00},  
	{0x0273,0x4B},  
	{0x0274,0x00},  
	{0x0275,0x09},  
	{0x0276,0x00},  
	{0x0277,0x4B},  
	{0x0278,0x00},  
	{0x0279,0x01},  
	{0x027A,0x00},  
	{0x027B,0x55},  
	{0x027C,0x00},  
	{0x027D,0x00},  
	{0x027E,0x05},  
	{0x027F,0x5E},  
	{0x0280,0x00},  
	{0x0281,0x03},  
	{0x0282,0x00},  
	{0x0283,0x45},  
	{0x0284,0x00},  
	{0x0285,0x03},  
	{0x0286,0x00},  
	{0x0287,0x45},  
	{0x0288,0x05},  
	{0x0289,0x5c},  
	{0x028A,0x05},  
	{0x028B,0x60},  
	{0x02A0,0x01},  
	{0x02A1,0xe0},  
	{0x02A2,0x02},  
	{0x02A3,0x22},  
	{0x02A4,0x05},  
	{0x02A5,0x5C},  
	{0x02A6,0x05},  
	{0x02A7,0x60},  
	{0x02A8,0x05},  
	{0x02A9,0x5C},  
	{0x02AA,0x05},  
	{0x02AB,0x60},  
	{0x02D2,0x0F},  
	{0x02DB,0x00},  
	{0x02DC,0x00},  
	{0x02DD,0x00},  
	{0x02DE,0x0C},  
	{0x02DF,0x00},  
	{0x02E0,0x04},  
	{0x02E1,0x00},  
	{0x02E2,0x00},  
	{0x02E3,0x00},  
	{0x02E4,0x0F},  
	{0x02F0,0x05},  
	{0x02F1,0x05},  
	{0x0310,0x00},  
	{0x0311,0x01},  
	{0x0312,0x05},  
	{0x0313,0x5A},  
	{0x0314,0x00},  
	{0x0315,0x01},  
	{0x0316,0x05},  
	{0x0317,0x5A},  
	{0x0318,0x00},  
	{0x0319,0x05},  
	{0x031A,0x00},  
	{0x031B,0x2F},  
	{0x031C,0x00},  
	{0x031D,0x05},  
	{0x031E,0x00},  
	{0x031F,0x2F},  
	{0x0320,0x00},  
	{0x0321,0xAB},  
	{0x0322,0x02},  
	{0x0323,0x55},  
	{0x0324,0x00},  
	{0x0325,0xAB},  
	{0x0326,0x02},  
	{0x0327,0x55},  
	{0x0328,0x00},  
	{0x0329,0x01},  
	{0x032A,0x00},  
	{0x032B,0x10},  
	{0x032C,0x00},  
	{0x032D,0x01},  
	{0x032E,0x00},  
	{0x032F,0x10},  
	{0x0330,0x00},  
	{0x0331,0x02},  
	{0x0332,0x00},  
	{0x0333,0x2e},  
	{0x0334,0x00},  
	{0x0335,0x02},  
	{0x0336,0x00},  
	{0x0337,0x2e},  
	{0x0358,0x00},  
	{0x0359,0x46},  
	{0x035A,0x05},  
	{0x035B,0x59},  
	{0x035C,0x00},  
	{0x035D,0x46},  
	{0x035E,0x05},  
	{0x035F,0x59},  
	{0x0360,0x00},  
	{0x0361,0x46},  
	{0x0362,0x00},  
	{0x0363,0xa4},  //a2},Black sun
	{0x0364,0x00},  
	{0x0365,0x46},  
	{0x0366,0x00},  
	{0x0367,0xa4},  //a2},Black sun
	{0x0368,0x00},  
	{0x0369,0x46},  
	{0x036A,0x00},  
	{0x036B,0xa6},  //a9},S2 off
	{0x036C,0x00},  
	{0x036D,0x46},  
	{0x036E,0x00},  
	{0x036F,0xa6},  //a9},S2 off
	{0x0370,0x00},  
	{0x0371,0xb0},  
	{0x0372,0x05},  
	{0x0373,0x59},  
	{0x0374,0x00},  
	{0x0375,0xb0},  
	{0x0376,0x05},  
	{0x0377,0x59},  
	{0x0378,0x00},  
	{0x0379,0x45},  
	{0x037A,0x00},  
	{0x037B,0xAA},  
	{0x037C,0x00},  
	{0x037D,0x99},  
	{0x037E,0x01},  
	{0x037F,0xAE},  
	{0x0380,0x01},  
	{0x0381,0xB1},  
	{0x0382,0x02},  
	{0x0383,0x56},  
	{0x0384,0x05},  
	{0x0385,0x6D},  
	{0x0386,0x00},  
	{0x0387,0xDC},  
	{0x03A0,0x05},  
	{0x03A1,0x5E},  
	{0x03A2,0x05},  
	{0x03A3,0x62},  
	{0x03A4,0x01},  
	{0x03A5,0xc9},  
	{0x03A6,0x01},  
	{0x03A7,0x27},  
	{0x03A8,0x05},  
	{0x03A9,0x59},  
	{0x03AA,0x02},  
	{0x03AB,0x55},  
	{0x03AC,0x01},  
	{0x03AD,0xc5},  
	{0x03AE,0x01},  
	{0x03AF,0x27},  
	{0x03B0,0x05},  
	{0x03B1,0x55},  
	{0x03B2,0x02},  
	{0x03B3,0x55},  
	{0x03B4,0x00},  
	{0x03B5,0x0A},  
	{0x03D0,0xee},  
	{0x03D1,0x15},  
	{0x03D2,0xb0},  
	{0x03D3,0x08},  
	{0x03D4,0x18},  //08},LDO OUTPUT 
	{0x03D5,0x44},  
	{0x03D6,0x54},  
	{0x03D7,0x56},  
	{0x03D8,0x44},  
	{0x03D9,0x06},  
	{0x0500,0x18},  
	{0x0580,0x01},  
	{0x0581,0x00},  
	{0x0582,0x80},  
	{0x0583,0x00},  
	{0x0584,0x80},  
	{0x0585,0x00},  
	{0x0586,0x80},  
	{0x0587,0x00},  
	{0x0588,0x80},  
	{0x0589,0x00},  
	{0x058A,0x80},  
	{0x05A0,0x01},  
	{0x05B0,0x01},  
	{0x05C2,0x00},  
	{0x05C3,0x00},  
	{0x0080,0xC7},  
	{0x0119,0x00},  
	{0x011A,0x15},  
	{0x011B,0xC0},  
	{0x0115,0x00},  
	{0x0116,0x2A},  
	{0x0117,0x4C},  
	{0x0118,0x20},  
	{0x0223,0xED},  
	{0x0224,0xE4},  
	{0x0225,0x09},  
	{0x0226,0x36},  
	{0x023E,0x80},  
	{0x05B0,0x00},  
	{0x03D0,0xe9},  
	{0x03D1,0x75},  
	{0x03D2,0xAC},  
	{0x0800,0x0F},  //07},//0F},EMI disable
	{0x0801,0x08},  
	{0x0802,0x00},  //04},//00},apb clock speed down
	
	{0x0010,0x05},  
	{0x0012,0x00},  
	{0x0013,0x00},  
	{0x0024,0x07},  
	{0x0025,0xA8},  
	{0x0026,0x0A},  
	{0x0027,0x30},  
	{0x0030,0x00},  
	{0x0031,0xFF},  
	{0x0032,0x06},  
	{0x0033,0xB0},  
	{0x0034,0x02},  
	{0x0035,0xD8},  
	{0x003A,0x00},  
	{0x003B,0x2E},  
	{0x004A,0x03},  
	{0x004B,0xC8},  
	{0x004C,0x05},  
	{0x004D,0x08},  
	{0x0C98,0x05},  
	{0x0C99,0x5E},  
	{0x0C9A,0x05},  
	{0x0C9B,0x62},  
	{0x0500,0x18},  
	{0x05A0,0x01},  
	{0x0084,0x30},  //10},BLC control
	{0x008D,0xFF},  
	{0x0090,0x02},  //0b},BLC defect pixel th
	{0x00A7,0x80},  //FF},
	{0x021A,0x05},  
	{0x022B,0xb0},  //f0},RAMP filter
	{0x0232,0x37},  //17},black sun enable
	{0x0010,0x01},  //01},
	{0x0740,0x1A},  
	{0x0742,0x1A},  
	{0x0743,0x1A},  
	{0x0744,0x1A},  
	{0x0745,0x04},  
	{0x0746,0x32},  
	{0x0747,0x05},  
	{0x0748,0x01},  
	{0x0749,0x90},  
	{0x074A,0x1A},  
	{0x074B,0xB1},							 
	{0x0500,0x19},  //0x19},//1b},LSC disable
	{0x0510,0x10},  
									   
	{0x0217,0x44},  //adaptive NCP on
	{0x0218,0x00},  //scn_sel
									   
	{0x02ac,0x00},  //outdoor on
	{0x02ad,0x00},  
	{0x02ae,0x00},  //outdoor off
	{0x02af,0x00},  
	{0x02b0,0x00},  //indoor on
	{0x02b1,0x00},  
	{0x02b2,0x00},  //indoor off
	{0x02b3,0x00},  
	{0x02b4,0x60},  //dark1 on
	{0x02b5,0x21},  
	{0x02b6,0x66},  //dark1 off
	{0x02b7,0x8a},  
									   
	{0x02c0,0x36},  //outdoor NCP en
	{0x02c1,0x36},  //indoor NCP en
	{0x02c2,0x36},  //dark1 NCP en
	{0x02c3,0x36},  //3f},//dark2 NCP disable
	{0x02c4,0xE4},  //outdoor NCP voltage
	{0x02c5,0xE4},  //indoor NCP voltage
	{0x02c6,0xE4},  //dark1 NCP voltage
	{0x02c7,0xdb},  //24},//dark2 NCP voltage
	
	//{0x061A,0x01},  
	//{0x061B,0x04},  
	//{0x061C,0x00},  
	//{0x061D,0x00},  
	//{0x061E,0x00},  
	//{0x061F,0x03},  
	//{0x0613,0x01},  
	//{0x0615,0x01},  
	//{0x0616,0x01},  
	//{0x0617,0x00},  
	//{0x0619,0x01},  
	//{0x0008,0x0F},  
	//{0x0630,0x05},  
	//{0x0631,0x08},  
	//{0x0632,0x03},  
	//{0x0633,0xC8},  
	//{0x0663,0x05},  //0a},trail time 
	//{0x0660,0x03},  
										
	//{0x0119,0x00},   
	//{0x011A,0x2b},   
	//{0x011B,0x80},   
	//									 },  
	//{0x0010,0x00},   
	//{0x0011,0x00},   
	//{0x0500,0x11},   
	//									 
	//{0x0630,0x0A},   
	//{0x0631,0x30},   
	//{0x0632,0x07},   
	//{0x0633,0xA8},   
	
	{0x0001,0x00},	//PWRCTLB  
*/

	
	//{0x0001,0x02);//  sw reset			 
	{0x0001,0x01}, //sleep
	
	{0x03d4,0x28},
	{REG_DLY,10},
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	

	{0x0011,0x93}, 
  
	{0x0020,0x00}, 
	{0x0021,0x00}, 
	{0x0022,0x00}, 
	{0x0023,0x00}, 
	

	
	{0x0038,0x02}, 
	{0x0039,0x2C},
	
	
	{0x003C,0x00}, 
	{0x003D,0x0C}, 
	{0x003E,0x00}, 
	{0x003F,0x0C}, 
	{0x0040,0x00}, //Hblank H
	{0x0041,0x35}, //2E} Hblank L
//changed by zhiqiang original 35

	{0x0042,0x00}, 
	{0x0043,0x14}, 
	{0x0045,0x07}, 
	{0x0046,0x01}, 
	{0x0047,0xD0}, 
	
	//{0x004A,0x02},  
	//{0x004B,0xD8},   
	//{0x004C,0x05}, 
	//{0x004D,0x08}, 

	{0x0050,0x00}, 
	{0x0052,0x10}, 
	{0x0053,0x10}, 
	{0x0054,0x10}, 
	{0x0055,0x08}, 
	{0x0056,0x80}, 
	{0x0057,0x08}, 
	{0x0058,0x08}, 
	{0x0059,0x08}, 
	{0x005A,0x08}, 
	{0x005B,0x02}, 
	{0x0070,0x03},	//EMI OFF
	
	//{0x0080,0xC0}, 
	{0x0081,0x01}, //09},//0B},BLC scheme
	{0x0082,0x23}, 
	{0x0083,0x00}, 

	//{0x0084,0x30}, 
	{0x0085,0x00}, 
	{0x0086,0x00}, 
	{0x008C,0x02}, 
	//{0x008D,0xFA}, 
	//{0x0090,0x0b}, 
	{0x00A0,0x0f}, //0C},//0B},RAMP DC OFFSET
	{0x00A1,0x00}, 
	{0x00A2,0x00}, 
	{0x00A3,0x00}, 
	{0x00A4,0xFF}, 
	{0x00A5,0x00}, 
	{0x00A6,0x00}, 
	//{0x00A7,0xa6}, 
	{0x00A8,0x7F}, 
	{0x00A9,0x7F}, 
	{0x00AA,0x7F}, 
	{0x00B4,0x00}, //08},BLC offset
	{0x00B5,0x00}, //08},
	{0x00B6,0x02}, //07},
	{0x00B7,0x01}, //07},
	{0x00D4,0x00}, 
	{0x00D5,0xaa}, //a9},RAMP T1
	{0x00D6,0x01}, 
	{0x00D7,0xc9}, 
	{0x00D8,0x05}, 
	{0x00D9,0x59}, 
	{0x00DA,0x00}, 
	{0x00DB,0xb0}, 
	{0x00DC,0x01}, 
	{0x00DD,0xc9}, //c5},
	//{0x0119,0x00}, 
	//{0x011A,0x00}, 
	//{0x011B,0x00}, 
	{0x011C,0x1F}, 
	{0x011D,0xFF}, 
	{0x011E,0xFF}, 
	{0x011F,0xFF}, 
	
	//{0x0115,0x00}, 
	//{0x0116,0x16}, 
	//{0x0117,0x27}, 
	//{0x0118,0xE0}, 
	{0x012A,0xFF}, 
	{0x012B,0x00}, 
	{0x0129,0x40}, 
	{0x0210,0x00}, 
	{0x0212,0x00}, 
	{0x0213,0x00}, 
	{0x0216,0x00}, 
	//{0x0217,0x40}, 
	//{0x0218,0x00}, 
	{0x0219,0x33}, //66},Pixel bias
	//{0x021A,0x15}, //15},
	{0x021B,0x55}, 
	{0x021C,0x85}, 
	{0x021D,0xFF}, 
	{0x021E,0x01}, 
	{0x021F,0x00}, 
	{0x0220,0x02}, 
	{0x0221,0x00}, 
	{0x0222,0xA0}, 
	
	//{0x0223,0x2D}, 
	//{0x0224,0x24}, 
	//{0x0225,0x00}, 
	//{0x0226,0x3F}, 
	{0x0227,0x0A}, 
	{0x0228,0x5C}, 
	{0x0229,0x2d}, //41},//00},//2C},RAMP swing range
	{0x022A,0x04}, 
	
	//{0x022B,0x9f}, 
	{0x022C,0x01}, 
	{0x022D,0x23}, 
	//{0x0232,0x10}, 
	{0x0237,0x00}, 
	{0x0238,0x00}, 
	{0x0239,0xA5}, 
	{0x023A,0x20}, 
	{0x023B,0x00}, 
	{0x023C,0x22}, 
	
	//{0x023E,0x00}, 
	{0x023F,0x80}, 
	{0x0240,0x04}, 
	{0x0241,0x07}, 
	{0x0242,0x00}, 
	{0x0243,0x01}, 
	{0x0244,0x80}, 
	{0x0245,0xE0}, 
	{0x0246,0x00}, 
	{0x0247,0x00}, 
	{0x024A,0x00}, 
	{0x024B,0x14}, 
	{0x024D,0x00}, 
	{0x024E,0x03}, 
	{0x024F,0x00}, 
	{0x0250,0x53}, 
	{0x0251,0x00}, 
	{0x0252,0x07}, 
	{0x0253,0x00}, 
	{0x0254,0x4F}, 
	{0x0255,0x00}, 
	{0x0256,0x07}, 
	{0x0257,0x00}, 
	{0x0258,0x4F}, 
	{0x0259,0x0C}, 
	{0x025A,0x0C}, 
	{0x025B,0x0C}, 
	{0x026C,0x00}, 
	{0x026D,0x09}, 
	{0x026E,0x00}, 
	{0x026F,0x4B}, 
	{0x0270,0x00}, 
	{0x0271,0x09}, 
	{0x0272,0x00}, 
	{0x0273,0x4B}, 
	{0x0274,0x00}, 
	{0x0275,0x09}, 
	{0x0276,0x00}, 
	{0x0277,0x4B}, 
	{0x0278,0x00}, 
	{0x0279,0x01}, 
	{0x027A,0x00}, 
	{0x027B,0x55}, 
	{0x027C,0x00}, 
	{0x027D,0x00}, 
	{0x027E,0x05}, 
	{0x027F,0x5E}, 
	{0x0280,0x00}, 
	{0x0281,0x03}, 
	{0x0282,0x00}, 
	{0x0283,0x45}, 
	{0x0284,0x00}, 
	{0x0285,0x03}, 
	{0x0286,0x00}, 
	{0x0287,0x45}, 
	{0x0288,0x05}, 
	{0x0289,0x5c}, 
	{0x028A,0x05}, 
	{0x028B,0x60}, 
	{0x02A0,0x01}, 
	{0x02A1,0xe0}, 
	{0x02A2,0x02}, 
	{0x02A3,0x22}, 
	{0x02A4,0x05}, 
	{0x02A5,0x5C}, 
	{0x02A6,0x05}, 
	{0x02A7,0x60}, 
	{0x02A8,0x05}, 
	{0x02A9,0x5C}, 
	{0x02AA,0x05}, 
	{0x02AB,0x60}, 
	{0x02D2,0x0F}, 
	{0x02DB,0x00}, 
	{0x02DC,0x00}, 
	{0x02DD,0x00}, 
	{0x02DE,0x0C}, 
	{0x02DF,0x00}, 
	{0x02E0,0x04}, 
	{0x02E1,0x00}, 
	{0x02E2,0x00}, 
	{0x02E3,0x00}, 
	{0x02E4,0x0F}, 
	{0x02F0,0x05}, 
	{0x02F1,0x05}, 
	{0x0310,0x00}, 
	{0x0311,0x01}, 
	{0x0312,0x05}, 
	{0x0313,0x5A}, 
	{0x0314,0x00}, 
	{0x0315,0x01}, 
	{0x0316,0x05}, 
	{0x0317,0x5A}, 
	{0x0318,0x00}, 
	{0x0319,0x05}, 
	{0x031A,0x00}, 
	{0x031B,0x2F}, 
	{0x031C,0x00}, 
	{0x031D,0x05}, 
	{0x031E,0x00}, 
	{0x031F,0x2F}, 
	{0x0320,0x00}, 
	{0x0321,0xAB}, 
	{0x0322,0x02}, 
	{0x0323,0x55}, 
	{0x0324,0x00}, 
	{0x0325,0xAB}, 
	{0x0326,0x02}, 
	{0x0327,0x55}, 
	{0x0328,0x00}, 
	{0x0329,0x01}, 
	{0x032A,0x00}, 
	{0x032B,0x10}, 
	{0x032C,0x00}, 
	{0x032D,0x01}, 
	{0x032E,0x00}, 
	{0x032F,0x10}, 
	{0x0330,0x00}, 
	{0x0331,0x02}, 
	{0x0332,0x00}, 
	{0x0333,0x2e}, 
	{0x0334,0x00}, 
	{0x0335,0x02}, 
	{0x0336,0x00}, 
	{0x0337,0x2e}, 
	{0x0358,0x00}, 
	{0x0359,0x46}, 
	{0x035A,0x05}, 
	{0x035B,0x59}, 
	{0x035C,0x00}, 
	{0x035D,0x46}, 
	{0x035E,0x05}, 
	{0x035F,0x59}, 
	{0x0360,0x00}, 
	{0x0361,0x46}, 
	{0x0362,0x00}, 
	{0x0363,0xa4}, //a2},Black sun
	{0x0364,0x00}, 
	{0x0365,0x46}, 
	{0x0366,0x00}, 
	{0x0367,0xa4}, //a2},Black sun
	{0x0368,0x00}, 
	{0x0369,0x46}, 
	{0x036A,0x00}, 
	{0x036B,0xa6}, //a9},S2 off
	{0x036C,0x00}, 
	{0x036D,0x46}, 
	{0x036E,0x00}, 
	{0x036F,0xa6}, //a9},S2 off
	{0x0370,0x00}, 
	{0x0371,0xb0}, 
	{0x0372,0x05}, 
	{0x0373,0x59}, 
	{0x0374,0x00}, 
	{0x0375,0xb0}, 
	{0x0376,0x05}, 
	{0x0377,0x59}, 
	{0x0378,0x00}, 
	{0x0379,0x45}, 
	{0x037A,0x00}, 
	{0x037B,0xAA}, 
	{0x037C,0x00}, 
	{0x037D,0x99}, 
	{0x037E,0x01}, 
	{0x037F,0xAE}, 
	{0x0380,0x01}, 
	{0x0381,0xB1}, 
	{0x0382,0x02}, 
	{0x0383,0x56}, 
	{0x0384,0x05}, 
	{0x0385,0x6D}, 
	{0x0386,0x00}, 
	{0x0387,0xDC}, 
	{0x03A0,0x05}, 
	{0x03A1,0x5E}, 
	{0x03A2,0x05}, 
	{0x03A3,0x62}, 
	{0x03A4,0x01}, 
	{0x03A5,0xc9}, 
	{0x03A6,0x01}, 
	{0x03A7,0x27}, 
	{0x03A8,0x05}, 
	{0x03A9,0x59}, 
	{0x03AA,0x02}, 
	{0x03AB,0x55}, 
	{0x03AC,0x01}, 
	{0x03AD,0xc5}, 
	{0x03AE,0x01}, 
	{0x03AF,0x27}, 
	{0x03B0,0x05}, 
	{0x03B1,0x55}, 
	{0x03B2,0x02}, 
	{0x03B3,0x55}, 
	{0x03B4,0x00}, 
	{0x03B5,0x0A}, 
	
	//{0x03D0,0xee}, 
	//{0x03D1,0x15}, 
	//{0x03D2,0xb0}, 
	{0x03D3,0x08}, 
	//{0x03D4,0x18}, //08},LDO OUTPUT 
	{0x03D5,0x44}, 
	{0x03D6,0x51}, 
	{0x03D7,0x56}, 
	{0x03D8,0x44}, 
	{0x03D9,0x03},//06},24mA ->12mA /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

	//{0x0500,0x18}, 
	{0x0580,0x01}, 
	{0x0581,0x00}, 
	{0x0582,0x80}, 
	{0x0583,0x00}, 
	{0x0584,0x80}, 
	{0x0585,0x00}, 
	{0x0586,0x80}, 
	{0x0587,0x00}, 
	{0x0588,0x80}, 
	{0x0589,0x00}, 
	{0x058A,0x80}, 
	
	//{0x05A0,0x01}, 
	//{0x05B0,0x01}, 
	{0x05C2,0x00}, 
	{0x05C3,0x00}, 
	{0x0080,0xC7}, 
	{0x0119,0x00}, 
	{0x011A,0x15}, 
	{0x011B,0xC0}, 
/*
	{0x0115,0x00}, 
	{0x0116,0x2A}, 
	{0x0117,0x4C}, 
	{0x0118,0x20}, 
*/
	{0x0223,0xED}, 
	{0x0224,0xE4}, 
	{0x0225,0x09}, 
	{0x0226,0x36}, 
	{0x023E,0x80}, 
	{0x05B0,0x00}, 

	{0x03D0,0xe9}, 
	{0x03D1,0x75}, 
	{0x03D2,0xAC},//PLL reset disable 20120418 ryu add

	
	{0x0800,0x0F},//EMI disable
	{0x0801,0x08}, 
	{0x0802,0x00}, //04},//00},apb clock speed down
	//{0x0010,0x05}, 
	{0x0012,0x00}, 
	{0x0013,0x00}, 
	{0x0024,0x07}, 
	{0x0025,0xA8}, 
	{0x0026,0x0A}, 
	{0x0027,0x30}, 
	{0x0030,0x00}, 
	{0x0031,0x03}, 
	{0x0032,0x07}, 
	{0x0033,0xAC}, 
	{0x0034,0x03}, 
	{0x0035,0xD4}, 
	{0x003A,0x00}, 
	{0x003B,0x2E}, 
	{0x004A,0x03}, 
	{0x004B,0xD4}, 
	{0x004C,0x05}, 
	{0x004D,0x18}, 
	{0x0C98,0x05}, 
	{0x0C99,0x5E}, 
	{0x0C9A,0x05}, 
	{0x0C9B,0x62},
	
	//{0x0500,0x18}, 
	{0x05A0,0x01}, 
	{0x0084,0x30}, //10},BLC control
	{0x008D,0xFF}, 
	{0x0090,0x02}, //0b},BLC defect pixel th
	{0x00A7,0x80}, //FF},
	{0x021A,0x15}, 
	{0x022B,0xb0}, //f0},RAMP filter
	{0x0232,0x37}, //17},black sun enable
	{0x0010,0x41}, //01},
	{0x0740,0x1A}, 
	{0x0742,0x1A}, 
	{0x0743,0x1A}, 
	{0x0744,0x1A}, 
	{0x0745,0x04}, 
	{0x0746,0x32}, 
	{0x0747,0x05}, 
	{0x0748,0x01}, 
	{0x0749,0x90}, 
	{0x074A,0x1A}, 
	{0x074B,0xB1},							 
	{0x0500,0x19}, //0x19},//1b},LSC disable
	{0x0510,0x10}, 
									   
	{0x0217,0x44}, //adaptive NCP on
	{0x0218,0x00}, //scn_sel
									   
	{0x02ac,0x00}, //outdoor on
	{0x02ad,0x00}, 
	{0x02ae,0x00}, //outdoor off
	{0x02af,0x00}, 
	{0x02b0,0x00}, //indoor on
	{0x02b1,0x00}, 
	{0x02b2,0x00}, //indoor off
	{0x02b3,0x00}, 
	{0x02b4,0x60}, //dark1 on
	{0x02b5,0x21}, 
	{0x02b6,0x66}, //dark1 off
	{0x02b7,0x8a}, 
									   
	{0x02c0,0x36}, //outdoor NCP en
	{0x02c1,0x36}, //indoor NCP en
	{0x02c2,0x36}, //dark1 NCP en
	{0x02c3,0x36}, //3f},//dark2 NCP disable
	{0x02c4,0xE4}, //outdoor NCP voltage
	{0x02c5,0xE4}, //indoor NCP voltage
	{0x02c6,0xE4}, //dark1 NCP voltage
	{0x02c7,0xdb}, //24},//dark2 NCP voltage
	
	{0x0001,0x00},


	
};

//for capture                                                                         
static struct regval_list sensor_qsxga_regs[] = { //qsxga: 2592x1944@15fps
		{0x0001, 0x01}, 
		{0x0001, 0x00}, 			
		{0x0001, 0x01}, 
												  
		{0x0119, 0x00}, 
		{0x011A, 0x2b}, 
		{0x011B, 0x80}, 
										 
		{0x0010, 0x00}, 
		{0x0011, 0x00}, 
		{0x0500, 0x11}, 
										 
		{0x0001, 0x00}, 	
};

//static struct regval_list sensor_qxga_regs[] = { //qxga: 2048*1536
//  

//};                                      

//static struct regval_list sensor_uxga_regs[] = { //UXGA: 1600*1200
// 

//};

static struct regval_list sensor_sxga_regs[] = { //SXGA: 1280*960@30fps

		{0x0001, 0x01},
		{0x0001, 0x00},
		{0x0001, 0x01},

		{0x0010, 0x01},
		{0x0011, 0x00},
		{0x0040, 0x00},  //Hblank H
		{0x0041, 0x34},  //2E} Hblank L
		{0x0042, 0x00},  
		{0x0043, 0x0C},  
		{0x004A, 0x03},  
		{0x004B, 0xC8},  
		{0x004C, 0x05},  
		{0x004D, 0x08}, 
		{0x0500, 0x19},
										  
		{0x0119, 0x00},
		{0x011A, 0x15},
		{0x011B, 0xC0},
															  
		{0x0001, 0x00},	 
	 
};

//static struct regval_list sensor_xga_regs[] = { //XGA: 1024*768
//  
//  //{REG_TERM,VAL_TERM},
//};

//for video
//static struct regval_list sensor_1080p_regs[] = { //1080: 1920*1080@30fps
//
//};



static struct regval_list sensor_720p_regs[] = { //720: 1280*720@30fps
	{0x0001, 0x01},	  
	{0x0001, 0x00},
	{0x0001, 0x01},
	
	{0x0010, 0x41},
	{0x0011, 0x93},
	{0x0034, 0x03},
	{0x0035, 0xD4},
	{0x0040, 0x00},
	{0x0041, 0x35},

	{0x03D9,0x03},//06}, 24mA ->12mA /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0042, 0x00},
	{0x0043, 0x14},
	{0x0500, 0x19},



	{0x0011, 0x93}, //fixed frame setting turn off
	{0x0013, 0x40}, //fixed frame setting turn off




	//{,0x0010, 0x01},
	//{,0x0011, 0x00},
	//{,0x0120, 0x00}, //Fixed exposure time 15
	//{,0x0121, 0x55}, //Fixed exposure time 15
	//{,0x0122, 0x79}, //Fixed exposure time 15
	//{,0x0123, 0x01}, //Fixed exposure time 15
	
	{0x011C,0x1F}, // Max exposure time 
	{0x011D,0xFF}, // Max exposure time 
	{0x011E,0xFF}, // Max exposure time/
	{0x011F,0xFF}, // Max exposure time 
/*
	{,0x0115, 0x00}, //// manual exposure time
	{,0x0116, 0x2A}, //// manual exposure time
	{,0x0117, 0x4C}, //// manual exposure time
	{,0x0118, 0x20}, //// manual exposure time
*/
	{0x0001, 0x00},	


};

//static struct regval_list sensor_svga_regs[] = { //SVGA: 800*600
//

//};

//static struct regval_list sensor_vga_regs[] = { //VGA:  640*480
//  

//};

//misc
static struct regval_list sensor_oe_disable_regs[] = {
	//{0x3000,0x00},
	//{0x3001,0x00},
	//{0x3002,0x00},
  //{REG_TERM,VAL_TERM},
};

static struct regval_list sensor_oe_enable_regs[] = {
	//{0x3000,0x0f},
	//{0x3001,0xff},
  //{0x3002,0xe4},
  //{REG_TERM,VAL_TERM},
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */

static struct regval_list sensor_fmt_raw[] = {

  //{REG_TERM,VAL_TERM},
};

/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
int cci_read_a16_d8(struct i2c_client *client, unsigned short addr,
    unsigned char *value)
{
  unsigned char data[3];
  struct i2c_msg msg[2];
  int ret;

  data[0] = (addr&0xff00)>>8;
  data[1] = (addr&0x00ff);
  data[2] = 0xee;

  client->addr=(HI542_READ_ADDR>>1);
  /*
   * Send out the register address...
   */ 
  msg[0].addr = client->addr;
  msg[0].flags = 0;
  msg[0].len = 2;
  msg[0].buf = &data[0];
  /*
   * ...then read back the result.
   */
  msg[1].addr = client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 1;
  msg[1].buf = &data[2];
  
  ret = i2c_transfer(client->adapter, msg, 2);
  if (ret >= 0) {
    *value = data[2];
    ret = 0;
  } else {
    vfe_dev_err("%s error! slave = 0x%x, addr = 0x%4x, value = 0x%2x\n ",__func__, client->addr, addr,*value);
  }

  
  return ret;
}

int cci_write_a16_d8(struct i2c_client *client, unsigned short addr,
    unsigned char value)
{
  struct i2c_msg msg;
  unsigned char data[3];
  int ret;
   int retry = 3; 
	  
  data[0] = (addr&0xff00)>>8;
  data[1] = (addr&0x00ff);
  data[2] = value;
  
  client->addr=(HI542_WRITE_ADDR>>1);
/*
  client->addr=(HI542_WRITE_ADDR>>1);
  msg.addr = client->addr;
  msg.flags = 0;
  msg.len = 3;
  msg.buf = data;

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret >= 0) {
    ret = 0;
  } else {
    vfe_dev_err("%s error! slave = 0x%x, addr = 0x%4x, value = 0x%4x\n ",__func__, client->addr, addr,value);
  }
*/

    do {
        ret = i2c_master_send(client, data, 3);
        if (ret == 3) {
		  return ret; 
	}
        else {	
           vfe_dev_err("%s error! slave = 0x%x, addr = 0x%4x, value = 0x%4x\n ",__func__, client->addr, addr,value);
        }
        msleep(1); 
    } while ((retry --) > 0);  
  return -1;
}

static int sensor_read(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char *value)
{
	int ret=0;
	int cnt=0;
	
  struct i2c_client *client = v4l2_get_subdevdata(sd);
  ret = cci_read_a16_d8(client,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_read_a16_d8(client,reg,value);
  	cnt++;
  }
  if(cnt>0)
  	vfe_dev_dbg("sensor read retry=%d\n",cnt);
  
  return ret;
}

static int sensor_write(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char value)
{
	int ret=0;
	int cnt=0;
	
  struct i2c_client *client = v4l2_get_subdevdata(sd);
  
  ret = cci_write_a16_d8(client,reg,value);
  while(ret !=3&&cnt<2)
  {
  	ret = cci_write_a16_d8(client,reg,value);
  	cnt++;
  }
  if(cnt>=1)
  	vfe_dev_dbg("sensor write retry=%d\n",cnt);
  
  return ret;
}

/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *regs, int array_size)
{
	int i=0;
	
  if(!regs)
  	return -EINVAL;
  
  while(i<array_size)
  {
	if(regs->addr == REG_DLY) {
               msleep(regs->data);
   	 } 
   	 else 
	 {  
    		//printk("write 0x%x=0x%x\n", regs->addr, regs->data);
     	    sensor_write(sd, regs->addr, regs->data);
    	}
    	i++;
   	 regs++;
  }
  return 0;
}




/* 
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
/*
static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
    
  LOG_ERR_RET(sensor_read(sd, 0x3821, &rdval))
  
  rdval &= (1<<1);
  rdval >>= 1;
    
  *value = rdval;

  info->hflip = *value;
  return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  if(info->hflip == value)
    return 0;
    
  LOG_ERR_RET(sensor_read(sd, 0x3821, &rdval))
  
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
  
  LOG_ERR_RET(sensor_write(sd, 0x3821, rdval))
  
  mdelay(10);
  info->hflip = value;
  return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  LOG_ERR_RET(sensor_read(sd, 0x3820, &rdval))
  
  rdval &= (1<<1);  
  *value = rdval;
  rdval >>= 1;
  
  info->vflip = *value;
  return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  if(info->vflip == value)
    return 0;
  
  LOG_ERR_RET(sensor_read(sd, 0x3820, &rdval))

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

  LOG_ERR_RET(sensor_write(sd, 0x3820, rdval))
  
  mdelay(10);
  info->vflip = value;
  return 0;
}
*/

static int sensor_g_opclk(struct v4l2_subdev *sd, unsigned int *value)
{
	unsigned char pll_pre_devider,pll_main_devider,pll_mclk_devider,mclk_devider;

	sensor_read(sd, 0x03D1, &pll_pre_devider);
	pll_pre_devider=pll_pre_devider&0x0f;
	sensor_read(sd, 0x03D0, &pll_main_devider);	
        pll_main_devider=pll_main_devider&0x7f;
	sensor_read(sd, 0x03D2, &pll_mclk_devider);	   
        pll_mclk_devider=(pll_mclk_devider>>2)&0x07;
	     switch(pll_mclk_devider)
	    {
	    	case 0:
			mclk_devider = 1;
			break;
	    	case 1:
			mclk_devider = 2;
			break;
	    	case 2:
			mclk_devider = 4;
			break;	
	    	case 3:
			mclk_devider = 5;
			break;
	    	case 4:
			mclk_devider = 6;
			break;
	    	case 5:
			mclk_devider = 8;
			break;
	    	case 6:
			mclk_devider = 10;
			break;
		default:
			mclk_devider = 1;
	    }       
	*value = ((MCLK/(pll_pre_devider+1))*pll_main_devider)/mclk_devider;

	return 0;
}

static int sensor_g_exp_max_min(struct v4l2_subdev *sd)
{
	unsigned char expminlow,expminmid,expminhigh;
	unsigned char explow,expmidh,expmidl,exphigh;

	
	sensor_read(sd, 0x0119, &expminhigh);
	sensor_read(sd, 0x011A, &expminmid);
	sensor_read(sd, 0x011B, &expminlow);	
        vfe_dev_dbg("sensor_get_MIN_exposure = %d\n", expminhigh*256+expminmid*16+expminlow);

	sensor_read(sd, 0x011C, &exphigh);
	sensor_read(sd, 0x011D, &expmidh);
	sensor_read(sd, 0x011E, &expmidl);	
	sensor_read(sd, 0x011F, &explow);		
        vfe_dev_dbg("sensor_get_MAX_exposure = %d\n", exphigh*256*16+expmidh*256+expmidl*16+explow);
	return 0;
}



static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->exp;
	vfe_dev_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow,expmidh,expmidl,exphigh;
	struct sensor_info *info = to_state(sd);
	unsigned int opclk=0;
	unsigned int exp=0;
	
	sensor_g_exp_max_min(sd);

	if(info->exp == exp_val)
		return 0;
	vfe_dev_dbg("sensor_set_exposure = %d\n", exp_val);
	sensor_g_opclk(sd,&opclk);
	vfe_dev_dbg("sensor_g_opclk opclk= %d\n", opclk);
	vfe_dev_dbg("info->current_wins->vts= %d\n", info->current_wins->vts);
	//exp=(exp_val*(opclk/1000000)*16)/(info->current_wins->vts+130+52);
	exp = 0x100 * (2608+130+52);
	//exp = ((opclk/exp_val)/16) * (info->current_wins->vts+130+52);

	vfe_dev_dbg("sensor_set_exposure = %d\n", exp);
	if(exp>0x1fffffff)
		exp=0x1fffffff;
	

  
	exphigh = (unsigned char) ( (0xff000000&exp)>>24);
	expmidh  = (unsigned char) ( (0xff0000&exp)>>16);
	expmidl  = (unsigned char) ( (0x00ff00&exp)>>8);
	explow  = (unsigned char) ( (0x0000ff&exp) );
	
	sensor_write(sd, 0x0118, explow);
	sensor_write(sd, 0x0117, expmidl);
	sensor_write(sd, 0x0116, expmidh);
	sensor_write(sd, 0x0115, exphigh);	

	printk("hi542  sensor_set_exp = %d, Done!\n", exp_val);

	
	info->exp = exp_val;
	return 0;
}
static int sensor_g_GAIN_max_min(struct v4l2_subdev *sd)
{
	unsigned char maxgain,mingain;


	
	sensor_read(sd, 0x012B, &maxgain);
	sensor_read(sd, 0x012A, &mingain);
	
        vfe_dev_dbg("sensor_g_GAIN_max_min maxgain= %x,mingain=%x\n", maxgain,mingain);
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->gain;
	vfe_dev_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
        unsigned char iReg;

	sensor_g_GAIN_max_min(sd);
	vfe_dev_dbg("hi542 sensor_s_gain iGain :0x%x\n", gain_val); 

	//if(gain_val==0)
	//	gain_val =1;

	iReg =gain_val/16;

	if(iReg==0)
		iReg =1;
	iReg = 256/iReg - 32;

	sensor_write(sd, 0x0129, iReg);

	info->gain = gain_val;
	
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
/*	
	unsigned char rdval;
	
	ret=sensor_read(sd, 0x0100, &rdval);
	if(ret!=0)
		return ret;
	
	if(on_off==CSI_STBY_ON)//sw stby on
	{
		ret=sensor_write(sd, 0x0100, rdval&0xfe);
	}
	else//sw stby off
	{
		ret=sensor_write(sd, 0x0100, rdval|0x01);
	}
*/
	return ret;
}

static int sensor_set_power_sleep(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
	
	unsigned char rdval;
	
	ret=sensor_read(sd, 0x0001, &rdval);
	if(ret!=0)
		return ret;
	
	if(on_off==ON)
	{
		ret=sensor_write(sd, 0x0001, rdval|0x01);
	}
	else
	{
		ret=sensor_write(sd, 0x0001, rdval&0xfe);
	}

	return ret;
}

static int sensor_set_pll_enable(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
	
	unsigned char rdval;
	
	vfe_dev_dbg("sensor_set_pll_enable!\n");
	ret=sensor_read(sd, 0x0800, &rdval);
	if(ret!=0)
		return ret;
	
	if(on_off==ON)
	{
		ret=sensor_write(sd, 0x0800, rdval|0x04);
	}
	else
	{
		ret=sensor_write(sd, 0x0800, rdval&0xfb);
	}

	return ret;
}



enum hi542_clk_div {
  CLK_DIV_BY_1        = 0,
  CLK_DIV_BY_1_dot_5  = 1,
  CLK_DIV_BY_2        = 2,
  CLK_DIV_BY_2_dot_5  = 3,
  CLK_DIV_BY_3        = 4,
  CLK_DIV_BY_4        = 5,
  CLK_DIV_BY_6        = 6,
  CLK_DIV_BY_8        = 7,
};
int frame_rate_relat[] = {120,80,60,48,40,30,20,15};

static int sensor_s_framerate(struct v4l2_subdev *sd, unsigned int frame_rate)
{
  int set_clk_div;
	//struct sensor_info *info = to_state(sd);

    switch(frame_rate)
    {
    	case 120:
			set_clk_div = CLK_DIV_BY_1;
			break;
		case 80:
			set_clk_div = CLK_DIV_BY_1_dot_5;
			break;
		case 60:
			set_clk_div = CLK_DIV_BY_2;
			break;
		case 48:
			set_clk_div = CLK_DIV_BY_2_dot_5;
			break;
		case 40:
			set_clk_div = CLK_DIV_BY_3;
			break;
		case 30:
			set_clk_div = CLK_DIV_BY_4;
			break;
		case 20:
			set_clk_div = CLK_DIV_BY_6;
			break;
		case 15:
			set_clk_div = CLK_DIV_BY_8;
			break;
		default:
			set_clk_div = CLK_DIV_BY_1;
			break;		
    }

	//printk("set_clk_div = %d\n",set_clk_div);

	//sensor_write(sd,0x3012,set_clk_div);
	
	return 0;
}



/*
 * Stuff that knows about the sensor.
 */
 
static int sensor_power(struct v4l2_subdev *sd, int on)
{
  struct i2c_client *client = v4l2_get_subdevdata(sd);
  int ret;
  
  //insure that clk_disable() and clk_enable() are called in pair 
  //when calling CSI_SUBDEV_STBY_ON/OFF and CSI_SUBDEV_PWR_ON/OFF
  ret = 0;
  switch(on)
  {
    case CSI_SUBDEV_STBY_ON:
      vfe_dev_print("disalbe oe!\n");
  //    ret = sensor_write_array(sd, sensor_oe_disable_regs, ARRAY_SIZE(sensor_oe_disable_regs));
//      if(ret < 0)
 //       vfe_dev_err("disalbe oe falied!\n");
      //software standby on
      ret = sensor_s_sw_stby(sd, CSI_STBY_ON);
      if(ret < 0)
        vfe_dev_err("soft stby falied!\n");
      mdelay(10);
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      i2c_lock_adapter(client->adapter);
      //standby on io
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      i2c_unlock_adapter(client->adapter);  

      break;
	  
    case CSI_SUBDEV_STBY_OFF:
      vfe_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      i2c_lock_adapter(client->adapter);    
      //active mclk before stadby out
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      mdelay(10);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      mdelay(10);
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      i2c_unlock_adapter(client->adapter);        
      //software standby
      ret = sensor_s_sw_stby(sd, CSI_STBY_OFF);
      if(ret < 0)
        vfe_dev_err("soft stby off falied!\n");
      mdelay(10);
      vfe_dev_print("enable oe!\n");
   //   ret = sensor_write_array(sd, sensor_oe_enable_regs,  ARRAY_SIZE(sensor_oe_enable_regs));
  //    if(ret < 0)
    //    vfe_dev_err("enable oe falied!\n");
      break;
   
    case CSI_SUBDEV_PWR_ON:
      vfe_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      i2c_lock_adapter(client->adapter);
      //power on reset
      vfe_gpio_set_status(sd,PWDN,1);//set the gpio to output
      vfe_gpio_set_status(sd,RESET,1);//set the gpio to output
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);  
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      mdelay(1);
      //power supply
      //vfe_gpio_write(sd,POWER_EN,CSI_PWR_ON);
      vfe_set_pmu_channel(sd,IOVDD,ON);
      //reset on io
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
  
      vfe_set_pmu_channel(sd,AVDD,ON);
      vfe_set_pmu_channel(sd,DVDD,ON);
   //   vfe_set_pmu_channel(sd,AFVDD,ON);

      //standby on with io
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);

    //   sensor_set_pll_enable(sd,ON);
   //    sensor_set_power_sleep(sd,OFF);  
         mdelay(10);
      //active mclk before power on
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);

/*
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      mdelay(10);
      //reset after power on
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);
      mdelay(30);
      */
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      i2c_unlock_adapter(client->adapter);  
      break;

	  
    case CSI_SUBDEV_PWR_OFF:
      vfe_dev_dbg("CSI_SUBDEV_PWR_OFF!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      i2c_lock_adapter(client->adapter);

      vfe_gpio_set_status(sd,PWDN,1);//set the gpio to output
      vfe_gpio_set_status(sd,RESET,1);//set the gpio to output
      vfe_gpio_write(sd,RESET,CSI_RST_ON);  
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);

      sensor_set_power_sleep(sd,ON);  
      sensor_set_pll_enable(sd,OFF);

      //inactive mclk before power off
      vfe_set_mclk(sd,OFF);

	  
      //power supply off
      vfe_gpio_write(sd,POWER_EN,CSI_PWR_OFF);
     // vfe_set_pmu_channel(sd,AFVDD,OFF);
      vfe_set_pmu_channel(sd,DVDD,OFF);
      vfe_set_pmu_channel(sd,AVDD,OFF);
      //standby and reset io
      mdelay(10);

     //standby off with io  
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
	  
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);

      vfe_set_pmu_channel(sd,IOVDD,OFF);  
	  
      //set the io to hi-z
      vfe_gpio_set_status(sd,RESET,0);//set the gpio to input
      vfe_gpio_set_status(sd,PWDN,0);//set the gpio to input
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
  switch(val)
  {
    case 0:
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);
      mdelay(10);
      break;
    case 1:
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      mdelay(10);
      break;
    default:
      return -EINVAL;
  }
    
  return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
  unsigned char rdval;
  
  LOG_ERR_RET(sensor_read(sd, 0x0004, &rdval))
  if(rdval != V4L2_IDENT_SENSOR)
  {
        printk(KERN_DEBUG"*********sensor error,read id is %d.\n",rdval);
    	return -ENODEV;
  }
   else
   {
        printk(KERN_DEBUG"*********find hi542 raw data camera sensor now.\n");
  	return 0;
   }
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
  int ret;
  struct sensor_info *info = to_state(sd);
  
  vfe_dev_dbg("sensor_init\n");

  /*Make sure it is a target sensor*/
  ret = sensor_detect(sd);
  if (ret) {
    vfe_dev_err("chip found is not an target chip.\n");
    return ret;
  }


   
  vfe_get_standby_mode(sd,&info->stby_mode);
  
  if((info->stby_mode == HW_STBY || info->stby_mode == SW_STBY) \
      && info->init_first_flag == 0) {
    vfe_dev_print("stby_mode and init_first_flag = 0\n");
    return 0;
  } 
  
  info->focus_status = 0;
  info->low_speed = 0;
  info->width = QSXGA_WIDTH;
  info->height = QSXGA_HEIGHT;
  info->hflip = 0;
  info->vflip = 0;
  info->gain = 0;

  info->tpf.numerator = 1;            
  info->tpf.denominator = 30;    /* 30fps */    
  
  ret = sensor_write_array(sd, sensor_default_regs, ARRAY_SIZE(sensor_default_regs));  
  if(ret < 0) {
    vfe_dev_err("write sensor_default_regs error\n");
    return ret;
  }
  
  if(info->stby_mode == 0)
    info->init_first_flag = 0;
  
  info->preview_first_flag = 1;
  
  return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
  int ret=0;
  struct sensor_info *info = to_state(sd);
//  vfe_dev_dbg("[]cmd=%d\n",cmd);
//  vfe_dev_dbg("[]arg=%0x\n",arg);
  switch(cmd) {
    case GET_CURRENT_WIN_CFG:
      if(info->current_wins != NULL)
      {
        memcpy( arg,
                info->current_wins,
                sizeof(struct sensor_win_size) );
        ret=0;
      }
      else
      {
        vfe_dev_err("empty wins!\n");
        ret=-1;
      }
      break;
    case SET_FPS:
      ret=0;
//      if((unsigned int *)arg==1)
//        ret=sensor_write(sd, 0x3036, 0x78);
//      else
//        ret=sensor_write(sd, 0x3036, 0x32);
      break;
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
  enum v4l2_mbus_pixelcode mbus_code;
  struct regval_list *regs;
  int regs_size;
  int bpp;   /* Bytes per pixel */
}sensor_formats[] = {
	{
		.desc		= "Raw RGB Bayer",
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.regs 		= sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp		= 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

  

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size sensor_win_sizes[] = {
	  /* qsxga: 2592*1944 */
  
	  {
      .width      = QSXGA_WIDTH,
      .height     = QSXGA_HEIGHT,
      .hoffset    = 0,
      .voffset    = 0,
      .hts        = 2608,
      .vts        = 1960,
      .pclk       = 84*1000*1000,
      .fps_fixed  = 1,
      .bin_factor = 1,
      .intg_min   = 1<<4,
      .intg_max   = 1968<<4,
      .gain_min   = 1<<4,
      .gain_max   = 8<<4,
      .regs       = sensor_qsxga_regs,
      .regs_size  = ARRAY_SIZE(sensor_qsxga_regs),
      .set_size   = NULL,
    },
    /* 1080P */
/*
    {
      .width			= HD1080_WIDTH,
      .height 		= HD1080_HEIGHT,
      .hoffset	  = 0,
      .voffset	  = 0,
      .hts        = 2500,
      .vts        = 1120,
      .pclk       = 84*1000*1000,
      .fps_fixed  = 1,
      .bin_factor = 1,
      .intg_min   = 1<<4,
      .intg_max   = 1120<<4,
      .gain_min   = 1<<4,
      .gain_max   = 16<<4,
      .regs       = sensor_1080p_regs,//
      .regs_size  = ARRAY_SIZE(sensor_1080p_regs),//
      .set_size		= NULL,
    },
*/
	/* UXGA */
//	{
//      .width			= UXGA_WIDTH,
//      .height 		= UXGA_HEIGHT,
//      .hoffset	  = 0,
//      .voffset	  = 0,
//      .hts        = 2800,//limited by sensor
//      .vts        = 1000,
//      .pclk       = 84*1000*1000,
//      .fps_fixed  = 1,
//      .bin_factor = 1,
//      .intg_min   = ,
//      .intg_max   = ,
//      .gain_min   = ,
//      .gain_max   = ,
//      .regs			= sensor_uxga_regs,
//      .regs_size	= ARRAY_SIZE(sensor_uxga_regs),
//      .set_size		= NULL,
//	},
  	/* SXGA *///1280x960
    {     
      .width			= SXGA_WIDTH,
      .height 		= SXGA_HEIGHT,
      .hoffset	  = 0,
      .voffset	  = 0,
      .hts        = 1288,
      .vts        = 968,
      .pclk       = 84*1000*1000,
      .fps_fixed  = 1,
      .bin_factor = 1,
      .intg_min   = 1<<4,
      .intg_max   = 968<<4,
      .gain_min   = 1<<4,
      .gain_max   = 8<<4,
      .regs		    = sensor_sxga_regs,
      .regs_size	= ARRAY_SIZE(sensor_sxga_regs),
      .set_size		= NULL,
    },
    /* 720p */

    {
      .width      = HD720_WIDTH,
      .height     = HD720_HEIGHT,
      .hoffset    = 0,
      .voffset    = 0,
      .hts        = 1288,
      .vts        = 728,
      .pclk       = 84*1000*1000,
      .fps_fixed  = 1,
      .bin_factor = 1,
      .intg_min   = 1<<4,
      .intg_max   = 728<<4,
      .gain_min   = 1<<4,
      .gain_max   = 8<<4,
      .regs			  = sensor_720p_regs,//
      .regs_size	= ARRAY_SIZE(sensor_720p_regs),//
      .set_size		= NULL,
    },

    /* XGA */
//    {
//      .width			= XGA_WIDTH,
//      .height 		= XGA_HEIGHT,
//      .hoffset    = 0,
//      .voffset    = 0,
//      .hts        = 2800,//limited by sensor
//      .vts        = 1000,
//      .pclk       = 84*1000*1000,
//      .fps_fixed  = 1,
//      .bin_factor = 1,
//      .intg_min   = ,
//      .intg_max   = ,
//      .gain_min   = ,
//      .gain_max   = ,
//      .regs			  = sensor_xga_regs,
//      .regs_size	= ARRAY_SIZE(sensor_xga_regs),
//      .set_size		= NULL,
//    },
  /* SVGA */
//    {
//      .width			= SVGA_WIDTH,
//      .height 		= SVGA_HEIGHT,
//      .hoffset	  = 0,
//      .voffset	  = 0,
//      .hts        = 2800,//limited by sensor
//      .vts        = 1000,
//      .pclk       = 84*1000*1000,
//      .fps_fixed  = 1,
//      .bin_factor = 1,
//      .intg_min   = ,
//      .intg_max   = ,
//      .gain_min   = ,
//      .gain_max   = ,
//      .regs       = sensor_svga_regs,
//      .regs_size  = ARRAY_SIZE(sensor_svga_regs),
//      .set_size   = NULL,
//    },
  /* VGA */
//    {
//      .width			= VGA_WIDTH,
//      .height 		= VGA_HEIGHT,
//      .hoffset	  = 0,
//      .voffset	  = 0,
//      .hts        = 2800,//limited by sensor
//      .vts        = 1000,
//      .pclk       = 84*1000*1000,
//      .fps_fixed  = 1,
//      .bin_factor = 1,
//      .intg_min   = ,
//      .intg_max   = ,
//      .gain_min   = ,
//      .gain_max   = ,
//      .regs       = sensor_vga_regs,
//      .regs_size  = ARRAY_SIZE(sensor_vga_regs),
//      .set_size   = NULL,
//    },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
                 enum v4l2_mbus_pixelcode *code)
{
  if (index >= N_FMTS)
    return -EINVAL;

  *code = sensor_formats[index].mbus_code;
  return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd,
                            struct v4l2_frmsizeenum *fsize)
{
  if(fsize->index > N_WIN_SIZES-1)
  	return -EINVAL;
  
  fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
  fsize->discrete.width = sensor_win_sizes[fsize->index].width;
  fsize->discrete.height = sensor_win_sizes[fsize->index].height;
  
  return 0;
}

static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
    struct v4l2_mbus_framefmt *fmt,
    struct sensor_format_struct **ret_fmt,
    struct sensor_win_size **ret_wsize)
{
  int index;
  struct sensor_win_size *wsize;
  struct sensor_info *info = to_state(sd);

  for (index = 0; index < N_FMTS; index++)
    if (sensor_formats[index].mbus_code == fmt->code)
      break;

  if (index >= N_FMTS) 
    return -EINVAL;
  
  if (ret_fmt != NULL)
    *ret_fmt = sensor_formats + index;
    
  /*
   * Fields: the sensor devices claim to be progressive.
   */
  
  fmt->field = V4L2_FIELD_NONE;
  
  /*
   * Round requested image size down to the nearest
   * we support, but not below the smallest.
   */
  for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
       wsize++)
    if (fmt->width >= wsize->width && fmt->height >= wsize->height)
      break;
    
  if (wsize >= sensor_win_sizes + N_WIN_SIZES)
    wsize--;   /* Take the smallest one */
  if (ret_wsize != NULL)
    *ret_wsize = wsize;
  /*
   * Note the size we'll actually handle.
   */
  fmt->width = wsize->width;
  fmt->height = wsize->height;
  info->current_wins = wsize;
  //pix->bytesperline = pix->width*sensor_formats[index].bpp;
  //pix->sizeimage = pix->height*pix->bytesperline;

  return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd, 
             struct v4l2_mbus_framefmt *fmt)
{
  return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
           struct v4l2_mbus_config *cfg)
{
  cfg->type = V4L2_MBUS_PARALLEL;
  cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL ;
  
  return 0;
}


/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd, 
             struct v4l2_mbus_framefmt *fmt)
{
  int ret;
  struct sensor_format_struct *sensor_fmt;
  struct sensor_win_size *wsize;
  struct sensor_info *info = to_state(sd);
  
  vfe_dev_dbg("sensor_s_fmt\n");
  
  sensor_write_array(sd, sensor_oe_disable_regs, ARRAY_SIZE(sensor_oe_disable_regs));
  
  ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
  if (ret)
    return ret;

  if(info->capture_mode == V4L2_MODE_VIDEO)
  {
    //video
  }
  else if(info->capture_mode == V4L2_MODE_IMAGE)
  {
    //image 
    
  }

  sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

  ret = 0;
  if (wsize->regs)
    LOG_ERR_RET(sensor_write_array(sd, wsize->regs, wsize->regs_size))
  
  if (wsize->set_size)
    LOG_ERR_RET(wsize->set_size(sd))

  info->fmt = sensor_fmt;
  info->width = wsize->width;
  info->height = wsize->height;

  vfe_dev_print("s_fmt set width = %d, height = %d\n",wsize->width,wsize->height);

  if(info->capture_mode == V4L2_MODE_VIDEO)
  {
    //video
   
  } else {
    //capture image

  }
	
	//sensor_write_array(sd, sensor_oe_enable_regs, ARRAY_SIZE(sensor_oe_enable_regs));
	
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
     
  return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
  struct v4l2_captureparm *cp = &parms->parm.capture;
  //struct v4l2_fract *tpf = &cp->timeperframe;
  struct sensor_info *info = to_state(sd);
  //unsigned char div;
  
  vfe_dev_dbg("sensor_s_parm\n");
  
  if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    return -EINVAL;
  
  if (info->tpf.numerator == 0)
    return -EINVAL;
    
  info->capture_mode = cp->capturemode;
  
  return 0;
}


static int sensor_queryctrl(struct v4l2_subdev *sd,
    struct v4l2_queryctrl *qc)
{
  /* Fill in min, max, step and default value for these controls. */
  /* see include/linux/videodev2.h for details */
  
  switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1*16, 8*16, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 1, 65536*16, 1, 1);
	case V4L2_CID_FRAME_RATE:
		return v4l2_ctrl_query_fill(qc, 15, 120, 1, 30);
  }
  return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
  switch (ctrl->id) {
  case V4L2_CID_GAIN:
    return sensor_g_gain(sd, &ctrl->value);
  case V4L2_CID_EXPOSURE:
  	return sensor_g_exp(sd, &ctrl->value);
  }
  return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
  struct v4l2_queryctrl qc;
  int ret;
  
  qc.id = ctrl->id;
  ret = sensor_queryctrl(sd, &qc);
  if (ret < 0) {
    return ret;
  }

  if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
    return -ERANGE;
  }
  
  switch (ctrl->id) {
    case V4L2_CID_GAIN:
      return sensor_s_gain(sd, ctrl->value);
    case V4L2_CID_EXPOSURE:
	  return sensor_s_exp(sd, ctrl->value);
	case V4L2_CID_FRAME_RATE:
	  return sensor_s_framerate(sd, ctrl->value);
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
  .enum_mbus_fmt = sensor_enum_fmt,
  .enum_framesizes = sensor_enum_size,
  .try_mbus_fmt = sensor_try_fmt,
  .s_mbus_fmt = sensor_s_fmt,
  .s_parm = sensor_s_parm,
  .g_parm = sensor_g_parm,
  .g_mbus_config = sensor_g_mbus_config,
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
//  int ret;

  info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
  if (info == NULL)
    return -ENOMEM;
  sd = &info->sd;
  glb_sd = sd;
  v4l2_i2c_subdev_init(sd, client, &sensor_ops);

  info->fmt = &sensor_formats[0];
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
  { "hi542", 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);


static struct i2c_driver sensor_driver = {
  .driver = {
    .owner = THIS_MODULE,
  .name = "hi542",
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

