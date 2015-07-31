/*
	Copyright (c) 2010 by ilitek Technology.
	All rights reserved.

	ilitek I2C touch screen driver for Android platform

	Author:	 Steward Fu
	Maintain:Michael Hsu 
	Version: 1
	History:
		2010/10/26 Firstly released
		2010/10/28 Combine both i2c and hid function together
		2010/11/02 Support interrupt trigger for I2C interface
		2010/11/10 Rearrange code and add new IOCTL command
		2010/11/23 Support dynamic to change I2C address
		2010/12/21 Support resume and suspend functions
		2010/12/23 Fix synchronous problem when application and driver work at the same time
		2010/12/28 Add erasing background before calibrating touch panel
		2011/01/13 Rearrange code and add interrupt with polling method
		2011/01/14 Add retry mechanism
		2011/01/17 Support multi-point touch
		2011/01/21 Support early suspend function
		2011/02/14 Support key button function
		2011/02/18 Rearrange code
		2011/03/21 Fix counld not report first point
		2011/03/25 Support linux 2.36.x 
		2011/05/31 Added "echo dbg > /dev/ilitek_ctrl" to enable debug message
				   Added "echo info > /dev/ilitek_ctrl" to show tp informaiton
				   Added VIRTUAL_KEY_PAD to enable virtual key pad
				   Added CLOCK_INTERRUPT to change interrupt from Level to Edge
				   Changed report behavior from Interrupt to Interrupt with Polling
				   Added disable irq when doing firmware upgrade via APK, it needs to use APK_1.4.9
		2011/06/21 Avoid button is pressed when press AA
		2011/08/03 Added ilitek_i2c_calibration function
		2011/08/18 Fixed multi-point tracking id
				   Added ROTATE_FLAG to change x-->y, y-->x
				   Fixed when draw line from non-AA to AA, the line will not be appeared on screen.
		2011/09/29 Added Stop Polling in Interrupt mode
				   Fixed Multi-Touch return value
				   Added release last point
		2011/10/26 Fixed ROTATE bug
				   Added release key button when finger up.
				   Added ilitek_i2c_calibration_status for read calibration status
		2011/11/09 Fixed release last point issue
				   enable irq when i2c error.
		2011/11/28 implement protocol 2.1.
		2012/02/10 Added muti_touch key.
				   加入中斷旗標
		2012/04/02 加入 input_report_key(因為android 4.0 event input有做修改)
		
*/
#include "ilitek.h"

#define DRIVER_VERSION "aimvF"

int touch_key_hold_press = 0;
int touch_key_code[] = {KEY_MENU,KEY_HOME,KEY_BACK,KEY_VOLUMEDOWN,KEY_VOLUMEUP};
int touch_key_press[] = {0, 0, 0, 0, 0};
unsigned long touch_time=0;

//#define VIRTUAL_KEY_PAD
#define VIRTUAL_FUN_1	1	//0X81 with key_id
#define VIRTUAL_FUN_2	2	//0x81 with x position
#define VIRTUAL_FUN_3	3	//Judge x & y position
//#define VIRTUAL_FUN		VIRTUAL_FUN_2
#define BTN_DELAY_TIME	500 //ms

#define TOUCH_POINT    0x80
#define TOUCH_KEY      0xC0
#define RELEASE_KEY    0x40
#define RELEASE_POINT    0x00
//#define ROTATE_FLAG

#define CLOCK_INTERRUPT

//define key pad range
#define KEYPAD01_X1	0
#define KEYPAD01_X2	1000
#define KEYPAD02_X1	1000
#define KEYPAD02_X2	2000
#define KEYPAD03_X1	2000
#define KEYPAD03_X2	3000
#define KEYPAD04_X1	3000
#define KEYPAD04_X2	3968
#define KEYPAD_Y	2100
// definitions
#define ILITEK_I2C_RETRY_COUNT			3
#define ILITEK_I2C_DRIVER_NAME			"ilitek_i2c"
#define ILITEK_FILE_DRIVER_NAME			"ilitek_file"
#define ILITEK_DEBUG_LEVEL			KERN_INFO
#define ILITEK_ERROR_LEVEL			KERN_ALERT

// i2c command for ilitek touch screen
#define ILITEK_TP_CMD_READ_DATA			    0x10
#define ILITEK_TP_CMD_READ_SUB_DATA		    0x11
#define ILITEK_TP_CMD_GET_RESOLUTION		0x20
#define ILITEK_TP_CMD_SLEEP                 0x30
#define ILITEK_TP_CMD_GET_FIRMWARE_VERSION	0x40
#define ILITEK_TP_CMD_GET_PROTOCOL_VERSION	0x42
#define	ILITEK_TP_CMD_CALIBRATION			0xCC
#define	ILITEK_TP_CMD_CALIBRATION_STATUS	0xCD
#define ILITEK_TP_CMD_ERASE_BACKGROUND		0xCE

// define the application command
#define ILITEK_IOCTL_BASE                       100
#define ILITEK_IOCTL_I2C_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 0, unsigned char*)
#define ILITEK_IOCTL_I2C_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 2, unsigned char*)
#define ILITEK_IOCTL_I2C_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 3, int)
#define ILITEK_IOCTL_USB_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 4, unsigned char*)
#define ILITEK_IOCTL_USB_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 5, int)
#define ILITEK_IOCTL_USB_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 6, unsigned char*)
#define ILITEK_IOCTL_USB_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 7, int)
#define ILITEK_IOCTL_I2C_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 8, int)
#define ILITEK_IOCTL_USB_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 9, int)
#define ILITEK_IOCTL_I2C_SET_ADDRESS            _IOWR(ILITEK_IOCTL_BASE, 10, int)
#define ILITEK_IOCTL_I2C_UPDATE                 _IOWR(ILITEK_IOCTL_BASE, 11, int)
#define ILITEK_IOCTL_STOP_READ_DATA             _IOWR(ILITEK_IOCTL_BASE, 12, int)
#define ILITEK_IOCTL_START_READ_DATA            _IOWR(ILITEK_IOCTL_BASE, 13, int)
#define ILITEK_IOCTL_GET_INTERFANCE				_IOWR(ILITEK_IOCTL_BASE, 14, int)//default setting is i2c interface
#define ILITEK_IOCTL_I2C_SWITCH_IRQ				_IOWR(ILITEK_IOCTL_BASE, 15, int)


// module information
MODULE_AUTHOR("Steward_Fu");
MODULE_DESCRIPTION("ILITEK I2C touch driver for Android platform");
MODULE_LICENSE("GPL");

// device data
struct dev_data {
        // device number
        dev_t devno;
        // character device
        struct cdev cdev;
        // class device
        struct class *class;
};

// global variables

static struct dev_data dev;

static char Report_Flag;

static const unsigned short normal_i2c[2] = {0x41,I2C_CLIENT_END};

// i2c id table
static const struct i2c_device_id ilitek_i2c_id[] ={
	{ILITEK_I2C_DRIVER_NAME, 0}, {}
};
MODULE_DEVICE_TABLE(i2c, ilitek_i2c_id);

