/*
 * drivers/amlogic/media/common/uvm/meson_uvm_core.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/pagemap.h>
#include <linux/amlogic/meson_uvm_core.h>

static int uvm_debug_level;
module_param(uvm_debug_level, int, 0644);

#define UVM_PRINTK(level, fmt, arg...) \
	do {	\
		if (uvm_debug_level >= (level))	\
			pr_info("UVM: " fmt, ## arg); \
	} while (0)

static void uvm_hook_mod_release(struct kref *kref);
static struct uvm_hook_mod *uvm_find_hook_mod(struct uvm_handle *handle,
					      int type);

static void uvm_handle_destroy(struct kref *kref)
{
	struct uvm_handle *handle;

	handle = container_of(kref, struct uvm_handle, ref);

	kfree(handle->ua);

	kfree(handle);
}

static int meson_uvm_attach(struct dma_buf *dmabuf, struct device *dev,
			    struct dma_buf_attachment *attach)
{
	UVM_PRINTK(2, "%s called, %s.\n", __func__, current->comm);
	return 0;
}

static void meson_uvm_detach(struct dma_buf *dmabuf,
			     struct dma_buf_attachment *attach)
{
	UVM_PRINTK(2, "%s called, %s.\n", __func__, current->comm);
	/* TODO */
}

static struct sg_table *meson_uvm_map_dma_buf(
			struct dma_buf_attachment *attachment,
			enum dma_data_direction direction)
{
	struct dma_buf *dmabuf;
	struct uvm_handle *handle;
	struct uvm_alloc *ua;
	struct sg_table *sgt;
	bool gpu_access = false;
	u64 flag = 0;

	if (strstr(dev_name(attachment->dev), "bifrost") ||
		strstr(dev_name(attachment->dev), "mali")) {
		gpu_access = true;
	}

	dmabuf = attachment->dmabuf;
	handle = dmabuf->priv;
	ua = handle->ua;

	UVM_PRINTK(1, "%s called, %s. gpu_access:%d ua->flags:%llu dmabuf:%p\n",
			__func__, current->comm, gpu_access, ua->flags, dmabuf);

	if (ua->flags & BIT(UVM_DELAY_ALLOC) && gpu_access) {
		if (ua->flags & BIT(UVM_SKIP_REALLOC))
			return ERR_PTR(-ENOMEM);

		if (ua->delay_alloc(dmabuf, ua->obj, &flag)) {
			ua->flags |= flag;
			UVM_PRINTK(0, "delay_realloc fail. ua->flags:%llu\n", ua->flags);
			return ERR_PTR(-ENOMEM);
		}
	}

	if (ua->flags & BIT(UVM_IMM_ALLOC) && gpu_access && ua->scalar > 1) {
		UVM_PRINTK(1, "begin ua->gpu_realloc. size: %zu scalar: %d\n",
					ua->size, ua->scalar);
		if (ua->gpu_realloc(dmabuf, ua->obj, ua->scalar)) {
			UVM_PRINTK(0, "gpu_realloc fail\n");
			return ERR_PTR(-ENOMEM);
		}
	}

	sgt = ua->sgt;
	if (!sgt) {
		UVM_PRINTK(0, "meson_uvm: null sgt.\n");
		return ERR_PTR(-ENOMEM);
	}

	if (ua->flags & BIT(UVM_SECURE_ALLOC))
		return sgt;

	if (ua->flags & BIT(UVM_FAKE_ALLOC)) {
		dma_map_page(attachment->dev, sg_page(sgt->sgl), 0,
					UVM_FAKE_SIZE, direction);
	} else if (!dma_map_sg(attachment->dev, sgt->sgl, sgt->nents,
				direction)) {
		UVM_PRINTK(0, "meson_uvm: dma_map_sg call failed.\n");
		return ERR_PTR(-ENOMEM);
	}
	dma_sync_sg_for_device(attachment->dev, sgt->sgl,
			       sgt->nents, DMA_BIDIRECTIONAL);
	return sgt;
}

