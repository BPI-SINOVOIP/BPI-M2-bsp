/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _SUNXI_SC_H_
#define _SUNXI_SC_H_


//Justin Porting for BPI-M2 20150727 Start
struct sunxi_sc_regs {
	volatile u32 sram_ctrl_reg0;
	volatile u32 sram_ctrl_reg1;
	volatile u32 pad[7];
	volatile u32 ver_reg;
	volatile u32 nmi_irq_ctrl_reg;
	volatile u32 nmi_irq_pend_reg;
	volatile u32 nmi_irq_enable_reg;
};
//Justin Porting for BPI-M2 20150727 End


#endif

