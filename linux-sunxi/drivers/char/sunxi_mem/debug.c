/*
 * drivers/char/sunxi_mem/debug.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi physical memory allocator driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/sunxi_physmem.h>
#include <mach/memory.h>
#include <asm/setup.h>
#include "sunxi_physmem_i.h"

#ifdef CONFIG_PROC_FS
extern struct tag_mem32 hwres_mem;
extern int sunxi_mem_get_memnode(u32 base_addr, struct mem_list * pnode);
static int sunmm_stats_show(struct seq_file *m, void *unused)
{
	u32 rest_total = hwres_mem.start, largest_size = 0;
	struct mem_list node;
	int list_index, i;

	seq_printf(m, "sunxi memory layout: (+: used, -: free)\n");
	while(rest_total < hwres_mem.start + hwres_mem.size) {
		list_index = sunxi_mem_get_memnode(rest_total, &node);
		if(list_index < 0) {
			WARN(1, "cannot find list node for addr 0x%08x, BUG!\n", rest_total);
			continue;
		}
		for(i = 0; i < node.size / SZ_64K; i++)
			seq_printf(m, "%s", (list_index==0 ? "-" : "+"));
		rest_total += node.size;
	}
	seq_printf(m, "\n");
	rest_total = sunxi_mem_get_rest_size();
	largest_size = sunxi_get_largest_free();
	rest_total /= (1024*1024);
	largest_size /= (1024*1024);
	seq_printf(m, "sunxi memory: total free size %dM, largest free block %dM\n", rest_total, largest_size);
	return 0;
}

static int sunmm_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunmm_stats_show, NULL);
}

static const struct file_operations sunmm_dbg_fops = {
	.owner = THIS_MODULE,
	.open = sunmm_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init sunmm_dbg_init(void)
{
	proc_create("sunxi_mem", S_IRUGO, NULL, &sunmm_dbg_fops);
	return 0;
}

static void  __exit sunmm_dbg_exit(void)
{
	remove_proc_entry("sunxi_mem", NULL);
}

core_initcall(sunmm_dbg_init);
module_exit(sunmm_dbg_exit);
#endif