// declare i2c function table
static struct i2c_driver ilitek_i2c_driver = {
	.class = I2C_CLASS_HWMON,
	.id_table = ilitek_i2c_id,
	.driver = {
		.name = ILITEK_I2C_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.resume = ilitek_i2c_resume,
    .suspend  = ilitek_i2c_suspend,
	.shutdown = ilitek_i2c_shutdown,
	.probe = ilitek_i2c_probe,
	.remove = ilitek_i2c_remove,
	.address_list   = normal_i2c,
};

// declare file operations
struct file_operations ilitek_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	.unlocked_ioctl = ilitek_file_ioctl,
#else
	.ioctl = ilitek_file_ioctl,
#endif
	.read = ilitek_file_read,
	.write = ilitek_file_write,
	.open = ilitek_file_open,
	.release = ilitek_file_close,
};

/*
description
	open function for character device driver
prarmeters
	inode
	    inode
	filp
	    file pointer
return
	status
*/
static int 
ilitek_file_open(struct inode *inode, struct file *filp)
{
	DBG("%s\n",__func__);
	return 0; 
}
/*
description
	calibration function
prarmeters
	count
	    buffer length
return
	status
*/
static int ilitek_i2c_calibration(size_t count)
{

	int ret;
	unsigned char buffer[128]={0};
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = count, .buf = buffer,}
	};
	
	buffer[0] = ILITEK_TP_CMD_ERASE_BACKGROUND;
	msgs[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	if(ret < 0){
		printk(ILITEK_DEBUG_LEVEL "%s, i2c erase background, failed\n", __func__);
	}
	else{
		printk(ILITEK_DEBUG_LEVEL "%s, i2c erase background, success\n", __func__);
	}

	buffer[0] = ILITEK_TP_CMD_CALIBRATION;
	msgs[0].len = 1;
	msleep(2000);
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	msleep(1000);
	return ret;
}
/*
description
	read calibration status
prarmeters
	count
	    buffer length
return
	status
*/
static int ilitek_i2c_calibration_status(size_t count)
{
	int ret;
	unsigned char buffer[128]={0};
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = count, .buf = buffer,}
	};
	buffer[0] = ILITEK_TP_CMD_CALIBRATION_STATUS;
	ilitek_i2c_transfer(i2c.client, msgs, 1);
	msleep(500);
	ilitek_i2c_read(i2c.client, ILITEK_TP_CMD_CALIBRATION_STATUS, buffer, 1);
	printk("%s, i2c calibration status:0x%X\n",__func__,buffer[0]);
	ret=buffer[0];
	return ret;
}
/*
description
	write function for character device driver
prarmeters
	filp
	    file pointer
	buf
	    buffer
	count
	    buffer length
	f_pos
	    offset
return
	status
*/
static ssize_t 
ilitek_file_write(
	struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	unsigned char buffer[128]={0};
        
	// before sending data to touch device, we need to check whether the device is working or not
	if(i2c.valid_i2c_register == 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c device driver doesn't be registered\n", __func__);
		return -1;
	}

	// check the buffer size whether it exceeds the local buffer size or not
	if(count > 128){
		printk(ILITEK_ERROR_LEVEL "%s, buffer exceed 128 bytes\n", __func__);
		return -1;
	}

	// copy data from user space
	ret = copy_from_user(buffer, buf, count-1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, copy data from user space, failed", __func__);
		return -1;
	}

	// parsing command
	if(strcmp(buffer, "calibrate") == 0){
		ret=ilitek_i2c_calibration(count);
		if(ret < 0){
			printk(ILITEK_DEBUG_LEVEL "%s, i2c send calibration command, failed\n", __func__);
		}
		else{
			printk(ILITEK_DEBUG_LEVEL "%s, i2c send calibration command, success\n", __func__);
		}
		ret=ilitek_i2c_calibration_status(count);
		if(ret == 0x5A){
			printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, success\n", __func__);
		}
		else if (ret == 0xA5){
			printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, failed\n", __func__);
		}
		else{
			printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, i2c protoco failed\n", __func__);
		}
		return count;
	}else if(strcmp(buffer, "dbg") == 0){
		DBG_FLAG=!DBG_FLAG;
		printk("%s, %s DBG message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}else if(strcmp(buffer, "dbgco") == 0){
		DBG_COR=!DBG_COR;
		printk("%s, %s DBG COORDINATE message(%X).\n",__func__,DBG_COR?"Enabled":"Disabled",DBG_COR);
	}else if(strcmp(buffer, "info") == 0){
		ilitek_i2c_read_tp_info();
	}else if(strcmp(buffer, "report") == 0){
		Report_Flag=!Report_Flag;
	}else if(strcmp(buffer, "chxy") == 0){
		EXCHANG_XY=!EXCHANG_XY;
	}else if(strcmp(buffer, "revx") == 0){
		REVERT_X=!REVERT_X;
	}else if(strcmp(buffer, "revy") == 0){
		REVERT_Y=!REVERT_Y;
	}else if(strcmp(buffer, "suspd") == 0){
	    pm_message_t pmsg;
	    pmsg.event = 0;
	    ilitek_i2c_suspend(i2c.client, pmsg);
	}else if(strcmp(buffer, "resm") == 0){
	    ilitek_i2c_resume(i2c.client);
	}else if(strcmp(buffer, "reset") == 0){
        ilitek_reset(i2c.reset_gpio);
    	ilitek_i2c_reread_tp_info();
	}
	
	return -1;
}

/*
description
        ioctl function for character device driver
prarmeters
	inode
		file node
        filp
            file pointer
        cmd
            command
        arg
            arguments
return
        status
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long ilitek_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int  ilitek_file_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	static unsigned char buffer[64]={0};
	static int len=0;
	int ret;
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = len, .buf = buffer,}
        };

	// parsing ioctl command
	switch(cmd){
		case ILITEK_IOCTL_I2C_WRITE_DATA:
			ret = copy_from_user(buffer, (unsigned char*)arg, len);
			if(ret < 0){
						printk(ILITEK_ERROR_LEVEL "%s, copy data from user space, failed\n", __func__);
						return -1;
				}
			ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c write, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_READ_DATA:
			msgs[0].flags = I2C_M_RD;
	
			ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read, failed\n", __func__);
				return -1;
			}
			ret = copy_to_user((unsigned char*)arg, buffer, len);
			
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, copy data to user space, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_WRITE_LENGTH:
		case ILITEK_IOCTL_I2C_READ_LENGTH:
			len = arg;
			break;
		case ILITEK_IOCTL_I2C_UPDATE_RESOLUTION:
		case ILITEK_IOCTL_I2C_SET_ADDRESS:
		case ILITEK_IOCTL_I2C_UPDATE:
			break;
		case ILITEK_IOCTL_START_READ_DATA:
			i2c.stop_polling = 0;
			if(i2c.client->irq != 0 )
				ilitek_i2c_irq_enable();
			break;
		case ILITEK_IOCTL_STOP_READ_DATA:
			i2c.stop_polling = 1;
			if(i2c.client->irq != 0 )
				ilitek_i2c_irq_disable();
			break;
		case ILITEK_IOCTL_I2C_SWITCH_IRQ:
			ret = copy_from_user(buffer, (unsigned char*)arg, 1);
			if (buffer[0]==0)
			{
				if(i2c.client->irq != 0 ){
					ilitek_i2c_irq_disable();
				}
			}
			else
			{
				if(i2c.client->irq != 0 ){
					ilitek_i2c_irq_enable();				}
			}
			break;	
		default:
			return -1;
	}
    	return 0;
}

/*
description
	read function for character device driver
prarmeters
	filp
	    file pointer
	buf
	    buffer
	count
	    buffer length
	f_pos
	    offset
return
	status
*/
static ssize_t
ilitek_file_read(
        struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

/*
description
	close function
prarmeters
	inode
	    inode
	filp
	    file pointer
return
	status
*/
static int 
ilitek_file_close(
	struct inode *inode, struct file *filp)
{
	DBG("%s\n",__func__);
        return 0;
}

/*
description
	set input device's parameter
prarmeters
	input
		input device data
	max_tp
		single touch or multi touch
	max_x
		maximum	x value
	max_y
		maximum y value
return
	nothing
*/
static void 
ilitek_set_input_param(
	struct input_dev *input, 
	int max_tp, 
	int max_x, 
	int max_y)
{
	int key;
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	#ifndef ROTATE_FLAG
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_x+2, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_y+2, 0, 0);
	#else
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_y+2, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_x+2, 0, 0);
	#endif
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, max_tp, 0, 0);
	for(key=0; key<sizeof(touch_key_code); key++){
        	if(touch_key_code[key] <= 0){
            		continue;
		}
        	set_bit(touch_key_code[key] & KEY_MAX, input->keybit);
	}
	input->name = "ilitek_aimvF";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &(i2c.client)->dev;
}

