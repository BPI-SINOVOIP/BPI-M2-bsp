#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/ctp.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/gpio.h> 
#include <mach/sys_config.h> 
#include <linux/gpio.h>

struct ctp_config_info config_info;
EXPORT_SYMBOL_GPL(config_info);

#define CTP_IRQ_NUMBER     (config_info.irq_gpio_number)

static u32 debug_mask = 0;

#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("***CTP***"fmt, ## arg)

int ctp_i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;
	
	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;		
	
	ret=i2c_transfer(client->adapter, &msg,1);
	return ret;
}
EXPORT_SYMBOL(ctp_i2c_write_bytes);

int ctp_i2c_read_bytes_addr16(struct i2c_client *client, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int ret=-1;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr = client->addr;
	msgs[0].len = 2;		//data address
	msgs[0].buf = buf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr = client->addr;
	msgs[1].len = len-2;
	msgs[1].buf = buf+2;
	
	ret=i2c_transfer(client->adapter, msgs, 2);
	return ret;
}
EXPORT_SYMBOL(ctp_i2c_read_bytes_addr16);

bool ctp_i2c_test(struct i2c_client * client)
{
        int ret,retry;
        uint8_t test_data[1] = { 0 };	//only write a data address.
        
        for(retry=0; retry < 2; retry++)
        {
                ret =ctp_i2c_write_bytes(client, test_data, 1);	//Test i2c.
        	if (ret == 1)
        	        break;
        	msleep(50);
        }
        
        return ret==1 ? true : false;
} 
EXPORT_SYMBOL(ctp_i2c_test);

bool ctp_get_int_enable(u32 *enable)
{
        int ret = -1;
        ret = sw_gpio_eint_get_enable(CTP_IRQ_NUMBER,enable);
        if(ret != 0){
                return false;
        }
        return true;
}
EXPORT_SYMBOL(ctp_get_int_enable);
bool ctp_set_int_enable(u32 enable)
{
        u32 sta_enable;
        int ret = -1;
        if((enable != 0) || (enable != 1)){
                return false;
        }
        ret = ctp_get_int_enable(&sta_enable);
        if(ret == true){
                if(sta_enable == enable)
                        return true;
        }
        ret = sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,enable);
        if(ret != 0){
                return false;
        }
                return true;       
}
EXPORT_SYMBOL(ctp_set_int_enable);

bool ctp_get_int_port_rate(u32 *clk)
{
        struct gpio_eint_debounce pdbc = {0,0};
        int ret = -1;
        ret = sw_gpio_eint_get_debounce(CTP_IRQ_NUMBER,&pdbc);
        if(ret != 0){
                return false;
        }
        *clk = pdbc.clk_sel;
        return true;
}
EXPORT_SYMBOL(ctp_get_int_port_rate);

bool ctp_set_int_port_rate(u32 clk)
{
        struct gpio_eint_debounce pdbc;
        int ret = -1;

        if((clk != 0) && (clk != 1)){
                return false;
        }
        ret = sw_gpio_eint_get_debounce(CTP_IRQ_NUMBER,&pdbc);
        if(ret == 0){
                if(pdbc.clk_sel == clk){
                        return true;
                }
        }
        pdbc.clk_sel = clk;
        ret = sw_gpio_eint_set_debounce(CTP_IRQ_NUMBER,pdbc);
        if(ret != 0){
                return false;
        }
        return true;
}
EXPORT_SYMBOL(ctp_set_int_port_rate);
bool ctp_get_int_port_deb(u32 *clk_pre_scl)
{
        struct gpio_eint_debounce pdbc = {0,0};
        int ret = -1;
        ret = sw_gpio_eint_get_debounce(CTP_IRQ_NUMBER,&pdbc);
        if(ret !=0){
                return false;
        }
        *clk_pre_scl = pdbc.clk_pre_scl;
        return true;
} 
EXPORT_SYMBOL(ctp_get_int_port_deb);
bool ctp_set_int_port_deb(u32 clk_pre_scl)
{
        struct gpio_eint_debounce pdbc;
        int ret = -1;

        ret = sw_gpio_eint_get_debounce(CTP_IRQ_NUMBER,&pdbc);
        if(ret ==0){
                if(pdbc.clk_pre_scl == clk_pre_scl){
                        return true;
                }
        }
        pdbc.clk_pre_scl = clk_pre_scl;
        ret = sw_gpio_eint_set_debounce(CTP_IRQ_NUMBER,pdbc);
        if(ret != 0){
                return false;
        }
        return true;        
}
EXPORT_SYMBOL(ctp_set_int_port_deb);  
   
