/*
 * sunxi Camera core header file
 * Author:raymonxiu
*/
#ifndef _SUNXI_CSI_CORE_H_
#define _SUNXI_CSI_CORE_H_

#include <linux/types.h>
#include <media/videobuf-core.h>
#include <media/v4l2-device.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <media/v4l2-mediabus.h>//linux-3.0
#include <mach/sys_config.h>
#include <linux/gpio.h>
#include <../arch/arm/mach-sun6i/include/mach/clock.h>
#include <../arch/arm/mach-sun6i/include/mach/gpio.h>
#include <../arch/arm/mach-sun6i/include/mach/sys_config.h>
#include <linux/earlysuspend.h>

//for internel driver debug
#define DBG_EN   		0
//debug level 0~3
#define DBG_LEVEL 	1

//for internel driver debug
#if(DBG_EN==1)		
#define csi_dbg(l,x,arg...) if(l <= DBG_LEVEL) printk(KERN_DEBUG"[CSI_DEBUG]"x,##arg)
#else
#define csi_dbg(l,x,arg...) 
#endif

//print when error happens
#define csi_err(x,arg...) printk(KERN_ERR"[CSI_ERR]"x,##arg)

//print unconditional, for important info
#define csi_print(x,arg...) printk(KERN_INFO"[CSI]"x,##arg)

#define MAX_NUM_INPUTS 2

struct csi_subdev_info {
	const char *name;
	struct i2c_board_info board_info;
};

/*
 * if format
 */
typedef enum tag_CSI_IF
{
    CSI_IF_INTLV								=0x00,     /* 1SEG DATA in one channel  */ 
    CSI_IF_SPL									=0x01,     /* 2SEG: 1SEG Y in one channel , 1SEG UV in second channel */
    CSI_IF_PL										=0x02,     /* 3SEG YUV444 */
    CSI_IF_PL_SPL								=0x03,     /* 3SEG YUV444 to 2SEG YUV422  */
	
    CSI_IF_CCIR656_1CH					=0x04,   /* 1SEG ccir656  1ch   */
    CSI_IF_CCIR656_1CH_SPL			=0x05,   /* 2SEG  ccir656 1ch   */
    CSI_IF_CCIR656_1CH_PL				=0x06,   /*3SEG ccir656  1ch   */
    CSI_IF_CCIR656_1CH_PL_SPL		=0x07,   /* 3SEG to 2SEG ccir656  1ch   */
    
    CSI_IF_CCIR656_2CH					=0x0c,   /* D7~D0:ccir656  2ch   */
    CSI_IF_CCIR656_4CH					=0x0d,   /* D7~D0:ccir656  4ch   */
    
    CSI_IF_MIPI									=0x80,	 /* MIPI CSI */   
}__csi_if_t;

/*
 *	data width
 */
typedef enum tag_CSI_DATA_WIDTH
{
	CSI_8BIT			=0, 
 	CSI_10BIT			=1,   
	CSI_12BIT			=2,    
}__csi_data_width_t;


/*
 * input data format
 */
typedef enum tag_CSI_INPUT_FMT
{
	CSI_RAW=0,     /* raw stream  */
	//CSI_BAYER=1,
 	CSI_YUV422=3,    /* yuv422      */
	CSI_YUV420=4,    /* yuv420      */
}__csi_input_fmt_t;

/*
 * output data format
 */
typedef enum tag_CSI_OUTPUT_FMT
{
    /* only when input is raw */
    CSI_FIELD_RAW_8    = 0,
    CSI_FIELD_RAW_10   = 1,
    CSI_FIELD_RAW_12   = 2,
    CSI_FIELD_RGB565   = 4,
    CSI_FIELD_RGB888   = 5,
    CSI_FIELD_PRGB888  = 6,
    CSI_FRAME_RAW_8    = 8,
    CSI_FRAME_RAW_10   = 9,
    CSI_FRAME_RAW_12   = 10,
    CSI_FRAME_RGB565   = 12,
    CSI_FRAME_RGB888   = 13,
    CSI_FRAME_PRGB888  = 14,

    /* only when input is bayer */
    //CSI_PLANAR_RGB242 = 0,               /* planar rgb242 */

    /* only when input is yuv422/yuv420 */
    CSI_FIELD_PLANAR_YUV422 = 0,         /* parse a field(odd or even) into planar yuv420 */
    CSI_FIELD_PLANAR_YUV420 = 1,         /* parse a field(odd or even) into planar yuv420 */
    CSI_FRAME_PLANAR_YUV420 = 2,				 /* parse and reconstruct every 2 fields(odd and even) into a frame, format is planar yuv420 */
    CSI_FRAME_PLANAR_YUV422 = 3,
    CSI_FIELD_UV_CB_YUV422  = 4,         
    CSI_FIELD_UV_CB_YUV420  = 5,
    CSI_FRAME_UV_CB_YUV420  = 6,
    CSI_FRAME_UV_CB_YUV422  = 7,
    CSI_FIELD_MB_YUV422     = 8,
    CSI_FIELD_MB_YUV420     = 9,
    CSI_FRAME_MB_YUV420     = 10,
    CSI_FRAME_MB_YUV422     = 11,
    CSI_FIELD_UV_CB_YUV422_10  = 12,
    CSI_FIELD_UV_CB_YUV420_10  = 13,
    //CSI_INTLC_INTLV_YUV422  = 15,

}__csi_output_fmt_t;

