//#define PLATFORM    "A13"
//#define PLATFORM    "A10"
//#define PLATFORM    "A31"
#define PLATFORM        31

#define TRY_ALLWIN_ENABALE_IRQ_FUC  0
//#define ILI_UPDATE_FW

#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#if ((PLATFORM == 10)||(PLATFORM == 13)||(PLATFORM == 31))
#include <linux/gpio.h>
#endif
#include <linux/regulator/consumer.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#endif

#if ((PLATFORM == 31))
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/ctp.h>
#endif

#if ((PLATFORM == 10)||(PLATFORM == 13))
#include <mach/sys_config.h>
#include "../ctp_platform_ops.h"
#endif




#if ((PLATFORM == 31))
extern struct ctp_config_info config_info;
#define IRQ_NUMBER          (config_info.irq_gpio_number)
#endif

#if ((PLATFORM == 10)||(PLATFORM == 13))
#define IRQ_NUMBER          SW_INT_IRQNO_PIO
static int gpio_int_hdle = 0;
static int	int_cfg_addr[]={PIO_INT_CFG0_OFFSET,PIO_INT_CFG1_OFFSET,
			PIO_INT_CFG2_OFFSET, PIO_INT_CFG3_OFFSET};


static void* __iomem gpio_addr = NULL;
#endif

#if (PLATFORM == 10)
#define IRQ_GPIO			(IRQ_EINT21)
#endif

#if (PLATFORM == 13)
#define IRQ_GPIO			(IRQ_EINT11)
#endif

#if ((PLATFORM == 10)||(PLATFORM == 13)||(PLATFORM == 31)||(PLATFORM == 31))
static int twi_id = 0;
#endif



//#define RESET_GPIO  XXXXXXXXXXXXXXXXXXXX

#ifdef RESET_GPIO
static int reset_pin = RESET_GPIO;
#else
static int reset_pin = 0;
#endif

#ifdef IRQ_GPIO
static int irq_gpio = IRQ_GPIO;
#else 
//static int irq_gpio = 0;

#endif


//static int irq_number = 0;



static char EXCHANG_XY = 1;
static char REVERT_X = 1;
static char REVERT_Y = 1;
static char DBG_FLAG,DBG_COR;

#define DBG(fmt, args...)   if (DBG_FLAG)printk("%s(%d): " fmt, __func__,__LINE__,  ## args)
#define DBG_CO(fmt, args...)   if (DBG_FLAG||DBG_COR)printk("%s: " fmt, "ilitek",  ## args)



#ifdef ILI_UPDATE_FW
unsigned char CTPM_FW[]={
	#include "ilitek.ili"
};
#endif


// declare touch point data
/*struct touch_data {
	// x, y value
	int x, y;
	// check wehther this point is valid or not
	int valid;
	// id information
	int id;
};
*/
// declare i2c data member
struct i2c_data {
	// input device
        struct input_dev *input_dev;
        // i2c client
        struct i2c_client *client;
        // polling thread
        struct task_struct *thread;
        //firmware version
        unsigned char firmware_ver[4];
        // maximum x
        int max_x;
        // maximum y
        int max_y;
	    // maximum touch point
    	int max_tp;
    	// maximum key button
    	int max_btn;
        // the total number of x channel
        int x_ch;
        // the total number of y channel
        int y_ch;
        // check whether i2c driver is registered success
        int valid_i2c_register;
        // check whether input driver is registered success
        int valid_input_register;
	// check whether the i2c enter suspend or not
	int stop_polling;
	// read semaphore
	struct semaphore wr_sem;
	// protocol version
	int protocol_ver;
	int set_polling_mode;
	// valid irq request
	int valid_irq_request;
	//reset request flag
	int reset_request_success;
	// work queue for interrupt use only
	struct workqueue_struct *irq_work_queue;
	// work struct for work queue
	struct work_struct irq_work;
	