void ctp_free_platform_resource(void)
{
        gpio_free(config_info.wakeup_gpio_number);
#ifdef TOUCH_KEY_LIGHT_SUPPORT
        gpio_free(config_info.key_light_gpio_number);
#endif	
	return;
}
EXPORT_SYMBOL(ctp_free_platform_resource);

/**
 * ctp_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
int ctp_init_platform_resource(void)
{	
	int ret = -1;
	script_item_u   item;
	script_item_value_type_e   type;
        
        type = script_get_item("ctp_para", "ctp_wakeup", &item);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("script_get_item ctp_wakeup err\n");
		return -1;
	}
	config_info.wakeup_gpio_number = item.gpio.gpio;
	
	type = script_get_item("ctp_para", "ctp_int_port", &item);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("script_get_item ctp_int_port type err\n");
		return -1;
	}
        config_info.irq_gpio_number = item.gpio.gpio;
        
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
        type = script_get_item("ctp_para", "ctp_light", &item);
        if(SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("script_get_item ctp_light err\n");
		return -1;
        }
        config_info.key_light_gpio_number = item.gpio.gpio;
        ret = gpio_request_one(config_info.key_light_gpio_number,GPIOF_OUT_INIT_HIGH,NULL);
        if(ret != 0){
                printk("key_light pin set to output function failure!\n");
        } 
#endif
        
        ret = gpio_request_one(config_info.wakeup_gpio_number,GPIOF_OUT_INIT_HIGH,NULL);
        if(ret != 0){
                printk("wakeup pin set to output function failure!\n");
        } 
                    
	return 0;
}
EXPORT_SYMBOL(ctp_init_platform_resource);

void ctp_print_info(struct ctp_config_info info,int debug_level)
{
       if(debug_level == DEBUG_INIT)
       {
                
                dprintk(DEBUG_INIT,"info.ctp_used:%d\n",info.ctp_used);
                dprintk(DEBUG_INIT,"info.twi_id:%d\n",info.twi_id);
                dprintk(DEBUG_INIT,"info.screen_max_x:%d\n",info.screen_max_x);
                dprintk(DEBUG_INIT,"info.screen_max_y:%d\n",info.screen_max_y);
                dprintk(DEBUG_INIT,"info.revert_x_flag:%d\n",info.revert_x_flag);
                dprintk(DEBUG_INIT,"info.revert_y_flag:%d\n",info.revert_y_flag);
                dprintk(DEBUG_INIT,"info.exchange_x_y_flag:%d\n",info.exchange_x_y_flag); 
                dprintk(DEBUG_INIT,"info.irq_gpio_number:%d\n",info.irq_gpio_number);
                dprintk(DEBUG_INIT,"info.wakeup_gpio_number:%d\n",info.wakeup_gpio_number);  
       }      
}
EXPORT_SYMBOL(ctp_print_info);
/**
 * ctp_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_fetch_sysconfig_para(void)
{
	int ret = -1;
	script_item_u   val;
        if(debug_mask == DEBUG_OTHERS_INFO){
	        script_dump_mainkey("ctp_para");
        }
	pr_info("=====%s=====. \n", __func__);

	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_used", &val)){
		pr_err("%s: ctp_used script_get_item  err. \n", __func__);
		goto script_get_item_err;
	}
	config_info.ctp_used = val.val;
	
	if(1 != config_info.ctp_used){
		pr_err("%s: ctp_unused. \n",  __func__);
		return ret;
	}
	
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_twi_id", &val)){
		pr_err("%s: ctp_twi_id script_get_item err. \n",__func__ );
		goto script_get_item_err;
	}
	config_info.twi_id = val.val;
	
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_screen_max_x", &val)){
		pr_err("%s: ctp_screen_max_x script_get_item err. \n",__func__ );
		goto script_get_item_err;
	}
	config_info.screen_max_x = val.val;
	
        if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_screen_max_y", &val)){
        		pr_err("%s: ctp_screen_max_y script_get_item err. \n",__func__ );
        		goto script_get_item_err;
        }
        config_info.screen_max_y = val.val;
        
        if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_revert_x_flag", &val)){
                pr_err("%s: ctp_revert_x_flag script_get_item err. \n",__func__ );
                goto script_get_item_err;
        }
        config_info.revert_x_flag = val.val;
        
        if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_revert_y_flag", &val)){
                pr_err("%s: ctp_revert_y_flag script_get_item err. \n",__func__ );
                goto script_get_item_err;
        }
        config_info.revert_y_flag = val.val;
        
        if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ctp_para", "ctp_exchange_x_y_flag", &val)){
                pr_err("%s: ctp_exchange_x_y_flag script_get_item err. \n",__func__ );
                goto script_get_item_err;
        }
        config_info.exchange_x_y_flag = val.val;

	return 0;

script_get_item_err:
	pr_notice("=========script_get_item_err============\n");
	return ret;
}

/**
 * ctp_wakeup - function
 *
 */
