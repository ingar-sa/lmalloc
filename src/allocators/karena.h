#include <linux/ioctl.h>

#define KARENA_MAGIC 'K'

#define KARENA_CREATE _IOWR(KARENA_MAGIC, 1, size_t)
#define KARENA_ALLOC _IOW(KARENA_MAGIC, 2, struct karena_alloc)

void *karena_create(size_t size);
void *karena_alloc(void *arena, size_t size);

struct karena_alloc {
	size_t size;
	unsigned long addr;
};
