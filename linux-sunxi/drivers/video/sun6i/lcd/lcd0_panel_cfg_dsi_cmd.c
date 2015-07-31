
#include "lcd_panel_cfg.h"

//delete this line if you want to use the lcd para define in sys_config1.fex
#define LCD_PARA_USE_CONFIG

#ifdef LCD_PARA_USE_CONFIG
static __u8 g_gamma_tbl[][2] = 
{
//{input value, corrected value}
    {0, 0},
    {15, 15},
    {30, 30},
    {45, 45},
    {60, 60},
    {75, 75},
    {90, 90},
    {105, 105},
    {120, 120},
    {135, 135},
    {150, 150},
    {165, 165},
    {180, 180},
    {195, 195},
    {210, 210},
    {225, 225},
    {240, 240},
    {255, 255},
};

static void LCD_cfg_panel_info(__panel_para_t * info)
{ 
    memset(info,0,sizeof(__panel_para_t));

	//interface
    info->lcd_if            = LCD_IF_DSI;
    info->lcd_dsi_if        = LCD_DSI_IF_COMMAND_MODE;
    info->lcd_dsi_lane		= LCD_DSI_2LANE;
    info->lcd_dsi_format	= LCD_DSI_FORMAT_RGB888;
	info->lcd_cpu_te		= LCD_CPU_TE_ENABLE;
	
    //timing
    info->lcd_x             = 480;			//Hor Pixels
    info->lcd_y             = 800;			//Ver Pixels
    info->lcd_dclk_freq     = 10;       	//Pixel Data Cycle
//    info->lcd_fps     		= 5;       		//Pixel Data Cycle

    info->lcd_dsi_dphy_timing_en = 2;

	//color
    info->lcd_frm           = 0;        	//0: direct; 	1: rgb666 ditcher;	2:rgb656 dither
    info->lcd_gamma_en 		= 0;

	//backlight
    info->lcd_pwm_not_used  = 0;
    info->lcd_pwm_ch        = 0;
    info->lcd_pwm_freq      = 12500;		//Hz
    info->lcd_pwm_pol       = 0;
}
#endif

