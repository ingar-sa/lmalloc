#include <linux/ioctl.h>

#define MY_DEVICE_MAGIC 'M'

#define MEMEXPAND_ALLOCATE _IOWR(MY_DEVICE_MAGIC, 1, struct memexpand_data)

struct memexpand_data {
	size_t size;
	unsigned long addr;
};

struct memexpand_mmap_info {
	void *data;
	size_t size;
};
