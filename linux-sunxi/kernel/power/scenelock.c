/* kernel/power/scenelock.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *
 *  By      : liming
 *  Version : v1.0
 *  Date    : 2013-4-17 09:08
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/scenelock.h>
#include "power.h"
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif

enum {
	DEBUG_EXIT_SUSPEND = 1U << 0,
	DEBUG_WAKEUP = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_EXPIRE = 1U << 3,
	DEBUG_SCENE_LOCK = 1U << 4,
};
static int debug_mask = 0xff;//DEBUG_WAKEUP;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define SCENE_LOCK_TYPE_MASK              (0x0f)
#define SCENE_LOCK_INITIALIZED            (1U << 8)
#define SCENE_LOCK_ACTIVE                 (1U << 9)

static DEFINE_SPINLOCK(list_lock);
static LIST_HEAD(inactive_locks);
static struct list_head active_scene_locks[SCENE_MAX];

#ifdef CONFIG_HAS_WAKELOCK
struct wake_lock mp3_extended_standby_wake_lock;
#endif

/* Caller must acquire the list_lock spinlock */
static void print_active_locks(aw_power_scene_e type)
{
	struct scene_lock *lock;

	BUG_ON(type >= SCENE_MAX);
	list_for_each_entry(lock, &active_scene_locks[type], link) {
		pr_info("active scene lock %s\n", lock->name);
	}
}

static long has_scene_lock_locked(aw_power_scene_e type)
{
	struct scene_lock *lock, *n;

	BUG_ON(type >= SCENE_MAX);
	list_for_each_entry_safe(lock, n, &active_scene_locks[type], link) {
		if (lock->flags & SCENE_LOCK_ACTIVE) {
			return 0;
		}
	}
	return -1;
}

long has_scene_lock(aw_power_scene_e type)
{
	long ret;
	unsigned long irqflags;
	spin_lock_irqsave(&list_lock, irqflags);
	ret = has_scene_lock_locked(type);
	if ((!ret) && (debug_mask & DEBUG_WAKEUP))
		print_active_locks(type);
	spin_unlock_irqrestore(&list_lock, irqflags);
	return ret;
}

