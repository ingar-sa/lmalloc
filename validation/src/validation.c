#define LM_H_IMPLEMENTATION
#include <src/lm.h>
#undef LM_H_IMPLEMENTATION

LM_LOG_GLOBAL_REGISTER();
LM_LOG_REGISTER(validation);

#include <src/metrics/timing.h>
#include <src/u_arena/u_arena.h>

#include <stddef.h>

void u_arena_test(void)
{
	LmLogDebug("Arena with non-contiguous memory test");
	LmLogDebug("Size of u_arena: %zd", sizeof(u_arena));

	long long start = lm_get_time_stamp(PROC_CPUTIME);
	u_arena *a = u_arena_create(4096, false, 16);
	long long end = lm_get_time_stamp(PROC_CPUTIME);
	lm_log_timing(start, end, "arena creation", NS, DBG,
		      LM_LOG_MODULE_LOCAL);
	lm_log_timing(start, end, "arena creation", US, DBG,
		      LM_LOG_MODULE_LOCAL);
	lm_log_timing(start, end, "arena creation", MS, DBG,
		      LM_LOG_MODULE_LOCAL);
	lm_log_timing(start, end, "arena creation", S, DBG,
		      LM_LOG_MODULE_LOCAL);

	ptrdiff_t a_to_a_mem_diff = LmPtrDiff(a->mem, a);
	LmLogDebug("Distance from arena to its memory: %zd", a_to_a_mem_diff);

	start = lm_get_time_stamp(PROC_CPUTIME);
	int *array = u_arena_array(a, int, 10);
	end = lm_get_time_stamp(PROC_CPUTIME);
	lm_log_timing(start, end, "int array allocation: ", NS, DBG,
		      LM_LOG_MODULE_LOCAL);

	ptrdiff_t array_size = LmPtrDiff(u_arena_pos(a), array);
	bool array_is_aligned = LmIsAligned(array_size, a->alignment);

	LmLogDebug(
		"sizeof type: %zd, a: %p, a->mem: %p, array: %p, pos: %p, array size: %zd, aligned: %s",
		sizeof(*array), (void *)a, (void *)a->mem, (void *)array,
		(void *)u_arena_pos(a), array_size,
		LmBoolToString(array_is_aligned));
}

void u_arena_contiguous_test(void)
{
	LmLogDebug("Arena with contiguous memory test");

	size_t l1_cache_line_size = lm_get_l1_cache_line_size();
	long long start = lm_get_time_stamp(PROC_CPUTIME);
	u_arena *a = u_arena_create(4096 - l1_cache_line_size, true, 16);
	long long end = lm_get_time_stamp(PROC_CPUTIME);
	lm_log_timing(start, end, "arena creation", NS, DBG,
		      LM_LOG_MODULE_LOCAL);

	ptrdiff_t a_to_a_mem_diff = LmPtrDiff(a->mem, a);
	LmLogDebug("Distance from arena to its memory: %zd", a_to_a_mem_diff);

	int *array = u_arena_array(a, int, 10);
	ptrdiff_t array_size = LmPtrDiff(u_arena_pos(a), array);
	bool array_is_aligned = LmIsAligned(array_size, a->alignment);

	LmLogDebug(
		"sizeof type: %zd, a: %p, a->mem: %p, array: %p, pos: %p, array size: %zd, aligned: %s",
		sizeof(*array), (void *)a, (void *)a->mem, (void *)array,
		(void *)u_arena_pos(a), array_size,
		LmBoolToString(array_is_aligned));
}

int main(void)
{
	u_arena_test();
	u_arena_contiguous_test();
	return 0;
}
