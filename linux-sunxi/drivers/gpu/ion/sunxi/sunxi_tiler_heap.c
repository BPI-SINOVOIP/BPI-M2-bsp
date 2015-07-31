/*
 * drivers/gpu/ion/sunxi_tiler_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/spinlock.h>

#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/sunxi_ion.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/mach/map.h>
#include <asm/page.h>
#include <linux/module.h>

#include "../ion_priv.h"

static int sunxi_tiler_heap_allocate(struct ion_heap *heap,
				    struct ion_buffer *buffer,
				    unsigned long size, unsigned long align,
				    unsigned long flags)
{
	if (size == 0)
		return 0;

	pr_err("%s: This should never be called directly -- use the "
	       "OMAP_ION_TILER_ALLOC flag to the ION_IOC_CUSTOM "
	       "instead\n", __func__);
	return -EINVAL;
}

struct sunxi_tiler_info 
{
	bool lump;			/* true for a single lump allocation */
	u32 n_phys_pages;		/* number of physical pages */
	u32 *phys_addrs;		/* array addrs of pages */
};

int sunxi_tiler_alloc(struct ion_heap *heap,
		     struct ion_client *client,
		     struct sunxi_ion_tiler_alloc_data *data)
{
	struct ion_handle *handle;
	struct ion_buffer *buffer;
	struct sunxi_tiler_info *info;
	u32 n_phys_pages;
	ion_phys_addr_t addr;
	int i, ret;
    u32 size_temp = 0;
    u32 line_stride;
    u32 align_w;

	if (data->fmt == TILER_PIXEL_FMT_PAGE && data->h != 1) {
		pr_err("%s: Page mode (1D) allocations must have a height "
		       "of one\n", __func__);
		return -EINVAL;
	}

    align_w = (((data->w) + (32) - 1L) & ~((32) - 1L));
    