static void meson_uvm_unmap_dma_buf(struct dma_buf_attachment *attachment,
				    struct sg_table *sgt,
				    enum dma_data_direction direction)
{
	struct dma_buf *dmabuf;
	struct uvm_handle *handle;
	struct uvm_alloc *ua;

	if (!attachment || !sgt)
		return;

	dmabuf = attachment->dmabuf;
	handle = dmabuf->priv;
	ua = handle->ua;

	UVM_PRINTK(1, "%s called, %s. sgt:%p dmabuf:%p handle:%p ua:%p\n",
				__func__, current->comm, sgt, dmabuf, handle, ua);

	if (ua->flags & (BIT(UVM_SECURE_ALLOC) | BIT(UVM_SKIP_REALLOC)))
		return;

	if (ua->flags & BIT(UVM_FAKE_ALLOC))
		dma_unmap_page(attachment->dev, sgt->sgl->dma_address,
			       UVM_FAKE_SIZE, direction);
	else
		dma_unmap_sg(attachment->dev, sgt->sgl, sgt->nents, direction);
}

static void meson_uvm_release(struct dma_buf *dmabuf)
{
	struct uvm_hook_mod *uhmod, *uhtmp;

	struct uvm_handle *handle = dmabuf->priv;
	struct uvm_alloc *ua = handle->ua;

	UVM_PRINTK(2, "%s called, %s.\n", __func__, current->comm);

	if (ua->free)
		ua->free(ua->obj);

	list_for_each_entry_safe(uhmod, uhtmp, &handle->mod_attached, list)
		kref_put(&uhmod->ref, uvm_hook_mod_release);

	kref_put(&handle->ref, uvm_handle_destroy);
}

static int meson_uvm_begin_cpu_access(struct dma_buf *dmabuf,
				      enum dma_data_direction dir)
{
	UVM_PRINTK(2, "%s called.\n", __func__);
	return 0;
}

static int meson_uvm_end_cpu_access(struct dma_buf *dmabuf,
				    enum dma_data_direction dir)
{
	UVM_PRINTK(2, "%s called.\n", __func__);
	return 0;
}

static void *meson_uvm_kmap(struct dma_buf *dmabuf, unsigned long offset)
{
	UVM_PRINTK(2, "%s called.\n", __func__);
	return NULL;
}

static void meson_uvm_kunmap(struct dma_buf *dmabuf,
			     unsigned long offset, void *ptr)
{
	UVM_PRINTK(2, "%s called.\n", __func__);
	/* TODO */
}

static void *meson_uvm_kmap_atomic(struct dma_buf *dmabuf,
				   unsigned long offset)
{
	UVM_PRINTK(2, "%s called.\n", __func__);
	return NULL;
}

static void meson_uvm_kunmap_atomic(struct dma_buf *dmabuf,
				    unsigned long offset, void *ptr)
{
	UVM_PRINTK(2, "%s called.\n", __func__);
	/* TODO */
}

static void *meson_uvm_vmap(struct dma_buf *dmabuf)
{
	struct uvm_handle *handle;
	struct sg_table *sgt;
	struct scatterlist *sg;
	struct page **pages;
	struct page **tmp;
	int npages, i, j;
	pgprot_t pgprot;
	void *vaddr;

	handle = dmabuf->priv;
	sgt = handle->ua->sgt;
	npages = PAGE_ALIGN(handle->size) / PAGE_SIZE;
	pgprot = pgprot_writecombine(PAGE_KERNEL);

	pages = vmalloc(sizeof(struct page *) * npages);
	if (!pages)
		return NULL;

	tmp = pages;
	for_each_sg(sgt->sgl, sg, sgt->nents, i) {
		int npages_this_entry = PAGE_ALIGN(sg->length) / PAGE_SIZE;
		struct page *page = sg_page(sg);

		WARN_ON(i >= npages);
		for (j = 0; j < npages_this_entry; j++)
			*(tmp++) = page++;
	}
	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	vfree(pages);

	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	handle->ua->vaddr = vaddr;
	UVM_PRINTK(2, "%s called.\n", __func__);
	return vaddr;
}

