#include <src/lm.h>
LM_LOG_REGISTER(tight_loop_test);

#include <src/allocators/allocator_wrappers.h>
#include <src/metrics/timing.h>

#include "tight_loop_test.h"
#include "tests.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/wait.h>

extern UArena *main_ua;

static void all_sizes_repeatedly(UArena *test_ua, uint64_t alloc_iterations,
				 alloc_fn_t alloc_fn, const char *alloc_fn_name,
				 size_t *alloc_sizes, size_t alloc_sizes_len,
				 const char *size_name,
				 const char *log_directory)
{
	LmLogInfoR("\n\n%s'ing all %s sizes repeatedly %lu times: \n",
		   alloc_fn_name, size_name, alloc_iterations);
	uint64_t total_iterations = alloc_iterations * alloc_sizes_len;
	size_t timing_vals_sz = total_iterations * sizeof(uint64_t);
	UArena *timings_ua = ua_create(timing_vals_sz, UA_CONTIGUOUS, UA_MMAPD,
				       sizeof(uint64_t));
	uint64_t *timing_arr =
		UaPushArray(timings_ua, uint64_t, total_iterations);
	init_alloc_tcoll(total_iterations, timing_arr);
	struct alloc_tcoll *tcoll = get_alloc_tcoll();
	for (size_t i = 0; i < alloc_iterations; ++i) {
		for (uint j = 0; j < alloc_sizes_len; ++j) {
			uint8_t *ptr = alloc_fn(test_ua, NULL, alloc_sizes[j]);
			if (!!0 && LM_UNLIKELY(!ptr)) {
				LmLogDebug("Arena ran out of memory! Freeing");
				ua_free(test_ua);
				// Overwrite failed allocation timing
				tcoll->cur -= 1;
				ptr = alloc_fn(test_ua, NULL, alloc_sizes[j]);
			}
			*ptr = 1;
		}
	}

	if (test_ua)
		ua_free(test_ua);

	enum alloc_type atype = get_alloc_type(alloc_fn);
	struct alloc_tstats *tstats = get_alloc_tstats();
	lm_log_tsc_timing_avg(tstats->total_tsc, tstats->iter, "", NS, true,
			      INF, LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");

	UAScratch uas = ua_scratch_begin(main_ua);

	LmString data_dump_filename = lm_string_make(log_directory, uas.ua);
	lm_string_append_fmt(data_dump_filename, "%s-%s.bin",
			     alloct_string(atype), size_name);
	if (write_alloc_timing_data_to_file(data_dump_filename, atype) != 0) {
		LmLogError("Failed to write data to file %s",
			   data_dump_filename);
		return;
	}

	ua_scratch_release(uas);
	ua_destroy(&timings_ua);
}
// NOTE: (isa): If the memory is freed, then malloc will
// use just as little time as the arena for all allocations,
// likely due to it simply reusing the same slot in the free list.
// It could be interesting to have this as its own test.
static void each_size_by_itself(UArena *test_ua, uint64_t alloc_iterations,
				alloc_fn_t alloc_fn, const char *alloc_fn_name,
				size_t *alloc_sizes, size_t alloc_sizes_len,
				const char *size_name,
				const char *log_directory)
{
	LmLogInfoR("\n%s'ing each %s size %lu times\n", alloc_fn_name,
		   size_name, alloc_iterations);

	size_t timing_vals_sz = alloc_iterations * sizeof(uint64_t);
	UArena *timings_ua = ua_create(timing_vals_sz, UA_CONTIGUOUS, UA_MMAPD,
				       sizeof(uint64_t));
	uint64_t *timing_arr =
		UaPushArray(timings_ua, uint64_t, alloc_iterations);

	for (size_t j = 0; j < alloc_sizes_len; ++j) {
		LmLogInfoR("\n%zd bytes: \n", alloc_sizes[j]);

		struct alloc_tcoll *timings = get_alloc_tcoll();
		init_alloc_tcoll(alloc_iterations, timing_arr);

		for (uint64_t i = 0; i < alloc_iterations; ++i) {
			uint8_t *ptr = alloc_fn(test_ua,NULL, alloc_sizes[j]);
			if (!!0 && LM_UNLIKELY(!ptr)) {
				LmLogDebug("Arena ran out of memory! Freeing");
				ua_free(test_ua);
				timings->cur -=
					1; // Overwrite failed allocation timing
				ptr = alloc_fn(test_ua,NULL, alloc_sizes[j]);
			}
			*ptr = 1;
		}

		ua_free(test_ua);

		struct alloc_tstats *tstats = get_alloc_tstats();
		enum alloc_type atype = get_alloc_type(alloc_fn);
		lm_log_tsc_timing_avg(tstats->total_tsc, tstats->iter, "", NS,
				      true, INF, LM_LOG_MODULE_LOCAL);
		LmLogInfoR("\n");

		UAScratch uas = ua_scratch_begin(main_ua);
		LmString data_dump_filename =
			lm_string_make(log_directory, uas.ua);
		lm_string_append_fmt(data_dump_filename, "%s-%zdB.bin",
				     alloct_string(atype), alloc_sizes[j]);
		if (write_alloc_timing_data_to_file(data_dump_filename,
						    atype) != 0) {
			LmLogError("Failed to write data to file %s",
				   data_dump_filename);
			return;
		}
		ua_scratch_release(uas);
	}

	ua_destroy(&timings_ua);
}

void tight_loop_test(struct ua_params *ua_params, bool running_in_debugger,
		     uint64_t alloc_iterations, alloc_fn_t alloc_fn,
		     const char *alloc_fn_name, size_t *alloc_sizes,
		     size_t alloc_sizes_len, const char *size_name,
		     LmString log_filename, const char *file_mode,
		     const char *log_directory)
{
	if (ua_params) {
		size_t largest_sz = alloc_sizes[alloc_sizes_len - 1];
		size_t mem_needed_for_largest_sz =
			largest_sz * (size_t)alloc_iterations;
		LmAssert(
			ua_params->arena_sz >= mem_needed_for_largest_sz,
			"Arena has insufficient memory for %s sizes allocation test. Arena size: %zd, needed size: %zd",
			size_name, ua_params->arena_sz,
			mem_needed_for_largest_sz);
	}

	UAScratch uas = ua_scratch_begin(main_ua);
	if (!running_in_debugger) {
		LmLogInfo("Running tight loop tests in forked mode");

		pid_t pid;
		int status;
		if ((pid = fork()) == -1) {
			LmLogError("Fork failed: %s", strerror(errno));
		} else if (pid == 0) {
			LmLogDebugR(
				"\n--- Successfully forked for each size by itself test");
			FILE *log_file =
				lm_open_file_by_name(log_filename, file_mode);
			LmSetLogFileLocal(log_file);

			UArena *ua = NULL;
			if (ua_params)
				ua = ua_create(ua_params->arena_sz,
					       ua_params->contiguous,
					       ua_params->mallocd,
					       ua_params->alignment);

			LmLogInfoR("\n\n------------------------------\n");
			LmLogInfo("%s -- %s", alloc_fn_name, size_name);

			each_size_by_itself(ua, alloc_iterations, alloc_fn,
					    alloc_fn_name, alloc_sizes,
					    alloc_sizes_len, size_name,
					    log_directory);
			if (ua)
				ua_destroy(&ua);
			LmRemoveLogFileLocal();
			lm_close_file(log_file);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == -1) {
			LmLogError("Fork failed: %s", strerror(errno));
		} else if (pid == 0) {
			LmLogDebugR(
				"\nSuccessfully forked for each size repeatedly test");
			FILE *log_file =
				lm_open_file_by_name(log_filename, file_mode);
			LmSetLogFileLocal(log_file);

			UArena *ua = NULL;
			if (ua_params)
				ua = ua_create(ua_params->arena_sz,
					       ua_params->contiguous,
					       ua_params->mallocd,
					       ua_params->alignment);

			all_sizes_repeatedly(ua, alloc_iterations, alloc_fn,
					     alloc_fn_name, alloc_sizes,
					     alloc_sizes_len, size_name,
					     log_directory);
			if (ua)
				ua_destroy(&ua);
			LmRemoveLogFileLocal();
			lm_close_file(log_file);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
		LmSetLogFileLocal(log_file);
		static UArena *ua = NULL;
		if (!ua && ua_params)
			ua = ua_create(ua_params->arena_sz,
				       ua_params->contiguous,
				       ua_params->mallocd,
				       ua_params->alignment);

		LmLogInfoR("\n\n------------------------------\n");
		LmLogInfo("%s -- %s", alloc_fn_name, size_name);

		each_size_by_itself(ua, alloc_iterations, alloc_fn,
				    alloc_fn_name, alloc_sizes, alloc_sizes_len,
				    size_name, log_directory);

		all_sizes_repeatedly(ua, alloc_iterations, alloc_fn,
				     alloc_fn_name, alloc_sizes,
				     alloc_sizes_len, size_name, log_directory);
		if (ua)
			ua_destroy(&ua);

		LmRemoveLogFileLocal();
		lm_close_file(log_file);
	}

	ua_scratch_release(uas);
}
