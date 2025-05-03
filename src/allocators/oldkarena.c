#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "oldkarena.h"

static int fd = 0;

void *oka_create(size_t size)
{
	void *memory;

	printf("oka create called\n");

	if (!fd) {
		fd = open("/dev/okarena", O_RDWR);
		if (fd < 0) {
			perror("Failed to open device");
			return 0;
		}
	}

	if (ioctl(fd, OKARENA_CREATE, &size) < 0) {
		perror("Memory allocation failed");
		close(fd);
		return 0;
	}

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

	return memory;
}

void *oka_alloc(void *arena, size_t size)
{
	struct oka_data alloc;
	alloc.addr = (unsigned long)arena;
	alloc.size = size;

	if (ioctl(fd, OKARENA_ALLOC, &alloc)) {
		perror("Arena allocation failed");
		return 0;
	}

	return (void *)alloc.addr;
}

void *oka_seek(void *arena, size_t pos)
{
	struct oka_data alloc;
	alloc.addr = (unsigned long)arena;
	alloc.size = pos;

	if (ioctl(fd, OKARENA_SEEK, &alloc)) {
		perror("Arena seek failed!");
		return NULL;
	}

	return (void *)alloc.addr;
}

void *oka_free(void *arena)
{
	return oka_seek(arena, 0);
}

void oka_pop(void *arena, size_t size)
{
	struct oka_data alloc = {
		.addr = (unsigned long)arena,
		.size = size,
	};

	if (ioctl(fd, OKARENA_POP, &alloc)) {
		perror("Pop failed");
	}
}

size_t oka_pos(void *arena)
{
	struct oka_data alloc = {
		.addr = (unsigned long)arena,
	};

	if (ioctl(fd, OKARENA_POS, &alloc)) {
		perror("Pos failed");
		return (size_t)-1;
	}
	return alloc.size;
}

size_t oka_reserve(void *arena, size_t sz)
{
	struct oka_data alloc = {
		.addr = (unsigned long)arena,
		.size = sz,
	};

	if (ioctl(fd, OKARENA_RESERVE, &alloc)) {
		perror("Reserve failed");
		return (size_t)-1;
	}

	return alloc.size;
}

size_t oka_size(void *arena)
{
	struct oka_data alloc = {
		.addr = (unsigned long)arena,
	};

	if (ioctl(fd, OKARENA_SIZE, &alloc)) {
		perror("Size failed");
		return (size_t)-1;
	}

	return alloc.size;
}

void oka_destroy(void *arena)
{
	struct oka_data alloc = {
		.addr = (unsigned long)arena,
	};

	size_t size = oka_size(arena);

	if (ioctl(fd, OKARENA_DESTROY, &alloc)) {
		perror("Destroy failed");
	}

	munmap(arena, size);
}
