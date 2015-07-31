#include "dev_disp.h"


extern struct device	*display_dev;
extern fb_info_t g_fbi;

extern __s32 disp_video_set_dit_mode(__u32 scaler_index, __u32 mode);
extern __s32 disp_video_get_dit_mode(__u32 scaler_index);

static __u32 sel;
static __u32 hid;
//#define DISP_CMD_CALLED_BUFFER_LEN 100
//static __u32 disp_cmd_called[DISP_CMD_CALLED_BUFFER_LEN];
//static __u32 disp_cmd_called_index;

#define ____SEPARATOR_GLABOL_NODE____

static ssize_t disp_sel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sel);
}

static ssize_t disp_sel_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        sel = val;
	}
    
	return count;
}

static ssize_t disp_hid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", HANDTOID(hid));
}

static ssize_t disp_hid_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>3) || (val < 0))
    {
        printk("Invalid value, 0~3 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        hid = IDTOHAND(val);
	}
    
	return count;
}
static DEVICE_ATTR(sel, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_sel_show, disp_sel_store);

static DEVICE_ATTR(hid, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hid_show, disp_hid_store);

static ssize_t disp_sys_status_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  ssize_t count = 0;
	int num_screens, screen_id;
	int num_layers, layer_id;
	int hpd;

	num_screens = 2;
	for(screen_id=0; screen_id < num_screens; screen_id ++) {
		count += sprintf(buf + count, "screen %d:\n", screen_id);
		/* output */
		if(BSP_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_LCD) {
			count += sprintf(buf + count, "\tlcd output\tbacklight(%3d)", BSP_disp_lcd_get_bright(screen_id));
			count += sprintf(buf + count, "\t%4dx%4d\n", BSP_disp_get_screen_width(screen_id), 			BSP_disp_get_screen_height(screen_id));
		} else if(BSP_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_HDMI) {
			count += sprintf(buf + count, "\thdmi output");
			if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_50HZ) {
				count += sprintf(buf + count, "\t%16s", "720p50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_60HZ) {
				count += sprintf(buf + count, "\t%16s", "720p60hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_60HZ) {
				count += sprintf(buf + count, "\t%16s", "1080p60hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_50HZ) {
				count += sprintf(buf + count, "\t%16s", "1080p50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_50HZ) {
				count += sprintf(buf + count, "\t%16s", "1080i50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_60HZ) {
				count += sprintf(buf + count, "\t%16s", "1080i60hz");
			}
			count += sprintf(buf + count, "\t%4dx%4d\n", BSP_disp_get_screen_width(screen_id), BSP_disp_get_screen_height(screen_id));
		}

		hpd = BSP_disp_hdmi_get_hpd_status(screen_id); 
		count += sprintf(buf + count, "\t%11s\n", hpd? "hdmi plugin":"hdmi unplug");
		count += sprintf(buf + count, "    type  |  status  |  id  | pipe | z | pre_mult |    alpha   | colorkey |  format  | framebuffer |  	   source crop      |       frame       |   trd   |         address\n");
		count += sprintf(buf + count, "----------+--------+------+------+---+----------+------------+----------+----------+-------------+-----------------------+-------------------+---------+-----------------------------\n");
		num_layers = 4;
		/* layer info */
		for(layer_id=0; layer_id<num_layers; layer_id++) {
			__disp_layer_info_t layer_para;
			int ret;

			ret = BSP_disp_layer_get_para(screen_id, IDTOHAND(layer_id), &layer_para);
			if(ret == 0) {
				count += sprintf(buf + count, " %8s |", (layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)? "SCALER":"NORAML");
				count += sprintf(buf + count, " %8s |", BSP_disp_layer_is_open(screen_id, IDTOHAND(layer_id))? "enable":"disable");
				count += sprintf(buf + count, " %4d |", layer_id);
				count += sprintf(buf + count, " %4d |", layer_para.pipe);
				count += sprintf(buf + count, " %1d |", layer_para.prio);
				count += sprintf(buf + count, " %8s |", (layer_para.fb.pre_multiply)? "Y":"N");
				count += sprintf(buf + count, " %5s(%3d) |", (layer_para.alpha_en)? "globl":"pixel", layer_para.alpha_val);
				count += sprintf(buf + count, " %8s |", (layer_para.ck_enable)? "enable":"disable");
				count += sprintf(buf + count, " %2d,%2d,%2d |", layer_para.fb.mode, layer_para.fb.format, layer_para.fb.seq);
				count += sprintf(buf + count, " [%4d,%4d] |", layer_para.fb.size.width, layer_para.fb.size.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.src_win.x, layer_para.src_win.y, layer_para.src_win.width, layer_para.src_win.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.scn_win.x, layer_para.scn_win.y, layer_para.scn_win.width, layer_para.scn_win.height);
				count += sprintf(buf + count, " [%1d%1d,%1d%1d] |", layer_para.fb.b_trd_src, layer_para.fb.trd_mode, layer_para.b_trd_out, layer_para.out_trd_mode);
				count += sprintf(buf + count, " [%8x,%8x,%8x] |", layer_para.fb.addr[0], layer_para.fb.addr[1], layer_para.fb.addr[2]);
				count += sprintf(buf + count, "\n");
			}
		}
		count += sprintf(buf + count, "\n\tsmart backlight: %s\n", BSP_disp_drc_get_enable(screen_id)? "enable":"disable");

	}

	return count;
}

static ssize_t disp_sys_status_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(sys_status, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_sys_status_show, disp_sys_status_store);


#define ____SEPARATOR_REG_DUMP____
static ssize_t disp_reg_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "there is nothing here!");
}

static ssize_t disp_reg_dump_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
        return -EINVAL;

    if(strnicmp(buf, "be0", 3) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_IMAGE0);
    }else if(strnicmp(buf, "be1", 3) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_IMAGE1);
    }else if(strnicmp(buf, "fe0", 3) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_SCALER0);
    }else if(strnicmp(buf, "fe1", 3) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_SCALER1);
    }else if(strnicmp(buf, "lcd0", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_LCDC0);
    }else if(strnicmp(buf, "lcd1", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_LCDC1);
    }else if(strnicmp(buf, "tve0", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_TVEC0);
    }else if(strnicmp(buf, "tve1", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_TVEC1);
    }else if(strnicmp(buf, "deu0", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_DEU0);
    }else if(strnicmp(buf, "deu1", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_DEU1);
    }else if(strnicmp(buf, "cmu0", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_CMU0);
    }else if(strnicmp(buf, "cmu1", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_CMU1);
    }else if(strnicmp(buf, "drc0", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_DRC0);
    }else if(strnicmp(buf, "drc1", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_DRC1);
    }else if(strnicmp(buf, "dsi", 3) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_DSI);
    }else if(strnicmp(buf, "dphy", 8) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_DSI_DPHY);
    }else if(strnicmp(buf, "hdmi", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_HDMI);
    }else if(strnicmp(buf, "ccmu", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_CCMU);
    }else if(strnicmp(buf, "pioc", 4) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_PIOC);
    }else if(strnicmp(buf, "pwm", 3) == 0)
    {
        BSP_disp_print_reg(1, DISP_REG_PWM);
    }else
    {
        printk("Invalid para!\n");
    }
    
	return count;
}

