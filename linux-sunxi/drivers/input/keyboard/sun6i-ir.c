/****************************************************************
*
*VERSION 1.0 Inital Version
*note: when swith to real ic, need to define SYS_CLK_CFG_EN & undef FPGA_SIM_CONFIG, vice versa.
*****************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/timer.h> 
#include <mach/clock.h>
#include <linux/gpio.h>
#include <linux/scenelock.h>
#include <linux/power/aw_pm.h>
#include <mach/sys_config.h>

#include <mach/irqs.h>
#include <mach/hardware.h>
#include <linux/clk.h>
#include <mach/gpio.h>
#include <mach/ar100.h>
#undef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif

#include "ir-keymap.h"

#ifdef CONFIG_AW_FPGA_V4_PLATFORM
#define FPGA_SIM_CONFIG                    /* input clk is 24M */
#define SW_INT_IRQNO_IR0 (16+32)
#else
#define SYS_GPIO_CFG_EN
#define SYS_CLK_CFG_EN
#define SW_INT_IRQNO_IR0 (69)
#endif

#ifdef SYS_CLK_CFG_EN
static struct clk *ir_clk;
static struct clk *ir_clk_source;
#endif

#ifdef SYS_GPIO_CFG_EN
static struct gpio_hdle {
	script_item_u	val;
	script_item_value_type_e  type;		
}ir_gpio_hdle;
#endif

/* Registers */
#define IR_REG(x)        (x)
#define IR0_BASE         (0xf1f02000)
#define IR_BASE          IR0_BASE
#define IR_IRQNO         (SW_INT_IRQNO_IR0)

/* CCM register */
#define CCM_BASE         0xf1c20000
/* PIO register */
#define PI_BASE          0xf1c20800
                                    
#define IR_CTRL_REG      IR_REG(0x00)     /* IR Control */
#define IR_RXCFG_REG     IR_REG(0x10)     /* Rx Config */
#define IR_RXDAT_REG     IR_REG(0x20)     /* Rx Data */
#define IR_RXINTE_REG    IR_REG(0x2C)     /* Rx Interrupt Enable */
#define IR_RXINTS_REG    IR_REG(0x30)     /* Rx Interrupt Status */
#define IR_SPLCFG_REG    IR_REG(0x34)     /* IR Sample Config */

//Bit Definition of IR_RXINTS_REG Register
#define IR_RXINTS_RXOF   (0x1<<0)         /* Rx FIFO Overflow */
#define IR_RXINTS_RXPE   (0x1<<1)         /* Rx Packet End */
#define IR_RXINTS_RXDA   (0x1<<4)         /* Rx FIFO Data Available */

#define IR_FIFO_SIZE     (64)             /* 64Bytes */
/* Frequency of Sample Clock = 23437.5Hz, Cycle is 42.7us */
/* Pulse of NEC Remote >560us */
#ifdef FPGA_SIM_CONFIG
#define IR_RXFILT_VAL    (16)             /* Filter Threshold = 8*42.7 = ~341us < 500us */
#define IR_RXIDLE_VAL    (5)              /* Idle Threshold = (2+1)*128*42.7 = ~16.4ms > 9ms */
#define IR_ACTIVE_T      (99)             /* Active Threshold */
#define IR_ACTIVE_T_C    (0)              /* Active Threshold */

#define IR_L1_MIN        (160)            /* 80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN        (80)             /* 40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */ 
#define IR_PMAX          (52)             /* 26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID          (52)             /* 26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX          (106)            /* 53*42.7 = ~2263us ~= 561*4, D < IR_DMAX */

#else
#define IR_RXFILT_VAL    (8)              /* Filter Threshold = 8*42.7 = ~341us	< 500us */	
#define IR_RXIDLE_VAL    (2)              /* Idle Threshold = (2+1)*128*42.7 = ~16.4ms > 9ms */
#define IR_ACTIVE_T      (99)             /* Active Threshold */
#define IR_ACTIVE_T_C    (0)              /* Active Threshold */

