#include <src/lm.h>
LM_LOG_REGISTER(tests);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/allocators/allocator_wrappers.h>
#include <src/metrics/timing.h>

#include "test_setup.h"
#include "tests.h"
#include "network_test.h"
#include "tight_loop_test.h"

#include <stddef.h>
#include <sys/wait.h>

static void run_tight_loop_test(struct u_arena_test_params *u_arena_params,
				uint alloc_iterations, alloc_fn_t alloc_fn,
				const char *alloc_fn_name, size_t *alloc_sizes,
				size_t alloc_sizes_len, const char *size_name,
				const char *log_filename, const char *file_mode)
{
	UArena *a = NULL;
	if (u_arena_params)
		a = ua_create(u_arena_params->arena_sz,
			      u_arena_params->contiguous,
			      u_arena_params->mallocd,
			      u_arena_params->alignment);

	tight_loop_test(a, alloc_iterations, alloc_fn, alloc_fn_name,
			alloc_sizes, alloc_sizes_len, size_name, log_filename,
			file_mode);

	if (a)
		ua_destroy(&a);
}

static void
run_tight_loop_test_forked(struct u_arena_test_params *u_arena_params,
			   uint alloc_iterations, alloc_fn_t alloc_fn,
			   const char *alloc_fn_name, size_t *alloc_sizes,
			   size_t alloc_sizes_len, const char *size_name,
			   const char *log_filename, const char *file_mode)
{
	pid_t pid;
	int status;
	if ((pid = fork()) == 0) {
		run_tight_loop_test(u_arena_params, alloc_iterations, alloc_fn,
				    alloc_fn_name, alloc_sizes, alloc_sizes_len,
				    size_name, log_filename, file_mode);
		exit(EXIT_SUCCESS);
	} else {
		waitpid(pid, &status, 0);
	}
}

static void run_tight_loop_test_all_sizes(struct u_arena_test_params *params,
					  bool running_in_debugger,
					  uint iterations, alloc_fn_t alloc_fn,
					  const char *alloc_fn_name,
					  const char *log_filename,
					  const char *file_mode)
{
	if (iterations > 1000) {
		LmLogWarning(
			"%u iterations will allocate a lot of memory. Setting 'iterations'"
			" to 1000 instead (override must be disabled in the code)",
			iterations);
		iterations = 1000;
	}

	if (!running_in_debugger) {
		run_tight_loop_test_forked(params, iterations, alloc_fn,
					   alloc_fn_name, small_sizes,
					   LmArrayLen(small_sizes), "small",
					   log_filename, file_mode);

		run_tight_loop_test_forked(params, iterations, alloc_fn,
					   alloc_fn_name, medium_sizes,
					   LmArrayLen(medium_sizes), "medium",
					   log_filename, file_mode);

		run_tight_loop_test_forked(params, iterations, alloc_fn,
					   alloc_fn_name, large_sizes,
					   LmArrayLen(large_sizes), "large",
					   log_filename, file_mode);
	} else {
		run_tight_loop_test(params, iterations, alloc_fn, alloc_fn_name,
				    small_sizes, LmArrayLen(small_sizes),
				    "small", log_filename, file_mode);

		run_tight_loop_test(params, iterations, alloc_fn, alloc_fn_name,
				    medium_sizes, LmArrayLen(medium_sizes),
				    "medium", log_filename, file_mode);

		run_tight_loop_test(params, iterations, alloc_fn, alloc_fn_name,
				    large_sizes, LmArrayLen(large_sizes),
				    "large", log_filename, file_mode);
	}
}

static void run_network_test(struct u_arena_test_params *u_arena_params,
			     bool running_in_debugger, alloc_fn_t alloc_fn,
			     const char *alloc_fn_name, free_fn_t free_fn,
			     realloc_fn_t realloc_fn, uint iterations,
			     const char *log_filename, const char *file_mode)
{
	UArena *a = NULL;
	if (u_arena_params)
		a = ua_create(u_arena_params->arena_sz,
			      u_arena_params->contiguous,
			      u_arena_params->mallocd,
			      u_arena_params->alignment);

	if (!running_in_debugger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == 0) {
			network_test(a, alloc_fn, alloc_fn_name, free_fn,
				     realloc_fn, iterations, log_filename,
				     file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		network_test(a, alloc_fn, alloc_fn_name, free_fn, realloc_fn,
			     iterations, log_filename, file_mode);
	}

	if (a)
		ua_destroy(&a);
}

int u_arena_tests(struct u_arena_test_params *params)
{
#if 1
	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(ua_alloc_functions); ++i) {
		alloc_fn_t alloc_fn = ua_alloc_functions[i];
		const char *alloc_fn_name = ua_alloc_function_names[i];
		free_fn_t free_fn = ua_free_functions[0];
		realloc_fn_t realloc_fn = ua_realloc_functions[0];

#if 1
		run_tight_loop_test_all_sizes(params,
					      params->running_in_debugger,
					      params->alloc_iterations,
					      alloc_fn, alloc_fn_name,
					      params->log_filename, file_mode);
#endif

#if 0
		run_network_test(params, params->running_in_debugger, alloc_fn,
				 alloc_fn_name, free_fn, realloc_fn,
				 params->alloc_iterations, params->log_filename,
				 file_mode);
#endif
	}

#endif
	return 0;
}

int malloc_tests(struct malloc_test_params *params)
{
#if 1
	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(malloc_and_fam); ++i) {
		alloc_fn_t alloc_fn = malloc_and_fam[i];
		const char *alloc_fn_name = malloc_and_fam_names[i];
		free_fn_t free_fn = free_functions[0];
		realloc_fn_t realloc_fn = realloc_functions[0];

#if 1
		run_tight_loop_test_all_sizes(NULL, params->running_in_debugger,
					      params->alloc_iterations,
					      alloc_fn, alloc_fn_name,
					      params->log_filename, file_mode);
#endif

#if 0
		run_network_test(NULL, params->running_in_debugger, alloc_fn,
				 alloc_fn_name, free_fn, realloc_fn,
				 params->alloc_iterations, params->log_filename,
				 file_mode);
#endif
	}
#endif
	return 0;
}