    struct timer_list timer;
	int reset_gpio;
	int irq_status;
	//irq_status enable:1 disable:0
	struct completion complete;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

#if (PLATFORM == 31)
    int irq_handler;        // return form sw_gpio_irq_request;
#endif
};

static struct i2c_data i2c;


// all implemented global functions must be defined in here 
// in order to know how many function we had implemented
#ifdef ILI_UPDATE_FW
static int ilitek_upgrade_firmware(void);
#endif

static int ilitek_i2c_register_device(void);
static void ilitek_set_input_param(struct input_dev*, int, int, int);
static int ilitek_i2c_read_tp_info(void);
static int ilitek_i2c_reread_tp_info(void);

static int ilitek_init(void);
static void ilitek_exit(void);

// i2c functions
static int ilitek_i2c_transfer(struct i2c_client*, struct i2c_msg*, int);
static int ilitek_i2c_read(struct i2c_client*, uint8_t, uint8_t*, int);
static int ilitek_i2c_process_and_report(void);
static int ilitek_i2c_suspend(struct i2c_client*, pm_message_t);
static int ilitek_i2c_resume(struct i2c_client*);
static void ilitek_i2c_shutdown(struct i2c_client*);
static int ilitek_i2c_probe(struct i2c_client*, const struct i2c_device_id*);
static int ilitek_i2c_remove(struct i2c_client*);
#ifdef CONFIG_HAS_EARLYSUSPEND
        static void ilitek_i2c_early_suspend(struct early_suspend *h);
        static void ilitek_i2c_late_resume(struct early_suspend *h);
#endif
static int ilitek_i2c_polling_thread(void*);
static irqreturn_t ilitek_i2c_isr(int, void*);
static void ilitek_i2c_irq_work_queue_func(struct work_struct*);

// file operation functions
static int ilitek_file_open(struct inode*, struct file*);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long ilitek_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
static int  ilitek_file_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
static int ilitek_file_open(struct inode*, struct file*);
static ssize_t ilitek_file_write(struct file*, const char*, size_t, loff_t*);
static ssize_t ilitek_file_read(struct file*, char*, size_t, loff_t*);
static int ilitek_file_close(struct inode*, struct file*);

static void ilitek_i2c_irq_enable(void);//luca 20120120
static void ilitek_i2c_irq_disable(void);//luca 20120120




static void ilitek_reset(int reset_gpio)
{
    DBG("Enter\n");
    #if ((PLATFORM == 31))    
    //ctp_wakeup(0,100);
    //msleep(100);
    ctp_wakeup(0,100);
    msleep(100);
    return;
    #endif
    
    #if ((PLATFORM == 10)||(PLATFORM == 13))
    gpio_write_one_pin_value(reset_gpio, 1, "ctp_wakeup");
    msleep(100);
    gpio_write_one_pin_value(reset_gpio, 0, "ctp_wakeup");
    msleep(100);
    gpio_write_one_pin_value(reset_gpio, 1, "ctp_wakeup");
    msleep(100);
    return;
    #endif
    
	
    gpio_direction_output(reset_gpio,1);
	msleep(100);
    gpio_direction_output(reset_gpio,0);
    msleep(100);
	gpio_direction_output(reset_gpio,1);
	msleep(100);
	return;
}

/*
judge if driver should loading and do some previous work,
if do not loading driver,return value should < 0
*/
static int ilitek_should_load_driver(void)
{
    //add judge code here
    return 0;
}

#if ((PLATFORM == 10)||(PLATFORM == 13)||(PLATFORM == 31))
static struct i2c_board_info i2c_info_dev =  {
	I2C_BOARD_INFO("ilitek_i2c", 0x41),
	.platform_data	= NULL,
};

static int add_ctp_device(void) {
	//int twi_id = 0;
	//char name[I2C_NAME_SIZE];
	struct i2c_adapter *adap;

	//script_parser_fetch("ctp_para", "ctp_twi_id", &twi_id, 1);

	adap = i2c_get_adapter(twi_id);
	i2c.client = i2c_new_device(adap, &i2c_info_dev);

	return 0;
}
#endif


