#include <linux/ioctl.h>

#define OKARENA_MAGIC 'O'

#define OKARENA_CREATE _IOWR(OKARENA_MAGIC, 1, size_t)
#define OKARENA_ALLOC _IOWR(OKARENA_MAGIC, 2, struct oka_data)
#define OKARENA_SEEK _IOWR(OKARENA_MAGIC, 4, struct oka_data)
#define OKARENA_POP _IOWR(OKARENA_MAGIC, 5, struct oka_data)
#define OKARENA_POS _IOWR(OKARENA_MAGIC, 6, struct oka_data)
#define OKARENA_RESERVE _IOWR(OKARENA_MAGIC, 7, struct oka_data)
#define OKARENA_DESTROY _IOW(OKARENA_MAGIC, 8, struct oka_data)
#define OKARENA_SIZE _IOWR(OKARENA_MAGIC, 9, struct oka_data)

struct oka_data {
	size_t size;
	unsigned long addr;
};