static DEVICE_ATTR(reg_dump, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_reg_dump_show, disp_reg_dump_store);

#define ____SEPARATOR_PRINT_CMD_LEVEL____
extern __u32 disp_print_cmd_level;
static ssize_t disp_print_cmd_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", disp_print_cmd_level);
}

static ssize_t disp_print_cmd_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val != 0) && (val != 1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        disp_print_cmd_level = val;
	}
    
	return count;
}

static DEVICE_ATTR(print_cmd_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_print_cmd_level_show, disp_print_cmd_level_store);

#define ____SEPARATOR_CMD_PRINT_LEVEL____
extern __u32 disp_cmd_print;
static ssize_t disp_cmd_print_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", disp_cmd_print);
}

static ssize_t disp_cmd_print_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 16, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

   
    disp_cmd_print = val;    
    
	return count;
}

static DEVICE_ATTR(cmd_print, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_cmd_print_show, disp_cmd_print_store);



#define ____SEPARATOR_PRINT_LEVEL____
static ssize_t disp_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "debug=%s\n", bsp_disp_get_print_level()?"on" : "off");
}

static ssize_t disp_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
        return -EINVAL;

    if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
    {
        bsp_disp_set_print_level(1);
	}
    else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	{
        bsp_disp_set_print_level(0);
    }
    else if (strnicmp(buf, "2", 1) == 0)
	{
        bsp_disp_set_print_level(2);
    }
    else
    {
        return -EINVAL;
    }
    
	return count;
}

static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_debug_show, disp_debug_store);

#define ____SEPARATOR_CFG_COUNT____
static ssize_t disp_cfg_cnt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "cfg_cnt=%d\n", bsp_disp_cfg_get(sel));
}

static ssize_t disp_cfg_cnt_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(cfg_cnt, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_cfg_cnt_show, disp_cfg_cnt_store);

static ssize_t disp_cache_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "cache=%s\n", bsp_disp_cmd_cache_get(sel)? "true":"false");
}

static ssize_t disp_cache_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(cache, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_cache_show, disp_cache_store);

