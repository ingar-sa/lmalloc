#include <src/lm.h>
LM_LOG_REGISTER(u_arena_test);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/metrics/timing.h>

#include "allocator_tests.h"

#include <stddef.h>
#include <sys/wait.h>

typedef void *(*u_arena_alloc_fn)(UArena *a, size_t sz);

typedef void *(*alloc_fn_t)(UArena *a, size_t sz);

inline void *malloc_wrapper(UArena *a, size_t sz)
{
	(void)a;
	return malloc(sz);
}

static const alloc_fn_t u_arena_alloc_functions[] = {
	u_arena_alloc, u_arena_zalloc, u_arena_falloc, u_arena_fzalloc
};

static const char *u_arena_alloc_function_names[] = { "alloc", "zalloc",
						      "falloc", "fzalloc" };

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

static void tight_loop_test(struct u_arena_test_params *params,
			    alloc_fn_t alloc_fn, const char *alloc_fn_name,
			    size_t *alloc_sizes, size_t alloc_sizes_len,
			    const char *size_name, const char *file_mode)
{
	uint alloc_iterations = params->alloc_iterations;
	size_t largest_sz = alloc_sizes[alloc_sizes_len - 1];
	size_t mem_needed_for_largest_sz =
		largest_sz * (size_t)params->alloc_iterations;
	LmAssert(
		params->arena_sz >= mem_needed_for_largest_sz,
		"Arena has insufficient memory for %s sizes allocation test. Arena size: %zd, needed size: %zd",
		size_name, params->arena_sz, mem_needed_for_largest_sz);

	FILE *log_file = lm_open_file_by_name(params->log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- %s", alloc_fn_name, size_name);

	UArena *a = u_arena_create(params->arena_sz, params->contiguous,
				   params->mallocd, params->alignment);

	// TODO: (isa): Average for all small sizes
	for (size_t j = 0; j < alloc_sizes_len; ++j) {
		LmLogDebugR("\n%s'ing %zd bytes %d times", alloc_fn_name,
			    alloc_sizes[j], alloc_iterations);

		TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
				uint8_t *ptr = alloc_fn(a, small_sizes[j]);
				*ptr = 1;);

		u_arena_free(a);
	}

	LmLogDebugR("\n%s'ing all sizes repeatedly %d times", alloc_fn_name,
		    alloc_iterations);

	TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
			size_t size = alloc_sizes[i % alloc_sizes_len];
			uint8_t *ptr = alloc_fn(a, size); *ptr = 1;);

	u_arena_destroy(&a);

	lm_close_file(log_file);
	if (!params->running_in_debugger)
		exit(EXIT_SUCCESS);
}

static void time_series_test(struct u_arena_test_params *params,
			     u_arena_alloc_fn alloc_fn,
			     const char *alloc_fn_name, const char *file_mode)
{
	(void)alloc_fn;
	FILE *log_file = lm_open_file_by_name(params->log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- time series test", alloc_fn_name);
#if 0
	UArena *a = u_arena_create(params->arena_sz, params->contiguous,
				   params->mallocd, params->alignment);

	TimeSeriesData ts_data;
	EventHeader ev_header;
	UserAccount usr_account;
	PersonRecord p_record;
	MessageBuffer msg_buf;
#endif
}

#define FORK_AND_WAIT(fn_call)                    \
	do {                                      \
		pid_t pid;                        \
		int status;                       \
		if ((pid = fork()) == 0) {        \
			fn_call;                  \
		} else {                          \
			waitpid(pid, &status, 0); \
		}                                 \
	} while (0)

static void tight_loop(struct u_arena_test_params *params,
		       u_arena_alloc_fn alloc_fn, const char *alloc_fn_name,
		       const char *file_mode)
{
	if (!params->running_in_debugger) {
		FORK_AND_WAIT(tight_loop_test(
			params, alloc_fn, alloc_fn_name, small_sizes,
			LmArrayLen(small_sizes), "small", file_mode));

		FORK_AND_WAIT(tight_loop_test(
			params, alloc_fn, alloc_fn_name, medium_sizes,
			LmArrayLen(medium_sizes), "medium", file_mode));

		FORK_AND_WAIT(tight_loop_test(
			params, alloc_fn, alloc_fn_name, large_sizes,
			LmArrayLen(large_sizes), "large", file_mode));
		LmLogDebug("Hi from munit!");
	} else {
#if 1
		tight_loop_test(params, alloc_fn, alloc_fn_name, small_sizes,
				LmArrayLen(small_sizes), "small", file_mode);
#endif

#if 1
		tight_loop_test(params, alloc_fn, alloc_fn_name, medium_sizes,
				LmArrayLen(medium_sizes), "medium", file_mode);
#endif

#if 1
		tight_loop_test(params, alloc_fn, alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large", file_mode);
#endif
		LmRemoveLogFileLocal();
		LmLogDebug("Hi from debugger!");
	}
}

int u_arena_tests(struct u_arena_test_params *params)
{
	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(u_arena_alloc_functions); ++i) {
		u_arena_alloc_fn alloc_fn = u_arena_alloc_functions[i];
		const char *alloc_fn_name = u_arena_alloc_function_names[i];

		tight_loop(params, alloc_fn, alloc_fn_name, file_mode);
	}

	return 0;
}

int u_arena_tests_debug(struct u_arena_test_params *params)
{
	(void)params;
	return 0;
}