/*
description
	send message to i2c adaptor
parameter
	client
		i2c client
	msgs
		i2c message
	cnt
		i2c message count
return
	>= 0 if success
	others if error
*/
static int ilitek_i2c_transfer(struct i2c_client *client, struct i2c_msg *msgs, int cnt)
{
	int ret, count=ILITEK_I2C_RETRY_COUNT;
	while(count >= 0){
		count-= 1;
		ret = down_interruptible(&i2c.wr_sem);
                ret = i2c_transfer(client->adapter, msgs, cnt);
                up(&i2c.wr_sem);
                if(ret < 0){
                        msleep(500);
			continue;
                }
		break;
	}
	return ret;
}

/*
description
	read data from i2c device
parameter
	client
		i2c client data
	addr
		i2c address
	data
		data for transmission
	length
		data length
return
	status
*/
static int 
ilitek_i2c_read(
	struct i2c_client *client,
	uint8_t cmd, 
	uint8_t *data, 
	int length)
{
	int ret;
	struct i2c_msg msgs_cmd[] = {
	{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
	};
	
	struct i2c_msg msgs_ret[] = {
	{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
	};

	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
	}	

	ret = ilitek_i2c_transfer(client, msgs_ret, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);		
	}
	
	return ret;
}


/*
description
	process i2c data and then report to kernel
parameters
	none
return
	status
*/
static int ilitek_i2c_process_and_report(void)
{
	int i, len, ret, x=0, y=0,key,org_x=0, org_y=0,mult_tp_id;
	unsigned char key_id=0,key_flag=1;
	static unsigned char last_id=0;
	struct input_dev *input = i2c.input_dev;
    unsigned char buf[9]={0};
	unsigned char tp_id;
	//unsigned char release_counter=0;
	{
		// clear warning
		key_flag = key_flag;
		key_id = key_id;
		org_x = org_x;
		org_y = org_y;
	}
    // read i2c data from device
	DBG("Enter\n");
    ret = ilitek_i2c_read(i2c.client, ILITEK_TP_CMD_READ_DATA, buf, 9);
	if(ret < 0){
		return ret;
	}

	// multipoint process
	if(i2c.protocol_ver & 0x200){
		len = buf[0];
		//printk("len=%X\n",buf[0]);
		ret = 1;
		// read touch point
		for(i=0; i<len; i++){
			// parse point
			if(ilitek_i2c_read(i2c.client, ILITEK_TP_CMD_READ_SUB_DATA, buf, 5)){
				x = (((int)buf[1]) << 8) + buf[2];
				y = (((int)buf[3]) << 8) + buf[4];

                if(EXCHANG_XY){
                    int temp = x;
                    x = y;
                    y = temp;
                }

                if(REVERT_X){
                    x = i2c.max_x - x;
                }
                if(REVERT_Y){
                    y = i2c.max_y - y;
                }

                DBG_CO("id = %d,x = %d,y = %d\n",((buf[0] & 0x3F)-1),x,y);

				mult_tp_id = buf[0];
				switch ((mult_tp_id & 0xC0)){
#ifdef VIRTUAL_KEY_PAD
					
					
					case RELEASE_KEY:
						//release key
						DBG("Key: Release\n");
						for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
							if(touch_key_press[key]){
								input_report_key(input, touch_key_code[key], 0);
								touch_key_press[key] = 0;
								DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
								DBG(ILITEK_DEBUG_LEVEL "%s key release, %X, %d, %d\n", __func__, buf[0], x, y);
							}
							touch_key_hold_press=0;
							//ret = 1;// stop timer interrupt	
						}		

						break;
					
					case TOUCH_KEY:
						//touch key
						#if VIRTUAL_FUN==VIRTUAL_FUN_1
						key_id = buf[1] - 1;
						#endif	
						#if VIRTUAL_FUN==VIRTUAL_FUN_2
						if (abs(jiffies-touch_time) < msecs_to_jiffies(BTN_DELAY_TIME))
							break;
						//DBG("Key: Enter\n");
						x = (((int)buf[4]) << 8) + buf[3];
						
						//printk("%s,x=%d\n",__func__,x);
						if (x > KEYPAD01_X1 && x<KEYPAD01_X2)		// btn 1
							key_id=0;
						else if (x > KEYPAD02_X1 && x<KEYPAD02_X2)	// btn 2
							key_id=1;
						else if (x > KEYPAD03_X1 && x<KEYPAD03_X2)	// btn 3
							key_id=2;
						else if (x > KEYPAD04_X1 && x<KEYPAD04_X2)	// btn 4
							key_id=3;
						else 
							key_flag=0;
						#endif
						if((touch_key_press[key_id] == 0) && (touch_key_hold_press == 0 && key_flag)){
							input_report_key(input, touch_key_code[key_id], 1);
							touch_key_press[key_id] = 1;
							touch_key_hold_press = 1;
							DBG("Key:%d ID:%d press x=%d,touch_key_hold_press=%d,key_flag=%d\n", touch_key_code[key_id], key_id,x,touch_key_hold_press,key_flag);
						}
						break;
						
#endif	
					case TOUCH_POINT:
	
#ifdef VIRTUAL_KEY_PAD		
						#if VIRTUAL_FUN==VIRTUAL_FUN_3
						if((buf[0] & 0x80) != 0 && ( y > KEYPAD_Y) && i==0){
							DBG("%s,touch key\n",__func__);
							if((x > KEYPAD01_X1) && (x < KEYPAD01_X2)){
								input_report_key(input,  touch_key_code[0], 1);
								touch_key_press[0] = 1;
								touch_key_hold_press = 1;
								DBG("%s,touch key=0 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
							}
							else if((x > KEYPAD02_X1) && (x < KEYPAD02_X2)){
								input_report_key(input, touch_key_code[1], 1);
								touch_key_press[1] = 1;
								touch_key_hold_press = 1;
								DBG("%s,touch key=1 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
							}
							else if((x > KEYPAD03_X1) && (x < KEYPAD03_X2)){
								input_report_key(input, touch_key_code[2], 1);
								touch_key_press[2] = 1;
								touch_key_hold_press = 1;
								DBG("%s,touch key=2 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
							}
							else {
								input_report_key(input, touch_key_code[3], 1);
								touch_key_press[3] = 1;
								touch_key_hold_press = 1;
								DBG("%s,touch key=3 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
							}
							
						}
						if((buf[0] & 0x80) != 0 && y <= KEYPAD_Y)
							touch_key_hold_press=0;
						if((buf[0] & 0x80) != 0 && y <= KEYPAD_Y)
						#endif
#endif
						{				
						// report to android system
						DBG("Point, ID=%02X, X=%04d, Y=%04d,touch_key_hold_press=%d\n",buf[0]  & 0x3F, x,y,touch_key_hold_press);	
						input_report_key(input, BTN_TOUCH,  1);//Luca 20120402
						input_event(input, EV_ABS, ABS_MT_TRACKING_ID, (buf[0] & 0x3F)-1);
						input_event(input, EV_ABS, ABS_MT_POSITION_X, x+1);
						input_event(input, EV_ABS, ABS_MT_POSITION_Y, y+1);
						input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
						input_mt_sync(input);
						ret=0;
						}
						break;
						
					case RELEASE_POINT:
						if (touch_key_hold_press !=0 && i==0){
							for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
								if(touch_key_press[key]){
									input_report_key(input, touch_key_code[key], 0);
									touch_key_press[key] = 0;
									DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
									DBG(ILITEK_DEBUG_LEVEL "%s key release, %X, %d, %d,touch_key_hold_press=%d\n", __func__, buf[0], x, y,touch_key_hold_press);
								}
								touch_key_hold_press=0;
								//ret = 1;// stop timer interrupt	
							}		
						}
						// release point
                        /*
						release_counter++;
						if (release_counter==5){
							input_report_key(input, BTN_TOUCH,  0);
							input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
							input_mt_sync(input);
						}
						*/

						//ret=1;				
						break;
						
					default:
						break;
				}
			}
		}
		// release point
		if(len== 0){
			DBG("Release3, ID=%02X, X=%04d, Y=%04d\n",buf[0]  & 0x3F, x,y);
			input_report_key(input, BTN_TOUCH,  0);
			//input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
			input_mt_sync(input);
			//ret = 1;
			if (touch_key_hold_press !=0){
				for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
					if(touch_key_press[key]){
						input_report_key(input, touch_key_code[key], 0);
						touch_key_press[key] = 0;
						DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
						DBG(ILITEK_DEBUG_LEVEL "%s key release, %X, %d, %d\n", __func__, buf[0], x, y);
					}
					touch_key_hold_press=0;
					//ret = 1;// stop timer interrupt	
				}		
			}
		}
		input_sync(input);
		DBG("%s,ret=%d\n",__func__,ret);
	}
	
	else{
		// parse point
		ret = 0;
		
		
		tp_id = buf[0];
		if (Report_Flag!=0){
			printk("%s(%d):",__func__,__LINE__);
			for (i=0;i<16;i++)
				DBG("%02X,",buf[i]);
			DBG("\n");
		}
		switch (tp_id)
		{
			case 0://release point
#ifdef VIRTUAL_KEY_PAD				
				if (touch_key_hold_press !=0)
				{
					for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
						if(touch_key_press[key]){
							//input_report_key(input, touch_key_code[key], 0);
							touch_key_press[key] = 0;
							DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
						}
					}
					touch_key_hold_press = 0;
				}
				else
#endif
				{
					for(i=0; i<i2c.max_tp; i++){
						// check 
						if (!(last_id & (1<<i)))
							continue;	
							
						#ifndef ROTATE_FLAG
						x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
						y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
						#else
						org_x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
						org_y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
						x = i2c.max_y - org_y + 1;
						y = org_x + 1;					
						#endif

                        if(EXCHANG_XY){
                            int temp = x;
                            x = y;
                            y = temp;
                        }

                        if(REVERT_X){
                            x = i2c.max_x - x;
                        }
                        if(REVERT_Y){
                            y = i2c.max_y - y;
                        }

                        
						touch_key_hold_press=2; //2: into available area
						input_report_key(input, BTN_TOUCH,  1);//Luca 20120402
						input_event(i2c.input_dev, EV_ABS, ABS_MT_TRACKING_ID, i);
						input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_X, x+1);
						input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_Y, y+1);
						input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
						input_mt_sync(i2c.input_dev);
						DBG("Last Point[%d]= %d, %d\n", buf[0]&0x3F, x, y);
						last_id=0;
					}
					input_sync(i2c.input_dev);//20120407				
					input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
					input_mt_sync(i2c.input_dev);
					ret = 1; // stop timer interrupt
				}
				break;
#ifdef VIRTUAL_KEY_PAD				
			case 0x81:
				if (abs(jiffies-touch_time) < msecs_to_jiffies(BTN_DELAY_TIME))
					break;
				DBG("Key: Enter\n");
	
				#if VIRTUAL_FUN==VIRTUAL_FUN_1
				key_id = buf[1] - 1;
				#endif
				
				#if VIRTUAL_FUN==VIRTUAL_FUN_2
				x = (int)buf[1] + ((int)buf[2] * 256);
				if (x > KEYPAD01_X1 && x<KEYPAD01_X2)		// btn 1
					key_id=0;
				else if (x > KEYPAD02_X1 && x<KEYPAD02_X2)	// btn 2
					key_id=1;
				else if (x > KEYPAD03_X1 && x<KEYPAD03_X2)	// btn 3
					key_id=2;
				else if (x > KEYPAD04_X1 && x<KEYPAD04_X2)	// btn 4
					key_id=3;
				else 
					key_flag=0;			
				#endif
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
    				input_mt_sync(input);
				if((touch_key_press[key_id] == 0) && (touch_key_hold_press == 0 && key_flag)){
					input_report_key(input, touch_key_code[key_id], 1);
					touch_key_press[key_id] = 1;
					touch_key_hold_press = 1;
					DBG("Key:%d ID:%d press\n", touch_key_code[key_id], key_id);
				}			
				break;
			case 0x80:
				DBG("Key: Release\n");
				for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
					if(touch_key_press[key]){
						input_report_key(input, touch_key_code[key], 0);
						touch_key_press[key] = 0;
						DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
                   	}
				}		
				touch_key_hold_press=0;
				ret = 1;// stop timer interrupt	
				break;
