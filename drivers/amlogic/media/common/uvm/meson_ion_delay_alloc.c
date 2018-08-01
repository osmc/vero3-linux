/*
 * drivers/amlogic/media/common/uvm/meson_ion_delay_alloc.c
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
#include <ion/ion.h>
#include <ion/ion_priv.h>
#include <linux/meson_ion.h>

#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/video_sink/v4lvideo_ext.h>

#include "meson_ion_delay_alloc.h"
#include "meson_uvm_conf.h"

static struct uvm_device *uvm_dev;
static struct dma_buf_ops uvm_dma_buf_ops;

static int enable_screencap;
module_param_named(enable_screencap, enable_screencap, int, 0664);

static int meson_uvm_alloc_buffer(struct dma_buf *dmabuf)
{
	int num_pages;
	struct ion_handle *handle;
	phys_addr_t pat;
	size_t len;
	struct sg_table *sgt;
	struct vframe_s *vf;
	enum ion_heap_type heap_type;
	struct file_private_data *file_private_data;

	struct uvm_buffer *buffer = dmabuf->priv;

	/* use ion_alloc to alloc the buffer */
	num_pages = PAGE_ALIGN(buffer->size) / PAGE_SIZE;

	file_private_data = buffer->file_private_data;
	if (file_private_data->flag & V4LVIDEO_FLAG_DI_NR)
		vf = &file_private_data->vf_ext;
	else
		vf = &file_private_data->vf;

	if (vf && (vf->type & VIDTYPE_COMPRESS))
		heap_type = ION_HEAP_TYPE_SYSTEM;
	else
		heap_type = ION_HEAP_TYPE_CUSTOM;

	pr_debug("num_pages: %d.\n", num_pages);
	handle = ion_alloc(uvm_dev->uvm_client, buffer->size, 0,
						(1 << heap_type), 0);
	if (IS_ERR(handle)) {
		pr_err("%s: ion_alloc fail.\n", __func__);
		return -1;
	}

	ion_phys(uvm_dev->uvm_client, handle,
			(ion_phys_addr_t *)&pat, &len);
	buffer->handle = handle;
	buffer->paddr = pat;
	sgt = handle->buffer->sg_table;
	dma_sync_sg_for_device(uvm_dev->pdev,
			       sgt->sgl, sgt->nents, DMA_BIDIRECTIONAL);

	return 0;
}

static int meson_uvm_map_buffer(struct dma_buf *dmabuf)
{
	int ret, i, j, num_pages;
	pgprot_t pgprot;
	void *vaddr;

	struct page **tmp;
	dma_addr_t paddr = 0;
	struct scatterlist *sg = NULL;
	struct sg_table *src_sgt = NULL;
	struct scatterlist *src_sgl = NULL;
	struct sg_table *dst_sgt = NULL;
	struct scatterlist *dst_sgl = NULL;
	struct uvm_buffer *buffer = dmabuf->priv;
	struct ion_handle *handle = buffer->handle;

	if (handle) {
		src_sgt = handle->buffer->sg_table;
		num_pages = PAGE_ALIGN(buffer->size) / PAGE_SIZE;

		/* map the new allocated buffer */
		dst_sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
		if (!dst_sgt) {
			ret = -ENOMEM;
			return ret;
		}

		ret = sg_alloc_table(dst_sgt, src_sgt->nents, GFP_KERNEL);
		if (ret) {
			kfree(dst_sgt);
			return -ENOMEM;
		}

		dst_sgl = dst_sgt->sgl;
		src_sgl = src_sgt->sgl;

		for (i = 0; i < src_sgt->nents; i++) {
			paddr = page_to_phys(sg_page(src_sgl));
			pr_debug("%d, %pa, %u.\n", i, &paddr, src_sgl->length);
			sg_set_page(dst_sgl, sg_page(src_sgl),
				src_sgl->length, 0);
			dst_sgl = sg_next(dst_sgl);
			src_sgl = sg_next(src_sgl);
		}

		buffer->sgt = dst_sgt;

		buffer->pages = vmalloc(sizeof(struct page *) * num_pages);
		if (!buffer->pages) {
			kfree(dst_sgt);
			ret = -ENOMEM;
			return ret;
		}

		pgprot = pgprot_writecombine(PAGE_KERNEL);
		tmp = buffer->pages;
		for_each_sg(dst_sgt->sgl, sg, dst_sgt->nents, i) {
			int npages_this_entry =
				PAGE_ALIGN(sg->length) / PAGE_SIZE;
			struct page *page = sg_page(sg);

			for (j = 0; j < npages_this_entry; j++)
				*(tmp++) = page++;
		}

		vaddr = vmap(buffer->pages, num_pages, VM_MAP, pgprot);
		if (!vaddr) {
			pr_err("vmap fail, size: %d\n",
					num_pages << PAGE_SHIFT);
			kfree(dst_sgt);
			return -ENOMEM;
		}
		vfree(buffer->pages);
		pr_debug("buffer vaddr: %p.\n", vaddr);
		buffer->vaddr = vaddr;
	}
	return 0;
}

