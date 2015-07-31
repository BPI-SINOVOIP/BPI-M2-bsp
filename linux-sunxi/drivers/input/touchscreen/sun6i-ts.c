/* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
*
* Copyright (c) 2009
*
* ChangeLog
*
*
*/
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/mm.h> 
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/tick.h>
#include <asm-generic/cputime.h>

#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif


static int tp_flag = 0;

/* tp status value */
#define TP_INITIAL             (-1)
#define TP_DOWN                (0)
#define TP_UP                  (1) 
#define TP_DATA_VA             (2)  

#define DUAL_TOUCH             (dual_touch_distance)
#define TOUCH_CHANGE           (3)
#define TP_DATA_AV_NO          (0x3)

//#define FIX_ORIENTATION
#define ORIENTATION_DEFAULT_VAL   (-1)
//#define TP_INT_PERIOD_TEST
//#define TP_TEMP_DEBUG
//#define TP_FREQ_DEBUG

#define TP_FIX_CENTER

#define IRQ_TP                 (60)
#define TP_BASSADDRESS         (0xf1c25000)
#define TP_CTRL0               (0x00)
#define TP_CTRL1               (0x04)
#define TP_CTRL2               (0x08)
#define TP_CTRL3               (0x0c)
#define TP_INT_FIFOC           (0x10)
#define TP_INT_FIFOS           (0x14)
#define TP_TPR                 (0x18)
#define TP_CDAT                (0x1c)
#define TEMP_DATA              (0x20)
#define TP_DATA                (0x24)


#define ADC_FIRST_DLY          (0x1<<24)
#define ADC_FIRST_DLY_MODE     (0x1<<23) 
#define ADC_CLK_SELECT         (0x0<<22)
#define ADC_CLK_DIVIDER        (0x2<<20)    
//#define CLK                    (6)
#define CLK                    (7)
#define FS_DIV                 (CLK<<16)
#define ACQ                    (0x3f)
#define T_ACQ                  (ACQ)

#define STYLUS_UP_DEBOUNCE     (0<<12)
#define STYLUS_UP_DEBOUCE_EN   (0<<9)
#define TOUCH_PAN_CALI_EN      (1<<7)
#define TP_DUAL_EN             (1<<6)
#define TP_MODE_EN             (1<<5)
#define TP_ADC_SELECT          (0<<4)
#define ADC_CHAN_SELECT        (0)

//#define TP_SENSITIVE_ADJUST    (0xf<<28)
#define TP_SENSITIVE_ADJUST    (tp_sensitive_level<<28)        /* mark by young for test angda 5" 0xc */
#define TP_MODE_SELECT         (0x1<<26)
#define PRE_MEA_EN             (0x1<<24)
#define PRE_MEA_THRE_CNT       (tp_press_threshold<<0)         /* 0x1f40 */


#define FILTER_EN              (1<<2)
#define FILTER_TYPE            (0x01<<0)

#define TP_DATA_IRQ_EN         (1<<16)
#define TP_DATA_XY_CHANGE      (tp_exchange_x_y<<13)           /* tp_exchange_x_y */
#define TP_FIFO_TRIG_LEVEL     (3<<8)
#define TP_FIFO_FLUSH          (1<<4)
#define TP_UP_IRQ_EN           (1<<1)
#define TP_DOWN_IRQ_EN         (1<<0)

#define FIFO_DATA_PENDING      (1<<16)
#define TP_UP_PENDING          (1<<1)
#define TP_DOWN_PENDING        (1<<0)

#define SINGLE_TOUCH_MODE      (1)
#define CHANGING_TO_DOUBLE_TOUCH_MODE (2)
#define DOUBLE_TOUCH_MODE      (3)
#define UP_TOUCH_MODE           (4)

#define SINGLE_CNT_LIMIT       (40)
#define DOUBLE_CNT_LIMIT       (2)
#define UP_TO_SINGLE_CNT_LIMIT (10)
                              
#define TPDATA_MASK            (0xfff)
#define FILTER_NOISE_LOWER_LIMIT  (2)
#define MAX_DELTA_X            (700-100)                       /* avoid excursion */
#define MAX_DELTA_Y            (1200-200)
#define X_TURN_POINT           (330)                           /* x1 < (1647 - MAX_DELTA_X) /3 */
#define X_COMPENSATE           (4*X_TURN_POINT)
#define Y_TURN_POINT           (260)                           /* y1 < (1468 -MAX_DELTA_Y ) */
#define Y_COMPENSATE           (2*Y_TURN_POINT)

#ifdef TP_FIX_CENTER
#define X_CENTER_COORDINATE    (2048)                          /* for construct two point */ 
#define Y_CENTER_COORDINATE    (2048)
#else
#define X_CENTER_COORDINATE    (sample_data->x)
#define Y_CENTER_COORDINATE    (sample_data->y)
#endif

#define CYCLE_BUFFER_SIZE      (64)                            /* must be 2^n */
#define DELAY_PERIOD           (6)                             /* delay 60 ms, unit is 10ms */ 
#define DELAY_COMPENSTAE_PEROID   (3)                          /* the os is busy, can not process the data in time. */

#define ZOOM_CHANGE_LIMIT_CNT  (3)
#define ZOOM_IN                (1)
#define ZOOM_OUT               (2)
#define ZOOM_INIT_STATE        (3)
#define ZOOM_STATIC            (4)

#define SAMPLE_TIME            (9.6)                           /* unit is ms. ??? */  
#define SAMPLE_TIME_FACTOR     (9.6/SAMPLE_TIME)

#define MIN_DX                 (DUAL_TOUCH*20)
#define MIN_DY                 (DUAL_TOUCH*20)
#define MAX_DX                 (DUAL_TOUCH*40)
#define MAX_DY                 (DUAL_TOUCH*40)
#define DELTA_DS_LIMIT         (1)
#define HOLD_DS_LIMIT          (3)
#define ZOOM_IN_CNT_LIMIT      (3)
#define TOTAL_TIMES            (4)
#define FIRST_ZOOM_IN_COMPENSTATE  (3)                         /* actually zoom out, usually with zoom in ops first. */
#define ZOOM_OUT_CNT_LIMIT         (tp_regidity_level)         /* related with screen's regidity */
#define GLIDE_DELTA_DS_MAX_TIMES   (4)
#define GLIDE_DELTA_DS_MAX_LIMIT   (glide_delta_ds_max_limit)


#ifndef TRUE
#define TRUE   1
#define FALSE  0
#endif

