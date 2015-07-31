/*
 * arch/arm/mach-sun6i/pm/standby/standby_ar100.h
 *
 * Copyright 2012 (c) Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__ASM_ARCH_STANDBY_A100_H
#define	__ASM_ARCH_STANDBY_A100_H

//the sync mode between ar100 and ac327
#define STANDBY_AR100_SYNC (1<<1)
#define STANDBY_AR100_ASYNC (1<<2)
 
 int standby_ar100_init(void);
 int standby_ar100_exit(void);
 
/*
 * notify ar100 to wakeup: restore cpus freq, volt, and init_dram.
 * para:  mode.
 * STANDBY_AR100_SYNC:
 * STANDBY_AR100_ASYNC:
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int standby_ar100_notify_restore(unsigned long mode);
/*
 * check ar100 restore status.
 * para:  none.
 * return: result, 0 - restore completion successed, !0 - notify failed;
 */
int standby_ar100_check_restore_status(void);
/*
 * query standby wakeup source.
 * para:  point of buffer to store wakeup event informations.
 * return: result, 0 - query successed, !0 - query failed;
 */
int standby_ar100_query_wakeup_src(unsigned long *event);
/*
 * enter normal standby.
 * para:  parameter for enter normal standby.
 * return: result, 0 - normal standby successed, !0 - normal standby failed;
 */
int standby_ar100_standby_normal(struct normal_standby_para *para);


#endif	//__ASM_ARCH_STANDBY_A100_H