static int meson_uvm_fill_pattern(struct dma_buf *dmabuf)
{
	struct v4l_data_t val_data;
	struct uvm_buffer *buffer = dmabuf->priv;
	struct vframe_s *vf;
	struct file_private_data *file_private_data;

	file_private_data = buffer->file_private_data;

	val_data.file_private_data = file_private_data;
	val_data.dst_addr = buffer->vaddr;
	val_data.byte_stride = buffer->byte_stride;
	val_data.width = buffer->width;
	val_data.height = buffer->height;
	val_data.phy_addr[0] = buffer->paddr;

	pr_debug("the phy addr is %pa.\n", &val_data.phy_addr[0]);

	if (file_private_data->flag & V4LVIDEO_FLAG_DI_NR)
		vf = &file_private_data->vf_ext;
	else
		vf = &file_private_data->vf;

	if (!buffer->index || buffer->index != vf->omx_index) {
		v4lvideo_data_copy(&val_data, NULL);
		buffer->index = vf->omx_index;
	}

	vunmap(buffer->vaddr);
	return 0;
}

static int meson_uvm_attach(struct dma_buf *dmabuf, struct device *dev,
					struct dma_buf_attachment *attach)
{
	if (!strstr(dev_name(dev), "bifrost") &&
		!strstr(dev_name(dev), "mali")) {
		pr_err("non gpu device should not be attached.\n");
		return -ENODEV;
	}

	return 0;
}

static void meson_uvm_detach(struct dma_buf *dmabuf,
				struct dma_buf_attachment *attach)
{
	pr_debug("meson_uvm_detach called, %s.\n", current->comm);
	/* TODO */
}

static struct sg_table *meson_uvm_map_dma_buf(
			struct dma_buf_attachment *attachment,
			enum dma_data_direction direction)
{
	struct dma_buf *dmabuf;
	struct uvm_buffer *buffer;
	struct uvm_device *ud;
	struct sg_table *sgt;

	dmabuf = attachment->dmabuf;
	buffer = dmabuf->priv;
	ud = buffer->dev;

	pr_debug("meson_uvm_map_dma_buf called, %s.\n", current->comm);

	if (!enable_screencap && current->tgid == ud->pid &&
	    buffer->commit_display) {
		pr_err("screen cap should not access the uvm buffer.\n");
		return ERR_PTR(-ENODEV);
	}

	if (!buffer->handle && meson_uvm_alloc_buffer(dmabuf)) {
		pr_err("uvm_map_dma_buf fail.\n");
		return ERR_PTR(-ENOMEM);
	}

	meson_uvm_map_buffer(dmabuf);
	meson_uvm_fill_pattern(dmabuf);

	sgt = buffer->sgt;
	if (!dma_map_sg(attachment->dev, sgt->sgl, sgt->nents, direction)) {
		pr_err("meson_uvm: dma_map_sg call failed.\n");
		sgt = ERR_PTR(-ENOMEM);
	}
	dma_sync_sg_for_device(uvm_dev->pdev,
			       sgt->sgl, sgt->nents, DMA_BIDIRECTIONAL);
	return sgt;
}

static void meson_uvm_unmap_dma_buf(struct dma_buf_attachment *attachment,
		struct sg_table *sgt, enum dma_data_direction direction)
{
	struct dma_buf *dmabuf;
	struct uvm_buffer *buffer;