#define IR_L1_MIN        (80)             /* 80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN        (40)             /* 40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */
#define IR_PMAX          (26)             /* 26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID          (26)             /* 26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX          (53)             /* 53*42.7 = ~2263us ~= 561*4, D < IR_DMAX */
#endif

#define IR_ERROR_CODE    (0xffffffff)
#define IR_REPEAT_CODE   (0x00000000)
#define DRV_VERSION      "1.00"

#define REPORT_REPEAT_KEY_VALUE

#ifdef CONFIG_HAS_EARLYSUSPEND	
struct sun6i_ir_data {
	struct early_suspend early_suspend;
};
#endif

struct ir_raw_buffer {
	unsigned long dcnt;                  		/*Packet Count*/
	#define	IR_RAW_BUF_SIZE		128
	unsigned char buf[IR_RAW_BUF_SIZE];	
};

static unsigned int ir_cnt = 0;
static struct input_dev *ir_dev;
static struct timer_list *s_timer; 
static unsigned long ir_code=0;
static int timer_used=0;
static struct ir_raw_buffer	ir_rawbuf;
static u32 power_key = 0;
extern unsigned int normal_standby_wakesource;
static u32 ir_addr_code = 0x9f00;


#ifdef CONFIG_HAS_EARLYSUSPEND
static struct sun6i_ir_data *ir_data;
#else
#ifdef CONFIG_PM
struct dev_pm_domain ir_pm_domain;
#endif
#endif

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);



static inline void ir_reset_rawbuffer(void)
{
	ir_rawbuf.dcnt = 0;
}

static inline void ir_write_rawbuffer(unsigned char data)
{
	if (ir_rawbuf.dcnt < IR_RAW_BUF_SIZE) 
		ir_rawbuf.buf[ir_rawbuf.dcnt++] = data;
	else
		printk(KERN_DEBUG "ir_write_rawbuffer: IR Rx Buffer Full!!\n");
}

static inline unsigned char ir_read_rawbuffer(void)
{
	unsigned char data = 0x00;
	
	if (ir_rawbuf.dcnt > 0)
		data = ir_rawbuf.buf[--ir_rawbuf.dcnt];
		
	return data;
}

static inline int ir_rawbuffer_empty(void)
{
	return (ir_rawbuf.dcnt == 0);
}

static inline int ir_rawbuffer_full(void)
{
	return (ir_rawbuf.dcnt >= IR_RAW_BUF_SIZE);	
}

static void ir_clk_cfg(void)
{
#ifdef SYS_CLK_CFG_EN
	unsigned long rate = 3000000; /* 3Mhz */    
#else
	unsigned long tmp = 0;
#endif
	
#ifdef SYS_CLK_CFG_EN
	ir_clk_source = clk_get(NULL, CLK_SYS_HOSC);		
	if (!ir_clk_source || IS_ERR(ir_clk_source)) {
		printk(KERN_DEBUG "try to get ir_clk_source clock failed!\n");
		return;
	}

	rate = clk_get_rate(ir_clk_source);
	dprintk(DEBUG_INIT, "%s: get ir_clk_source rate %dHZ\n", __func__, (__u32)rate);

	ir_clk = clk_get(NULL, CLK_MOD_R_CIR);		
	if (!ir_clk || IS_ERR(ir_clk)) {
		printk(KERN_DEBUG "try to get ir clock failed!\n");
		return;
	}

	if(clk_set_parent(ir_clk, ir_clk_source))
		printk("%s: set ir_clk parent to ir_clk_source failed!\n", __func__);

	if (clk_set_rate(ir_clk, 3000000)) {
		printk(KERN_DEBUG "set ir clock freq to 3M failed!\n");
	}

	
	if (clk_enable(ir_clk)) {
			printk(KERN_DEBUG "try to enable ir_clk failed!\n");	
	}
	
	if (clk_reset(ir_clk, AW_CCU_CLK_NRESET)) {
			printk(KERN_DEBUG "try to nreset ir_clk failed!\n");	
	}
#else	
	/* Enable APB Clock for IR */	
	tmp = readl(CCM_BASE + 0x10);
	tmp |= 0x1<<10;  //IR	
	writel(tmp, CCM_BASE + 0x10);	

	/* config Special Clock for IR	(24/8=3MHz) */
	tmp = readl(CCM_BASE + 0x34);
	tmp &= ~(0x3<<8);  	
	tmp |= (0x1<<8);   	/* Select 24MHz */
	tmp |= (0x1<<7);   	/* Open Clock */
	tmp &= ~(0x3f<<0);  
	tmp |= (7<<0);	 	/* Divisor = 8 */
	writel(tmp, CCM_BASE + 0x34);
#endif

	return;
}

