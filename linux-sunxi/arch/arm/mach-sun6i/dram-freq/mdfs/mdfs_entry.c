/*
 * arch/arm/mach-sun6i/dram-freq/mdfs/mdfs.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN6I dram frequency dynamic scaling driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "mdfs.h"

static unsigned int sp_backup;
struct aw_mdfs_info mdfs_info;

extern char __bss_start[];
extern char __bss_end[];

extern void mdfs_memcpy(void *dest, const void *src, int n);
extern void mdfs_memset(void *s, int c, int n);
extern int  mdfs_start(struct aw_mdfs_info *mdfs);

int mdfs_main(struct aw_mdfs_info *mdfs)
{
    MDFS_DBG("%s enter\n", __func__);
    /* clear bss section */
    mdfs_memset(__bss_start, 0, (unsigned int)(__bss_end - __bss_start));
    /* save stack pointer registger, switch stack to sram */
    sp_backup = save_sp();
    /* copy mdfs info to sram space */
    mdfs_memcpy(&mdfs_info, mdfs, sizeof(struct aw_mdfs_info));
    /* start mdfs */
    mdfs_start(&mdfs_info);
    /* restore stack pointer */
    restore_sp(sp_backup);
    MDFS_DBG("%s done\n", __func__);

    return 0;
}
