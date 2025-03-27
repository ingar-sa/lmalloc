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
	lm_log_timing(start, end, "arena creation", US, DBG,
		      LM_LOG_MODULE_LOCAL);

	ptrdiff_t a_to_a_mem_diff = LmPtrDiff(a->mem, a);
	LmLogDebug("Distance from arena to its memory: %zd", a_to_a_mem_diff);

	start = lm_get_time_stamp(PROC_CPUTIME);
	int *array = u_arena_array(a, int, 10);
	end = lm_get_time_stamp(PROC_CPUTIME);
	lm_log_timing(start, end, "int array allocation: ", US, DBG,
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
	u_arena *a =
		u_arena_create(LmMebiByte(1) - l1_cache_line_size, true, 16);
	long long end = lm_get_time_stamp(PROC_CPUTIME);
	lm_log_timing(start, end, "arena creation", US, DBG,
		      LM_LOG_MODULE_LOCAL);

	ptrdiff_t a_to_a_mem_diff = LmPtrDiff(a->mem, a);
	LmLogDebug("Distance from arena to its memory: %zd", a_to_a_mem_diff);

//#define N_BYTES (734 * sizeof(int) + 1)
#define N_BYTES (LmKibiByte(125) + 425)

	start = lm_get_time_stamp(PROC_CPUTIME);
	uint8_t *array = u_arena_alloc_no_align(a, N_BYTES);
	end = lm_get_time_stamp(PROC_CPUTIME);
	lm_log_timing(start, end, "int array arena", US, DBG,
		      LM_LOG_MODULE_LOCAL);
	long long arena_time = end - start;

	start = lm_get_time_stamp(PROC_CPUTIME);
	uint8_t *array2 = malloc(N_BYTES);
	end = lm_get_time_stamp(PROC_CPUTIME);
	lm_log_timing(start, end, "int array malloc", US, DBG,
		      LM_LOG_MODULE_LOCAL);
	long long malloc_time = end - start;

	struct timing_comp tc;
	int res = lm_compare_timing(malloc_time, arena_time, &tc);
	lm_log_timing_comp(tc, res, DBG, LM_LOG_MODULE_LOCAL);

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
	//u_arena_test();
	u_arena_contiguous_test();
	return 0;
}