static void meson_uvm_vunmap(struct dma_buf *dmabuf, void *vaddr)
{
	struct uvm_handle *handle = dmabuf->priv;

	UVM_PRINTK(2, "%s called.\n", __func__);
	vunmap(handle->ua->vaddr);
}

static int meson_uvm_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	int i;
	int ret;
	struct scatterlist *sg;
	struct uvm_handle *handle = dmabuf->priv;
	struct sg_table *table = handle->ua->sgt;
	unsigned long addr = vma->vm_start;
	unsigned long offset = vma->vm_pgoff * PAGE_SIZE;

	UVM_PRINTK(2, "%s called.\n", __func__);

	if (!table) {
		UVM_PRINTK(0, "buffer was not allocated.\n");
		return -EINVAL;
	}

	for_each_sg(table->sgl, sg, table->nents, i) {
		struct page *page = sg_page(sg);
		unsigned long remainder = vma->vm_end - addr;
		unsigned long len = sg->length;

		if (offset >= sg->length) {
			offset -= sg->length;
			continue;
		} else if (offset) {
			page += offset / PAGE_SIZE;
			len = sg->length - offset;
			offset = 0;
		}
		len = min(len, remainder);
		ret = remap_pfn_range(vma, addr, page_to_pfn(page), len,
				      vma->vm_page_prot);
		if (ret)
			return ret;
		addr += len;
		if (addr >= vma->vm_end)
			return 0;
	}

	return 0;
}

static struct dma_buf_ops meson_uvm_dma_ops = {
	.attach = meson_uvm_attach,
	.detach = meson_uvm_detach,
	.map_dma_buf = meson_uvm_map_dma_buf,
	.unmap_dma_buf = meson_uvm_unmap_dma_buf,
	.release = meson_uvm_release,
	.begin_cpu_access = meson_uvm_begin_cpu_access,
	.end_cpu_access = meson_uvm_end_cpu_access,
	.kmap = meson_uvm_kmap,
	.kmap_atomic = meson_uvm_kmap_atomic,
	.kunmap = meson_uvm_kunmap,
	.kunmap_atomic = meson_uvm_kunmap_atomic,
	.vmap = meson_uvm_vmap,
	.vunmap = meson_uvm_vunmap,
	.mmap = meson_uvm_mmap,
};

static struct uvm_handle *uvm_handle_alloc(size_t len, size_t align,
					   ulong flags)
{
	struct uvm_handle *handle;
	struct uvm_alloc *ua;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return ERR_PTR(-ENOMEM);

	kref_init(&handle->ref);
	mutex_init(&handle->lock);
	handle->size = len;
	handle->align = align;
	handle->flags = flags;
	INIT_LIST_HEAD(&handle->mod_attached);

	ua = kzalloc(sizeof(*ua), GFP_KERNEL);
	if (!ua) {
		kfree(handle);
		return ERR_PTR(-ENOMEM);
	}
	handle->ua = ua;

	return handle;
}

static struct dma_buf *uvm_dmabuf_export(struct uvm_handle *handle)
{
	struct dma_buf *dmabuf;

	struct dma_buf_export_info exp_info = {
		.exp_name = KBUILD_MODNAME,
		.owner = THIS_MODULE,
		.ops = &meson_uvm_dma_ops,
		.size = handle->size,
		.flags = O_RDWR,
		.priv = handle,
	};

	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		UVM_PRINTK(0, "dma_buf_export fail.\n");
		return ERR_PTR(-ENOMEM);
	}

	handle->dmabuf = dmabuf;

	return dmabuf;
}

struct dma_buf *uvm_alloc_dmabuf(size_t len, size_t align, ulong flags)
{
	struct uvm_handle *handle;
	struct dma_buf *dmabuf;

	handle = uvm_handle_alloc(len, align, flags);
	if (IS_ERR(handle))
		return ERR_PTR(-ENOMEM);

	dmabuf = uvm_dmabuf_export(handle);
	if (IS_ERR(dmabuf)) {
		kfree(handle);
		return ERR_PTR(-EINVAL);
	}

