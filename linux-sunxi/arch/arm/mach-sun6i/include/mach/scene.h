/*
 * arch/arm/mach-sun6i/include/mach/scene.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Kevin <kevin@reuuimllatech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SCENE_H__
#define __SCENE_H__

#include <linux/notifier.h>

enum scene_type {
    SCENE_TYPE_DEF  = 0,
    SCENE_TYPE1     = 1,
    SCENE_TYPE2     = 2,
    SCENE_TYPE3     = 3,
    SCENE_TYPE4     = 4,
    SCENE_TYPE5     = 5,
    SCENE_TYPE6     = 6,
    SCENE_TYPE7     = 7,
    SCENE_TYPE8     = 8,
    SCENE_TYPE9     = 9,
    SCENE_TYPE_MAX  = 10,
};

extern int scene_register_notifier(struct notifier_block *nb);
extern int scene_unregister_notifier(struct notifier_block *nb);

#endif /* __SCENE_H__ */