enum {
	DEBUG_REPORT_STATUS_INFO = 1U << 0,
	DEBUG_REPORT_DATA_INFO = 1U << 1,
	DEBUG_INT_INFO = 1U << 2,
	DEBUG_FILTER_INFO = 1U << 3,
	DEBUG_TASKLET_INFO = 1U << 4,
	DEBUG_ORIENTATION_INFO  = 1U << 5,
	DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO = 1U << 6,
	DEBUG_SUSPEND_INFO = 1U << 7,
	DEBUG_INIT = 1U << 8,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

struct sun6i_ts_data {
	struct resource *res;
	struct input_dev *input;
	void __iomem *base_addr;
	int irq;
	char phys[32];
	unsigned int count;                  /* for report threshold & touchmod(double to single touch mode) change */
	unsigned long buffer_head;           /* for cycle buffer */
	unsigned long buffer_tail;
	int ts_sample_status;                /* for touchscreen status when sampling */
	int ts_process_status;               /* for record touchscreen status when process data */
	int double_point_cnt;                /* for noise reduction when in double_touch_mode */
	int single_touch_cnt;                /* for noise reduction when change to double_touch_mode */
	int ts_delay_period;                 /* will determine responding sensitivity */
	int touchflag;
#ifdef CONFIG_HAS_EARLYSUSPEND	
	struct early_suspend early_suspend;
#endif
};

struct ts_sample_data {
	unsigned int x1;
	unsigned int y1;
	unsigned int x;
	unsigned int y;
	unsigned int dx;
	unsigned int dy;
	unsigned int z1;
	unsigned int z2;
	int sample_status;                   /* record the sample point status when sampling */
	unsigned int sample_time;
};

struct sun6i_ts_data * mtTsData =NULL;	
static int touch_mode = UP_TOUCH_MODE;
static int change_mode = TRUE;
static int tp_irq_state = TRUE;

static cputime64_t cur_wall_time = 0L;

static struct ts_sample_data cycle_buffer[CYCLE_BUFFER_SIZE] = {{0},};
static struct ts_sample_data *prev_sample;
static struct ts_sample_data *prev_data_sample;
static struct timer_list data_timer;
/* when 1, mean tp driver have recervied data, and begin to report data, and start timer to reduce up&down signal */
static int data_timer_status = 0; 
static struct ts_sample_data prev_single_sample;
static struct ts_sample_data prev_double_sample_data;
static struct ts_sample_data prev_report_samp;
static int orientation_flag = 0;
static int zoom_flag = 0;
static int accmulate_zoom_out_ds = 0;
static int accmulate_zoom_in_ds = 0;
static int zoom_in_count = 0;
static int zoom_out_count = 0;
static int zoom_change_cnt = 0;
static int hold_cnt = 0;
/* config from sysconfig files. */
static int glide_delta_ds_max_limit = 0;
static int tp_regidity_level = 0;
static int tp_press_threshold_enable = 0;
static int tp_press_threshold = 0;     /* usded to adjust sensitivity of touch */
static int tp_sensitive_level = 0;     /* used to adjust sensitivity of pen down detection */
static int tp_exchange_x_y = 0;

#define ZOOM_IN_OUT_BUFFER_SIZE_TIMES (2)
#define ZOOM_IN_OUT_BUFFER_SIZE (1<<ZOOM_IN_OUT_BUFFER_SIZE_TIMES)
#define TRANSFER_DATA_BUFFER_SIZE     (4)

static struct ts_sample_data zoom_in_data_buffer[ZOOM_IN_OUT_BUFFER_SIZE] = {{0},};
static struct ts_sample_data zoom_out_data_buffer[ZOOM_IN_OUT_BUFFER_SIZE] = {{0},};
static struct ts_sample_data transfer_data_buffer[TRANSFER_DATA_BUFFER_SIZE] = {{0},};

static int zoom_in_buffer_cnt = 0;
static int zoom_out_buffer_cnt = 0;

static spinlock_t tp_do_tasklet_sync;
static int tp_do_tasklet_running = 0;
/* for test */
static int dual_touch_distance = 0;

#ifdef PRINT_UP_SEPARATOR
static int separator_flag = 0;
#endif

static int reported_single_point_cnt = 0;
static int reported_data_start_time = 0;

static atomic_t report_up_event_implement_sync = ATOMIC_INIT(1);
static int report_up_event_implement_running = 0;

static int reference_point_flag = 1;

void tp_do_tasklet(unsigned long data);
DECLARE_TASKLET(tp_tasklet,tp_do_tasklet,0);

static int  tp_init(void);
/* ͣ���豸 */
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sun6i_ts_early_suspend(struct early_suspend *h)
{
	dprintk(DEBUG_SUSPEND_INFO, "[%s] enter standby state: %d. \n", __FUNCTION__, (int)standby_type);
	
	if (NORMAL_STANDBY == standby_type) {
		writel(0,TP_BASSADDRESS + TP_CTRL1);
	} 
	/*process for super standby*/	
	return ;
}

/* ���»��� */
static void sun6i_ts_late_resume(struct early_suspend *h)
{
	dprintk(DEBUG_SUSPEND_INFO, "[%s] return from standby state: %d. \n", __FUNCTION__, (int)standby_type); 

	/*process for normal standby*/
	if (NORMAL_STANDBY == standby_type) {
		 writel(STYLUS_UP_DEBOUNCE|STYLUS_UP_DEBOUCE_EN|TP_DUAL_EN|TP_MODE_EN,TP_BASSADDRESS + TP_CTRL1);
	/*process for super standby*/	
	} else if(SUPER_STANDBY == standby_type) {
		tp_init();
	}
	return ;
}
#else
/* ͣ���豸 */
#ifdef CONFIG_PM
static int sun6i_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	dprintk(DEBUG_SUSPEND_INFO, "[%s] enter standby state: %d. \n", __FUNCTION__, (int)standby_type);
	
	if (NORMAL_STANDBY == standby_type) {
		writel(0,TP_BASSADDRESS + TP_CTRL1);
	} 
	/*process for super standby*/	
	return 0;
}

static int sun6i_ts_resume(struct platform_device *pdev)
{ 
        dprintk(DEBUG_SUSPEND_INFO, "[%s] return from standby state: %d. \n", __FUNCTION__, (int)standby_type); 

	/*process for normal standby*/
	if (NORMAL_STANDBY == standby_type) {
		 writel(STYLUS_UP_DEBOUNCE|STYLUS_UP_DEBOUCE_EN|TP_DUAL_EN|TP_MODE_EN,TP_BASSADDRESS + TP_CTRL1);
	/*process for super standby*/	
	} else if(SUPER_STANDBY == standby_type) {
		tp_init();
	}
	return 0;
}
#endif
#endif
	
static int  tp_init(void)
{	
	/* TP_CTRL0: 0x0027003f */
#ifdef TP_FREQ_DEBUG
	writel(0x0028001f, TP_BASSADDRESS + TP_CTRL0);
#else
	writel(ADC_CLK_DIVIDER|FS_DIV|T_ACQ, TP_BASSADDRESS + TP_CTRL0);	   
#endif
	/* TP_CTRL2: 0xc4000000 */
	if (1 == tp_press_threshold_enable) {
		writel(TP_SENSITIVE_ADJUST |TP_MODE_SELECT | PRE_MEA_EN | PRE_MEA_THRE_CNT, TP_BASSADDRESS + TP_CTRL2);
	} else {
		writel(TP_SENSITIVE_ADJUST|TP_MODE_SELECT,TP_BASSADDRESS + TP_CTRL2);
  }
    

	/* TP_CTRL3: 0x05 */
#ifdef TP_FREQ_DEBUG
	writel(0x06, TP_BASSADDRESS + TP_CTRL3);
#else
	writel(FILTER_EN|FILTER_TYPE,TP_BASSADDRESS + TP_CTRL3);
#endif
    
#ifdef TP_TEMP_DEBUG
	/* TP_INT_FIFOC: 0x00010313 */
	writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN|0x40000, TP_BASSADDRESS + TP_INT_FIFOC);
	writel(0x10fff, TP_BASSADDRESS + TP_TPR);
#else
	/* TP_INT_FIFOC: 0x00010313 */
	writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN, TP_BASSADDRESS + TP_INT_FIFOC);
#endif
	/* TP_CTRL1: 0x00000070 -> 0x00000030 */
	writel(TP_DATA_XY_CHANGE|STYLUS_UP_DEBOUNCE|STYLUS_UP_DEBOUCE_EN|TP_DUAL_EN|TP_MODE_EN,TP_BASSADDRESS + TP_CTRL1);
    
	return (0);
}
static void change_to_single_touch_mode(void)
{
	touch_mode = SINGLE_TOUCH_MODE;
	reported_single_point_cnt = 0;
	return;
}

static void backup_transfered_data(struct ts_sample_data *sample_data)
{
	static int index = 0;
	
	index = reported_single_point_cnt%TRANSFER_DATA_BUFFER_SIZE;
	transfer_data_buffer[index].dx = sample_data->dx;
	transfer_data_buffer[index].dy = sample_data->dy;
	transfer_data_buffer[index].x = sample_data->x;
	transfer_data_buffer[index].y = sample_data->y;
	
	if (reported_single_point_cnt > (TRANSFER_DATA_BUFFER_SIZE<<10)) {
		reported_single_point_cnt -= (TRANSFER_DATA_BUFFER_SIZE<<9);
	}
	reported_single_point_cnt++;
	    
	return;
}

