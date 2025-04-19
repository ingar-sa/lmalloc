#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "karena.h"

static int fd = 0;

void *karena_create(size_t size)
{
	void *memory;

	if (!fd) {
		fd = open("/dev/karena", O_RDWR);
		if (fd < 0) {
			perror("Failed to open device");
			return 0;
		}
	}

	if (ioctl(fd, KARENA_CREATE, &size) < 0) {
		perror("Memory allocation failed");
		close(fd);
		return 0;
	}

	// printf("Memory size allocated: %lu bytes\n", size);

	memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (memory == MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return 0;
	}

	if (!memory) {
		perror("Memory is null pointer\n");
		close(fd);
		return 0;
	}

	// printf("Memory mapped at address: %p\n", memory);

	// close(fd);

	return memory;
}

void *karena_alloc(void *arena, size_t size)
{
	struct karena_alloc alloc;
	alloc.addr = (unsigned long)arena;
	alloc.size = size;

	// int fd;
	// fd = open("/dev/karena", O_RDWR);
	// if (fd < 0) {
	// 	perror("Failed to open device");
	// 	return 0;
	// }

	if (ioctl(fd, KARENA_ALLOC, &alloc)) {
		perror("Arena allocation failed");
		return 0;
	}

	return (void *)alloc.addr;
}

// int main(void)
// {
// 	int *memory = karena_create(1024);
//
// 	unsigned long *first = karena_alloc(memory, sizeof(unsigned long));
// 	unsigned long *second = karena_alloc(memory, sizeof(unsigned long));
//
// 	if (!first) {
// 		perror("first null pointer");
// 		return 1;
// 	}
// 	if (!second) {
// 		perror("second null pointer");
// 		return 1;
// 	}
//
// 	*first = 42L;
// 	*second = 44L;
// 	printf("First: %lu @ %p\n", *first, first);
// 	printf("Second: %lu @ %p\n", *second, second);
//
// 	return 0;
// }
