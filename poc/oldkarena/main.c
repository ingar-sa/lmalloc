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

void *karena_seek(void *arena, size_t pos)
{
	struct karena_alloc alloc;
	alloc.addr = (unsigned long)arena;
	alloc.size = pos;
	if (ioctl(fd, KARENA_SEEK, &alloc)) {
		perror("Arena seek failed!");
		return NULL;
	}

	return (void *)alloc.addr;
}

void *karena_free(void *arena)
{
	return karena_seek(arena, 0);
}

void karena_pop(void *arena, size_t size)
{
	struct karena_alloc alloc = {
		.addr = (unsigned long)arena,
		.size = size,
	};

	if (ioctl(fd, KARENA_POP, &alloc)) {
		perror("Pop failed");
	}
}

size_t karena_pos(void *arena)
{
	struct karena_alloc alloc = {
		.addr = (unsigned long)arena,
		.size = -1,
	};

	if (ioctl(fd, KARENA_POS, &alloc)) {
		perror("Pos failed");
		return -1;
	}
	return alloc.size;
}

size_t karena_reserve(void *arena, size_t sz)
{
	struct karena_alloc alloc = {
		.addr = (unsigned long)arena,
		.size = sz,
	};

	if (ioctl(fd, KARENA_RESERVE, &alloc)) {
		perror("Reserve failed");
		return -1;
	}

	return alloc.size;
}

size_t karena_size(void *arena)
{
	struct karena_alloc alloc = {
		.addr = (unsigned long)arena,
	};

	if (ioctl(fd, KARENA_SIZE, &alloc)) {
		perror("Size failed");
		return -1;
	}

	return alloc.size;
}

void karena_destroy(void *arena)
{
	struct karena_alloc alloc = {
		.addr = (unsigned long)arena,
	};

	size_t size = karena_size(arena);

	if (ioctl(fd, KARENA_DESTROY, &alloc)) {
		perror("Destroy failed");
	}

	munmap(arena, size);
}

int main(void)
{
	void *arena = karena_create(1024);

	unsigned long *first = karena_alloc(arena, sizeof(unsigned long));
	unsigned long *second = karena_alloc(arena, sizeof(unsigned long));

	if (!first) {
		perror("first null pointer");
		return 1;
	}
	if (!second) {
		perror("second null pointer");
		return 1;
	}

	*first = 42L;
	*second = 44L;
	printf("First: %lu @ %p\n", *first, first);
	printf("Second: %lu @ %p\n", *second, second);

	printf("Current pos: %lu\n", karena_pos(arena));

	karena_pop(arena, sizeof(unsigned long));

	unsigned long *forth = karena_alloc(arena, sizeof(unsigned long));

	*forth = 12L;
	printf("Second: %lu @ %p\n", *second, second);
	printf("Forth: %lu @ %p\n", *forth, forth);

	printf("Trying to free %lx\n", (unsigned long)arena);

	fflush(stdout);

	karena_free(arena);

	if (!arena) {
		printf("Arena is null pointer after free\n");
		exit(1);
	}
	unsigned long *third = karena_alloc(arena, sizeof(unsigned long));
	*third = 69L;

	printf("Third: %lu @ %p\n", *third, third);

	karena_destroy(arena);

	// printf("Third: %lu @ %p\n", *third, third);

	return 0;
}
