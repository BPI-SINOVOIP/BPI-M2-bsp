#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()??ékthread_run()
#include <linux/err.h> //IS_ERR()??éPTR_ERR()
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
//#include <mach/sys_config.h>

#include <linux/drv_display.h>
#include "mach/ar100.h"

static char modules_name[64] = "gm7121";
extern __s32 disp_set_tv_func(__u32 sel, __disp_tv_func *func);
#define GM7121_Config(sub_addr,data) tv_i2c_write(sub_addr, data)
void gm7121_init(__disp_tv_mode_t tv_mode);
static __disp_tv_mode_t g_tv_mode;
static __u32 tv_i2c_id = 1;
static __u32 tv_i2c_used = 0;
static __u32 tv_screen_id = 0;
static struct i2c_adapter *tv_i2c_adapter;
static struct i2c_client *tv_i2c_client;
__s32 tv_i2c_write(__u8 sub_addr, __u8 data);
__s32 tv_i2c_read(__u8 sub_addr, __u8 *data);

//voltage(7~33): 0.1v,   e.g. 1 voltage = 0.1v
__s32 axp_dldo3_en(__bool b_en, __u32 voltage)
{
    __u8 data;
    __s32 ret;
    __u8 addr;
    
    pr_warn("axp_dldo3_en b_en=%d, voltage=%d\n", b_en, voltage);

    voltage = (voltage < 7)? 7 :voltage;
    voltage = (voltage > 33)?33:voltage;
    data = voltage - 7;
    addr = 0x17;
    ret = ar100_axp_write_reg(&addr, &data, 1);
    if(ret != 0)
    {
        pr_warn("set dldo3 to %d.%dv fail\n", voltage/10, voltage%10);
    }
    addr = 0x12;
    ret = ar100_axp_read_reg(&addr, &data, 1);
    if(ret != 0)
    {
        pr_warn("axp read reg fail\n");
    }
    addr = 0x12;
    data = (b_en)? (data | 0x20):(data & 0xdf);
    ret = ar100_axp_write_reg(&addr, &data, 1);
    if(ret != 0)
    {
        pr_warn("%s dldo3 fail\n", (b_en)? "enable":"disable");
    }

    return 0;
}

__s32 axp_set_dc1sw(int on_off)
{
    __u8   data;
    __u8   addr;
    __s32  ret;

    addr = 0x12;
    ret = ar100_axp_read_reg(&addr, &data, 1);
    if(ret != 0)
    {
        pr_warn("axp read reg fail\n");
        return -1;
    }
    if(on_off)
    {
        data |= (1 << 7);
    }
    else
    {
        data &= ~(1 << 7);
    }
    ret = ar100_axp_write_reg(&addr, &data, 1);
    if(ret != 0)
    {
        pr_warn("set dc1sw  fail\n");
        return -1;
    }

    return 0;
}

__s32 gm7121_tv_power_on(__u32 on_off)
{
    pr_warn("[TV]gm7121_tv_power_on\n");
    if(on_off)
    {
        axp_set_dc1sw(1);
        axp_dldo3_en(1, 33);
        pr_warn("axp_dldo3_en\n");
    }
    else
    {
        axp_dldo3_en(0, 7);
        axp_set_dc1sw(0);
    }

    return 0;
}

__s32 gm7121_tv_open(void)
{
    gm7121_init(g_tv_mode);

    return 0;
}

__s32 gm7121_tv_close(void)
{
    return 0;
}

__s32 gm7121_tv_set_mode(__disp_tv_mode_t tv_mode)
{
    g_tv_mode = tv_mode;

    return 0;
}

__s32 gm7121_tv_get_hpd_status(void)
{
    return 0;
}

