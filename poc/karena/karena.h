#include <linux/ioctl.h>

#define KARENA_MAGIC 'K'

#define KARENA_CREATE _IOWR(KARENA_MAGIC, 1, size_t)
#define KARENA_ALLOC _IOWR(KARENA_MAGIC, 2, struct karena_alloc)
#define KARENA_SEEK _IOWR(KARENA_MAGIC, 4, struct karena_alloc)
#define KARENA_POP _IOWR(KARENA_MAGIC, 5, struct karena_alloc)
#define KARENA_POS _IOWR(KARENA_MAGIC, 6, struct karena_alloc)
#define KARENA_RESERVE _IOWR(KARENA_MAGIC, 7, struct karena_alloc)
#define KARENA_DESTROY _IOW(KARENA_MAGIC, 8, struct karena_alloc)
#define KARENA_SIZE _IOWR(KARENA_MAGIC, 9, struct karena_alloc)

struct karena_alloc {
	size_t size;
	unsigned long addr;
};
