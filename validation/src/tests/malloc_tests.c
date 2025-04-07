#include <src/lm.h>
LM_LOG_REGISTER(malloc_test);

#include <src/metrics/timing.h>

#include "allocator_tests.h"

#include <stdlib.h>
#include <sys/wait.h>

static void small_calloc(LmString log_filename)
{
	FILE *log_file = lm_open_file_by_name(log_filename, "a");
	LmSetLogFileLocal(log_file);
	LmLogDebugR("\n------------------------------");
	LmLogDebug("Small calloc");

	const int iterations = 10000;
	for (int j = 0; j < (int)LmArrayLen(small_sizes); ++j) {
		LmLogDebugR("\nAllocating %zd bytes %d times", small_sizes[j],
			    iterations);

		LM_TIME_LOOP(individual_sizes, PROC_CPUTIME, int, ss, 0,
			     iterations, <, ++,
			     uint8_t *ptr = calloc(1, small_sizes[j]);
			     *ptr = 1;)

		LM_LOG_TIMING_AVG(individual_sizes, iterations,
				  "Timing of individual size", US, LM_LOG_RAW,
				  DBG, LM_LOG_MODULE_LOCAL);
	}

	LmLogDebugR("\nAllocating all sizes repeatedly %d times", iterations);
	LM_TIME_LOOP(all_sizes, PROC_CPUTIME, int, ss, 0, iterations, <, ++,
		     uint8_t *ptr = calloc(
			     1, small_sizes[ss % LmArrayLen(small_sizes)]);
		     *ptr = 1;)
	LM_LOG_TIMING_AVG(all_sizes, iterations,
			  "Timing of allocating all sizes repeatedly", US,
			  LM_LOG_RAW, DBG, LM_LOG_MODULE_LOCAL);

	lm_close_file(log_file);
	exit(EXIT_SUCCESS);
}

static void small_malloc(LmString log_filename)
{
	FILE *log_file = lm_open_file_by_name(log_filename, "a");
	LmSetLogFileLocal(log_file);

	LmLogDebug("Small malloc");

	const int iterations = 10000;
	for (int j = 0; j < (int)LmArrayLen(small_sizes); ++j) {
		LmLogDebugR("\nAllocating %zd bytes %d times", small_sizes[j],
			    iterations);

		LM_TIME_LOOP(individual_sizes, PROC_CPUTIME, int, ss, 0,
			     iterations, <, ++,
			     uint8_t *ptr = malloc(small_sizes[j]);
			     *ptr = 1;)

		LM_LOG_TIMING_AVG(individual_sizes, iterations,
				  "Timing of individual size", US, LM_LOG_RAW,
				  DBG, LM_LOG_MODULE_LOCAL);
	}

	LmLogDebugR("\nAllocating all sizes repeatedly %d times", iterations);
	LM_TIME_LOOP(all_sizes, PROC_CPUTIME, int, ss, 0, iterations, <, ++,
		     uint8_t *ptr =
			     malloc(small_sizes[ss % LmArrayLen(small_sizes)]);
		     *ptr = 1;)
	LM_LOG_TIMING_AVG(all_sizes, iterations,
			  "Timing of allocating all sizes repeatedly", US,
			  LM_LOG_RAW, DBG, LM_LOG_MODULE_LOCAL);

	lm_close_file(log_file);
	exit(EXIT_SUCCESS);
}

int malloc_tests(void)
{
	LmString filename = lm_string_make("./logs/malloc_test.txt");
	pid_t pid;
	int status;

	if ((pid = fork()) == 0) {
		small_malloc(filename);
	} else {
		waitpid(pid, &status, 0);
	}

	if ((pid = fork()) == 0) {
		small_calloc(filename);
	} else {
		waitpid(pid, &status, 0);
	}

	return 0;
}