__s32 gm7121_tv_get_mode_support(__disp_tv_mode_t tv_mode)
{
    if(tv_mode == DISP_TV_MOD_PAL || tv_mode == DISP_TV_MOD_NTSC)
        return 1;

    return 0;
}
void gm7121_init(__disp_tv_mode_t tv_mode)
{
    //__disp_tv_mode_t tv_mode = DISP_TV_MOD_PAL;//DISP_TV_MOD_PAL;//DISP_TV_MOD_NTSC;
    //-------------------SAA7121 START-------------------------------	
    pr_warn("[TV]gm7121_init, tv_Mode=%d\n", tv_mode);
#if 0
    GM7121_Config(0x00,0x00);    
    GM7121_Config(0x01,0x00);    
    GM7121_Config(0x02,0x00);    
    GM7121_Config(0x03,0x00);    
    GM7121_Config(0x04,0x00);    
    GM7121_Config(0x05,0x00);    
    GM7121_Config(0x06,0x00);    
    GM7121_Config(0x07,0x00);    
    GM7121_Config(0x08,0x00);    
    GM7121_Config(0x09,0x00);    
    GM7121_Config(0x0A,0x00);    
    GM7121_Config(0x0B,0x00);    
    GM7121_Config(0x0C,0x00);    
    GM7121_Config(0x0D,0x00);    
    GM7121_Config(0x0E,0x00);    
    GM7121_Config(0x0F,0x00);    
    GM7121_Config(0x10,0x00);    
    GM7121_Config(0x11,0x00);    
    GM7121_Config(0x12,0x00);    
    GM7121_Config(0x13,0x00);    
    GM7121_Config(0x14,0x00);    
    GM7121_Config(0x15,0x00);    
    GM7121_Config(0x16,0x00);    
    GM7121_Config(0x17,0x00);    
    GM7121_Config(0x18,0x00);    
    GM7121_Config(0x19,0x00);    
    GM7121_Config(0x1A,0x00);    
    GM7121_Config(0x1B,0x00);    
    GM7121_Config(0x1C,0x00);    
    GM7121_Config(0x1D,0x00);    
    GM7121_Config(0x1E,0x00);    
    GM7121_Config(0x1F,0x00);    
    GM7121_Config(0x20,0x00);    
    GM7121_Config(0x21,0x00);    
    GM7121_Config(0x22,0x00);    
    GM7121_Config(0x23,0x00);    
    GM7121_Config(0x24,0x00);    
    GM7121_Config(0x25,0x00);    
    GM7121_Config(0x26,0x1D);    
    GM7121_Config(0x27,0x05); 
#endif
    GM7121_Config(0x28,0x21);    
    GM7121_Config(0x29,0x1D);    
    GM7121_Config(0x2A,0x00);    
    GM7121_Config(0x2B,0x00);    
    GM7121_Config(0x2C,0x00);    
    GM7121_Config(0x2D,0x00);    
    GM7121_Config(0x2E,0x00);    
    GM7121_Config(0x2F,0x00);    
    GM7121_Config(0x30,0x00);    
    GM7121_Config(0x31,0x00);    
    GM7121_Config(0x32,0x00);    
    GM7121_Config(0x33,0x00);    
    GM7121_Config(0x34,0x00);    
    GM7121_Config(0x35,0x00);    
    GM7121_Config(0x36,0x00);    
    GM7121_Config(0x37,0x00);    
    GM7121_Config(0x38,0x00);    
    GM7121_Config(0x39,0x00);    

    //GM7121_Config(0x3A,0x93);    //color  strape 
    GM7121_Config(0x3A,0x13);   //data	主模式
    //*GM7121_Config(0x3A,0x03);     //sync from rcv1 and rcv2   从模式

    GM7121_Config(0x5A,0x00);    
    GM7121_Config(0x5B,0x6d);    
    GM7121_Config(0x5C,0x9f);    
    //GM7121_Config(0x5D,0x1e);  
    GM7121_Config(0x5E,0x1c);    
    GM7121_Config(0x5F,0x35);    
    GM7121_Config(0x60,0x00); 
     
    if (tv_mode == DISP_TV_MOD_PAL)
	{
	GM7121_Config(0x5D,0x0e);
	    GM7121_Config(0x61,0x06);    //PAL
	    GM7121_Config(0x63,0xCB);    //PAL    
        GM7121_Config(0x64,0x8A);    //PAL 
        GM7121_Config(0x65,0x09);    //PAL  
        GM7121_Config(0x66,0x2A);    //PAL  
	}
	else if (tv_mode == DISP_TV_MOD_NTSC)
	{
	GM7121_Config(0x5D,0x1e);
        GM7121_Config(0x61,0x01);    //NTSC
        GM7121_Config(0x63,0x1f);    //NTSC
        GM7121_Config(0x64,0x7c);    //NTSC
        GM7121_Config(0x65,0xF0);    //NTSC
        GM7121_Config(0x66,0x21);    //NTSC
	}	   	
	else
	{
	GM7121_Config(0x5D,0x0e);
	    GM7121_Config(0x61,0x06);    //PAL
	    GM7121_Config(0x63,0xCB);    //PAL    
        GM7121_Config(0x64,0x8A);    //PAL 
        GM7121_Config(0x65,0x09);    //PAL  
        GM7121_Config(0x66,0x2A);    //PAL 
	}
	
	GM7121_Config(0x62,0x3B);     //RTCI Enable	
#if 0        
	GM7121_Config(0x67,0x00);     
    GM7121_Config(0x68,0x00);    
    GM7121_Config(0x69,0x00);    
    GM7121_Config(0x6A,0x00); 
#endif       
    GM7121_Config(0x6B,0x12);    //主模式
    //GM7121_Config(0x6B,0x20);     // 从模式
    //*GM7121_Config(0x6B,0x00);     // 从模式 7121C 20110105
    
    GM7121_Config(0x6C,0x01);    //主模式	
    //GM7121_Config(0x6C,0x96);    //从模式	
    //GM7121_Config(0x6C,0x51);    //从模式	 7121C 20110105
    //*GM7121_Config(0x6C,0x06);    //从模式
    
    GM7121_Config(0x6D,0x20);    //主模式
    //GM7121_Config(0x6D,0x18);    //从模式 
    //GM7121_Config(0x6D,0x11);    //从模式  7121C 20110105
    //*GM7121_Config(0x6D,0x00);    //从模式 
    
    GM7121_Config(0x6E,0x80);    //video with color	
    GM7121_Config(0x6F,0x00);    
    GM7121_Config(0x70,0x14);    
    GM7121_Config(0x71,0x00);    
    GM7121_Config(0x72,0x00);    
    GM7121_Config(0x73,0x00);    
    GM7121_Config(0x74,0x00);    
    GM7121_Config(0x75,0x00);    
    GM7121_Config(0x76,0x00);     
    GM7121_Config(0x77,0x00);    
    GM7121_Config(0x78,0x00);    
    GM7121_Config(0x79,0x00);    
    GM7121_Config(0x7A,0x16);    
    GM7121_Config(0x7B,0x36);    
    GM7121_Config(0x7C,0x40);    
    GM7121_Config(0x7D,0x00);    
    GM7121_Config(0x7E,0x00);    
    GM7121_Config(0x7F,0x00); 
}