#define ____SEPARATOR_LAYER_PARA____
static ssize_t disp_layer_para_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    __disp_layer_info_t layer_para;
    __s32 ret = 0;

    ret = BSP_disp_layer_get_para(sel, hid, &layer_para);
    if(ret != 0)
    {
        return sprintf(buf, "screen%d layer%d is not init\n", sel, HANDTOID(hid));
    }
    else
    {
        return sprintf(buf, "=== screen%d layer%d para ====\nmode: %d\naddr=<%x,%x,%x>\nfb.size=<%dx%d>\nfb.fmt=<%d, %d, %d>\ntrd_src=<%d, %d> trd_out=<%d, %d>\npipe:%d\tprio: %d\nalpha: <%d, %d>\tcolor_key_en: %d\nsrc_window:<%d,%d,%d,%d>\nscreen_window:<%d,%d,%d,%d>\npre_multiply=%d\n======= screen%d layer%d para ====\n", 
        sel, HANDTOID(hid),layer_para.mode, layer_para.fb.addr[0], layer_para.fb.addr[1], layer_para.fb.addr[2],
        layer_para.fb.size.width, layer_para.fb.size.height, layer_para.fb.mode, layer_para.fb.format, 
        layer_para.fb.seq, layer_para.fb.b_trd_src,  layer_para.fb.trd_mode, 
        layer_para.b_trd_out, layer_para.out_trd_mode, layer_para.pipe, 
        layer_para.prio, layer_para.alpha_en, layer_para.alpha_val, 
        layer_para.ck_enable, layer_para.src_win.x, layer_para.src_win.y, 
        layer_para.src_win.width, layer_para.src_win.height, layer_para.scn_win.x, layer_para.scn_win.y, 
        layer_para.scn_win.width, layer_para.scn_win.height,layer_para.fb.pre_multiply, sel, HANDTOID(hid));
    }
}

static ssize_t disp_layer_para_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
    printk("there no room for anything\n");
        
	return count;
}

static DEVICE_ATTR(layer_para, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_para_show, disp_layer_para_store);


#define ____SEPARATOR_SCRIPT_DUMP____
static ssize_t disp_script_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(sel == 0)
    {
        script_dump_mainkey("disp_init");
        script_dump_mainkey("lcd0_para");
        script_dump_mainkey("hdmi_para");
        script_dump_mainkey("power_sply");
        script_dump_mainkey("clock");
        script_dump_mainkey("dram_para");
    }else if(sel == 1)
    {
        script_dump_mainkey("lcd1_para");
    }

    return sprintf(buf, "%s\n", "oh yeah!");
}

static ssize_t disp_script_dump_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char main_key[32];

    if(strlen(buf) == 0) {
		printk("Invalid para\n");
		return -1;
	}

    memcpy(main_key, buf, strlen(buf)+1);

    if(sel == 0)
    {
        script_dump_mainkey("disp_init");
        script_dump_mainkey("lcd0_para");
        script_dump_mainkey("hdmi_para");
        script_dump_mainkey("power_sply");
        script_dump_mainkey("clock");
        script_dump_mainkey("dram_para");
    }else if(sel == 1)
    {
        script_dump_mainkey("lcd1_para");
    }

	return count;
}

static DEVICE_ATTR(script_dump, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_script_dump_show, disp_script_dump_store);


#define ____SEPARATOR_LCD____
static ssize_t disp_lcd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(BSP_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_LCD)
    {
        return sprintf(buf, "screen%d lcd on!\n", sel);
    }
    else
    {
        return sprintf(buf, "screen%d lcd off!\n", sel);
    }
}

static ssize_t disp_lcd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val==0))
    {
        DRV_lcd_close(sel);
    }else
    {
        BSP_disp_hdmi_close(sel);
        DRV_lcd_open(sel);
	}
    
	return count;
}

static DEVICE_ATTR(lcd, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_lcd_show, disp_lcd_store);

static ssize_t disp_lcd_bl_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_lcd_get_bright(sel));
}

static ssize_t disp_lcd_bl_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val < 0) || (val > 255))
    {
        printk("Invalid value, 0~255 is expected!\n");
    }else
    {
        BSP_disp_lcd_set_bright(sel, val, 0);
	}
    
	return count;
}

static DEVICE_ATTR(lcd_bl, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_lcd_bl_show, disp_lcd_bl_store);

static ssize_t disp_lcd_src_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "screen%d lcd src=%d\n", sel, BSP_disp_lcd_get_src(sel));
}

static ssize_t disp_lcd_src_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 6)
    {
        printk("Invalid value, <=6 is expected!\n");
    }else
    {
        BSP_disp_lcd_set_src(sel, val);
	}

	return count;
}

static DEVICE_ATTR(lcd_src, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_lcd_src_show, disp_lcd_src_store);


static ssize_t disp_fps_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	__u32 screen_fps = bsp_disp_get_fps(sel);
    return sprintf(buf, "screen%d fps=%d.%d\n", sel, screen_fps/10,screen_fps%10);
}

static ssize_t disp_fps_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{   
	int err;
    unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 75)
    {
        printk("Invalid value, <=75 is expected!\n");
    }else
    {
        BSP_disp_lcd_set_fps(sel, val);
	}

    return count;
}

static DEVICE_ATTR(fps, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_fps_show, disp_fps_store);



#define ____SEPARATOR_HDMI____
static ssize_t disp_hdmi_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(BSP_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_HDMI)
    {
        return sprintf(buf, "screen%d hdmi on, mode=%d\n", sel, BSP_disp_hdmi_get_mode(sel));
    }
    else
    {
        return sprintf(buf, "screen%d hdmi off!\n", sel);
    }
}