/*
 * field seq/pol
 */

typedef enum tag_CSI_FIELD_POL
{
    CSI_FIELD_TF		= 0,    /* top filed first */
    CSI_FIELD_BF		= 1,   	/* bottom field first */
}__csi_field_pol_t;

/*
 * input src type
 */
typedef enum tag_CSI_SRC_TYPE
{
    CSI_PROGRESSIVE=0,    /* progressive */
    CSI_INTERLACE=1,   /* interlace */
}__csi_src_type_t;


/*
 * input field selection, only when input is ccir656
 */
typedef enum tag_CSI_FIELD_SEL
{
    CSI_ODD,    /* odd field */
    CSI_EVEN,   /* even field */
    CSI_EITHER, /* either field */
}__csi_field_sel_t;

/*
 * input data sequence
 */
typedef enum tag_CSI_SEQ
{
    /* only when input is yuv422 */
    CSI_YUYV=0,
    CSI_YVYU,
    CSI_UYVY,
    CSI_VYUY,

    /* only when input is byer */
    CSI_RGRG=0,               /* first line sequence is RGRG... */
    CSI_GRGR,                 /* first line sequence is GRGR... */
    CSI_BGBG,                 /* first line sequence is BGBG... */
    CSI_GBGB,                 /* first line sequence is GBGB... */
}__csi_seq_t;

/*
 * input reference signal polarity
 */
typedef enum tag_CSI_REF
{
    CSI_LOW,    /* active low */
    CSI_HIGH,   /* active high */
}__csi_ref_t;

/*
 * input data valid of the input clock edge type
 */
typedef enum tag_CSI_CLK
{
    CSI_FALLING,    /* active falling */
    CSI_RISING,     /* active rising */
}__csi_clk_t;


/*
 * csi if configuration
 */
typedef struct tag_CSI_IF_CONF
{
    __csi_src_type_t   src_type;   	/* interlaced or progressive */
    __csi_field_pol_t  field_pol;   /* top or bottom field first */
    __csi_ref_t        vref;        /* input vref signal polarity */
    __csi_ref_t        href;        /* input href signal polarity */
    __csi_clk_t        clock;       /* input data valid of the input clock edge type */
		__csi_data_width_t data_width;	/* csi data width */
		__csi_if_t				 csi_if;
}__csi_if_conf_t;


/*
 * csi mode configuration
 */
typedef struct tag_CSI_FMT_CONF
{
    __csi_input_fmt_t  input_fmt;   /* input data format */
    __csi_output_fmt_t output_fmt;  /* output data format */
    __csi_field_sel_t  field_sel;   /* input field selection */
    __csi_seq_t        seq;         /* input data sequence */
}__csi_fmt_conf_t;


/*
 * csi buffer
 */

typedef enum tag_CSI_BUF
{
		CSI_BUF_0_A = 0,    /* FIFO for Y address A */  
		CSI_BUF_0_B,    /* FIFO for Y address B */
		CSI_BUF_1_A,    /* FIFO for Cb address A */
		CSI_BUF_1_B,    /* FIFO for Cb address B */
		CSI_BUF_2_A,    /* FIFO for Cr address A */
		CSI_BUF_2_B,    /* FIFO for Cr address B */
}__csi_buf_t;

/*
 * csi capture status
 */
typedef struct tag_CSI_CAPTURE_STATUS
{
    _Bool picture_in_progress;  
    _Bool video_in_progress;    
}__csi_capture_status;


/*
 * csi interrupt
 */
