#include <src/lm.h>
LM_LOG_REGISTER(u_arena_test);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/metrics/timing.h>

#include "allocator_tests.h"

#include <stddef.h>
#include <sys/wait.h>

static void small_zalloc(UArena *a, uint alloc_iterations,
			 bool running_in_debugger, LmString log_filename)
{
	FILE *log_file = lm_open_file_by_name(log_filename, "a");
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("Small zalloc");

	for (int j = 0; j < (int)LmArrayLen(small_sizes); ++j) {
		LmLogDebugR("\nAllocating %zd bytes %d times", small_sizes[j],
			    alloc_iterations);

		LM_TIME_LOOP(individual_sizes, PROC_CPUTIME, uint, ss, 0,
			     alloc_iterations, <, ++,
			     uint8_t *ptr = u_arena_zalloc(a, small_sizes[j]);
			     *ptr = 1;)

		LM_LOG_TIMING_AVG(individual_sizes, alloc_iterations,
				  "Timing of individual size", US, LM_LOG_RAW,
				  DBG, LM_LOG_MODULE_LOCAL);
		u_arena_free(a);
	}

	LmLogDebugR("\nAllocating all sizes repeatedly %d times",
		    alloc_iterations);
	LM_TIME_LOOP(all_sizes, PROC_CPUTIME, uint, ss, 0, alloc_iterations, <,
		     ++,
		     uint8_t *ptr = u_arena_zalloc(
			     a, small_sizes[ss % LmArrayLen(small_sizes)]);
		     *ptr = 1;)
	// TODO: (isa): Determine if mod operation adds a lot of overhead
	LM_LOG_TIMING_AVG(all_sizes, alloc_iterations,
			  "Timing of allocating all sizes repeatedly", US,
			  LM_LOG_RAW, DBG, LM_LOG_MODULE_LOCAL);

	lm_close_file(log_file);
	if (!running_in_debugger)
		exit(EXIT_SUCCESS);
}

static void small_alloc(UArena *a, uint alloc_iterations,
			bool running_in_debugger, LmString log_filename)
{
	FILE *log_file = lm_open_file_by_name(log_filename, "a");
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("Small alloc");

	for (int j = 0; j < (int)LmArrayLen(small_sizes); ++j) {
		LmLogDebugR("\nAllocating %zd bytes %d times", small_sizes[j],
			    alloc_iterations);

		LM_TIME_LOOP(individual_sizes, PROC_CPUTIME, uint, ss, 0,
			     alloc_iterations, <, ++,
			     uint8_t *ptr = u_arena_alloc(a, small_sizes[j]);
			     *ptr = 1;)

		LM_LOG_TIMING_AVG(individual_sizes, alloc_iterations,
				  "Timing of individual size", US, LM_LOG_RAW,
				  DBG, LM_LOG_MODULE_LOCAL);
		u_arena_free(a);
	}

	LmLogDebugR("\nAllocating all sizes repeatedly %d times",
		    alloc_iterations);
	LM_TIME_LOOP(all_sizes, PROC_CPUTIME, uint, ss, 0, alloc_iterations, <,
		     ++,
		     uint8_t *ptr = u_arena_alloc(
			     a, small_sizes[ss % LmArrayLen(small_sizes)]);
		     *ptr = 1;)
	// TODO: (isa): Determine if mod operation adds a lot of overhead
	LM_LOG_TIMING_AVG(all_sizes, alloc_iterations,
			  "Timing of allocating all sizes repeatedly", US,
			  LM_LOG_RAW, DBG, LM_LOG_MODULE_LOCAL);

	lm_close_file(log_file);
	if (!running_in_debugger)
		exit(EXIT_SUCCESS);
}

static void _medium_allocations(void)
{
}

static void large_allocations(void)
{
}

static void bulk_allocations(void)
{
}

int u_arena_tests(struct u_arena_test_params *params)
{
	LmString filename = lm_string_make("./logs/u_arena_tests_");
	filename = lm_string_append_fmt(filename, "%s_%s.txt",
					(params->mallocd ? "m" : "nm"),
					(params->contiguous ? "c" : "nc"));
	pid_t pid;
	int status;
	if ((pid = fork()) == 0) {
		UArena *a = u_arena_create(params->arena_sz, params->contiguous,
					   params->mallocd, params->alignment);

		small_alloc(a, params->alloc_iterations, false, filename);
	} else {
		waitpid(pid, &status, 0);
	}

	if ((pid = fork()) == 0) {
		UArena *a = u_arena_create(params->arena_sz, params->contiguous,
					   params->mallocd, params->alignment);

		small_zalloc(a, params->alloc_iterations, false, filename);
	} else {
		waitpid(pid, &status, 0);
	}

	return 0;
}

int u_arena_tests_debug(struct u_arena_test_params *params)
{
	LmString filename = lm_string_make("./logs/u_arena_tests_");
	filename = lm_string_append_fmt(filename, "%s_%s.txt",
					(params->mallocd ? "m" : "nm"),
					(params->contiguous ? "c" : "nc"));

	UArena *a = u_arena_create(params->arena_sz, params->contiguous,
				   params->mallocd, params->alignment);

	small_alloc(a, params->alloc_iterations, true, filename);

	u_arena_destroy(&a);

	a = u_arena_create(params->arena_sz, params->contiguous,
			   params->mallocd, params->alignment);

	small_zalloc(a, params->alloc_iterations, true, filename);

	return 0;
}