static ssize_t disp_hdmi_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val==0xff))
    {
        BSP_disp_hdmi_close(sel);
    }else
    {
        BSP_disp_hdmi_close(sel);
        if(BSP_disp_hdmi_set_mode(sel,(__disp_tv_mode_t)val) == 0)
        {
            BSP_disp_hdmi_open(sel);
        }
	}
    
	return count;
}

static DEVICE_ATTR(hdmi, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hdmi_show, disp_hdmi_store);

static ssize_t disp_hdmi_hpd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

    return sprintf(buf, "screen%d hdmi hpd=%d\n", sel, BSP_disp_hdmi_get_hpd_status(sel));
}

static ssize_t disp_hdmi_hpd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(hdmi_hpd, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hdmi_hpd_show, disp_hdmi_hpd_store);

static ssize_t disp_tv_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	if(BSP_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_TV)
    {
        return sprintf(buf, "screen%d tv on, mode=%d\n", sel, BSP_disp_tv_get_mode(sel));
    }
    else
    {
        return sprintf(buf, "screen%d tv off!\n", sel);
    }
}

static ssize_t disp_tv_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val==0xff))
    {
        BSP_disp_tv_close(sel);
    }else
    {
        BSP_disp_tv_close(sel);
        if(BSP_disp_tv_set_mode(sel,(__disp_tv_mode_t)val) == 0)
        {
            BSP_disp_tv_open(sel);
        }
	}
	return count;
}
static DEVICE_ATTR(tv, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_tv_show, disp_tv_store);

static ssize_t disp_layer_en_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
    return sprintf(buf, "screen%d layer en =%s\n", sel, BSP_disp_get_disp_layer_enable(sel)? "on":"off");
}

static ssize_t disp_layer_en_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
        return -EINVAL;

    if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
    {
        BSP_disp_set_disp_layer_enable(sel,1);
	}
    else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	{
        BSP_disp_set_disp_layer_enable(sel,0);
    }
    else
    {
        return -EINVAL;
    }
	return count;
}
static DEVICE_ATTR(layer_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_en_show, disp_layer_en_store);

static ssize_t disp_hdmi_cts_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "cts %s\n", BSP_disp_hdmi_get_cts_enable()? "on":"off");
}

static ssize_t disp_hdmi_cts_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
        return -EINVAL;

    if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
    {
        BSP_disp_hdmi_cts_enable(1);
	}
    else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	{
        BSP_disp_hdmi_cts_enable(0);
    }
    else
    {
        return -EINVAL;
    }

	return count;
}

static DEVICE_ATTR(hdmi_cts, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hdmi_cts_show, disp_hdmi_cts_store);


static ssize_t disp_hdmi_test_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "screen%d hdmi_test_mode=%d\n", sel, BSP_disp_hdmi_get_test_mode(sel));
}

static ssize_t disp_hdmi_test_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
        return -EINVAL;

    if (strnicmp(buf, "720p50hz", 8) == 0)
    {
        BSP_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_720P_50HZ);
	}
    else if (strnicmp(buf, "720p60hz", 8) == 0)
    {
        BSP_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_720P_60HZ);
	}
    else if (strnicmp(buf, "1080p50hz", 9) == 0)
    {
        BSP_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_1080P_50HZ);
	}
    else if (strnicmp(buf, "1080p60hz", 9) == 0)
    {
        BSP_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_1080P_60HZ);
	}
    else if (strnicmp(buf, "1080p24hz", 9) == 0)
    {
        BSP_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_1080P_24HZ);
	}
    else if (strnicmp(buf, "576p", 4) == 0)
    {
        BSP_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_576P);
	}
    else if (strnicmp(buf, "480p", 4) == 0)
    {
        BSP_disp_hdmi_set_test_mode(sel, DISP_TV_MOD_480P);
	}
    else
    {
        return -EINVAL;
    }

	return count;
}

static DEVICE_ATTR(hdmi_test_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hdmi_test_mode_show, disp_hdmi_test_mode_store);


#define ____SEPARATOR_VSYNC_EVENT____
static ssize_t disp_vsync_event_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0xff;
}

static ssize_t disp_vsync_event_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        BSP_disp_vsync_event_enable(sel, val);
	}
    
	return count;
}

static DEVICE_ATTR(vsync_event_enable, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_vsync_event_enable_show, disp_vsync_event_enable_store);


#define ____SEPARATOR_LAYER____
static ssize_t disp_layer_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    __disp_layer_info_t para;
    int ret;

    ret = BSP_disp_layer_get_para(sel, hid, &para);
    if(0 == ret)
	{
	    return sprintf(buf, "%d\n", para.mode);
    }else
    {
        return sprintf(buf, "%s\n", "not used!");
    }
}