#endif
			default:
				last_id=buf[0];
				for(i=0; i<i2c.max_tp; i++){
					// check 
					if (!(buf[0] & (1<<i)))
						continue;	
						
					#ifndef ROTATE_FLAG
					x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
					y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
					#else
					org_x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
					org_y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
					x = i2c.max_y - org_y + 1;
					y = org_x + 1;					
					#endif

	                if(EXCHANG_XY){
                        int temp = x;
                        x = y;
                        y = temp;
                    }

                    if(REVERT_X){
                        x = i2c.max_x - x;
                    }
                    if(REVERT_Y){
                        y = i2c.max_y - y;
                    }
                    
#ifdef VIRTUAL_KEY_PAD						
					#if VIRTUAL_FUN==VIRTUAL_FUN_3
					if (y > KEYPAD_Y){
						if (abs(jiffies-touch_time) < msecs_to_jiffies(BTN_DELAY_TIME))
							break;									
						x = (int)buf[1] + ((int)buf[2] * 256);
						if (x > KEYPAD01_X1 && x<KEYPAD01_X2)		// btn 1
							key_id=0;
						else if (x > KEYPAD02_X1 && x<KEYPAD02_X2)	// btn 2
							key_id=1;
						else if (x > KEYPAD03_X1 && x<KEYPAD03_X2)	// btn 3
							key_id=2;
						else if (x > KEYPAD04_X1 && x < KEYPAD04_X2)	// btn 4
							key_id=3;
						else 
							key_flag=0;			
						if (touch_key_hold_press==2){
							input_report_key(input, BTN_TOUCH,  0);//Luca 20120402
							input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
							input_mt_sync(i2c.input_dev);
							touch_key_hold_press=0;
						}
						if((touch_key_press[key_id] == 0) && (touch_key_hold_press == 0 && key_flag)){
							//input_report_key(input, touch_key_code[key_id], 1);
							touch_key_press[key_id] = 1;
							touch_key_hold_press = 1;
							DBG("Key:%d ID:%d press\n", touch_key_code[key_id], key_id);					
						}
					}
					else if (touch_key_hold_press){
						for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]) ; key++){
							if(touch_key_press[key]){
								//input_report_key(input, touch_key_code[key], 0);
								touch_key_press[key] = 0;
								DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
							}
						}
						touch_key_hold_press = 0;
					}
					else
					#endif
					touch_time=jiffies + msecs_to_jiffies(BTN_DELAY_TIME);