static void ir_clk_uncfg(void)
{
#ifdef SYS_CLK_CFG_EN
	if(NULL == ir_clk || IS_ERR(ir_clk)) {
		printk("ir_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable(ir_clk);
		clk_put(ir_clk);
		ir_clk = NULL;
	}

	if(NULL == ir_clk_source || IS_ERR(ir_clk_source)) {
		printk("ir_clk_source handle is invalid, just return!\n");
		return;
	} else {	
		clk_put(ir_clk_source);
		ir_clk_source = NULL;
	}
#else	
#endif
 
	return;
}
static void ir_sys_cfg(void)
{
#ifdef SYS_GPIO_CFG_EN
	ir_gpio_hdle.type = script_get_item("ir_para", "ir_rx", &(ir_gpio_hdle.val));
	
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != ir_gpio_hdle.type)
		printk(KERN_ERR "IR gpio type err! \n");
	
	dprintk(DEBUG_INIT, "value is: gpio %d, mul_sel %d, pull %d, drv_level %d, data %d\n", 
		ir_gpio_hdle.val.gpio.gpio, ir_gpio_hdle.val.gpio.mul_sel, ir_gpio_hdle.val.gpio.pull,  
		ir_gpio_hdle.val.gpio.drv_level, ir_gpio_hdle.val.gpio.data);
	 
	if(0 != gpio_request(ir_gpio_hdle.val.gpio.gpio, NULL)) {	
		printk(KERN_ERR "ERROR: IR Gpio_request is failed\n");
	}

	
	if (0 != sw_gpio_setall_range(&ir_gpio_hdle.val.gpio, 1)) {
		printk(KERN_ERR "IR gpio set err!");
		goto end;
	}
#else
	unsigned long tmp;
	/* config IO: PIOB4 to IR_Rx */
	tmp = readl(PI_BASE + 0x24); /* PIOB_CFG0_REG */
	tmp &= ~(0xf<<16);
	tmp |= (0x2<<16);
	writel(tmp, PI_BASE + 0x24);
#endif

	ir_clk_cfg();

	return;	
#ifdef SYS_GPIO_CFG_EN
end:
	gpio_free(ir_gpio_hdle.val.gpio.gpio);
	return;
#endif
	
}

static void ir_sys_uncfg(void)
{
#ifdef SYS_GPIO_CFG_EN
	gpio_free(ir_gpio_hdle.val.gpio.gpio);
#else
#endif

	ir_clk_uncfg();

	return;	
}

