/* arch/arm/mach-sun6i/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * Benn Huang (benn@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <mach/platform.h>

#ifdef CONFIG_DEBUG_AW_UART0
#define UART_PADDR	AW_UART0_BASE
#define UART_VADDR	(0xf0000000 + AW_UART0_BASE)
	
#elif defined (CONFIG_DEBUG_AW_R_UART)
#define UART_PADDR	AW_R_UART_BASE
#define UART_VADDR	(0xf0000000 + AW_R_UART_BASE)
#else
#error "Please select a uart port"
#endif

	.macro	addruart, rp, rv, tmp
	ldr	\rv, =UART_VADDR
	ldr	\rp, =UART_PADDR
	.endm


        .macro  senduart, rd, rx
	str	\rd, [\rx, #AW_UART_THR]
        .endm

        .macro  busyuart, rd, rx
        .endm

        .macro  waituart, rd, rx
1002:	ldr	\rd, [\rx, #AW_UART_USR]
	tst	\rd, #0x2
	beq	1002b
        .endm