	if(data->fmt == TILER_PIXEL_FMT_16BIT)
	{
	    size_temp = align_w * data->h * 2;
        line_stride = align_w * 2;
	}
    else
    {
        size_temp = align_w * data->h * 4;
        line_stride = align_w * 4;
    }
    //printk("%s, line %d, data->w %d, data->h %d,data->fmt %d\n", __func__, __LINE__, data->w, data->h,data->fmt);
    n_phys_pages = ((size_temp) + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
    //printk("%s, line %d, n_phys_pages %d\n", __func__, __LINE__, n_phys_pages);
	info = kzalloc(sizeof(struct sunxi_tiler_info) +
		       sizeof(u32) * n_phys_pages, GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->n_phys_pages = n_phys_pages;
	info->phys_addrs = (u32 *)(info + 1);
    
	addr = ion_carveout_allocate(heap, n_phys_pages*PAGE_SIZE, 0);
    //printk("%s, line %d, n_phys_pages %d, addr %x\n", __func__, __LINE__, n_phys_pages, addr);
	if (addr == ION_CARVEOUT_ALLOCATE_FAIL) 
    {
		for (i = 0; i < n_phys_pages; i++) 
        {
			addr = ion_carveout_allocate(heap, PAGE_SIZE, 0);

			if (addr == ION_CARVEOUT_ALLOCATE_FAIL) 
            {
				ret = -ENOMEM;
				pr_err("%s: failed to allocate pages to back "
					"tiler address space\n", __func__);
				goto err;
			}
			info->phys_addrs[i] = addr;
		}
	} 
    else 
    {
		info->lump = true;
		for (i = 0; i < n_phys_pages; i++)
		{
			info->phys_addrs[i] = addr + i*PAGE_SIZE;
		}
	}

	data->stride = line_stride;

	/* create an ion handle  for the allocation */
	handle = ion_alloc(client, 0, 0, 1 << SUNXI_ION_HEAP_TYPE_TILER);
	if (IS_ERR_OR_NULL(handle)) 
    {
		ret = PTR_ERR(handle);
		pr_err("%s: failure to allocate handle to manage tiler"
		       " allocation\n", __func__);
		goto err;
	}

	buffer = ion_handle_buffer(handle);
	buffer->size = info->n_phys_pages * PAGE_SIZE;
	buffer->priv_virt = info;
	data->handle = handle;
	return 0;
    
err:
	if (info->lump)
		ion_carveout_free(heap, addr, n_phys_pages * PAGE_SIZE);
	else
		for (i -= 1; i >= 0; i--)
			ion_carveout_free(heap, info->phys_addrs[i], PAGE_SIZE);
	kfree(info);
	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_tiler_alloc);

void sunxi_tiler_heap_free(struct ion_buffer *buffer)
{
	struct sunxi_tiler_info *info = buffer->priv_virt;

    //printk("%s, line %d, buffer 0x%08x\n", __func__, __LINE__, (u32)buffer);
	if (info->lump) 
    {
		ion_carveout_free(buffer->heap, info->phys_addrs[0],
				  info->n_phys_pages*PAGE_SIZE);
	} 
    else 
    {
		int i;
		for (i = 0; i < info->n_phys_pages; i++)
			ion_carveout_free(buffer->heap,
					  info->phys_addrs[i], PAGE_SIZE);
	}

	kfree(info);
}
EXPORT_SYMBOL_GPL(sunxi_tiler_heap_free);

static int sunxi_tiler_phys(struct ion_heap *heap,
			   struct ion_buffer *buffer,
			   ion_phys_addr_t *addr, size_t *len)
{
	struct sunxi_tiler_info *info = buffer->priv_virt;

	*addr = info->phys_addrs[0];
	*len = buffer->size;
	return 0;
}

int sunxi_tiler_pages(struct ion_client *client, struct ion_handle *handle,
		     int *n, u32 **tiler_addrs)
{
	ion_phys_addr_t addr;
	size_t len;
	int ret;
	struct sunxi_tiler_info *info = ion_handle_buffer(handle)->priv_virt;

	/* validate that the handle exists in this client */
	ret = ion_phys(client, handle, &addr, &len);
	if (ret)
		return ret;

	*n = info->n_phys_pages;
	*tiler_addrs = info->phys_addrs;
	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_tiler_pages);

int sunxi_tiler_heap_map_user(struct ion_heap *heap, struct ion_buffer *buffer,
			     struct vm_area_struct *vma)
{
	struct sunxi_tiler_info *info = buffer->priv_virt;
	unsigned long addr = vma->vm_start;
	u32 vma_pages = (vma->vm_end - vma->vm_start) / PAGE_SIZE;
	int n_pages = min(vma_pages, info->n_phys_pages);
	int i, ret;

	for (i = vma->vm_pgoff; i < n_pages; i++, addr += PAGE_SIZE) {
		ret = remap_pfn_range(vma, addr,
				      __phys_to_pfn(info->phys_addrs[i]),
				      PAGE_SIZE,
				      pgprot_noncached(vma->vm_page_prot));
		if (ret)
			return ret;
	}
	return 0;
}

static struct ion_heap_ops sunxi_tiler_ops = {
	.allocate = sunxi_tiler_heap_allocate,
	.free = sunxi_tiler_heap_free,
	.phys = sunxi_tiler_phys,
	.map_user = sunxi_tiler_heap_map_user,
};

struct ion_heap *sunxi_tiler_heap_create(struct ion_platform_heap *data)
{
	struct ion_heap *heap;

	heap = ion_carveout_heap_create(data);
	if (!heap)
		return ERR_PTR(-ENOMEM);
	heap->ops = &sunxi_tiler_ops;
	heap->type = SUNXI_ION_HEAP_TYPE_TILER;
	heap->name = data->name;
	heap->id = data->id;
	return heap;
}

void sunxi_tiler_heap_destroy(struct ion_heap *heap)
{
	kfree(heap);
}