#endif					
					{
						touch_key_hold_press=2; //2: into available area
						input_report_key(input, BTN_TOUCH,  1);//Luca 20120402
						input_event(i2c.input_dev, EV_ABS, ABS_MT_TRACKING_ID, i);
						input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_X, x+1);
						input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_Y, y+1);
						input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
						input_mt_sync(i2c.input_dev);
						DBG("Point[%d]= %d, %d\n", buf[0]&0x3F, x, y);
					}
					
				}
				break;
		}

		input_sync(i2c.input_dev);
	}
    return ret;
}

static void ilitek_i2c_timer(unsigned long handle)
{
    struct i2c_data *priv = (void *)handle;
    DBG("Enter\n");

    schedule_work(&priv->irq_work);
}

/*
description
	work queue function for irq use
parameter
	work
		work queue
return
	nothing
*/
static void 
ilitek_i2c_irq_work_queue_func(
	struct work_struct *work)
{
	int ret;
	DBG("Enter\n");

	ret = ilitek_i2c_process_and_report();
#ifdef CLOCK_INTERRUPT
	ilitek_i2c_irq_enable();
#else
    if (ret == 0){
	struct i2c_data *priv =  
		container_of(work, struct i2c_data, irq_work);
		if (!i2c.stop_polling)
			mod_timer(&priv->timer, jiffies + msecs_to_jiffies(0));
	}
    else if (ret == 1){
		if (!i2c.stop_polling){
			ilitek_i2c_irq_enable();
		}
		DBG("stop_polling\n");
	}
	else if(ret < 0){
		msleep(100);
		DBG(ILITEK_ERROR_LEVEL "%s, process error\n", __func__);
		ilitek_i2c_irq_enable();
    }	
#endif
}

/*
description
        i2c polling thread
parameters
        arg
                arguments
return
        return status
*/
static int 
ilitek_i2c_polling_thread(
	void *arg)
{

	int ret=0;
	DBG("Enter\n");
	// check input parameter
	printk(ILITEK_DEBUG_LEVEL "%s, enter\n", __func__);

	// mainloop
	while(1){
		// check whether we should exit or not
		if(kthread_should_stop()){
			printk(ILITEK_DEBUG_LEVEL "%s, stop\n", __func__);
			break;
		}

		// this delay will influence the CPU usage and response latency
		msleep(10);
		
		// when i2c is in suspend or shutdown mode, we do nothing
		if(i2c.stop_polling){
			msleep(1000);
			continue;
		}

		// read i2c data
		if(ilitek_i2c_process_and_report() < 0){
			msleep(3000);
			printk(ILITEK_ERROR_LEVEL "%s, process error\n", __func__);
		}
	}
	
	printk(ILITEK_DEBUG_LEVEL "%s, exit\n", __func__);
	return ret;
}

/*
description
	i2c early suspend function
parameters
	h
		early suspend pointer
return
	nothing
*/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void ilitek_i2c_early_suspend(struct early_suspend *h)
{
	ilitek_i2c_suspend(i2c.client, PMSG_SUSPEND);
	printk("%s\n", __func__);
}
#endif

/*
description
        i2c later resume function
parameters
        h
                early suspend pointer
return
        nothing
*/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void ilitek_i2c_late_resume(struct early_suspend *h)
{
	ilitek_i2c_resume(i2c.client);
	printk("%s\n", __func__);
}
#endif


/*
description
        i2c suspend function
parameters
        client
		i2c client data
	mesg
		suspend data
return
        return status
*/

static int 
ilitek_i2c_suspend(
	struct i2c_client *client, pm_message_t mesg)
{
//    uint8_t cmd = ILITEK_TP_CMD_SLEEP;
    int ret = 0;
    //struct i2c_msg msgs_cmd[] = {
//	{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
//	};
	
	DBG("Enter\n");
	if(i2c.valid_irq_request != 0){
		ilitek_i2c_irq_disable();
	}
	else{
		i2c.stop_polling = 1;
 	       	printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread polling\n", __func__);
  	}

  	//ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
	}
	
	return 0;
}

/*
description
        i2c resume function
parameters
        client
		i2c client data
return
        return status
*/
static int ilitek_i2c_resume(struct i2c_client *client)
{
    DBG("Enter\n");
   // if(i2c.request_reset_success)
    {
        ilitek_reset(i2c.reset_gpio);
    	ilitek_i2c_reread_tp_info();
    }
	
    if(i2c.valid_irq_request != 0){
                ilitek_i2c_irq_enable();
        }
	else{
		i2c.stop_polling = 0;
        	printk(ILITEK_DEBUG_LEVEL "%s, start i2c thread polling\n", __func__);
	}

	return 0;
}

/*
description
        i2c shutdown function
parameters
        client
                i2c client data
return
        nothing
*/
static void
ilitek_i2c_shutdown(
        struct i2c_client *client)
{
        printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
        i2c.stop_polling = 1;
}

/*
description
	when adapter detects the i2c device, this function will be invoked.
parameters
	client
		i2c client data
	id
		i2c data
return
	status
*/
static int 
ilitek_i2c_probe(
	struct i2c_client *client, 
	const struct i2c_device_id *id)
{
	DBG("Enter\n");
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
                printk(ILITEK_ERROR_LEVEL "%s, I2C_FUNC_I2C not support\n", __func__);
                return -1;
        }
	i2c.client = client;
    printk(ILITEK_DEBUG_LEVEL "%s, i2c new style format\n", __func__);
	return 0;
}

/*
description
	when the i2c device want to detach from adapter, this function will be invoked.
parameters
	client
		i2c client data
return
	status
*/
static int 
ilitek_i2c_remove(
	struct i2c_client *client)
{
	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
	i2c.stop_polling = 1;
	return 0;
}

