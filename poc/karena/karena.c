static char *case_id = "";
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, case_id

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "karena.h"

MODULE_LICENSE("GPL");

#define DEVICE_NAME "karena"

struct karena_device_data {
	struct cdev cdev;
	struct mmap_info *mmap_info;
};

struct KArena {
	size_t size;
	size_t cur;
	unsigned long addr;
};

static struct KArena karenas[100];

struct thread_arenas {};

struct mmap_info {
	size_t size;
	unsigned long index;
	void *data;
};

static struct class *class;
static dev_t dev;

static struct karena_device_data karena;

static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
static long karena_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg);
static int karena_mmap(struct file *file, struct vm_area_struct *vma);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.unlocked_ioctl = karena_ioctl,
	.mmap = karena_mmap,
};

static int __init karena_init(void)
{
	if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0)
		return -ENOMEM;

	class = class_create(DEVICE_NAME);
	if (IS_ERR(class)) {
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(class);
	}

	if (IS_ERR(device_create(class, NULL, dev, NULL, DEVICE_NAME))) {
		class_destroy(class);
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	cdev_init(&karena.cdev, &fops);
	if (cdev_add(&karena.cdev, dev, 1) < 0) {
		device_destroy(class, dev);
		class_destroy(class);
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	karena.mmap_info = vzalloc(sizeof(struct mmap_info));

	pr_info("Device has been inserted!\n");

	return 0;
}

static void __exit karena_exit(void)
{
	cdev_del(&karena.cdev);
	device_destroy(class, dev);
	class_destroy(class);
	unregister_chrdev_region(dev, 1);
}

module_init(karena_init);
module_exit(karena_exit);

static unsigned int find_open_slot(void)
{
	struct KArena *info;
	unsigned int index = 0;

	info = karenas;
	while (info->addr != 0) {
		index++;
		info = &karenas[index];
		if (index > 99)
			return -1;
	}

	return index;
}

static long karena_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct KArena *info;
	struct ka_data alloc;
	unsigned int index = 0;

	if (copy_from_user(&alloc, (void __user *)arg, sizeof(alloc))) {
		pr_err("Could not copy data from user\n");
		return -EFAULT;
	}

	if (cmd != KARENA_CREATE) {
		info = &karenas[alloc.arena];
	}

	switch (cmd) {
	case KARENA_CREATE:
		case_id = "CREATE";
		pr_info("Creating arena\n");

		index = find_open_slot();
		pr_info("index: %u", index);

		if (index == -1) {
			pr_err("Did not find open slot");
			return -EFAULT;
		}

		info = &karenas[index];

		pr_info("Found slot for arena @ index %u\n", index);
		karena.mmap_info->index = index;
		alloc.arena = index;

		if (!info)
			return -ENOMEM;
		info->cur = 0;

		pr_info("Reqested arena size: %lu\n", alloc.size);

		info->size = PAGE_ALIGN(alloc.size);
		if (info->size == 0) {
			kfree(info);
			return -EINVAL;
		}
		pr_info("Aligned size: %lu\n", info->size);

		karena.mmap_info->data = vzalloc(info->size);
		if (!karena.mmap_info->data) {
			kfree(info);
			info = NULL;
			return -ENOMEM;
		}

		karena.mmap_info->size = info->size;

		alloc.size = info->size;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			vfree((void *)info->addr);
			kfree(info);
			karena.mmap_info = NULL;
			return -EFAULT;
		}

		break;
	case KARENA_ALLOC:
		case_id = "ALLOC";

		// pr_info("Arena %lu @ %lx, pos %lu\n", alloc.arena, info->addr,
		// 	info->cur);
		if (info->cur + alloc.size > info->size) {
			pr_err("Not enough space in arena");
			return -ENOMEM;
		}
		alloc.arena = info->cur + (unsigned long)info->addr;
		info->cur += alloc.size;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy alloc back to user\n");
			return -EFAULT;
		}

		break;
	case KARENA_SEEK:
		case_id = "SEEK";

		info->cur = alloc.size;
		alloc.arena = info->addr + info->cur;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy alloc back to user\n");
			return -EFAULT;
		}

		break;
	case KARENA_POP:
		case_id = "POP";

		if (info->cur < alloc.size) {
			pr_err("Pop too big\n");
			return -EFAULT;
		}

		info->cur -= alloc.size;

		break;
	case KARENA_POS:
		case_id = "POS";

		alloc.size = info->cur;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy pos back to user");
			return -EFAULT;
		}

		break;

	case KARENA_RESERVE:
		case_id = "RESERVE";

		if (info->size < info->cur + alloc.size) {
			alloc.size = info->size - info->cur;
		} else {
			info->cur += alloc.size;
		}

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy to user");
			return -EFAULT;
		}

		break;
	case KARENA_DESTROY:
		case_id = "DESTROY";

		pr_info();

		info->addr = 0;
		// vfree(((struct mmap_info *)vma->vm_private_data)->data);
		// kfree((struct mmap_info *)vma->vm_private_data);
		// vma->vm_private_data = NULL;

		break;

	case KARENA_SIZE:
		case_id = "SIZE";

		alloc.size = info->size;

		pr_info("Alloc size: %lu\n", alloc.size);

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy to user");
			return -EFAULT;
		}

		break;

	case KARENA_BOOTSTRAP:
		case_id = "BOOT";

		if (info->size - info->cur < alloc.size) {
			pr_err("Not enough space in backing arena");
			return -EFAULT;
		}

		unsigned long addr;

		addr = info->addr + info->cur;
		info->cur += alloc.size;

		index = find_open_slot();
		if (index == -1) {
			pr_err("Could not find open slot for arena");
			return -EFAULT;
		}

		pr_info("Found slot for arena @ index %u\n", index);

		karenas[index].addr = addr;
		karenas[index].cur = 0;
		karenas[index].size = alloc.size;

		alloc.arena = index;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy to user");
			return -EFAULT;
		}

		break;

	case KARENA_BASE:
		case_id = "BASE";

		alloc.arena = info->addr;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy to user");
			return -EFAULT;
		}

		break;

	default:
		case_id = "DEFAULT";
		pr_info("Unknown ioctl command: %u\n", cmd);
		return -ENOTTY;
	}

	return 0;
}