static void ir_reg_cfg(void)
{
	unsigned long tmp = 0;
	/* Enable IR Mode */
	tmp = 0x3<<4;
	writel(tmp, IR_BASE+IR_CTRL_REG);
	
	/* Config IR Smaple Register */
#ifdef FPGA_SIM_CONFIG
	tmp = 0x3<<0;  /* Fsample = 24MHz/512 = 46875Hz (21.33us) */
#else
	tmp = 0x1<<0;  /* Fsample = 3MHz/128 =23437.5Hz (42.7us) */
#endif
 
        
	tmp |= (IR_RXFILT_VAL&0x3f)<<2;		/* Set Filter Threshold */
	tmp |= (IR_RXIDLE_VAL&0xff)<<8; 	/* Set Idle Threshold */
	tmp |= (IR_ACTIVE_T&0xff)<<16;          /* Set Active Threshold */
	tmp |= (IR_ACTIVE_T_C&0xff)<<23;
	writel(tmp, IR_BASE+IR_SPLCFG_REG);
	
	/* Invert Input Signal */
	writel(0x1<<2, IR_BASE+IR_RXCFG_REG);
	
	/* Clear All Rx Interrupt Status */
	writel(0xff, IR_BASE+IR_RXINTS_REG);
	
	/* Set Rx Interrupt Enable */
	tmp = (0x1<<4)|0x3;
	tmp |= ((IR_FIFO_SIZE>>1)-1)<<8; 	/* Rx FIFO Threshold = FIFOsz/2; */
	//tmp |= ((IR_FIFO_SIZE>>2)-1)<<8;	/* Rx FIFO Threshold = FIFOsz/4; */
	writel(tmp, IR_BASE+IR_RXINTE_REG);
	
	/* Enable IR Module */
	tmp = readl(IR_BASE+IR_CTRL_REG);
	tmp |= 0x3;
	writel(tmp, IR_BASE+IR_CTRL_REG);

	return;
}

static void ir_setup(void)
{	
	dprintk(DEBUG_INIT, "ir_setup: ir setup start!!\n");

	ir_code = 0;
	timer_used = 0;
	ir_reset_rawbuffer();	
	ir_sys_cfg();	
	ir_reg_cfg();
	
	dprintk(DEBUG_INIT, "ir_setup: ir setup end!!\n");

	return;
}

static inline unsigned char ir_get_data(void)
{
	return (unsigned char)(readl(IR_BASE + IR_RXDAT_REG));
}

static inline unsigned long ir_get_intsta(void)
{
	return (readl(IR_BASE + IR_RXINTS_REG));
}

static inline void ir_clr_intsta(unsigned long bitmap)
{
	unsigned long tmp = readl(IR_BASE + IR_RXINTS_REG);

	tmp &= ~0xff;
	tmp |= bitmap&0xff;
	writel(tmp, IR_BASE + IR_RXINTS_REG);
}

static unsigned long ir_packet_handler(unsigned char *buf, unsigned long dcnt)
{
	unsigned long len;
	unsigned char val = 0x00;
	unsigned char last = 0x00;
	unsigned long code = 0;
	int bitCnt = 0;
	unsigned long i=0;
	unsigned int active_delay = 0;
	
	//print_hex_dump_bytes("--- ", DUMP_PREFIX_NONE, buf, dcnt);

	dprintk(DEBUG_DATA_INFO, "dcnt = %d \n", (int)dcnt);
	
	/* Find Lead '1' */
	active_delay = (IR_ACTIVE_T+1)*(IR_ACTIVE_T_C ? 128:1);
	dprintk(DEBUG_DATA_INFO, "%d active_delay = %d\n", __LINE__, active_delay);
	len = 0;
	if (128 <= active_delay)
		len += (active_delay>>1);
	for (i=0; i<dcnt; i++) {
		val = buf[i];
		if (val & 0x80) {
			len += val & 0x7f;
		} else {
			if (len > IR_L1_MIN)
				break;
			
			len = 0;
		}
	}

	dprintk(DEBUG_DATA_INFO, "%d len = %ld\n", __LINE__, len);

	if ((val&0x80) || (len<=IR_L1_MIN))
		return IR_ERROR_CODE; /* Invalid Code */

	/* Find Lead '0' */
	len = 0;
	for (; i<dcnt; i++) {
		val = buf[i];		
		if (val & 0x80) {
			if(len > IR_L0_MIN)
				break;
			
			len = 0;
		} else {
			len += val & 0x7f;
		}		
	}
	
	if ((!(val&0x80)) || (len<=IR_L0_MIN))
		return IR_ERROR_CODE; /* Invalid Code */
	
	/* go decoding */
	code = 0;  /* 0 for Repeat Code */
	bitCnt = 0;
	last = 1;
	len = 0;
	for (; i<dcnt; i++) {
		val = buf[i];		
		if (last) {
			if (val & 0x80) {
				len += val & 0x7f;
			} else {
				if (len > IR_PMAX) {		/* Error Pulse */
					return IR_ERROR_CODE;
				}
				last = 0;
				len = val & 0x7f;
			}
		} else {
			if (val & 0x80) {
				if (len > IR_DMAX){		/* Error Distant */
					return IR_ERROR_CODE;
				} else {
					if (len > IR_DMID)  {
						/* data '1'*/
						code |= 1<<bitCnt;
					}
					bitCnt ++;
					if (bitCnt == 32)
						break;  /* decode over */
				}	
				last = 1;
				len = val & 0x7f;
			} else {
				len += val & 0x7f;
			}
		}
	}
	
	return code;
}

