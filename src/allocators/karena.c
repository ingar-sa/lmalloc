#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "karena.h"

static int fd = 0;

KA_THREAD_ARENAS_REGISTER(threadkas, 2);

KArena *ka_create(size_t size)
{
	struct ka_data alloc = {
		.size = size,
	};

	if (!fd) {
		fd = open("/dev/karena", O_RDWR);
		if (fd < 0) {
			perror("Failed to open device");
			return NULL;
		}
	}

	if (ioctl(fd, KARENA_CREATE, &alloc) < 0) {
		perror("Memory allocation failed");
		close(fd);
		return NULL;
	}

	// printf("Memory size allocated: %lu bytes\n", size);

	if (mmap(NULL, alloc.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) ==
	    MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return NULL;
	}

	return (KArena *)alloc.arena;
}

void *ka_alloc(KArena *arena, size_t size)
{
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
		.size = size,
	};

	if (ioctl(fd, KARENA_ALLOC, &alloc)) {
		perror("Arena allocation failed");
		return 0;
	}

	return (void *)alloc.arena;
}

void *ka_seek(KArena *arena, size_t pos)
{
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
		.size = pos,
	};

	if (ioctl(fd, KARENA_SEEK, &alloc)) {
		perror("Arena seek failed!");
		return NULL;
	}

	return (void *)alloc.arena;
}

void *ka_free(KArena *arena)
{
	return ka_seek(arena, 0);
}

void ka_pop(KArena *arena, size_t size)
{
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
		.size = size,
	};

	if (ioctl(fd, KARENA_POP, &alloc)) {
		perror("Pop failed");
	}
}

size_t ka_pos(KArena *arena)
{
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
		.size = 0,
	};

	if (ioctl(fd, KARENA_POS, &alloc)) {
		perror("Pos failed");
		return (size_t)-1;
	}
	return alloc.size;
}

size_t ka_reserve(KArena *arena, size_t sz)
{
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
		.size = sz,
	};

	if (ioctl(fd, KARENA_RESERVE, &alloc)) {
		perror("Reserve failed");
		return (size_t)-1;
	}

	return alloc.size;
}

size_t ka_size(KArena *arena)
{
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
	};

	if (ioctl(fd, KARENA_SIZE, &alloc)) {
		perror("Size failed");
		return (size_t)-1;
	}

	return alloc.size;
}

void ka_destroy(KArena *arena)
{
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
	};

	size_t size = ka_size(arena);

	if (ioctl(fd, KARENA_DESTROY, &alloc)) {
		perror("Destroy failed");
	}

	munmap((void *)arena, size);
}

KArena *ka_bootstrap(KArena *arena, size_t size)
{
	KArena *ka;
	struct ka_data alloc = {
		.arena = (unsigned long)arena,
		.size = size,
	};

	if (ioctl(fd, KARENA_BOOTSTRAP, &alloc)) {
		perror("Destroy failed");
		return NULL;
	}

	return (KArena *)alloc.arena;
}

void ka__thread_arenas_init__(KArena *ta_buf[], struct ka__thread_arenas__ *tas,
			      struct ka__thread_arenas__ **ta_instance)
{
	tas->kas = ta_buf;
	*ta_instance = tas;
}

void ka__thread_arenas_init_extern__(struct ka__thread_arenas__ *tas,
				     struct ka__thread_arenas__ **ta_instance)
{
	*ta_instance = tas;
}

int ka__thread_arenas_add__(KArena *a, struct ka__thread_arenas__ *ta_instance)
{
	if (ta_instance->count + 1 <= ta_instance->max_count) {
		ta_instance->kas[ta_instance->count++] = a;
		return 0;
	}

	return 1;
}

KAScratch ka_scratch_begin(KArena *ka)
{
	KAScratch kas = { 0 };
	if (ka) {
		kas.ka = ka;
		kas.f5 = ka_pos(ka);
	}
	return kas;
}

KAScratch ka__scratch_get__(KArena **conflicts, int conflict_count,
			    struct ka__thread_arenas__ *tas)
{
	KArena *ka = NULL;
	if (conflict_count > 0) {
		for (int i = 0; i < tas->count; ++i) {
			for (int j = 0; j < conflict_count; ++j) {
				if (tas->kas[i] != conflicts[j]) {
					ka = tas->kas[i];
					goto exit;
				}
			}
		}
	} else {
		ka = tas->kas[0];
	}
exit:
	return (ka == NULL) ? (KAScratch){ 0 } : ka_scratch_begin(ka);
}
