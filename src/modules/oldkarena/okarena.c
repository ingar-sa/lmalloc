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

#include "../../allocators/oldkarena.h"

MODULE_LICENSE("GPL");

#define DEVICE_NAME "okarena"

struct karena_device_data {
	struct cdev cdev;
	struct mmap_info *mmap_info;
};

struct thread_arenas {};

struct mmap_info {
	size_t size;
	size_t cur;
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

static struct vm_area_struct *get_alloc_and_find_vma(struct oka_data *alloc,
						     unsigned long arg)
{
	struct vm_area_struct *vma;

	if (copy_from_user(alloc, (void __user *)arg,
			   sizeof(struct oka_data))) {
		pr_err("Could not copy alloc from user\n");
		return NULL;
	}

	vma = find_vma(current->mm, alloc->addr);
	if (!vma) {
		pr_err("No vma for the address %lu\n", alloc->addr);
		return NULL;
	}

	return vma;
}

static long karena_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	size_t size;
	struct mmap_info *info;
	struct oka_data alloc;
	struct vm_area_struct *vma = NULL;

	switch (cmd) {
	case OKARENA_CREATE:
		case_id = "CREATE";
		pr_info("Creating arena\n");
		info = kzalloc(sizeof(struct mmap_info), GFP_KERNEL);
		if (!info)
			return -ENOMEM;
		info->cur = 0;

		if (copy_from_user(&size, (void __user *)arg, sizeof(size))) {
			pr_err("Could not copy data from user\n");
			return -EFAULT;
		}
		pr_info("Reqested arena size: %lu\n", size);

		info->size = PAGE_ALIGN(size);
		if (info->size == 0) {
			kfree(info);
			return -EINVAL;
		}
		pr_info("Aligned size: %lu\n", info->size);

		info->data = vzalloc(info->size);
		if (!info->data) {
			kfree(info);
			info = NULL;
			return -ENOMEM;
		}

		pr_info("Memory allocated at: %p\n", info->data);

		karena.mmap_info = info;

		pr_info("Allocated kernel memory at %p, size: %lu\n",
			info->data, info->size);

		if (copy_to_user((void __user *)arg, &info->size,
				 sizeof(info->size))) {
			vfree(info->data);
			kfree(info);
			karena.mmap_info = NULL;
			return -EFAULT;
		}

		break;
	case OKARENA_ALLOC:
		case_id = "ALLOC";

		vma = get_alloc_and_find_vma(&alloc, arg);

		info = vma->vm_private_data;
		pr_info("cur at before: %lu\n", info->cur);
		alloc.addr = info->cur + alloc.addr;
		info->cur += alloc.size;
		vma->vm_private_data = info;

		pr_info("addr at: %lx\n", alloc.addr);
		pr_info("cur at after: %lu\n", info->cur);

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy alloc back to user\n");
			return -EFAULT;
		}

		break;
	case OKARENA_SEEK:
		case_id = "SEEK";
		vma = get_alloc_and_find_vma(&alloc, arg);

		((struct mmap_info *)vma->vm_private_data)->cur = alloc.size;
		alloc.addr += alloc.size;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy alloc back to user\n");
			return -EFAULT;
		}

		break;
	case OKARENA_POP:
		case_id = "POP";
		vma = get_alloc_and_find_vma(&alloc, arg);

		info = vma->vm_private_data;
		if (info->cur < alloc.size) {
			pr_err("Pop too big\n");
			return -EFAULT;
		}

		((struct mmap_info *)vma->vm_private_data)->cur -= alloc.size;

		break;
	case OKARENA_POS:
		case_id = "POS";
		vma = get_alloc_and_find_vma(&alloc, arg);

		alloc.size = ((struct mmap_info *)vma->vm_private_data)->cur;

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy pos back to user");
			return -EFAULT;
		}

		break;

	case OKARENA_RESERVE:
		case_id = "RESERVE";

		vma = get_alloc_and_find_vma(&alloc, arg);
		info = (struct mmap_info *)vma->vm_private_data;

		if (info->size < info->cur + alloc.size) {
			alloc.size = info->size - info->cur;
		} else {
			((struct mmap_info *)vma->vm_private_data)->cur +=
				alloc.size;
		}

		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy to user");
			return -EFAULT;
		}

		break;
	case OKARENA_DESTROY:
		case_id = "DESTROY";

		vma = get_alloc_and_find_vma(&alloc, arg);

		vfree(((struct mmap_info *)vma->vm_private_data)->data);
		kfree((struct mmap_info *)vma->vm_private_data);
		vma->vm_private_data = NULL;

		break;

	case OKARENA_SIZE:
		case_id = "SIZE";

		vma = get_alloc_and_find_vma(&alloc, arg);

		alloc.size = ((struct mmap_info *)vma->vm_private_data)->size;

		pr_info("Alloc size: %lu\n", alloc.size);

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

	vma->vm_ops = &vm_ops;
	vma->vm_private_data = karena.mmap_info;
	vm_flags_set(vma, VM_DONTEXPAND);

	pr_info("Memory mapped with size %lu\n", karena.mmap_info->size);

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
