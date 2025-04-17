#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "memexpand.h"

MODULE_LICENSE("GPL");

#define DEVICE_NAME "memexpand"

struct memexpand_device_data {
	struct cdev cdev;
	struct memexpand_mmap_info *mmap_info;
};

static struct memexpand_device_data memexpand;
static dev_t dev_num;
static struct class *my_class;

static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static long memexpand_ioctl(struct file *file, unsigned int cmd,
			    unsigned long arg);
static int memexpand_mmap(struct file *file, struct vm_area_struct *vma);

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.unlocked_ioctl = memexpand_ioctl,
	.mmap = memexpand_mmap,
};

static int __init my_init(void)
{
	if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0)
		return -ENOMEM;

	my_class = class_create("my_class");
	if (IS_ERR(my_class)) {
		unregister_chrdev_region(dev_num, 1);
		return PTR_ERR(my_class);
	}

	if (IS_ERR(device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME))) {
		class_destroy(my_class);
		unregister_chrdev_region(dev_num, 1);
		return -1;
	}

	cdev_init(&memexpand.cdev, &my_fops);
	if (cdev_add(&memexpand.cdev, dev_num, 1) < 0) {
		device_destroy(my_class, dev_num);
		class_destroy(my_class);
		unregister_chrdev_region(dev_num, 1);
		return -1;
	}

	memexpand.mmap_info = NULL;

	pr_info("Device has been inserted!\n");

	return 0;
}

static void __exit my_exit(void)
{
	if (memexpand.mmap_info) {
		if (memexpand.mmap_info->data)
			vfree(memexpand.mmap_info->data);
		kfree(memexpand.mmap_info);
	}

	cdev_del(&memexpand.cdev);
	device_destroy(my_class, dev_num);
	class_destroy(my_class);
	unregister_chrdev_region(dev_num, 1);
}

module_init(my_init);
module_exit(my_exit);

static long memexpand_ioctl(struct file *file, unsigned int cmd,
			    unsigned long arg)
{
	struct memexpand_data data;
	struct memexpand_device_data *dev_data = file->private_data;
	struct memexpand_mmap_info *info;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
		return -EFAULT;

	pr_info("Size requested: %lu\n", data.size);

	switch (cmd) {
	case MEMEXPAND_ALLOCATE:
		info = kzalloc(sizeof(struct memexpand_mmap_info), GFP_KERNEL);
		if (!info)
			return -ENOMEM;

		info->size = PAGE_ALIGN(data.size);
		if (info->size == 0) {
			kfree(info);
			return -EINVAL;
		}

		info->data = vzalloc(info->size);
		if (!info->data) {
			kfree(info);
			info = NULL;
			return -ENOMEM;
		}

		dev_data->mmap_info = info;

		pr_info("Allocated kernel memory at %p, size: %lu\n",
			info->data, info->size);

		data.size = info->size;

		if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
			vfree(info->data);
			kfree(info);
			dev_data->mmap_info = NULL;
			return -EFAULT;
		}

		break;

	default:
		pr_info("Unknown ioctl command: %u\n", cmd);
		return -ENOTTY;
	}

	return 0;
}

static vm_fault_t memexpand_vm_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct memexpand_mmap_info *info;
	unsigned long offset;
	struct page *page;

	info = vma->vm_private_data;
	if (!info) {
		pr_err("No mmap info in vma private data/n");
		return VM_FAULT_SIGBUS;
	}

	offset = vmf->pgoff << PAGE_SHIFT;
	if (offset >= info->size) {
		pr_err("Offset out of bouds: %lu >= %lu\n", offset, info->size);
		return VM_FAULT_SIGBUS;
	}

	page = vmalloc_to_page(info->data + offset);
	if (!page) {
		pr_err("Failed to get page for offset %lu\n", offset);
		return VM_FAULT_SIGBUS;
	}

	get_page(page);
	vmf->page = page;

	return 0;
}

static const struct vm_operations_struct memexpand_vm_ops = {
	.fault = memexpand_vm_fault,
};

static int memexpand_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct memexpand_device_data *dev_data = file->private_data;

	if (!dev_data || !dev_data->mmap_info) {
		pr_err("No memory allocated for mapping\n");
		return -EINVAL;
	}

	if ((vma->vm_end - vma->vm_start) > dev_data->mmap_info->size) {
		pr_err("Requested mapping too large\n");
		return -EINVAL;
	}

	if (vma->vm_pgoff != 0) {
		pr_err("Non-zero offset not supported\n");
		return -EINVAL;
	}

	vma->vm_ops = &memexpand_vm_ops;
	vma->vm_private_data = dev_data->mmap_info;
	vm_flags_set(vma, VM_DONTEXPAND);

	pr_info("Memory mapped with size %lu\n", dev_data->mmap_info->size);

	return 0;
}

static int my_open(struct inode *inode, struct file *file)
{
	file->private_data = &memexpand;
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	return 0;
}