#if ((PLATFORM == 31))
/*
description
	read calibration status
prarmeters
	count
	    buffer length
return
	status
*/
static int ctp_get_system_config(void)
{   
        //ctp_print_info(config_info);
        twi_id = config_info.twi_id;
        //screen_max_x = config_info.screen_max_x;
        //screen_max_y = config_info.screen_max_y;

        REVERT_X = config_info.revert_x_flag;
        REVERT_Y = config_info.revert_y_flag;
        EXCHANG_XY = config_info.exchange_x_y_flag;
        if(twi_id == 0){
                pr_err("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}
#endif


#if ((PLATFORM == 10)||(PLATFORM == 13))
static int ctp_get_system_config(void)
{    
    script_parser_fetch("ctp_para", "ctp_twi_id", &twi_id, 1);
    return 1;
}

static void set_pin_mul_sel(int gpio_hdle, const char * gpio_name, int mul_sel) {
    user_gpio_set_t gpio_status;
    int ret;

    memset(&gpio_status, 0x00, sizeof(gpio_status));
    ret = gpio_get_one_pin_status(gpio_hdle, &gpio_status, gpio_name, 1);
    if (ret != EGPIO_SUCCESS) {
        pr_err("%s, %d, %s\n", __FUNCTION__, __LINE__, gpio_name);
    }

    gpio_status.mul_sel = mul_sel;
    ret = gpio_set_one_pin_status(gpio_hdle, &gpio_status, gpio_name, 1);
    if (ret != EGPIO_SUCCESS) {
        pr_err("%s, gpio change status err, %d, %s\n", __FUNCTION__, ret, gpio_name);
    }

    // test
    memset(&gpio_status, 0x00, sizeof(gpio_status));
    ret = gpio_get_one_pin_status(gpio_hdle, &gpio_status, gpio_name, 1);
    if (ret != EGPIO_SUCCESS || gpio_status.mul_sel != mul_sel) {
        pr_err("%s, %d, %s\n", __FUNCTION__, __LINE__, gpio_name);
    }
}

static void cfg_gpio_interrupt(void) {
    int reg_val;

    //Config IRQ_EINT21 Negative Edge Interrupt
    //最終結果設置寄存器(gpio_addr + PIO_INT_CFG2_OFFSET)
    //21至23位為011(A10)或001(A13)
    reg_val = readl(gpio_addr + PIO_INT_CFG2_OFFSET);
    reg_val &=(~(7<<20));//&
    #if (PLATFORM == 10) 
    reg_val |=(3<<20);
    #endif     
    #if (PLATFORM == 13) 
    reg_val |=(1<<20);
    #endif
    writel(reg_val,gpio_addr + PIO_INT_CFG2_OFFSET);

    //Enable IRQ_EINT21 of PIO Interrupt
    reg_val = readl(gpio_addr + PIO_INT_CTRL_OFFSET);
    reg_val |=(1<<IRQ_GPIO);//把第IRQ_GPIO位 置 1
    writel(reg_val,gpio_addr + PIO_INT_CTRL_OFFSET);
}


/**
 * ctp_clear_penirq - clear int pending
 *
 */
static void ctp_clear_penirq(void)
{
	int reg_val;
	//clear the IRQ_EINT29 interrupt pending
	//pr_info("clear pend irq pending\n");
	reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);
	//writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
	//writel(reg_val&(1<<(IRQ_EINT21)),gpio_addr + PIO_INT_STAT_OFFSET);
	if((reg_val = (reg_val&(1<<(IRQ_GPIO))))){
		printk("==CTP_IRQ_NO=\n");              
		writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
	}
	return;
}



/**
 * ctp_set_irq_mode - according sysconfig's subkey "ctp_int_port" to config int port.
 * 
 * return value: 
 *              0:      success;
 *              others: fail; 
 */
 //ctp_set_irq_mode("ctp_para", "ctp_int_port", IRQ_EINT21, NEGATIVE_EDGE);				
static int ctp_set_irq_mode(char *major_key , char *subkey, int ext_int_num, ext_int_mode int_mode)
{
	int ret = 0;
	__u32 reg_num = 0;
	__u32 reg_addr = 0;
	__u32 reg_val = 0;
	//config gpio to int mode
	pr_info("%s: config gpio to int mode. \n", __func__);
#ifndef SYSCONFIG_GPIO_ENABLE
#else
	if(gpio_int_hdle){
		gpio_release(gpio_int_hdle, 2);
	}
	gpio_int_hdle = gpio_request_ex(major_key, subkey);
	if(!gpio_int_hdle){
		pr_info("request tp_int_port failed. \n");
		ret = -1;
		goto request_tp_int_port_failed;
	}
#endif

#ifdef AW_GPIO_INT_API_ENABLE
#else
	pr_info(" INTERRUPT CONFIG\n");
	reg_num = ext_int_num%8;//
	reg_addr = ext_int_num/8; // 
	reg_val = readl(gpio_addr + int_cfg_addr[reg_addr]);
	reg_val &= (~(7 << (reg_num * 4)));
	reg_val |= (int_mode << (reg_num * 4));
	writel(reg_val,gpio_addr+int_cfg_addr[reg_addr]);
                                                               
	ctp_clear_penirq();
                                                               
	reg_val = readl(gpio_addr+PIO_INT_CTRL_OFFSET); 
	reg_val |= (1 << ext_int_num);
	writel(reg_val,gpio_addr+PIO_INT_CTRL_OFFSET);

	udelay(1);
#endif

request_tp_int_port_failed:
	return ret;  
}

#endif






/*
do some previous work depends on platform,
if return value  < 0,driver will not register,
if return value  >= 0,conrinue register work
*/
static int ilitek_register_prepare(void)
{
    #if ((PLATFORM == 10)||(PLATFORM == 13)||(PLATFORM == 31))
    //fetch data
    if(!ctp_get_system_config()){
        pr_err("%s:read config fail!\n",__func__);
        return -1;
    }
    //add ctp device
    
    add_ctp_device();
    #endif
    return 0;
}




//全志平臺不要定義RESET_GPIO宏
//A10和A13 reset pin是從配置文件讀出來的，
//讀的過程中做了request gpio的動作,
//a31的reset pin是固定的，不需要獲取
//其他平臺的reset pin是直接宏定義的，需根據
//具體情況決定要不要做request gpio的動作
/*
request reset gpio and reset tp,
return value < 0 means fail
*/
static int ilitek_request_init_reset(void)
{
    #if ((PLATFORM == 10)||(PLATFORM == 13))
    reset_pin = gpio_request_ex("ctp_para", 0);
	if (!reset_pin) {
		pr_err("%s, %d\n", __func__, __LINE__);
	}	
	#endif	
	#if ((PLATFORM == 31))
	return 0;
	#endif
    i2c.reset_gpio = reset_pin;
    ilitek_reset(i2c.reset_gpio);
    return 0;
}
/*
set i2c.client->irq,could add some other work about irq here
return value < 0 means fail

*/
static int ilitek_set_irq(void)
{
    #if ((PLATFORM == 10)||(PLATFORM == 13)||(PLATFORM == 31))
    i2c.client->irq = IRQ_NUMBER;
    #endif
	return 0;
}
/*
config irq pin,could add some other work about irq here
return value < 0 means fail
*/
static int Request_IRQ(void){
    int ret = 0;
    
#if ((PLATFORM == 31))
    ret = sw_gpio_irq_request(i2c.client->irq, TRIG_EDGE_NEGATIVE, (peint_handle)ilitek_i2c_isr, &i2c);
    i2c.irq_handler = ret;
    return (!ret);
#else
    ret = request_irq(i2c.client->irq, ilitek_i2c_isr, IRQF_TRIGGER_FALLING/*| IRQF_SHARED*/ , "ilitek_i2c_irq", &i2c);
    return ret;
#endif
}				

static int ilitek_free_irq(unsigned int irq, void *dev)
{
#if ((PLATFORM == 31))
    sw_gpio_irq_free(i2c.irq_handler);
#else
    free_irq(irq, dev);
#endif

    return 0;
}

static int ilitek_config_irq(void)
{

    #if ((PLATFORM == 10)||(PLATFORM == 13))

    gpio_addr = ioremap(PIO_BASE_ADDRESS, PIO_RANGE_SIZE);
	if (!gpio_addr) {
		pr_err("%s, %d\n", __func__, __LINE__);
		return -1;
	}
    	#if 1
    	gpio_int_hdle = gpio_request_ex("ctp_para", "ctp_int_port");
        if(!gpio_int_hdle){
    		printk("ilitek request tp_int_port failed. gpio_int_hdle = %d\n",gpio_int_hdle);
    		return -1;
    	}
    	//設置寄存器的值
    	set_pin_mul_sel(gpio_int_hdle, "ctp_int_port", 6);
    	cfg_gpio_interrupt();
    	#else
	{
        int ret = 0;
        ret = ctp_set_irq_mode("ctp_para", "ctp_int_port", IRQ_GPIO, NEGATIVE_EDGE);                
        if(ret != 0)
            return ret;
	}
    	#endif
	#endif
	
	#if((PLATFORM == 31))
        ctp_set_int_port_rate(1);
	ctp_set_int_port_deb(0x07);
	return 0;
	#endif
	return 0;
}



void ilitek_set_finish_init_flag(void)
{
    return;
}


/*
description
	i2c interrupt service routine
parameters
	irq
		interrupt number
	dev_id
		device parameter
return
	return status
*/
static irqreturn_t 
ilitek_i2c_isr(
	int irq, void *dev_id)
{

	DBG("Enter\n");
	
#if TRY_ALLWIN_ENABALE_IRQ_FUC	
#if (PLATFORM == 31)
    if(i2c.irq_status ==1){
		sw_gpio_eint_set_enable(i2c.client->irq, 0);
		//pr_err("disable nosync\n");
		i2c.irq_status = 0;
	}
	queue_work(i2c.irq_work_queue, &i2c.irq_work);

	return 0;
#endif

#endif

	if(i2c.irq_status ==1){
		disable_irq_nosync(i2c.client->irq);
		DBG("disable nosync\n");
		i2c.irq_status = 0;
	}
	queue_work(i2c.irq_work_queue, &i2c.irq_work);

	return 0;
	//return 0;//A20 for LINUX 3.3 kernel only ?
}

/*
description
        i2c irq enable function
*/
static void ilitek_i2c_irq_enable(void)
{

#if TRY_ALLWIN_ENABALE_IRQ_FUC	
#if (PLATFORM == 31)
	if (i2c.irq_status == 0){
		i2c.irq_status = 1;
		sw_gpio_eint_set_enable(i2c.client->irq, 1);
		DBG("enable\n");
	}
	else
		DBG("no enable\n");
	return;
#endif

#endif

	if (i2c.irq_status == 0){
		i2c.irq_status = 1;
		enable_irq(i2c.client->irq);
		DBG("enable\n");
		
	}
	else
		DBG("no enable\n");
	return;
}
/*
description
        i2c irq disable function
*/
static void ilitek_i2c_irq_disable(void)
{

#if TRY_ALLWIN_ENABALE_IRQ_FUC	
#if (PLATFORM == 31)
    if (i2c.irq_status == 1){
		i2c.irq_status = 0;
		sw_gpio_eint_set_enable(i2c.client->irq, 0);
		DBG("disable\n");
	}
	else
		DBG("no disable\n");
	return;
#endif

#endif

	if (i2c.irq_status == 1){
		i2c.irq_status = 0;
		disable_irq(i2c.client->irq);
		DBG("disable\n");
	}
	else
		DBG("no disable\n");
	return;
}