static void report_single_point_implement(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{
	input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
	input_report_abs(ts_data->input, ABS_MT_POSITION_X, sample_data->x);
	input_report_abs(ts_data->input, ABS_MT_POSITION_Y, sample_data->y);   
	
	dprintk(DEBUG_REPORT_DATA_INFO, "report single point: x = %d,sample_data->y = %d. \n", sample_data->x, sample_data->y);
	    
	input_mt_sync(ts_data->input);                 
	input_sync(ts_data->input);
	
	return;
}

static void report_single_point(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{
	backup_transfered_data(sample_data);
	report_single_point_implement(ts_data, sample_data);
	return;
}

static void report_slide_data(struct sun6i_ts_data *ts_data)
{
	int start = 0;
	int end = 0;
	int dx = 0;
	int dy = 0;
	int ds_times = 4;
	/* index = reported_single_point_cnt%TRANSFER_DATA_BUFFER_SIZE; */
	struct ts_sample_data sample_data;
    
	if (reported_single_point_cnt <= 1) {
		//only one reference point, can not judge direction */
		dx = -MIN_DX;
		dy = MIN_DY;
		end = (reported_single_point_cnt-1)%TRANSFER_DATA_BUFFER_SIZE;
		sample_data.x = max(0, min(4096, (int)(transfer_data_buffer[end].x + dx)));
		sample_data.y = max(0, min(4096, (int)(transfer_data_buffer[end].y + dy)));

		report_single_point_implement(ts_data, &sample_data);
		
		sample_data.x = transfer_data_buffer[end].x;
		sample_data.y = transfer_data_buffer[end].y;
	} else	{
		if (reported_single_point_cnt <=TRANSFER_DATA_BUFFER_SIZE)
			start = 0;
		else
			start = reported_single_point_cnt - TRANSFER_DATA_BUFFER_SIZE;
		start = start%TRANSFER_DATA_BUFFER_SIZE;
		end = (reported_single_point_cnt-1)%TRANSFER_DATA_BUFFER_SIZE;
		dx = transfer_data_buffer[end].x -transfer_data_buffer[start].x;
		dy = transfer_data_buffer[end].y -transfer_data_buffer[start].y;
		if(dx < 0){
			dx = -dx;
			dx = min(MAX_DX, max(MIN_DX, dx*ds_times/(end - start)));
			dx = -dx;
		} else {
			dx = min(MAX_DX, max(MIN_DX, dx*ds_times/(end - start)));
		}
		
		if(dy < 0){
			dy = -dy;
			dy = min(MAX_DY, max(MIN_DY, dy*ds_times/(end - start)));
			dy = -dy;
		} else {
			dy = min(MAX_DY, max(MIN_DY, dy*ds_times/(end - start)));
		}
		sample_data.x = max(0, min(4096, (int)(transfer_data_buffer[end].x + dx)));
		sample_data.y = max(0, min(4096, (int)(transfer_data_buffer[end].y + dy)));

		report_single_point_implement(ts_data, &sample_data);
	}
}

static void report_up_event_implement(struct sun6i_ts_data *ts_data)
{
	static const int UP_EVENT_DELAY_TIME = 3;
	static const int SLIDE_MIN_CNT = 3;
	if(atomic_sub_and_test(1, &report_up_event_implement_sync)) {
		/* get the resource */
		if (1 == report_up_event_implement_running) {
			atomic_inc(&report_up_event_implement_sync);
			printk("other thread is running the rountine. \n");
			return;	
		} else {
			report_up_event_implement_running = 1;
			atomic_inc(&report_up_event_implement_sync);
		}    
	} else {
		printk("failed to get the lock. other thread is using the lock. \n");
		return;
	}

	if (0 == data_timer_status) {
		printk("report_up_event_implement have been called. \n");
		goto report_up_event_implement_out;
		return;
	}

	dprintk(DEBUG_REPORT_STATUS_INFO, "enter report_up_event_implement. jiffies == %lu. \n", jiffies);
	
	if ( (SINGLE_TOUCH_MODE == touch_mode) && \
	(reported_single_point_cnt<SLIDE_MIN_CNT) && (reported_single_point_cnt>0) && \
	(prev_sample->sample_time >= (prev_data_sample->sample_time + UP_EVENT_DELAY_TIME))) {
		/*  obvious, a slide, how to compenstate? */
		report_slide_data(ts_data);
	}
    
	/*  note: below operation may be interfere by intterrupt, but it does not matter */
	input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,0);
	input_sync(ts_data->input);
	del_timer(&data_timer);
	data_timer_status = 0;
	ts_data->ts_process_status = TP_UP;
	ts_data->double_point_cnt = 0;
	ts_data->touchflag = 0; 
	touch_mode = UP_TOUCH_MODE;
	change_mode = TRUE;
	reported_single_point_cnt = 0;
	reported_data_start_time = 0;

report_up_event_implement_out:
	report_up_event_implement_running = 0;

#ifdef PRINT_UP_SEPARATOR
	printk("separator: #######%d, %d, %d###########. \n\n\n\n\n\n\n", separator_flag, separator_flag, separator_flag);
	separator_flag++;
#endif

    return;
}

static int judge_zoom_orientation(struct ts_sample_data *sample_data)
{
	int dx,dy;
	int ret = 0;
	if (1 == reference_point_flag) {
		dx = sample_data->x - prev_single_sample.x;
		dy = sample_data->y - prev_single_sample.y;
		if (dx*dy > 0) {
			ret = -1;
		} else if (dx*dy < 0) {
			ret = 1;
		}     
	} else	{
		dprintk(DEBUG_ORIENTATION_INFO, "judge_zoom_orientation: lack reference point .\n");
	}
	       
	dprintk(DEBUG_ORIENTATION_INFO, "sun6i-ts: orientation_flag == %d . \n", ret);
	return ret;
}
static void filter_double_point_init(struct ts_sample_data *sample_data, int backup_samp_flag)
{
	/* backup prev_double_sample_data */
	zoom_flag = ZOOM_INIT_STATE;
	accmulate_zoom_out_ds = 0;
	zoom_out_count = 0;
	accmulate_zoom_in_ds = 0;
	zoom_in_count = 0;
	if (1 == backup_samp_flag) {
		memcpy((void*)(&prev_double_sample_data), (void*)sample_data, sizeof(*sample_data));	
	}

	hold_cnt = 0;
	/* when report two point, the first two point will be reserved for reference purpose. */
	return;
}

static void change_to_double_mode(struct sun6i_ts_data *ts_data)
{
	if((CHANGING_TO_DOUBLE_TOUCH_MODE != touch_mode) && \
	(DOUBLE_TOUCH_MODE != touch_mode)&& \
	(UP_TOUCH_MODE != touch_mode)) {
		printk("change_to_double_mode: err, not the expected state. touch_mode = %d. \n", touch_mode);
	}
	touch_mode = DOUBLE_TOUCH_MODE;
	change_mode = FALSE;
	ts_data->single_touch_cnt = 0; /* according this counter, change to single touch mode */
	return;
}

static void change_to_zoom_in(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{ 
	zoom_flag = ZOOM_IN;
	zoom_change_cnt = 0;
	accmulate_zoom_out_ds = 0;
	zoom_out_count = 0;
	change_to_double_mode(ts_data);
	return;
}

static void change_to_zoom_out(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{
	zoom_flag = ZOOM_OUT;
	zoom_change_cnt = 0;
	accmulate_zoom_in_ds = 0;
	zoom_in_count = 0;
	change_to_double_mode(ts_data);
	return;
}

static void filter_zoom_in_data_init(void)
{
	zoom_in_buffer_cnt = 0;
	return;
}

static void filter_zoom_out_data_init(void)
{
	zoom_out_buffer_cnt = 0;
	return;
}
static void filter_zoom_in_data(struct ts_sample_data * report_data, struct ts_sample_data *sample_data)
{
	static int i = 0; 
	static int index = 0;
	static int count = 0;
	/* backup data to filter noise, only when ds < 40, need this operation. */

	index = zoom_in_buffer_cnt%ZOOM_IN_OUT_BUFFER_SIZE;
	zoom_in_data_buffer[index].dx = sample_data->dx;
	zoom_in_data_buffer[index].dy = sample_data->dy;
	zoom_in_data_buffer[index].x = sample_data->x;
	zoom_in_data_buffer[index].y = sample_data->y;
	        
	if(zoom_in_buffer_cnt > (ZOOM_IN_OUT_BUFFER_SIZE<<10)){
		zoom_in_buffer_cnt -= (ZOOM_IN_OUT_BUFFER_SIZE<<9);
	}
	        
	if(zoom_in_buffer_cnt >= ZOOM_IN_OUT_BUFFER_SIZE){
		index = ZOOM_IN_OUT_BUFFER_SIZE - 1;
	} 
	/* index mean the real count. */
	sample_data->dx = 0;
	sample_data->dy = 0;
	sample_data->x = 0;
	sample_data->y = 0;
	count = 0;
	for (i = 0; i <=  index; i++) {
		sample_data->dx += zoom_in_data_buffer[i].dx;
		sample_data->dy += zoom_in_data_buffer[i].dy;
		sample_data->x += zoom_in_data_buffer[i].x;
		sample_data->y += zoom_in_data_buffer[i].y;
		count++;
	}
      
	sample_data->dx /= count;
	sample_data->dy /= count;
	sample_data->x /= count;
	sample_data->y /= count;

	report_data->x = sample_data->x;
	report_data->y = sample_data->y;
	report_data->dx = sample_data->dx;
	report_data->dy = sample_data->dy;
        
	zoom_in_buffer_cnt++;
        
	return;
}

static void filter_zoom_out_data(struct ts_sample_data * report_data, struct ts_sample_data *sample_data)
{
	static int i = 0; 
	static int index = 0;
	static int count = 0;
	/* backup data to filter noise, only when ds < 40, need this operation. */

	index = zoom_out_buffer_cnt%ZOOM_IN_OUT_BUFFER_SIZE;
	zoom_out_data_buffer[index].dx = sample_data->dx;
	zoom_out_data_buffer[index].dy = sample_data->dy;
	zoom_out_data_buffer[index].x = sample_data->x;
	zoom_out_data_buffer[index].y = sample_data->y;
	        
	if (zoom_out_buffer_cnt > (ZOOM_IN_OUT_BUFFER_SIZE<<10)) {
		zoom_out_buffer_cnt -= (ZOOM_IN_OUT_BUFFER_SIZE<<9);
	}
	        
	if (zoom_out_buffer_cnt >= ZOOM_IN_OUT_BUFFER_SIZE) {
		index = ZOOM_IN_OUT_BUFFER_SIZE - 1;
	}
	/* index mean the real count. */
	sample_data->dx = 0;
	sample_data->dy = 0;
	sample_data->x = 0;
	sample_data->y = 0;
	count = 0;
	        
	for(i = 0; i <=  index; i++) {
		sample_data->dx += zoom_out_data_buffer[i].dx;
		sample_data->dy += zoom_out_data_buffer[i].dy;
		sample_data->x += zoom_out_data_buffer[i].x;
		sample_data->y += zoom_out_data_buffer[i].y;
		count++;
	}
      
	sample_data->dx /= count;
	sample_data->dy /= count;
	sample_data->x /= count;
	sample_data->y /= count;

	report_data->x = sample_data->x;
	report_data->y = sample_data->y;
	report_data->dx = sample_data->dx;
	report_data->dy = sample_data->dy;
	        
	zoom_out_buffer_cnt++;
	        
	return;
}

static int filter_double_point(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{
	int ret = 0;
	static int prev_sample_ds = 0;
	static int cur_sample_ds = 0;
	static int delta_ds = 0;
    
	if (ZOOM_INIT_STATE == zoom_flag && (0 == zoom_out_count && 0 ==  zoom_in_count)) {
		prev_sample_ds = int_sqrt((prev_double_sample_data.dx)*(prev_double_sample_data.dx) \
			+ (prev_double_sample_data.dy)*(prev_double_sample_data.dy));
	}
    
	cur_sample_ds = int_sqrt((sample_data->dx)*(sample_data->dx) + (sample_data->dy)*(sample_data->dy));
	delta_ds = cur_sample_ds - prev_sample_ds;
	    
	/* update prev_double_sample_data */
	memcpy((void*)&prev_double_sample_data, (void*)sample_data, sizeof(*sample_data));
	prev_sample_ds = cur_sample_ds;
	    
	if(delta_ds > HOLD_DS_LIMIT){                    /* zoom in */
	        
		if(ZOOM_OUT == zoom_flag) {                    /* zoom in when zoom out */
			if(delta_ds > min(GLIDE_DELTA_DS_MAX_LIMIT, (GLIDE_DELTA_DS_MAX_TIMES*accmulate_zoom_out_ds/zoom_out_count))) {
				/* noise */
				cur_sample_ds = prev_sample_ds;            /* discard the noise, and can not be reference. */
				dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "sun6i-ts: noise, zoom in when zoom out. \n");
				ret = TRUE;
			} else {
				/* normal zoom in */
				zoom_change_cnt++;
				accmulate_zoom_in_ds += delta_ds;
				zoom_in_count++;
				if(zoom_change_cnt > ZOOM_IN_CNT_LIMIT) {
					dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "change to ZOOM_IN from ZOOM_OUT. \n");
					change_to_zoom_in(ts_data, sample_data);
					filter_zoom_in_data_init();
					filter_zoom_in_data(&prev_report_samp, sample_data);
				} else {
					dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "sun6i-ts: normal zoom in, but this will cause twitter. \n");
					ret = TRUE;
				}
			}
		} else  if(ZOOM_IN == zoom_flag) {
			if(delta_ds > min(GLIDE_DELTA_DS_MAX_LIMIT, (GLIDE_DELTA_DS_MAX_TIMES*accmulate_zoom_in_ds/zoom_in_count))){
				cur_sample_ds = prev_sample_ds;            /* discard the noise, and can not be reference. */
				ret = TRUE;
			} else {
				accmulate_zoom_in_ds += delta_ds;
				zoom_in_count++;
				filter_zoom_in_data(&prev_report_samp, sample_data);
			}
			zoom_change_cnt = 0;
			accmulate_zoom_out_ds = 0;
			zoom_out_count = 0;
		}else if (ZOOM_INIT_STATE == zoom_flag ||ZOOM_STATIC == zoom_flag) {
			zoom_in_count++;
			if(zoom_in_count > (ZOOM_CHANGE_LIMIT_CNT + FIRST_ZOOM_IN_COMPENSTATE)) {
				accmulate_zoom_in_ds = delta_ds;
				zoom_in_count = 1;
				if(ZOOM_INIT_STATE == zoom_flag) {
					orientation_flag = judge_zoom_orientation(sample_data);
					report_up_event_implement(ts_data);
				}
				filter_zoom_in_data_init();
				filter_zoom_in_data(&prev_report_samp, sample_data);
				dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "change to ZOOM_IN from ZOOM_INIT_STATE. \n");
				change_to_zoom_in(ts_data, sample_data);
				#if 1
				dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "ZOOM_INIT_STATE: delta_ds= %d. \n", delta_ds);
				#endif
			} else	{
				ret = TRUE;
			}
		}        
	} else if (delta_ds<(-HOLD_DS_LIMIT)) {                        /* zoom out */   
		delta_ds = -delta_ds;
		        
		if(ZOOM_IN == zoom_flag) {                                   /* zoom out when zoom in */
			dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "delta_ds = %d, (4*accmulate_zoom_in_ds/zoom_in_count) = %d. \n", \
				-delta_ds, (4*accmulate_zoom_in_ds/zoom_in_count));
			if (delta_ds > min(GLIDE_DELTA_DS_MAX_LIMIT, (GLIDE_DELTA_DS_MAX_TIMES*accmulate_zoom_in_ds/zoom_in_count))) {  /* noise */
	 			
	 			cur_sample_ds = prev_sample_ds;                          /* discard the noise, and can not be reference. */
	 			dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "sun6i-ts: noise, zoom out when zoom in. \n");
	 			
	 			ret = TRUE;
			} else {                                                   /* normal zoom out */
				zoom_change_cnt++;
				accmulate_zoom_out_ds += delta_ds;
				zoom_out_count++;
				if (zoom_change_cnt > ZOOM_OUT_CNT_LIMIT) {
					dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "change to ZOOM_OUT from ZOOM_IN. \n");
					change_to_zoom_out(ts_data, sample_data);
					filter_zoom_out_data_init();
					filter_zoom_out_data(&prev_report_samp, sample_data);
				} else {
					dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "sun6i-ts: normal zoom out, but this will cause twitter. \n");
					ret = TRUE;
				}
			}
		}else if(ZOOM_OUT == zoom_flag){                             /* zoom out when zoom out */            
			if (delta_ds > min(GLIDE_DELTA_DS_MAX_LIMIT, (GLIDE_DELTA_DS_MAX_TIMES*accmulate_zoom_out_ds/zoom_out_count))) {
				cur_sample_ds = prev_sample_ds;
				ret = TRUE;
			} else {
				accmulate_zoom_out_ds += delta_ds;
				zoom_out_count++; 
				filter_zoom_out_data(&prev_report_samp, sample_data);
			}
			zoom_change_cnt = 0;
			accmulate_zoom_in_ds = 0;
			zoom_in_count = 0;
		} else if (ZOOM_INIT_STATE == zoom_flag ||ZOOM_STATIC == zoom_flag) {
			zoom_out_count ++;
			if (zoom_out_count > ZOOM_CHANGE_LIMIT_CNT) {
				accmulate_zoom_out_ds = delta_ds;
				zoom_out_count = 1;
				if (ZOOM_INIT_STATE == zoom_flag) {
					orientation_flag = judge_zoom_orientation(sample_data);
					report_up_event_implement(ts_data);
				}
				filter_zoom_out_data_init();
				filter_zoom_out_data(&prev_report_samp, sample_data);
				dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "change to ZOOM_OUT from ZOOM_INIT_STATE. \n");
				change_to_zoom_out(ts_data, sample_data);
#if 1
				dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "ZOOM_INIT_STATE: delta_ds= %d. \n", delta_ds);
