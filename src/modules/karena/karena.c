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
#include "../../allocators/karena.h"

MODULE_LICENSE("GPL");

#define DEVICE_NAME "karena"

struct karena_device_data {
	struct cdev cdev;
	struct mutex lock;
	unsigned long current_arena_index;
};

struct KArena {
	size_t size;
	size_t cur;
	unsigned long uaddr;
	bool bootstrapped;
	void *kaddr;
	pid_t owner_pid;
};

static struct KArena karenas[100];

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

static unsigned int find_open_slot(void)
{
	mutex_lock(&karena.lock);
	struct KArena *info;
	unsigned int index = 0;

	info = karenas;
	while (info->uaddr != 0) {
		index++;
		info = &karenas[index];
		if (index > 99) {
			mutex_unlock(&karena.lock);
			return -1;
		}
	}

	info->uaddr = 1;
	mutex_unlock(&karena.lock);

	return index;
}

static long handle_arena_create(struct file *file, struct ka_data *alloc)
{
	pr_info("Creating arena\n");
	struct karena_device_data *dev_data = file->private_data;
	struct KArena *info;

	unsigned int index = find_open_slot();
	pr_info("Found slot for arena @ index %u\n", index);

	if (index == -1) {
		pr_err("Did not find open slot");
		return -EFAULT;
	}

	info = &karenas[index];
	if (!info)
		return -ENOMEM;

	info->uaddr = 0;
	info->bootstrapped = false;
	info->owner_pid = task_pid_nr(current);
	info->cur = 0;

	dev_data->current_arena_index = index;
	alloc->arena = index;

	pr_info("Reqested arena size: %lu\n", alloc->size);

	info->size = PAGE_ALIGN(alloc->size);
	if (info->size == 0) {
		kfree(info);
		return -EINVAL;
	}

	alloc->size = info->size;

	pr_info("Aligned size: %lu\n", info->size);

	info->kaddr = vmalloc(info->size);
	if (!info->kaddr) {
		kfree(info);
		info = NULL;
		return -ENOMEM;
	}

	return 0;
}

static long handle_arena_alloc(struct KArena *arena, struct ka_data *alloc)
{
	if (__builtin_expect(!!(arena->cur + alloc->size <= arena->size), 1)) {
		alloc->arena = arena->uaddr + arena->cur;
		arena->cur += alloc->size;
		return 0;
	}
	return -ENOMEM;

	// unsigned long index = alloc->arena;
	// unsigned long cur = arena->cur;

	// pr_info("Allocating %lu to arena %lu, at address %lx, base %lx, offset %lx\n",
	// 	alloc->size, index, alloc->arena, arena->uaddr, cur);
}

static long handle_arena_seek(struct KArena *arena, struct ka_data *alloc)
{
	arena->cur = alloc->size;
	alloc->arena = arena->uaddr + arena->cur;

	return 0;
}

static long handle_arena_pop(struct KArena *arena, struct ka_data *alloc)
{
	if (arena->cur < alloc->size) {
		pr_err("Pop too big\n");
		return -EFAULT;
	}

	arena->cur -= alloc->size;

	return 0;
}
static long handle_arena_pos(struct KArena *arena, struct ka_data *alloc)
{
	alloc->size = arena->cur;
	return 0;
}

static long handle_arena_reserve(struct KArena *arena, struct ka_data *alloc)
{
	if (arena->size < arena->cur + alloc->size) {
		alloc->size = arena->size - arena->cur;
	} else {
		arena->cur += alloc->size;
	}

	return 0;
}

static long handle_arena_destroy(struct KArena *arena, struct ka_data *alloc)
{
	pr_info("destroying arena %lu", alloc->arena);
	alloc->size = arena->size;
	arena->uaddr = 0;
	arena->cur = 0;
	arena->size = 0;
	arena->owner_pid = 0;
	if (arena->bootstrapped) {
		arena->bootstrapped = false;
		alloc->size = 0;
		return 0;
	}

	if (arena->kaddr)
		vfree(arena->kaddr);

	return 0;
}

static long handle_arena_size(struct KArena *arena, struct ka_data *alloc)
{
	alloc->size = arena->size;

	return 0;
}

static long handle_arena_bootstrap(struct KArena *arena, struct ka_data *alloc)
{
	unsigned int index;
	unsigned long addr;

	if (arena->size - arena->cur < alloc->size) {
		pr_err("Not enough space in backing arena");
		return -EFAULT;
	}

	addr = arena->uaddr + arena->cur;
	arena->cur += alloc->size;

	index = find_open_slot();
	if (index == -1) {
		pr_err("Could not find open slot for arena");
		return -EFAULT;
	}

	pr_info("Found slot for arena @ index %u, addr %lx, size %lx, made from arena %lu\n",
		index, addr, alloc->size, alloc->arena);

	karenas[index].uaddr = addr;
	karenas[index].cur = 0;
	karenas[index].size = alloc->size;
	karenas[index].bootstrapped = true;
	karenas[index].owner_pid = arena->owner_pid;
	karenas[index].kaddr = arena->kaddr;

	alloc->arena = index;
	return 0;
}