static vm_fault_t karena_vm_fault(struct vm_fault *vmf)
{
	might_sleep();

	struct vm_area_struct *vma = vmf->vma;
	struct mmap_info *info;
	unsigned long offset;
	struct page *page;

	info = vma->vm_private_data;
	if (!info) {
		pr_err("No mmap info in vma private data\n");
		return VM_FAULT_SIGBUS;
	}

	offset = vmf->pgoff << PAGE_SHIFT;
	if (offset >= info->size) {
		pr_err("Offset out of bounds: %lu >= %lu\n", offset,
		       info->size);
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

static const struct vm_operations_struct vm_ops = {
	.fault = karena_vm_fault,
};

static int karena_mmap(struct file *file, struct vm_area_struct *vma)
{
	case_id = "MMAP";

	if (!karena.mmap_info) {
		pr_err("No memory allocated for mapping\n");
		return -EINVAL;
	}

	if ((vma->vm_end - vma->vm_start) > karena.mmap_info->size) {
		pr_err("Requested mapping too large\n");
		return -EINVAL;
	}

	if (vma->vm_pgoff != 0) {
		pr_err("Non-zero offset not supported\n");
		return -EINVAL;
	}

	karenas[karena.mmap_info->index].addr = vma->vm_start;
	vma->vm_ops = &vm_ops;
	vma->vm_private_data = karena.mmap_info;
	vm_flags_set(vma, VM_DONTEXPAND);

	pr_info("Memory mapped @ %lx with size %lu\n", vma->vm_start,
		karena.mmap_info->size);

	return 0;
}

static int dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	return 0;
}