	return dmabuf;
}
EXPORT_SYMBOL(uvm_alloc_dmabuf);

bool dmabuf_is_uvm(struct dma_buf *dmabuf)
{
	return dmabuf->ops == &meson_uvm_dma_ops;
}
EXPORT_SYMBOL(dmabuf_is_uvm);

struct uvm_buf_obj *dmabuf_get_uvm_buf_obj(struct dma_buf *dmabuf)
{
	struct uvm_handle *handle;

	handle = dmabuf->priv;

	return handle->ua->obj;
}
EXPORT_SYMBOL(dmabuf_get_uvm_buf_obj);

static struct sg_table *uvm_copy_sgt(struct sg_table *src_table)
{
	struct sg_table *dst_table = NULL;
	struct scatterlist *dst_sgl = NULL;
	struct scatterlist *src_sgl = NULL;
	int i, ret;

	dst_table = kzalloc(sizeof(*dst_table), GFP_KERNEL);
	if (!dst_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(dst_table, src_table->nents, GFP_KERNEL);
	if (ret) {
		kfree(dst_table);
		return ERR_PTR(-ENOMEM);
	}

	dst_sgl = dst_table->sgl;
	src_sgl = src_table->sgl;
	for (i = 0; i < src_table->nents; i++) {
		sg_set_page(dst_sgl, sg_page(src_sgl), src_sgl->length, 0);
		sg_dma_address(dst_sgl) = sg_phys(src_sgl);
		sg_dma_len(dst_sgl) = sg_dma_len(src_sgl);
		dst_sgl = sg_next(dst_sgl);
		src_sgl = sg_next(src_sgl);
	}

	return dst_table;
}

int dmabuf_bind_uvm_alloc(struct dma_buf *dmabuf, struct uvm_alloc_info *info)
{
	struct uvm_handle *handle;
	struct uvm_alloc *ua;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return -EINVAL;
	}

	handle = dmabuf->priv;
	ua = handle->ua;

	ua->size = info->size;
	ua->flags = info->flags;
	ua->free = info->free;
	ua->scalar = info->scalar;
	ua->delay_alloc = info->delay_alloc;
	ua->gpu_realloc = info->gpu_realloc;

	if (info->obj) {
		ua->obj = info->obj;
		ua->obj->dmabuf = dmabuf;
	}

	if (info->sgt)
		ua->sgt = uvm_copy_sgt(info->sgt);

	return 0;
}
EXPORT_SYMBOL(dmabuf_bind_uvm_alloc);

int dmabuf_bind_uvm_delay_alloc(struct dma_buf *dmabuf,
				struct uvm_alloc_info *info)
{
	struct uvm_handle *handle;
	struct uvm_alloc *ua;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return -EINVAL;
	}

	handle = dmabuf->priv;
	ua = handle->ua;

	if (info->sgt)
		ua->sgt = uvm_copy_sgt(info->sgt);

	return 0;
}
EXPORT_SYMBOL(dmabuf_bind_uvm_delay_alloc);

int dmabuf_set_vframe(struct dma_buf *dmabuf, struct vframe_s *vf,
		      enum uvm_hook_mod_type type)
{
	struct uvm_handle *handle;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return -EINVAL;
	}

	handle = dmabuf->priv;
	memcpy(&handle->vf, vf, sizeof(struct vframe_s));
	handle->vfp = vf;

	handle->mod_attached_mask |= (1 << type);

	return 0;
}
EXPORT_SYMBOL(dmabuf_set_vframe);

struct vframe_s *dmabuf_get_vframe(struct dma_buf *dmabuf)
{
	struct uvm_handle *handle;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return ERR_PTR(-EINVAL);
	}

	handle = dmabuf->priv;
	kref_get(&handle->ref);

	return handle->vfp ? &handle->vf : NULL;
}
EXPORT_SYMBOL(dmabuf_get_vframe);

int dmabuf_put_vframe(struct dma_buf *dmabuf)
{
	struct uvm_handle *handle;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return -EINVAL;
	}

	handle = dmabuf->priv;
	return kref_put(&handle->ref, uvm_handle_destroy);
}
EXPORT_SYMBOL(dmabuf_put_vframe);