static ssize_t disp_layer_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    int ret;
    __disp_layer_info_t para;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>4))
    {
        printk("Invalid value, <5 is expected!\n");
        
    }else
    {
        ret = BSP_disp_layer_get_para(sel, hid, &para);
        if(0 == ret)
        {
            para.mode = (__disp_layer_work_mode_t)val;
            if(para.mode == DISP_LAYER_WORK_MODE_SCALER)
            {
                para.scn_win.width = BSP_disp_get_screen_width(sel);
                para.scn_win.height = BSP_disp_get_screen_height(sel);
            }
            BSP_disp_layer_set_para(sel, hid, &para);
        }else
        {
            printk("not used!\n");
        }
	}
    
	return count;
}

static DEVICE_ATTR(layer_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_mode_show, disp_layer_mode_store);



#define ____SEPARATOR_VIDEO_NODE____
static ssize_t disp_video_dit_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (unsigned int)disp_video_get_dit_mode(sel));
}

static ssize_t disp_video_dit_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>3))
    {
        printk("Invalid value, 0~3 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        disp_video_set_dit_mode(sel, (unsigned int)val);
	}
    
	return count;
}
static DEVICE_ATTR(video_dit_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_video_dit_mode_show, disp_video_dit_mode_store);

static ssize_t disp_video_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	__disp_layer_info_t layer_para;
    __disp_video_fb_t video_fb;
    __s32 ret = 0;
    __u32 i;
    __u32 cnt = 0;

    for(i=100; i<104; i++)
    {
        ret = BSP_disp_layer_get_para(sel, i, &layer_para);
        if(ret == 0)
        {
            ret = BSP_disp_video_get_fb(sel, i, &video_fb);
            if(ret == 0)
            {
                cnt += sprintf(buf+cnt, "=== screen%d layer%d para ====\nmode: %d\nfb.size=<%dx%d>\nfb.fmt=<%d, %d, %d>\ntrd_src=<%d, %d> trd_out=<%d, %d>\npipe:%d\tprio: %d\nalpha: <%d, %d>\tcolor_key_en: %d\nsrc_window:<%d,%d,%d,%d>\nscreen_window:<%d,%d,%d,%d>\npre_multiply=%d\n======= screen%d layer%d para ====\n", 
                        sel, HANDTOID(i),layer_para.mode, layer_para.fb.size.width, 
                        layer_para.fb.size.height, layer_para.fb.mode, layer_para.fb.format, 
                        layer_para.fb.seq, layer_para.fb.b_trd_src,  layer_para.fb.trd_mode, 
                        layer_para.b_trd_out, layer_para.out_trd_mode, layer_para.pipe, 
                        layer_para.prio, layer_para.alpha_en, layer_para.alpha_val, 
                        layer_para.ck_enable, layer_para.src_win.x, layer_para.src_win.y, 
                        layer_para.src_win.width, layer_para.src_win.height, layer_para.scn_win.x, layer_para.scn_win.y, 
                        layer_para.scn_win.width, layer_para.scn_win.height,layer_para.fb.pre_multiply, sel, HANDTOID(i));

                cnt += sprintf(buf+cnt, "=== video info ==\nid=%d\n%s\nmaf_valid=%d\tfre_frame_valid=%d\ttop_field_first=%d\n=== video info ===\n",
                    video_fb.id, (video_fb.interlace? "Interlace":"Progressive"), video_fb.maf_valid, video_fb.pre_frame_valid, video_fb.top_field_first);
            }
        }
    }

    return cnt;
    
}

static ssize_t disp_video_info_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{ 
	return count;
}
static DEVICE_ATTR(video_info, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_video_info_show, disp_video_info_store);

static ssize_t disp_video_fps_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	__u32 i;
    __u32 fps;
    __u32 cnt = 0;

    for(i=100; i<104; i++)
    {
        if(BSP_disp_video_get_start(sel, i) == 1)
        {
            fps = bsp_disp_video_get_fps(sel, i);
            cnt += sprintf(buf+cnt, "screen%d layer%d fps=%d.%d\n", sel, HANDTOID(i), fps/10,fps%10);
        }
    }
    return cnt;
}

static ssize_t disp_video_fps_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{   
	return count;
}

static DEVICE_ATTR(video_fps, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_video_fps_show, disp_video_fps_store);



#define ____SEPARATOR_DEU_NODE____
static ssize_t disp_deu_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_deu_get_enable(sel, hid));
}

static ssize_t disp_deu_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_enable(sel, hid, (unsigned int)val);
	}
    
	return count;
}


static ssize_t disp_deu_luma_sharp_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_deu_get_luma_sharp_level(sel, hid));
}

static ssize_t disp_deu_luma_sharp_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_luma_sharp_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_deu_chroma_sharp_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_deu_get_chroma_sharp_level(sel, hid));
}

static ssize_t disp_deu_chroma_sharp_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_chroma_sharp_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_deu_black_exten_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_deu_get_black_exten_level(sel, hid));
}

static ssize_t disp_deu_black_exten_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_black_exten_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_deu_white_exten_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_deu_get_white_exten_level(sel, hid));
}

static ssize_t disp_deu_white_exten_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_white_exten_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static DEVICE_ATTR(deu_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_enable_show, disp_deu_enable_store);