/*
description
	read data from i2c device with delay between cmd & return data
parameter
	client
		i2c client data
	addr
		i2c address
	data
		data for transmission
	length
		data length
return
	status
*/
static int 
ilitek_i2c_read_info(
	struct i2c_client *client,
	uint8_t cmd, 
	uint8_t *data, 
	int length)
{
	int ret;
	struct i2c_msg msgs_cmd[] = {
	{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
	};
	
	struct i2c_msg msgs_ret[] = {
	{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
	};

	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
	}
	
	msleep(10);
	ret = ilitek_i2c_transfer(client, msgs_ret, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);		
	}
	
	printk(ILITEK_ERROR_LEVEL "%s, Driver Vesrion: %s\n", __func__, DRIVER_VERSION);
	return ret;
}

/*
description
	read touch information
parameters
	none
return
	status
*/
static int
ilitek_i2c_read_tp_info(
	void)
{
	int res_len,i;
	unsigned char buf[32]={0};
	DBG("Enter\n");

	// read firmware version
        if(ilitek_i2c_read_info(i2c.client, ILITEK_TP_CMD_GET_FIRMWARE_VERSION, buf, 4) < 0){
		return -1;
	}
    for(i = 0;i<4;i++)  i2c.firmware_ver[i] = buf[i];
    printk(ILITEK_DEBUG_LEVEL "%s, firmware version %d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2], buf[3]);

	// read protocol version
        res_len = 6;
        if(ilitek_i2c_read_info(i2c.client, ILITEK_TP_CMD_GET_PROTOCOL_VERSION, buf, 2) < 0){
		return -1;
	}	
        i2c.protocol_ver = (((int)buf[0]) << 8) + buf[1];
        printk(ILITEK_DEBUG_LEVEL "%s, protocol version: %d.%d\n", __func__, buf[0], buf[1]);
        if(i2c.protocol_ver >= 0x200){
               	res_len = 8;
        }

        // read touch resolution
	i2c.max_tp = 2;
        if(ilitek_i2c_read_info(i2c.client, ILITEK_TP_CMD_GET_RESOLUTION, buf, res_len) < 0){
		return -1;
	}
        if(i2c.protocol_ver >= 0x200){
                // maximum touch point
                i2c.max_tp = buf[6];
                // maximum button number
                i2c.max_btn = buf[7];
        }
	// calculate the resolution for x and y direction
        i2c.max_x = buf[0];
        i2c.max_x+= ((int)buf[1]) * 256;
        i2c.max_y = buf[2];
        i2c.max_y+= ((int)buf[3]) * 256;
        i2c.x_ch = buf[4];
        i2c.y_ch = buf[5];

        if(EXCHANG_XY){
            int temp = i2c.max_x;
            i2c.max_x = i2c.max_y;
            i2c.max_y = temp;
        }
        
        printk(ILITEK_DEBUG_LEVEL "%s, max_x: %d, max_y: %d, ch_x: %d, ch_y: %d\n", 
		__func__, i2c.max_x, i2c.max_y, i2c.x_ch, i2c.y_ch);
        printk(ILITEK_DEBUG_LEVEL "%s, max_tp: %d, max_btn: %d\n", __func__, i2c.max_tp, i2c.max_btn);
	return 0;
}




/*
description
	read touch information
parameters
	none
return
	status
*/
static int
ilitek_i2c_reread_tp_info(
	void)
{
	int res_len;
	int count=ILITEK_I2C_RETRY_COUNT;
	unsigned char buf[32]={0};			  

	// read protocol version
        res_len = 6;

	for(count = ILITEK_I2C_RETRY_COUNT;count >= 0;count --){
		if(ilitek_i2c_read_info(i2c.client, ILITEK_TP_CMD_GET_PROTOCOL_VERSION, buf, 2) < 0){
			printk("%s : i2c read error",__func__);
			return -1;
		}
		if((buf[0] == 0xff)&&(buf[1] == 0xff)){
			continue;
		}
		else{  
			i2c.protocol_ver = (((int)buf[0]) << 8) + buf[1];       
		}
		break;
	}			
	printk(ILITEK_DEBUG_LEVEL "%s, protocol version: %d.%d\n", __func__,buf[0],buf[1]);
        
	if(i2c.protocol_ver < 0x200){
    	i2c.max_tp = 2;
	}	

	return 0;
}





#ifdef ILI_UPDATE_FW

/*
description
	upgrade F/W
prarmeters
		
return
	status
*/       
static int ilitek_upgrade_firmware(void)
{
	int ret=0,upgrade_status=0,i,j,k = 0,ap_len = 0,df_len = 0;
	unsigned char buffer[128]={0};
	unsigned long ap_startaddr,df_startaddr,ap_endaddr,df_endaddr,ap_checksum = 0,df_checksum = 0;
	unsigned char firmware_ver[4];
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 0, .buf = buffer,}
	};
	ap_startaddr = ( CTPM_FW[0] << 16 ) + ( CTPM_FW[1] << 8 ) + CTPM_FW[2];
	ap_endaddr = ( CTPM_FW[3] << 16 ) + ( CTPM_FW[4] << 8 ) + CTPM_FW[5];
	ap_checksum = ( CTPM_FW[6] << 16 ) + ( CTPM_FW[7] << 8 ) + CTPM_FW[8];
	df_startaddr = ( CTPM_FW[9] << 16 ) + ( CTPM_FW[10] << 8 ) + CTPM_FW[11];
	df_endaddr = ( CTPM_FW[12] << 16 ) + ( CTPM_FW[13] << 8 ) + CTPM_FW[14];
	df_checksum = ( CTPM_FW[15] << 16 ) + ( CTPM_FW[16] << 8 ) + CTPM_FW[17];
	firmware_ver[0] = CTPM_FW[18];
	firmware_ver[1] = CTPM_FW[19];
	firmware_ver[2] = CTPM_FW[20];
	firmware_ver[3] = CTPM_FW[21];
	df_len = ( CTPM_FW[22] << 16 ) + ( CTPM_FW[23] << 8 ) + CTPM_FW[24];
	ap_len = ( CTPM_FW[25] << 16 ) + ( CTPM_FW[26] << 8 ) + CTPM_FW[27];
	printk("ap_startaddr=0x%d,ap_endaddr=0x%d,ap_checksum=0x%d\n",ap_startaddr,ap_endaddr,ap_checksum);
	printk("df_startaddr=0x%d,df_endaddr=0x%d,df_checksum=0x%d\n",df_startaddr,df_endaddr,df_checksum);	
	buffer[0]=0xc0;
	msgs[0].len = 1;
	ret = ilitek_i2c_read(i2c.client, 0xc0, buffer, 1);
	if(ret < 0){
		return 3;
	}
	msleep(30);
	printk("ic. mode =%d\n",buffer[0]);
		
	if(buffer[0]!=0x55){
		for(i=0;i<4;i++){
			printk("i2c.firmware_ver[%d]=%d,firmware_ver[%d]=%d\n",i,i2c.firmware_ver[i],i,firmware_ver[i]);
		
			if((i2c.firmware_ver[i] > firmware_ver[i])||((i == 3) && (i2c.firmware_ver[3] == firmware_ver[3]))){
				return 1;				
			}
			else if(i2c.firmware_ver[i] < firmware_ver[i]){
				break;
			}
			
		}	
		

		
		buffer[0]=0xc4;
		msgs[0].len = 1;
		ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
		if(ret < 0){
		return 3;
		}
		msleep(30);
		buffer[0]=0xc2;
		ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
		if(ret < 0){
		return 3;
		}		
		msleep(100);
	}
	buffer[0]=0xc0;
	msgs[0].len = 1;
	ret = ilitek_i2c_read(i2c.client, 0xc0, buffer, 1);
	if(ret < 0){
	return 3;
	}

	msleep(30);
	printk("ILITEK:%s, upgrade firmware...\n", __func__);
	buffer[0]=0xc4;
	msgs[0].len = 10;
	buffer[1] = 0x5A;
	buffer[2] = 0xA5;
	buffer[3] = 0;
	buffer[4] = CTPM_FW[3];
	buffer[5] = CTPM_FW[4];
	buffer[6] = CTPM_FW[5];
	buffer[7] = CTPM_FW[6];
	buffer[8] = CTPM_FW[7];
	buffer[9] = CTPM_FW[8];
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	if(ret < 0){
	return 3;
	}

	msleep(30);
	
	buffer[0]=0xc4;
	msgs[0].len = 10;
	buffer[1] = 0x5A;
	buffer[2] = 0xA5;
	buffer[3] = 1;
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 0;
	buffer[7] = 0;
	buffer[8] = 0;
	buffer[9] = 0;
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	if(ret < 0){
	return 3;
	}

	msleep(30);
	
	j=0;
	for(i=0; i < df_len; i+=32){
		j+= 1;
		if((j % 16) == 1){
			msleep(60);
		}
		for(k=0; k<32; k++){
			buffer[1 + k] = CTPM_FW[i + 32 + k];
		}

		buffer[0]=0xc3;
		msgs[0].len = 33;
		ret = ilitek_i2c_transfer(i2c.client, msgs, 1);	
		if(ret < 0){
		return 3;
		}
		upgrade_status = (i * 100) / df_len;
		msleep(10);
		printk("%cILITEK: Firmware Upgrade(Data flash), %02d%c. ",0x0D,upgrade_status,'%');
	}
	
	buffer[0]=0xc4;
	msgs[0].len = 10;
	buffer[1] = 0x5A;
	buffer[2] = 0xA5;
	buffer[3] = 0;
	buffer[4] = CTPM_FW[3];
	buffer[5] = CTPM_FW[4];
	buffer[6] = CTPM_FW[5];
	buffer[7] = CTPM_FW[6];
	buffer[8] = CTPM_FW[7];
	buffer[9] = CTPM_FW[8];
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	if(ret < 0){
		return 3;
	}
	msleep(30);
	
	j=0;
	for(i = 0; i < ap_len; i+=32){
		j+= 1;
		if((j % 16) == 1){
			msleep(60);
		}
		for(k=0; k<32; k++){
			buffer[1 + k] = CTPM_FW[i + df_len + 32 + k];
		}

		buffer[0]=0xc3;
		msgs[0].len = 33;
		ret = ilitek_i2c_transfer(i2c.client, msgs, 1);	
		if(ret < 0){
		return 3;
		}
		upgrade_status = (i * 100) / ap_len;
		msleep(10);
		printk("%cILITEK: Firmware Upgrade(AP), %02d%c. ",0x0D,upgrade_status,'%');
	}
	
	printk("ILITEK:%s, upgrade firmware completed\n", __func__);
	buffer[0]=0xc4;
	msgs[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	if(ret < 0){
	return 3;
	}

	msleep(30);
	buffer[0]=0xc1;
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	if(ret < 0){
	return 3;
	}

	buffer[0]=0xc0;
	msgs[0].len = 1;
	ret = ilitek_i2c_read(i2c.client, 0xc0, buffer, 1);
	if(ret < 0){
		return 3;
	}
	msleep(30);
	printk("ic. mode =%d , it's  %s \n",buffer[0],((buffer[0] == 0x5A)?"AP MODE":"BL MODE"));

	msleep(100);
	return 2;
}
#endif

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
	unsigned char buf[32] = {0};

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
       return -ENODEV;

    if(twi_id == adapter->nr){
 		if(ilitek_i2c_read_info(client, ILITEK_TP_CMD_GET_FIRMWARE_VERSION, buf, 4) < 0){
			printk ("not find ilietek\n");
        	return -ENODEV;
    	}
		else {
			printk ("find ilietek\n");
			return 0;
		}
	}
	return -ENODEV;
}