static int ir_code_valid(unsigned long code)
{
	unsigned long tmp1, tmp2;

#ifdef IR_CHECK_ADDR_CODE
	/* Check Address Value */
	if ((code&0xffff) != (ir_addr_code&0xffff))
		return 0; /* Address Error */
	
	tmp1 = code & 0x00ff0000;
	tmp2 = (code & 0xff000000)>>8;
	
	return ((tmp1^tmp2)==0x00ff0000);  /* Check User Code */
#else	
	/* Do Not Check Address Value */
	tmp1 = code & 0x00ff00ff;
	tmp2 = (code & 0xff00ff00)>>8;
	
	//return ((tmp1^tmp2)==0x00ff00ff);
	return (((tmp1^tmp2) & 0x00ff0000)==0x00ff0000 );
#endif /* #ifdef IR_CHECK_ADDR_CODE */
}

static irqreturn_t ir_irq_service(int irqno, void *dev_id)
{
	unsigned long intsta = ir_get_intsta();
	
	dprintk(DEBUG_INT, "IR IRQ Serve\n");

	ir_clr_intsta(intsta);
	
	//if(intsta & (IR_RXINTS_RXDA|IR_RXINTS_RXPE))  /* FIFO Data Valid */
	/*Read Data Every Time Enter this Routine*/
	{
		//unsigned long dcnt =  (ir_get_intsta()>>8) & 0x1f;
		unsigned long dcnt =  (ir_get_intsta()>>8) & 0x7f;
		unsigned long i = 0;
		
		/* Read FIFO */
		for (i=0; i<dcnt; i++) {
			if (ir_rawbuffer_full()) {

				ir_get_data();
				
			} else {
				ir_write_rawbuffer(ir_get_data());
			}			
		}		
	}
	
	if (intsta & IR_RXINTS_RXPE) {	 /* Packet End */
		unsigned long code;
		int code_valid;

		if (ir_rawbuffer_full()) {
			dprintk(DEBUG_INT, "ir_irq_service: Raw Buffer Full!!\n");
			ir_rawbuf.dcnt = 0;
			return IRQ_HANDLED;
		}
		
		code = ir_packet_handler(ir_rawbuf.buf, ir_rawbuf.dcnt);
		ir_rawbuf.dcnt = 0;
		code_valid = ir_code_valid(code);
					
		if (timer_used) {
			if (code_valid) {  /* the pre-key is released */ 			
				input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 0);
				input_sync(ir_dev);	
				
				dprintk(DEBUG_INT, "IR KEY UP\n");
				
				ir_cnt = 0;
			}
			if ((code==IR_REPEAT_CODE)||(code_valid)) {	/* Error, may interfere from other sources */
				mod_timer(s_timer, jiffies + (HZ/5));
			}
		} else {
			if (code_valid) {
				if (!timer_pending(s_timer)) {
					s_timer->expires = jiffies + (HZ/5);	/* 200ms timeout */
					add_timer(s_timer);

				} else
					mod_timer(s_timer, jiffies + (HZ/5));
				timer_used = 1;	
			}
		}
		
		if (timer_used) {
			ir_cnt++;
			if (ir_cnt == 1) {
				if (code_valid)	
					ir_code = code;  /* update saved code with a new valid code */
				
				dprintk(DEBUG_INT, "IR RAW CODE : %lu\n",(ir_code>>16)&0xff);
				
				input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 1);
			
				dprintk(DEBUG_INT, "IR CODE : %d\n",ir_keycodes[(ir_code>>16)&0xff]);
				
				input_sync(ir_dev);			
				
				dprintk(DEBUG_INT, "IR KEY VALE %d\n",ir_keycodes[(ir_code>>16)&0xff]);
				
			}

			
		}
		
		dprintk(DEBUG_INT, "ir_irq_service: Rx Packet End, code=0x%x, ir_code=0x%x, timer_used=%d \n", (int)code, (int)ir_code, timer_used);
	}	
	
	if (intsta & IR_RXINTS_RXOF) {  /* FIFO Overflow */
		/* flush raw buffer */
		ir_reset_rawbuffer();		

		dprintk(DEBUG_INT, "ir_irq_service: Rx FIFO Overflow!!\n");

	}	
	
	return IRQ_HANDLED;
}