	dmabuf = attachment->dmabuf;
	buffer = dmabuf->priv;

	pr_debug("meson_uvm_unmap_dma_buf called, %s.\n", current->comm);
	dma_unmap_sg(attachment->dev, sgt->sgl, sgt->nents, direction);

}

static void meson_uvm_release(struct dma_buf *dmabuf)
{
	struct uvm_buffer *buffer = dmabuf->priv;

	pr_debug("meson_uvm_release called.\n");

	if (buffer->handle) {
		ion_free(uvm_dev->uvm_client, buffer->handle);
		sg_free_table(buffer->sgt);
		kfree(buffer->sgt);
	}

	kfree(buffer);
}

static int meson_uvm_begin_cpu_access(struct dma_buf *dmabuf,
					enum dma_data_direction dir)
{
	pr_debug("meson_uvm_begin_cpu_access called.\n");
	return 0;
}

static int meson_uvm_end_cpu_access(struct dma_buf *dmabuf,
					enum dma_data_direction dir)
{
	pr_debug("meson_uvm_end_cpu_access called.\n");
	return 0;
}

static void *meson_uvm_kmap(struct dma_buf *dmabuf, unsigned long offset)
{
	pr_debug("meson_uvm_kmap called.\n");
	return NULL;
}

static void meson_uvm_kunmap(struct dma_buf *dmabuf,
		unsigned long offset, void *ptr)
{
	pr_debug("meson_uvm_kunmap called.\n");
	/* TODO */
}

static void *meson_uvm_kmap_atomic(struct dma_buf *dmabuf,
		unsigned long offset)
{
	pr_debug("meson_uvm_kmap_atomic called.\n");
	return NULL;
}

static void meson_uvm_kunmap_atomic(struct dma_buf *dmabuf,
		unsigned long offset, void *ptr)
{
	pr_debug("meson_uvm_kunmap_atomic called.\n");
	/* TODO */
}

static void *meson_uvm_vmap(struct dma_buf *dmabuf)
{
	pr_debug("meson_uvm_vmap called.\n");
	return NULL;
}

static void meson_uvm_vunmap(struct dma_buf *dmabuf, void *vaddr)
{
	pr_debug("meson_uvm_vunmap called.\n");
	/* TODO */
}

static int meson_uvm_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	int i;
	int ret;

	struct uvm_buffer *buffer = dmabuf->priv;
	struct sg_table *table = buffer->sgt;
	unsigned long addr = vma->vm_start;
	unsigned long offset = vma->vm_pgoff * PAGE_SIZE;
	struct scatterlist *sg;

	pr_debug("meson_uvm_mmap called.\n");

	if (!buffer->sgt) {
		pr_err("buffer was not allocated.\n");
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

static struct dma_buf_ops uvm_dma_buf_ops = {
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

static struct dma_buf *uvm_dmabuf_export(struct uvm_buffer *buffer)
{
	struct dma_buf *dmabuf;

	struct dma_buf_export_info exp_info = {
		.exp_name = KBUILD_MODNAME,
		.owner = THIS_MODULE,
		.ops = &uvm_dma_buf_ops,
		.size = buffer->size,
		.flags = buffer->flags,
		.priv = buffer,
	};

	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		pr_err("uvm_dmabuf_export fail.\n");
		return ERR_PTR(-ENOMEM);
	}

	return dmabuf;
}
static int uvm_alloc_buffer(struct uvm_alloc_data *uad)
{
	struct dma_buf *dmabuf;
	struct uvm_buffer *buffer;
	int fd;

	buffer = kzalloc(sizeof(struct uvm_buffer), GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	buffer->dev = uvm_dev;
	buffer->size = PAGE_ALIGN(uad->size);
	buffer->align = uad->align;
	buffer->flags = uad->flags;
	buffer->byte_stride = uad->byte_stride;
	buffer->width = uad->width;
	buffer->height = uad->height;

	dmabuf = uvm_dmabuf_export(buffer);
	if (IS_ERR(dmabuf))
		goto err;

	buffer->dmabuf = dmabuf;
	buffer->file_private_data = v4lvideo_get_vf(uad->v4l2_fd);
	if (!buffer->file_private_data)
		pr_err("v4lvideo_get_vf failed.\n");

	dmabuf->priv = buffer;

	fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	if (fd < 0) {
		dma_buf_put(dmabuf);
		goto err;
	}

	return fd;

err:
	kfree(buffer);
	return -ENOMEM;
}

int uvm_set_commit_display(int fd, int commit_display)
{
	struct dma_buf *dmabuf;
	struct uvm_buffer *buffer;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("invalid dmabuf fd.\n");
		return -EINVAL;
	}

	buffer = dmabuf->priv;
	buffer->commit_display = commit_display;

	dma_buf_put(dmabuf);
	return 0;
}

static long uvm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct uvm_device *ud;
	union uvm_ioctl_arg data;
	int pid;
	int ret = 0;
	int fd = 0;

	ud = file->private_data;

	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
		return -EFAULT;

	switch (cmd) {
	case UVM_IOC_ALLOC:
		fd = uvm_alloc_buffer(&data.alloc_data);
		if (fd < 0)
			return -ENOMEM;

		data.alloc_data.fd = fd;

		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)))
			return -EFAULT;

		break;
	case UVM_IOC_SET_PID:
		pid = data.pid_data.pid;
		if (pid < 0)
			return -ENOMEM;

		ud->pid = pid;
		break;
	case UVM_IOC_SET_FD:
		fd = data.fd_data.fd;
		ret = uvm_set_commit_display(fd, data.fd_data.commit_display);

		if (ret < 0) {
			pr_err("invalid dambuf fd.\n");
			return -EINVAL;
		}
		break;
	default:
		return -ENOTTY;
	}

	return ret;

}