#endif
			} else {
				/* have not known orientation, discard the point */
				ret = TRUE;
			}            
		}      
	} else {    
		hold_cnt++;
		cur_sample_ds = prev_sample_ds;
		if (hold_cnt > 100000)
			hold_cnt = 100;	

		if (unlikely(ZOOM_INIT_STATE == zoom_flag )) {
			dprintk(DEBUG_FILTER_DOUBLE_POINT_STATUS_INFO, "ZOOM_INIT_STATE: delta_ds == %d. \n", delta_ds);
			if(hold_cnt <= ZOOM_CHANGE_LIMIT_CNT) { //discard the first 3 point
				ret = TRUE;
			} else {
				/* when change to static mode, and not know orientation yet, need judge orientation. */
				orientation_flag = judge_zoom_orientation(sample_data); 
				report_up_event_implement(ts_data);
				zoom_flag = ZOOM_STATIC;
				change_to_double_mode(ts_data);
				memcpy((void*)&prev_report_samp, (void*)sample_data, sizeof(*sample_data));
			}
		} else {
			memcpy((void*)sample_data, (void*)&prev_report_samp, sizeof(*sample_data));
		} 			
	}
	return ret;
}

static void report_double_point(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{
	int x1,x2,y1,y2;

	y1 = 0;
	y2 = 0;

	if (TRUE == filter_double_point(ts_data, sample_data)) { //noise
		return;
	}
        
	/* when report double point, need to clear single_touch_cnt */
	ts_data->single_touch_cnt = 0;
        
	if (sample_data->dx < X_TURN_POINT) {
		x1 = X_CENTER_COORDINATE - (sample_data->dx<<2);
		x2 = X_CENTER_COORDINATE + (sample_data->dx<<2);  
	} else {
		x1 = X_CENTER_COORDINATE - X_COMPENSATE - ((sample_data->dx) - X_TURN_POINT);
		x2 = X_CENTER_COORDINATE + X_COMPENSATE + ((sample_data->dx) - X_TURN_POINT); 
	}
#ifdef FIX_ORIENTATION
	orientation_flag = ORIENTATION_DEFAULT_VAL;
#endif
	if(0 == orientation_flag) {
		dprintk(DEBUG_ORIENTATION_INFO, "orientation_flag: orientation is not supported or have not known, set the default orientation. \n");
		orientation_flag = ORIENTATION_DEFAULT_VAL;
	}
       
	if (-1 == orientation_flag) {   
		if (sample_data->dy < Y_TURN_POINT) {
			y1 = Y_CENTER_COORDINATE - (sample_data->dy<<1);
			y2 = Y_CENTER_COORDINATE + (sample_data->dy<<1);
		} else {
			y1 = Y_CENTER_COORDINATE - Y_COMPENSATE - (sample_data->dy - Y_TURN_POINT);
			y2 = Y_CENTER_COORDINATE + Y_COMPENSATE + (sample_data->dy - Y_TURN_POINT);
		}
	}else if (1 == orientation_flag) {          
		if(sample_data->dy < Y_TURN_POINT){
			y2 = Y_CENTER_COORDINATE - (sample_data->dy<<1);
			y1 = Y_CENTER_COORDINATE + (sample_data->dy<<1);
		} else {
			y2 = Y_CENTER_COORDINATE - Y_COMPENSATE - (sample_data->dy - Y_TURN_POINT);
			y1 = Y_CENTER_COORDINATE + Y_COMPENSATE + (sample_data->dy - Y_TURN_POINT);
		}
	}
	
	input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
	input_report_abs(ts_data->input, ABS_MT_POSITION_X, x1);
	input_report_abs(ts_data->input, ABS_MT_POSITION_Y, y1);
	input_mt_sync(ts_data->input);
		
	input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
	input_report_abs(ts_data->input, ABS_MT_POSITION_X, x2);
	input_report_abs(ts_data->input, ABS_MT_POSITION_Y, y2);
	input_mt_sync(ts_data->input);
	input_sync(ts_data->input);

	dprintk(DEBUG_REPORT_STATUS_INFO, "report two point: x1 = %d, y1 = %d; x2 = %d, y2 = %d. \n",x1, y1, x2, y2);
	dprintk(DEBUG_REPORT_STATUS_INFO, "sample_data->dx = %d, sample_data->dy = %d. \n", sample_data->dx, sample_data->dy);

	return;
}

static void report_data(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{
	if (TRUE == change_mode) {                                       /* only up event happened, change_mode is allowed. */
		printk("err: report_data: never execute. \n ");
		ts_data->single_touch_cnt++; 
		if (ts_data->single_touch_cnt > UP_TO_SINGLE_CNT_LIMIT) { 
			/* change to single touch mode */
			change_to_single_touch_mode();
			report_single_point(ts_data, sample_data);
			dprintk(DEBUG_REPORT_STATUS_INFO, "change touch mode to SINGLE_TOUCH_MODE from UP state. \n");
		}
	} else if (FALSE == change_mode) {
		/* keep in double touch mode 
		   remain in double touch mode */   
		ts_data->single_touch_cnt++;        
		if (ts_data->single_touch_cnt > SINGLE_CNT_LIMIT) {       /* to avoid unconsiously touch */
			/* change to single touch mode */
			change_to_single_touch_mode();
			report_single_point(ts_data, sample_data);            
			dprintk(DEBUG_REPORT_STATUS_INFO, "change touch mode to SINGLE_TOUCH_MODE from double_touch_mode. \n");
		}                                                                              
	}  
	return;
}

static void report_up_event(unsigned long data)
{
	struct sun6i_ts_data *ts_data = (struct sun6i_ts_data *)data;

	/*when the time is out, and the buffer data can not affect the timer to re-timing immediately,
	*this will happen, 
	*from this we can conclude, the delay_time is not proper, need to be longer
	*/
	if (ts_data->buffer_head != ts_data->buffer_tail) { 
		mod_timer(&data_timer, jiffies + ts_data->ts_delay_period);
		/* direct calling tasklet, do not use int bottom half, may result in some bad behavior.!!! */
		tp_do_tasklet(ts_data->buffer_head); 
		return;
	}
	report_up_event_implement(ts_data);
	return;
}

static void process_data(struct sun6i_ts_data *ts_data, struct ts_sample_data *sample_data)
{
	ts_data->touchflag = 1;
	if (((sample_data->dx) > DUAL_TOUCH)&&((sample_data->dy) > DUAL_TOUCH)) {
	 	ts_data->touchflag = 2;
	 	ts_data->double_point_cnt++;
		if (UP_TOUCH_MODE == touch_mode ) {
			dprintk(DEBUG_ORIENTATION_INFO, "sun6i-ts: need to get the single point. \n");
			
			reference_point_flag = 0;
			touch_mode = SINGLE_TOUCH_MODE;
		}
		if(ts_data->double_point_cnt > DOUBLE_CNT_LIMIT) {
			if(sample_data->dx < MAX_DELTA_X && sample_data->dy < MAX_DELTA_Y) { 
				if(SINGLE_TOUCH_MODE == touch_mode){
					touch_mode = CHANGING_TO_DOUBLE_TOUCH_MODE;
					orientation_flag = 0;
					filter_double_point_init(sample_data, 1);
					dprintk(DEBUG_ORIENTATION_INFO, "sun6i-ts: CHANGING_TO_DOUBLE_TOUCH_MODE orientation_flag == %d . \n", \
						orientation_flag);
					return;
				}
				report_double_point(ts_data, sample_data);
			}
		}   
	} else if(1 == ts_data->touchflag) {
		if (DOUBLE_TOUCH_MODE == touch_mode ) {
			/* normally, to really change to single_touch_mode, spend about 100ms */
			/* discard old data, remain in double_touch_mode,and change to ZOOM_INIT_STATE */
			if(6 == ts_data->single_touch_cnt ) {                         
				filter_zoom_in_data_init();
				filter_zoom_out_data_init(); 
				prev_single_sample.x = sample_data->x;
				prev_single_sample.y = sample_data->y;
				reference_point_flag = 1;
				orientation_flag = 0;
				filter_double_point_init(sample_data, 0);
			} else if (ts_data->single_touch_cnt > 6) {                   /* update prev_single_sample */
				prev_single_sample.x = sample_data->x;
				prev_single_sample.y = sample_data->y;
				reference_point_flag = 1;
			}
			report_data(ts_data, sample_data);
		/*remain in single touch mode */	
		} else if (SINGLE_TOUCH_MODE == touch_mode  ||UP_TOUCH_MODE == touch_mode  || CHANGING_TO_DOUBLE_TOUCH_MODE == touch_mode) {
			if (SINGLE_TOUCH_MODE == touch_mode  ||UP_TOUCH_MODE == touch_mode) {
				prev_single_sample.x = sample_data->x;
				prev_single_sample.y = sample_data->y;
				reference_point_flag = 1;
			}
			if(SINGLE_TOUCH_MODE != touch_mode)
				change_to_single_touch_mode();
	                 
			report_single_point(ts_data, sample_data);
		}                    
	}
    
	return;
}

void tp_do_tasklet(unsigned long data)
{
	struct sun6i_ts_data *ts_data = mtTsData;
	struct ts_sample_data *sample_data;
	int head = 0;
	int tail = 0;

	if (1 == spin_trylock(&tp_do_tasklet_sync)) {
		if(1 == tp_do_tasklet_running) {
			spin_unlock(&tp_do_tasklet_sync);
			return;	
		} else {
			tp_do_tasklet_running = 1;
			spin_unlock(&tp_do_tasklet_sync);
		}
	} else {
		return;	
	}	    	
	head = (int)data;
	tail = (int)ts_data->buffer_tail;                    /* tail may have changed, while the data is remain? */

	if((tail + CYCLE_BUFFER_SIZE*2) < head)              /* tail have been modify to avoid overflow */
		goto out;
    
	dprintk(DEBUG_TASKLET_INFO, "enter tasklet. head = %d, tail = %d. jiffies == %lu. \n", head, tail, jiffies);
	    
	while ((tail) < (head)) {                              /* when tail == head, mean the buffer is empty */
		sample_data = &cycle_buffer[tail&(CYCLE_BUFFER_SIZE-1)];
		tail++;
		dprintk(DEBUG_FILTER_INFO, "sample_data->sample_status == %d, ts_data->ts_process_status == %d \n", \
			sample_data->sample_status, ts_data->ts_process_status);

#ifdef TP_INT_PERIOD_TEST
		continue;
#endif
		if (TP_UP == sample_data->sample_status || TP_DOWN == sample_data->sample_status) {
			/* when received up & down event, reinitialize ts_data->double_point_cnt to debounce for DOUBLE_TOUCH_MODE */        
			ts_data->double_point_cnt = 0; 
	            
			if ((TP_DATA_VA == ts_data->ts_process_status || TP_DOWN == ts_data->ts_process_status) && data_timer_status) {
				/* delay   20ms , ignore up event & down event */
				dprintk(DEBUG_FILTER_INFO, "(prev_sample->sample_time + ts_data->ts_delay_period) == %u, \
					(sample_data->sample_time) == %u. \n", \
					(prev_sample->sample_time + ts_data->ts_delay_period), \
					(sample_data->sample_time));
				dprintk(DEBUG_FILTER_INFO, "up or down: sample_data->sample_time = %u.sample_data->sample_status = %d \n", \
					sample_data->sample_time, sample_data->sample_status);
				if (time_after_eq((unsigned long)(prev_sample->sample_time + ts_data->ts_delay_period - DELAY_COMPENSTAE_PEROID), \
					(unsigned long)(sample_data->sample_time))) {
					/* notice: sample_time may overflow */
					dprintk(DEBUG_FILTER_INFO, "ignore up event & down event. \n");
					mod_timer(&data_timer, jiffies + ts_data->ts_delay_period);
					prev_sample->sample_time = sample_data->sample_time;
					continue;
				}	                
			}            
		}
	        
		switch (sample_data->sample_status) {
		case TP_DOWN:
	      		
			if(1 == data_timer_status){
				report_up_event_implement(ts_data);
			}
			ts_data->touchflag = 0; 

			ts_data->ts_process_status = TP_DOWN;
			ts_data->double_point_cnt = 0;
			prev_sample->sample_time = sample_data->sample_time;
			reported_data_start_time = sample_data->sample_time;
			dprintk(DEBUG_REPORT_STATUS_INFO, "actuall TP_DOWN . \n");
			break;
			
		case TP_DATA_VA:

			dprintk(DEBUG_FILTER_INFO, "data: sample_data->sample_time = %u. \n", sample_data->sample_time);
			prev_data_sample->sample_time = sample_data->sample_time;
			prev_sample->sample_time = sample_data->sample_time;
			process_data(ts_data, sample_data);
			if (0 == data_timer_status) {
				mod_timer(&data_timer, jiffies + ts_data->ts_delay_period);
				data_timer_status = 1;
				prev_data_sample->x = sample_data->x;
				prev_data_sample->y = sample_data->y;
				                    
				dprintk(DEBUG_REPORT_STATUS_INFO, "timer is start up. \n");
			} else {
				mod_timer(&data_timer, jiffies + ts_data->ts_delay_period);
				dprintk(DEBUG_REPORT_STATUS_INFO, "more ts_data->ts_delay_period ms delay. jiffies + ts_data->ts_delay_period = %lu. \n",\
					jiffies + ts_data->ts_delay_period);
			}
			ts_data->ts_process_status = TP_DATA_VA;
			break;
	     		
		case TP_UP :

			/* actually, this case will never be run */
			if (1 == ts_data->touchflag || 2 == ts_data->touchflag) {
				dprintk(DEBUG_REPORT_STATUS_INFO, "actually TP_UP. \n");

				report_up_event((unsigned long)ts_data);             
			}
			break;
	      		
		default:	
			break;
	      	
		}
	}
		
	/* update buffer_tail */
	ts_data->buffer_tail = (unsigned long)tail;

	/* avoid overflow */
	if (ts_data->buffer_tail > (CYCLE_BUFFER_SIZE << 4)) {
		writel(0, ts_data->base_addr + TP_INT_FIFOC);          /* disable irq */
			        
		ts_data->buffer_tail -= (CYCLE_BUFFER_SIZE<<3);
		ts_data->buffer_head -= (CYCLE_BUFFER_SIZE<<3);        /* head may have been change by interrupt */       

		if(TRUE == tp_irq_state) {
			/* enable irq */
			writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN, ts_data->base_addr + TP_INT_FIFOC);
		}
	}
	   
	if (FALSE == tp_irq_state) {
		/* enable irq */
		writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN, ts_data->base_addr + TP_INT_FIFOC);
		tp_irq_state = TRUE;
	}
out:
	tp_do_tasklet_running = 0;   
}