/*
description
	register i2c device and its input device
parameters
	none
return
	status
*/
static int 
ilitek_i2c_register_device(
	void)
{
	int ret;
	DBG("Enter register\n");
	ilitek_i2c_driver.detect = ctp_detect;
	ret = i2c_add_driver(&ilitek_i2c_driver);
	printk(ILITEK_DEBUG_LEVEL " add return\n");
	if(ret == 0){
		i2c.valid_i2c_register = 1;
		printk(ILITEK_DEBUG_LEVEL "%s, add i2c device, success\n", __func__);
		if(i2c.client == NULL){
			printk(ILITEK_ERROR_LEVEL "%s, no i2c board information\n", __func__);
			return -1;
		}
		printk(ILITEK_DEBUG_LEVEL "%s, client.addr: 0x%X\n", __func__, (unsigned int)i2c.client->addr);
		printk(ILITEK_DEBUG_LEVEL "%s, client.adapter: 0x%X\n", __func__, (unsigned int)i2c.client->adapter);
		printk(ILITEK_DEBUG_LEVEL "%s, client.driver: 0x%X\n", __func__, (unsigned int)i2c.client->driver);
		if((i2c.client->addr == 0) || (i2c.client->adapter == 0) || (i2c.client->driver == 0)){
			printk(ILITEK_ERROR_LEVEL "%s, invalid register\n", __func__);
			return ret;
		}

		ret = ilitek_request_init_reset();
        if(ret < 0){
            printk("ilitek request reset err\n");
        }
        
		// read touch parameter
		ret = ilitek_i2c_read_tp_info();  
		if(ret < 0){
            printk("ilitek read tp info fail\n");
            return ret;
        }
        else{
            i2c.reset_request_success = 1;
        }
        
#ifdef ILI_UPDATE_FW
		ret = ilitek_upgrade_firmware();
		if(ret==1)	printk("Do not need update\n"); 
		else if(ret==2) printk("update end\n");
		else if(ret==3) printk("i2c communication error\n");

        if(i2c.reset_request_success){
            ilitek_reset(i2c.reset_gpio);
        }

		// read touch parameter
		ret=ilitek_i2c_read_tp_info();
		if(ret < 0)
		{
			return ret;
		}       
#endif

		// register input device
		i2c.input_dev = input_allocate_device();
		if(i2c.input_dev == NULL){
			printk(ILITEK_ERROR_LEVEL "%s, allocate input device, error\n", __func__);
			return -1;
		}
		ilitek_set_input_param(i2c.input_dev, i2c.max_tp, i2c.max_x, i2c.max_y);
        	ret = input_register_device(i2c.input_dev);
        	if(ret){
               		printk(ILITEK_ERROR_LEVEL "%s, register input device, error\n", __func__);
                	return ret;
        	}
               	printk(ILITEK_ERROR_LEVEL "%s, register input device, success\n", __func__);
		i2c.valid_input_register = 1;

#ifdef CONFIG_HAS_EARLYSUSPEND
		i2c.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		i2c.early_suspend.suspend = ilitek_i2c_early_suspend;
		i2c.early_suspend.resume = ilitek_i2c_late_resume;
		register_early_suspend(&i2c.early_suspend);
#endif

		#ifndef POLLING_MODE

        //指定中斷號即i2c.client->irq的值
		ret = ilitek_set_irq();
        printk("%s, IRQ: 0x%X\n", __func__, (i2c.client->irq));
        if(ret < 0){
            printk("ilitek set irq fail\n");
        }
        
		if(i2c.client->irq != 0 ){ // == => polling mode, != => interrup mode
			i2c.irq_work_queue = create_singlethread_workqueue("ilitek_i2c_irq_queue");
			if(i2c.irq_work_queue){
				INIT_WORK(&i2c.irq_work, ilitek_i2c_irq_work_queue_func);
				#ifdef CLOCK_INTERRUPT
				if(Request_IRQ()){
					printk(ILITEK_ERROR_LEVEL "%s, request irq, error\n", __func__);
				}
				else{
                    ret = ilitek_config_irq();
                    if(ret < 0){
                        printk("ilitek config irq fail\n");
                    }
					i2c.valid_irq_request = 1;
					i2c.irq_status = 1;
					printk(ILITEK_ERROR_LEVEL "%s, request irq, success\n", __func__);
				}	
				#else				
				init_timer(&i2c.timer);
				i2c.timer.data = (unsigned long)&i2c;
				i2c.timer.function = ilitek_i2c_timer;
				if(request_irq(i2c.client->irq, ilitek_i2c_isr, IRQF_TRIGGER_LOW, "ilitek_i2c_irq", &i2c)){
					printk(ILITEK_ERROR_LEVEL "%s, request irq, error\n", __func__);
				}
				else{
			        ret = ilitek_config_irq();
                    if(ret < 0){
                        printk("ilitek config irq fail\n");
                    }
					i2c.valid_irq_request = 1;
					i2c.irq_status = 1;
					printk(ILITEK_ERROR_LEVEL "%s, request irq, success\n", __func__);
				}
				#endif
				
			}
		}
		else
		#endif
		{
#ifdef POLLING_MODE
seting_polling_mode:
#endif
			printk( "%s,i2c.client->irq = %d , setting polling mode \n", __func__,(i2c.client->irq));
			i2c.stop_polling = 0;
			i2c.thread = kthread_create(ilitek_i2c_polling_thread, NULL, "ilitek_i2c_thread");
			if(i2c.thread == (struct task_struct*)ERR_PTR){
					i2c.thread = NULL;
					printk(ILITEK_ERROR_LEVEL "%s, kthread create, error\n", __func__);
			}
			else{
					i2c.set_polling_mode = 1;
					wake_up_process(i2c.thread);
			}
		}
		#ifdef POLLING_MODE
        if(i2c.set_polling_mode != 1){
            goto seting_polling_mode;
        }   
		#endif
	}
	else{
		printk(ILITEK_ERROR_LEVEL "%s, add i2c device, error\n", __func__);
		return ret;
	}
	return 0;
}

