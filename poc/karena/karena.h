#include <linux/ioctl.h>

#define KARENA_MAGIC 'K'

#define KARENA_CREATE _IOWR(KARENA_MAGIC, 1, struct ka_data)
#define KARENA_ALLOC _IOWR(KARENA_MAGIC, 2, struct ka_data)
#define KARENA_SEEK _IOWR(KARENA_MAGIC, 4, struct ka_data)
#define KARENA_POP _IOWR(KARENA_MAGIC, 5, struct ka_data)
#define KARENA_POS _IOWR(KARENA_MAGIC, 6, struct ka_data)
#define KARENA_RESERVE _IOWR(KARENA_MAGIC, 7, struct ka_data)
#define KARENA_DESTROY _IOW(KARENA_MAGIC, 8, struct ka_data)
#define KARENA_SIZE _IOWR(KARENA_MAGIC, 9, struct ka_data)
#define KARENA_BOOTSTRAP _IOWR(KARENA_MAGIC, 10, struct ka_data)

typedef unsigned long KArena ;

struct ka_data {
	size_t size;
	unsigned long arena;
};
