#include <src/lm.h>
LM_LOG_REGISTER(tight_loop_test);

#include <src/allocators/allocator_wrappers.h>
#include <src/metrics/timing.h>

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
#if 1
	if (ua) {
		size_t largest_sz = alloc_sizes[alloc_sizes_len - 1];
		size_t mem_needed_for_largest_sz =
			largest_sz * (size_t)alloc_iterations;
		LmAssert(
			ua->cap >= mem_needed_for_largest_sz,
			"Arena has insufficient memory for %s sizes allocation test. Arena size: %zd, needed size: %zd",
			size_name, ua->cap, mem_needed_for_largest_sz);
	}
#endif

	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- %s", alloc_fn_name, size_name);
#if 0
	for (size_t j = 0; j < alloc_sizes_len; ++j) {
		LmLogDebugR("\n%s'ing %zd bytes %d times", alloc_fn_name,
			    alloc_sizes[j], alloc_iterations);

		for (uint i = 0; i < alloc_iterations; ++i) {
			uint8_t *ptr = alloc_fn(ua, alloc_sizes[j]);
			*ptr = 1;
#if 0
                        // NOTE: (isa): If the ptr is freed, then malloc will 
                        // use just as little time as the arena for all allocations, 
                        // likely due to it simply reusing the same slot in the free list
			if (ua)
				ua_free(ua);
			else
				free(ptr);
#endif
		}

		if (ua)
			ua_free(ua);
		uint64_t alloc_timing = 0;
		uint64_t alloc_iter = 0;
		if (alloc_fn == ua_alloc_wrapper_timed) {
			alloc_timing = get_and_clear_ua_alloc_timing();
			alloc_iter = get_and_clear_ua_alloc_iterations();
		} else if (alloc_fn == ua_zalloc_wrapper_timed) {
			alloc_timing = get_and_clear_ua_zalloc_timing();
			alloc_iter = get_and_clear_ua_zalloc_iterations();
		} else if (alloc_fn == ua_falloc_wrapper_timed) {
			alloc_timing = get_and_clear_ua_falloc_timing();
			alloc_iter = get_and_clear_ua_falloc_iterations();
		} else if (alloc_fn == ua_fzalloc_wrapper_timed) {
			alloc_timing = get_and_clear_ua_fzalloc_timing();
			alloc_iter = get_and_clear_ua_fzalloc_iterations();
		} else if (alloc_fn == malloc_wrapper_timed) {
			alloc_timing = get_and_clear_malloc_timing();
			alloc_iter = get_and_clear_malloc_iterations();
		} else if (alloc_fn == calloc_wrapper_timed) {
			alloc_timing = get_and_clear_calloc_timing();
			alloc_iter = get_and_clear_calloc_iterations();
		}
		lm_log_tsc_timing_avg(alloc_timing, alloc_iter,
				      "Average time spent in alloc (TSC): ", NS,
				      false, DBG, LM_LOG_MODULE_LOCAL);
	}
#endif
	LmLogDebugR("\n%s'ing %s sizes %d times", alloc_fn_name, size_name,
		    alloc_iterations);
	for (size_t j = 0; j < alloc_iterations; ++j) {
		for (uint i = 0; i < alloc_sizes_len; ++i) {
			uint8_t *ptr = alloc_fn(ua, alloc_sizes[i]);
			*ptr = 1;
		}
	}

	if (ua)
		ua_free(ua);
	uint64_t alloc_timing = 0;
	uint64_t alloc_iter = 0;
	if (alloc_fn == ua_alloc_wrapper_timed) {
		alloc_timing = get_and_clear_ua_alloc_timing();
		alloc_iter = get_and_clear_ua_alloc_iterations();
	} else if (alloc_fn == ua_zalloc_wrapper_timed) {
		alloc_timing = get_and_clear_ua_zalloc_timing();
		alloc_iter = get_and_clear_ua_zalloc_iterations();
	} else if (alloc_fn == ua_falloc_wrapper_timed) {
		alloc_timing = get_and_clear_ua_falloc_timing();
		alloc_iter = get_and_clear_ua_falloc_iterations();
	} else if (alloc_fn == ua_fzalloc_wrapper_timed) {
		alloc_timing = get_and_clear_ua_fzalloc_timing();
		alloc_iter = get_and_clear_ua_fzalloc_iterations();
	} else if (alloc_fn == malloc_wrapper_timed) {
		alloc_timing = get_and_clear_malloc_timing();
		alloc_iter = get_and_clear_malloc_iterations();
	} else if (alloc_fn == calloc_wrapper_timed) {
		alloc_timing = get_and_clear_calloc_timing();
		alloc_iter = get_and_clear_calloc_iterations();
	}
	lm_log_tsc_timing_avg(alloc_timing, (long long)alloc_iter,
			      "Avg (TSC): ", NS, true, DBG,
			      LM_LOG_MODULE_LOCAL);
	//(long long)(alloc_iterations * alloc_sizes_len),

#if 0
	LmLogDebugR("\n%s'ing all sizes repeatedly %d times", alloc_fn_name,
		    alloc_iterations);

        
	TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
			size_t size = alloc_sizes[i % alloc_sizes_len];
			uint8_t *ptr = alloc_fn(ua, size); *ptr = 1;);
#endif

	LmRemoveLogFileLocal();
	lm_close_file(log_file);
}