static void ir_timer_handle(unsigned long arg)
{
	del_timer(s_timer);
	timer_used = 0;
	/* Time Out, means that the key is up */
	input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 0);
	input_sync(ir_dev);	

	dprintk(DEBUG_INT, "IR KEY TIMER OUT UP\n");

	ir_cnt = 0;
	
	dprintk(DEBUG_INT, "ir_timer_handle: timeout \n");	
}

static void ir_get_powerkey()
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item("ir_para", "ir_power_key_code", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("#########  ir power key code config type err!  ######");
		return;
	}
	power_key = script_val.val;
	//get ir addr code
	type = script_get_item("ir_para", "ir_addr_code", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("#########  ir ir_addr_code config type err!  ######");
		return;
	}
	ir_addr_code = script_val.val;
	printk("ir_addr_code = 0x%x     power_key=0x%x\n ",ir_addr_code,power_key);
}

/* 停用设备 */
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sun6i_ir_early_suspend(struct early_suspend *h)
{
	//unsigned long tmp = 0;
	//int ret;
	//struct sun6i_ir_data *ts = container_of(h, struct sun6i_ir_data, early_suspend);

	dprintk(DEBUG_SUSPEND, "enter earlysuspend: sun6i_ir_suspend. \n");

	//tmp = readl(IR_BASE+IR_CTRL_REG);
	//tmp &= 0xfffffffc;
	//writel(tmp, IR_BASE+IR_CTRL_REG);
#ifdef SYS_CLK_CFG_EN
	if(NULL == ir_clk || IS_ERR(ir_clk)) {
		printk("ir_clk handle is invalid, just return!\n");
		return;
	} else {	
		clk_disable(ir_clk);
	}
#endif
	return ;
}

/* 重新唤醒 */
static void sun6i_ir_late_resume(struct early_suspend *h)
{
	//unsigned long tmp = 0;
	//int ret;
	//struct sun6i_ir_data *ts = container_of(h, struct sun6i_ir_data, early_suspend);

	dprintk(DEBUG_SUSPEND, "enter laterresume: sun6i_ir_resume. \n");

	ir_code = 0;
	timer_used = 0;	
	ir_reset_rawbuffer();	
	ir_clk_cfg();	
	ir_reg_cfg();

	return ; 
}
#else
#ifdef CONFIG_PM
static int sun6i_ir_suspend(struct device *dev)
{

	dprintk(DEBUG_SUSPEND, "enter earlysuspend: sun6i_ir_suspend. \n");

	//tmp = readl(IR_BASE+IR_CTRL_REG);
	//tmp &= 0xfffffffc;
	//writel(tmp, IR_BASE+IR_CTRL_REG);
#ifdef SYS_CLK_CFG_EN
	if(NULL == ir_clk || IS_ERR(ir_clk)) {
		printk("ir_clk handle is invalid, just return!\n");
		return -1;
	} else {	
		clk_disable(ir_clk);
	}
#endif 
	return 0;
}