static irqreturn_t sun6i_isr_tp(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct sun6i_ts_data *ts_data = (struct sun6i_ts_data *)platform_get_drvdata(pdev);

	unsigned int reg_val;
	unsigned int reg_fifoc;
	int head_index = (int)(ts_data->buffer_head&(CYCLE_BUFFER_SIZE-1));
	int tail = (int)ts_data->buffer_tail;
    
#ifdef TP_INT_PERIOD_TEST 
	static int count = 0; 
#endif

#ifdef TP_TEMP_DEBUG
	static unsigned int temp_cnt = 0;
	static unsigned int temp_data = 0;
#endif

	reg_val  = readl(TP_BASSADDRESS + TP_INT_FIFOS);
	if (!(reg_val&(TP_DOWN_PENDING | FIFO_DATA_PENDING | TP_UP_PENDING))) {
		//printk("non tp irq . \n");
#ifdef TP_TEMP_DEBUG
		if (reg_val&0x40000) {
			writel(reg_val&0x40000,TP_BASSADDRESS + TP_INT_FIFOS);
			reg_val = readl(TP_BASSADDRESS + TEMP_DATA);
			if(temp_cnt < (TOTAL_TIMES - 1)) {
				temp_data += reg_val;
				temp_cnt++;
			} else {
				temp_data += reg_val;
				temp_data /= TOTAL_TIMES;
				printk("temp = ");
				printk("%d\n",temp_data);
				temp_data = 0;
				temp_cnt  = 0;
			}
				
			return IRQ_HANDLED;
		}
#endif
	    
        return IRQ_NONE;
	}
	/* when head-tail == CYCLE_BUFFER_SIZE, mean the buffer is full. */
	if (((tail+CYCLE_BUFFER_SIZE)) <= (ts_data->buffer_head)) { 
		printk("warn: cycle buffer is full. \n");
		                                                   
		writel(0, ts_data->base_addr + TP_INT_FIFOC);       /* disable irq */
		tp_irq_state = FALSE;
		writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);      /* clear irq pending */
        
		tp_tasklet.data = ts_data->buffer_head;	
		printk("schedule tasklet. ts_data->buffer_head = %lu, \
			ts_data->buffer_tail = %lu.\n", ts_data->buffer_head, ts_data->buffer_tail);
	                
		tasklet_schedule(&tp_tasklet);
		return IRQ_HANDLED;
	}
	
	if (reg_val&TP_DOWN_PENDING) {
		writel(reg_val&TP_DOWN_PENDING,TP_BASSADDRESS + TP_INT_FIFOS);
		dprintk(DEBUG_INT_INFO, "press the screen: jiffies to ms == %u , jiffies == %lu, time: = %llu \n", \
		jiffies_to_msecs((long)get_jiffies_64()), jiffies, get_cpu_idle_time_us(0, &cur_wall_time));

		ts_data->ts_sample_status = TP_DOWN;
		ts_data->count  = 0;
		cycle_buffer[head_index].sample_status = TP_DOWN;
		cycle_buffer[head_index].sample_time= jiffies;
		/* update buffer_head */
		ts_data->buffer_head++;
#ifdef TP_INT_PERIOD_TEST
		count = 0;
#endif 

	} else if(reg_val&FIFO_DATA_PENDING) {       /* do not report data on up status */  
		ts_data->count++;
		if (ts_data->count > FILTER_NOISE_LOWER_LIMIT) {
		cycle_buffer[head_index].x      = readl(TP_BASSADDRESS + TP_DATA);
		cycle_buffer[head_index].y      = readl(TP_BASSADDRESS + TP_DATA);      
		cycle_buffer[head_index].dx     = readl(TP_BASSADDRESS + TP_DATA);
		cycle_buffer[head_index].dy     = readl(TP_BASSADDRESS + TP_DATA); 
		cycle_buffer[head_index].sample_time= jiffies;
       
		cycle_buffer[head_index].sample_status = TP_DATA_VA;
		ts_data->ts_sample_status = TP_DATA_VA;
		/* flush fifo */
		reg_fifoc = readl(ts_data->base_addr+TP_INT_FIFOC);
		reg_fifoc |= TP_FIFO_FLUSH;
		writel(reg_fifoc, ts_data->base_addr+TP_INT_FIFOC); 
  
		dprintk(DEBUG_INT_INFO,"data coming, jiffies to ms == %u , jiffies == %lu, time: = %llu \n", \
		jiffies_to_msecs((long)get_jiffies_64()), jiffies, get_cpu_idle_time_us(0, &cur_wall_time));

		/* update buffer_head */
		ts_data->buffer_head++;
#ifdef TP_INT_PERIOD_TEST
		count++;
		printk("jiffies = %d. count = %d. \n", jiffies, count);
#endif

		} else {
			/* flush fifo, the data you do not want to reserved, need to be flush out fifo */
			reg_fifoc = readl(ts_data->base_addr+TP_INT_FIFOC);
			reg_fifoc |= TP_FIFO_FLUSH;
			writel(reg_fifoc, ts_data->base_addr+TP_INT_FIFOC); 
		}
		udelay(1); 
		writel(reg_val&FIFO_DATA_PENDING,TP_BASSADDRESS + TP_INT_FIFOS);       
	} else if (reg_val&TP_UP_PENDING) {
		writel(reg_val&TP_UP_PENDING,TP_BASSADDRESS + TP_INT_FIFOS);
		dprintk(DEBUG_INT_INFO,"up the screen. jiffies to ms == %u ,  jiffies == %lu,  time: = %llu \n", \
		jiffies_to_msecs((long)get_jiffies_64()), jiffies, get_cpu_idle_time_us(0, &cur_wall_time));
		            
		cycle_buffer[head_index].sample_status = TP_UP;
		ts_data->count  = 0;
		cycle_buffer[head_index].sample_time= jiffies;
		/* update buffer_head */
		ts_data->buffer_head++;
	}

	tp_tasklet.data = ts_data->buffer_head;	
	tasklet_schedule(&tp_tasklet);
	return IRQ_HANDLED;
}

