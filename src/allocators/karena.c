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
	struct ka_data alloc;
	alloc.arena = (unsigned long)arena;
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

	return (void *)alloc.arena;
}

void *karena_seek(void *arena, size_t pos)
{
	struct ka_data alloc;
	alloc.arena = (unsigned long)arena;
	alloc.size = pos;
	if (ioctl(fd, KARENA_SEEK, &alloc)) {
		perror("Arena seek failed!");
		return NULL;
	}

	return (void *)alloc.arena;
}
