/* include/linux/scenelock.h
 *
 * Copyright (C) 2013-2014 allwinner.
 *
 *  By      : liming
 *  Version : v1.0
 *  Date    : 2013-4-17 09:08
 */

#ifndef _LINUX_SCENELOCK_H
#define _LINUX_SCENELOCK_H

#include <linux/list.h>
#include <linux/ktime.h>
#include <linux/extended_standby.h>

typedef enum AW_POWER_SCENE
{
    SCENE_USB_STANDBY,
    SCENE_TALKING_STANDBY,
    SCENE_MP3_STANDBY,
    SCENE_MAX
} aw_power_scene_e;

struct scene_lock {
#ifdef CONFIG_SCENELOCK
	struct list_head    link;
	int                 flags;
	int                 count;
	const char         *name;
#endif
};

#ifdef CONFIG_SCENELOCK

void scene_lock_init(struct scene_lock *lock, aw_power_scene_e type, const char *name);
void scene_lock_destroy(struct scene_lock *lock);
void scene_lock(struct scene_lock *lock);
void scene_unlock(struct scene_lock *lock);

/* check_scene_locked returns a zero value if the scene_lock is currently
 * locked.
 */
int check_scene_locked(aw_power_scene_e type);

/* scene_lock_active returns 0 if no scene locks of the specified type are active,
 * and non-zero if one or more scene locks are held.
 */
int scene_lock_active(struct scene_lock *lock);

int enable_wakeup_src(cpu_wakeup_src_e src, int para);

int disable_wakeup_src(cpu_wakeup_src_e src, int para);

int check_wakeup_state(cpu_wakeup_src_e src, int para);

int standby_show_state(void);

#else

static inline void scene_lock_init(struct scene_lock *lock, int type,
					const char *name) {}
static inline void scene_lock_destroy(struct scene_lock *lock) {}
static inline void scene_lock(struct scene_lock *lock) {}
static inline void scene_unlock(struct scene_lock *lock) {}

static inline int check_scene_locked(aw_power_scene_e type) { return 0; }
static inline int scene_lock_active(struct scene_lock *lock) { return 0; }

static inline int enable_wakeup_src(cpu_wakeup_src_e src, int para) { return 0; }
static inline int disable_wakeup_src(cpu_wakeup_src_e src, int para) { return 0; }
static inline int check_wakeup_state(cpu_wakeup_src_e src, int para) { return 0; }
static inline int standby_show_state(void) { return 0; }

#endif

#endif