static int sun6i_ts_open(struct input_dev *dev)
{
	/* enable clock */
	return 0;
}

static void sun6i_ts_close(struct input_dev *dev)
{
	/* disable clock */
}

static struct sun6i_ts_data *sun6i_ts_data_alloc(struct platform_device *pdev)
{
	 
	struct sun6i_ts_data *ts_data = kzalloc(sizeof(*ts_data), GFP_KERNEL);

	if (!ts_data)
		return NULL;

	ts_data->input = input_allocate_device();
	if (!ts_data->input) {
		kfree(ts_data);
		return NULL;
	}

	
	ts_data->input->evbit[0] =  BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	set_bit(BTN_TOUCH, ts_data->input->keybit);
	
	input_set_abs_params(ts_data->input, ABS_MT_TOUCH_MAJOR, 0, 1000, 0, 0);
	input_set_abs_params(ts_data->input, ABS_MT_POSITION_X, 0, 4095, 0, 0);
	input_set_abs_params(ts_data->input, ABS_MT_POSITION_Y, 0, 4095, 0, 0);


	ts_data->input->name = pdev->name;
	ts_data->input->phys = "sun6i_ts/input0";
	ts_data->input->id.bustype = BUS_HOST ;
	ts_data->input->id.vendor = 0x0001;
	ts_data->input->id.product = 0x0001;
	ts_data->input->id.version = 0x0100;