static	void bm480800_8892ftgb_init(__u32 sel)
{
	__u8 cmd;
	__u8 para[256];
	__u8* p;
	__u32 num;
	__u32 i;

	cmd 	= 0xb9; //set_extc
	p		= para;
	num		= 3;
	*(p++)	= 0xff;
	*(p++)	= 0x83;
	*(p++)	= 0x69;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xb1; //set power 
	p		= para;
	num		= 19;
	*(p++)	= 0x85;
	*(p++)	= 0x00;
	*(p++)	= 0x34;
	*(p++)	= 0x07;
	*(p++)	= 0x00;
	*(p++)	= 0x0d;
	*(p++)	= 0x0d;
	*(p++)	= 0x1a;
	*(p++)	= 0x22;
	*(p++)	= 0x3f;
	*(p++)	= 0x3f;
	*(p++)	= 0x01;
	*(p++)	= 0x23;
	*(p++)	= 0x01;
	*(p++)	= 0xe6;
	*(p++)	= 0xe6;
	*(p++)	= 0xe6;
	*(p++)	= 0xe6;
	*(p++)	= 0xe6;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xb2; // set display 480x800
	p		= para;
	num		= 15;
	*(p++)	= 0x00;
	*(p++)	= 0x20;
	*(p++)	= 0x05;
	*(p++)	= 0x05;
	*(p++)	= 0x70;
	*(p++)	= 0x00;
	*(p++)	= 0xff;
	*(p++)	= 0x00;
	*(p++)	= 0x00;
	*(p++)	= 0x00;
	*(p++)	= 0x00;
	*(p++)	= 0x03;
	*(p++)	= 0x03;
	*(p++)	= 0x00;
	*(p++)	= 0x01;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xb4; // set display column inversion
	p		= para;
	num		= 5;
	*(p++)	= 0x00;
	*(p++)	= 0x18;
	*(p++)	= 0x80;
	*(p++)	= 0x06;
	*(p++)	= 0x02;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xb6; // set vcom
	p		= para;
	num		= 2;
	*(p++)	= 0x42;
	*(p++)	= 0x42;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xd5; // setgip
	p		= para;
	num		= 26;
	*(p++)	= 0x00;
	*(p++)	= 0x03;
	*(p++)	= 0x03;
	*(p++)	= 0x00;
	*(p++)	= 0x01;
	*(p++)	= 0x04;
	*(p++)	= 0x28;
	*(p++)	= 0x70;
	*(p++)	= 0x11;
	*(p++)	= 0x13;
	*(p++)	= 0x00;
	*(p++)	= 0x00;
	*(p++)	= 0x40;
	*(p++)	= 0x06;
	*(p++)	= 0x51;
	*(p++)	= 0x07;
	*(p++)	= 0x00;
	*(p++)	= 0x00;
	*(p++)	= 0x41;
	*(p++)	= 0x06;
	*(p++)	= 0x50;
	*(p++)	= 0x07;
	*(p++)	= 0x07;
	*(p++)	= 0x0f;
	*(p++)	= 0x04;
	*(p++)	= 0x00;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xe0; // set gamma
	p		= para;
	num		= 34;
	*(p++)	= 0x00;
	*(p++)	= 0x0a;
	*(p++)	= 0x0f;
	*(p++)	= 0x2e;
	*(p++)	= 0x33;
	*(p++)	= 0x3f;
	*(p++)	= 0x1d;
	*(p++)	= 0x3e;
	*(p++)	= 0x07;
	*(p++)	= 0x0d;
	*(p++)	= 0x0f;
	*(p++)	= 0x12;
	*(p++)	= 0x15;
	*(p++)	= 0x13;
	*(p++)	= 0x15;
	*(p++)	= 0x10;
	*(p++)	= 0x17;
	*(p++)	= 0x00;
	*(p++)	= 0x0a;
	*(p++)	= 0x0f;
	*(p++)	= 0x2e;
	*(p++)	= 0x33;
	*(p++)	= 0x3f;
	*(p++)	= 0x1d;
	*(p++)	= 0x3e;
	*(p++)	= 0x07;
	*(p++)	= 0x0d;
	*(p++)	= 0x0f;
	*(p++)	= 0x12;
	*(p++)	= 0x15;
	*(p++)	= 0x13;
	*(p++)	= 0x15;
	*(p++)	= 0x10;
	*(p++)	= 0x17;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xba; // set mipi
	p		= para;
	num		= 13;
	*(p++)	= 0x00;
	*(p++)	= 0xa0;
	*(p++)	= 0xc6;
	*(p++)	= 0x00;
	*(p++)	= 0x0a;
	*(p++)	= 0x00;
	*(p++)	= 0x10;
	*(p++)	= 0x30;
	*(p++)	= 0x6f;
	*(p++)	= 0x02;
	*(p++)	= 0x11;
	*(p++)	= 0x18;
	*(p++)	= 0x40;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:39h
	cmd 	= 0xc1; //set dgc function
	p		= para;
	num		= 127;
	*(p++)	= 0x01;
	//r
	*(p++)	= 0x02;
	*(p++)	= 0x08;
	*(p++)	= 0x10;
	*(p++)	= 0x18;
	*(p++)	= 0x1f;
	*(p++)	= 0x28;
	*(p++)	= 0x30;
	*(p++)	= 0x37;
	*(p++)	= 0x3f;
	*(p++)	= 0x49;
	*(p++)	= 0x51;
	*(p++)	= 0x59;
	*(p++)	= 0x60;
	*(p++)	= 0x68;
	*(p++)	= 0x70;
	*(p++)	= 0x79;
	*(p++)	= 0x80;
	*(p++)	= 0x88;
	*(p++)	= 0x8f;
	*(p++)	= 0x97;
	*(p++)	= 0x9f;
	*(p++)	= 0xa7;
	*(p++)	= 0xaf;
	*(p++)	= 0xb8;
	*(p++)	= 0xc0;
	*(p++)	= 0xc8;
	*(p++)	= 0xce;
	*(p++)	= 0xd7;
	*(p++)	= 0xe0;
	*(p++)	= 0xe7;
	*(p++)	= 0xf0;
	*(p++)	= 0xf7;
	*(p++)	= 0xff;
	*(p++)	= 0x5a;
	*(p++)	= 0x7d;
	*(p++)	= 0xc4;
	*(p++)	= 0xec;
	*(p++)	= 0x84;
	*(p++)	= 0x18;
	*(p++)	= 0x5c;
	*(p++)	= 0xd3;
	*(p++)	= 0x80;
	//g
	*(p++)	= 0x02;
	*(p++)	= 0x08;
	*(p++)	= 0x10;
	*(p++)	= 0x18;
	*(p++)	= 0x1f;
	*(p++)	= 0x28;
	*(p++)	= 0x30;
	*(p++)	= 0x37;
	*(p++)	= 0x3f;
	*(p++)	= 0x49;
	*(p++)	= 0x51;
	*(p++)	= 0x59;
	*(p++)	= 0x60;
	*(p++)	= 0x68;
	*(p++)	= 0x70;
	*(p++)	= 0x79;
	*(p++)	= 0x80;
	*(p++)	= 0x88;
	*(p++)	= 0x8f;
	*(p++)	= 0x97;
	*(p++)	= 0x9f;
	*(p++)	= 0xa7;
	*(p++)	= 0xaf;
	*(p++)	= 0xb8;
	*(p++)	= 0xc0;
	*(p++)	= 0xc8;
	*(p++)	= 0xce;
	*(p++)	= 0xd7;
	*(p++)	= 0xe0;
	*(p++)	= 0xe7;
	*(p++)	= 0xf0;
	*(p++)	= 0xf7;
	*(p++)	= 0xff;
	*(p++)	= 0x5a;
	*(p++)	= 0x7d;
	*(p++)	= 0xc4;
	*(p++)	= 0xec;
	*(p++)	= 0x84;
	*(p++)	= 0x18;
	*(p++)	= 0x5c;
	*(p++)	= 0xd3;
	*(p++)	= 0x80;
	//b
	*(p++)	= 0x02;
	*(p++)	= 0x08;
	*(p++)	= 0x10;
	*(p++)	= 0x18;
	*(p++)	= 0x1f;
	*(p++)	= 0x28;
	*(p++)	= 0x30;
	*(p++)	= 0x37;
	*(p++)	= 0x3f;
	*(p++)	= 0x49;
	*(p++)	= 0x51;
	*(p++)	= 0x59;
	*(p++)	= 0x60;
	*(p++)	= 0x68;
	*(p++)	= 0x70;
	*(p++)	= 0x79;
	*(p++)	= 0x80;
	*(p++)	= 0x88;
	*(p++)	= 0x8f;
	*(p++)	= 0x97;
	*(p++)	= 0x9f;
	*(p++)	= 0xa7;
	*(p++)	= 0xaf;
	*(p++)	= 0xb8;
	*(p++)	= 0xc0;
	*(p++)	= 0xc8;
	*(p++)	= 0xce;
	*(p++)	= 0xd7;
	*(p++)	= 0xe0;
	*(p++)	= 0xe7;
	*(p++)	= 0xf0;
	*(p++)	= 0xf7;
	*(p++)	= 0xff;
	*(p++)	= 0x5a;
	*(p++)	= 0x7d;
	*(p++)	= 0xc4;
	*(p++)	= 0xec;
	*(p++)	= 0x84;
	*(p++)	= 0x18;
	*(p++)	= 0x5c;
	*(p++)	= 0xd3;
	*(p++)	= 0x80;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:15h
	cmd 	= 0x3a; //set colmod
	p		= para;
	num		= 1;
	*(p++)	= 0x77;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:05h
	cmd 	= 0x11; //sleep out
	p		= para;
	num		= 0;
	dsi_dcs_wr(0,cmd,para,num);
	LCD_delay_ms(120);


	cmd		= 0x35;
	p		= para;	
	num		= 1;
	*(p++)	= 0x00;	
	dsi_dcs_wr(0,cmd,para,num);


	//data type:05h
	cmd 	= 0x29; //display on
	p		= para;
	num		= 0;
	dsi_dcs_wr(0,cmd,para,num);

	//data type:05h
	cmd 	= 0x2c; //write ram
	p		= para;
	num		= 0;
	dsi_dcs_wr(0,cmd,para,num);



	/*
	cmd 	= 0xb9; //set_extc
	p		= para;
	num		= 3;
	*(p++)	= 0x00;
	*(p++)	= 0x00;
	*(p++)	= 0x00;
	dsi_dcs_wr(0,cmd,para,num);
	lcd_dsi_trnd(0);
	*/
}