int ctp_wakeup(int status,int ms)
{
       u32 gpio_status;
       dprintk(DEBUG_INIT,"***CTP*** %s:status:%d,ms = %d\n",__func__,status,ms); 
        
        gpio_status = sw_gpio_getcfg(config_info.wakeup_gpio_number);
        if(gpio_status != 1){
                sw_gpio_setcfg(config_info.wakeup_gpio_number,1);
        }
        if(status == 0){
                
                if(ms == 0) {
                         __gpio_set_value(config_info.wakeup_gpio_number, 0);
                }else {
                        __gpio_set_value(config_info.wakeup_gpio_number, 0);
                        msleep(ms);
                        __gpio_set_value(config_info.wakeup_gpio_number, 1);
                }
        }
        if(status == 1){
                if(ms == 0) {
                         __gpio_set_value(config_info.wakeup_gpio_number, 1);
                }else {
                        __gpio_set_value(config_info.wakeup_gpio_number, 1); 
                        msleep(ms);
                        __gpio_set_value(config_info.wakeup_gpio_number, 0); 
                }      
        }
        msleep(5);  
        if(gpio_status != 1){
                sw_gpio_setcfg(config_info.wakeup_gpio_number,gpio_status);
        }
	return 0;
}
EXPORT_SYMBOL(ctp_wakeup);
/**
 * ctp_key_light - function
 *
 */
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
int ctp_key_light(int status,int ms)
{
       u32 gpio_status;
       dprintk(DEBUG_INIT,"***CTP*** %s:status:%d,ms = %d\n",__func__,status,ms); 
        
        gpio_status = sw_gpio_getcfg(config_info.key_light_gpio_number);
        if(gpio_status != 1){
                sw_gpio_setcfg(config_info.key_light_gpio_number,1);
        }
        if(status == 0){
                 if(ms == 0) {
                         __gpio_set_value(config_info.key_light_gpio_number, 0);
                }else {
                        __gpio_set_value(config_info.key_light_gpio_number, 0);
                        msleep(ms);
                        __gpio_set_value(config_info.key_light_gpio_number, 1);
                }
        }
        if(status == 1){
                 if(ms == 0) {
                         __gpio_set_value(config_info.key_light_gpio_number, 1);
                }else{
                        __gpio_set_value(config_info.key_light_gpio_number, 1); 
                        msleep(ms);
                        __gpio_set_value(config_info.key_light_gpio_number, 0); 
                }      
        }
        msleep(10);  
        if(gpio_status != 1){
                sw_gpio_setcfg(config_info.key_light_gpio_number,gpio_status);
        } 
	return 0;
}
EXPORT_SYMBOL(ctp_key_light);
#endif

static int __init ctp_init(void)
{
	int err = -1;
	if (ctp_fetch_sysconfig_para()){
	        printk("%s: ctp_fetch_sysconfig_para err.\n", __func__);
		return 0;
	}else{
                err = ctp_init_platform_resource();
        	if(0 != err){
        		printk("%s:ctp_ops.init_platform_resource err. \n", __func__);    
        	}
        }
        ctp_print_info(config_info,DEBUG_INIT);
        return 0;
}
static void __exit ctp_exit(void)
{
	ctp_free_platform_resource();
	return;
}

module_init(ctp_init);
module_exit(ctp_exit);
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ctp init");
MODULE_AUTHOR("Olina yin");
MODULE_ALIAS("platform:AW");