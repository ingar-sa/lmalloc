#include <src/lm.h>
LM_LOG_REGISTER(u_arena_test);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/metrics/timing.h>

#include "test_setup.h"

#include "tests.h"

#include <stddef.h>
#include <sys/wait.h>

typedef void *(*alloc_fn_t)(UArena *a, size_t sz);
typedef void (*free_fn_t)(UArena *a, void *ptr);

static const alloc_fn_t u_arena_alloc_functions[] = {
	u_arena_alloc, u_arena_zalloc, u_arena_falloc, u_arena_fzalloc
};
static const char *u_arena_alloc_function_names[] = { "alloc", "zalloc",
						      "falloc", "fzalloc" };

static inline void u_arena_free_wrapper(UArena *a, void *ptr)
{
	(void)ptr;
	u_arena_free(a);
}

static const free_fn_t u_arena_free_functions[] = { u_arena_free_wrapper };
static const char *u_arena_free_function_names[] = { "free" };

static inline void *malloc_wrapper(UArena *a, size_t sz)
{
	(void)a;
	return malloc(sz);
}

static inline void *calloc_wrapper(UArena *a, size_t sz)
{
	(void)a;
	return calloc(1, sz);
}

static const alloc_fn_t malloc_and_fam[] = { malloc_wrapper, calloc_wrapper };
static const char *malloc_and_fam_names[] = { "malloc", "calloc" };

static inline void free_wrapper(UArena *a, void *ptr)
{
	(void)a;
	free(ptr);
}

static const free_fn_t malloc_free_functions[] = { free_wrapper };
static const char *malloc_free_function_names[] = { "free" };

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

static void tight_loop_test(UArena *a, uint alloc_iterations,
			    alloc_fn_t alloc_fn, const char *alloc_fn_name,
			    size_t *alloc_sizes, size_t alloc_sizes_len,
			    const char *size_name, const char *log_filename,
			    const char *file_mode, bool running_in_debugger)
{
	if (a) {
		size_t largest_sz = alloc_sizes[alloc_sizes_len - 1];
		size_t mem_needed_for_largest_sz =
			largest_sz * (size_t)alloc_iterations;
		LmAssert(
			a->cap >= mem_needed_for_largest_sz,
			"Arena has insufficient memory for %s sizes allocation test. Arena size: %zd, needed size: %zd",
			size_name, a->cap, mem_needed_for_largest_sz);
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
				uint8_t *ptr = alloc_fn(a, small_sizes[j]);
				*ptr = 1;);

		if (a)
			u_arena_free(a);
	}

	LmLogDebugR("\n%s'ing all sizes repeatedly %d times", alloc_fn_name,
		    alloc_iterations);

	TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
			size_t size = alloc_sizes[i % alloc_sizes_len];
			uint8_t *ptr = alloc_fn(a, size); *ptr = 1;);

	LmRemoveLogFileLocal();
	lm_close_file(log_file);
	if (!running_in_debugger)
		exit(EXIT_SUCCESS);
}

#if 0
static void time_series_test(UArena *a, alloc_fn_t alloc_fn,
			     const char *alloc_fn_name, free_fn_t free_fn,
			     const char *free_fn_name, const char *log_filename,
			     const char *file_mode)
{
	(void)alloc_fn;
	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- time series test", alloc_fn_name);
	TimeSeriesData ts_data;
	EventHeader ev_header;
	UserAccount usr_account;
	PersonRecord p_record;
	MessageBuffer msg_buf;

	LmRemoveLogFileLocal();
	lm_close_file(log_file);
}
#endif