static int tv_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    __disp_tv_func tv_ops;

    pr_info("[DISP_I2C] tv_i2c_probe\n");
    tv_i2c_client = client;
    tv_ops.tv_power_on = gm7121_tv_power_on;
    tv_ops.tv_open = gm7121_tv_open;
    tv_ops.tv_close = gm7121_tv_close;
    tv_ops.tv_get_hpd_status = gm7121_tv_get_hpd_status;
    tv_ops.tv_set_mode = gm7121_tv_set_mode;
    tv_ops.tv_get_mode_support = gm7121_tv_get_mode_support;
    disp_set_tv_func(0, &tv_ops);

    return 0;
}

static int __devexit tv_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id tv_i2c_id_table[] = {
    { "tv_i2c", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, tv_i2c_id_table);
 
int tv_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    if(tv_i2c_id == client->adapter->nr){
        const char *type_name = "tv_i2c";
        tv_i2c_adapter = client->adapter;
        pr_info("[DISP_I2C] tv_i2c_detect, get right i2c adapter, id=%d\n", tv_i2c_adapter->nr);
        strlcpy(info->type, type_name, I2C_NAME_SIZE);
        return 0;
    }
    return -ENODEV;
}
 
/* 0x60为I2C设备地址 */
static  unsigned short normal_i2c[] = {0x46, I2C_CLIENT_END};
 