static int uvm_open(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct uvm_device *ud = container_of(miscdev, struct uvm_device, dev);

	pr_debug("%s: %d\n", __func__, __LINE__);
	file->private_data = ud;

	return 0;
}

static int uvm_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations uvm_fops = {
	.owner = THIS_MODULE,
	.open = uvm_open,
	.release = uvm_release,
	.unlocked_ioctl = uvm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = uvm_ioctl,
#endif
};

static int meson_uvm_probe(struct platform_device *pdev)
{
	uvm_dev = kzalloc(sizeof(struct uvm_device), GFP_KERNEL);
	if (!uvm_dev) {
		pr_err("malloc uvm_device fail.\n");
		return -ENOMEM;
	}

	uvm_dev->dev.minor = MISC_DYNAMIC_MINOR;
	uvm_dev->dev.name = "uvm";
	uvm_dev->dev.fops = &uvm_fops;
	uvm_dev->pdev = &pdev->dev;
	mutex_init(&uvm_dev->buffer_lock);

	uvm_dev->uvm_client = meson_ion_client_create(-1, "meson-uvm");
	if (!uvm_dev->uvm_client) {
		pr_err("ion client create error.\n");
		goto err;
	}

	return misc_register(&uvm_dev->dev);

err:
	kfree(uvm_dev);
	return -ENOMEM;
}

static int meson_uvm_remove(struct platform_device *pdev)
{
	misc_deregister(&uvm_dev->dev);
	return 0;
}

static const struct of_device_id meson_uvm_match[] = {
	{.compatible = "amlogic, meson_uvm"},
	{},
};

static struct platform_driver meson_uvm_driver = {
	.driver = {
		.name = "meson_uvm_dirver",
		.owner = THIS_MODULE,
		.of_match_table = meson_uvm_match,
	},
	.probe = meson_uvm_probe,
	.remove = meson_uvm_remove,
};

static int __init meson_uvm_init(void)
{
	if (!use_uvm) {
		pr_info("meson_ion_delay_alloc call init\n");
		return platform_driver_register(&meson_uvm_driver);
	}

	return 0;
}

static void __exit meson_uvm_exit(void)
{
	if (!use_uvm)
		platform_driver_unregister(&meson_uvm_driver);
}

module_init(meson_uvm_init);
module_exit(meson_uvm_exit);
MODULE_DESCRIPTION("AMLOGIC unified video memory management driver");
MODULE_LICENSE("GPL");
