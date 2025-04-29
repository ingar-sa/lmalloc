#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "karena.h"

static int fd = 0;

KArena karena_create(size_t size)
{
	struct ka_data alloc = {
		.size = size,
	};

	if (!fd) {
		fd = open("/dev/karena", O_RDWR);
		if (fd < 0) {
			perror("Failed to open device");
			return -1;
		}
	}

	if (ioctl(fd, KARENA_CREATE, &alloc) < 0) {
		perror("Memory allocation failed");
		close(fd);
		return -1;
	}

	// printf("Memory size allocated: %lu bytes\n", size);

	if (mmap(NULL, alloc.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) ==
	    MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return -1;
	}

	return alloc.arena;
}

void *karena_alloc(KArena arena, size_t size)
{
	struct ka_data alloc;
	alloc.arena = arena;
	alloc.size = size;

	if (ioctl(fd, KARENA_ALLOC, &alloc)) {
		perror("Arena allocation failed");
		return 0;
	}

	return (void *)alloc.arena;
}

void *karena_seek(KArena arena, size_t pos)
{
	struct ka_data alloc = {
		.arena = arena,
		.size = pos,
	};

	if (ioctl(fd, KARENA_SEEK, &alloc)) {
		perror("Arena seek failed!");
		return NULL;
	}

	return (void *)alloc.arena;
}

void *karena_free(KArena arena)
{
	return karena_seek(arena, 0);
}

void karena_pop(KArena arena, size_t size)
{
	struct ka_data alloc = {
		.arena = arena,
		.size = size,
	};

	if (ioctl(fd, KARENA_POP, &alloc)) {
		perror("Pop failed");
	}
}

size_t karena_pos(KArena arena)
{
	struct ka_data alloc = {
		.arena = arena,
		.size = 0,
	};

	if (ioctl(fd, KARENA_POS, &alloc)) {
		perror("Pos failed");
		return -1;
	}
	return alloc.size;
}

size_t karena_reserve(KArena arena, size_t sz)
{
	struct ka_data alloc = {
		.arena = arena,
		.size = sz,
	};

	if (ioctl(fd, KARENA_RESERVE, &alloc)) {
		perror("Reserve failed");
		return -1;
	}

	return alloc.size;
}

size_t karena_size(KArena arena)
{
	struct ka_data alloc = {
		.arena = arena,
	};

	if (ioctl(fd, KARENA_SIZE, &alloc)) {
		perror("Size failed");
		return -1;
	}

	return alloc.size;
}

void karena_destroy(KArena arena)
{
	struct ka_data alloc = {
		.arena = arena,
	};

	size_t size = karena_size(arena);

	if (ioctl(fd, KARENA_DESTROY, &alloc)) {
		perror("Destroy failed");
	}

	munmap((void *)arena, size);
}

KArena karena_bootstrap(KArena arena, size_t size)
{
	struct ka_data alloc = {
		.arena = arena,
		.size = size,
	};

	if (ioctl(fd, KARENA_BOOTSTRAP, &alloc)) {
		perror("Destroy failed");
		return -1;
	}

	return alloc.arena;
}

int main(void)
{
	KArena arena = karena_create(1024);
	if (arena == -1) {
		perror("Arena allocation failed");
		return 1;
	}
	printf("KArena: %lu\n", arena);
	fflush(stdout);

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

	// printf("Trying to free %lx\n", (unsigned long)arena);
	//
	// fflush(stdout);
	//
	// karena_free(arena);

	printf("Current pos: %lu\n", karena_pos(arena));

	KArena arena3 = karena_bootstrap(arena, 128);
	printf("KArena3: %lu\n", arena3);
	printf("Current pos: %lu\n", karena_pos(arena3));
	unsigned long *foo = karena_alloc(arena3, sizeof(unsigned long));
	printf("foo: %lu @ %p\n", *foo, foo);

	KArena arena2 = karena_create(2048);
	unsigned long *blah = karena_alloc(arena2, sizeof(unsigned long));
	*blah = 55;
	printf("blah from second arena: %lu\n", *blah);
	printf("KArena2: %lu\n", arena2);

	unsigned long *third = karena_alloc(arena, sizeof(unsigned long));
	*third = 69L;

	printf("Third: %lu @ %p\n", *third, third);

	karena_destroy(arena);
	karena_destroy(arena2);
	karena_destroy(arena3);

	// printf("Third: %lu @ %p\n", *third, third);

	printf("DONE!\n");
	return 0;
}