bool is_valid_mod_type(struct dma_buf *dmabuf,
		       enum uvm_hook_mod_type type)
{
	struct uvm_handle *handle;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(1, "dmabuf is not uvm.\n");
		return 0;
	}

	handle = dmabuf->priv;

	return (handle->mod_attached_mask & (1 << type)) ? 1 : 0;
}
EXPORT_SYMBOL(is_valid_mod_type);

int uvm_attach_hook_mod(struct dma_buf *dmabuf,
			struct uvm_hook_mod_info *info)
{
	struct uvm_handle *handle;
	struct uvm_hook_mod *uhmod;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return -EINVAL;
	}

	handle = dmabuf->priv;
	uhmod = kzalloc(sizeof(*uhmod), GFP_KERNEL);
	if (!uhmod) {
		UVM_PRINTK(0, "kzalloc uvm_hook_mod fail.\n");
		return -ENOMEM;
	}

	kref_init(&uhmod->ref);
	uhmod->type = info->type;
	uhmod->arg = info->arg;
	uhmod->free = info->free;
	uhmod->acquire_fence = info->acquire_fence;

	mutex_lock(&handle->lock);
	list_add_tail(&uhmod->list, &handle->mod_attached);
	handle->n_mod_attached++;
	handle->mod_attached_mask |= 1 << (uhmod->type);
	mutex_unlock(&handle->lock);

	UVM_PRINTK(2, "%s called.\n", __func__);

	return 0;
}
EXPORT_SYMBOL(uvm_attach_hook_mod);

int uvm_detach_hook_mod(struct dma_buf *dmabuf, int type)
{
	UVM_PRINTK(2, "%s called.\n", __func__);
	return uvm_put_hook_mod(dmabuf, type);
}
EXPORT_SYMBOL(uvm_detach_hook_mod);

static struct uvm_hook_mod *uvm_find_hook_mod(struct uvm_handle *handle,
					      int type)
{
	struct uvm_hook_mod *uhmod = NULL;

	mutex_lock(&handle->lock);
	if (!list_empty(&handle->mod_attached)) {
		list_for_each_entry(uhmod, &handle->mod_attached, list) {
			if (uhmod->type == type)
				break;
		}
	}
	mutex_unlock(&handle->lock);

	if (!uhmod) {
		UVM_PRINTK(1, "%s fail.\n", __func__);
		return NULL;
	}

	return uhmod;
}

struct uvm_hook_mod *uvm_get_hook_mod(struct dma_buf *dmabuf,
				      int type)
{
	struct uvm_handle *handle;
	struct uvm_hook_mod *uhmod = NULL;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return ERR_PTR(-EINVAL);
	}

	handle = dmabuf->priv;
	uhmod = uvm_find_hook_mod(handle, type);
	if (uhmod)
		kref_get(&uhmod->ref);

	UVM_PRINTK(2, "%s %px, %d.\n", __func__, uhmod, type);
	return uhmod;
}
EXPORT_SYMBOL(uvm_get_hook_mod);

static void uvm_hook_mod_release(struct kref *kref)
{
	struct uvm_hook_mod *uhmod;

	uhmod = container_of(kref, struct uvm_hook_mod, ref);
	list_del(&uhmod->list);
	UVM_PRINTK(2, "%s called, %px.\n", __func__, uhmod);
	uhmod->free(uhmod->arg);
}

int uvm_put_hook_mod(struct dma_buf *dmabuf, int type)
{
	struct uvm_handle *handle;
	struct uvm_hook_mod *uhmod = NULL;

	if (!dmabuf || !dmabuf_is_uvm(dmabuf)) {
		UVM_PRINTK(0, "dmabuf is not uvm.\n");
		return -EINVAL;
	}

	handle = dmabuf->priv;
	uhmod = uvm_find_hook_mod(handle, type);

	if (uhmod)
		return kref_put(&uhmod->ref, uvm_hook_mod_release);
	else
		return -EINVAL;
}
EXPORT_SYMBOL(uvm_put_hook_mod);
