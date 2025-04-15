#include <src/lm.h>
LM_LOG_REGISTER(u_arena_test);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/metrics/timing.h>

#include "allocator_tests.h"

#include <stddef.h>
#include <sys/wait.h>

typedef void *(*u_arena_alloc_fn)(UArena *a, size_t sz);

static const int n_u_arena_alloc_fns = 4;

static const u_arena_alloc_fn u_arena_alloc_functions[] = {
	u_arena_alloc, u_arena_zalloc, u_arena_falloc, u_arena_fzalloc
};

#define ARENA_ALLOC_FN_COUNT LmArrayLen(u_arena_alloc_functions)

static const char *u_arena_alloc_function_names[] = { "alloc", "zalloc",
						      "falloc", "fzalloc" };

static void small_sizes_test(UArena *a, uint alloc_iterations,
			     bool running_in_debugger,
			     u_arena_alloc_fn alloc_fn,
			     const char *alloc_fn_name, LmString log_filename,
			     const char *file_mode)
{
	int small_sizes_arr_len = (int)LmArrayLen(small_sizes);
	size_t largest_sz = small_sizes[small_sizes_arr_len - 1];
	size_t mem_needed_for_largest_sz =
		largest_sz * (size_t)alloc_iterations;
	LmAssert(
		a->cap >= mem_needed_for_largest_sz,
		"Arena has insufficient memory for small sizes allocation test. Arena size: %zd, needed size: %zd",
		a->cap, mem_needed_for_largest_sz);
	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- small", alloc_fn_name);

	// TODO: (isa): Average for all small sizes
	for (int j = 0; j < small_sizes_arr_len; ++j) {
		LmLogDebugR("\n%s'ing %zd bytes %d times", alloc_fn_name,
			    small_sizes[j], alloc_iterations);

		LM_TIME_LOOP(individual_sizes, PROC_CPUTIME, uint, ss, 0,
			     alloc_iterations, <, ++,
			     uint8_t *ptr = alloc_fn(a, small_sizes[j]);
			     *ptr = 1;)

		LM_LOG_TIMING_AVG(individual_sizes, alloc_iterations,
				  "Avg: ", US, LM_LOG_RAW, DBG,
				  LM_LOG_MODULE_LOCAL);

		u_arena_free(a);
	}

	LmLogDebugR("\n%sing all sizes repeatedly %d times", alloc_fn_name,
		    alloc_iterations);

	// TODO: (isa): Determine if mod operation adds a lot of overhead
	LM_TIME_LOOP(all_sizes, PROC_CPUTIME, uint, ss, 0, alloc_iterations, <,
		     ++,
		     uint8_t *ptr = alloc_fn(
			     a, small_sizes[ss % LmArrayLen(small_sizes)]);
		     *ptr = 1;)

	LM_LOG_TIMING_AVG(all_sizes, alloc_iterations, "Avg: ", US, LM_LOG_RAW,
			  DBG, LM_LOG_MODULE_LOCAL);

	u_arena_free(a);
	lm_close_file(log_file);
	if (!running_in_debugger)
		exit(EXIT_SUCCESS);
}

int u_arena_tests(struct u_arena_test_params *params)
{
	const char *file_mode = "a";
	pid_t pid;
	int status;

	for (int i = 0; i < n_u_arena_alloc_fns; ++i) {
		if ((pid = fork()) == 0) {
			u_arena_alloc_fn alloc_fn = u_arena_alloc_functions[i];
			const char *alloc_fn_name =
				u_arena_alloc_function_names[i];

			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);

			small_sizes_test(a, params->alloc_iterations, false,
					 alloc_fn, alloc_fn_name,
					 params->log_filename, file_mode);
		} else {
			waitpid(pid, &status, 0);
		}
	}

	return 0;
}

int u_arena_tests_debug(struct u_arena_test_params *params)
{
	const char *file_mode = "a";

	for (int i = 0; i < (int)ARENA_ALLOC_FN_COUNT; ++i) {
		u_arena_alloc_fn alloc_fn = u_arena_alloc_functions[i];
		const char *alloc_fn_name = u_arena_alloc_function_names[i];

		UArena *a = u_arena_create(params->arena_sz, params->contiguous,
					   params->mallocd, params->alignment);

		small_sizes_test(a, params->alloc_iterations, true, alloc_fn,
				 alloc_fn_name, params->log_filename,
				 file_mode);

		u_arena_destroy(&a);
	}

	return 0;
}
