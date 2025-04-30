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
		return -1;
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
		return -1;
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
		return -1;
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

int main(void)
{
	KaThreadArenasInit(threadkas);
	KArena *thread = ka_create(1024);
	KaThreadArenasAdd(thread);

	KArena *arena = ka_create(1024);
	if ((unsigned long)arena == -1) {
		perror("Arena allocation failed");
		fflush(stdout);
		return 1;
	}
	printf("KArena: %lu\n", (unsigned long)arena);
	fflush(stdout);

	unsigned long *first = ka_alloc(arena, sizeof(unsigned long));
	unsigned long *second = ka_alloc(arena, sizeof(unsigned long));

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

	printf("Current pos: %lu\n", ka_pos(arena));

	ka_pop(arena, sizeof(unsigned long));

	unsigned long *forth = ka_alloc(arena, sizeof(unsigned long));

	*forth = 12L;
	printf("Second: %lu @ %p\n", *second, second);
	printf("Forth: %lu @ %p\n", *forth, forth);

	// printf("Trying to free %lx\n", (unsigned long)arena);
	//
	// fflush(stdout);
	//
	// ka_free(arena);

	printf("Current pos: %lu\n", ka_pos(arena));

	KArena *arena3 = ka_bootstrap(arena, 128);
	printf("KArena3: %lu\n", (unsigned long)arena3);
	printf("Current pos: %lu\n", ka_pos(arena3));
	unsigned long *foo = ka_alloc(arena3, sizeof(unsigned long));
	printf("foo: %lu @ %p\n", *foo, foo);

	KArena *arena2 = ka_create(2048);
	unsigned long *blah = ka_alloc(arena2, sizeof(unsigned long));
	*blah = 55;
	printf("blah from second arena: %lu\n", *blah);
	printf("KArena2: %lu\n", (unsigned long)arena2);

	unsigned long *third = ka_alloc(arena, sizeof(unsigned long));
	*third = 69L;

	printf("Third: %lu @ %p\n", *third, third);

	ka_destroy(arena);
	ka_destroy(arena2);
	ka_destroy(arena3);

	// printf("Third: %lu @ %p\n", *third, third);

	printf("DONE!\n");
	return 0;
}