void scene_lock_init(struct scene_lock *lock, aw_power_scene_e type, const char *name)
{
	unsigned long irqflags = 0;

	if (name)
		lock->name = name;
	BUG_ON(!lock->name);

	if (debug_mask & DEBUG_SCENE_LOCK)
		pr_info("scene_lock_init name=%s\n", lock->name);
	lock->count = 0;
	lock->flags = (type & SCENE_LOCK_TYPE_MASK) | SCENE_LOCK_INITIALIZED;

	INIT_LIST_HEAD(&lock->link);
	spin_lock_irqsave(&list_lock, irqflags);
	list_add(&lock->link, &inactive_locks);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(scene_lock_init);

void scene_lock_destroy(struct scene_lock *lock)
{
	unsigned long irqflags;
	if (debug_mask & DEBUG_SCENE_LOCK) {
		if (NULL != lock->name)
			pr_info("scene_lock_destroy name=%s\n", lock->name);
	}
	spin_lock_irqsave(&list_lock, irqflags);
	lock->flags &= ~SCENE_LOCK_INITIALIZED;
	list_del(&lock->link);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(scene_lock_destroy);

void scene_lock(struct scene_lock *lock)
{
	aw_power_scene_e type;
	unsigned long irqflags;
	bool err = false;
	unsigned long flags = 0;

	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & SCENE_LOCK_TYPE_MASK;
	BUG_ON(type >= SCENE_MAX);
	BUG_ON(!(lock->flags & SCENE_LOCK_INITIALIZED));

	if (!(lock->flags & SCENE_LOCK_ACTIVE)) {
		lock->flags |= SCENE_LOCK_ACTIVE;
	}
	lock->count += 1;
	list_del(&lock->link);

	if (debug_mask & DEBUG_SCENE_LOCK)
		pr_info("scene_lock: %s, type %d, count %d\n", lock->name,
			type, lock->count);

	list_add(&lock->link, &active_scene_locks[type]);
	spin_unlock_irqrestore(&list_lock, irqflags);

	if (SCENE_USB_STANDBY == type)
		flags |= USB_STANDBY_FLAGS;
	else if (SCENE_TALKING_STANDBY == type)
		flags |= TALKING_STANDBY_FLAGS;
	else if (SCENE_MP3_STANDBY == type) {
		flags |= MP3_STANDBY_FLAGE;
#ifdef CONFIG_HAS_WAKELOCK
		wake_lock(&mp3_extended_standby_wake_lock);
#endif
	}
	err = set_extended_standby_manager(flags);
	if (!err)
		printk("set extended standby manager err\n");
}
EXPORT_SYMBOL(scene_lock);


void scene_unlock(struct scene_lock *lock)
{
	aw_power_scene_e type;
	unsigned long irqflags;
	bool err = false;
	unsigned long flags = 0;

	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & SCENE_LOCK_TYPE_MASK;

	if (debug_mask & DEBUG_SCENE_LOCK)
			pr_info("scene_unlock: %s, count: %d\n", lock->name, lock->count);
	if (1 < lock->count) {
		lock->count -= 1;
	} else if (1 == lock->count) {
		lock->count -= 1;
		lock->flags &= ~(SCENE_LOCK_ACTIVE);
		list_del(&lock->link);
		list_add(&lock->link, &inactive_locks);

		spin_unlock_irqrestore(&list_lock, irqflags);

		if (!check_scene_locked(SCENE_USB_STANDBY))
			flags |= USB_STANDBY_FLAGS;
		else if (!check_scene_locked(SCENE_TALKING_STANDBY))
			flags |= TALKING_STANDBY_FLAGS;
		else if (!check_scene_locked(SCENE_MP3_STANDBY))
			flags |= MP3_STANDBY_FLAGE;
		err = set_extended_standby_manager(0);
		if (!err)
			printk("clear extended standby manager err\n");
		err = set_extended_standby_manager(flags);
		if (!err)
			printk("set extended standby manager err\n");

		if (check_scene_locked(SCENE_MP3_STANDBY)) {
#ifdef CONFIG_HAS_WAKELOCK
			wake_unlock(&mp3_extended_standby_wake_lock);
#endif
		}
		return;
	}
	spin_unlock_irqrestore(&list_lock, irqflags);

}
EXPORT_SYMBOL(scene_unlock);

int check_scene_locked(aw_power_scene_e type)
{
	return !!has_scene_lock(type);
}
EXPORT_SYMBOL(check_scene_locked);

int scene_lock_active(struct scene_lock *lock)
{
	return !!(lock->flags & SCENE_LOCK_ACTIVE);
}
EXPORT_SYMBOL(scene_lock_active);

/******************************* wakeup src *************************************/

/**
 *	enable_wakeup_src - 	enable the wakeup src.
 *
 *	function:		the device driver care about the wakeup src.
 *				if the device driver do want the system be wakenup while in standby state.
 *				the device driver should use this function to enable corresponding intterupt.
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src, 
 *				else ignored.
 *	notice:			1. for gpio intterupt, only access the enable bit, mean u need care about other config,
 *				such as: int mode, pull up or pull down resistance, etc.
 *				2. At a31, only gpio��pa, pb, pe, pg, pl, pm��int wakeup src is supported. 
*/
int enable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	int err = -1;
	err = extended_standby_enable_wakeup_src(src, para);
	return err;
}
EXPORT_SYMBOL(enable_wakeup_src);

/**
 *	disable_wakeup_src - 	disable the wakeup src.
 *
 *	function:		if the device driver do not want the system be wakenup while in standby state again.
 *				the device driver should use this function to disable the corresponding intterupt.
 *
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src, 
 *				else ignored.
 *	notice:			for gpio intterupt, only access the enable bit, mean u need care about other config,
 *				such as: int mode, pull up or pull down resistance, etc.
 */
int disable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	int err = -1;
	err = extended_standby_disable_wakeup_src(src, para);
	return err;
}
EXPORT_SYMBOL(disable_wakeup_src);

/**
 *	check_wakeup_state -    to get the corresponding wakeup src intterupt state, enable or disable.
 *
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src, 
 *				else ignored.
 *
 *	return value:		enable, 	return 1,
 *				disable,	return 2,
 *				error: 		return -1.
 */
int check_wakeup_state(cpu_wakeup_src_e src, int para)
{
	int err = -1;
	err = extended_standby_check_wakeup_state(src, para);
	return err;
}
EXPORT_SYMBOL(check_wakeup_state);

/**
 *	show_standby_state - 	show current standby state, for debug purpose.
 *
 *	function:		standby state including locked_scene, power_supply dependancy, the wakeup src.
 *
 *	return value:		succeed, return 0, else return -1.
 */
int standby_show_state(void)
{
	int err = -1;
	err = extended_standby_show_state();
	return err;
}
EXPORT_SYMBOL(standby_show_state);

/******************************* wakeup src *************************************/


static int __init scenelocks_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(active_scene_locks); i++)
		INIT_LIST_HEAD(&active_scene_locks[i]);
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&mp3_extended_standby_wake_lock, WAKE_LOCK_SUSPEND, "mp3_extended_standby");
#endif
	return 0;
}

static void  __exit scenelocks_exit(void)
{
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy(&mp3_extended_standby_wake_lock);
#endif
}

core_initcall(scenelocks_init);
module_exit(scenelocks_exit);