static DEVICE_ATTR(deu_luma_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_luma_sharp_level_show, disp_deu_luma_sharp_level_store);

static DEVICE_ATTR(deu_chroma_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_chroma_sharp_level_show, disp_deu_chroma_sharp_level_store);

static DEVICE_ATTR(deu_black_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_black_exten_level_show, disp_deu_black_exten_level_store);

static DEVICE_ATTR(deu_white_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_white_exten_level_show, disp_deu_white_exten_level_store);



#define ____SEPARATOR_LAYER_ENHANCE_NODE____
static ssize_t disp_layer_enhance_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_layer_get_enable(sel, hid));
}

static ssize_t disp_layer_enhance_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_layer_enable(sel, hid, (unsigned int)bright_val);
	}
    
	return count;
}


static ssize_t disp_layer_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_layer_get_bright(sel, hid));
}

static ssize_t disp_layer_bright_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_layer_set_bright(sel, hid, (unsigned int)bright_val);
	}
    
	return count;
}

static ssize_t disp_layer_contrast_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_layer_get_contrast(sel, hid));
}

static ssize_t disp_layer_contrast_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long contrast_val;
    
	err = strict_strtoul(buf, 10, &contrast_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(contrast_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", contrast_val);
        BSP_disp_cmu_layer_set_contrast(sel, hid, (unsigned int)contrast_val);
	}
    
	return count;
}

static ssize_t disp_layer_saturation_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_layer_get_saturation(sel, hid));
}

static ssize_t disp_layer_saturation_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long saturation_val;
    
	err = strict_strtoul(buf, 10, &saturation_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(saturation_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", saturation_val);
        BSP_disp_cmu_layer_set_saturation(sel, hid,(unsigned int)saturation_val);
	}
    
	return count;
}

static ssize_t disp_layer_hue_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_layer_get_hue(sel,hid));
}

static ssize_t disp_layer_hue_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long hue_val;
    
	err = strict_strtoul(buf, 10, &hue_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(hue_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", hue_val);
        BSP_disp_cmu_layer_set_hue(sel, hid,(unsigned int)hue_val);
	}
    
	return count;
}

static ssize_t disp_layer_enhance_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_layer_get_mode(sel,hid));
}

static ssize_t disp_layer_enhance_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long mode_val;
    
	err = strict_strtoul(buf, 10, &mode_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(mode_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", mode_val);
        BSP_disp_cmu_layer_set_mode(sel, hid,(unsigned int)mode_val);
	}
    
	return count;
}

#define ____SEPARATOR_SCREEN_ENHANCE_NODE____
static ssize_t disp_enhance_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_get_enable(sel));
}

static ssize_t disp_enhance_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_enable(sel,(unsigned int)bright_val);
	}
    
	return count;
}



static ssize_t disp_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_get_bright(sel));
}

static ssize_t disp_bright_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_set_bright(sel, (unsigned int)bright_val);
	}
    
	return count;
}

static ssize_t disp_contrast_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_get_contrast(sel));
}

static ssize_t disp_contrast_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long contrast_val;
    
	err = strict_strtoul(buf, 10, &contrast_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(contrast_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", contrast_val);
        BSP_disp_cmu_set_contrast(sel, (unsigned int)contrast_val);
	}
    
	return count;
}

static ssize_t disp_saturation_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_get_saturation(sel));
}

static ssize_t disp_saturation_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long saturation_val;
    
	err = strict_strtoul(buf, 10, &saturation_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(saturation_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", saturation_val);
        BSP_disp_cmu_set_saturation(sel, (unsigned int)saturation_val);
	}
    
	return count;
}

static ssize_t disp_hue_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_get_hue(sel));
}

static ssize_t disp_hue_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long hue_val;
    
	err = strict_strtoul(buf, 10, &hue_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(hue_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", hue_val);
        BSP_disp_cmu_set_hue(sel, (unsigned int)hue_val);
	}
    
	return count;
}

static ssize_t disp_enhance_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_cmu_get_mode(sel));
}

static ssize_t disp_enhance_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long mode_val;
    
	err = strict_strtoul(buf, 10, &mode_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(mode_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", mode_val);
        BSP_disp_cmu_set_mode(sel, (unsigned int)mode_val);
	}
    
	return count;
}

static DEVICE_ATTR(layer_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_enhance_enable_show, disp_layer_enhance_enable_store);

static DEVICE_ATTR(layer_bright, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_bright_show, disp_layer_bright_store);

static DEVICE_ATTR(layer_contrast, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_contrast_show, disp_layer_contrast_store);

static DEVICE_ATTR(layer_saturation, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_saturation_show, disp_layer_saturation_store);

static DEVICE_ATTR(layer_hue, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_hue_show, disp_layer_hue_store);

static DEVICE_ATTR(layer_enhance_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_enhance_mode_show, disp_layer_enhance_mode_store);


static DEVICE_ATTR(screen_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_enhance_enable_show, disp_enhance_enable_store);

static DEVICE_ATTR(screen_bright, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_bright_show, disp_bright_store);

