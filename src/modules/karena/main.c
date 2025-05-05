#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "../../allocators/karena.h"


KA_THREAD_ARENAS_REGISTER(threadkas, 2);

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
	unsigned long *base = ka_base(arena);

	if (first == base)
		printf("first and base matches!\n");

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

	// ka_destroy(arena);
	// ka_destroy(arena2);
	// ka_destroy(arena3);

	// printf("Third: %lu @ %p\n", *third, third);

	printf("DONE!\n");
	return 0;
}