	ts_data->input->open = sun6i_ts_open;
	ts_data->input->close = sun6i_ts_close;
	ts_data->input->dev.parent = &pdev->dev; 
	ts_data->ts_sample_status = TP_INITIAL;
	ts_data->ts_process_status = TP_INITIAL;
	ts_data->double_point_cnt = 0;
	ts_data->single_touch_cnt = 0;
	ts_data->ts_delay_period = DELAY_PERIOD  + DELAY_COMPENSTAE_PEROID;
	
	ts_data->buffer_head = 0;
	ts_data->buffer_tail = 0;

	return ts_data;
}




static void sun6i_ts_data_free(struct sun6i_ts_data *ts_data)
{
	if (!ts_data)
		return;
	if (ts_data->input)
		input_free_device(ts_data->input);
	kfree(ts_data);
}


static int __devinit sun6i_ts_probe(struct platform_device *pdev)
{
	int err =0;
	int irq = platform_get_irq(pdev, 0);
	struct sun6i_ts_data *ts_data;	
	tp_flag = 0;

	dprintk(DEBUG_INIT, "sun6i-ts.c: sun6i_ts_probe: start...\n");

	ts_data = sun6i_ts_data_alloc(pdev);
	if (!ts_data) {
		dev_err(&pdev->dev, "Cannot allocate driver structures\n");
		err = -ENOMEM;
		goto err_out;
	}

	mtTsData = ts_data; 

	spin_lock_init(&tp_do_tasklet_sync);
	
	dprintk(DEBUG_INIT, "begin get platform resourec\n");
    
	ts_data->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!ts_data->res) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "Can't get the MEMORY\n");
		goto err_out1;
	}

	ts_data->base_addr = (void __iomem *)TP_BASSADDRESS;

	ts_data->irq = irq;

	platform_set_drvdata(pdev, ts_data);	

	/* All went ok, so register to the input system */
	err = input_register_device(ts_data->input);
	if (err)
		goto err_out2;