static DEVICE_ATTR(screen_contrast, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_contrast_show, disp_contrast_store);

static DEVICE_ATTR(screen_saturation, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_saturation_show, disp_saturation_store);

static DEVICE_ATTR(screen_hue, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hue_show, disp_hue_store);

static DEVICE_ATTR(screen_enhance_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_enhance_mode_show, disp_enhance_mode_store);


#define ____SEPARATOR_DRC_NODE____
static ssize_t disp_drc_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_drc_get_enable(sel));
}

static ssize_t disp_drc_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val != 0) && (val != 1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_drc_enable(sel, (unsigned int)val);
	}
    
	return count;
}


static DEVICE_ATTR(drc_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_drc_enable_show, disp_drc_enable_store);

#define ____SEPARATOR_COLORBAR____
static ssize_t disp_colorbar_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "there is nothing here!");
}

extern __s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var);

static ssize_t disp_colorbar_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>7)) {
        printk("Invalid value, 0~7 is expected!\n");
    }
    else {
        fb_draw_colorbar((__u32)g_fbi.fbinfo[val]->screen_base, g_fbi.fbinfo[val]->var.xres, 
            g_fbi.fbinfo[val]->var.yres, &(g_fbi.fbinfo[val]->var));;
	}
    
	return count;
}

static DEVICE_ATTR(colorbar, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_colorbar_show, disp_colorbar_store);

#define ____SEPARATOR_GAMMA_TEST____
static ssize_t disp_gamma_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "there is nothing here!");
}

static ssize_t disp_gamma_test_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val == 1))
    {
         __u32 gamma_tab[256] = 
        { 
            0x00000000,0x00010101,0x00020202,0x00030303,0x00040404,0x00050505,0x00060606,0x00070707,
            0x00080808,0x00090909,0x000A0A0A,0x000B0B0B,0x000C0C0C,0x000D0D0D,0x000D0D0D,0x000E0E0E,
            0x000F0F0F,0x00101010,0x00111111,0x00111111,0x00121212,0x00131313,0x00141414,0x00141414,
            0x00151515,0x00161616,0x00161616,0x00171717,0x00181818,0x00191919,0x00191919,0x001A1A1A,
            0x001B1B1B,0x001B1B1B,0x001C1C1C,0x001D1D1D,0x001E1E1E,0x001E1E1E,0x001F1F1F,0x00202020,
            0x00212121,0x00212121,0x00222222,0x00232323,0x00242424,0x00242424,0x00252525,0x00262626,
            0x00272727,0x00282828,0x00292929,0x00292929,0x002A2A2A,0x002B2B2B,0x002C2C2C,0x002D2D2D,
            0x002E2E2E,0x002F2F2F,0x00303030,0x00313131,0x00313131,0x00323232,0x00333333,0x00343434,
            0x00353535,0x00363636,0x00373737,0x00383838,0x00393939,0x003A3A3A,0x003B3B3B,0x003C3C3C,
            0x003D3D3D,0x003E3E3E,0x003F3F3F,0x00404040,0x00414141,0x00424242,0x00434343,0x00444444,
            0x00454545,0x00464646,0x00474747,0x00484848,0x004A4A4A,0x004B4B4B,0x004C4C4C,0x004D4D4D,
            0x004E4E4E,0x004F4F4F,0x00505050,0x00515151,0x00525252,0x00535353,0x00555555,0x00565656,
            0x00575757,0x00585858,0x00595959,0x005A5A5A,0x005B5B5B,0x005C5C5C,0x005E5E5E,0x005F5F5F,
            0x00606060,0x00616161,0x00626262,0x00636363,0x00656565,0x00666666,0x00676767,0x00686868,
            0x00696969,0x006B6B6B,0x006C6C6C,0x006D6D6D,0x006E6E6E,0x006F6F6F,0x00717171,0x00727272,
            0x00737373,0x00747474,0x00757575,0x00777777,0x00787878,0x00797979,0x007A7A7A,0x007B7B7B,
            0x007D7D7D,0x007E7E7E,0x007F7F7F,0x00808080,0x00828282,0x00838383,0x00848484,0x00858585,
            0x00868686,0x00888888,0x00898989,0x008A8A8A,0x008B8B8B,0x008D8D8D,0x008E8E8E,0x008F8F8F,
            0x00909090,0x00929292,0x00939393,0x00949494,0x00959595,0x00979797,0x00989898,0x00999999,
            0x009A9A9A,0x009B9B9B,0x009D9D9D,0x009E9E9E,0x009F9F9F,0x00A0A0A0,0x00A2A2A2,0x00A3A3A3,
            0x00A4A4A4,0x00A5A5A5,0x00A6A6A6,0x00A8A8A8,0x00A9A9A9,0x00AAAAAA,0x00ABABAB,0x00ACACAC,
            0x00AEAEAE,0x00AFAFAF,0x00B0B0B0,0x00B1B1B1,0x00B2B2B2,0x00B4B4B4,0x00B5B5B5,0x00B6B6B6,
            0x00B7B7B7,0x00B8B8B8,0x00B9B9B9,0x00BBBBBB,0x00BCBCBC,0x00BDBDBD,0x00BEBEBE,0x00BFBFBF,
            0x00C0C0C0,0x00C1C1C1,0x00C3C3C3,0x00C4C4C4,0x00C5C5C5,0x00C6C6C6,0x00C7C7C7,0x00C8C8C8,
            0x00C9C9C9,0x00CACACA,0x00CBCBCB,0x00CDCDCD,0x00CECECE,0x00CFCFCF,0x00D0D0D0,0x00D1D1D1,
            0x00D2D2D2,0x00D3D3D3,0x00D4D4D4,0x00D5D5D5,0x00D6D6D6,0x00D7D7D7,0x00D8D8D8,0x00D9D9D9,
            0x00DADADA,0x00DBDBDB,0x00DCDCDC,0x00DDDDDD,0x00DEDEDE,0x00DFDFDF,0x00E0E0E0,0x00E1E1E1,
            0x00E2E2E2,0x00E3E3E3,0x00E4E4E4,0x00E5E5E5,0x00E5E5E5,0x00E6E6E6,0x00E7E7E7,0x00E8E8E8,
            0x00E9E9E9,0x00EAEAEA,0x00EBEBEB,0x00ECECEC,0x00ECECEC,0x00EDEDED,0x00EEEEEE,0x00EFEFEF,
            0x00F0F0F0,0x00F0F0F0,0x00F1F1F1,0x00F2F2F2,0x00F3F3F3,0x00F3F3F3,0x00F4F4F4,0x00F5F5F5,
            0x00F6F6F6,0x00F6F6F6,0x00F7F7F7,0x00F8F8F8,0x00F8F8F8,0x00F9F9F9,0x00FAFAFA,0x00FAFAFA,
            0x00FBFBFB,0x00FCFCFC,0x00FCFCFC,0x00FDFDFD,0x00FDFDFD,0x00FEFEFE,0x00FEFEFE,0x00FFFFFF
        };

            
            BSP_disp_set_gamma_table(sel, (__u32 *)gamma_tab,1024);
            BSP_disp_gamma_correction_enable(sel);
    }
    else
    {
        BSP_disp_gamma_correction_disable(sel);
	}
    
	return count;
}

