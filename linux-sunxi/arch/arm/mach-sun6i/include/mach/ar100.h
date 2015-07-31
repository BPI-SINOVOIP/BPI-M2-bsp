/*
 * arch/arm/mach-sun6i/include/mach/ar100.h
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

#ifndef __ASM_ARCH_A100_H
#define __ASM_ARCH_A100_H

#include <linux/power/aw_pm.h>

/* the modes of ar100 dvfs */
#define AR100_DVFS_SYN      (1<<0)

/* axp driver interfaces */
#define AXP_TRANS_BYTE_MAX  (8)

/* ar100 call-back */
typedef int (*ar100_cb_t)(void *arg);

/**
 * set target frequency.
 * @freq:    target frequency to be set, based on HZ.
 * @cb:      callback handler
 * @cb_arg:  arguments of callback handler
 *
 * return: result, 0 - set frequency successed, !0 - set frequency failed;
 */
int ar100_dvfs_set_cpufreq(unsigned int freq, unsigned long mode, ar100_cb_t cb, void *cb_arg);

/**
 * enter super standby.
 * @para:  parameter for enter normal standby.
 *
 * return: result, 0 - super standby successed, !0 - super standby failed;
 */
int ar100_standby_super(struct super_standby_para *para, ar100_cb_t cb, void *cb_arg);

/**
 * query super-standby wakeup source.
 * @para:  point of buffer to store wakeup event informations.
 *
 * return: result, 0 - query successed, !0 - query failed;
 */
int ar100_query_wakeup_source(unsigned long *event);

/**
 * query super-standby dram crc result.
 * @perror:  pointer of dram crc result.
 * @ptotal_count: pointer of dram crc total count
 * @perror_count: pointer of dram crc error count
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int ar100_query_dram_crc_result(unsigned long *perror, unsigned long *ptotal_count,
	unsigned long *perror_count);

int ar100_set_dram_crc_result(unsigned long error, unsigned long total_count,
	unsigned long error_count);

/**
 * notify ar100 cpux restored.
 * @para:  none.
 *
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int ar100_cpux_ready_notify(void);


/**
 * read axp register data.
 * @addr:    point of registers address;
 * @data:    point of registers data;
 * @len :    number of read registers, max len:8;
 *
 * return: result, 0 - read register successed,
 *                !0 - read register failed or the len more then max len;
 */
int ar100_axp_read_reg(unsigned char *addr, unsigned char *data, unsigned long len);

/**
 * write axp register data.
 * addr:     point of registers address;
 * data:     point of registers data;
 * len :     number of write registers, max len:8;
 *
 * return: result, 0 - write register successed,
 *                !0 - write register failedor the len more then max len;
 */
int ar100_axp_write_reg(unsigned char *addr, unsigned char *data, unsigned long len);

/**
 * clear axp register bits sync.
 * addr :     point of registers address;
 * mask :     point of mask bits data;
 * delay:     point of delay times;
 * len  :     number of write registers, max len:8;
 *
 * return: result, 0 - clear register successed,
 *                !0 - clear register failed, or the len more then max len;
 *
 * clear axp register bits internal:
 * data = read_axp_reg(addr);
 * data = data & (~mask);
 * write_axp_reg(addr, data);
 *
 */
int ar100_axp_clr_regs_bits_sync(unsigned char *addr, unsigned char *mask, unsigned char *delay, unsigned long len);

/**
 * set axp register bits sync.
 * addr :     point of registers address;
 * mask :     point of mask bits data;
 * delay:     point of delay times;
 * len  :     number of write registers, max len:8;
 *
 * return: result, 0 - clear register successed,
 *                !0 - clear register failed, or the len more then max len;
 * clear axp register bits internal:
 * data = read_axp_reg(addr);
 * data = data | mask;
 * write_axp_reg(addr, data);
 */
int ar100_axp_set_regs_bits_sync(unsigned char *addr, unsigned char *mask, unsigned char *delay, unsigned long len);

/**
 * register call-back function, call-back function is for ar100 notify some event to ac327,
 * axp interrupt for ex.
 * func:  call-back function;
 * para:  parameter for call-back function;
 *
 * return: result, 0 - register call-back function successed;
 *                !0 - register call-back function failed;
 * NOTE: the function is like "int callback(void *para)";
 */
int ar100_axp_cb_register(ar100_cb_t func, void *para);


/**
 * unregister call-back function.
 * @func:  call-back function which need be unregister;
 */
void ar100_axp_cb_unregister(ar100_cb_t func);

int ar100_disable_axp_irq(void);
int ar100_enable_axp_irq(void);

int ar100_axp_get_chip_id(unsigned char *chip_id);

int ar100_message_loopback(void);

/* talk-standby interfaces */
int ar100_standby_talk(struct super_standby_para *para, ar100_cb_t cb, void *cb_arg);
int ar100_cpux_talkstandby_ready_notify(void);

void ar100_fake_power_off(void);

#endif  /* __ASM_ARCH_A100_H */
