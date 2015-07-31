/*
 * sunxi operation system resource
 * Author:raymonxiu
 */
#ifndef __VFE__OS__H__
#define __VFE__OS__H__

#include <linux/device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
//#include <linux/gpio.h>

#include <mach/clock.h>
#include <mach/sys_config.h>
#include <mach/irqs.h>
#include <mach/gpio.h>

extern unsigned int vfe_dbg_en;
extern unsigned int vfe_dbg_lv;

//for internel driver debug
#define vfe_dbg(l,x,arg...) if(vfe_dbg_en && (l <= vfe_dbg_lv)) printk(KERN_DEBUG"[VFE_DEBUG]"x,##arg)
//print when error happens
#define vfe_err(x,arg...) printk(KERN_ERR"[VFE_ERR]"x,##arg)
#define vfe_warn(x,arg...) printk(KERN_WARNING"[VFE_WARN]"x,##arg)
//print unconditional, for important info
#define vfe_print(x,arg...) printk(KERN_NOTICE"[VFE]"x,##arg)

typedef unsigned int __hdle;

extern struct clk *os_clk_get(struct device *dev, const char *id);
extern void  os_clk_put(struct clk *clk);
extern int os_clk_set_parent(struct clk *clk, struct clk *parent);
extern int os_clk_set_rate(struct clk *clk, unsigned long rate);
extern int os_clk_enable(struct clk *clk);
extern void os_clk_disable(struct clk *clk);
extern int os_clk_reset_assert(struct clk *clk);
extern int os_clk_reset_deassert(struct clk *clk); 
extern int os_request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,const char *name, void *dev);
extern __hdle os_gpio_request(unsigned gpio, const char *label);
//extern __hdle os_gpio_request_ex(char *main_name, const char *sub_name);
extern int os_gpio_release(unsigned gpio);                
extern int os_gpio_write(struct gpio_config *gpio, unsigned int status);
extern int os_gpio_set_status(struct gpio_config *gpio, unsigned int status);

#endif //__VFE__OS__H__