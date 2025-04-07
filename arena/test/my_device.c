#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include "my_device.h"

MODULE_LICENSE("GPL");

struct my_device_data {
	struct cdev cdev;
	int value;
};

static struct my_device_data my_device;
static dev_t dev_num;
static struct class *my_class;

static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.unlocked_ioctl = my_ioctl,
};

static int __init my_init(void)
{
	if (alloc_chrdev_region(&dev_num, 0, 1, "my_device") < 0)
		return -ENOMEM;

	my_class = class_create("my_class");
	// my_class = class_create(THIS_MODULE, "my_class");
	if (IS_ERR(my_class)) {
		unregister_chrdev_region(dev_num, 1);
		return PTR_ERR(my_class);
	}

	if (IS_ERR(device_create(my_class, NULL, dev_num, NULL, "my_device"))) {
		class_destroy(my_class);
		unregister_chrdev_region(dev_num, 1);
		return -1;
	}

	cdev_init(&my_device.cdev, &my_fops);
	if (cdev_add(&my_device.cdev, dev_num, 1) < 0) {
		device_destroy(my_class, dev_num);
		class_destroy(my_class);
		unregister_chrdev_region(dev_num, 1);
		return -1;
	}

	my_device.value = 0;

	pr_info("Device has been inserted!\n");

	return 0;
}

static void __exit my_exit(void)
{
	cdev_del(&my_device.cdev);
	device_destroy(my_class, dev_num);
	class_destroy(my_class);
	unregister_chrdev_region(dev_num, 1);
}

module_init(my_init);
module_exit(my_exit);

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int value;
	struct my_data data;

	switch (cmd) {
	case IOCTL_GET_VALUE:
		if (copy_to_user((int __user *)arg, &my_device.value,
				 sizeof(int)))
			return -EFAULT;
		break;

	case IOCTL_SET_VALUE:
		if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
			return -EFAULT;
		my_device.value = value;
		break;

	case IOCTL_DO_OPERATION:
		my_device.value++;
		break;

	case IOCTL_EXCHANGE_DATA:
		if (copy_from_user(&data, (struct my_data __user *)arg,
				   sizeof(struct my_data)))
			return -EFAULT;

		data.result = data.input1 + data.input2;

		if (copy_to_user((struct my_data __user *)arg, &data,
				 sizeof(struct my_data)))
			return -EFAULT;
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}

static int my_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	return 0;
}