static DEVICE_ATTR(gamma_test, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_gamma_test_show, disp_gamma_test_store);

static struct attribute *disp_attributes[] = {
    &dev_attr_screen_enhance_en.attr,
    &dev_attr_screen_bright.attr,
    &dev_attr_screen_contrast.attr,
    &dev_attr_screen_saturation.attr,
    &dev_attr_screen_hue.attr,
    &dev_attr_screen_enhance_mode.attr,
    &dev_attr_layer_enhance_en.attr,
    &dev_attr_layer_bright.attr,
    &dev_attr_layer_contrast.attr,
    &dev_attr_layer_saturation.attr,
    &dev_attr_layer_hue.attr,
    &dev_attr_layer_enhance_mode.attr,
    &dev_attr_drc_en.attr,
    &dev_attr_deu_en.attr,
    &dev_attr_deu_luma_level.attr,
    &dev_attr_deu_chroma_level.attr,
    &dev_attr_deu_black_level.attr,
    &dev_attr_deu_white_level.attr,
    &dev_attr_video_dit_mode.attr,
    &dev_attr_sel.attr,
    &dev_attr_hid.attr,
    &dev_attr_reg_dump.attr,
    &dev_attr_layer_mode.attr,
    &dev_attr_vsync_event_enable.attr,
    &dev_attr_lcd.attr,
    &dev_attr_lcd_bl.attr,
    &dev_attr_hdmi.attr,
    &dev_attr_script_dump.attr,
    &dev_attr_colorbar.attr,
    &dev_attr_layer_para.attr,
    &dev_attr_hdmi_hpd.attr,
    &dev_attr_print_cmd_level.attr,
    &dev_attr_cmd_print.attr,
    &dev_attr_gamma_test.attr,
    &dev_attr_debug.attr,
    &dev_attr_fps.attr,
    &dev_attr_video_info.attr,
    &dev_attr_video_fps.attr,
    &dev_attr_lcd_src.attr,
    &dev_attr_hdmi_cts.attr,
    &dev_attr_hdmi_test_mode.attr,
    &dev_attr_cfg_cnt.attr,
    &dev_attr_cache.attr,
    &dev_attr_sys_status.attr,
    &dev_attr_tv.attr,
    &dev_attr_layer_en.attr,
		NULL
};

static struct attribute_group disp_attribute_group = {
	.name = "attr",
	.attrs = disp_attributes
};

int disp_attr_node_init(void)
{
    unsigned int ret;

    ret = sysfs_create_group(&display_dev->kobj,
                             &disp_attribute_group);
    sel = 0;
    hid = 100;
    return 0;
}

int disp_attr_node_exit(void)
{
    return 0;
}