#ifdef CONFIG_HAS_EARLYSUSPEND	
	printk("==register_early_suspend =\n");	
	ts_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;	
	ts_data->early_suspend.suspend = sun6i_ts_early_suspend;
	ts_data->early_suspend.resume	= sun6i_ts_late_resume;	
	register_early_suspend(&ts_data->early_suspend);
#endif

	init_timer(&data_timer);
	data_timer.expires = jiffies + ts_data->ts_delay_period;
	data_timer.data = (unsigned long)ts_data;
	data_timer.function = report_up_event;

	prev_sample = kzalloc(sizeof(*prev_sample), GFP_KERNEL);
	prev_data_sample = kzalloc(sizeof(*prev_data_sample), GFP_KERNEL);
        
        dprintk(DEBUG_INIT, "tp init\n");

	tp_init();

	
	err = request_irq(irq, sun6i_isr_tp,
		IRQF_DISABLED, pdev->name, pdev);
	if (err) {
		dev_err(&pdev->dev, "Cannot request ts IRQ\n");
		goto err_out3;
	}

	dprintk(DEBUG_INIT, "sun6i-ts.c: sun6i_ts_probe: end\n");

    return 0;

err_out3:
	
err_out2:
	if (ts_data->irq)
		free_irq(ts_data->irq, pdev);
err_out1:
	sun6i_ts_data_free(ts_data);
err_out: 	
     
	dprintk(DEBUG_INIT, "sun6i-ts.c: sun6i_ts_probe: failed!\n");
	
	return err;
}

static int __devexit sun6i_ts_remove(struct platform_device *pdev)
{
	
	struct sun6i_ts_data *ts_data = platform_get_drvdata(pdev);	
	free_irq(ts_data->irq, pdev);
	/* cancel tasklet? */
	tasklet_disable(&tp_tasklet);
	
#ifdef CONFIG_SMP
	del_timer_sync(&data_timer);
#else
	del_timer(&data_timer);
#endif
	data_timer_status = 0;

	platform_set_drvdata(pdev, NULL);
	
#ifdef CONFIG_HAS_EARLYSUSPEND	
	unregister_early_suspend(&ts_data->early_suspend);	
#endif
	input_unregister_device(ts_data->input);
	sun6i_ts_data_free(ts_data);

	return 0;	
}
	

static struct platform_driver sun6i_ts_driver = {
	.probe		= sun6i_ts_probe,
	.remove		= __devexit_p(sun6i_ts_remove),
#ifdef CONFIG_HAS_EARLYSUSPEND

#else
#ifdef CONFIG_PM
	.suspend	= sun6i_ts_suspend,
	.resume		= sun6i_ts_resume,
#endif
#endif
	.driver		= {
		.name	= "sun6i-ts",
	},
};


static void sun6i_ts_nop_release(struct device *dev)
{
	/* Nothing */
}

static struct resource sun6i_ts_resource[] = {
	{
	.flags  = IORESOURCE_IRQ,
	.start  = IRQ_TP ,
	.end    = IRQ_TP ,
	},

	{
	.flags	= IORESOURCE_MEM,
	.start	= TP_BASSADDRESS,
	.end	= TP_BASSADDRESS + 0x100-1,
	},
};

struct platform_device sun6i_ts_device = {
	.name		= "sun6i-ts",
	.id		    = -1,
	.dev = {
		.release = sun6i_ts_nop_release,
		},
	.resource	= sun6i_ts_resource,
	.num_resources	= ARRAY_SIZE(sun6i_ts_resource),
};


static int __init sun6i_ts_init(void)
{
	int device_used = 0;
	int ret = -1;
	/* get the config para */
	int tp_screen_size = 0;
	script_item_u	val;
	script_item_value_type_e  type;
     
	dprintk(DEBUG_INIT, "sun6i-ts.c: sun6i_ts_init: start ...\n");

	type = script_get_item("rtp_para", "rtp_used", &val);

 	if (SCIRPT_ITEM_VALUE_TYPE_INT  != type) {
		pr_err("%s: type err  rtp_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	device_used = val.val;
	
	if (1 == device_used) {
		type = script_get_item("rtp_para", "rtp_screen_size", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT  != type){
	        pr_err("sun6i_ts_init: script_get err. \n");
	        goto script_get_err;
		}
		tp_screen_size = val.val;
		printk("sun6i-ts: tp_screen_size is %d inch.\n", tp_screen_size);
		if (7 == tp_screen_size) {
			dual_touch_distance = 20;
			glide_delta_ds_max_limit = 90;
			tp_regidity_level = 7;
		}else if(5 == tp_screen_size) {
			dual_touch_distance = 35;
			glide_delta_ds_max_limit = 150;
			tp_regidity_level = 5;
		} else {
			pr_err("sun6i-ts: tp_screen_size is not supported. \n");
			goto script_get_err;
		}

		type = script_get_item("rtp_para", "rtp_regidity_level", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT  != type) {
			pr_err("sun6i_ts_init: script_get err rtp_regidity_level. \n");
			goto script_get_err;
		}
		tp_regidity_level = val.val;
		printk("sun6i-ts: tp_regidity_level is %d.\n", tp_regidity_level);

		if (tp_regidity_level < 2 || tp_regidity_level > 10) {
			printk("sun6i-ts: only tp_regidity_level between 2 and 10  is supported. \n");
			goto script_get_err;
		}

		type = script_get_item("rtp_para", "rtp_press_threshold_enable", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT  != type) {
			pr_err("sun6i_ts_init: script_get err rtp_press_threshold_enable. \n");
			goto script_get_err;
		}
		tp_press_threshold_enable = val.val;
		printk("sun6i-ts: tp_press_threshold_enable is %d.\n", tp_press_threshold_enable);

		if(0 != tp_press_threshold_enable  && 1 != tp_press_threshold_enable) {
			printk("sun6i-ts: only tp_press_threshold_enable  0 or 1  is supported. \n");
			goto script_get_err;
		}

		if (1 == tp_press_threshold_enable) {
			type = script_get_item("rtp_para", "rtp_press_threshold", &val);
			if (SCIRPT_ITEM_VALUE_TYPE_INT  != type) {
				pr_err("sun6i_ts_init: script_get err rtp_press_threshold. \n");
				goto script_get_err;
			}
			tp_press_threshold = val.val;
			printk("sun6i-ts: rtp_press_threshold is %d.\n", tp_press_threshold);

			if(tp_press_threshold < 0 || tp_press_threshold > 0xFFFFFF) {
				printk("sun6i-ts: only tp_regidity_level between 0 and 0xFFFFFF  is supported. \n");
				goto script_get_err;
			}
		}
		
		type = script_get_item("rtp_para", "rtp_sensitive_level", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT  != type) {
			pr_err("sun6i_ts_init: script_get err rtp_sensitive_level. \n");
			goto script_get_err;
		}
		tp_sensitive_level = val.val;
		printk("sun6i-ts: rtp_sensitive_level is %d.\n", tp_sensitive_level);

		if (tp_sensitive_level < 0 || tp_sensitive_level > 0xf) {
			printk("sun6i-ts: only tp_regidity_level between 0 and 0xf  is supported. \n");
			goto script_get_err;
		}

		type = script_get_item("rtp_para", "rtp_exchange_x_y_flag", &val);	    
		if (SCIRPT_ITEM_VALUE_TYPE_INT  != type) {
			pr_err("sun6i_ts_init: script_get err rtp_exchange_x_y_flag. \n");
			goto script_get_err;
		}
		tp_exchange_x_y = val.val;
		printk("sun6i-ts: rtp_exchange_x_y_flag is %d.\n", tp_exchange_x_y);

		if (0 != tp_exchange_x_y && 1 != tp_exchange_x_y) {
			printk("sun6i-ts: only tp_exchange_x_y==1 or  tp_exchange_x_y==0 is supported. \n");
			goto script_get_err;
		}
	            
	} else {
		goto script_get_err;
	}
		
	platform_device_register(&sun6i_ts_device);
	ret = platform_driver_register(&sun6i_ts_driver);

script_get_err:
	return ret;
}

static void __exit sun6i_ts_exit(void)
{
	platform_driver_unregister(&sun6i_ts_driver);
	platform_device_unregister(&sun6i_ts_device);
}

module_init(sun6i_ts_init);
module_exit(sun6i_ts_exit);

MODULE_AUTHOR("zhengdixu <@>");
MODULE_DESCRIPTION("sun6i touchscreen driver");
MODULE_LICENSE("GPL");