static void u_arena_tight_loop(struct u_arena_test_params *params,
			       alloc_fn_t alloc_fn, const char *alloc_fn_name,
			       const char *file_mode)
{
	if (!params->running_in_debugger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == 0) {
			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);
			tight_loop_test(a, params->alloc_iterations, alloc_fn,
					alloc_fn_name, small_sizes,
					LmArrayLen(small_sizes), "small",
					params->log_filename, file_mode,
					params->running_in_debugger);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);
			tight_loop_test(a, params->alloc_iterations, alloc_fn,
					alloc_fn_name, small_sizes,
					(sizeof(medium_sizes) /
					 sizeof(medium_sizes[0])),
					"medium", params->log_filename,
					file_mode, params->running_in_debugger);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);
			tight_loop_test(
				a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				(sizeof(large_sizes) / sizeof(large_sizes[0])),
				"large", params->log_filename, file_mode,
				params->running_in_debugger);
		} else {
			waitpid(pid, &status, 0);
		}

		LmLogDebug("Hi from munit!");
	} else {
#if 1
		UArena *a = u_arena_create(params->arena_sz, params->contiguous,
					   params->mallocd, params->alignment);
		tight_loop_test(a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode,
				params->running_in_debugger);
		u_arena_destroy(&a);

#endif
#if 1
		a = u_arena_create(params->arena_sz, params->contiguous,
				   params->mallocd, params->alignment);
		tight_loop_test(a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode,
				params->running_in_debugger);
		u_arena_destroy(&a);
#endif
#if 1
		a = u_arena_create(params->arena_sz, params->contiguous,
				   params->mallocd, params->alignment);
		tight_loop_test(a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode,
				params->running_in_debugger);
		u_arena_destroy(&a);
#endif
		LmLogDebug("Hi from debugger!");
	}
}

int u_arena_tests(struct u_arena_test_params *params)
{
	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(u_arena_alloc_functions); ++i) {
		alloc_fn_t alloc_fn = u_arena_alloc_functions[i];
		const char *alloc_fn_name = u_arena_alloc_function_names[i];

		u_arena_tight_loop(params, alloc_fn, alloc_fn_name, file_mode);
	}

	return 0;
}

static void malloc_tight_loop(struct malloc_test_params *params,
			      alloc_fn_t alloc_fn, const char *alloc_fn_name,
			      const char *file_mode)
{
	if (!params->running_in_debugger) {
		pid_t pid;
		int status;

		if ((pid = fork()) == 0) {
			tight_loop_test(
				((void *)0), params->alloc_iterations, alloc_fn,
				alloc_fn_name, small_sizes,
				(sizeof(small_sizes) / sizeof(small_sizes[0])),
				"small", params->log_filename, file_mode,
				params->running_in_debugger);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			tight_loop_test(((void *)0), params->alloc_iterations,
					alloc_fn, alloc_fn_name, medium_sizes,
					(sizeof(medium_sizes) /
					 sizeof(medium_sizes[0])),
					"medium", params->log_filename,
					file_mode, params->running_in_debugger);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			tight_loop_test(
				((void *)0), params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				(sizeof(large_sizes) / sizeof(large_sizes[0])),
				"large", params->log_filename, file_mode,
				params->running_in_debugger);
		} else {
			waitpid(pid, &status, 0);
		}

	} else {
		tight_loop_test(NULL, params->alloc_iterations, alloc_fn,
				alloc_fn_name, small_sizes,
				LmArrayLen(small_sizes), "small",
				params->log_filename, file_mode,
				params->running_in_debugger);

		tight_loop_test(NULL, params->alloc_iterations, alloc_fn,
				alloc_fn_name, medium_sizes,
				LmArrayLen(medium_sizes), "medium",
				params->log_filename, file_mode,
				params->running_in_debugger);

		tight_loop_test(NULL, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode,
				params->running_in_debugger);
	}
}

int malloc_tests(struct malloc_test_params *params)
{
	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(malloc_and_fam); ++i) {
		alloc_fn_t alloc_fn = malloc_and_fam[i];
		const char *alloc_fn_name = malloc_and_fam_names[i];

		malloc_tight_loop(params, alloc_fn, alloc_fn_name, file_mode);
	}

	return 0;
}