/* 重新唤醒 */
static int sun6i_ir_resume(struct device *dev)
{
    unsigned long ir_event = 0;
	dprintk(DEBUG_SUSPEND, "enter laterresume: sun6i_ir_resume. \n");

	ir_code = 0;
	timer_used = 0;	
	ir_reset_rawbuffer();	
	ir_clk_cfg();	
	ir_reg_cfg();
    
    ar100_query_wakeup_source(&ir_event);
    dprintk(DEBUG_SUSPEND, "%s: event 0x%lx\n", __func__, ir_event);
    if (CPUS_WAKEUP_IR&ir_event || normal_standby_wakesource&CPUS_WAKEUP_IR) {
        input_report_key(ir_dev, power_key, 1);
        input_sync(ir_dev);
        msleep(1);
        input_report_key(ir_dev, power_key, 0);
        input_sync(ir_dev);
    }
	return 0; 
}
#endif
#endif

static int __init ir_init(void)
{
	int i,ret;
	int err =0;
	ir_get_powerkey();
	ir_dev = input_allocate_device();
	if (!ir_dev) {
		printk(KERN_ERR "ir_dev: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	ir_dev->name = "sun6i-ir";  
	ir_dev->phys = "RemoteIR/input1"; 
	ir_dev->id.bustype = BUS_HOST;      
	ir_dev->id.vendor = 0x0001;
	ir_dev->id.product = 0x0001;
	ir_dev->id.version = 0x0100;

    #ifdef REPORT_REPEAT_KEY_VALUE
	ir_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP) ;
    #else
    	ir_dev->evbit[0] = BIT_MASK(EV_KEY);
    #endif
    
	for (i = 0; i < 256; i++)
		set_bit(ir_keycodes[i], ir_dev->keybit);
		
	if (request_irq(IR_IRQNO, ir_irq_service, 0, "RemoteIR",
			ir_dev)) {
		err = -EBUSY;
		goto fail2;
	}

	ir_setup();
	  
	s_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	if (!s_timer) {
		ret =  - ENOMEM;
		printk(KERN_DEBUG " IR FAIL TO  Request Time\n");
		goto fail3;
	}
	init_timer(s_timer);
	s_timer->function = &ir_timer_handle;

#ifdef CONFIG_HAS_EARLYSUSPEND
#else
#ifdef CONFIG_PM
	ir_pm_domain.ops.suspend = sun6i_ir_suspend;
	ir_pm_domain.ops.resume = sun6i_ir_resume;
	ir_dev->dev.pm_domain = &ir_pm_domain;	
#endif
#endif

	err = input_register_device(ir_dev);
	if (err)
		goto fail4;
	printk("IR Initial OK\n");

#ifdef CONFIG_HAS_EARLYSUSPEND	
	dprintk(DEBUG_INIT, "==register_early_suspend =\n");
	ir_data = kzalloc(sizeof(*ir_data), GFP_KERNEL);
	if (ir_data == NULL) {
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ir_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;	
	ir_data->early_suspend.suspend = sun6i_ir_early_suspend;
	ir_data->early_suspend.resume	= sun6i_ir_late_resume;	
	register_early_suspend(&ir_data->early_suspend);
#endif

	dprintk(DEBUG_INIT, "ir_init end\n");

	return 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
err_alloc_data_failed:
#endif
fail4:	
 	kfree(s_timer);
fail3:	
 	free_irq(IR_IRQNO, ir_dev);
fail2:	
 	input_free_device(ir_dev);
fail1:	
 	return err;
}

static void __exit ir_exit(void)
{	
#ifdef CONFIG_HAS_EARLYSUSPEND	
	unregister_early_suspend(&ir_data->early_suspend);	
#endif

	free_irq(IR_IRQNO, ir_dev);
	input_unregister_device(ir_dev);
	ir_sys_uncfg();
 	kfree(s_timer);
}

module_init(ir_init);
module_exit(ir_exit);

MODULE_DESCRIPTION("Remote IR driver");
MODULE_AUTHOR("DanielWang");
MODULE_LICENSE("GPL");

