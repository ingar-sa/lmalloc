#include "arena_u/arena_u.h"

#include "arena_u/arena_u.c"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define BOOL_TO_STRING(bool) ((bool) ? "true" : "false")

#define IS_ALIGNED(val, alignment) ((val % alignment) == 0)

#define PTR_DIFF(a, b) (ptrdiff_t)((uintptr_t)a - (uintptr_t)b)

void arena_u_test(void)
{
	printf("\nArena with non-contiguous memory test\n");
	printf("Size of arena_u: %zd\n", sizeof(arena_u));

	arena_u *a = arena_u_create(4096 - 64, false, 16);

	ptrdiff_t a_to_a_mem_diff = PTR_DIFF(a->mem, a);
	printf("Distance from arena to its memory: %zd\n", a_to_a_mem_diff);

	int *array = arena_u_array(a, int, 10);
	ptrdiff_t array_size = PTR_DIFF(arena_u_pos(a), array);
	bool array_is_aligned = IS_ALIGNED(array_size, a->alignment);

	printf("sizeof type: %zd, a: %p, a->mem: %p, array: %p, pos: %p, array size: %zd, aligned: %s\n",
	       sizeof(*array), a, a->mem, array, arena_u_pos(a), array_size,
	       BOOL_TO_STRING(array_is_aligned));
}

void arena_u_contiguous_test(void)
{
	printf("\nArena with contiguous memory test\n");

	arena_u *a = arena_u_create(4096 - 64, true, 16);

	ptrdiff_t a_to_a_mem_diff = PTR_DIFF(a->mem, a);
	printf("Distance from arena to its memory: %zd\n", a_to_a_mem_diff);

	int *array = arena_u_array(a, int, 10);
	ptrdiff_t array_size = PTR_DIFF(arena_u_pos(a), array);
	bool array_is_aligned = IS_ALIGNED(array_size, a->alignment);

	printf("sizeof type: %zd, a: %p, a->mem: %p, array: %p, pos: %p, array size: %zd, aligned: %s\n",
	       sizeof(*array), a, a->mem, array, arena_u_pos(a), array_size,
	       BOOL_TO_STRING(array_is_aligned));
}

int main(void)
{
	arena_u_test();
	arena_u_contiguous_test();
	return 0;
}