static __s32 LCD_open_flow(__u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 50);   //open lcd power, and delay 50ms


	
	LCD_OPEN_FUNC(sel, bm480800_8892ftgb_init, 200);   //open lcd power, and delay 50ms	
	LCD_OPEN_FUNC(sel, TCON_open, 500);     //open lcd controller, and delay 500ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static __s32 LCD_close_flow(__u32 sel)
{	
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, TCON_close, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 1000);   //close lcd power, and delay 1000ms

	return 0;
}

static void LCD_power_on(__u32 sel)
{
	__inf("LCD_power_on\n");
    LCD_POWER_EN(sel, 1);//config lcd_power pin to open lcd power
}

static void LCD_power_off(__u32 sel)
{
    LCD_POWER_EN(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(__u32 sel)
{
	__inf("LCD_bl_open\n");
    LCD_PWM_EN(sel, 1);//open pwm module
    LCD_BL_EN(sel, 1);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(__u32 sel)
{
    LCD_BL_EN(sel, 0);//config lcd_bl_en pin to close lcd backlight
    LCD_PWM_EN(sel, 0);//close pwm module
}

//sel: 0:lcd0; 1:lcd1
static __s32 LCD_user_defined_func(__u32 sel, __u32 para1, __u32 para2, __u32 para3)
{
    return 0;
}

void LCD_get_panel_funs_0(__lcd_panel_fun_t * fun)
{
    __inf("LCD_get_panel_funs_0\n");
#ifdef LCD_PARA_USE_CONFIG
    fun->cfg_panel_info = LCD_cfg_panel_info;//delete this line if you want to use the lcd para define in sys_config1.fex
#endif
    fun->cfg_open_flow = LCD_open_flow;
    fun->cfg_close_flow = LCD_close_flow;
    fun->lcd_user_defined_func = LCD_user_defined_func;
}
EXPORT_SYMBOL(LCD_get_panel_funs_0);

