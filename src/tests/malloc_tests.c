#include <src/lm.h>
LM_LOG_REGISTER(malloc_test);

#include <src/metrics/timing.h>

#include "allocator_tests.h"

#include <stdlib.h>
#include <sys/wait.h>

static void small_calloc(LmString log_filename, uint alloc_iterations,
			 bool running_in_debugger)
{
	FILE *log_file = lm_open_file_by_name(log_filename, "a");
	LmSetLogFileLocal(log_file);
	LmLogDebugR("\n------------------------------");
	LmLogDebug("Small calloc");

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

static void small_malloc(LmString log_filename, uint alloc_iterations,
			 bool running_in_debugger)
{
	FILE *log_file = lm_open_file_by_name(log_filename, "w");
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("Small malloc");

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
	LmString filename = lm_string_make("./logs/malloc_test.txt");
	pid_t pid;
	int status;
	if ((pid = fork()) == 0) {
		small_malloc(filename, params->alloc_iterations, false);
	} else {
		waitpid(pid, &status, 0);
	}

	if ((pid = fork()) == 0) {
		small_calloc(filename, params->alloc_iterations, false);
	} else {
		waitpid(pid, &status, 0);
	}

	return 0;
}

int malloc_tests_debugger(void)
{
	LmString filename = lm_string_make("./logs/malloc_test.txt");
	small_malloc(filename, true);
	small_calloc(filename, true);

	return 0;
}
