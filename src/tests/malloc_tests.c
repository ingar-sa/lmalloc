#include <src/lm.h>
LM_LOG_REGISTER(malloc_test);

#include <src/metrics/timing.h>

#include "allocator_tests.h"

#include <stdlib.h>
#include <sys/wait.h>

static void calloc_test(uint alloc_iterations, bool running_in_debugger,
			LmString log_filename, const char *file_mode)
{
	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);
	LmLogDebugR("\n------------------------------");
	LmLogDebug("Small calloc");

	// TODO: (isa): Average for all small sizes
	for (int j = 0; j < (int)LmArrayLen(small_sizes); ++j) {
		LmLogDebugR("\nAllocating %zd bytes %d times", small_sizes[j],
			    alloc_iterations);

		LM_TIME_LOOP(individual_sizes, PROC_CPUTIME, uint, ss, 0,
			     alloc_iterations, <, ++,
			     uint8_t *ptr = calloc(1, small_sizes[j]);
			     *ptr = 1;)

		LM_LOG_TIMING_AVG(individual_sizes, alloc_iterations,
				  "Timing of individual size", US, LM_LOG_RAW,
				  DBG, LM_LOG_MODULE_LOCAL);
	}

	LmLogDebugR("\nAllocating all sizes repeatedly %d times",
		    alloc_iterations);
	LM_TIME_LOOP(all_sizes, PROC_CPUTIME, uint, ss, 0, alloc_iterations, <,
		     ++,
		     uint8_t *ptr = calloc(
			     1, small_sizes[ss % LmArrayLen(small_sizes)]);
		     *ptr = 1;)
	LM_LOG_TIMING_AVG(all_sizes, alloc_iterations,
			  "Timing of allocating all sizes repeatedly", US,
			  LM_LOG_RAW, DBG, LM_LOG_MODULE_LOCAL);

	lm_close_file(log_file);
	if (!running_in_debugger)
		exit(EXIT_SUCCESS);
}

static void malloc_test(uint alloc_iterations, bool running_in_debugger,
			LmString log_filename, const char *file_mode)
{
	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("Small malloc");

	// TODO: (isa): Average for all small sizes
	for (int j = 0; j < (int)LmArrayLen(small_sizes); ++j) {
		LmLogDebugR("\nAllocating %zd bytes %d times", small_sizes[j],
			    alloc_iterations);

		LM_TIME_LOOP(individual_sizes, PROC_CPUTIME, uint, ss, 0,
			     alloc_iterations, <, ++,
			     uint8_t *ptr = malloc(small_sizes[j]);
			     *ptr = 1;)

		LM_LOG_TIMING_AVG(individual_sizes, alloc_iterations,
				  "Timing of individual size", US, LM_LOG_RAW,
				  DBG, LM_LOG_MODULE_LOCAL);
	}

	LmLogDebugR("\nAllocating all sizes repeatedly %d times",
		    alloc_iterations);
	LM_TIME_LOOP(all_sizes, PROC_CPUTIME, uint, ss, 0, alloc_iterations, <,
		     ++,
		     uint8_t *ptr =
			     malloc(small_sizes[ss % LmArrayLen(small_sizes)]);
		     *ptr = 1;)
	LM_LOG_TIMING_AVG(all_sizes, alloc_iterations,
			  "Timing of allocating all sizes repeatedly", US,
			  LM_LOG_RAW, DBG, LM_LOG_MODULE_LOCAL);

	lm_close_file(log_file);
	if (!running_in_debugger)
		exit(EXIT_SUCCESS);
}

int malloc_tests(struct malloc_test_params *params)
{
	pid_t pid;
	int status;
	const char *file_mode = "a";

	if ((pid = fork()) == 0) {
		malloc_test(params->alloc_iterations, false,
			    params->log_filename, file_mode);
	} else {
		waitpid(pid, &status, 0);
	}

	if ((pid = fork()) == 0) {
		calloc_test(params->alloc_iterations, false,
			    params->log_filename, file_mode);
	} else {
		waitpid(pid, &status, 0);
	}

	return 0;
}

int malloc_tests_debug(struct malloc_test_params *params)
{
	const char *file_mode = "a";

	malloc_test(params->alloc_iterations, true, params->log_filename,
		    file_mode);
	calloc_test(params->alloc_iterations, true, params->log_filename,
		    file_mode);

	return 0;
}
