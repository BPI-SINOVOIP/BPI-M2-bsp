/*
 * arch/arm/mach-sun6i/include/mach/gpio.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i gpio driver header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SW_GPIO_H
#define __SW_GPIO_H

#include <linux/types.h>

/* pio/rpio base */
#define PIO_VBASE(n) 		(0xf1c20800 + ((n) << 5) + ((n) << 2)) /* pio(PA ~ PF), 0xf1c20800 + n * 0x24 */
#define RPIO_VBASE(n) 		(0xf1f02c00 + ((n) << 5) + ((n) << 2)) /* r-pio(PL ~ PM), 0xf1f02c00 + n * 0x24 */

/* einit config reg vbase */
#define PIO_VBASE_EINT_PA	(PIO_VBASE(0) + 0x200                 )
#define PIO_VBASE_EINT_PB	(PIO_VBASE(0) + 0x220                 )
#define PIO_VBASE_EINT_PE	(PIO_VBASE(0) + 0x240                 )
#define PIO_VBASE_EINT_PG	(PIO_VBASE(0) + 0x260                 )
#define PIO_VBASE_EINT_R_PL	(RPIO_VBASE(0) + 0x200                )
#define PIO_VBASE_EINT_R_PM	(RPIO_VBASE(0) + 0x220                )

/* port number for each pio */
#define PA_NR			28
#define PB_NR			8
#define PC_NR			28
#define PD_NR			28
#define PE_NR			17
#define PF_NR			6
#define PG_NR			19
#define PH_NR			31
/* for R-PORT PIO */
#define PL_NR			9
#define PM_NR			8
#ifdef CONFIG_AW_AXP22
/* for axp power PIO */
#define AXP_NR			5
#endif /* CONFIG_AW_AXP22 */

/*
 * base index for each pio
 */
#define SUN6I_GPIO_SPACE	2 /* for debugging purposes so that failed if request extra gpio_nr */
#define AW_GPIO_NEXT(gpio)	gpio##_NR_BASE + gpio##_NR + SUN6I_GPIO_SPACE
enum sun6i_gpio_number {
	PA_NR_BASE = 0,
	PB_NR_BASE = AW_GPIO_NEXT(PA),
	PC_NR_BASE = AW_GPIO_NEXT(PB),
	PD_NR_BASE = AW_GPIO_NEXT(PC),
	PE_NR_BASE = AW_GPIO_NEXT(PD),
	PF_NR_BASE = AW_GPIO_NEXT(PE),
	PG_NR_BASE = AW_GPIO_NEXT(PF),
	PH_NR_BASE = AW_GPIO_NEXT(PG),

	/* for R-PORT PIO */
	PL_NR_BASE = AW_GPIO_NEXT(PH), /* last is PH */
	PM_NR_BASE = AW_GPIO_NEXT(PL),
#ifdef CONFIG_AW_AXP22
	/* for axp power PIO */
	AXP_NR_BASE = AW_GPIO_NEXT(PM), /* last is PM */
	GPIO_INDEX_END = AW_GPIO_NEXT(AXP), /* last is AXP */
#else
	GPIO_INDEX_END = AW_GPIO_NEXT(PM), /* last is PM */
#endif /* CONFIG_AW_AXP22 */
};

/* pio index definition */
#define GPIOA(n)		(PA_NR_BASE + (n))
#define GPIOB(n)		(PB_NR_BASE + (n))
#define GPIOC(n)		(PC_NR_BASE + (n))
#define GPIOD(n)		(PD_NR_BASE + (n))
#define GPIOE(n)		(PE_NR_BASE + (n))
#define GPIOF(n)		(PF_NR_BASE + (n))
#define GPIOG(n)		(PG_NR_BASE + (n))
#define GPIOH(n)		(PH_NR_BASE + (n))
#define GPIOL(n)		(PL_NR_BASE + (n))
#define GPIOM(n)		(PM_NR_BASE + (n))
#ifdef CONFIG_AW_AXP22
/* for axp power PIO */
#define GPIO_AXP(n)		(AXP_NR_BASE + (n))
#endif /* CONFIG_AW_AXP22 */

/* pio default macro */
#define GPIO_PULL_DEFAULT	((u32)-1         )
#define GPIO_DRVLVL_DEFAULT	((u32)-1         )
#define GPIO_DATA_DEFAULT	((u32)-1         )

/* pio end, invalid macro */
#define GPIO_INDEX_INVALID	(0xFFFFFFF0      )
#define GPIO_CFG_INVALID	(0xEEEEEEEE      )
#define GPIO_PULL_INVALID	(0xDDDDDDDD      )
#define GPIO_DRVLVL_INVALID	(0xCCCCCCCC      )
#define IRQ_NUM_INVALID		(0xFFFFFFFF      )
#define AXP_PORT_VAL		(0x0000FFFF      ) /* port val for axp pin in sys_config.fex */