static struct i2c_driver tv_i2c_driver = {
    .class = I2C_CLASS_HWMON,
    .probe        = tv_i2c_probe,
    .remove        = __devexit_p(tv_i2c_remove),
    .id_table    = tv_i2c_id_table,
    .driver    = {
        .name    = "tv_i2c",
        .owner    = THIS_MODULE,
    },
    .detect        = tv_i2c_detect,
    .address_list    = normal_i2c,
};
 
int  tv_i2c_init(void)
{
    script_item_u   val;
    script_item_value_type_e  type;

    type = script_get_item("tv0_para", "tv_twi_used", &val);
    if(SCIRPT_ITEM_VALUE_TYPE_INT == type)
    {
        tv_i2c_used = val.val;
        if(tv_i2c_used == 1)
        {
            type = script_get_item("tv0_para", "tv_twi_id", &val);
            tv_i2c_id = (type == SCIRPT_ITEM_VALUE_TYPE_INT)? val.val:tv_i2c_id;

            type = script_get_item("tv0_para", "tv_twi_addr", &val);
            normal_i2c[0] = (type == SCIRPT_ITEM_VALUE_TYPE_INT)? val.val:normal_i2c[0];

            return i2c_add_driver(&tv_i2c_driver);
        }
    }
    return 0;
}
 
void tv_i2c_exit(void)
{
    i2c_del_driver(&tv_i2c_driver);
}

__s32 tv_i2c_write(__u8 sub_addr, __u8 data)
{
    __s32 ret = 0;
    __u8 i2c_data[2];
    struct i2c_msg msg;

    if(tv_i2c_used)
    {
        i2c_data[0] = sub_addr;
        i2c_data[1] = data;
        msg.addr = tv_i2c_client->addr;
        msg.flags = 0;
        msg.len = 2;
        msg.buf = i2c_data;
        ret = i2c_transfer(tv_i2c_adapter, &msg, 1);
    }

	return ret;
}

__s32 tv_i2c_read(__u8 sub_addr, __u8 *data)
{
    __s32 ret = 0;
    struct i2c_msg msgs[] = {
    {
    	.addr	= tv_i2c_client->addr,
    	.flags	= 0,
    	.len	= 1,
    	.buf	= &sub_addr,
    },
    {
    	.addr	= tv_i2c_client->addr,
    	.flags	= I2C_M_RD,
    	.len	= 1,
    	.buf	= data,
    },
    };

	if(tv_i2c_used)
    {
        ret = i2c_transfer(tv_i2c_adapter, msgs, 2);
    }

	return ret;
}

int __init gm7121_module_init(void)
{
    pr_info("[TV]==gm7121_module_init finish==\n");

    return tv_i2c_init();
}

static void __exit gm7121_module_exit(void)
{
    pr_info("gm7121_module_exit\n");
    tv_i2c_exit();
}

late_initcall(gm7121_module_init);
module_exit(gm7121_module_exit);

EXPORT_SYMBOL(tv_i2c_write);
EXPORT_SYMBOL(tv_i2c_read);

MODULE_AUTHOR("tyle");
MODULE_DESCRIPTION("gm7121 driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gm7121");