/*
description
	initiali function for driver to invoke.
parameters

	nothing
return
	status
*/
static int 
ilitek_init(
	void)
{
	int ret = 0;
	DBG("Enter\n");

	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
    ret = ilitek_should_load_driver();
    if(ret < 0){
		return ret;
	}
	
	
	// initialize global variable
    	memset(&dev, 0, sizeof(struct dev_data));
    	memset(&i2c, 0, sizeof(struct i2c_data));

	// initialize mutex object
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)	
	init_MUTEX(&i2c.wr_sem);
#else
	sema_init(&i2c.wr_sem,1);
#endif

	i2c.wr_sem.count = 1;

    //完成註冊I2C設備的動作，包括獲取總線號，
    //獲取adapter和根據adapter和boardinfo註冊i2c設備
	ret = ilitek_register_prepare();
	if(ret < 0){
		return ret;
	}
	
	// register i2c device
	ret = ilitek_i2c_register_device();
	if(ret < 0){
		i2c_del_driver(&ilitek_i2c_driver);
		printk(ILITEK_ERROR_LEVEL "%s, register i2c device, error\n", __func__);
		return ret;
	}

	// allocate character device driver buffer
	ret = alloc_chrdev_region(&dev.devno, 0, 1, ILITEK_FILE_DRIVER_NAME);
    	if(ret){
        	printk(ILITEK_ERROR_LEVEL "%s, can't allocate chrdev\n", __func__);
		return ret;
	}
    	printk(ILITEK_DEBUG_LEVEL "%s, register chrdev(%d, %d)\n", __func__, MAJOR(dev.devno), MINOR(dev.devno));
	
	// initialize character device driver
	cdev_init(&dev.cdev, &ilitek_fops);
	dev.cdev.owner = THIS_MODULE;
    	ret = cdev_add(&dev.cdev, dev.devno, 1);
    	if(ret < 0){
        	printk(ILITEK_ERROR_LEVEL "%s, add character device error, ret %d\n", __func__, ret);
		return ret;
	}
	dev.class = class_create(THIS_MODULE, ILITEK_FILE_DRIVER_NAME);
	if(IS_ERR(dev.class)){
        	printk(ILITEK_ERROR_LEVEL "%s, create class, error\n", __func__);
		return ret;
    	}
	device_create(dev.class, NULL, dev.devno, NULL, "ilitek_ctrl");
	Report_Flag=0;
	
    //某些客戶因為要做兼容的目的，
    //需要設置標誌位來表明已經有TP driver註冊了
	ilitek_set_finish_init_flag();
	return 0;
}

/*
description
	driver exit function
parameters
	none
return
	nothing
*/
static void 
ilitek_exit(
	void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&i2c.early_suspend);
#endif
	// delete i2c driver
	if(i2c.client->irq != 0){
        	if(i2c.valid_irq_request != 0){
                    ilitek_free_irq(i2c.client->irq, &i2c);
                	printk(ILITEK_DEBUG_LEVEL "%s, free irq\n", __func__);
                	if(i2c.irq_work_queue){
                        	destroy_workqueue(i2c.irq_work_queue);
                        	printk(ILITEK_DEBUG_LEVEL "%s, destory work queue\n", __func__);
                	}
        	}
	}
	else{
        	if(i2c.thread != NULL){
                	kthread_stop(i2c.thread);
                	printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread\n", __func__);
        	}
	}
        if(i2c.valid_i2c_register != 0){
                i2c_del_driver(&ilitek_i2c_driver);
                printk(ILITEK_DEBUG_LEVEL "%s, delete i2c driver\n", __func__);
        }
        if(i2c.valid_input_register != 0){
                input_unregister_device(i2c.input_dev);
                printk(ILITEK_DEBUG_LEVEL "%s, unregister i2c input device\n", __func__);
        }
        
	// delete character device driver
	cdev_del(&dev.cdev);
	unregister_chrdev_region(dev.devno, 1);
	device_destroy(dev.class, dev.devno);
	class_destroy(dev.class);
	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
}

/* set init and exit function for this module */
module_init(ilitek_init);
module_exit(ilitek_exit);