static long handle_arena_base(struct KArena *arena, struct ka_data *alloc)
{
	unsigned long index = alloc->arena;
	alloc->arena = arena->uaddr;
	pr_info("Base: %lx for arena %lu", alloc->arena, index);
	return 0;
}

static long karena_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct KArena *info;
	struct ka_data alloc;
	long ret;

	if (copy_from_user(&alloc, (void __user *)arg, sizeof(alloc))) {
		pr_err("Could not copy data from user\n");
		return -EFAULT;
	}

	if (cmd != KARENA_CREATE) {
		info = &karenas[alloc.arena];
	}

	switch (cmd) {
	case KARENA_CREATE:
		ret = handle_arena_create(file, &alloc);
		break;
	case KARENA_ALLOC:
		ret = handle_arena_alloc(info, &alloc);
		break;
	case KARENA_SEEK:
		ret = handle_arena_seek(info, &alloc);
		break;
	case KARENA_POP:
		ret = handle_arena_pop(info, &alloc);
		break;
	case KARENA_POS:
		ret = handle_arena_pos(info, &alloc);
		break;
	case KARENA_RESERVE:
		ret = handle_arena_reserve(info, &alloc);
		break;
	case KARENA_DESTROY:
		ret = handle_arena_destroy(info, &alloc);
		break;
	case KARENA_SIZE:
		ret = handle_arena_size(info, &alloc);
		break;
	case KARENA_BOOTSTRAP:
		ret = handle_arena_bootstrap(info, &alloc);
		break;
	case KARENA_BASE:
		ret = handle_arena_base(info, &alloc);
		break;
	default:
		ret = -ENOTTY;
		pr_info("Unknown ioctl command: %u\n", cmd);
	}

	if (ret)
		return ret;

	if (cmd != KARENA_POP && cmd != KARENA_DESTROY) {
		if (copy_to_user((void __user *)arg, &alloc, sizeof(alloc))) {
			pr_err("Could not copy to user");
			return -EFAULT;
		}
	}

	return 0;
}

static vm_fault_t karena_vm_fault(struct vm_fault *vmf)
{
	might_sleep();

	struct vm_area_struct *vma = vmf->vma;
	struct KArena *arena;
	unsigned long offset;
	struct page *page;

	arena = vma->vm_private_data;
	if (!arena) {
		pr_err("No arena in vma private data\n");
		return VM_FAULT_SIGBUS;
	}

	offset = vmf->pgoff << PAGE_SHIFT;
	if (offset >= arena->size) {
		pr_err("Offset out of bounds: %lu >= %lu\n", offset,
		       arena->size);
		return VM_FAULT_SIGBUS;
	}

	page = vmalloc_to_page(arena->kaddr + offset);
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
	struct karena_device_data *dev_data = file->private_data;
	unsigned long arena_index = dev_data->current_arena_index;
	struct KArena *arena = &karenas[arena_index];

	if (!arena || arena->uaddr != 0) {
		pr_err("Invalid arena or arena already mapped\n");
		return -EINVAL;
	}

	if ((vma->vm_end - vma->vm_start) > arena->size) {
		pr_err("Requested mapping too large\n");
		return -EINVAL;
	}

	if (vma->vm_pgoff != 0) {
		pr_err("Non-zero offset not supported\n");
		return -EINVAL;
	}

	arena->uaddr = vma->vm_start;

	vma->vm_ops = &vm_ops;
	vma->vm_private_data = arena;
	vm_flags_set(vma, VM_DONTEXPAND);

	pr_info("Memory mapped for arena %lu @ %lx with size %lu\n",
		arena_index, vma->vm_start, arena->size);

	return 0;
}

static int dev_open(struct inode *inode, struct file *file)
{
	struct karena_device_data *dev_data =
		container_of(inode->i_cdev, struct karena_device_data, cdev);
	file->private_data = dev_data;

	dev_data->current_arena_index = -1;
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	pr_info("release called");
	int i;
	pid_t pid = task_pid_nr(current);

	for (i = 0; i < 100; i++) {
		if (karenas[i].uaddr != 0 && karenas[i].owner_pid == pid) {
			pr_info("cleaning up arena %d", i);
			karenas[i].uaddr = 0;
			karenas[i].cur = 0;
			karenas[i].size = 0;
			karenas[i].owner_pid = 0;
			if (karenas[i].bootstrapped) {
				karenas[i].bootstrapped = false;
				return 0;
			}

			if (karenas[i].kaddr) {
				vfree(karenas[i].kaddr);
				karenas[i].kaddr = NULL;
			}
		}
	}
	return 0;
}
