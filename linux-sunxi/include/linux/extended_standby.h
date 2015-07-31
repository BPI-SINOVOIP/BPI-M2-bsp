/* extended_standby.h
 *
 * Copyright (C) 2013-2014 allwinner.
 *
 *  By      : liming
 *  Version : v1.0
 *  Date    : 2013-4-17 09:08
 */

#ifndef _EXTENDED_STANDBY_H
#define _EXTENDED_STANDBY_H
#include <linux/power/aw_pm.h>
typedef enum CPU_WAKEUP_SRC {
/* the wakeup source of main cpu: cpu0, only used in normal standby */	
	CPU0_MSGBOX_SRC		= CPU0_WAKEUP_MSGBOX,  /* external interrupt, pmu event for ex. */
	CPU0_KEY_SRC		= CPU0_WAKEUP_KEY,     /* key event, volume home menu enter */

/* the wakeup source of assistant cpu: cpus */
	CPUS_LOWBATT_SRC	= CPUS_WAKEUP_LOWBATT, /* low battery event */
	CPUS_USB_SRC		= CPUS_WAKEUP_USB ,    /* usb insert event */
	CPUS_AC_SRC		= CPUS_WAKEUP_AC,      /* charger insert event */
	CPUS_ASCEND_SRC		= CPUS_WAKEUP_ASCEND,  /* power key ascend event */
	CPUS_DESCEND_SRC	= CPUS_WAKEUP_DESCEND, /* power key descend event */
	CPUS_SHORT_KEY_SRC	= CPUS_WAKEUP_SHORT_KEY , /* power key short press event */
	CPUS_LONG_KEY_SRC	= CPUS_WAKEUP_LONG_KEY,   /* power key long press event */
	CPUS_IR_SRC		= CPUS_WAKEUP_IR,      /* IR key event */
	CPUS_ALM0_SRC		= CPUS_WAKEUP_ALM0,    /* alarm0 event */
	CPUS_ALM1_SRC		= CPUS_WAKEUP_ALM1,    /* alarm1 event */
	CPUS_TIMEOUT_SRC	= CPUS_WAKEUP_TIMEOUT, /* debug test event */
	CPUS_GPIO_SRC		= CPUS_WAKEUP_GPIO,    /* GPIO interrupt event, only used in extended standby */
	CPUS_USBMOUSE_SRC	= CPUS_WAKEUP_USBMOUSE,/* USBMOUSE key event, only used in extended standby */
	CPUS_LRADC_SRC		= CPUS_WAKEUP_LRADC ,  /* key event, volume home menu enter, only used in extended standby */
	CPUS_CODEC_SRC		= CPUS_WAKEUP_CODEC,   /* earphone insert/pull event, only used in extended standby */
	CPUS_KEY_SL_SRC		= CPUS_WAKEUP_KEY      /* power key short and long press event */
}cpu_wakeup_src_e;

typedef struct extended_standby_manager{
	extended_standby_t *pextended_standby;
	unsigned long event;
	unsigned long wakeup_gpio_map;
	unsigned long wakeup_gpio_group;
}extended_standby_manager_t;

typedef enum POWER_SCENE_FLAGS
{
	TALKING_STANDBY_FLAGS           = (1<<0x0),
	USB_STANDBY_FLAGS               = (1<<0x1),
	MP3_STANDBY_FLAGE               = (1<<0x2),
} power_scene_flags;

const extended_standby_manager_t *get_extended_standby_manager(void);

bool set_extended_standby_manager(unsigned long flags);

int extended_standby_enable_wakeup_src(cpu_wakeup_src_e src, int para);

int extended_standby_disable_wakeup_src(cpu_wakeup_src_e src, int para);

int extended_standby_check_wakeup_state(cpu_wakeup_src_e src, int para);

int extended_standby_show_state(void);
#endif