/* config value for external int */
#define GPIO_CFG_EINT   	(0b110	)	/* config value to eint for pa, pb, pe, pg */
#define R_GPIO_CFG_EINT   	(0b010	)	/* config value to eint for r-pl, r-pm */
#define GPIO_CFG_INPUT   	(0	)	/* mul sel value for input */
#define GPIO_CFG_OUTPUT   	(1	)	/* mul sel value for output */

/* port number for gpiolib */
#ifdef ARCH_NR_GPIOS
#undef ARCH_NR_GPIOS
#endif
#define ARCH_NR_GPIOS		(GPIO_INDEX_END)

/* gpio config info */
struct gpio_config {
	u32	gpio;		/* gpio global index, must be unique */
	u32 	mul_sel;	/* multi sel val: 0 - input, 1 - output... */
	u32 	pull;		/* pull val: 0 - pull up/down disable, 1 - pull up... */
	u32 	drv_level;	/* driver level val: 0 - level 0, 1 - level 1... */
	u32	data;		/* data val: 0 - low, 1 - high, only vaild when mul_sel is input/output */
};

/* gpio eint trig type */
enum gpio_eint_trigtype {
	TRIG_EDGE_POSITIVE = 0,
	TRIG_EDGE_NEGATIVE,
	TRIG_LEVL_HIGH,
	TRIG_LEVL_LOW,
	TRIG_EDGE_DOUBLE,	/* positive/negative */
	TRIG_INALID
};

/* gpio eint debounce para */
struct gpio_eint_debounce {
	u32   clk_sel;		/* pio interrupt clock select, 0-LOSC, 1-HOSC */
	u32   clk_pre_scl;	/* debounce clk pre-scale n, the select,
				 * clock source is pre-scale by 2^n.
				 */
};

/* gpio external config info */
struct gpio_config_eint_all {
	u32	gpio;		/* the global gpio index */
	u32	pull;		/* gpio pull val */
	u32 	drvlvl;		/* gpio driver level */
	u32	enabled;	/* in set function: used to enable/disable the eint, 1: enable, 0: disable
				 * in get function: return the eint enabled status, 1: enabled, 0: disabled
				 */
	u32	irq_pd;		/* in set function: 1 means to clr irq pend status, 0 no use
				 * in get function: return the actual irq pend stauts, eg, 1 means irq occur.
				 */
	enum gpio_eint_trigtype trig_type; /* trig type of the gpio */
};

/* gpio eint call back function */
typedef u32 (*peint_handle)(void *para);

/*
 * exported api below
 */

int sw_gpio_to_name(u32 gpio, char *name);

/* api for multi function */
u32 sw_gpio_setcfg(u32 gpio, u32 val);
u32 sw_gpio_getcfg(u32 gpio);
u32 sw_gpio_setpull(u32 gpio, u32 val);
u32 sw_gpio_getpull(u32 gpio);
u32 sw_gpio_setdrvlevel(u32 gpio, u32 val);
u32 sw_gpio_getdrvlevel(u32 gpio);
u32 sw_gpio_setall_range(struct gpio_config *pcfg, u32 cfg_num);
u32 sw_gpio_getall_range(struct gpio_config *pcfg, u32 cfg_num);
void sw_gpio_dump_config(struct gpio_config *pcfg, u32 cfg_num);
u32 sw_gpio_suspend(void);
u32 sw_gpio_resume(void);

/* api for external int */
u32 sw_gpio_irq_request(u32 gpio, enum gpio_eint_trigtype trig_type,
			peint_handle handle, void *para);
void sw_gpio_irq_free(u32 handle);
u32 sw_gpio_eint_set_trigtype(u32 gpio, enum gpio_eint_trigtype trig_type);
u32 sw_gpio_eint_get_trigtype(u32 gpio, enum gpio_eint_trigtype *pval);
u32 sw_gpio_eint_get_enable(u32 gpio, u32 *penable);
u32 sw_gpio_eint_set_enable(u32 gpio, u32 enable);
u32 sw_gpio_eint_get_irqpd_sta(u32 gpio);
u32 sw_gpio_eint_clr_irqpd_sta(u32 gpio);
u32 sw_gpio_eint_get_debounce(u32 gpio, struct gpio_eint_debounce *pdbc);
u32 sw_gpio_eint_set_debounce(u32 gpio, struct gpio_eint_debounce dbc);
u32 sw_gpio_eint_setall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num);
u32 sw_gpio_eint_getall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num);
void sw_gpio_eint_dumpall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num);

#endif /* __SW_GPIO_H */

