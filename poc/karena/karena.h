#include <linux/ioctl.h>

#define KARENA_MAGIC 'K'

#define KARENA_CREATE _IOWR(KARENA_MAGIC, 1, size_t)
#define KARENA_ALLOC _IOWR(KARENA_MAGIC, 2, struct karena_alloc)
#define KARENA_FREE _IOW(KARENA_MAGIC, 3, struct karena_alloc)

struct karena_alloc {
	size_t size;
	unsigned long addr;
};