typedef enum tag_CSI_INT
{
    CSI_INT_CAPTURE_DONE     = 0X1,
    CSI_INT_FRAME_DONE       = 0X2,
    CSI_INT_BUF_0_OVERFLOW   = 0X4,
    CSI_INT_BUF_1_OVERFLOW   = 0X8,
    CSI_INT_BUF_2_OVERFLOW   = 0X10,
    CSI_INT_PROTECTION_ERROR = 0X20,
    CSI_INT_HBLANK_OVERFLOW  = 0X40,
    CSI_INT_VSYNC_TRIG       = 0X80,
}__csi_int_t;

/*
 * csi interrupt status
 */
typedef struct tag_CSI_INT_STATUS
{
    _Bool capture_done;
    _Bool frame_done;
    _Bool buf_0_overflow;
    _Bool buf_1_overflow;
    _Bool buf_2_overflow;
    _Bool protection_error;
    _Bool hblank_overflow;
    _Bool vsync_trig;
}__csi_int_status_t;

/*
 * csi sub device info
 */
typedef struct tag_CSI_SUBDEV_INFO
{
    int								 mclk;				/* the mclk frequency for sensor module in HZ unit*/
    __csi_ref_t        vref;        /* input vref signal polarity */
    __csi_ref_t        href;        /* input href signal polarity */
    __csi_clk_t        clock;       /* input data valid of the input clock edge type */
    int								 iocfg;				/*0 for csi back , 1 for csi front*/				
    int 							 stby_mode;			
}__csi_subdev_info_t;

struct csi_buf_addr {
	dma_addr_t	y;
	dma_addr_t	cb;
	dma_addr_t	cr;
};

struct csi_plane_size {
	u32	p0_size;
	u32 p1_size;
	u32 p2_size;
};

struct csi_fmt {
	u8					name[32];
	__csi_if_t									csi_if;
	enum v4l2_mbus_pixelcode		ccm_fmt;//linux-3.0
	u32   											fourcc;          /* v4l2 format id */
	enum v4l2_field							field;
	__csi_input_fmt_t						input_fmt;	
	__csi_output_fmt_t 					output_fmt;	
	//enum pkt_fmt 								mipi_pkt_fmt;
	__csi_field_sel_t						csi_field;
	int   											depth;
	u16	  											planes_cnt;
};

struct csi_size{
	u32		width;
	u32		height;
	u32		buf_len_y;
	u32		buf_len_c;
};

struct csi_channel
{
	struct csi_size size;
	unsigned char vc;
	//enum pkt_fmt mipi_pkt_fmt;
};

/* buffer for one video frame */
struct csi_buffer {
	struct videobuf_buffer vb;
	struct csi_fmt        *fmt;
};

struct csi_dmaqueue {
	struct list_head active;
	
	/* Counters to control fps rate */
	int frame;
	int ini_jiffies;
};

static LIST_HEAD(csi_devlist);

struct ccm_config {
	char ccm[I2C_NAME_SIZE];
	char iovdd_str[32];
	char avdd_str[32];
	char dvdd_str[32];
	int twi_id;
	uint i2c_addr;
	int vflip;
	int hflip;
	int stby_mode;
	int interface;
	int flash_pol;		
//	user_gpio_set_t reset_io;
//	user_gpio_set_t standby_io;
//	user_gpio_set_t power_io;
//	user_gpio_set_t flash_io;
//	user_gpio_set_t af_power_io;
	//modified to 33
	struct gpio_config reset_io;
	struct gpio_config standby_io;
	struct gpio_config power_io;
	struct gpio_config flash_io;
	struct gpio_config af_power_io;
	
	struct regulator 	 *iovdd;		  /*interface voltage source of sensor module*/
	struct regulator 	 *avdd;			/*anlog voltage source of sensor module*/
	struct regulator 	 *dvdd;			/*core voltage source of sensor module*/
	
	uint vol_iovdd;
	uint vol_avdd;
	uint vol_dvdd;
	
	__csi_subdev_info_t ccm_info;  
	struct v4l2_subdev			*sd;
};

struct csi_dev {
	struct list_head       	csi_devlist;
	struct v4l2_device 	   	v4l2_dev;
	struct v4l2_subdev			*sd;
	struct platform_device	*pdev;

	int						id;
	
	spinlock_t              slock;
	struct mutex						standby_lock;
	//up when suspend,down when resume. ensure open is being called after resume has been done
	struct semaphore        standby_seq_sema;   
	/* various device info */
	struct video_device     *vfd;

