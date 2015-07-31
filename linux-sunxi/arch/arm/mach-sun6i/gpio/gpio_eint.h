/*
 * arch/arm/mach-sun6i/gpio/gpio_eint.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i gpio header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __GPIO_EINT_H
#define __GPIO_EINT_H

#define PIO_EINT_OFF_REG_CFG		0
#define PIO_EINT_OFF_REG_CTRL		0x10
#define PIO_EINT_OFF_REG_STATUS		0x14
#define PIO_EINT_OFF_REG_DEBOUNCE	0x18

#define R_PL_EINT_START			5 	/* r_pl_5 as S_PL_EINT0 */

struct gpio_irq_handle {
	u32 	gpio;
	peint_handle handler;
	void	*parg;
};

u32 gpio_eint_set_trig(struct aw_gpio_chip *pchip, u32 offset, enum gpio_eint_trigtype trig_val);
u32 gpio_eint_get_trig(struct aw_gpio_chip *pchip, u32 offset, enum gpio_eint_trigtype *pval);
u32 gpio_eint_set_enable(struct aw_gpio_chip *pchip, u32 offset, u32 enable);
u32 gpio_eint_get_enable(struct aw_gpio_chip *pchip, u32 offset, u32 *penable);
u32 gpio_eint_get_irqpd_sta(struct aw_gpio_chip *pchip, u32 offset);
u32 gpio_eint_clr_irqpd_sta(struct aw_gpio_chip *pchip, u32 offset);
u32 gpio_eint_set_debounce(struct aw_gpio_chip *pchip, struct gpio_eint_debounce val);
u32 gpio_eint_get_debounce(struct aw_gpio_chip *pchip, struct gpio_eint_debounce *pval);
bool is_gpio_canbe_eint(u32 gpio);

#endif /* __GPIO_EINT_H */

