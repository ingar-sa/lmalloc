#include <src/lm.h>
LM_LOG_REGISTER(tight_loop_test);

#include <src/allocators/allocator_wrappers.h>

#include "tests.h"
#include "tight_loop_test.h"

// WARN: (isa): If the arena functions suddenly take a lot of time,
// check if TIME_U_ARENA is set to 1 in the header. If so, set it to 0

#define TIME_TIGHT_LOOP(clock, iterations, op)                               \
	do {                                                                 \
		LM_START_TIMING(loop, clock);                                \
		for (uint i = 0; i < iterations; ++i) {                      \
			op                                                   \
		}                                                            \
		LM_END_TIMING(loop, clock);                                  \
		LM_LOG_TIMING_AVG(loop, iterations, "Avg: ", US, LM_LOG_RAW, \
				  DBG, LM_LOG_MODULE_LOCAL);                 \
	} while (0)

void tight_loop_test(UArena *ua, uint alloc_iterations, alloc_fn_t alloc_fn,
		     const char *alloc_fn_name, size_t *alloc_sizes,
		     size_t alloc_sizes_len, const char *size_name,
		     const char *log_filename, const char *file_mode)
{
	if (ua) {
		size_t largest_sz = alloc_sizes[alloc_sizes_len - 1];
		size_t mem_needed_for_largest_sz =
			largest_sz * (size_t)alloc_iterations;
		LmAssert(
			ua->cap >= mem_needed_for_largest_sz,
			"Arena has insufficient memory for %s sizes allocation test. Arena size: %zd, needed size: %zd",
			size_name, ua->cap, mem_needed_for_largest_sz);
	}

	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- %s", alloc_fn_name, size_name);

	// TODO: (isa): Average for all small sizes
	for (size_t j = 0; j < alloc_sizes_len; ++j) {
		LmLogDebugR("\n%s'ing %zd bytes %d times", alloc_fn_name,
			    alloc_sizes[j], alloc_iterations);

		TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
				uint8_t *ptr = alloc_fn(ua, small_sizes[j]);
				*ptr = 1;);

		if (ua)
			ua_free(ua);
	}

	LmLogDebugR("\n%s'ing all sizes repeatedly %d times", alloc_fn_name,
		    alloc_iterations);

	TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
			size_t size = alloc_sizes[i % alloc_sizes_len];
			uint8_t *ptr = alloc_fn(ua, size); *ptr = 1;);

	LmRemoveLogFileLocal();
	lm_close_file(log_file);
}