	struct csi_dmaqueue     vidq;

	/* Several counters */
	unsigned 		   		ms;
	unsigned long           jiffies;

	/* Input Number */
	int			   			input;

	/* video capture */
	struct csi_fmt          *fmt;
	unsigned int            width;
	unsigned int            height;
	unsigned int						frame_size;
	struct videobuf_queue   vb_vidq;
	unsigned int 						capture_mode;
	
	/*working state*/
	unsigned long 		   	generating;
	int								opened;
	
	/* suspend */
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif

	/* work queue */
	struct work_struct resume_work;
	struct delayed_work probe_work;
	
	/*pin,clock,irq resource*/
	//int								csi_pin_hd;
	script_item_u     *csi_pin_list;
	int               csi_pin_cnt;//add for 33
	struct clk				*csi_clk_src;
	struct clk				*csi_ahb_clk;
	struct clk				*csi_module_clk;
	struct clk				*csi_dram_clk;
	struct clk				*csi_isp_src_clk;
	struct clk				*csi_isp_clk;
	int								irq;
	void __iomem			*regs;
	struct resource		*regs_res;
	void __iomem			*dphy_regs;
	struct resource		*dphy_regs_res;
	void __iomem			*protocol_regs;
	struct resource		*protocol_regs_res;
	/*power issue*/
	
	int								 stby_mode;
	struct regulator 	 *iovdd;		  /*interface voltage source of sensor module*/
  struct regulator 	 *avdd;			/*anlog voltage source of sensor module*/
  struct regulator 	 *dvdd;			/*core voltage source of sensor module*/
	
	uint vol_iovdd;
	uint vol_avdd;
	uint vol_dvdd;
	
	/* attribution */
	unsigned int interface;
	unsigned int vflip;
	unsigned int hflip;
	unsigned int flash_pol;
	
	/* csi io */
//	user_gpio_set_t reset_io;
//	user_gpio_set_t standby_io;
//	user_gpio_set_t power_io;
//	user_gpio_set_t flash_io;
//	user_gpio_set_t af_power_io;
	//modified to 33
	struct gpio_config reset_io;
	struct gpio_config standby_io;
	struct gpio_config power_io;
	struct gpio_config flash_io;
	struct gpio_config af_power_io;
	
	/*parameters*/
	unsigned int			cur_ch;
	__csi_if_conf_t   csi_if_cfg;
	__csi_fmt_conf_t			csi_fmt_cfg;
	struct csi_buf_addr		csi_buf_addr;
	struct csi_plane_size	csi_plane;
	struct csi_size				csi_size;
	
	/* ccm config */
  int dev_qty;
	int module_flag;
	__csi_subdev_info_t *ccm_info;  /*current config*/
	struct ccm_config *ccm_cfg[MAX_NUM_INPUTS];
	
	/* mipi config */
	unsigned char lane_num;
	unsigned char total_ch;
	struct csi_channel ch[4];
};

extern void bsp_csi_set_base_addr(unsigned int addr);
extern void bsp_csi_open(void);
extern void bsp_csi_close(void);
extern void bsp_csi_if_configure(__csi_if_conf_t *csi_if_cfg);
extern void bsp_csi_fmt_configure(unsigned int ch, __csi_fmt_conf_t *csi_fmt_cfg);
extern void inline bsp_csi_set_buffer_address(unsigned int ch, __csi_buf_t buf, unsigned int addr);
extern unsigned int inline bsp_csi_get_buffer_address(unsigned int ch, __csi_buf_t buf);
extern void bsp_csi_capture_video_start(unsigned int ch);
extern void bsp_csi_capture_video_stop(unsigned int ch);
extern void bsp_csi_capture_picture(unsigned int ch);
extern void bsp_csi_capture_get_status(unsigned int ch, __csi_capture_status * status);
extern void bsp_csi_set_size(unsigned int ch, unsigned int length_h, unsigned int length_v, unsigned int buf_length_y, unsigned int buf_length_c);
extern void bsp_csi_set_offset(unsigned int ch, unsigned int start_h, unsigned int start_v);
extern void bsp_csi_int_enable(unsigned int ch, __csi_int_t interrupt);
extern void bsp_csi_int_disable(unsigned int ch, __csi_int_t interrupt);
extern void inline bsp_csi_int_get_status(unsigned int ch, __csi_int_status_t * status);
extern void inline bsp_csi_int_clear_status(unsigned int ch, __csi_int_t interrupt);

#endif  /* _CSI_H_ */
