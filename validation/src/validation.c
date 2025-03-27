#define EIT_H_IMPLEMENTATION
#include <src/eit.h>
#undef EIT_H_IMPLEMENTATION

EIT_LOG_GLOBAL_REGISTER();
EIT_LOG_REGISTER(validation);

#include <src/u_arena/u_arena.h>

#include <stddef.h>
#include <assert.h>

void u_arena_test(void)
{
	EitLogDebug("Arena with non-contiguous memory test");
	EitLogDebug("Size of u_arena: %zd", sizeof(u_arena));

	u_arena *a = u_arena_create(4096 - 64, false, 16);

	ptrdiff_t a_to_a_mem_diff = EitPtrDiff(a->mem, a);
	EitLogDebug("Distance from arena to its memory: %zd", a_to_a_mem_diff);

	int *array = u_arena_array(a, int, 10);
	ptrdiff_t array_size = EitPtrDiff(u_arena_pos(a), array);
	bool array_is_aligned = EitIsAligned(array_size, a->alignment);

	EitLogDebug(
		"sizeof type: %zd, a: %p, a->mem: %p, array: %p, pos: %p, array size: %zd, aligned: %s",
		sizeof(*array), (void *)a, (void *)a->mem, (void *)array,
		(void *)u_arena_pos(a), array_size,
		EitBoolToString(array_is_aligned));
}

void u_arena_contiguous_test(void)
{
	EitLogDebug("Arena with contiguous memory test");

	u_arena *a = u_arena_create(4096 - 64, true, 16);

	ptrdiff_t a_to_a_mem_diff = EitPtrDiff(a->mem, a);
	EitLogDebug("Distance from arena to its memory: %zd", a_to_a_mem_diff);

	int *array = u_arena_array(a, int, 10);
	ptrdiff_t array_size = EitPtrDiff(u_arena_pos(a), array);
	bool array_is_aligned = EitIsAligned(array_size, a->alignment);

	EitLogDebug(
		"sizeof type: %zd, a: %p, a->mem: %p, array: %p, pos: %p, array size: %zd, aligned: %s",
		sizeof(*array), (void *)a, (void *)a->mem, (void *)array,
		(void *)u_arena_pos(a), array_size,
		EitBoolToString(array_is_aligned));
}

int main(void)
{
	u_arena_test();
	u_arena_contiguous_test();
	return 0;
}
